/*
** Copyright (c) 2004 Ingres Corporation
*/
SQLSTATE_MAPPING_REQUIRED

/*
**    Message file for Facility code: _GL
**
**    History:
**
**	16-feb-1993 (dianeh)
**		Added NO_SQLSTATE_MAPPING to end of file.
**	26-jul-1993 (tyler)
**		Added E_GL6001_PM_OPEN_FAILED message.
**	22-oct-1993 (dianeh)
**		Refine text destined for messages.txt file.
**	07-dec-1993 (tyler)
**		Added loggable STATUS values for PM functions.
**	6-Jan-1994 (daveb)
**	    	Changed typo in PM message number: S...6000 should be E...6000
**	19-jun-1996 (sweeney)
**		Fix a couple of typos.
**	16-oct-96 (mcgem01)
**		MISC_ING_ERRORS changed to MISC_ERRORS for the Jasmine/Ingres
**		identity crisis.
**	22-Nov-00 (loera01) Bug 103318
**		Added E_GL600B_PM_DUP_INIT to SS50000_MISC_ERRORS to give more
**		granuality to duplicate attempts at calling PMinit().
**	09-May-02 (gupsh01) 
**		Added error codes, E_GL7001_E_LC_LOCALE_NOT_FOUND, 
**		E_GL7002_E_LC_CHARSET_NOT_SET and E_GL7003_E_LC_FORMAT_INCORRECT
**		for lc facility.
**	28-May-02 (gupsh01)
**		Modified the error message E_GL7001_E_LC_LOCALE_NOT_FOUND, to 
**		report that we could not find the IANA name for the charset.
*/

#define _GL_CLASS  213

E_GL4001_MO_ALREADY_ATTACHED:SS42502_DUPL_OBJECT
"MOattach was given a classid and instance that was already attached."
/*%
** Parameters:
**      None.
**
** Explanation:
**      The internal MOattach() routine detected an attempt to duplicate
**      an MO object that was already attached.  This isn't allowed, and
**      the original object was left intact.  This is a programming error.
**
** System Status:
**      The MO object space may not be the way the programmer intended.
**      Depending on the object, this may or may not affect the system
**      status.
**
** Recommendation:
**      None.
*/
E_GL4003_MO_BAD_MONITOR:SS50000_MISC_ERRORS
"MOset_monitor was given a NULL monitor for a {class,mon_data} that
did not have a monitor to delete."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL4004_MO_BAD_MSG:SS50000_MISC_ERRORS
"An MO method or monitor function was given a message type that
it didn't understand."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL4005_MO_BAD_SIZE:SS50000_MISC_ERRORS
"An MO method was given an object size that it was not prepared to handle."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL4006_MO_CLASSID_TRUNCATED:SS50000_MISC_ERRORS
"An MO function or client method was given an output buffer for the
classid that was too small for the value it needed to write.  It
truncated the result to fit."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL4008_MO_INSTANCE_TRUNCATED:SS50000_MISC_ERRORS
"An MO function or client method was given an output buffer for the
instance that was too small for the value it needed to write.  It
truncated the result to fit."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL4009_MO_NO_CLASSID:SS5000H_UNAVAILABLE_RESOURCE
"An MO function did not find a defined class when it expected one to exist."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL400A_MO_NO_DETACH:SS42503_INSUF_PRIV
"MOdetach was called for an object whose class definition prohibited
detach calls.  It was left attached."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL400B_MO_NO_INSTANCE:SS5000H_UNAVAILABLE_RESOURCE
"An MO function could not locate an an object instance that it
expected to find, aborting processing."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL400C_MO_NO_NEXT:SS01000_WARNING
"An MO function has determined there ar no successors to an object.
This usually terminates a normal scan, and is not an error."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL400D_MO_NO_READ:SS42503_INSUF_PRIV
"An MO function attempted to get a value for an object whose class
definition prohibited read access.  No value was obtained."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL400E_MO_NO_STRING_SPACE:SS50003_EXHAUSTED_RESOURCE
"MO tried to allocate space for a string, but ran into it's programmed
limit.  The allocation did not succeed.  The string space limit can be
altered by setting the 'exp.gl.glf.mo.string.limit' object."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL400F_MO_NO_WRITE:SS42503_INSUF_PRIV
"An MO function attempted to set the value of an object whose class
definition prohibited write access.  The object was not modified."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL4010_MO_NULL_METHOD:SS50000_MISC_ERRORS
"A class definition handed to MOclassdef contained NULL method
pointers where a non-NULL method was required.  The definition was
not made, and subsequent definitions were not attempted."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL4011_MO_VALUE_TRUNCATED:SS50000_MISC_ERRORS
"An MO function or client method was given an output buffer for the
value that was too small for what it needed to write.  It truncated
the result to fit."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL4012_MO_MEM_LIMIT_EXCEEDED:SS50003_EXHAUSTED_RESOURCE
"An MO function tried to allocate memory that would have exceeded the
programmed MO memory limit.  The allocation did not succeed.  The
memory limit may be altered by setting the 'exp.gl.glf.mo.mem.limit'
object."
/*%
** Parameters:
**      None.
**
** Recommendation:
**      None.
*/
E_GL5001_MU_UNUSED_ERR:SS50000_MISC_ERRORS
"Unused MU error"
E_GL5002_MUDEF_SEM_INITIALIZED:SS50000_MISC_ERRORS
"Internal error: attempt to initialize a default sem that is active."
E_GL5003_MUDEF_REM_NON_DEF:SS50000_MISC_ERRORS
"Internal error: attempt to remove an active default semaphore."
E_GL5004_MUDEF_P_NON_DEF:SS50000_MISC_ERRORS
"Internal error:  attempt to lock non-default sem by default code."
E_GL5005_MUDEF_P_DEADLOCK:SS50000_MISC_ERRORS
"Internal error:  DEADLOCK - attempt to lock default semaphore that is already held."
E_GL5006_MUDEF_V_NON_DEF:SS50000_MISC_ERRORS
"Internal error:  attempt to release a non-default sem by the default code."
E_GL5007_MUDEF_V_BAD_VAL:SS50000_MISC_ERRORS
"Internal error:  attempt to release a default semaphore with a bad value."
E_GL5008_MUCS_SEM_INITIALIZED:SS50000_MISC_ERRORS
"Internal error:  attempt to reinitialize a CS MU semaphore."
E_GL5009_MUCS_SEM_OVER_CHANGE:SS50000_MISC_ERRORS
"Internal error:  MU semaphore held over change to CS semaphores."
E_GL500A_MUCS_P_SEM_BAD_STATE:SS50000_MISC_ERRORS
"Internal error: Attempt to lock a corrupt MU semaphore by the CS support."
E_GL500B_MUCS_V_SEM_BAD_STATE:SS50000_MISC_ERRORS
"Internal error: Attempt to release a corrupt MU semaphore by the CS support."
E_GL500C_MUCS_R_SEM_BAD_STATE:SS50000_MISC_ERRORS
"Internal error: Attempt to remove a corrupt MU semaphore by the CS support."

