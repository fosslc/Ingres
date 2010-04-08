/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apiehndl.h>
# include <apierhnd.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apigeter.c
**
** Description:
**	This file contains the IIapi_getErrorInfo() function interface.
**
**      IIapi_getErrorInfo  API get error information function
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
**	20-Dec-94 (gordy)
**	    Clean up common handle processing.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	 9-Mar-95 (gordy)
**	    Silence compiler warnings.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
*/




/*
** Name: IIapi_getErrorInfo
**
** Description:
**	This function provide an interface for the frontend application
**      to retrieve error information from the DBMS server.  Please refer
**      to the API user specification for function description.
**
**      getEInfoParm  input and output parameters for IIapi_getErrorInfo()
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
**	20-Dec-94 (gordy)
**	    Clean up common handle processing.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_getErrorInfo( IIAPI_GETEINFOPARM II_FAR *getEInfoParm )
{
    IIAPI_HNDL	*handle;
    IIAPI_ERRORHNDL	*errHndl;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_getErrorInfo: retrieving errors from API\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! getEInfoParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getErrorInfo: null getErrorInfo parameters\n" );
	return;
    }
    
    getEInfoParm->ge_status = IIAPI_ST_SUCCESS;
    handle = (IIAPI_HNDL *)getEInfoParm->ge_errorHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getErrorInfo: API is not initialized\n" );
	getEInfoParm->ge_status = IIAPI_ST_NOT_INITIALIZED;
	return;
    }
    
    if ( ! IIapi_isEnvHndl(  (IIAPI_ENVHNDL *)handle  )  &&
	 ! IIapi_isConnHndl( (IIAPI_CONNHNDL *)handle )  && 
	 ! IIapi_isTranHndl( (IIAPI_TRANHNDL *)handle )  &&
	 ! IIapi_isStmtHndl( (IIAPI_STMTHNDL *)handle )  && 
	 ! IIapi_isDbevHndl( (IIAPI_DBEVHNDL *)handle ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getErrorInfo: invalid handle\n" );
	getEInfoParm->ge_status = IIAPI_ST_INVALID_HANDLE;
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_getErrorInfo: handle = %p\n", handle );
    
    if ( ! ( errHndl = IIapi_getErrorHndl( handle ) ) )
	getEInfoParm->ge_status = IIAPI_ST_NO_DATA;
    else
    {
	getEInfoParm->ge_type = errHndl->er_type;
	MEcopy( errHndl->er_SQLSTATE, 
		sizeof(getEInfoParm->ge_SQLSTATE), getEInfoParm->ge_SQLSTATE );
	getEInfoParm->ge_errorCode = errHndl->er_errorCode;
	getEInfoParm->ge_message = errHndl->er_message;
	getEInfoParm->ge_serverInfoAvail = errHndl->er_serverInfoAvail;

	if ( ! getEInfoParm->ge_serverInfoAvail )
	    getEInfoParm->ge_serverInfo = NULL;
	else
	    getEInfoParm->ge_serverInfo = errHndl->er_serverInfo;
    }
    
    return;
}
