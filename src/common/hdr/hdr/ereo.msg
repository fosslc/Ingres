/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ereo.msg	- Message file for exeobj directory.
**
** Description:
**	This is the error definition file for EXEOBJ.
**
** History:
**	1-feb-1990 (Joe)
**	     First Written
**	11/20/91 (dkh) - ER changes for internationalization.
**	5-feb-1993 (emerson)
**	    Added E_EO001E - E_EO0022 for dynamic expressions.
**	    Added F_EO0003_EXECFORLOCAL for local procedures.
**	12-apr-1993 (emerson)
**	    Modified E_EO001D_NOTAFRAME because procedures can now receive
**	    notification of non-delivery of user events, as long as there's
**	    a frame in the call stack.
**	16-jul-93 (bruceb)
**	    Added E_EO0023_DEFERCONFIG_NOWINDOW.
**	17-jul-93 (emerson)
**	    Corrected typo in previous change.
**	9-sep-1993 (emerson)
**	    Added the expression string to dynamic expression error messages
**	    (E_EO001E - E_EO0020 and E_EO0022).
**	31-jan-1994 (emerson)
**	    Added E_EO0029_EVTBADROWCLASS.
**	2-feb-1994 (emerson)
**	    Refined E_EO0029_EVTBADROWCLASS.
**	1-mar-1994 (blaise)
**	    Fixed spelling mistake ("heirarchy"->"hierarchy").
**	21-mar-1994 (emerson)
**	    Added E_EO002A_DYNEXPR_BAD_CONTEXT.
**/

# define	_EO_CLASS	200

E_EO0000_EVTBADNAME	"Event '%0c' is unknown.\n
The event name '%0c' is unknown.  Please check your Windows 4GL documentation
for the allowed event names."

/*%
** Arguments:
**	0	This is the name the user gave.
**
** Explanation:
**	The user gave a name for an event that is not known to the
**	system.
**
** System Status:
**	In the 4GL compiler this will be reported as an error and
**	the 4GL will not be successfully compiled.
**
** User Action:
**	Check the documentation to find the legal event names.
*/

E_EO0001_EVTBADCLASS	"Incorrect class of object for event '%0c'\n
The event '%0c' is not allowed on the class of the given object.
The allowed classes for this event are %1c or their(its) subclasses."

/*%
** Arguments:
**	0	This is the name of the event.
**
**	1	This is a list of the allowed classes that the
**		event can be issued on.
**
** Explanation:
**	The user tried to do an ON event for an object that doesn't
**	generate the given event.
**
** System Status:
**	In the 4GL compiler this will be reported as an error and
**	the 4GL will not be successfully compiled.
**
** User Action:
**	Check the documentation to see what events are legal for
**	different kinds of objects.
*/

E_EO0002_FLUSH_NOWINDOW	"Flush invoked on non-window object.\n
The flush method was invoked on an executable object that does not have
a frame as an ancestor.  In general, Flush should be appled to CurFrame.
Flush can only be applied to CurProcedure if an ancestor of the current
procedure is a frame."

/*%
** Explanation:
**	The flush method was applied to an object that does not
**	have a window available to it.  In general flush can only
**	be done to a procedure, or to a procedure that has a frame
**	as an ancestor.
**
** System Status:
**	The Flush is not done.
**
** User Action:
**	Correct the invocation so that Flush is applied to an
**	object that has a window available to it.
*/

E_EO0003_FLUSH_WAIT	"Error during flush.\n
A previous error was encountered that prohibited a flush from
being done.  Normally this means that flush wasn't done on
CurFrame or CurProcedure.  Only apply Flush to CurFrame or to
CurProcedure (but then only if the procedure has a frame as an
ancestor.)."

/*%
** Explanation:
**	The system uses the waitfor method to implement the
**	flush method.  During the execution of a flush, a waitfor
**	could not be done.  The problem with the waitfor will
**	be reported, and must be corrected for the waitfor to
**	run.
**
** System Status:
**	The Flush is not done.
**
** User Action:
**	Correct the previous error to get the flush to succeed.
*/

E_EO0004_OBJTO3GL  "Objects and arrays can not be passed to 3GL procedures.\n
Parameter number %1d to the 3GL procedure '%0c' is an object or array value.
Objects and arrays can not be passed to 3GL procedures.  You will have to
remove this argument from the parameter list to this procedure."

