/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apithndl.h>
# include <apisphnd.h>
# include <apierhnd.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apisavep.c
**
** Description:
**	This file contains the IIapi_savePoint() function interface.
**
**      IIapi_savePoint  API save point function
**
** History:
**      01-oct-93 (ctham)
**          creation
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
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	    Make callback for all errors.
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
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/




/*
** Name: IIapi_savePoint
**
** Description:
**	This function provide an interface for the frontend application
**      to create a checkpoint for a transaction for future partial
**      rollback.  Please refer to the API user specification for
**      function description.
**
**      savePtParm  input and output parameters for IIapi_savePoint()
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
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      17-may-94 (ctham)
**          use II_EXPORT for functions which will cross the interface
**          between API and the application.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	26-Oct-94 (gordy)
**	    Can't create savepoint with active statements.
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
IIapi_savePoint( IIAPI_SAVEPTPARM II_FAR *savePtParm )
{
    IIAPI_TRANHNDL	*tranHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_savePoint: making savepoint in a transaction\n" );
    
    /* 
    ** Validate Input parameters.
    */
    if ( ! savePtParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_savePoint: null savePoint parameters\n" );
	return;
    }
    
    savePtParm->sp_genParm.gp_completed = FALSE;
    savePtParm->sp_genParm.gp_status = IIAPI_ST_SUCCESS;
    savePtParm->sp_genParm.gp_errorHandle = NULL;
    tranHndl = (IIAPI_TRANHNDL *)savePtParm->sp_tranHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_savePoint: API is not initialized\n" );
	IIapi_appCallback( &savePtParm->sp_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isTranHndl( tranHndl )  ||  IIAPI_STALE_HANDLE( tranHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_savePoint: invalid transaction handle\n" );
	IIapi_appCallback( &savePtParm->sp_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO )
        ( "IIapi_savePoint: tranHndl = %p\n", tranHndl );
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)tranHndl );
    
    /*
    ** Checking for condition which disallows savepoint
    */
    if ( ! IIapi_isQueEmpty( &tranHndl->th_stmtHndlList ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_savepoint: can't savepoint with active statements\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)tranHndl, 
				 E_AP0004_ACTIVE_QUERIES, 
				 II_SS25000_INV_XACT_STATE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &savePtParm->sp_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &savePtParm->sp_genParm, 
			       (IIAPI_HNDL *)tranHndl, IIAPI_ST_FAILURE );
	return;
    }
    
    if ( savePtParm->sp_savePoint == (char *)NULL  ||
	 savePtParm->sp_savePoint[0] == EOS )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )( "IIapi_savePoint: invalid savepoint\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)tranHndl, 
				 E_AP0013_INVALID_POINTER, 
				 II_SS42506_INVALID_IDENTIFIER,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &savePtParm->sp_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &savePtParm->sp_genParm, 
			       (IIAPI_HNDL *)tranHndl, IIAPI_ST_FAILURE );
	return;
    }
    
    IIapi_uiDispatch( IIAPI_EV_SAVEPOINT_FUNC, 
		      (IIAPI_HNDL *)tranHndl, (II_PTR)savePtParm );

    return;
}
