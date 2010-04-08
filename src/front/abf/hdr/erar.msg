/*
**      Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/

/**
** Name:        erar.msg -      ABF Run-time System Message Definitions File.
**
** Description:
**      Contains the message definitions for the ABF run-time system.
**
** History:
**      Revision 6.0  87/07/31  wong
**      Initial revision.
**      Revision 6.3/03/00
**      89/08  wong
**      Added message for 'look_up()' frame, E_AR002A-2E.  Deleted dump
**      messages, which are diagnostic.
**      Revision 6.4  89/08  wong
**      Added message for 'look_up()' frame, E_AR002A-2E.  Deleted dump
**      messages, which are diagnostic.
**
**      9/90 (Mike S)
**      None of the messages issued by abexeprog in response to UTexe errors
**      have more than 3 reliable parameters.  Edited these messages to remove
**      references to anything past %2c
**
**      Revision 6.3  90/03  wong
**      Added E_AR0027 error message.  Also, edited messages E_AR001B through
**      E_AR0022 for clarity.
**
**      23-oct-1990 (mgw)
**              Corrected misspellings in some messages.
**      11/09/90 (emerson)
**              Added message E_AR0065 (for bug 34389).
**      03/22/91 (emerson)
**              Added messages E_AR0039 through E_AR003F as part of support
**              for passing an array to look_up.  (Topaz project).
**      03/26/91 (emerson)
**              Added messages E_AR0080 through E_AR0084 as part of support
**              for starting applications not in forms mode.  (Topaz project).
**      04/07/91 (emerson)
**              Modifications for local procedures:
**              Added S_AR0049_LOCALPROC.
**      07/24/91 (emerson)
**              Added F_AR0015 and F_AR0016 for bug 30660.
**      09-aug-91 (davel)
**              Added E_AR0038  for bug 38012.
**      29-apr-1992 (mgw) Bug #37879
**              Added E_AR0085_OLpcall_failed for case where OLpcall
**              fails to call the Host Language Procedure.
**      21-sep-92 (blaise)
**              Corrected parameter E_AR000E_ABNULLOBJ.
**      17-nov-92 (davel)
**              Added error messages for multi-session support (E_AR0066 - 6A).
**      10-mar-93 (davel)
**              Added two more error messages for multi-session support
**              (E_AR006B - E_AR006C).
**      09-mar-93 (connie)
**              Added E_AR0086_Invalid_Inq4gl_cnst & E_AR0087_Unmatched_datatype
**      13-may-93 (essi)
**              Added messages for External File I/O support (E_AR0130 -0153).
**      30-jun-93 (essi)
**              Added E_AR0154 - E_AR0156 in further support of File I/O.
**      14-jul-1993 (mgw)
**              Grammar fix. Changed "...an 4GL..." to "...an INGRES/4GL..."
**      16-jul-93 (essi)
**              Added E_AR0157 - E_AR0159 in further support of File I/O.
**      21-jul-93 (donc)
**              Fix error E_AR0136 - Changed %2d to %1d
**      30-jul-93 (DonC)
**              Added E_AR0088_divzero, E_AR0089_numovf, E_AR0090_fltbad
**              in order to fix bug 45678.
**      09-sep-93 (essi)
**              Changed E_AR0143 message.
**      21-apr-94 (connie)
**              Added E_AR012F_ReadOnAppend
**      11-nov-94 (athda01)
**              Added E_AR015A_BinReclenNotProv, issued within InquireFile
**	04-may-97 (mcgem01)
**		Remove reference to TERM_INGRES.
**/

#define _AR_CLASS       41

E_AR0000_NOSUCHDB       "Error connecting to database `%0c'."

/*%
** Explanation:
**      The application could not successfully connect to the named database
**      most likely because the database does not exist, or possibly because
**      the commands specified using the ING_SET environment logical are
**      incorrect.
**
**      Occasionally, successful database connection may be precluded by
**      other errors (e.g., GCA errors, etc.) usually evidence of some sort
**      of installation problem or hardware or system failure.
**
** System Status:
**      The application will not start up.
**
** User Action:
**      Check that the database name was specified correctly and re-enter
**      the command line.  Also, check that the commands specified using
**      the ING_SET logical are correct.
*/

E_AR0001_OUTOFMEM
"The ABF run-time system has run out of dynamic memory.
This condition usually represents an internal programming problem.
The routine that ran out of memory was `%0c'."

/*%
** Explanation:
**
** System Status:
**      The application exits after issuing the error message.
**
** User Action:
**      Report this problem to your site technical contact.
*/

E_AR0002_ALLOCFRM       "Cannot initialize ABF form.\n
The ABF run-time system could not initialize the form `%1c', which is
used internally.  This is an installation problem.  Please report it
to your Site Technical Contact."

/*%
** Explanation:
**      The ABF run-time system could not start the INGRES Forms System or
**      initialize its required forms.  This could be for several reasons
**      including ...
**
** System Status:
**      The application terminates immediately.
**
** User Action:
**      Check that the system terminal type environment logical has been 
**	set correctly before reporting this problem to your site 
**	technical contact.
*/

E_AR0003_PRMTYPE        "Value passed to parameter of incompatible type.\n
A value of type `%4c' was passed to the parameter `%2c' in the %1c `%0c'.  The
type `%4c' is not compatible with the type of the parameter, `%3c'."

#define _BadPrmType     E_AR0003_PRMTYPE

/*%
** Explanation:
**      A value was passed to a parameter in a frame or procedure that did
**      not agree in type with that declared for the parameter.  That is,
**      the type of the value is not compatible with (coercible to) that
**      of the parameter.
**
** System Status:
**      The value of the parameter will be undefined.
**
** User Action:
**      Ensure that the type of a value passed to a parameter agrees
**      with the type declared for the parameter.
*/

E_AR0004_POSTOOSL       "Positional parameters passed to 4GL procedure.\n
The 4GL procedure `%0c' was called with parameters specified by position
(i.e., of the form arg1, arg2, ...).  A 4GL procedure can only be called with
parameters passed by name (i.e., of the form param1 = arg1, param2 = arg2,
...)."

/*%
** Explanation:
**      4GL procedures do not define any positional parameters.  All
**      parameters passed to 4GL procedures must be passed by name.
**
** System Status:
**      The value passed to the procedure will be ignored.
**
** User Action:
**      Pass parameters to 4GL procedures by name only.
*/

E_AR0005_KEYTOHOST      "Named parameters passed to host-language procedure.\n
The host-language procedure `%0c' was called with parameters specified by
name (i.e., of the form param1 = arg1, param2 = arg2, ...) as if it were
an INGRES/4GL procedure.  Host-language procedures can only be called with
parameters specified by position (i.e., of the form arg1, arg2, ...)."

/*%
** Explanation:
**      Host language procedures do not define parameters passed by name.  All
**      parameters passed to such procedures must be passed by position.
**
** System Status:
**      The value passed to the procedure will be ignored.
**
** User Action:
**      Pass parameters to host procedures by position only.
*/

E_AR0006_RETNOTYPE
"The %1c `%0c' was called expecting a return type of `%2c'. '%0c' does
not return a value since it was declared as returning 'none' in ABF."

/*%
** Explanation:
**      A frame or procedure was called and was expected to return a
**      value.  However, the frame/procedure had been declared in ABF
**      as not returning any value.
**
** System Status:
**      No value will be returned from the called frame/procedure.
**
** User Action:
**      Eliminate the reference to a return value in the calling frame,
**      or change the declaration of the called frame/procedure.
*/

