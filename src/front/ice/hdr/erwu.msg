/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: erwu.msg
**
** Description:
**      Messages for ice utilities.
**
** History:
**      16-Jan-2000 (fanra01)
**          Bug 100036
**          Created.
**      21-Mar-2001 (fanra01)
**          S103813
**          Add messages for ICE configuration utilities.
**      04-Apr-2001 (peeje01)
**          SIR 103810
**          Add messages for ICE Stack.
**      04-May-2001 (fanra01)
**          Sir 103813
**          Add a usage message for the icevar utility.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
*/

# define    _WU_CLASS   264     /* class previously for ice 2.0 install */
                                /* now taken for ice 2.5 ice utilities  */

E_WU0001_DBUSER_USAGE
"\nUsage:\n\n    icedbuser [-av] <action> <alias> <dbuser> <pwd1> <pwd2> <\"comment\">\n\n
        -a          alias and dbuser are to be identical.  dbuser is omitted.\n
        -v          pwd1 has been verfied.  pwd2 is omitted.\n
        action      select/insert/update/delete.\n
        alias       name used by ice for database user.\n
        dbuser      Ingres database user. Omitted if -a specified.\n
        pwd1        Ingres database user password.\n
        pwd2        confirmation of pwd1. Omitted if -v specified.\n
        comment     quote delimited comment string\n"

/*%
** Explanation:
**      An incorrect number of command line arguments have been passed
**      to the utility.
**
** System status:
**      Utility is terminated.
**
** User action:
**      Use the correct number of command line arguments
*/

E_WU0002_UNKNOWN_OPTION
"An unknown option was encountered and will be ignored.\n"

/*%
** Explanation:
**      An unknown option was found in the command line.
**
** System status:
**      Utility will continue and ignore the unknown option.
**
** User action:
**      Review the options and remove the unknown option.
*/

E_WU0003_INVALID_ACTION
"The specified action has not been recognized.\n
Valid actions are select/insert/update/delete.\n"

/*%
** Explanation:
**      The action specified on the command line was not recognised.
**
** System status:
**      Utility will terminate.
**
** User action:
**      Use either select/insert/update or delete.
*/

E_WU0004_DUPLICATE_ENTRY
"An entry with the specified alias is already defined.\n"

/*%
** Explanation:
**      The alias name specified has already been defined.
**
** System status:
**      Utility will terminate.
**
** User action:
**      Ensure that the alias name is unique.
*/

E_WU0005_ICE_API
"\nError: %%s\n"

S_WU0006_ADMIN_USER
"Admin User:\n"

S_WU0007_ADMIN_PASSWORD
"Password:\n"

S_WU0008_STATUS_MESSAGE
"User %%-20s ID %%8s %%s\n"

E_WU0009_UNNAMED_TARGET
"The specified action requires a target\n"

/*%
** Explanation:
**      The action specified on the command line did not contain a target
**
** System status:
**      Utility will terminate.
**
** User action:
**      Re-run the command specifying the name.
*/

E_WU000A_SVRLOC_USAGE
"\nUsage:\n\n    icesvrloc [-hip] <action> <name> <path> <\"extension\">\n\n
    -h          specifies an HTTP location.  path is HTTP relative.\n
    -i          specifies an ICE location. Complete physical path.\n
    -p          specifies a public location.\n
    action      select/insert/update/delete.\n
    name        name of location.\n
    path        path of specified location.\n
    extension   associates an extension with path\n"

/*%
** Explanation:
**      An incorrect number of command line arguments have been passed
**      to the utility.
**
** System status:
**      Utility is terminated.
**
** User action:
**      Use the correct number of command line arguments
*/

E_WU000B_INVALID_OUTPARM
"A byref parameter passed to an API function was not initialized for use.\n"

/*%
** Explanation:
**      An output parameter passed to an ICE API function has not been
**      initialized.
**
** System status:
**      API error status
**
** User action:
**      The address of the parameter should be supplied to the API.
*/

E_WU000C_INVALID_INPPARM
"An input parameter passed to an API function was not initialized.\n"

/*%
** Explanation:
**      An input parameter passed to an ICE API function has not been
**      initialized or iceapi function context has not been setup.
**
** System status:
**      API error status
**
** User action:
**      Check the code containing parameters passed to the ICE API and retry.
*/

E_WU000D_VALUE_NULL
"The requested field does not contain a value.\n"

