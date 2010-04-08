/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apichndl.h>
# include <apierhnd.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apisetcn.c
**
** Description:
**	This file contains the IIapi_setConnectParam() function interface.
**
**      IIapi_setConnectParam  API set connection parameter function
**
** History:
**	14-Nov-94 (gordy)
**	    Created.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	10-May-95 (gordy)
**	    Clear error from connection handle.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	 9-Jun-95 (gordy)
**	    Added status parameter to IIapi_localError().
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	13-Mar-97 (gordy)
**	    Input handle may be connection or environment handle.
**	22-Oct-04 (gordy)
**	    If connection handle is passed in, make sure to use the
**	    associated environment handle rather than the default.
**	    If neither connection handle nor environment handle is
**	    passed in, use the default environment handle.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/




/*
** Name: IIapi_setConnectParam
**
** Description:
**	This function provide an interface for the frontend application
**	to provide parameters to be passed to a DBMS server via
**	IIapi_connect() or IIapi_modifyConnect().  Please refer to the 
**	API user specification for function description.
**
**	setConPrmParm	input and output parameters for IIapi_setConnectParam()
**
** Return value:
**	none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	14-Nov-94 (gordy)
**	    Created.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	10-May-95 (gordy)
**	    Clear error from connection handle.
**	 9-Jun-95 (gordy)
**	    Added status parameter to IIapi_localError().
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	13-Mar-97 (gordy)
**	    Input handle may be connection or environment handle.
**	22-Oct-04 (gordy)
**	    If connection handle is passed in, make sure to use the
**	    associated environment handle rather than the default.
**	    If neither connection handle nor environment handle is
**	    passed in, use the default environment handle.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_setConnectParam( IIAPI_SETCONPRMPARM II_FAR *setConPrmParm )
{
    IIAPI_ENVHNDL	*envHndl = NULL;
    IIAPI_CONNHNDL	*connHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE ) 
	( "IIapi_setConnectParam: set connection parameter\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! setConPrmParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_cancel: null function parameters\n" );
	return;
    }
    
    setConPrmParm->sc_genParm.gp_completed = FALSE;
    setConPrmParm->sc_genParm.gp_status = IIAPI_ST_SUCCESS;
    setConPrmParm->sc_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)setConPrmParm->sc_connHandle;

    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_setConnectParam: API is not initialized\n" );
	IIapi_appCallback( &setConPrmParm->sc_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( connHndl )
	if ( IIapi_isEnvHndl( (IIAPI_ENVHNDL *)connHndl ) )
	{
	    envHndl = (IIAPI_ENVHNDL *)connHndl;
	    connHndl = NULL;
	}
	else  if ( ! IIapi_isConnHndl( connHndl )  ||
		   IIAPI_STALE_HANDLE( connHndl ) )
	{
	    IIAPI_TRACE(IIAPI_TR_ERROR)
		( "IIapi_setConnectParam: invalid connection handle\n" );
	    IIapi_appCallback( &setConPrmParm->sc_genParm, NULL,
			       IIAPI_ST_INVALID_HANDLE );
	    return;
	}
    
    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_setConnectParam: envHndl = %p, connHndl = %p\n", 
	  envHndl, connHndl );

    if ( envHndl )  
	IIapi_clearAllErrors( (IIAPI_HNDL *)envHndl );
    else  if ( connHndl )
	envHndl = IIapi_getEnvHndl( (IIAPI_HNDL *)connHndl );
    else
	envHndl = IIapi_defaultEnvHndl();

    if ( connHndl ) IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );

    /*
    ** Create new connection handle if needed.
    */
    if ( ! connHndl  &&  ! ( connHndl = IIapi_createConnHndl( envHndl ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_setConnectParam: createConnHndl failed\n" );
	IIapi_appCallback( &setConPrmParm->sc_genParm, NULL,
			   IIAPI_ST_OUT_OF_MEMORY );
	return;
    }
    
    setConPrmParm->sc_connHandle = (II_PTR)connHndl;
    
    if ( setConPrmParm->sc_paramID < IIAPI_CP_BASE  ||
	 setConPrmParm->sc_paramID > IIAPI_CP_TOP )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_setConnectParam: invalid param ID %d\n", 
	      setConPrmParm->sc_paramID );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0011_INVALID_PARAM_VALUE, 
				 II_SS22003_NUM_VAL_OUT_OF_RANGE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &setConPrmParm->sc_genParm, NULL,
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &setConPrmParm->sc_genParm,
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	return;
    }
    
    IIapi_uiDispatch( IIAPI_EV_SETCONNPARM_FUNC, 
		      (IIAPI_HNDL *)connHndl, (II_PTR)setConPrmParm );

    return;
}