/*%
** Explanation:
**	3GL procedures can not be passed an object or an array from
**	Windows 4GL.
**
** System Status:
**	The 3GL procedure will not be called.
**
** User Action:
**	Remove the object or array parameters.
*/

E_EO0005_BEEP_NOWINDOW		"Beep invoked on non-window object.\n
The method Beep was invoked on an executable object that does not have
a frame as an ancestor.  In general, Beep should be appled to FrameExec.
It can be applied to a ProcExec if the procedure was called by a frame."

/*%
** Explanation:
**	The beep method was applied to an object that does not
**	have a window available to it.  In general beep can only
**	be done to a frame, or to a procedure that has a frame
**	as an ancestor.
**
** System Status:
**	The beep is not done.
**
** User Action:
**	Correct the invocation so that beep is applied to an
**	object that has a window available to it.
*/

E_EO0006_PROCEXEC_SENDEVENT	"SendUserEvent invoked on a procedure.\n
The SendUserEvent method was invoked on a procedure.  SendUserEvent can
only be invoked on a frame.  Correct the code so that a SendUserEvent is
only done on a frame."

/*%
** Explanation:
**	The SendUserEvent although defined on ProcExec is an error.
**	SendUserEvent can only legally be applied to FrameExecs.
**
** System Status:
**	The event is not sent.
**
** User Action:
**	Correct the invocation so that SendUserEvent is only applied
**	to a frame.
*/

E_EO0007_LOG_OPEN	"Cannot open file temporary file %0c for debugger log."

/*%
** Explanation:
**	Cannot open a temporary file for the debugger log.
**
** System Status:
**	On screen log output is not available.  If log output has
**	been redirected, it will continue to go there.
**
** User Action:
**	Correct the situation which makes it impossible to open temporary
**	file.
*/

E_EO0008_LOG_WRITE	"Cannot write temporary file for debugger log."

/*%
** Explanation:
**	Write failed on debugger log.
**
** System Status:
**	On screen log output is not available.  If log output has
**	been redirected, it will continue to go there.
**
** User Action:
**	Correct the situation which caused the write failure.
*/

E_EO0009_INVALID_WINDOWICON "The bitmap is not valid for window icons.\n
The specified bitmap object is not valid for use as a window icon."

/*%
** Explanation:
**	Attempt to define a window icon failed.
**
** System Status:
**	The window icon is unchanged.
**
** User Action:
**	Try again with another icon.
*/

E_EO0010_INVALID_DEFAULTBUTTON "The default button is invalid.\n
The specified buttonfield object is not valid for use as a default button."

/*%
** Explanation:
**	Attempt to define a window icon failed.
**
** System Status:
**	The window icon is unchanged.
**
** User Action:
**	Try again with another icon.
*/

F_EO0000_EXECFOR	"%0c for '%1c'"

F_EO0001_EXECNULL	"%0c for NULL sourceobject"

F_EO0002_NOCONNECT	"No connection."

F_EO0003_EXECFORLOCAL	"%0c for local procedure '%2c' in '%1c'"

/*
**	appexec.c
*/

S_EO0011_NO_STARTING_OBJECT	"Can't find starting object %s.\n"

S_EO0012_CANT_INSTANTIATE	"Can't instantiate starting object %s.\n"

/*
**	frameex.c
*/

S_EO0013_CANT_FIND_FIELD	"Can't find field %s for ActiveField.\n"

S_EO0014_CANT_FIND_EVENT	"Can't find event %s for activate event.\n"

S_EO0015_BUT_NOT_ONE_OF	" but NOT one of "

S_EO0016_CANT_PUT_SEPARATOR	"Can't put separator on menubar.\n"

S_EO0017_MENUNAME_TOO_LONG	"Menu name %s is too long.\n"

/*
**	procexe.c
*/

E_EO0018_SESS_NOT_CONNECTED	
"DBSESSIONOBJECT is not connected.\n\
An attempt was made to make a DBSESSIONOBJECT which is not connected to a
database the session for the frame or procedure '%0c'.  The frame or
procedure's database session will not be changed."

/*%
** Explanation:
**	The state of the DBSESSIONOBJECT in question was not DS_CONNECTED.
**	This means that the DBSESSIONOBJECT is not connected to a database
**	session, and thus cannot become the session for a frame or procedure.
**
** System Status:
**	The frame or procedure will keep its previous session.
**
** User Action:
**	Connect the database session before attaching it to a frame or
**	procedure.
*/

