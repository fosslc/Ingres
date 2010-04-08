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
** Name: apirollb.c
**
** Description:
**	This file contains the IIapi_rollback() function interface.
**
**      IIapi_rollback  API rollback function
**
** History:
**      01-oct-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          Check for condition which prevents transaction deletion
**          before invoking the dispatcher.  Let the dispatcher do
**          the deletion of transaction handle.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
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
**	31-May-05 (gordy)
**	    Don't permit new requests on handles marked for deletion.
*/




/*
** Name: IIapi_rollback
**
** Description:
**	This function provide an interface for the frontend application
**      to rollback a transaction.  Please refer to the API user
**      specification for function description.
**
**      rollbackParm  input and output parameters for IIapi_rollback()
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
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
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
IIapi_rollback( IIAPI_ROLLBACKPARM II_FAR *rollbackParm )
{
    IIAPI_TRANHNDL	*tranHndl;
    IIAPI_SAVEPTHNDL	*saveHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE ) 
	( "IIapi_rollback: rolling back a transaction\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! rollbackParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_rollback: null rollback parameters\n" );
	return;
    }
    
    rollbackParm->rb_genParm.gp_completed = FALSE;
    rollbackParm->rb_genParm.gp_status = IIAPI_ST_SUCCESS;
    rollbackParm->rb_genParm.gp_errorHandle = NULL;
    tranHndl = (IIAPI_TRANHNDL *)rollbackParm->rb_tranHandle;
    saveHndl = (IIAPI_SAVEPTHNDL *)rollbackParm->rb_savePointHandle;

    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ("IIapi_rollback: API is not initialized\n");
	IIapi_appCallback( &rollbackParm->rb_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isTranHndl( tranHndl )  ||  IIAPI_STALE_HANDLE( tranHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_rollback: invalid transaction handle\n" );
	IIapi_appCallback( &rollbackParm->rb_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    if ( saveHndl  && ! IIapi_isSavePtHndl( saveHndl, tranHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_rollback: invalid savepoint handle\n" );
	IIapi_appCallback( &rollbackParm->rb_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO )
	( "IIapi_rollback: tranHndl = %p, savePointHndl = %p\n",
	  tranHndl, rollbackParm->rb_savePointHandle );
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)tranHndl );
    
    /*
    ** Check for conditions which disallows deletion of transaction
    */
    if ( ! IIapi_isQueEmpty( &tranHndl->th_stmtHndlList ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_rollback: can't delete with active statements\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)tranHndl, 
				 E_AP0004_ACTIVE_QUERIES, 
				 II_SS25000_INV_XACT_STATE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &rollbackParm->rb_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &rollbackParm->rb_genParm, 
			       (IIAPI_HNDL *)tranHndl, IIAPI_ST_FAILURE );
	return;
    }
    
    IIapi_uiDispatch( IIAPI_EV_ROLLBACK_FUNC, 
		      (IIAPI_HNDL *)tranHndl, (II_PTR)rollbackParm );

    return;
}