E_AR0007_ABNULLPRM      "Possible Null value passed to non-Null parameter.\n
A value of Nullable type was passed to the parameter `%2c', which is not
Nullable, in the %1c `%0c'.  This will result in a run-time error when a Null
value is passed to this parameter.  Use the 'ifnull()' function when passing
this value to ensure that a run-time error will not occur."

/*%
** Explanation:
**      A value of a Nullable type is being passed to a parameter that has been
**      declared as being not Nullable.  Passing a Null value to this parameter
**      will result in a run-time error.  Since this value could be Nullable
**      you can use the 'ifnull()' function to pass it, which will guarantee
**      that a Null value is never passed to the non-Null parameter.
**
** System Status:
**      If the value is not Null, it will be passed to the parameter; otherwise
**      a run-time error will occur and the parameter will be undefined.
**
** User Action:
**      Change the type of the parameter to be Nullable, or pass the
**      value using the 'ifnull()' function, such as in 'ifnull(value)',
**      instead.
*/

E_AR0008_ABNULLBYREF    "Possible Null value returned by reference.\n
The parameter `%0c' is a Nullable type in the current %1c and is being returned
by reference to a non-Nullable field.  This will result in a run-time error when
a Null value is returned by reference to this field.  Define the parameter to
be non-Nullable to ensure that a run-time error will not occur."

#define _NullByRef      E_AR0008_ABNULLBYREF

/*%
** Explanation:
**      A parameter that is a Nullable type is being returned by reference to
**      a field that is not Nullable.  At some point, if the parameter contains
**      a Null value and is returned by reference, a run-time error will occur.
**
** System Status:
**      If not Null, the parameter will be returned by reference; otherwise
**      a run-time error will occur and the value of the field will be
**      undefined.
**
** User Action:
**      Change the type of the parameter to be not Nullable, or change
**      the type of the field passed by reference to be Nullable.
*/

E_AR0009_NOTBLFLD       "Table field report parameters are not yet supported.\n
Values entered in the table field `%0c' on the current form cannot be passed as
parameters to the INGRES/REPORT command."

/*%
** Explanation:
**      A table field exists on a form used by a Report Frame.  At present
**      no semantics have been defined for passing rows in the table field
**      as parameters to the Report command.
**
** System Status:
**      Table fields are ignored when passing parameters to the Report command.
**
** User Action:
**      Do not create table fields on forms to be used by a Report Frame.
*/

E_AR000A_DBGFILE        "Could not open FRS debug file.\n
The application could not open the FRS debugging file named as a command-line
argument.  Please check your current directory access rights and disk space
before reporting this as a problem to your Site Technical Contact."

/*%
** Explanation:
**      DIAGNOSTIC.
**
** System Status:
**      The application will terminate immediately.
**
** User Action:
**      Make sure that the you have access rights to the current directory and
**      enough disk space to create and write files there before reporting this
**      as a problem to your site technical contact.
*/

E_AR000B_NOINIFRM       "Unable to initialize form for frame.\n
The form `%1c' for the frame `%0c' could not be initialized.  The frame will
not be executed."

/*%
** Explanation:
**      The Forms System returned an error when the ABF run-time system
**      tried to initialize a form for a frame before calling the frame.
**      Possibly, this is because the form does not exist, or its name was
**      typed incorrectly on the User Frame definition form.
**
** System Status:
**      The called frame will not be run.
**
** User Action:
**      Create the form if it does not exist, or correct the name of the form
**      defined for the frame before reporting this as a problem to your
**      site technical contact.
*/

E_AR000C_ABNXTYPE       "Unknown frame type called.\n
The called frame was of an unknown type.  The error occurred in routine `%0c'.
Please report this problem to your Site Technical Contact."

/*%
** Explanation:
**
** System Status:
**      The application exits after issuing the error message.
**
** User Action:
*/

E_AR000D_ABRETFRM       "Return value expected from non-4GL frame.\n
The frame `%0c' was called expecting it to return a value.  Since it is
a `%1c' frame, it cannot return a value."

/*%
** Explanation:
**      Frames such as QBF or Report frames that are not "User" frames (i.e.,
**      that are not defined by 4GL source code) do not return values to frames
**      that call them.
**
** System Status:
**      The field designated to hold the return value has an undefined value.
**
** User Action:
**      Delete the code assigning the return value from the frame.
*/

E_AR000E_ABNULLOBJ      "Undefined routine for %1c `%0c'.\n
The internal ABF run-time structures contain a NULL reference for the routine
for the %1c `%0c'.  Please report this internal problem to your Site Technical
Contact."

/*%
** Explanation:
**      The ABF run-time system found a NULL reference to a routine in its
**      internal structures when the application tried to call or run an
**      INGRES/4GL frame or procedure.  (This routine contains the executable
**      code for the frame or procedure.)  ABF should generate a symbol for
**      this routine, both in the internal run-time structures that it builds
**      and in the code generated for the frame or procedure, when an
**      application is built.  Otherwise, this is an internal programming
**      problem.
**
**      In versions before release 6, this error could occur when the source
**      files for a frame and a procedure shared a common file name but had
**      different extensions (e.g., "xyz.osl" and "xyz.c".)  Then, the resultant
**      object files would collide, most likely overwriting the code for the
**      frame's routine.  ABF guarantees that this cannot occur in release 6
**      applications by not allowing files to be specified for frames or
**      procedures that share a common file name.
**
** System Status:
**      The called frame will not be run.
**
** User Action:
**      Report this problem to your site technical contact.
*/

E_AR000F_RETTYPE        "Incompatible return type.\n
The %1c `%0c' returns a value of type `%2c' that is not compatible
with the expected type of `%3c'."

/*%
** Explanation:
**      A return value from a frame or procedure did not agree in type with the
**      expected type.  That is, the type of the return value is not compatible
**      with (coercible to) the expected type declared for the frame or
**      procedure.
**
** System Status:
**      The return value will be undefined.
**
** User Action:
**      Make sure that the type of the value returned agrees with the
**      expected type declared for the frame or procedure.
*/

E_AR0010_ABEXPREVAL     "Error in evaluating an expression.\n
An error occurred when the ABF run-time system tried to evaluate an expression.
Please report this internal problem to your Site Technical Contact."

/*%
** Explanation:
**      The ABF run-time system evaluates all expressions using the internal
**      Abstract Data Facility defined for INGRES.  This facility returned an
**      error for an expression is was to evaluate.  Since, 4GL generates the
**      internal representation for these expressions from the source code,
**      it is likely that it did so incorrectly.
**
** System Status:
**      The result of the expression is undefined.
**
** User Action:
**      Report this problem to your site technical contact.
*/

E_AR0011_ABNULLNAME     "Null was used where a string is required.\n
A Null value was used where an INGRES/4GL name or string value is required."

/*%
** Explanation:
**
** System Status:
**      The 4GL statement containing the NULL value will not be executed.
**
** User Action:
**      Use a string constant or non-NULL field for the expected 4GL name.
*/

E_AR0012_ABNOFIELD      "Bad field specified in query passed between frames.\n
The field `%0c' has been named in the query passed between frames.  No such
field exists in the form `%1c' in the frame being called."

/*%
** Explanation:
**
** System Status:
**      The named field will be ignored in the processing of the query.
**
** User Action:
**      Correct the query being passed into the called frame so
**      that it lists the proper fields.
*/

E_AR0013_ABNOTFCOL      "Unknown table field column in parameter query.\n
The column `%0c' in the table field `%1c' has been named in the query passed
between frames.  No such table field column exists in the form `%2c' in the
frame being called."

/*%
** Explanation:
**
** System Status:
**      The named table field column will be ignored in the
**      processing of the query.
**
** User Action:
**      Correct the query being passed into the called frame so
**      that it lists the proper table field columns.
*/

E_AR0014_NOQRYFRM       "Wrong form referenced in query passed between frames.\n
The frame `%0c' was passed a query that references the form `%1c'.  The frame's
form is named `%2c', and that form will be used instead of `%1c'."