E_EO001A_SESS_NULL
"DBSESSIONOBJECT is NULL.\n\
An attempt was made to make a DBSESSIONOBJECT which is NULL the session
for the frame or procedure '%0c'.  The frame or
procedure's database session will not be changed."

/*%
** Explanation:
**	The DBSESSIONOBJECT in question is NULL.  The DBSESSIONOBJECT is 
**	not connected to a database session, and thus cannot become 
**	the session for a frame or procedure.
**
** System Status:
**	The frame or procedure will keep its previous session.
**
** User Action:
**	Set the current frame or procedure's DBsession attribute to a 
**	non-NULL DBsessionObject.
*/

E_EO001B_OBJBYREF  
"Objects and arrays can not be passed BYREF to 3GL procedures.\n
Parameter number %1d to the 3GL procedure '%0c' is an object or array value.
Objects and arrays can not be passed BYREF to 3GL procedures.  You will have to
remove the BYREF keyword from the parameter list to this procedure."

/*%
** Explanation:
**	3GL procedures can not be passed an object or an array BYREF from
**	Windows 4GL.
**
** System Status:
**	The 3GL procedure will not be called.
**
** User Action:
**	Remove the BYREF keyword from object or array parameters.
*/

E_EO001C_CANNOT_SET_ISGRIDON
"The IsGridOn attribute of the framesource object may not be reflected in
the running frame."

E_EO001D_NOTAFRAME "Sender is not a frame, nor is it running under a frame.\n\
The 'errorevent' parameter can be used with the SendUserEvent method only
when the component sending the Userevent is a frame or a procedure called
from a frame (directly or indirectly)."

/*%
** Explanation:
**	The SendUserEvent was issued from a procedure in an application
**	which has not yet started any frames.  Since such procedures cannot
**	receive user events, you cannot specify an event to be sent back
**	to the calling component on error if the calling component is
**	such a procedure.
**
** System Status:
**	The userevent will be sent, but the errorevent parameter will
**	be ignored.
**
** User Action:
**	Remove the errorevent parameter, or send the event from a frame
**	or from a procedures called from a frame.
*/

/*
** dynexpr.c
*/
E_EO001E_DYNEXPR_NOT_LVALUE
"The setvalue method was invoked on a dynamic expression ('%0c')
that's not a legal left-hand side for an assignment."

/*%
** Explanation:
**	The setvalue method was invoked on a dynamic expression, but
**	the dynamic expression is not a legal left-hand side for an assignment.
**
** System Status:
**	The method will fail.
**
** User Action:
**	Change the dynamic expression or else don't invoke setvalue on it.
*/

E_EO001F_DYNEXPR_NOT_FIELD
"The getfield method was invoked on a dynamic expression ('%0c')
that's not a field."

/*%
** Explanation:
**	The getfield method was invoked on a dynamic expression, but
**	the dynamic expression does not specify a field.
**
** System Status:
**	The method will fail.
**
** User Action:
**	Change the dynamic expression or else don't invoke getfield on it.
*/

E_EO0020_DYNEXPR_BAD_CONTEXT
"A dynamic expression's context has terminated.\n
A getvalue, setvalue, or getfield method on a dynamic expression ('%0c')
could not be executed because the dynamic expression's context is a
frame, procedure, field script, or event block which has terminated."

/*%
** Explanation:
**	A dynamic expression was created specifying a context
**	(a frame, procedure, field script, or event block) which has now
**	terminated.  No getvalue, setvalue, or getfield method may be
**	invoked against such a dynamic expression.
**
** System Status:
**	The method will not be executed.
**
** User Action:
**	Don't do this.
*/

