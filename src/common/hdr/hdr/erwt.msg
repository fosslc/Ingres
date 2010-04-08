/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	erwt.msg -	Message file for class IT
**
** Description:
**	This file defines the messages for class IT.
**
** History:
**	Revision 6.2  89/07  wong
**	Added E_WT0020_MaxIL for JupBug #6367.
**
**	Revision 6.0  87/08/10  arthur
**	Initial revision.
**
**	25-jan-1991 (Joe)
**	    Added some errors for Windows 4GL.
**	28-jan-1991 (Joe)
**	    Added errors for dynamic calls.
**	30-apr-1991 (sandyd)
**	    Changed WT_CLASS from 49 to 210 to avoid collision with old ABF.
**	11/20/91 (dkh) - ER changes for internationalization.
**	5-feb-1993 (emerson)
**	    Added E_WT0057_INTERNAL_ERROR and E_WT0058_INVALID_IL_OPCODE.
**	10-apr-1993 (emerson)
**	    Modified E_WT0025_WAITFOR_NOTTOP to reflect the fact that
**	    WaitFor may now be issued from within a procedure.
**	    E_WT0026_WAITFOR_NOTFRM is probably now obsolete.
**	9-sep-1993 (emerson)
**	    Added E_WT005D - E_WT0065 for prochandles.
**	19-sep-1993 (emerson)
**	    Removed the line number and frame name from E_WT0024 and E_WT0025,
**	    since they are always preceded by E_WT0029_ERRLOCATION, which
**	    displays the line number and frame name.  Also corrected
**	    the wording of E_WT0024.
**	25-oct-1993 (emerson)
**	    Changed the wording of E_WT0028_BLOCKRETURN to correspond to
**	    various changes in the way E_WT0028 and E_WT0029 are printed.
**	    See IIITreturnEx in ilcall.c for details.
**	27-nov-1993 (emerson)
**	    Added E_WT0066_NULL_TO_NONNULL, to handle those cases where
**	    the interpreter detects this error rather than ADF.
**	    (It's equivalent to E_AD1012_NULL_TO_NONNULL).
**	30-nov-1993 (emerson)
**	    Removed the attribute name from the E_WT0031_NULLATTR message,
**	    since it's not available with the new IL_FASTATTR.
**	    Replaced E_WT0058_INVALID_IL_OPCODE by E_WT0067_INVALID_IL_OPCODE.
**	    (E_WT0067_INVALID_IL_OPCODE has a parameter for the opcode).
**	28-dec-1993 (emerson)
**	    Removed the method name from the E_WT003B_NULLMETHOD message,
**	    since it's not available with the new IL_FASTMETHOD.
**	    Added E_WT002F_NOPROC for IL_FASTMETHOD.  (It's similar to
**	    E_WT003A_NOPROC but displays the method number instead of name).
**	31-dec-1993 (emerson)
**	    Changed my mind:  I decided that the E_WT002F_NOPROC condition
**	    was so implausible an internal error that it wasn't worth
**	    the overhead of checking for it, so I removed the message.
**	13-mar-1994 (emerson)
**	    Added F_WT000B_local_procedure, F_WT000C_method,
**	    F_WT000D_dynamic_expression, F_WT000E_application,
**	    E_WT0068_CANNOT_INSTANTIATE,
**	    E_WT0069_TOO_MANY_VARS_COMPILE,
**	    E_WT006A_DYNDECL_COMPILE_FAIL, and
**	    E_WT006B_EXPR_TOO_COMPLEX_WIN.
**	26-apr-1996 (rogerwh)
**	    Added W_WT006C_CASE_NO_MATCH runtime warning for case statement.
**	10-feb-1998 (donc)
**	    Added generic External Object errors
**	7-dec-1999 (crido01) SIR 99619 
**	    Added E_WT006E_R4GLPC_NULL_APPSRV
**		  E_WT006F_R4GLPC_INVALID_APPSRV
**	19-mar-2003 (crido01) 
**	   Fix up grammar in 0070 error
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	
**/

# define	_WT_CLASS	210

/*
** This is the name of the command line arguments for the interpreter.
** These are used in an error message, and should be the same as
** what is documented to the user.
**
** They are currently used in the E_ITXXXX_duparg error.
*/
F_WT0000_executable	"executable"
F_WT0001_frame		"frame"
F_WT0002_database	"database"
F_WT0005_rtt		"runtime table"

/*
** These are the kinds of callable objects that exist for an application,
** either a frame or a procedure.  These are used in certain errors to
** tell the user what type of object has a problem.
**
** For example, an error might be something like:
**	Problem calling the frame X.
**
** The word 'frame' above would be whatever F_WT0003_frame is.
**
** Note these are separate from F_WT0001_frame, since that is the name
** of a command line argument, and not an object type.  I suppose in
** some cases they may be different.
*/
F_WT0003_frame			"frame"
F_WT0004_procedure		"procedure"
F_WT000B_local_procedure	"local procedure"
F_WT000C_method			"method"
F_WT000D_dynamic_expression	"dynamic expression"
F_WT000E_application		"application"


E_WT0000_comline
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

E_WT0001_flagbad
"An illegal flag '%0c' was specified for the 4GL interpreter."

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

E_WT0002_fail
"4GL interpreter internal error.\n
An error occurred in an internal routine of the 4GL interpreter.
Please report this internal problem to your technical support
representative."

/*%
** Explanation:
**	An error occurred in an internal routine of the 4GL interpreter.
**	There may, for example, have been an error in the process of
**	reading to or writing from the ABF system catalogs.
**	Since such an error should never occur, it should be
**	reported as a problem to your technical support
**	representative.
**
** System Status:
**
** User Action:
**	Report to INGRES technical support representative.
*/

E_WT0003_argbad
"A bad argument was passed to the internal routine '%0c'."

/*%
** Explanation:
**	An argument was found to be NULL or out-of-range in
**	the specified routine.
**	Since such an error should never occur, it should be
**	reported as a problem to your technical support
**	representative.
**
** System Status:
**
** User Action:
**	Report to INGRES technical support representative.
*/

E_WT0004_filopen
"The file '%0c' could not be opened."

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

E_WT0005_ilorder
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
**	Report to INGRES technical support representative.
*/

E_WT0006_ilmissing
"An intermediate language statement with operator '%0c' was
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

E_WT0007_eofil
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

E_WT0008_trfile
"The trace file '%0c' could not be opened."

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

E_WT0009_formname
"No form was specified for the frame '%0c'."

/*%
** Explanation:
**
** System Status:
**	The frame which has no form to display is exited.  Execution
**	continues with the frame/procedure which called it.
**
** User Action:
*/

E_WT000A_client
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

E_WT000B_fidbad
"An inconsistency exists in the system catalogs.  A
frame identifier does not have a corresponding object.  Please
report this problem to your technical support representative."

/*%
** Explanation:
**	This error occurs because of an inconsistency in the INGRES
**	system catalogs.  An 4GL frame/procedure is listed in one
**	catalog as having a particular identifying number, but no
**	object with that number exists in the object manager catalogs.
**
** System Status:
**	The called frame/procedure cannot be run.  Execution
**	continues within the calling frame/procedure.
**
** User Action:
**	Report to technical support representative.
*/

E_WT000C_cframe
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

E_WT000D_frdwr
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

E_WT000E_radbad
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

E_WT000F_frmdisp
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

E_WT0010_jmpbad
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

E_WT0011_expbad "4GL expression evaluation error.\n
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

E_WT0012_divzero
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

E_WT0013_numovf
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

E_WT0014_fltbad
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

E_WT0015_imgbad
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

E_WT0016_calbad
"An error occurred in calling the %0c '%1c'.\n
The value returned from the internal routine that tried to
get the IL for this object was '%2x'."

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
**	F_WT0000_executable
**	F_WT0001_frame
**	F_WT0002_database
*/
E_WT0017_duparg
"The command line contained a duplicate argument.
The '%0c' name was specified both with and without a flag."

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

E_WT0018_nomem
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

E_WT0019_sycall
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

E_WT001A_noobj
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
S_WT001B_Hit_Return_when_done	"Hit Return when done"
/*
**	The following message appears after a query has been
**	run which did not retrieve any rows.
**	NOTE: This message also appears in users' code produced
**	by the code generator.  Any translation of this message
**	must, therefore, be exactly the same as the translation
**	of message S_CG_0008___No_rows_retrieved__ (except for
**	the double quotes in that string constant).
*/
S_WT001C_No_rows_retrieved	"No rows retrieved"
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
S_WT001D_No_query_currently_ac	"No query currently active"
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
S_WT001E_No_more_rows	"No more rows"

E_WT001F_NULLINT
"An integer expression containing a NULL value was used in a '%0c' statement.
A NULL value is not legal in this context.  The expression should be changed
to be non-NULLable, or the ifnull function should be used.  The statement
will continue with a default value."

/*%
** Explanation:
**	Some statements that take integer expressions do not allow
**	NULL values.  For example, a table field reference can not
**	have a NULL value for the row number.  However, this error
**	is not detectable until run-time, since the expression may
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

E_WT0020_MaxIL	"4GL source-code exceeded internal limit.\n
The size of the internal code generated for a 4GL source-code file exceeded
the maximum allowed by %0d percent.  Decrease the size of the 4GL source-code by
extracting some functionality into separate 4GL procedures."

#define	ILE_MaxIL	E_WT0020_MaxIL

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

E_WT0021_CANTFINDCLASS
"The class '%0c' was not found.\n
The class named '%0c' was referenced in 4GL statement, but the class does
not exist in the class hierarchy for this application.  Check the use
of the class name '%0c'."

/*%
** Explanation:
**	The code reference a class name that is not know to 4GL.
**
** System Status:
**	The application will continue execution, but later statements
**	will get errors.
**
** User Action:
**	Make sure the class is defined in the application.
*/

E_WT0022_NULL_CURSOR	"NULL CursorObject variable used in %2c statement.\n
On line #%0d of %1c the variable used in the cursor statement %2c contained
the NULL value.  It must point to a valid CursorObject for this statement
to work."

/*%
** Parameters:
**	0	The line number this occurred on.
**
**	1	The name of the running object.
**
**	2	The name of the cursor statement being invoked.
**		Will be one of
**				OPEN
**				CLOSE
**				FETCH
**				DELETE
**				UPDATE
**
** Explanation:
**	The variable used in a statement for a cursor was NULL.
**	
** System Status:
**	The application will continue execution.
**
** User Action:
**	Make sure the variable contains a valid CursorObject being
**	executing one of the cursor statements.
*/

E_WT0023_CURSOR_ERROR	"Error during cursor statement %2c.\n
On line #%0d of %1c an error occurred while executing the cursor
statement %2c."

/*%
** Parameters:
**	0	The line number this occurred on.
**
**	1	The name of the running object.
**
**	2	The name of the cursor statement being invoked.
**		Will be one of
**				OPEN
**				CLOSE
**				FETCH
**				DELETE
**				UPDATE
**
** Explanation:
**	An error occurred while executing this cursor statement.
**	Accompanying errors should give a better notion of what is
**	wrong.
**	
** System Status:
**	The application will continue execution.
**
** User Action:
**	Correct the problems reported by the accompanying errors.
*/

E_WT0024_RUNPOPUP_NOTTOP	"Invalid invocation of a popup method.\n
A popup method was invoked on an executable object
that is not the currently executing frame or a procedure that it has called."

/*%
** Explanation:
**	The various popup methods can only be applied to the currently
**	executing frame or to a procedure that it has called.
**	That is, it must be applied to the topmost frame in the current
**	call stack or to a procedure above it.
**	
** System Status:
**	The popup will not be displayed, but the application will
**	continue executing.
**
** User Action:
**	Apply the popup method to CurFrame or CurProcedure.
*/

E_WT0025_WAITFOR_NOTTOP	"Invalid invocation of WaitFor.\n
The method WaitFor was invoked on an executable object
that is not the currently executing frame or a procedure that it has called."

/*%
** Explanation:
**	The WaitFor method can only be applied to the currently executing frame
**	or to a procedure that it has called.  That is, it must be applied to
**	the topmost frame in the current call stack or to a procedure above it.
**	Within a frame, you can issue CurFrame.WaitFor(...).  Within
**	a local procedure in a frame, you can issue CurFrame.WaitFor(...)
**	or CurProcedure.WaitFor(...) (they are equivalent).
**	Within a non-local procedure that was called from a frame
**	(directly or indirectly), you can issue CurProcedure.WaitFor(...)
**	to wait for an event to be sent to the calling frame.
**	
** System Status:
**	The application will not waitfor the event.
**
** User Action:
**	Only do CurFrame.WaitFor(...) or CurProcedure.WaitFor(...).
*/

E_WT0026_WAITFOR_NOTFRM	   "WaitFor can only be invoked on CurFrame.\n
On line #%0d of %1c the method WaitFor was invoked on an executable
object that is not a FrameExec. WaitFor can only be applied to CurFrame."

/*%
** Parameters:
**	0	The line number this occurred on.
**
**	1	The name of the running object.
**
** Explanation:
**	The waitfor method can only be applied to the variable
**	CurFrame.
**	
** System Status:
**	The application will not waitfor the event.
**
** User Action:
**	Only do CurFrame.WaitFor();
*/

E_WT0027_BLOCKEXIT	"Leaving the current event block due to error.\n"

/*%
** Explanation:
**     	The current event block will end and the display loop will be started.
**
** System Status:
**	The current event block is ended.
**
** User Action:
**	Fix the error that cause this exit to occur.
*/

E_WT0028_BLOCKRETURN	"Returning from %0c due to error.\n"

/*%
** Parameters:
**	0	The name of the frame or (local) procedure being returned from.
**
** Explanation:
**	Some kind of error has caused the current object to be closed down.
**	This message follows E_WT0029_ERRLOCATION whenever the error causes
**	a return from a frame or (local) procedure.
**
** System Status:
**	The currently executing object is closed down.
**
** User Action:
**	Correct the error causing this premature return.
*/

E_WT0029_ERRLOCATION	"Error at line %1d of %0c.\n"

/*%
** Parameters:
**	0	This is the name of the component currently executing.
**
**	1	This is the line number at which the error occurred.
**
** Explanation:
**	This error is printed before any other errors are dumped so
**	that the location of the error can be determined.
**
** System Status:
**	The code will continue executing.
**
** User Action:
**	Correct the other errors reported.
*/

E_WT0030_BADATTR	"Can't find attribute '%0c' for a variable reference.\n
The attribute '%0c' was used in a variable reference, but this attribute is not
defined for the type of the variable to which the attribute is being applied.
Recompile the code and check the spelling of the attribute name."

/*%
** Parameters:
**	1	This is the name of the attribute that couldn't be found.
**
** Explanation:
**	The 4GL code referenced an attribute that couldn't be found for the
**	class of the variable given.
**
** System Status:
**	The current event block or object will be left.
**
** User Action:
**	Recompile the code to see if the attribute name is defined for
**	the class of the variable.
*/

E_WT0031_NULLATTR	"Attempt to access an attribute of a NULL object.\n
An attempt was made to access an attribute of a variable whose value is NULL.
Make sure the variable is not NULL before doing any attribute access."

/*%
** Explanation:
**	An attribute access of a NULL object variable was done.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Make sure the object variable is not NULL before during the attribute
**	access.
*/

E_WT0032_NOTARRAY	"An array access was done on a non-array value.\n
An array access using the [] syntax was done on an object that is not an
array.  Recompile the code to correct this problem."

/*%
** Explanation:
**	An array access was done on a value that is not an array.  Normally
**	the compiler should catch this error, but somehow in this case
**	the code is incorrect.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Recompile the code with the error to correct the problem.
*/

E_WT0033_ROWRANGE	"Row %0d is out of range for an array access.\n
An attempt was made to access row %0d of an array.  The array does not
contain row %0d."

/*%
** Parameters:
**	1	The number of the row being accessed.
**
** Explanation:
**	An array access was done with a row that is out of range for
**	the array.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Make sure the row number is within the bounds of the array
**	before accessing the row.
*/

E_WT0034_NULLARRAY	"Attempt to access a NULL array.\n
An attempt was made to access an array variable whose value is NULL.
Make sure the variable is not NULL before doing any array access."

/*%
** Explanation:
**	An array access of a NULL array variable was done.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Make sure the array variable is not NULL before during any
**	access.
*/

E_WT0035_CAST_ROW	"Error during a cast on an array value.\n
An array whose class is 'array of %0c' is being cast to an array whose class
is 'array of %1c'.  The class '%0c' is not a subclass of '%1c', so the cast
will fail."

/*%
** Parameters:
**	1	The name of the class of the array that is being assigned
**		to an array.
**
**	2	The name of the class of the array variable being assigned to.
**
** Explanation:
**	An illegal array assignment is being done.  The classes of the
**	the values are not compatible.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Fix the statement with the cast so that the classes are
**	compatible.
*/

E_WT0036_CAST_ARRAY	"Non-array value being cast to an array.\n
A value of class '%0c' is being cast to an array.  Only array values can
be cast to arrays."

/*%
** Parameters:
**	1	The name of the class of the value being cast.
**
** Explanation:
**	An illegal array assignment is being done.  A non-array value
**	is being assigned to an array.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Fix the statement with the cast so that the value is an array value.
*/

E_WT0037_CAST_ERR	"Error during a cast.\n
A value of class '%0c' is being cast to a value of class '%1c'.
The class '%0c' is not a subclass of '%1c', so the cast will fail."

/*%
** Parameters:
**	1	The class of the value being cast.
**
**	2	The class the value is being cast to.
**
** Explanation:
**	An error occurred during a cast.  The class of the value is not
**	a subclass of the class it is being cast to.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Fix the statement with the cast so that the classes are
**	compatible.
*/

E_WT0059_ARRAY_ROW_ERROR
"Error in array row assignment.\n
An object of class '%1c' was assigned to a row of an array whose rows are
of class '%0c'.  Since '%1c' is not a subclass of '%0c', the assignment will
fail."

/*%
** Explanation:
**	Arrays are created with a given type.  An attempt was made to 
**	assign a row to an aray which was not a subclass of that array's
**	row type.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Either assign a row of the correct type, or create the array as
**	having a different type.
*/

E_WT0038_CAST_INTERNAL	"Internal error during a cast.\n
There is an internal error that occurred during a cast.  The object
or class is null.  Please report this to your Ingres Representative.
You can also try recompiling the code to see if the error will go away."

/*%
** Explanation:
**	An internal error occurred.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Report this to your Ingres representative.  Also, try recompiling
**	the code to see if the problem will go away.
*/

E_WT0039_BADMETHOD	"Can't find method '%0c' for a variable reference.\n
An attempt was made to invoke the method '%0c' on an object value of
class '%1c'.  The method can't be found for this class.  Check the spelling of 
the method name and recompile the code."

/*%
** Parameters:
**	1	This is the name of the method that couldn't be found.
**
**	2	This is the name of the class of the value the
**		method is being applied to.
**
** Explanation:
**	The 4GL code referenced a method that couldn't be found for the
**	class of the variable given.
**
** System Status:
**	The code will continue as if the method was a noop.
**
** User Action:
**	Recompile the code to see if the method name is defined for
**	the class of the variable.
*/

E_WT003A_NOPROC		"Internal Error: No procedure for method '%0c'.\n
An attempt was made to invoke the method '%0c' on an object value of
class '%1c'.  No procedure could be found for this method.  This is an
internal error and must be reported to your Ingres representative."

/*%
** Parameters:
**	1	This is the name of the method that couldn't be found.
**
**	2	This is the name of the class of the value the
**		method is being applied to.
**
** Explanation:
**	This is an internal error.
**
** System Status:
**	The code will continue as if the method was a noop.
**
** User Action:
**	Report this to your Ingres representative.
*/

E_WT003B_NULLMETHOD	"Attempt to invoke a method on a NULL object.\n
An attempt was made to invoke a method on a variable whose value is NULL.
Make sure the variable is not NULL before doing any method invocations."

/*%
** Explanation:
**	A method invocation was done on a NULL value.
**
** System Status:
**	The code will continue as if the method was a noop.
**
** User Action:
**	Make sure the object variable is not NULL before during the method
**	invocation.
*/

E_WT003C_NESTTFLD	"Attempt to invoke FIELD on a nested table field.\n
An attempt was made to invoke the FIELD function on a nested table field
reference.  This is not supported."

/*%
** Explanation:
**	An attempt was made to invoke the FIELD function on a
**	nested table field.  This is not supported.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Fix the statement.
*/

E_WT003D_BADCODE	"Internal Error: A bad index given in %0c.\n
This is an internal error.  Try recompiling the code to see if
the problem will go away and report this to your Ingres representative."

/*%
** Parameters:
**	1	The name of the routine being called.
**
** Explanation:
**	This is an internal error.
**
** System Status:
**	the system will continue.
**
** User Action:
**	Try recompiling the code and report this to your Ingres representative.
*/

E_WT003E_NOTTFLD	"Field attached to an array is not a table field.\n
The class of the field attached to a table field is a '%0c'.  It should be a
TableField.  Try recompiling the frame to get the error to go away."

/*%
** Parameters
**	1	The class of the field attached to a table field.
**
** Explanation:
**	Normally the field attached to an array is a table field.  In
**	this case, the field attached to an array was not a table field.
**	This may be an internal error.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Try recompiling the frame.
*/

E_WT003F_TFLDDISPLAY	"No table field found for display row access.\n
An array access referring to a displayed row (e.g. [] or [<>]) was done on
an array that is not displayed.  It maybe that a nested reference is being
done and the row is not displayed."

/*%
** Explanation:
**	An array access using the syntax [] or [<>] was done on an
**	array that is not currently displayed.  It may be that the
**	row has scrolled out of the display.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Correctly the access so that the row is displayed.
*/

E_WT0040_DISPRANGE	"Row number %0d is out of range of displayed rows.\n
An array access referring to a displayed row (e.g. [<>]) was done with
a row number that is not displayed."

/*%
** Parameters:
**	1	The number of the row being accessed.
**
** Explanation:
**	An array access using the syntax [<>] was done with a row that is
**	out of range for the displayed rows for the table field.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Make sure the row number is within the bounds of the table field
**	before accessing the row.
*/

E_WT0041_CURROW		"Table field does not have a current row.\n
An array access to the current row of a table field (e.g. []) was done
on a table field that does not have a current row."

/*%
** Explanation:
**	An array access using the syntax [] was done on a table field
**	that does not have a current row.  This usually means that the
**	table field does not have the focus.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Before using current row addressing, you should make sure the
**	table field has the focus.
*/

E_WT0042_NESTSUBFORM  "Attempt to get FIELD of a subform within a table field.\n
An attempt was made to invoke the FIELD function on a subform nested within
a table field reference.  This is not supported."

/*%
** Explanation:
**	An attempt was made to invoke the FIELD function on a
**	subform nested within a table field.  This is not supported.
**
** System Status:
**	The current event block or object will be closed.
**
** User Action:
**	Fix the statement.
*/

E_WT0043_NULLFLD	"Internal error: NULL field or result field in FIELD\n
A NULL internal variable was found.  Report this to your Ingres representative.
You should also try recompiling your code to see if the problem will go out."

/*%
** Explanation:
**	An internal error occurred.
**
** System Status:
**	Unknown.
**
** User Action:
**	Try recompiling the code and report the problem to your Ingres
**	representative.
*/

E_WT0044_UNDEFINEDOBJ "Object %0c is not defined for the current application.\n
An attempt was made to execute either a frame or procedure that is not
a component of the current application.  The component must either be
created, or if it's in another application already, that application
must be included with the current application."

/*%
** Explanation:
**	An attempt was made to execute a frame or procedure that could
**	not be found in the scope of the current application.  Note that
**	an application scope not only includes its own components, but
**	those of its included applications, as well.
**
** System Status:
**	The frame or procedure call will be ignored.
**
** User Action:
**	Either create the component, or else include an application
**	that already has one in the scope of the current application.
*/

E_WT0045_DYNCALL	"Object called using dynamic name can not be called.\n
A call statement which used a dynamic name had the name '%0c' 
in the variable.  The object '%0c' is not a callable object.  Only
procedures or frames can be called using dynamic names.  '%0c'
is a %1c.  Note, that built-in functions like sin and length can not
be called with dynamic names."

/*%
** Parameters:
**	1	This is the name of the object being called.
**
**	2	This is the kind of object it is.  This will be something
**		like variable or constant.
**
** Explanation:
**	The 4GL allows a dynamic name to be used as the name of
**	an object to call.  For example, CALLPROC :var says
**	to call the object named by the current value of the
**	the variable var.  In this case, the object that was found
**	is not an object that can be called.
**
** System Status:
**	The call statement will be ignored.
**
** User Action:
**	Correct the value of the variable to have the name of an
**	object to call.
*/

E_WT0046_NOTFRAME	"Object called using dynamic name is not a frame.\n
A call statement for a frame (CALLFRAME, OPENFRAME or GOTOFRAME) which used
a dynamic name had the name '%0c' in the variable.  The object '%0c' is not
a frame, so it can't be called with this statement.  If the object
is a procedure, you must use CALLPROC."

/*%
** Parameters:
**	1	This is the name of the object being called.
**
** Explanation:
**	The 4GL allows a dynamic name to be used as the name of
**	an object to call.  For example, CALLFRAME :var says
**	to call the object named by the current value of the
**	the variable var.  In this case, the object that was found
**	is not a frame so it can't be called with the given statement.
**
** System Status:
**	The call statement will be ignored.
**
** User Action:
**	If the object is a procedure, use the CALLPROC statement.
*/

E_WT0047_NOTPROC	"Object called using dynamic name is not a procedure.\n
A CALLPROC statement which used a dynamic name had the name '%0c'
in the variable.  The object '%0c' is not a procedure, so it can't
be called with the CALLPROC statement.  If the object is a frame,
you must use one of CALLFRAME, OPENFRAME or GOTOFRAME."

/*%
** Parameters:
**	1	This is the name of the object being called.
**
** Explanation:
**	The 4GL allows a dynamic name to be used as the name of
**	an object to call.  For example, CALLPROC :var says
**	to call the procedure named by the current value of the
**	the variable var.  In this case, the object that was found
**	is not a procedure so it can't be called with the given statement.
**
** System Status:
**	The call statement will be ignored.
**
** User Action:
**	If the object is a frame, use one of CALLFRAME, OPENFRAME, or
**	GOTOFRAME.
*/

E_WT0048_INTERRUPTED	"Current application interrupted.\n
The application has been interrupted - cleanup activity is being performed."

/*%
** Explanation:
**	Appears in the trace window to let the user know that their interrupt
**	has been "seen", and prevent them from issuing more interrupts.
**
** System Status:
**	Interrupt is being handled.
**
** User Action:
**	Wait for application interrupt to complete.
*/

E_WT0055_APP_ED_INTERRUPTED "The Application Editor has been interrupted.\n
Windows4GL will exit."

/*%
** Explanation:
**      Appears in the trace window when the Application Editor has been
**	interrupted.  This lets the user know that their interrupt has
**	been "seen", and prevent them from issuing more interrupts.
**
** System Status:
**      Interrupt is being handled. That is, the Application Editor is
**	shutting down, after which Windows4GL will exit.  All unsaved changes
**	will be lost.
** User Action:
**      Wait for Windows4GL to exit.
*/

E_WT0057_INTERNAL_ERROR
"An internal error has occurred in Windows/4GL in routine %0c.\n
Please notify your Site Technical Contact."

/*%
** Explanation:
**	The Windows/4GL interpreter has encountered a fatal internal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	Contact the Site Technical contact to report the error.
*/

E_WT0058_INVALID_IL_OPCODE
"The interpreter found an invalid intermediate language operation.\n
Please notify your Site Technical Contact.  Try recompiling the program."

/*%
** Explanation:
**	The Windows/4GL interpreter has encountered an invalid
**	intermediate language operation.
**
**	Note: this message has been superseded by E_WT0067_INVALID_IL_OPCODE.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	Try recompiling the program.  This may work around the problem.
**	In any case, contact the Site Technical contact to report the error.
*/

/*
**	ilcall.c
*/

S_WT0049_COULDNT_RESTART_4GL	"Couldn't restart 4GL interpreter.  Press ENTER"

E_WT005D_NO_DECLARER
"In attempting to call a local procedure,
the interpreter could not find the scope in which it was declared.\n
Please notify your Site Technical Contact.  Try recompiling the component."

/*%
** Explanation:
**	The Windows/4GL interpreter could not find the parent scope
**	for a local procedure that's being called.
**	This is an internal Windows/4GL error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	Try recompiling the component that's attempting to call
**	the local procedure, and the component that contains it.
**	(They'll be the same component unless you are invoking
**	the local procedure via a prochandle from another component).
**	This may work around the problem.
**	In any case, contact the Site Technical contact to report the error.
*/

E_WT005E_FRAME_IN_EXCLUSIVE	"The application tried to execute a frame
('%0c') while running a user routine (such as a handler) in exclusive mode.\n"

/*%
** Explanation:
**	The application tried to issue a CALLFRAME, GOTOFRAME or OPENFRAME,
**	while running a user-written routine in exclusive mode.
**	This could be either a procedure invoked via the 'runexclusive' method
**	on a prochandle, or else an error handler invoked by Windows4GL itself
**	(for example, a procedure whose prochandle has been placed into
**	the DataEntryErrorHandler attribute of the FrameExec for a frame
**	into which a user has just typed invalid data).
**
** System Status:
**	The CALLFRAME, GOTOFRAME or OPENFRAME statement will be skipped.
**
** User Action:
**	Change the application so that it doesn't invoke a frame
**	in exclusive mode.
*/

/*
**	ildisp.c
*/

S_WT004A_IL_BEGSUBMU_NOT_YET	"IL_BEGSUBMU is not yet implemented in Windows4GL.\n"

S_WT004B_IL_SUBMENU_FOUND	"IL_SUBMENU is not yet supported in Windows4GL.\n"

/*
**	ilerror.c
*/

S_WT004C_UNSUPPORTED_STATEMENT	"Unsupported statement\n\t"

S_WT004D_FOUND_NEED_RECOMP	"found in the intermediate code.  Need to recompile."

F_WT0006_UNKNOWN	"<Unknown>"

/*
** ilquery.c
*/
E_WT0056_BAD_SID
"An invalid operand for a SELECT statement was generated by the 4GL compiler.
Please notify your Site Technical Contact."

/*%
** Explanation:
**	The Windows4GL compiler has generated a bad operand for a SELECT
**	statement. This should never occur.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	Contact the Site Technical contact to report the error.
*/

/*
**	itilloop.qsc
*/

S_WT004E_EXCEPTION_DURING_EXEC	"Exception during execution:"

S_WT004F_INTERRUPT	"Interrupt\n"

S_WT0050_UNKNOWN_EXCEPTION	"Unknown exception\n"

S_WT0051_TRACEBACK	"Traceback:\n"

F_WT0007_AT_LINE	"\t%s @ line# %d\n"

F_WT0008_CLOSING_CUR_APP	"Closing the current application"

F_WT000A_EXIT_APPED	"Exit the Application Editor?"

E_WT005F_EXCLUSIVE_WAIT	"The application tried to wait for an event
while running a user routine (such as a handler) in exclusive mode.\n"

/*%
** Explanation:
**	The application tried to wait for a window system event, user event or
**	database event, while running a user-written routine in exclusive mode.
**	This could be either a procedure invoked via the 'runexclusive' method
**	on a prochandle, or else an error handler invoked by Windows4GL itself
**	(for example, a procedure whose prochandle has been placed into
**	the DataEntryErrorHandler attribute of the FrameExec for a frame
**	into which a user has just typed invalid data).
**
** System Status:
**	The currently executing procedure or method (that's running
**	in exclusive mode) will be terminated.
**
** User Action:
**	Change the procedure or method so that it doesn't wait for events.
*/

/*
**	itstack.c
*/

S_WT0052_NOTHING_TO_EXEC	"Stack has no executable object.\n"

E_WT0053_STACK_POP_ERROR	"Top of execution stack did not contain FRAME that Windows4gl expected.\n"

/*%
** Explanation:
**	Windows4gl did not find the FRAME that it expected when asked to
**	pop a FRAME off the execution stack.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	Not much user can do except to report problem to Tech Support.
*/

E_WT0054_RECURSIVE_INTERPLOOP
"The Windows4GL interpreter was re-entered while it was active.  This is a
fatal error."

/*%
** Explanation:
**	The Windows4GL interpreter was called while it was interpreting 
**	another frame or procedure.  This should never occur.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	Contact the Site Technical contact to report the error.
*/

/*
**	itclass.qsc
*/

F_WT0009_ARRAY_ACCESS	"Array Access"

E_WT005A_NOMETHINSCRIPT
"Method '%0c' is not implemented.\n\
When the script for the user class '%1c' was compiled, there was no code
for the method '%0c'."

/*%
** Explanation:
**	The script for the user class did not contain any code for the method.
**
** System Status:
**	The current event block or procedure will end.
**
** User Action:
**	Add the method to the script for the user class.
*/

E_WT005B_NULLAPPLPOINTER
"Null application pointer for '%0c'.\n\
The application pointer for the user class is NULL.  This is an internal error.
Please notify your Site Technical Contact."

/*%
** Explanation:
**	The application pointer for a user class is NULL.  This prevents methods
**	for the user class from being fetched.
**
** System Status:
**	The current event block or procedure will end.
**
** User Action:
**	Contact the Site Technical Contact to report the error.
*/

/*
** curstk.c
*/
E_WT005C_PRIVATEMEMBER
"'%0c' is a private %2c of class '%1c'.  Only methods of class '%1c'
and its subclasses can reference it."

/*%
** Explanation:
**	A reference was made to a private member of a class.  Only the methods
**	of the class and its subclasses can access this member.
**
** System Status:
**	The access will fail.  An attribute will not be gotten or set,
**	and a method will not be called.
**
** User Action:
**	Remove the reference to the private member.
*/

/*
** prchndl.c
*/
E_WT0060_PROCHANDLE_UNDEFINED
"Object %0c is not defined for the specified scope.\n
An attempt was made to get a prochandle for a procedure
which could not be found in the specified scope."

/*%
** Explanation:
**	An attempt was made to get a prochandle for a procedure
**	which could not be found in the specified scope.
**
** System Status:
**	The 'getprochandle' will be ignored.
**
** User Action:
**	Either create the procedure, or else include an application
**	that defines the procedure.
*/

E_WT0061_PROCHANDLE_OBJNOTPROC
"Object %0c is not a procedure.\n
An attempt was made to get a prochandle for %0c,
but it's a %1c, not a procedure."

/*%
** Explanation:
**	An attempt was made to get a prochandle for an object
**	that's not a procedure.
**
** System Status:
**	The 'getprochandle' will be ignored.
**
** User Action:
**	Specify the name of a procedure.
*/

E_WT0062_PROCHANDLE_NO_APPEXEC
"No running application could be found for the specified scope.\n
An attempt was made to get a prochandle for %0c,
but no running application could be found for the specified scope."

/*%
** Explanation:
**	An attempt was made to get a prochandle for a procedure,
**	but no running application could be found for the specified scope.
**	This is an internal Windows/4GL error.
**
** System Status:
**	The 'getprochandle' will be ignored.
**
** User Action:
**	Contact the Site Technical contact to report the error.
*/

E_WT0063_PROCHANDLE_EMPTY
"A prochandle was never initialized.\n
A Call or RunExclusive method on a prochandle
could not be executed because the prochandle was never initialized
(by an invocation of the GetProchandle method on an object of class
Scope, or a subclass thereof)."

/*%
** Explanation:
**	The message says it all.
**
** System Status:
**	The method will not be executed.
**
** User Action:
**	Don't do this.
*/

E_WT0064_PROCHANDLE_BADCONTEXT
"A prochandle's context has terminated.\n
A Call or RunExclusive method could not be executed on a prochandle
for the local procedure '%0c', because the frame or global procedure
in which the prochandle was obtained has subsequently terminated."

/*%
** Explanation:
**	The message says it all.
**
** System Status:
**	The method will not be executed.
**
** User Action:
**	Don't do this.
*/

E_WT0065_PROCHANDLE_BADSTACK
"A prochandle's context is for a different thread.\n
A Call method could not be executed on a prochandle
for the local procedure '%0c', because the frame or global procedure
in which the prochandle was obtained was in a different thread
than the one that's currently executing.
(Threads are created by OPENFRAME statements)."

/*%
** Explanation:
**	You cannot get a prochandle for a local procedure in one thread
**	and then try to call it in another thread.
**	(A "thread" is created by an OPENFRAME).
**
** System Status:
**	The method will not be executed.
**
** User Action:
**	Don't do this.  If the local procedure doesn't call, goto, or open
**	any frames, and doesn't wait for any events, consider using
**	the RunExclusive method instead of the Call method.
*/

/*
** ilexpr.c
*/
E_WT0066_NULL_TO_NONNULL
"An attempt was made to place a null value in a non-nullable datatype."

/*%
** Explanation:
**	An attempt was made to place a null value in a non-nullable datatype.
**
** System Status:
**	Control will pass to the next 4GL source statement.
**
** User Action:
**	Use a nullable datatype.
*/

/*
**	ildisp.c (continued)
*/
E_WT0067_INVALID_IL_OPCODE
"The interpreter found an invalid intermediate language operation (%0d).\n
Please notify your Site Technical Contact.  Try recompiling the program."

/*%
** Parameters:
**	1	This is the offending opcode (in decimal).
**
** Explanation:
**	The Windows/4GL interpreter has encountered an invalid
**	intermediate language operation.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	Try recompiling the program.  This may work around the problem.
**	In any case, contact the Site Technical contact to report the error.
*/

/*
**	if.c
*/
E_WT0068_CANNOT_INSTANTIATE
"Cannot instantiate an executable object.\n
The interpreter was unable to instantiate %0c '%1c'.
Subsequent message(s) will give more detail."

/*%
** Parameters:
**	0	The type of the offending executable object (frame, method,
**		global procedure, or local procedure).
**
**	1	The name of the offending executable object.
**
** Explanation:
**	The Windows/4GL interpreter was unable to instantiate an executable
**	object.  Subsequent message(s) will give more detail.
**
** System Status:
**	The executable object will not be executed.
**
** User Action:
**	Respond to the subsequent message(s).
*/

E_WT0069_TOO_MANY_VARS_COMPILE
"Too many variables declared.\n
No executable object (application, frame, global procedure, local procedure,
or method) can run on Microsoft Windows, release 3.x,
if it declares more than %0d variables.  This includes
all user-declared variables and compiler-generated temporary variables.
(For a frame, it also includes all fields except those
for which a variable was explicitly not declared).
But the executable object being instantiated (which was presumably
compiled on a different platform) declares %1d variables."

/*%
** Parameters:
**	0	The maximum number of variables allowed on Microsoft Windows.
**
**	1	The number of variables declared in the executable object
**		being instantiated.
**
** Explanation:
**	The Windows/4GL interpreter, running under Microsoft Windows,
**	release 3.x, was asked to instantiate an executable object 
**	which contains too many declared variables.
**	A previous message identifies the offending executable object.
**
** System Status:
**	The executable object will not be executed.
**
** User Action:
**	Try reducing the number of variables in the executable object,
**	or try splitting parts of it (each part including some variable
**	declarations) off into separate local procedures.
*/

E_WT006A_DYNDECL_COMPILE_FAIL
"The compilation for a dynamic declaration failed.\n
The interpreter was unable to dynamically declare a variable for %0c '%1c',
because the necessary 4GL compilation failed.
A subsequent message from the 4GL compiler identifies the specific problem."

/*%
** Parameters:
**	0	The type of the offending executable object (frame, method,
**		global procedure, or local procedure).
**
**	1	The name of the offending executable object.
**
** Explanation:
**	The Windows/4GL interpreter was asked to dynamically declare a variable
**	(probably on behalf of the declaredata method on fieldobject)
**	but the internal 4GL compilation failed.
**
** System Status:
**	The variable will not be declared.  If it was requested on behalf of
**	the declaredata method on fieldobject, the declaredata will fail
**	(as indicated by a previous message).
**
** User Action:
**	Respond to the subsequent error(s) reported by the compiler.
*/

/*
** ilexpr.c (continued)
*/
E_WT006B_EXPR_TOO_COMPLEX_WIN
"4GL expression is too complex for Windows.\n
A 4GL expression is too complex to be evaluated on Microsoft Windows,
release 3.x.  Try splitting it into multiple expressions."

/*%
** Explanation:
**	The Windows/4GL interpreter was asked to evaluate an expression
**	which is too complex to be evaluated on Microsoft Windows,
**	release 3.x.
**
** System Status:
**	Control will pass to the next 4GL source statement.
**
** User Action:
**	Try splitting the expression into multiple expressions
**	(by evaluating subexpressions into 4GL variables).
*/

/*
** ilcntrl.c
*/
W_WT006C_CASE_NO_MATCH
"Warning.  The case statement had no default action, but the value of the
selector expression did not match any of the case values specified."

/*%
** Explanation:
**      The execution of a 'case' statement found a value that did not
**      match any of the values given in the script, and no 'default'
**	case was given either, so nothing in this case statement will
**	be executed.
**
** System Status:
**	Control will pass to the next 4GL source statement.
**
** User Action:
**	A default case should be added to the case statement if this
**	situation needs attention, otherwise it can be ignored.
*/

E_WT006D_EXTOBJ_INVOKE_FAILURE
"The invocation of an external object method failed.\n
The method '%0c' for source '%1c' returned an error code of %2c and the
error message: \n
'%3c'"

/*
** r4glproc.c
*/

E_WT006E_R4GLPC_NULL_APPSRV
"The AppServ attribute is NULL.\n
The AppServ attribute must be set to a valid application server-type external object."


E_WT006F_R4GLPC_INVALID_APPSRV
"The AppServ attribute is not set to an application server-type object.\n
The AppServ attribute must be set to a valid application server-type external object."


/*%
** Explanation:
**	Fail to get a cell.
**
** System Status:
**	Return an error to the calling method.
**
** User Action:
**	Need to check the 4gl code.
*/

E_WT0070_CANNOT_GET_CELL
"Failed to get the cell in column '%0c' row '%1d'.\n
Check if that column and/or row exists."
 
