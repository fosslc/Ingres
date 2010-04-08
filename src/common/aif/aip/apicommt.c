/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apithndl.h>
# include <apierhnd.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apicommt.c
**
** Description:
**	 This file contains the IIapi_commit() function interface.
**
**      IIapi_commit  API commit function
**
** History:
**      01-oct-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          Check for condition which prevents transaction deletion
**          before invoking the dispatcher.
**      13-apr-94 (ctham)
**          Let the dispatcher do the deletion of transaction handle.
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
** Name: IIapi_commit
**
** Description:
**	This function provide an interface for the frontend application
**      to commit a transaction.  Please refer to the API user
**      specification for function description.
**
**      commitParm  input and output parameters for IIapi_commit()
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
**          Check for condition which prevents transaction deletion
**          before invoking the dispatcher.
**      13-apr-94 (ctham)
**          Let the dispatcher do the deletion of transaction handle.
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
**	 9-Jun-95 (gordy)
**	    Added status parameter to IIapi_localError().
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_commit( IIAPI_COMMITPARM II_FAR *commitParm )
{
    IIAPI_TRANHNDL    *tranHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_commit: commiting a transaction\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! commitParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR ) 
	    ( "IIapi_commit: null commit parameters\n" );
	return;
    }
    
    commitParm->cm_genParm.gp_completed = FALSE;
    commitParm->cm_genParm.gp_status = IIAPI_ST_SUCCESS;
    commitParm->cm_genParm.gp_errorHandle = NULL;
    tranHndl = (IIAPI_TRANHNDL *)commitParm->cm_tranHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_commit: API is not initialized\n" );
	IIapi_appCallback( &commitParm->cm_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isTranHndl( tranHndl )  ||  IIAPI_STALE_HANDLE( tranHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR ) 
	    ( "IIapi_commit: invalid transaction handle\n" );
	IIapi_appCallback( &commitParm->cm_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO )
        ( "IIapi_commit: tranHndl = %p\n", tranHndl );
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)tranHndl );

    /*
    ** Check for conditions which disallows deletion of transaction
    */
    if ( ! IIapi_isQueEmpty( (QUEUE *)&tranHndl->th_stmtHndlList ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_commit: can't delete with active statements\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)tranHndl, 
				 E_AP0004_ACTIVE_QUERIES, 
				 II_SS25000_INV_XACT_STATE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &commitParm->cm_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &commitParm->cm_genParm, 
			       (IIAPI_HNDL *)tranHndl, IIAPI_ST_FAILURE );
	return;
    }
    
    IIapi_uiDispatch( IIAPI_EV_COMMIT_FUNC, 
		      (IIAPI_HNDL *)tranHndl, (II_PTR)commitParm );

    return;
}
