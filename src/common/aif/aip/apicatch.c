/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apichndl.h>
# include <apiehndl.h>
# include <apierhnd.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apicatch.c
**
** Description:
**	This file contains the IIapi_catchEvent() function interface.
**
**      IIapi_catchEvent  API catch event function
**
** History:
**      01-oct-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          Let the dispatcher do the deletion of event handle.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      17-may-94 (ctham)
**          use II_EXPORT for functions which will cross the interface
**          between API and the application.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	 31-may-94 (ctham)
**	     changed ce_event{Name,Owner} to ce_selectEvent{Name,Owner}.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	25-Apr-95 (gordy)
**	    Allow event handle to be passed in.
**	19-May-95 (gordy)
**	    Fixed include statements.
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
** Name: IIapi_catchEvent
**
** Description:
**	This function provide an interface for the frontend application
**      to register for an incoming GCA event.  Please refer to the API user
**      specification for function description.
**
**      catchEventParm  input and output parameters for IIapi_catchEvent()
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      01-oct-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          Let the dispatcher do the deletion of event handle.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      17-may-94 (ctham)
**          use II_EXPORT for functions which will cross the interface
**          between API and the application.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	 31-may-94 (ctham)
**	     changed ce_event{Name,Owner} to ce_selectEvent{Name,Owner}.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	25-Apr-95 (gordy)
**	    Allow event handle to be passed in.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_catchEvent( IIAPI_CATCHEVENTPARM II_FAR *catchEventParm )
{
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_DBEVHNDL	*dbevHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_catchEvent: catching an Event\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! catchEventParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_catchEvent: null catchEvent parameters\n" );
	return;
    }
    
    catchEventParm->ce_genParm.gp_completed = FALSE;
    catchEventParm->ce_genParm.gp_status = IIAPI_ST_SUCCESS;
    catchEventParm->ce_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)catchEventParm->ce_connHandle;
    dbevHndl = (IIAPI_DBEVHNDL *)catchEventParm->ce_eventHandle;

    /*
    ** Make sure API is initialized.
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_catchEvent: API is not initialized\n" );
	IIapi_appCallback( &catchEventParm->ce_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_catchEvent: invalid connection handle\n" );
	IIapi_appCallback( &catchEventParm->ce_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    /*
    ** Validate event handle if passed in.
    */
    if ( dbevHndl  &&  
	 (! IIapi_isDbevHndl( dbevHndl )  ||  IIAPI_STALE_HANDLE( dbevHndl )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_catchEvent: invalid statement handle\n" );
	IIapi_appCallback( &catchEventParm->ce_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }

    IIAPI_TRACE( IIAPI_TR_INFO )
	( "IIapi_catchEvent: connHndl = %p, dbevHndl = %p\n", 
	  connHndl, dbevHndl );
    
    IIAPI_TRACE( IIAPI_TR_INFO )
	( "IIapi_catchEvent: dbevName = %s, dbevOwner = %s\n",
	  catchEventParm->ce_selectEventName 
		? catchEventParm->ce_selectEventName : "NULL",
	  catchEventParm->ce_selectEventOwner
		?catchEventParm->ce_selectEventOwner : "NULL");
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );
    if ( dbevHndl )  IIapi_clearAllErrors( (IIAPI_HNDL *)dbevHndl );

    /*
    ** Create database event handle if one was not passed in.
    */
    if ( ! dbevHndl  &&  ! (dbevHndl = IIapi_createDbevHndl(catchEventParm)) )
    {
	IIapi_appCallback( &catchEventParm->ce_genParm, NULL, 
			   IIAPI_ST_OUT_OF_MEMORY );
	return;
    }
    
    catchEventParm->ce_eventHandle = (II_PTR)dbevHndl;
    
    IIapi_uiDispatch( IIAPI_EV_CATCHEVENT_FUNC, 
		      (IIAPI_HNDL *)dbevHndl, (II_PTR)catchEventParm );

    return;
}