E_GL6000_PM_SYNTAX_ERROR:SS50000_MISC_ERRORS
"Syntax error at line %0d of \"%1c\"."
E_GL6001_PM_OPEN_FAILED:SS50000_MISC_ERRORS
"Unable to read \"%0c\"."
E_GL6002_PM_BAD_FILE:SS50000_MISC_ERRORS
"A syntax error was detected in a PM resource file."
E_GL6003_PM_BAD_INDEX:SS50000_MISC_ERRORS
"An invalid index for a resource name component was passed
to a PM function."
E_GL6004_PM_FOUND:SS50000_MISC_ERRORS
"A request to insert a duplicate PM resource name was denied."
E_GL6005_PM_NOT_FOUND:SS50000_MISC_ERRORS
"A non-existent resource name was passed to a PM function which requires
the name of an existing resource."
E_GL6006_PM_BAD_REGEXP:SS50000_MISC_ERRORS
"An invalid regular expression was passed to a PM function."
E_GL6007_PM_NO_INIT:SS50000_MISC_ERRORS
"Internal error: a PM function was called before the corresponding context
was initialized." 
E_GL6008_PM_NO_II_SYSTEM:SS50000_MISC_ERRORS
"Unable to locate default PM resource file(s) since %0c is undefined."
E_GL6009_PM_BAD_REQUEST:SS50000_MISC_ERRORS
"An invalid resource request was passed to a PM function."
E_GL600A_PM_NO_MEMORY:SS50000_MISC_ERRORS
"Memory allocation failed within a PM function."
E_GL600B_PM_DUP_INIT:SS50000_MISC_ERRORS
"PM internal context has already been initialized."
E_GL7001_E_LC_LOCALE_NOT_FOUND:SS50000_MISC_ERRORS
"No matching IANA name is found for II_CHARSET value supplied."
E_GL7002_E_LC_CHARSET_NOT_SET:SS50000_MISC_ERRORS
"No charset given and II_CHARSET is not set for the server."
E_GL7003_E_LC_FORMAT_INCORRECT:SS50000_MISC_ERRORS
"Syntax error at line %0d, invalid file format for xmlnames.map file \"%0d\"."


NO_SQLSTATE_MAPPING
