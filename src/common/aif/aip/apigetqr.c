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
** Name: apigetqr.c
**
** Description:
**	This file contains the IIapi_getQueryInfo() function interface.
**
**      IIapi_getQueryInfo  API get query information function
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
**	 5-Jul-99 (gordy)
**	    Removed call to IIapi_clearAllErrors().  Since this function
**	    returns query status, any previous errors not processed by
**	    the application should be preserved and returned as part of
**	    the query status info.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/




/*
** Name: IIapi_getQueryInfo
**
** Description:
**	This function provide an interface for the frontend application
**      to retrieve response data for a specific query.  Please refere to
**      the API user specification for function description.
**
**      getQInfoParm  input and output parameters for IIapi_getQueryInfo()
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
**	 5-Jul-99 (gordy)
**	    Removed call to IIapi_clearAllErrors().  Since this function
**	    returns query status, any previous errors not processed by
**	    the application should be preserved and returned as part of
**	    the query status info.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_getQueryInfo( IIAPI_GETQINFOPARM II_FAR *getQInfoParm )
{
    IIAPI_STMTHNDL	*stmtHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_getQueryInfo: retrieving response data from server\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! getQInfoParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getQueryInfo: null getQueryInfo parameters\n" );
	return;
    }
    
    getQInfoParm->gq_genParm.gp_completed = FALSE;
    getQInfoParm->gq_genParm.gp_status = IIAPI_ST_SUCCESS;
    getQInfoParm->gq_genParm.gp_errorHandle = NULL;
    stmtHndl = (IIAPI_STMTHNDL *)getQInfoParm->gq_stmtHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getQueryInfo: API is not initialized\n" );
	IIapi_appCallback( &getQInfoParm->gq_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isStmtHndl( stmtHndl )  ||  IIAPI_STALE_HANDLE( stmtHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getQueryInfo: invalid statement handle\n" );
	IIapi_appCallback( &getQInfoParm->gq_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE(IIAPI_TR_INFO)
	("IIapi_getQueryInfo: stmtHndl = %p\n", stmtHndl);
    
    IIapi_uiDispatch( IIAPI_EV_GETQINFO_FUNC, 
		      (IIAPI_HNDL *)stmtHndl, (II_PTR)getQInfoParm );

    return;
}
