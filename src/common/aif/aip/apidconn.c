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
** Name: apidconn.c
**
** Description:
**	This file contains the IIapi_disconnect() function interface.
**
**      IIapi_disconnect  API disconnect function
**
** History:
**      01-oct-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          Check for condition which prevents connection deletion
**          before invoking the dispatcher.
**      13-apr-94 (ctham)
**          Let the dispatcher do the deletion of connection handle.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      17-may-94 (ctham)
**          use II_EXPORT for functions which will cross the interface
**          between API and the application.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
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
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/




/*
** Name: IIapi_disconnect
**
** Description:
**	This function provide an interface for the frontend application
**      to disconnect with a DBMS server.  Please refer to the API user
**      specification for function description.
**
**      disconnParm  input and output parameters for IIapi_disconnect()
**
** Return value:
**      none.
**
** Re-enconncy:
**	This function does not update shared memory.
**
** History:
**      01-oct-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          Check for condition which prevents connection deletion
**          before invoking the dispatcher.
**      13-apr-94 (ctham)
**          Let the dispatcher do the deletion of connection handle.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      17-may-94 (ctham)
**          use II_EXPORT for functions which will cross the interface
**          between API and the application.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	 9-Jun-95 (gordy)
**	    Added status parameter to IIapi_localError().
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_disconnect( IIAPI_DISCONNPARM II_FAR *disconnParm )
{
    IIAPI_CONNHNDL	*connHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_disconnect: disconnecting a connection\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! disconnParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_disconnect: null disconnect parameters\n" );
	return;
    }

    disconnParm->dc_genParm.gp_completed = FALSE;
    disconnParm->dc_genParm.gp_status = IIAPI_ST_SUCCESS;
    disconnParm->dc_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)disconnParm->dc_connHandle;

    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_disconnect: API is not initialized\n" );
	IIapi_appCallback( &disconnParm->dc_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE(IIAPI_TR_ERROR)
	    ( "IIapi_disconnect: invalid connection handle\n" );
	IIapi_appCallback( &disconnParm->dc_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE(IIAPI_TR_INFO) ("IIapi_disconnect: connHndl = %p\n",
			    disconnParm->dc_connHandle);
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );
    
    /*
    ** Check for conditions that prevent deleting a connection
    */
    if ( ! IIapi_isQueEmpty( &connHndl->ch_tranHndlList ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_disconnect: can't disconnect with active transactions\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0003_ACTIVE_TRANSACTIONS, 
				 II_SS25000_INV_XACT_STATE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &disconnParm->dc_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &disconnParm->dc_genParm, 
			       (IIAPI_HNDL *)connHndl, 
			       IIAPI_ST_FAILURE );
	return;
    }
    
    if ( ! IIapi_isQueEmpty( &connHndl->ch_dbevHndlList )  ||
	 ! IIapi_isQueEmpty( &connHndl->ch_dbevCBList ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_disconnect: can't disconnect with active events\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0005_ACTIVE_EVENTS, 
				 II_SS25000_INV_XACT_STATE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &disconnParm->dc_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &disconnParm->dc_genParm, 
			       (IIAPI_HNDL *)connHndl, 
			       IIAPI_ST_FAILURE );
	return;
    }
    
    IIapi_uiDispatch( IIAPI_EV_DISCONN_FUNC, 
		      (IIAPI_HNDL *)connHndl, (II_PTR)disconnParm );

    return;
}
