/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:    iicmapi.h	- The connection manager interface
**
** Description:
**  The Connection Manager creates an drops connections to a data base for a user
**  By reusing dropped connections for the same data base & userid, the connections
**  are made much faster.
**
** History:
**      25-Feb-98 (shero03)
**          Created.
**	01-Jul-98 (shero03)
**	    Add API and LIBQ Disconnect
**	04-Mar-1999 (shero03)
**	    Pass in an address to an API error info block
**      22-jun-1999 (musro02)
**          Change typedef of bCommit from boolean to bool
**          Use typedef of II_PTR instead of PTR
**	29-Jun-1999 (shero03)
**	    Change II_PTR to PTR.
*/
#if !defined IICMPI_H

/*
**  Name: iicm_GetAPIConnection      - Connects a client to a specified   
**                                    database.
**
**  Description:
**      Connects a client to the specified database via API.
**
**  Inputs:
**      dbName          - Database name to connect to
**	flags		- Database connect options
**      user            - Registered client name
**      *err		- Pointer to error info block 
**
**  Outputs:
**     	phConn          - Pointer to connection handle
**
**  Returns:
**      OK if successful.
**
**  History:
**      25-Feb-98 (shero03)
**          Created.
*/
II_EXTERN STATUS II_EXPORT
IIcm_GetAPIConnection (char *dbName, char *flags, char *user, PTR *phConn, 
                  	PTR err);

/*
**  Name: iicm_DropAPIConnection      - Drops a client connection to a specified   
**                                    database.
**
**  Description:
**      Drops a client connection to a specified database via API.
**
**  Inputs:
**	phConn		- -> to the Connection handle.
**	bCommit		- True -> commit the connection
**			  False -> rollback the connection
**      *err		- Pointer to error info block 
**
**  Outputs:
**      none
**
**  Returns:
**      OK if successful.
**
**  History:
**      25-Feb-98 (shero03)
**          Created.
*/
II_EXTERN STATUS II_EXPORT
IIcm_DropAPIConnection (PTR phConn, bool bCommit, PTR err);

/*
**  Name: iicm_Cleanup      - Remove any connections for this process   
**
**  Description:
**      Drop any connections that this process may have established.
**
**  Inputs:
**	SID	Session ID
**
**  Outputs:
**      *err		- error code 
**
**  Returns:
**      OK if successful.
**
**  History:
**      03-Mar-98 (shero03)
**          Created.
*/
II_EXTERN STATUS II_EXPORT
IIcm_Cleanup (CS_SID sid, PTR err);

/*
**  Name: iicm_Initialize      - Starts the Connection Manager   
**
**  Description:
**      Initialize the Connection Manager so it can accept Connect & Drop requests
**	During initialization the Connection Manager allocates memory which will
**	be freed when CM_Termainte is called.  Otherwise a memory leak will occur.
**
**  Inputs:
**
**  Outputs:
**      *err		- error code 
**
**  Returns:
**      OK if successful.
**
**  History:
**      25-Feb-98 (shero03)
**          Created.
*/
II_EXTERN STATUS II_EXPORT
IIcm_Initialize (PTR err);

/*
**  Name: iicm_Terminate      - Stops the Connection Manager   
**
**  Description:
**      Terminate the Connection Manager so it can free all allocated 
**      storage.  Note that Connect and Drop requests are not valid after
**	this call.
**
**	N.B.  Currently this also terminates IIAPI.
**
**  Inputs:
**
**  Outputs:
**      *err		- error code 
**
**  Returns:
**      OK if successful.
**
**  History:
**      25-Feb-98 (shero03)
**          Created.
*/
II_EXTERN STATUS II_EXPORT
IIcm_Terminate (PTR err);

/*
**  Name: iicm_APIDisconnection - Disconnect a client's connection
**
**  Description:
**	This is an internal routine only.
**      Unconnects a client from a server via API.
**
**  Inputs:
**	phConn		- -> to the Connection handle.
**	bCommit		- True -> commit the connection
**			  False -> rollback the connection
**      *err		- Pointer to error info block 
**
**  Outputs:
**      none
**
**  Returns:
**      OK if successful.
**
**  History:
**      01-Jul-98 (shero03)
**          Created.
*/
II_EXTERN void II_EXPORT
IIcm_APIDisconnect (PTR phConn, bool bCommit, PTR err);

#endif 