/*%
** Explanation:
**      The action parameter does not contain a value.
**
** System status:
**      API error status
**
** User action:
**      Check that the action has been initialized and retry.
*/

E_WU000E_UNKNOWN_FIELD
"The requested parameters is not a member of the specified function"

/*%
** Explanation:
**      The function does not contain the specified parameter as a member
**      of its parameter list.
**
** System status:
**      API error status
**
** User action:
**      Check the list of valid parameters for the function or check the
**      spelling of the parameter name.
*/

E_WU000F_INVALID_INPPARAM
"An input parameter passed to an API function was not initialized.\n"

/*%
** Explanation:
**      An input parameter passed to an ICE API function has not been
**      initialized or an invalid handle has been passed.
**
** System status:
**      API error status
**
** User action:
**      Check the code containing parameters passed to the ICE API and retry.
*/

E_WU0010_INVALID_OUTPARAM
"A byref parameter passed to an API function was not initialized for use.\n"

/*%
** Explanation:
**      An output parameter passed to an ICE API function has not been
**      initialized.
**
** System status:
**      API error status
**
** User action:
**      The address of the parameter should be supplied to the API.
*/

E_WU0011_INVALID_USERNAME
"The name parameter of the ic_initialize function was not initialized\n"

/*%
** Explanation:
**      The username parameter of ic_initialized expects a string for the
**      name.  The pointer passed has not been initialized.
**
** System status:
**      API error status
**
** User action:
**      Check the setup of the name and the parameter being passed to
**      ic_initialize.
*/

E_WU0012_INVALID_PASSWORD
"The password parameter of the ic_initialize function was not initialized\n"

/*%
** Explanation:
**      The password parameter of ic_initialized expects a string for the
**      password.  The pointer passed has not been initialized.
**
** System status:
**      API error status
**
** User action:
**      Check the setup of the password and the parameter being passed to
**      ic_initialize.
*/

E_WU0013_OUT_OF_DATA
"Data for this result set has been exhausted.\n"

/*%
** Explanation:
**      The getnext function is attempting to get the next tuple in the
**      result set and has received notification that the result set is
**      exhausted.
**
** System status:
**      API error status
**
** User action:
**      None.
*/

E_WU0014_DATASET_EXHAUSTED
"All rows for this request have been returned.\n"

/*%
** Explanation:
**      The getnext function has previously returned and OUT_OF_DATA status
**      and has been recalled before a new query has been issued.
**
** System status:
**      API error status
**
** User action:
**      Submit a new query before trying to fetch values.
*/

E_WU0015_INVALID_FNAME
"The supplied function name was differet to the stored context.\n"

/*%
** Explanation:
**      An attempt to perform an operation on a function context failed because
**      the supplied function name differs from the one in the structure.
**
** System status:
**      API error status
**
** User action:
**      Check the function name on which to perform the action matches the
**      provided context function.
*/

E_WU0016_ROW_COMPLETE
"All values in the tuple have been returned.\n"

/*%
** Explanation:
**      The getnxtval function has reached the end of the tuple returning
**      values.
**
** System status:
**      API error status
**
** User action:
**      None.
*/

E_WU0017_INVALID_UNIT_NAME
"The specified unit name does not match any value in the repository.\n"

/*%
** Explanation:
**      A unit name has been passed in an operation.  The API is trying to
**      get the unit id to pass to another operation and is unable to resolve
**      the name.
**
** System status:
**      API error status
**
** User action:
**      Check the unit name and resubmit the request.
*/

E_WU0018_INVALID_ROLE_NAME
"The specified role name does not match any value in the repository.\n"

/*%
** Explanation:
**      A role name has been passed in an operation.  The API is trying to
**      get the role id to pass to another operation and is unable to resolve
**      the name.
**
** System status:
**      API error status
**
** User action:
**      Check the role name and resubmit the request.
*/

E_WU0019_INVALID_PROF_NAME
"The specified profile name does not match any value in the repository.\n"

/*%
** Explanation:
**      A profile name has been passed in an operation.  The API is trying to
**      get the profile id to pass to another operation and is unable to
**      resolve the name.
**
** System status:
**      API error status
**
** User action:
**      Check the profile name and resubmit the request.
*/

