/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ice_c_api.h
**
** Description:
**
## History:
##      30-Oct-98 (fanra01)
##          Created.
##      24-Mar-1999 (fanra01)
##          Update comment for return from ICE_C_Initialize.
##          Also define SUCCESS to match documentation.
##      28-Feb-2000 (fanra01)
##          Bug 100632
##          Add prototype for ICE_C_Terminate.
##      04-May-2001 (fanra01)
##          Sir 103813
##          Add prototype for ICE_C_ConnectAs.
##
*/

# ifndef ICE_C_INCLUDED
# define ICE_C_INCLUDED

/*
** Platform dependencies
*/
# include <iiapidep.h>

# ifndef OK
# define    OK      0
# endif

# ifndef SUCCESS
# define    SUCCESS OK
# endif

/*
** Name: ICE_C_CLIENT
**
** Description:
**      This structure defines a reference pointer that should be used in
**      all calls to the ICE C API.
**      This reference is returned from a call to ICE_C_connect.
*/
typedef II_CHAR*    ICE_C_CLIENT;

/*
** Name: ICE_STATUS
**
** Description:
**      Defines a datatype for returning status from an ICE C API function.
*/
typedef II_UINT4    ICE_STATUS;

/*
** Name: ICE_C_PARAMS
**
** Description:
**      Structure for passing parameters to/from the ICE C API.  A null
**      terminated list is used for input and output parameters to the
**      ICE_C_Execute function.
**
**      type    Determines whether this parameter is input or output.
**              An integer expression formed from one or more of the
**              manifest constants.
**              ICE_IN | ICE_BLOB
**              ICE_OUT| ICE_BLOB
**      name    Name of the parameter.  A null terminated character string.
**      value   Value of the parameter. A null terminated character string.
*/
typedef struct __ICE_C_PARAMS
{
    II_INT      type;
#define ICE_IN      1
#define ICE_OUT     2
#define ICE_BLOB    4
    II_CHAR*    name;
    II_CHAR*    value;
} ICE_C_PARAMS;

/*
** Name: ICE_C_Initialize
**
** Description:
**      Prepeares the interface for initial use.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      0    Function completed successfully
**      !0   Function failed
*/
ICE_STATUS
ICE_C_Initialize ();

/*
** Name: ICE_C_Connect
**
** Description:
**      Establish a connection with the specified node using the provided user
**      and password.
**
** Inputs:
**      node        name of the node to connect to.  If this value is NULL
**                  the local node is assumed.
**      user        specify the database user the connect should be made with.
**      password    password of the database user to connect as.
**
** Outputs:
**      client      client reference pointer.  This reference is owned by the
**                  ICE C API interfaced and should not be modified.
**                  This pointer should be used in all subsequent calls to the
**                  ICE C API functions.
**
** Returns:
**      0           successfully completed.
**      !0          function failed.  Value indicates reason for failure.
*/
ICE_STATUS
ICE_C_Connect( II_CHAR* node, II_CHAR* user, II_CHAR* password,
    ICE_C_CLIENT *client);

/*
** Name: ICE_C_ConnectAs
**
** Description:
**      Establish a connection with the specified node using the provided user,
**      password and cookie.
**
** Inputs:
**      node        name of the node to connect to.  If this value is NULL
**                  the local node is assumed.
**      cookie      session identifier.
**
** Outputs:
**      client      client reference pointer.  This reference is owned by the
**                  ICE C API interfaced and should not be modified.
**                  This pointer should be used in all subsequent calls to the
**                  ICE C API functions.
**
** Returns:
**      0           successfully completed.
**      !0          function failed.  Value indicates reason for failure.
*/
ICE_STATUS
ICE_C_ConnectAs( II_CHAR* node, II_CHAR* cookie, ICE_C_CLIENT *client);

/*
** Name: ICE_C_Execute
**
** Description:
**      Forms a requested action query and sends it to the connected node.
**
** Inputs:
**      client      reference returned by a call to ICE_C_Connect.
**      name        name of the function to execute.
**
** Outputs:
**      tab         the table is updated with result data accessed via the
**                  ICE_C_Fetch and ICE_C_GetAttribute functions.
**
** Returns:
**      0           successfully completed.
**      !0          function failed.  Value indicates reason for failure.
**
*/
ICE_STATUS
ICE_C_Execute( ICE_C_CLIENT client, II_CHAR* name, ICE_C_PARAMS tab[]);

/*
** Name: ICE_C_Fetch
**
** Description:
**      Updates the retrieved row position within the result set.
**
** Inputs:
**      client      reference returned by a call to ICE_C_Connect.
**
** Outputs:
**      end         TRUE indicates that the last row has been referenced.
**
** Returns:
**      0           successfully completed.
**      !0          function failed.  Value indicates reason for failure.
**
*/
ICE_STATUS
ICE_C_Fetch( ICE_C_CLIENT client, II_INT4* end);

/*
** Name: ICE_C_GetAttribute
**
** Description:
**      Returns the value of the specified column within the current row.
**
** Inputs:
**      client      reference returned by a call to ICE_C_Connect.
**      position    number of the ICE_OUT parameter as specified in the
**                  ICE_C_PARAMS array.
**
** Outputs:
**      None.
**
** Returns:
**      Pointer     to the text converted value.
**      NULL        on error.
*/
char*
ICE_C_GetAttribute( ICE_C_CLIENT client, II_INT4 position);

/*
** Name: ICE_C_Close
**
** Description:
**      Frees the resources created for the tuple data during fetch.
**
** Inputs:
**      client      reference returned by a call to ICE_C_Connect.
**
** Outputs:
**      None.
**
** Returns:
**      0           successfully completed.
**      !0          function failed.  Value indicates reason for failure.
**
*/
ICE_STATUS
ICE_C_Close( ICE_C_CLIENT client);

/*
** Name: ICE_C_Disconnect
**
** Description:
**      Issues a disconnect from the connected node.
**
** Inputs:
**      client      reference returned by a call to ICE_C_Connect.
**
** Outputs:
**      None.
**
** Returns:
**      0           successfully completed.
**      !0          function failed.  Value indicates reason for failure.
**
*/
ICE_STATUS
ICE_C_Disconnect( ICE_C_CLIENT *client);

/*
** Name: ICE_C_LastError
**
** Description:
**      Returns a textual error message for the last error.
**
** Inputs:
**      client      reference returned by a call to ICE_C_Connect.
**
** Outputs:
**      None.
**
** Returns:
**      error
**      NULL        if the no message available.
*/
char*
ICE_C_LastError( ICE_C_CLIENT client );

/*
** Name: ICE_C_Terminate
**
** Description:
**      Physically disconnects all active sessions.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      There is no error returned from this function.
*/
void
ICE_C_Terminate( void );

#endif
