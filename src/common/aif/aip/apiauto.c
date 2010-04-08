/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apierhnd.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apiauto.c
**
** Description:
**	This file contains the IIapi_autocommit() function interface.
**
**      IIapi_autocommit	API autocommit function
**
** History:
**	 9-Jul-96 (gordy)
**	    Created.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/




/*
** Name: IIapi_autocommit
**
** Description:
**	This function provide an interface for the frontend application
**      to manage the autocommit state in the DBMS server.  Please refer 
**	to the API user specification for function description.
**
**      autoParm	Input and output parameters for IIapi_autocommit()
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	 9-Jul-96 (gordy)
**	    Created.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_autocommit( IIAPI_AUTOPARM II_FAR *autoParm )
{
    IIAPI_TRANHNDL	*tranHndl;
    IIAPI_CONNHNDL	*connHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )("IIapi_autocommit: set autocommit state\n");
    
    /*
    ** Validate Input parameters
    */
    if ( ! autoParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_autocommit: null autocommit parameters\n" );
	return;
    }
    
    autoParm->ac_genParm.gp_completed = FALSE;
    autoParm->ac_genParm.gp_status = IIAPI_ST_SUCCESS;
    autoParm->ac_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)autoParm->ac_connHandle;
    tranHndl = (IIAPI_TRANHNDL *)autoParm->ac_tranHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_autocommit: API is not initialized\n" );
	IIapi_appCallback( &autoParm->ac_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! connHndl  &&  ! tranHndl )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_autocommit: no handle provided\n" );
	IIapi_appCallback( &autoParm->ac_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }

    if ( connHndl  &&  
	 (! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_autocommit: invalid connection handle\n" );
	IIapi_appCallback( &autoParm->ac_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }

    if ( tranHndl  &&  
	 (! IIapi_isTranHndl( tranHndl )  ||  IIAPI_STALE_HANDLE( tranHndl )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_autocommit: invalid transaction handle\n" );
	IIapi_appCallback( &autoParm->ac_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }

    IIAPI_TRACE( IIAPI_TR_INFO )
	( "IIapi_autocommit: connHndl = %p, tranHndl = %p\n",
	  autoParm->ac_connHandle, autoParm->ac_tranHandle );
    
    if ( connHndl )  IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );
    if ( tranHndl )  IIapi_clearAllErrors( (IIAPI_HNDL *)tranHndl );

    if ( ! tranHndl  &&  ! IIapi_isQueEmpty( &connHndl->ch_tranHndlList ) )
    {
	/*
	** Application is attempting to enable autocommit mode 
	** with an active transaction.
	*/
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_autocommit: can't enable autocommit in a transaction\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0003_ACTIVE_TRANSACTIONS, 
				 II_SS25000_INV_XACT_STATE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &autoParm->ac_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &autoParm->ac_genParm, 
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	return;
    }
	
    if ( tranHndl  &&  ! IIapi_isQueEmpty( &tranHndl->th_stmtHndlList ) )
    {
	/*
	** Application is attempting to disable autocommit mode
	** with active statements.
	*/
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ("IIapi_autocommit: can't disable autocommit with active statements\n");

	if ( ! IIapi_localError( (IIAPI_HNDL *)tranHndl, 
				 E_AP0004_ACTIVE_QUERIES, 
				 II_SS25000_INV_XACT_STATE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &autoParm->ac_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &autoParm->ac_genParm, 
			       (IIAPI_HNDL *)tranHndl, IIAPI_ST_FAILURE );
	return;
    }
    
    if ( ! tranHndl )
    {
	/*
	** Enable autocommit mode.  Create a transaction handle
	** and return it to the application.
	*/
	if ( ! ( tranHndl = IIapi_createTranHndl( NULL, connHndl ) ) )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_autocommit: createTranHndl failed\n" );
	    IIapi_appCallback( &autoParm->ac_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	    return;
	}

	autoParm->ac_tranHandle = (II_PTR)tranHndl;
    }
    else
    {
	/*
	** Disable autocommit mode.  The transaction handle
	** will be deleted, so clear the output parameter.
	*/
	autoParm->ac_tranHandle = NULL;
    }
    
    IIapi_uiDispatch( IIAPI_EV_AUTO_FUNC, 
		      (IIAPI_HNDL *)tranHndl, (II_PTR)autoParm );

    return;
}
