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
# include <apimisc.h>
# include <apidspth.h>
# include <apitrace.h>

/*
** Name: apimodcn.c
**
** Description:
**	This file contains the IIapi_modifyConnect() function
**	interface.
**
**	IIapi_modifyConnect	API modify connection function
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
**	25-Apr-95 (gordy)
**	    Fixed a typo.
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
** Name: IIapi_modifyConnect
**
** Description:
**	This function provide an interface for the frontend application
**	to modify the connection parameters after a connection has been
**	established.  Please refer to the API user specification for 
**	function description.
**
**	modifyConnParm	input and output parameters for IIapi_modifyConnect()
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
**	25-Apr-95 (gordy)
**	    Fixed a typo.
**	 9-Jun-95 (gordy)
**	    Added status parameter to IIapi_localError().
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_modifyConnect( IIAPI_MODCONNPARM II_FAR *modifyConnParm )
{
    IIAPI_CONNHNDL	*connHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE ) 
	( "IIapi_modifyConnect: modify connection to DBMS Server\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! modifyConnParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_modifyConnect: null function parameters\n" );
	return;
    }

    modifyConnParm->mc_genParm.gp_completed = FALSE;
    modifyConnParm->mc_genParm.gp_status = IIAPI_ST_SUCCESS;
    modifyConnParm->mc_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)modifyConnParm->mc_connHandle;

    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_modifyConnect: API is not initialized\n" );
	IIapi_appCallback( &modifyConnParm->mc_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE(IIAPI_TR_ERROR)
	    ( "IIapi_modifyConnect: invalid connection handle\n" );
	IIapi_appCallback( &modifyConnParm->mc_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }

    IIAPI_TRACE( IIAPI_TR_INFO )
	( "IIapi_modifyConnect: connHndl = %p\n", connHndl );
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );
    
    if ( ! connHndl->ch_sessionParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_modifyConnect: no connection parameters set\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP000F_NO_CONNECT_PARMS, 
				 II_SS21000_CARD_VIOLATION,
				 IIAPI_ST_WARNING ) )
	    IIapi_appCallback( &modifyConnParm->mc_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &modifyConnParm->mc_genParm, 
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_WARNING );
	return;
    }

    /*
    ** This function may not be called with active transactions.
    */
    if ( ! IIapi_isQueEmpty( (QUEUE *)&connHndl->ch_tranHndlList ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_modifyConnect: active transactions not allowed\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0003_ACTIVE_TRANSACTIONS, 
				 II_SS25000_INV_XACT_STATE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &modifyConnParm->mc_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &modifyConnParm->mc_genParm, 
			       (IIAPI_HNDL *)connHndl, 
			       IIAPI_ST_FAILURE );
	return;
    }
    
    IIapi_uiDispatch( IIAPI_EV_MODCONN_FUNC, 
		      (IIAPI_HNDL *)connHndl, (II_PTR)modifyConnParm );

    return;
}