#define _NoQryForm      E_AR0014_NOQRYFRM

/*%
** Explanation:
**
** System Status:
**      The form for the called frame will be used rather than the
**      form named in the passed-in query.
**
** User Action:
**      Correct the name of the called frame's form in the query being
**      passed between frames.
*/

E_AR0015_NOQRYTBL       "Wrong table field referenced in passed-in query.\n
The frame `%0c' was passed a query that references the table field `%2c'.  No
table field with that name exists in the form `%1c'.  The query will not be
executed."

#define _NoQryTbl       E_AR0015_NOQRYTBL

/*%
** Explanation:
**
** System Status:
**      The query will not be executed.
**
** User Action:
**      Correct the name of the called frame's table field in the
**      query being passed between frames.
*/

E_AR0016_DOMOVER        "Too many domains in query.\n
Too many elements have been specified in the target list of the current
query.  Please reduce this number."

/*%
** Explanation:
**
** System Status:
**      The query will not be executed.
**
** User Action:
**      Reduce the number of elements in the target list of the query.
*/

E_AR0017_ABBADQUAL      "Bad internal type within qualification function.\n
A bad internal type occurred in the processing of a qualification
function.  The error occurred in routine `%0c'.
Please report this internal problem to your Site Technical Contact."

/*%
** Explanation:
**
** System Status:
**      The query will not be executed.
**
** User Action:
**      Report this problem to your site technical contact.
*/

E_AR0018_PARAMSPACE     "Not enough space in query.\n
The query passed to this frame or procedure does not have enough
extra space for ABF to insert some needed information.  Please recompile
the 4GL code for the frame that called this frame or procedure."

/*%
** Explanation:
**
** System Status:
**      The query will not be executed.
**
** User Action:
**      Please recompile the 4GL code for the frame that passed
**      a query to the current frame.
*/

E_AR0019_NOPARAM        "Incorrect parameter name.\n
The parameter `%0c' was passed to the %2c `%1c'.  No parameter named `%0c'
exists in `%1c'."

#define _UnknownPrm     E_AR0019_NOPARAM

/*%
** Explanation:
**      A parameter with an incorrect name was passed to an INGRES/4GL
**      frame or procedure.
**
** System Status:
**      The parameter will be ignored.
**
** User Action:
**      Correct the name of the parameter in the calling frame or
**      procedure.
*/

E_AR001A_ABPROGERR      "Execution failure in program `%0c'.\n
The program `%0c' could not be executed, or failed to execute successfully.  The
execution module returned a value of `%1c'(`0x%2c').  %3c"

/*%
** Explanation:
**      The program could not be executed, possibly because it could not
**      be found, or it was executed but returned a failure status because
**      of an internal programming error.
**
** System Status:
**      The program could not be called, or was called but returned
**      with a failure status.
**
** User Action:
**      Check your installation before reporting this problem to your
**      site technical contact.
*/

E_AR001B_CmdTooBig      "Sub-system command too large.\n
The command to call the sub-system `%0c' is too long to be executed on this
system.  Please refer to the INGRES ABF/4GL Reference Manual and reduce the
number and/or size of the parameters passed to this sub-system."

/*%
** Explanation:
**
** System Status:
**      The sub-system will not be executed.
**
** User Action:
**      Reduce the size of the command by reducing the number and/or
**      size of the parameters.
*/

E_AR001C_4GLPrmList     "Internal sub-system format error.\n
The parameter format list for the sub-system `%0c' was incorrectly built by the
4GL compiler.  The internal error number was `%1c'.  Please report this
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
**      The sub-system will not be executed.
**
** User Action:
**      Make note of the information reported in the error message and then
**      report this internal problem to your site technical contact.
*/

E_AR001D_UTBadSpec      "Sub-system parameter specification error.\n
The sub-system execution module contains an error in the specification line for
the sub-system `%0c'.  Please report this installation problem to your Site
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
**      The sub-system will not be executed.
**
** User Action:
**      Make note of the information reported in the error message and then
**      report this problem to your site technical contact.
*/

E_AR001E_4GLPrmType     "Internal sub-system format type error.\n
A parameter format type for the sub-system `%0c' was incorrectly specified by
the 4GL compiler.  The internal error number is `%1c'.  Please report this
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
**      The sub-system will not be executed.
**
** User Action:
**      Make note of the information reported in the error message and then
**      report this internal problem to your site technical contact.
*/

E_AR001F_NoAppSymbol    "Undefined application symbol.\n
A CALL was made to the application by its symbol name, but no such symbol was
found in this system.  Please refer to the INGRES ABF/4GL Reference Manual for
more information."

/*%
** Explanation:
**      A CALL statement in 4GL was made to an application sub-system whose
**      name was specified as the "name" parameter to the call.  No symbol
**      with that name specifying the application to be called can be found
**      defined in the system.
**
** System Status:
**      The application could not be executed.
**
** User Action:
**      Check that the application symbol name has been defined on your system
**      and then correct the name of the application in the CALL statement.  Or,
**      you can specify the full pathname for the executable image of the
**      application that you wish to call as the "executable" parameter to the
**      call instead of using the "name" parameter.
*/

E_AR0020_NoSubSystem    "Unknown INGRES sub-system, `%0c'.\n
A CALL was made to the sub-system `%0c'.  No sub-system with that name is
recognized for this installation.  Please check the INGRES ABF/4GL Reference
Manual for allowed sub-systems."

/*%
** Explanation:
**      A CALL statement in 4GL specified the name of a sub-system that is not
**      known to INGRES.  Most likely, this is because you incorrectly specified
**      the sub-system in your 4GL source-code, but could also be because of an
**      installation problem.
**
** System Status:
**      The sub-system could not be executed.
**
** User Action:
**      Correct the name of the sub-system to be called.  If necessary, refer
**      to the INGRES ABF/4GL Reference Manual for a list of legal sub-system
**      names.  Otherwise, if you know that the sub-system is a legal one,
**      report this installation problem to your site technical contact.
*/

E_AR0021_UTBadModule    "Internal sub-system module error.\n
The sub-system execution module contains an error in the module type specification for the sub-system `%0c'.  Please report this installation problem to your
Site Technical Contact."

/*%
** Explanation:
**      The sub-system execution module uses an internal file that specifies
**      the execution parameters for each sub-system, which is part of the
**      installation for INGRES.  In this file, the module type for this
**      sub-system has been incorrectly specified.  This is an internal or
**      installation problem.
**
** System Status:
**      The sub-system will not be executed.
**
** User Action:
**      Make note of the information reported in the error message and then
**      report this problem to your site technical contact.
*/

E_AR0022_BadParam       "Unknown sub-system parameter.\n
The sub-system `%0c' was called from 4GL with a parameter which is
not recognized for the sub-system in this installation.  Please refer to the
INGRES ABF/4GL Reference Manual to determine the legal parameters defined for
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
**      The sub-system cannot be executed.
**
** User Action:
**      Correct the parameters to the sub-system being called.  If necessary,
**      refer to the INGRES ABF/4GL Reference Manual for a list of the legal
**      parameters for this sub-system.  Otherwise, if you know that the
**      parameter is a legal one for the sub-system, report this installation
**      problem to your site technical contact.
*/

E_AR0027_MaxSubSysPrms  "Maximum sub-system parameters exceeded.\n
The maximum allowed number of parameters for a sub-system has been exceeded.
Please refer to the INGRES ABF/4GL Reference Manual for the parameters allowed
for this sub-system."

/*%
** Explanation:
**      A CALL sub-system statement in 4GL specified more parameters than are
**      allowed for any sub-system.
**
** System Status:
**      The sub-system will not be executed.
**
** User Action:
**      Please refer to the INGRES ABF/4GL Reference Manual to determine the
**      correct, legal parameters allowed for this sub-system call.  Then
**      correct the parameters passed by the CALL statement in your 4GL source
**      code file.
*/

E_AR0025_ReqQueryMode   "Warning:  Form not in query mode.\n
A qualification predicate has been specified for a query, but the form `%0c' is
not in query mode.  Since fields can only be qualified when the form is in
query mode, the query will apply to all rows."

