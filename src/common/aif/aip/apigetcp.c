/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apishndl.h>
# include <apierhnd.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apigetcp.c
**
** Description:
**	This file contains the IIapi_getCopyMap() function interface.
**
**      IIapi_getCopyMap  API get copy map function
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
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
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
** Name: IIapi_getCopyMap
**
** Description:
**	This function provide an interface for the frontend application
**      to get copy map from the DBMS server for a pending 'copy'
**      statement.  Please refer to the API user
**      specification for function description.
**
**      getCopyMapParm  input and output parameters for IIapi_getCopyMap()
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
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_getCopyMap( IIAPI_GETCOPYMAPPARM II_FAR *getCopyMapParm )
{
    IIAPI_STMTHNDL	*stmtHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_getCopyMap: retrieving copy information from server\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! getCopyMapParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getCopyMap: null getCopyMap parameters\n" );
	return;
    }
    
    getCopyMapParm->gm_genParm.gp_completed = FALSE;
    getCopyMapParm->gm_genParm.gp_status = IIAPI_ST_SUCCESS;
    getCopyMapParm->gm_genParm.gp_errorHandle = NULL;
    stmtHndl = (IIAPI_STMTHNDL *)getCopyMapParm->gm_stmtHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getCopyMap: API is not initialized\n" );
	IIapi_appCallback( &getCopyMapParm->gm_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isStmtHndl( stmtHndl )  ||  IIAPI_STALE_HANDLE( stmtHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getCopyMap: invalid statement handle\n" );
	IIapi_appCallback( &getCopyMapParm->gm_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_getCopyMap: stmtHndl = %p\n", stmtHndl );
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)stmtHndl );
    
    IIapi_uiDispatch( IIAPI_EV_GETCOPYMAP_FUNC, 
		      (IIAPI_HNDL *)stmtHndl, (II_PTR)getCopyMapParm );

    return;
}