E_WU001A_REGDOCS_USAGE
"Usage:\n\n    regdocs <options> <filename>\n
    [-nnode] -oowner -uunitname -llocation [-tdoctype] [-fflags] [-ilistfile]\n
        -n  node name of target server\n
        -o  ice user that owns the file\n
        -u  business unit name files belong to\n
        -l  name of the location\n
        -t  document type - default page if not specified\n
            p       file is processed by the ice server\n
            f       file is unprocessed by the ice server\n
        -f  document flags - default external if not specified\n
            e       external location overrides rpfs flags\n
            g       file is accessible by any user\n
            r       file is located in the repository - enables pfs\n
            p       file is preloaded during server start\n
            f       file is always loaded\n
            s       file is loaded on demand\n
        -i  input filename\n"

/*%
** Explanation:
**      An incorrect number of command line arguments have been passed
**      to the utility.
**
** System status:
**      Utility is terminated.
**
** User action:
**      Use the correct number of command line arguments
*/

E_WU001B_DEREGDOCS_USAGE
"Usage:\n\n    deregdocs <options> <filename>\n
    -uunitname -llocation\n
        -u  business unit name files belong to\n
        -l  name of the location\n
        -i  input filename\n"

/*%
** Explanation:
**      An incorrect number of command line arguments have been passed
**      to the utility.
**
** System status:
**      Utility is terminated.
**
** User action:
**      Use the correct number of command line arguments
*/

E_WU001C_INVALID_LOC_NAME
"The specified location name does not match any value in the repository.\n"

/*%
** Explanation:
**      A location name has been passed in an operation.  The API is trying to
**      get the location id to pass to another operation and is unable to
**      resolve the name.
**
** System status:
**      API error status
**
** User action:
**      Check the location name and resubmit the request.
*/

E_WU001D_INVALID_FILENAME
"The specified filename for the business unit and location was not found.\n"

/*%
** Explanation:
**      The specified filename does not exist for the specified business unit
**      and location
**
** System status:
**      Unitlity is terminated
**
** User action:
**      Check the spelling of unit name, location name and filename and
**      resubmit the request.
*/

E_WU001E_INVALID_STACK
"The stack reference passed to the function was not valid\n"

/*%
** Explanation:
**      The reference to the stack in not valid or does not refer to a valid
**      ice stack.
**
** System status:
**      Unitlity is terminated
**
** User action:
**      The stack reference is returned from a call to stk_alloc.  Ensure that
**      stk_alloc is called and the stack pointer returned is used in
**      subsequent calls to the ICE stack API.
*/

E_WU001F_INVALID_PARAMETER
"A byref parameter passed to a stack API function was not initialized for use.\n"

/*%
** Explanation:
**      An argument passed to an ICE stack function to receive an output
**      value was not initialized.
**
** System status:
**      Unitlity is terminated
**
** User action:
**      Validate output parameters to the API function calls.
*/

E_WU0020_SIZE_UNAVAILABLE
"Unable to fulfil the memory request for the specified stack size\n"

/*%
** Explanation:
**      The requested size of stack memory cannot be allocated.
**
** System status:
**      Unitlity is terminated
**
** User action:
**      Reduce the size requested in stk_alloc and retry.
*/

E_WU0021_INVALID_ENTRY
"A byref entry parameter to a stack API function was not initialized for use.\n"

/*%
** Explanation:
**      An argument passed to an ICE stack function to receive an output
**      value was not initialized.
**
** System status:
**      Unitlity is terminated
**
** User action:
**      Validate output parameters to the API function calls.
*/

E_WU0022_NO_ENTRY
"No entries available on the stack.\n"

/*%
** Explanation:
**      An attempt has been made to pop an entry from the stack that has no
**      more entries.
**
** System status:
**      API error status.
**
** User action:
**      None.
*/

E_WU0023_INVALID_PARAMETER
"A byref parameter passed to an API function was not initialized for use.\n"

/*%
** Explanation:
**      An argument passed to an ICE stack function to receive an output
**      value was not initialized.
**
** System status:
**      Unitlity is terminated
**
** User action:
**      Validate output parameters to the API function calls.
*/

E_WU0024_ICEVAR_USAGE
"Usage:\n\n    icevar <cookie> <action> <scope> <name> [<value>]\n"

/*%
** Explanation:
**      An incorrect number of command line arguments have been passed
**      to the utility.
**
** System status:
**      Utility is terminated.
**
** User action:
**      Use the correct number of command line arguments
*/