#define ABQRYMODE       E_AR0025_ReqQueryMode

/*%
** Parameters:
**      %0c     the name of the currently running form.
**
** Explanation:
**      When a form is run in query mode, both values and operators can be
**      entered into fields.  These values and operators can then be used to
**      build a query using the 4GL qualification predicate.  No qualification
**      predicate can be constructed when the form is not in query mode.
**
** System Status:
**      The query will be executed with a predicate of "1 = 1" in place of
**      qualification predicate (meaning a predicate that is always true.)
**      This will result in all rows being retrieved by the query.
**
** User Action:
**      Make sure the mode of the form is set to query (using the SET_FORMS
**      statement from 4GL) whenever a qualification predicate is used for
**      a query.
*/

E_AR0026_RoleOverride   "Application role ID cannot be overridden.\n
The role ID for this application cannot be overridden by specifying a role ID on
the command-line.  The application will terminate."

/*%
** Explanation:
**      A role ID was specified on the command-line, but cannot override the
**      role ID with which the application was built.
**
** System Status:
**      The application will terminate as this is a security violation.
**
** User Action:
**      Do not attempt to override the application role ID by specifying one
**      one the command line to the application.
*/

E_AR0028_POSTOSQL       "Positional parameters passed to SQL procedure.\n
The SQL procedure `%0c' was called with parameters specified by position
(i.e., of the form arg1, arg2, ...).  An SQL procedure can only be called with
parameters passed by name (i.e., of the form param1 = arg1, param2 = arg2,
...)."

/*%
** Explanation:
**      SQL procedures do not define any positional parameters.  All
**      parameters passed to SQL procedures must be passed by name.
**
** System Status:
**      The value passed to the procedure will be ignored.
**
** User Action:
**      Pass parameters to SQL procedures by name only.
*/

E_AR004F_ABTRCFILE      "Could not open trace file.\n
ABF could not open the diagnostic trace file `%0c' in the temporary directory.
Please check your temporary directory access rights and disk space before
reporting this as a problem to your Site Technical Contact."

/*%
** Explanation:
**      DIAGNOSTIC.
**
** System Status:
**      ABF or ABFIMAGE will terminate immediately.
**
** User Action:
**      Make sure that the you have access rights to the temporary directory and
**      enough disk space to create and write files there before reporting this
**      as a problem to your site technical contact.
*/

E_AR0050        "The application `%0c' does not exist in the ABF catalogs."

/*%
** Explanation:
**
** System Status:
**      The application requested by the user could not be brought up.  ABF
**      will be brought up without an application or abfimage will be terminated
**
** User Action:
**      Check that name was spelled correctly.
*/

E_AR0051        "`%0c' is not a legal name for an application.  Application names can only contain letters and digits, and must be no more than 24 characters long."

/*%
** Explanation:
**
** System Status:
**      The application requested by the user could not be brought up.  ABF
**      will be brought up without an application or abfimage will be terminated
**
** User Action:
**      Check that name was spelled correctly.  Use a different name for a new
**      application that follows correct conventions.
*/

E_AR0052        "Cannot create temporary files.\n
Please check your temporary directory access rights and disk space before
reporting this as a problem to your Site Technical Contact."

/*%
** Explanation:
**      The temporary directory cannot have files created within it.  This could
**      be for several reasons, but most likely is because you do not have the
**      required access rights to the temporary directory or because you do not
**      have enough disk space available to create files in that directory.
**
**      The temporary directory is either the current directory or the directory
**      specified by the environment logical II_TEMPORARY.
**
** System Status:
**      ABF or ABFIMAGE will terminate immediately.
**
** User Action:
**      Make sure that the you have access rights to the temporary directory and
**      enough disk space to create and write files there before reporting this
**      as a problem to your site technical contact.
*/

E_AR0053_glo_undef "Global variable/constant `%0c' undefined.\n
A global variable or constant was not found."

/*%
** Explanation:
**      The application could not access a global variable or application
**      constant.  The variable/constant was probably deleted after the last
**      time the current 4GL frame or procedure was compiled.
**
** System Status:
**      The application is unable to initialize the frame or procedure, and
**      flow-of-control will return to the calling 4GL code.
**
** User Action:
**      Please recompile the 4GL code for the frame or procedure that is trying
**      to access the undefined global variable or application constant.
**      If this fails, report this as a problem to your site technical contact.
*/

E_AR0054_glo_mismatch "Global variable/constant `%0c' has wrong datatype.\n
`%0c' was expected to be `%1c', but was actually `%2c'.\
The frame or procedure needs to be recompiled."

/*%
** Explanation:
**      The application found a global variable or application constant, but it
**      was not the expected type.  The variable/constant was probably edited
**      after the last time the current 4GL frame or procedure was compiled.
**
** System Status:
**      The application is unable to initialize the frame or procedure, and
**      flow-of-control will return to the calling 4GL code.
**
** User Action:
**      Please recompile the 4GL code for the frame or procedure that is trying
**      to access the mismatched global variable or application constant.
**      If this fails, report this as a problem to your site technical contact.
*/

E_AR0055_bad_type "Type `%0c' not found.\n
The definition of class `%0c' was not found in this application.\n
The frame or procedure needs to be recompiled."

/*%
** Explanation:
**      The application found a variable declared to be a complex data type,
**      but the complex data type does not exist.  The data type was probably
**      deleted or changed after the last time the current 4GL frame or
**      procedure was compiled.
**
** System Status:
**      The application is unable to continue executing the frame or procedure.
**      Flow-of-control will return to the calling 4GL code.
**
** User Action:
**      Please recompile the 4GL code for any frames or procedures that use
**      this type definition.  If this fails, report this as a problem to
**      your site technical contact.
*/

E_AR0056_no_attribute "Field `%0c' not found in '%1c'.\n
The frame or procedure needs to be recompiled."

/*%
** Explanation:
**      The application found a reference to a field in a variable declared
**      to be a complex data type, but the complex data type does not have a
**      field with this name.  The complex data type was probably
**      changed after the last time the current 4GL frame or
**      procedure was compiled.
**
** System Status:
**      The application is unable to continue executing the frame or procedure.
**      Flow-of-control will return to the calling 4GL code.
**
** User Action:
**      Please recompile the 4GL code for any frames or procedures that use
**      this type definition.  If this fails, report this as a problem to
**      your site technical contact.
*/

E_AR0057_bad_index "Row number `%0c' out of range in array."

/*%
** Explanation:
**      The application found a reference to a row of an array, but the array
**      doesn't contain a row with that number.
**
** System Status:
**      The array reference will default to the highest-numbered row of the
**      array.  If the array is empty, the result of the array reference is
**      undefined.
**
** User Action:
**      This is a bug in the user's 4GL code.
*/

E_AR0058_typeerr "Could not assign `%0c' to `%1c'.\n
An INSERTROW command tried to assign a value of type `%0c' to
a value of type `%1c'.  These types are incompatible."