E_EO0021_DYNEXPR_UNINITIALIZED
"A dynamic expression was never initialized.\n
A getvalue, setvalue, or getfield method on a dynamic expression
could not be executed because the dynamic expression was never initialized
(by an invocation of the CreateDynexpr method on an object of class
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

E_EO0022_DYNEXPR_NOT_EVALUATED
"A dynamic expression ('%0c') could not be evaluated.\n
A getvalue, setvalue, or getfield method failed because the Windows/4GL
interpreter detected a fatal error when it tried to evaluate the dynamic
expression.  A subsequent message will specify the error."

/*%
** Explanation:
**	The Windows/4GL interpreter detected a fatal error when it tried to
**	evaluate a dynamic expression for a getvalue, setvalue, or getfield
**	method.  A subsequent message will specify the error.  Likely causes
**	are (1) a VALUE or FIELD parameter of improper type was specified when
**	invoking the method, or (2) a subscript in the dynamic expression
**	is out of range.
**
** System Status:
**	The method will fail.
**
** User Action:
**	Correct the error(s) reported in subsequent message(s).
*/

E_EO0023_DEFERCONFIG_NOWINDOW  "DeferConfigure invoked on non-window object.\n
The DeferConfigure method was invoked on an executable object that does not
have a frame as an ancestor.  In general, DeferConfigure should be appled to
CurFrame.  DeferConfigure can only be applied to CurProcedure if an ancestor
of the current procedure is a frame."

/*%
** Explanation:
**	The DeferConfigure method was applied to an object that does not
**	have a window available to it.  In general DeferConfigure can only
**	be done to a procedure, or to a procedure that has a frame
**	as an ancestor.
**
** System Status:
**	The DeferConfigure is not done.
**
** User Action:
**	Correct the invocation so that DeferConfigure is applied to an
**	object that has a window available to it.
*/

/*
** methexec.c
*/
E_EO0024_NOTCURRENT	"Invalid invocation of SendSuperclass.\n\
The SendSuperclass method can only be invoked on the currently executing
method."

/*%
** Explanation:
**	SendSuperclass was invoked on a MethodExec which is not the currently
**	executing method.  The SendSuperclass invocation will be ignored.
**
** System Status:
**	SendSuperclass is not done.
**
** User Action:
**	Correct the invocation so that SendSuperclass is applied to the
**	currently executing method (curmethod)
*/


S_EO0025_ARGSIGNORED	"Arguments passed to SendSuperclass will be ignored.\n\
The %0c method for class %2c will be passed the same arguments that 
the %0c method for class %1c was passed."
/*%
** Explanation:
**	The SendSuperclass method implicitly passed the ancestor class's
**	method the same arguments that were passed to the calling method.
**
** System Status:
**	The arguments specified in 4GL will be ignored.
**
** User Action:
**	Remove the arguments from the SendSuperclass invocation.
*/

E_EO0026_NOSUPERCLASS	
"Class %0c has no superclass.  This is an internal error."

E_EO0027_NOSUPERMETHOD	"No ancestor class supersedes %0c.\n\
There is no version of method %0c defined by any superclass of
%1c."

/*%
** Explanation:
**	SendSuperclass was invoked on a method which does not supersede
**	another version of this method.
**
** System Status:
**	SendSuperclass is not done.
**
** User Action:
**	Correct the invocation so that SendSuperclass is applied only to methods
**	which supersede an ancestor class's version of that method.
*/

E_EO0028_NONUNIQUEAPP "Non-unique included application name.\n\
The hierarchy of included application for the current application contains two
applications named %0c.  Included application names must be unique within
a hierarchy."

/*%
** Explanation:
**      All application names must be unique within a hierarchy of included
**      applications.
**
** System Status:
**      The application cannot be run.
**
** User Action:
**      One of the applications must be renamed or removed from the included
**      application hierarchy.
*/

E_EO0029_EVTBADROWCLASS	"Incorrect class of rows of object for event '%0c'\n
Neither the event '%0c' nor the event '%2c' is allowed
on the class of the rows of the given object.
The allowed classes for these events are %1c or their(its) subclasses."

/*%
** Arguments:
**	0	This is the name of the event (presumably begins with 'child').
**
**	1	This is a list of the allowed classes that the
**		event can be issued on.
**
**	2	This is the name of the event with the leading 'child'
**		(if present) stripped off.
**
** Explanation:
**	The user tried to do an ON CHILD<event> for a COLUMNFIELD object,
**	but its rows don't generate <event>, nor do any of their descendants.
**
** System Status:
**	In the 4GL compiler this will be reported as an error and
**	the 4GL will not be successfully compiled.
**
** User Action:
**	Check the documentation to see what events are legal for
**	different kinds of objects.
*/

/*
** dynexpr.c (continued)
*/
E_EO002A_DYNEXPR_BAD_CONTEXT
"A dynamic expression's scope is invalid.\n
A dynamic expression ('%0c') could not be created because the scope
on which it's being created is a frame, procedure, field script,
or event block which has terminated."

/*%
** Explanation:
**	The message says it all.
**
** System Status:
**	The createdynexpr method will not be executed.
**
** User Action:
**	Don't do this.
*/
