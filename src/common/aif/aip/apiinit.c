/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apichndl.h>
# include <apimisc.h>
# include <apilower.h>
# include <apitrace.h>

/*
** Name: apiinit.c
**
** Description:
**	This file contains the IIapi_initialize() function interface.
**
**      IIapi_initialize	API initialization function
**      IIapi_terminate		API termination function
**	IIapi_releaseEnv	Release environment resources.
**
** History:
**      16-may-94 (ctham)
**          creation
**	18-Nov-94 (gordy)
**	    Added version information.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 8-Mar-95 (gordy)
**	    Removed global variable.
**	 9-Mar-95 (gordy)
**	    Silence compiler warnings.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	28-Jul-95 (gordy)
**	    Use fixed length types.
**	17-Jan-96 (gordy)
**	    Added environment handles.  
**	 2-Oct-96 (gordy)
**	    Removed unused MINICODE ifdef's.
**	 7-Feb-97 (gordy)
**	    API Version 2: environment handles returned to application.
**	    IIapi_releaseEnv() added to free environment resources.
**	16-Jan-2007 (lunbr01)  Bug 117496
**	    OpenAPI apps fail with status IIAPI_ST_OUT_OF_MEMORY (8)
**	    when II_SYSTEM not set; this status is misleading
**	    (II_ST_FAILURE is better).
*/




/*
** Name: IIapi_initialize
**
** Description:
**	This function provide an interface for the frontend application
**      to initialize access to the INGRES API.
**
**      initParm	input and output parameters for IIapi_initialize()
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      16-may-94 (ctham)
**          creation
**	18-Nov-94 (gordy)
**	    Added version information.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 8-Mar-95 (gordy)
**	    Removed global variable.
**	17-Jan-96 (gordy)
**	    Added environment handles.  Initialization tracking 
**	    moved to IIapi_initAPI().
**	 7-Feb-97 (gordy)
**	    API Version 2: environment handles returned to application.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_initialize( IIAPI_INITPARM II_FAR *initParm )
{
    IIAPI_ENVHNDL	*envHndl;

    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_initialize: startup API\n" );
	
    /*
    ** Validate Input parameters
    */
    if ( ! initParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_initialize: null init parameters\n" );
	return;
    }
    
    initParm->in_status = IIAPI_ST_SUCCESS;

    if ( initParm->in_version < IIAPI_VERSION_1  ||
	 initParm->in_version > IIAPI_VERSION )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )( "IIapi_initialize: invalid version\n" );
	initParm->in_status = IIAPI_ST_FAILURE;
	return;
    }

    IIAPI_TRACE( IIAPI_TR_INFO )
	( "IIapi_initialize: version = %d\n", initParm->in_version );

    envHndl = IIapi_initAPI( initParm->in_version, initParm->in_timeout );

    if ( ! envHndl )
    {
	initParm->in_status = IIAPI_ST_FAILURE;
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO )
        ("IIapi_initialize: INGRES API initialized, envHndl = %p\n", envHndl);
    
    /*
    ** Return environment handle to caller.
    */
    if ( initParm->in_version >= IIAPI_VERSION_2 )
	initParm->in_envHandle = envHndl;

    return;
}




/*
** Name: IIapi_terminate
**
** Description:
**	This function provide an interface for the frontend application
**      to terminate access to the INGRES API.
**
**      termParm	input and output parameters for IIapi_terminate()
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      16-may-94 (ctham)
**          creation
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	16-Feb-95 (gordy)
**	    Added MUr_semaphore() call.
**	17-Jan-96 (gordy)
**	    Added environment handles.  
**	 7-Feb-97 (gordy)
**	    IIapi_releaseEnv() is used to free environment 
**	    handles since there is no backward compatible 
**	    way to add an environment handle to IIAPI_TERMPARM.  
**	    Remove references to environment handles.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_terminate( IIAPI_TERMPARM II_FAR *termParm )
{
    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_terminate: shutdown API\n" );
	
    /*
    ** Validate Input parameters
    */
    if ( ! termParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_terminate: null term parameters\n" );
	return;
    }
    
    termParm->tm_status = IIAPI_ST_SUCCESS;

    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_terminate: API is not initialized\n" );
	termParm->tm_status = IIAPI_ST_NOT_INITIALIZED;
	return;
    }

    /*
    ** Terminate the default environment handle
    ** and possibly API.  Return a warning if 
    ** this is not the last required termination 
    ** request.
    */
    if ( ! IIapi_termAPI( NULL ) )
	termParm->tm_status = IIAPI_ST_WARNING;

    return;
}




/*
** Name: IIapi_releaseEnv
**
** Description:
**	This function provide an interface for the frontend application
**      to and environment handle prior to terminating the INGRES API.
**
** Input:
**      relEnvParm	Input and output parameters for IIapi_releaseEnv()
**
** Output:
**	None.
**
** Return value:
**	II_VOID
**
** History:
**	 7-Feb-97 (gordy)
**	    Created.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_releaseEnv( IIAPI_RELENVPARM II_FAR *relEnvParm )
{
    IIAPI_ENVHNDL	*envHndl;
    QUEUE		*q;

    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_releaseEnv: Release Environment\n" );
	
    /*
    ** Validate Input parameters
    */
    if ( ! relEnvParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_releaseEnv: null parameters\n" );
	return;
    }
    
    relEnvParm->re_status = IIAPI_ST_SUCCESS;
    envHndl = (IIAPI_ENVHNDL *)relEnvParm->re_envHandle;

    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_releaseEnv: API is not initialized\n" );
	relEnvParm->re_status = IIAPI_ST_NOT_INITIALIZED;
	return;
    }

    if ( ! IIapi_isEnvHndl( envHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_releaseEnv: invalid environment handle\n" );
	relEnvParm->re_status = IIAPI_ST_INVALID_HANDLE;
	return;
    }

    IIAPI_TRACE( IIAPI_TR_INFO )
	( "IIapi_releaseEnv: envHndl = %p\n", envHndl );

    IIapi_clearAllErrors( (IIAPI_HNDL *)envHndl );

    /*
    ** Abort active connections.  The connection
    ** handles will be deleted when the environment
    ** handle is deleted.
    */
    for( 
	 q = envHndl->en_connHndlList.q_next;
	 q != &envHndl->en_connHndlList;
	 q = q->q_next
       )
    {
	IIAPI_HNDL *hndl = (IIAPI_HNDL *)q;

	/*
	** We set the connection handle state
	** to IDLE so that any GCA errors which
	** result from the abort will be ignored.
	*/
	relEnvParm->re_status = IIAPI_ST_WARNING;
	hndl->hd_smi.smi_state = IIAPI_IDLE;
	IIapi_abortGCA( (IIAPI_CONNHNDL *)q );
    }

    IIapi_termAPI( envHndl );

    return;
}