/*%
** Explanation:
**      The application tried to assign a value to another, but the
**      types of the values were not compatible.
**
** System Status:
**      The value of the parameter will be undefined.
**
** User Action:
**      Ensure that the type of the values match or are coercible.
*/

E_AR0059_notarray "Tried row operation on value of type `%0c'.\n
The application tried a row operation (INSERTROW, DELETEROW, or UNLOADTABLE)
on a value of type `%0c'.  Row operations can only be used on
arrays or table fields."

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

E_AR0060_class_fail "Internal failure in accessing class `%0c'.\n
Please report this to your Site Technical Contact."

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

E_AR0061_nullint
"An integer expression containing a NULL value was used in a `%0c' statement.
A NULL value is not legal in this context.  The expression should be changed
to be non-NULLable, or the ifnull function should be used.  The statement
will continue with a default value."

/*%
** Explanation:
**      Some statements that take integer expressions do not allow
**      NULL values.  For example, a table field reference can not
**      have a NULL value for the row number.  However, this error
**      is not detectable until run-time, since the expression may
**      evaluate to a non-NULL value.  The 4GL code needs to be
**      changed so that the expression guarantees a non-NULL value.
**
** System Status:
**      The statement continues with a default value.
**
** User Action:
**      Change the 4GL code so that a non-NULL value is produced.
*/

E_AR0062_not_scalar "Type `%0c' (of attribute `%1c') not
allowed in this context.  Don't use complex types in this context."

/*%
** Explanation:
**      Some statements that take class attributes cannot operate on
**      complex types.
**
** System Status:
**      The statement continues, but has no effect on this value.
**
** User Action:
**      Change the 4GL code so that only scalar data is referenced.
*/

E_AR0063_bad_convert_into "A datatype conversion error occurred while
putting a value into attribute `%0c' of class `%1c'"

/*%
** Explanation:
**      An operation tried to insert a value into an object or an array row.
**      The datatype of the object attribute did not match that of the
**      inserted value.
**
** System Status:
**      The statement continues, but has no effect on the object attribute.
**
** User Action:
**      Change the 4GL code so that an appropriate value is inserted.
*/

E_AR0064_bad_convert_from "A datatype conversion error occurred while
getting a value from attribute `%0c' of class `%1c'"

/*%
** Explanation:
**      An operation tried to get a value from an object or an array row.
**      The datatype of the object attribute did not match that of the
**      target variable.
**
** System Status:
**      The statement continues, but has no effect on the target variable.
**
** User Action:
**      Change the 4GL code so that an appropriate value is inserted.
*/

E_AR0065_BadHostArgType
"Invalid type of argument for a 3GL procedure.\n
An argument passed to the 3GL procedure '%0c' was of a datatype
that's not valid for a 3GL procedure (e.g. a record or an array).
The procedure will not be called."

/*%
** Explanation:
**      An argument of a complex datatype (e.g. record or array)
**      was erroneously passed to a 3GL procedure.
**
** System Status:
**      ABF will not call the 3GL procedure.
**
** User Action:
**      Modify the calling frame or procedure to pass different arguments
**      or to call a different (4GL) procedure.
*/

E_AR0066_NoDBSession
"Call to subsystem attempted with no active database connection.\n
The call to subsystem '%0c' will not be performed."

/*%
** Explanation:
**      The application must be switched to use an active database session
**      before any subsystems can be called. This also applies to "call
**      application" calls.  Use either the set_sql(session = ) statement
**      to switch to an existing connection, or use the 4GL CONNECT statement
**      to create a new connection.
**
** System Status:
**      The subsystem will not be called.
**
** User Action:
**      Make sure that the application is switched to an active session
**      before attempting to call a subsystem.
*/

E_AR0067_BadSessionID
"Invalid session identifier '%0c' specified in a %1c statement.\n
The %1c operation will not be performed."

/*%
** Explanation:
**      An invalid session identifier was specified as part of a CONNECT
**      or DISCONNECT statement - this value must be a positive integer.
**
** System Status:
**      The operation will not be performed.
**
** User Action:
**      Make sure that a valid session identifier is specified if you choose
**      to use the SESSION clause in the 4GL CONNECT and DISCONNECT statements.
*/

E_AR0068_BadConnName
"Invalid connection name '%0c' specified in a %1c statement.\n
The %1c operation will not be performed."

/*%
** Explanation:
**      An invalid connection name was specified as part of a CONNECT
**      or DISCONNECT statement - this value must not begin with the characters
**      "ii-".  This prefix is reserved for non-user sessions.
**
** System Status:
**      The operation will not be performed.
**
** User Action:
**      Make sure that a valid connection name is specified as part of
**      the 4GL CONNECT and DISCONNECT statements.
*/

E_AR0069_ConnectError
"Error establishing new database connection. The CONNECT operation has failed."

/*%
** Explanation:
**      A previous error has caused the CONNECT operation to fail.
**
** System Status:
**      The connect operation will not be performed and the application will
**      be no longer be switched to an existing database session.
**
** User Action:
**      Correct the cause of the CONNECT failure.
*/

E_AR006A_NOINIFRM_NODB  "Unable to initialize form for frame.\n
The form `%1c' for the frame `%0c' could not be initialized as there is no
current database session active.  The frame will not be executed."

/*%
** Explanation:
**      The ABF run-time system tried to initialize a form for a
**      frame while the application was not switched to an active database
**      session.  This may not be performed when running the image for frames
**      that have uncompiled forms.
**
** System Status:
**      The called frame will not be run.
**
** User Action:
**      The application must be switched to an active database session before
**      frames are called which have uncompiled forms, when the application
**      image is run.
*/

E_AR006B_DiscSystemSession
"Error attempting to disconnect the current session, which is a non-user
session; session identifier '%0c'."

/*%
** Explanation:
**      A DISCONNECT [CURRENT] statement was issued while the application was
**      switched to a non-user session (i.e. a session identified by a negative
**      session identifier).  This is not permissible.
**
** System Status:
**      The disconnect operation will not be performed.
**
** User Action:
**      Correct the cause of the invalid DISCONNECT statement.
*/

E_AR006C_DiscNoSession
"Error attempting to disconnect the current session, while no current session
is active."

/*%
** Explanation:
**      A DISCONNECT [CURRENT] statement was issued while the application was
**      not switched to a user session. This is not permissible.
**
** System Status:
**      The disconnect operation will not be performed.
**
** User Action:
**      Correct the cause of the invalid DISCONNECT statement.
*/

/*
**      Report Frame Messages ("abrtrw.qc")
*/
/* Menu Items */
F_AR0001_Report_Frame   "report frame"
F_AR0002_Batch          "batch"
/* Menu Explanations */
F_AR0003_RunReport_Explanation  "Run the report for this frame."
F_AR0004_Batch_Explanation      "Batch the report when it is run."

/* Help Title */
S_AR0023_RW_Help_Title  "Running a Report"

/*
** Names of ABF objects, used in error messages.
*/
F_AR0005_frame          "frame"
F_AR0006_procedure      "procedure"

/*
**      Unavailable Frame or Procedure Frame Messages ("abrtcall.qc")
*/

F_AR0015_CallFrame              "CallFrame"
F_AR0016_CallProc               "CallProc"

F_AR0007_CallFrameExpl          "Call another frame."
F_AR0008_Frame_to_call          "Frame to call"
F_AR0009_UndefinedFrame         "Unavailable Frame"
F_AR000A_CallProcExpl           "Call another procedure."
F_AR000B_Proc_to_call           "Procedure to call"
F_AR000C_UndefinedProcedure     "Unavailable Procedure"

/*
**      Graph Frame Messages ("abrtvig.qc")
*/

