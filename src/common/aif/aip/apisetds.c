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
# include <apivalid.h>
# include <apidspth.h>
# include <apins.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apisetds.c
**
** Description:
**	This file contains the IIapi_setDescriptor() function interface.
**
**      IIapi_setDescriptor  API set descriptor function
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
**	 9-Mar-95 9gordy)
**	    Silence compiler warnings.
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
**	13-Mar-97 (gordy)
**	    Added separate validation for Name Server connections.
**	22-Oct-04 (gordy)
**	    Return specific error code from descriptor validation.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/



/*
** Name: IIapi_setDescriptor
**
** Description:
**	This function provide an interface for the frontend application
**      to set descriptors of subsequent tuple data to the DBMS server.
**      Please refer to the API user specification for function description.
**
**      setDescrParm  input and output parameters for IIapi_setDescriptor()
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
**	 9-Jun-95 (gordy)
**	    Added status parameter to IIapi_localError().
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	13-Mar-97 (gordy)
**	    Added separate validation for Name Server connections.
**	22-Oct-04 (gordy)
**	    Return specific error code from descriptor validation.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_setDescriptor( IIAPI_SETDESCRPARM II_FAR *setDescrParm )
{
    IIAPI_STMTHNDL	*stmtHndl;
    IIAPI_CONNHNDL	*connHndl;
    II_ULONG		err_code;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_setDescriptor: describing parameters to DBMS server\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! setDescrParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_setDescriptor: null setDescriptor parameters\n" );
	return;
    }
    
    setDescrParm->sd_genParm.gp_completed = FALSE;
    setDescrParm->sd_genParm.gp_status = IIAPI_ST_SUCCESS;
    setDescrParm->sd_genParm.gp_errorHandle = NULL;
    stmtHndl = (IIAPI_STMTHNDL *)setDescrParm->sd_stmtHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_setDescriptor: API is not initialized\n" );
	IIapi_appCallback( &setDescrParm->sd_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isStmtHndl( stmtHndl )  ||  IIAPI_STALE_HANDLE( stmtHndl )  ||
	 ! (connHndl = IIapi_getConnHndl( (IIAPI_HNDL *)stmtHndl )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_setDescriptor: invalid statement handle\n" );
	IIapi_appCallback( &setDescrParm->sd_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_setDescriptor: stmtHndl = %p\n", stmtHndl );
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)stmtHndl );
    
    /*
    ** Validate required and optional parameters.
    */
    if ( connHndl->ch_type == IIAPI_SMT_NS )
	err_code = IIapi_validNSDescriptor( stmtHndl,
					    setDescrParm->sd_descriptorCount, 
					    setDescrParm->sd_descriptor );
    else
	err_code = IIapi_validDescriptor( stmtHndl, 
					  setDescrParm->sd_descriptorCount, 
					  setDescrParm->sd_descriptor );
	
    if ( err_code != OK )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_setDescriptor: invalid descriptor information\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)stmtHndl, err_code, 
				 II_SS22500_INVALID_DATA_TYPE, 
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &setDescrParm->sd_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &setDescrParm->sd_genParm, 
			       (IIAPI_HNDL *)stmtHndl, IIAPI_ST_FAILURE );
	return;
    }

    IIapi_uiDispatch( IIAPI_EV_SETDESCR_FUNC, 
		      (IIAPI_HNDL *)stmtHndl, (II_PTR)setDescrParm );

    return;
}
