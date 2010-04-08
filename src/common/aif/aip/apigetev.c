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
** Name: apigetev.c
**
** Description:
**	This file contains the IIapi_getEvent() function interface.
**
**      IIapi_getEvent		API getEvent function
**
** History:
**	12-Jul-96 (gordy)
**	    Created.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/




/*
** Name: IIapi_getEvent
**
** Description:
**	This function provide an interface for the frontend application
**      to wait for database events to arrive.  Please refer to the API 
**	user specification for function description.
**
**      getEventParm	Input and output parameters for IIapi_getEvent()
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	12-Jul-96 (gordy)
**	    Created.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_getEvent( IIAPI_GETEVENTPARM II_FAR *getEventParm )
{
    IIAPI_CONNHNDL	*connHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )("IIapi_getEvent: wait for database event\n");
    
    /*
    ** Validate Input parameters
    */
    if ( ! getEventParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getEvent: null getEvent parameters\n" );
	return;
    }
    
    getEventParm->gv_genParm.gp_completed = FALSE;
    getEventParm->gv_genParm.gp_status = IIAPI_ST_SUCCESS;
    getEventParm->gv_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)getEventParm->gv_connHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getEvent: API is not initialized\n" );
	IIapi_appCallback( &getEventParm->gv_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getEvent: invalid connection handle\n" );
	IIapi_appCallback( &getEventParm->gv_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }

    IIAPI_TRACE( IIAPI_TR_INFO )
	( "IIapi_getEvent: connHndl = %p, timeout = %d\n", 
	  getEventParm->gv_connHandle, getEventParm->gv_timeout );
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );

    IIapi_uiDispatch( IIAPI_EV_GETEVENT_FUNC, 
		      (IIAPI_HNDL *)connHndl, (II_PTR)getEventParm );

    return;
}