F_AR000D_DrawExpl               "Draw the graph for this frame."
S_AR0024_RunGraphHelpTitle      "Running a Graph"

/*
**      Data types used in error messages (on, for example, mismatched
**      parameter types).
*/
S_AR0030_floating_point "floating point"
S_AR0031_character      "character"
S_AR0032_none           "none"
S_AR0033_integer        "integer"

/*
** Lookup Frame Messages ("abrtlkup.qsc")
*/
F_AR0010_QualTitle      "Enter look-up qualification and select Go:"
F_AR0011_QualSelectExpl "Look-up values based on qualification."
F_AR0012_QualHelp       "Look-up Qualification"
F_AR0013_LookupHelp     "Look-up"
F_AR0014_Rtrving        "Retrieving look-up data . . ."

E_AR002A_NoQry  "Missing look-up query parameter.\n
The look-up frame requires a query to be specified as a parameter.  Please
consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      The look-up function requires a query to be specified using the
**      'ii_query' keyword parameter.  Otherwise, it cannot execute.
**
** System Status:
**      The look-up frame will not execute and no values will be returned.
**
** User Action:
**      Consult the INGRES ABF/4GL Reference Manual and then specify the
**      required query in the call to the look-up frame.
**
** Note: This message has been superseded by E_AR003A (as of 6.4).
*/

