/*
** Copyright (c) 2004, 2009 Ingres Corporation
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
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apipparm.c
**
** Description:
**	This file contains the IIapi_putParms() function interface.
**
**      IIapi_putParms  API put parameters function
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
**	11-Jan-95 (gordy)
**	    Cleaned up parameter verification.
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
**	20-Jun-95 (gordy)
**	    Application may now send any number of parameters.
**	    Set the request count in the statement handle.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
**	 5-Mar-09 (gordy)
**	    Validate parameter data.
*/




/*
** Name: IIapi_putParms
**
** Description:
**	This function provide an interface for the frontend application
**      to send parameters with a previously invoked query.  Please
**      refer to the API user specification for function description.
**
**      putParmParm  input and output parameters for IIapi_putParms()
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
**	11-Jan-95 (gordy)
**	    Cleaned up parameter verification.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	 9-Jun-95 (gordy)
**	    Added status parameter to IIapi_localError().
**	20-Jun-95 (gordy)
**	    Application may now send any number of parameters.
**	    Set the request count in the statement handle.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
**	 5-Mar-09 (gordy)
**	    Validate parameter data.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_putParms( IIAPI_PUTPARMPARM II_FAR *putParmParm )
{
    IIAPI_STMTHNDL	*stmtHndl;
    II_ULONG		err_code;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_putParms: sending parameters to server\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! putParmParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_putParms: null putParms parameters\n" );
	return;
    }
    
    putParmParm->pp_genParm.gp_completed = FALSE;
    putParmParm->pp_genParm.gp_status = IIAPI_ST_SUCCESS;
    putParmParm->pp_genParm.gp_errorHandle = NULL;
    stmtHndl = (IIAPI_STMTHNDL *)putParmParm->pp_stmtHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_putParms: API is not initialized\n" );
	IIapi_appCallback( &putParmParm->pp_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isStmtHndl( stmtHndl )  ||  IIAPI_STALE_HANDLE( stmtHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_putParms: invalid statement handle\n" );
	IIapi_appCallback( &putParmParm->pp_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_putParms: stmtHndl = %p\n", stmtHndl );
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)stmtHndl );
    
    /*
    ** Validate parameter count
    */
    if ( ! IIapi_validParmCount( stmtHndl, putParmParm ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_putParms: invalid parameter count\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)stmtHndl, 
				 E_AP0010_INVALID_COLUMN_COUNT, 
				 II_SS21000_CARD_VIOLATION,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &putParmParm->pp_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &putParmParm->pp_genParm, 
			       (IIAPI_HNDL *)stmtHndl, IIAPI_ST_FAILURE );
	return;
    }
    
    if ( (err_code = IIapi_validDataValue( putParmParm->pp_parmCount, 
    					   &stmtHndl->sh_parmDescriptor[ 
						stmtHndl->sh_parmIndex ],
					   putParmParm->pp_parmData )) != OK )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_putParms: parameter data conflicts with descriptor\n" );

	/*
	** We only issue a warning on data conflicts, but if the
	** warning can't be generated then abort the operation.
	*/
	if ( ! IIapi_localError( (IIAPI_HNDL *)stmtHndl, err_code, 
				 II_SS22500_INVALID_DATA_TYPE,
				 IIAPI_ST_WARNING ) )
	{
	    IIapi_appCallback( &putParmParm->pp_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	    return;
    	}
    }
    
    /*
    ** Initialize the parameter request counter in the statement handle.
    */
    stmtHndl->sh_parmSend = putParmParm->pp_parmCount;

    IIapi_uiDispatch( IIAPI_EV_PUTPARM_FUNC, 
		      (IIAPI_HNDL *)stmtHndl, (II_PTR)putParmParm );

    return;
}