E_AR002B_UnknownColumn  "Unknown look-up column `%1c'.\n
The parameter `%0c' specifies a column that does not exist in the query.  Please
consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      If the parameter 'ii_query' is specified, then any parameter
**      of the form 'ii_field'n must specify a column of the query
**      specified by the 'ii_query' parameter.
**
** System Status:
**      The look_up frame will not execute and no values will be returned.
**
** User Action:
**      Consult the INGRES ABF/4GL Reference Manual and then correct the
**      parameters passed to the look-up frame in this instance.
*/

E_AR002C_MissingColumn  "Missing look-up display column parameter.\n
The column at position `%0d' has not been specified, but subsequent columns have
been.  Please consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      The parameters to the look-up frame did not specify all the columns to
**      be displayed.  This frame expects all columns that are to be displayed
**      to be specified by name through the ordinal position parameters,
**      'ii_fieldn', where n is the number of the displayed column.  They must
**      be contiguous between one and the number of columns to be displayed by
**      the look-up frame.  (It doesn't make sense otherwise.)
**
**      Either one or more columns were not specified (e.g., 'ii_field1',
**      'ii_field2', and 'ii_field4' were specified but 'ii_field3' was not),
**      or a title was specified through an 'ii_field_titlen' parameter but
**      the number of displayed columns was less than n (e.g., 'ii_field1' and
**      'ii_field2' and 'ii_field_title3' were specified.)
**
** System Status:
**      The look-up frame will not execute and no values will be returned.
**
** User Action:
**      Consult the INGRES ABF/4GL Reference Manual and then correct the
**      parameters passed to the look-up frame in this instance.
*/

E_AR002D_NoColumns      "Missing look-up display column parameters.\n
At least one display column parameter must be specified for the look-up frame.
Please consult the INGRES ABF/4GL Reference Manual."

E_AR002E_NotByReference "Missing BYREF expression for parameter.\n
The column parameter `%0c' must be passed by reference to the look-up frame.
Please consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      The look-up frame allows a user to select values from a list and then
**      returns them by reference to its calling parameters.  You have specified
**      one of these parameters but did not specify its value using the by
**      reference function, BYREF().  Because of this, the look-up frame will
**      be unable to return a value to that parameter.
**
** System Status:
**      The look-up frame will not execute and no values will be returned.
**
** User Action:
**      Specify the parameter by reference by adding the BYREF function
**      to the calling value.  For example, "column = BYREF(parameter)"
**      instead of "column = parameter".
*/

E_AR002F_NoValuesReturned       "Missing column parameters.\n
A query was specified, but no column values were returned by reference
for the look-up frame.  Please consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      The look-up frame allows a user to select values from a list
**      and then returns them by reference to some of its calling parameters.
**      If none of these parameters were specified, but a query was specified
**      (rather than an array), then the look-up frame cannot return any values.
**      Since by calling the look-up frame you must have intended some values
**      to be returned, it is an error not to have specified any return values
**      by reference.
**
** System Status:
**      The look-up frame will not execute and no values will be returned.
**
** User Action:
**      Consult the INGRES ABF/4GL Reference Manual and then specify the
**      desired return values as by reference parameters.
*/

/*
** More Lookup Frame Messages ("abrtlkup.qsc")
** These were added as part of the support for allowing
** the Look_up frame to get values from an array.
** They don't following the previous Look_up message (E_AR002F)
** contiguously, because S_AR0030 thru S_AR0033 are being used
** for data types in error messages.  I have left AR0034 thru AR0037
** unused on the theory that we might mention more datatypes someday.
*/

E_AR0038_NoLookupinQUEL
"The look_up function is not supported in QUEL.\n
Please consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      The look_up function is not supported in QUEL. A look_up call
**      was detected in a QUEL frame.
**
** System Status:
**      The look_up frame will not execute and no values will be returned.
**
** User Action:
**      Remove the call to look_up from this QUEL frame or procedure.
*/

E_AR0039_BothQueryAndArray
"Both query and array parameter specified for look_up.\n
The look_up frame requires a query or array to be specified as a parameter,
but not both.  Please consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      The look_up function requires a query or array to be specified
**      using the 'ii_query' or 'ii_array' keyword parameter, but not both.
**      Otherwise, it cannot execute.
**
** System Status:
**      The look_up frame will not execute and no values will be returned.
**
** User Action:
**      Consult the INGRES ABF/4GL Reference Manual and then specify the
**      required query or array in the call to the look-up frame.
*/

E_AR003A_NoQueryOrArray "No query or array parameter specified for look_up.\n
The look_up frame requires a query or array to be specified as a parameter.
Please consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      The look_up function requires a query or array to be specified
**      using the 'ii_query' or 'ii_array' keyword parameter.
**      Otherwise, it cannot execute.
**
** System Status:
**      The look_up frame will not execute and no values will be returned.
**
** User Action:
**      Consult the INGRES ABF/4GL Reference Manual and then specify the
**      required query or array in the call to the look-up frame.
*/

E_AR003B_UnknownAttr    "Unknown look_up attribute `%1c'.\n
The parameter `%0c' specifies an attribute that either does not exist
in the array or else is an attribute of a complex type (a record or array).
Please consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      If the parameter 'ii_array' is specified, then any parameter
**      of the form 'ii_field'n must specify a simple attribute of the array
**      specified by the 'ii_array' parameter.
**
** System Status:
**      The look_up frame will not execute and no values will be returned.
**
** User Action:
**      Consult the INGRES ABF/4GL Reference Manual and then correct the
**      parameters passed to the look-up frame in this instance.
*/

E_AR003C_BothQualifyAndArray
"Both qualify and array parameter specified for look_up.\n
The look_up frame allows user query qualification
only if a query is specified as a parameter.
Please consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      The optional parameter 'ii_qualify' may be specified for the look_up
**      function only if a query is specified using the 'ii_query' parameter.
**
** System Status:
**      The look_up frame will not execute and no values will be returned.
**
** User Action:
**      Consult the INGRES ABF/4GL Reference Manual and then correct the
**      parameters passed to the look-up frame in this instance.
*/

E_AR003D_NotArray       "Array parameter does not specify an array.\n
The `ii_array' parameter must specify an array.
Please consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      The parameter 'ii_array' must specify an array.
**
** System Status:
**      The look_up frame will not execute and no values will be returned.
**
** User Action:
**      Consult the INGRES ABF/4GL Reference Manual and then correct the
**      parameters passed to the look-up frame in this instance.
*/

E_AR003E_BadColNum      "Invalid display column number `%0c'.\n
The parameter `%1c' is not a valid display column specification:
`%2c' must be followed by a positive number no greater than
the number of columns or attributes in the query or array.
Please consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      If a parameter name begins with 'ii_field' or 'ii_field_title',
**      then the full name must be of the form 'ii_field'n or 'ii_field_title'n,
**      where n is a positive number no greater than
**      the number of columns or attributes in the query or array.
**
** System Status:
**      The look_up frame will not execute and no values will be returned.
**
** User Action:
**      Consult the INGRES ABF/4GL Reference Manual and then correct the
**      parameters passed to the look-up frame in this instance.
*/

E_AR003F_DuplicateCols  "Duplicate display column parameters.\n
The parameters `ii_field%0d' and `ii_field%1d' both specify `%2c'.
Two different display column parameters may not specify
the same column or attribute.
Please consult the INGRES ABF/4GL Reference Manual."

/*%
** Explanation:
**      Two different parameters of the form 'ii_field'n may not specify
**      the same column or attribute of the query or array.
**
** System Status:
**      The look_up frame will not execute and no values will be returned.
**
** User Action:
**      Consult the INGRES ABF/4GL Reference Manual and then correct the
**      parameters passed to the look-up frame in this instance.
*/

/*
**      Types of frames in ABF.
*/
S_AR0040_QBF            "QBF"
S_AR0041_REPORT         "REPORT"
S_AR0042_GBF            "GBF"
S_AR0043_RUNGRAPH       "RUNGRAPH"

/*
**      Types of frames, as displayed in the call stack of frames
**      in the Undefined Frame and Undefined Procedure frames.
*/

S_AR0049_LOCALPROC      "LOCAL PROC"
S_AR004A_USER           "USER"
S_AR004B_GRAPHICS       "GRAPHICS"
S_AR004C_PROC           "PROC"
S_AR004D_UNDEF          "UNDEF"

/*
**      String appended to a data format to show that it is Nullable.  Used
**      in error messages.  ("abrtutil.qc")
*/
S_AR004E__with_NULL     " with NULL"

/*
** abrtseqv.sc
*/
E_AR0070_NotI4column "Column '%0c' is not a 4-byte integer\n
The column for which the sequence_value procedure generates a new value must be
a 4-byte integer.  Column '%0c' in table '%1c' is not.  The table definition
must be changed before sequence_value will work."

/*%
** Explanation:
**      The sequence_value procedure was called for a column which is not a
**      4-byte integer.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was
**      generated.
**
** User Action:
**      Either change the table definition or remove the call to sequence_value.
*/

E_AR0071_BadMaxGet "Errors occurred reading the current maximum value.\n
There is no record in ii_sequence_values for column '%0c' in table '%1c'.
This makes it necessary to determine the current maximum value for this column.
The previously displayed database errors make this impossible.  No new
value will be generated."

/*%
** Explanation:
**      Database errors occurred trying to determine the maximum value of the
**      specified column.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was
**      generated.
**
** User Action:
**     Determine the cause of the errors and eliminate them.
*/

E_AR0072_NoSqv_Parameter        "The parameter '%0c' is missing.\n
The mandatory parameter '%0c' was not passed to the sequence_value procedure,
or an empty string was passed.  No new value will be generated."

/*%
** Explanation:
**      Either the table_name or column_name were not passed to the
**      sequence_value procedure, or an empty string was passed.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was
**      generated.
**
** User Action:
**      Specify valid table and column names.
*/

E_AR0073_BadSqvNumber   "The '%0c' argument must be positive.\n
The '%0c' argument to the sequence_value procedure must be a positive number.
'%1d' was actually passed in.  No new value will be generated."

/*%
** Explanation:
**      Either the increment or start_value were passed in as a negative number
**       or zero to the sequence_value procedure.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was
**      generated.
**
** User Action:
**      Specify valid values, or let them default to 1.
*/

E_AR0074_Sqv_DBErr      "Database error(s) occurred in sequence_values\n
The previous error messages described errors which occurred in the
sequence_values procedure while attempting to access the ii_sequence_values
tables.  No new value will be generated."

/*%
** Explanation:
**      Database errors occurred while trying to access the ii_sequence_values
**      table.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was
**      generated.
**
** User Action:
**      Determine why the errors occurred, and eliminate them.
*/

E_AR0075_NoSuchTable    "Table '%0c' does not exist\n
Table '%0c' was passed to the sequence_value procedure.  No such table exists,
so no new value can be generated for it."
/*%
** Explanation:
**    The specified table does not exist.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was
**      generated.
**
** User Action:
**      Create the table, or correct the call to sequence_value.
*/


E_AR0076_ReqTFQueryMode "Warning:  Table field not in query mode.\n
A qualification predicate has been specified for a query which involves columns
from a table field, but the table field `%1c' on the form `%0c'is
not in query mode.  The qualifications from this table field will be ignored."

#define ABTFQRYMODE     E_AR0076_ReqTFQueryMode

/*%
** Parameters:
**      %0c     the name of the currently running form.
**      %1c     the name of the table field which was qualified upon
**
** Explanation:
**      When a table field is in query mode, both values and operators can be
**      entered into its columns.  These values and operators can then be used
**      to build a query using the 4GL qualification predicate.  No
**      qualification predicate can be constructed when the table field is
**      not in query mode.
**
** System Status:
**      The query will be executed, but the qualifications upon the table field
**      will be ignored.
**
** User Action:
**      Make sure the mode of the table field is set to query (using the
**      SET_FORMS or INITTABLE statement from 4GL) whenever a qualification
**      predicate involving it is used for a query.
*/

E_AR0080_StartObjNotFound       "Starting frame or procedure not found\n
You have requested that the application be started at `%0c',
which is neither a frame nor a procedure."
/*%
** Explanation:
**      You have requested that an imaged ABF application be started
**      at a specific frame or procedure, but it could not be found.
**
** System Status:
**      The application will terminate.
**
** User Action:
**      Restart the application, specifying a starting frame or procedure
**      that exists.
*/

E_AR0081_StartProcNotFound      "Starting procedure not found\n
You have requested that the application be started at procedure `%0c',
but it could not be found."
/*%
** Explanation:
**      You have requested that an imaged ABF application be started
**      at a specific procedure, but it could not be found.
**
** System Status:
**      The application will terminate.
**
** User Action:
**      Restart the application, specifying a starting frame or procedure
**      that exists.
*/

E_AR0082_BatchStartFrame        "Starting frame requires forms mode\n
You have requested that the application be started at `%0c', which is a frame,
but you have also requested that the application not be started in forms mode."
/*%
** Explanation:
**      You have requested that an imaged ABF application be started
**      at a specific frame, but you have also requested that the
**      application not be started in forms mode.  Frames require
**      forms mode.
**
** System Status:
**      The application will terminate.
**
** User Action:
**      Restart the application, specifying a starting procedure rather than
**      a starting frame, or else don't request that the application not be
**      started in forms mode.
*/

E_AR0083_BatchCallframe         "Callframe requires forms mode\n
The application you are running has issued a CALLFRAME `%0c',
but you have requested that the application not be started in forms mode."
/*%
** Explanation:
**      The application has issued a CALLFRAME statement, but you have
**      requested that the application not be started in forms mode.
**      Frames require forms mode.
**
** System Status:
**      The application will terminate.
**
** User Action:
**      Don't request that the application not be started in forms mode,
**      or start the application specifying a different starting procedure
**      that won't issue any CALLFRAME statements,
**      or revise the application to ensure that the starting procedure
**      you specified won't issue any CALLFRAME statements.
*/

E_AR0084_CalledProcNotFound     "Called procedure not found\n
The application you are running has issued a CALLPROC `%0c',
but it could not be found."
/*%
** Explanation:
**      The application has issued a CALLPROC statement,
**      but the requested procedure could not be found.
**      Since you have requested that the application not be started
**      in forms mode, you will not be prompted for a different procedure
**      to call.
**
** System Status:
**      The application will terminate.
**
** User Action:
**      Revise the application to ensure that the CALLPROC statement
**      specifies a procedure that exists.
*/

E_AR0085_OLpcall_failed "Host Language Procedure call failed\n
The call to Host Language Procedure, `%0c', failed. This probably
means that too many arguments were passed, but could also mean that
there was an error in passing some argument."
/*%
** Explanation:
**      On Unix, this means that the maximum number of arguments allowed
**      to be passed has been exceeded. On VMS it could mean that, or it
**      could mean that there was some problem passing some argument to
**      the Host Language Procedure.
**
** System Status:
**      The Host Language Procedure was not called. Execution continues.
**
** User Action:
**      Change the application to pass fewer arguments. A good target for
**      number of args to pass would be less than 40 for Unix, less than
**      256 for VMS.
*/
E_AR0086_Invalid_Inq4gl_cnst        "Invalid attribute `%0c' for the 4GL statement `%1c'.\n
Please refer to the INGRES ABF/4GL Reference Manual for valid attribute name."
/*%
** Explanation:
**      The attribute name was passed in as an invalid name.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was
**      generated.
**
** User Action:
**      Consult INGRES ABF/4GL Reference Manual for valid attribute
*/
E_AR0087_Unmatched_datatype   "The datatype of the %0c is not compatible with the attribute `%1c' in the 4GL statement `%2c'.\n
Please consult the INGRES ABF/4GL Reference Manual."
/*%
** Explanation:
**      The datatype specified is not compatible with the attribute.
**
** System Status:
**      A value of zero will be returned, indicating that no new value was
**      generated.
**
** User Action:
**      Consult INGRES ABF/4GL Reference Manual to correct the field datatype
*/

E_AR0088_divzero
"An attempt to divide by 0 occurred within an expression."

/*%
** Explanation:
**
** System Status:
**      The result of the expression is undefined.
**      The current 4GL statement is skipped.  Execution
**      continues with the next 4GL statement in the
**      frame/procedure.
**
** User Action:
*/

E_AR0089_numovf
"A numeric overflow or underflow occurred within an expression."

/*%
** Explanation:
**
** System Status:
**      The result of the expression is undefined.
**      The current 4GL statement is skipped.  Execution
**      continues with the next 4GL statement in the
**      frame/procedure.
**
** User Action:
*/

E_AR0090_fltbad
"An error occurred in evaluating floating point numbers
within an expression.  For example, there may have been numeric
overflow or underflow, or an attempt to divide by 0."

/*%
** Explanation:
**
** System Status:
**      The result of the expression is undefined.
**      The current 4GL statement is skipped.  Execution
**      continues with the next 4GL statement in the
**      frame/procedure.
**
** User Action:
*/

E_AR0130_HashInit       "Initialization of the hash table for open files failed. This may be due to lack of memory."

E_AR0131_MissingParam   "One or more parameters are missing."

E_AR0132_BadFileType    "Unsupported file type `%0c'. Supported types are `text', `stream' and `binary'."

E_AR0133_BadFileMode    "Unsupported file mode `%0c'. Supported modes are `append', `read', `update' and `create'."

E_AR0134_BadParam       "Invalid parameter `%0c'."

E_AR0135_BadUsrHandle   "Please provide a positive number as a handle. Negative numbers are reserved for auto-generation of handles. Zero is reserved."

E_AR0136_OpenFailed     "Failed to open `%0c'. Check open file limit and/or memory. System status: `%1d'."

E_AR0137_AllocFail      "Failed to allocate a piece of memory. Check memory."

E_AR0138_HashEntry      "Failed to enter file information into hash table. Check memory."

E_AR0139_NoOpenFile     "There are no open files."

E_AR013A_BadFileName    "Incorrect file name `%0c'."

E_AR013B_BadName        "Could not map file name `%0c' to proper location. Check validity of file name."

E_AR013C_AlreadyOpen    "File `%0c' is already open with handle `%1d'."

E_AR013D_BadValue       "Parameter `%0c' does not take value `%1c'."

E_AR013E_WriteToRead    "File `%0c' was opened for `read'. You may not write."

E_AR013F_ReadOnWrite    "File `%0c' was opened for `create'. You may not read."

E_AR012F_ReadOnAppend   "File `%0c' was opened for `append'. You may not read."

E_AR0140_LookupFailed   "The given handle `%0d' does not map to any open file."

E_AR0141_SeekFailed     "Failed to position file `%0c'. System status: `%1d'."

E_AR0142_FlushFailed    "Failed to flush file `%0c'. System status: `%1d'."

E_AR0143_HashDelete   "Failed to delete file information from hash table. Check memory."

E_AR0144_TellFailed     "Failed to get the current position of the file."

E_AR0145_ReadFlush      "No need to flush `read only' file."

E_AR0146_SeekPastEOF    "You may not position past end of file."

E_AR0147_SeekAheadBOF   "You may not position ahead of beginning of file."

E_AR0148_EndOfFile      "End of file."

E_AR0149_NullData       "Processing of NULL data in `text' files is not currently supported."

E_AR014A_BadRecSize     "Record size of binary files must be greater than zero."

E_AR014B_BadRewind      "Cannot rewind `append' and `create' mode files."

E_AR014C_RecText        "Inquiring record number on `text' and `stream' files not supported."

E_AR014D_SizeMismatch   "The given recordsize does not match the total size of item list."

E_AR014E_PosText        "Setting and Inquiring position on `text' files is not supported."

E_AR014F_MutualXor      "The `offset' and `record' cannot be used together."

E_AR0150_BadRecNum      "Zero is an invalid record number."

E_AR0151_BadItemList    "Non-string data types are not allowed in `text' files. Use either CHAR or VARCHAR."

E_AR0152_WriteFailed    "Failed to write to file `%0c'. Check disk space and open file limit."

E_AR0153_TextUpdate     "Update of `text' files is not supported."

E_AR0154_RecOrOff       "Either `record' or `offset' must be specified."

E_AR0155_NonRecFile     "Cannot position to record on a non-record oriented file."

E_AR0156_Exception      "Could not read the data properly. This may have been caused by writing improperly formatted data to binary or stream file."

E_AR0157_MaxRecSize     "The maximum record size for `text' files is 4096."

E_AR0158_FileNotFound   "File `%0c' does not exists or is not accessible"

E_AR0159_UnseenDelim    "Was not able to find the delimiter `%0c' while scanning for an item."

E_AR015A_BinReclenNotProv
"The record length for Binary file '%0c' has not been provided yet.
This prevents the Offset or Recordnum value that is returned from
being calculated as an absolute value from the beginning of the file.
The record length can be specified by the recordsize parameter of the
OpenFile statement or implicity via a ReadFile or WriteFile request."

/*%
** Explanation:
**
** System Status:
**      The resultant Offset or Recordnum value is returned.            n
**      Normal execution continues, but the values being returned for
**      the Binary file may not be the absolute values, as calculated
**      from the beginning of the file, that the program expects.
**
** User Action:
**      Ensure that for Binary files the record length has been provided
**      either explicitly on the OpenFile statement or implicitly via   h
**      a ReadFile or WriteFile statement before requesting InquireFile.
*/

