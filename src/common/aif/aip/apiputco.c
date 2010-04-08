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
** Name: apiputco.c
**
** Description:
**	This file contains the IIapi_putColumns() function interface.
**
**      IIapi_putColumns  API put columns function
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
**	    Cleaning up descriptor handling.
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
**	22-Apr-96 (gordy)
**	    Finished support for COPY FROM.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
**	 5-Mar-09 (gordy)
**	    Validate column data.
**	12-Mar-09 (gordy)
**	    Fixing type resulting from copying from IIapi_putParms().
*/



/*
** Name: IIapi_putColumns
**
** Description:
**	This function provide an interface for the frontend application
**      to send tuple data to the DBMS server.  Please refer to the API user
**      specification for function description.
**
**      putColParm  input and output parameters for IIapi_putColumns()
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
**	    Cleaning up descriptor handling.
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
**	22-Apr-96 (gordy)
**	    Finished support for COPY FROM.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
**	 5-Mar-09 (gordy)
**	    Validate column data.
**	12-Mar-09 (gordy)
**	    Prior change copied usage of parm Descriptor and Index
**	    form IIapi_putParms() implementation which should have
**	    been column Descriptor and Index.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_putColumns( IIAPI_PUTCOLPARM II_FAR *putColParm )
{
    IIAPI_STMTHNDL	*stmtHndl;
    II_ULONG		err_code;
    
    IIAPI_TRACE( IIAPI_TR_TRACE ) 
	( "IIapi_putColumns: sending tuple to server\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! putColParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_putColumns: null putColumns parameters\n" );
	return;
    }
    
    putColParm->pc_genParm.gp_completed = FALSE;
    putColParm->pc_genParm.gp_status = IIAPI_ST_SUCCESS;
    putColParm->pc_genParm.gp_errorHandle = NULL;
    stmtHndl = (IIAPI_STMTHNDL *)putColParm->pc_stmtHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_putColumns: API is not initialized\n" );
	IIapi_appCallback( &putColParm->pc_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }

    if ( ! IIapi_isStmtHndl( stmtHndl )  ||  IIAPI_STALE_HANDLE( stmtHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_putColumns: invalid statement handle\n" );
	IIapi_appCallback( &putColParm->pc_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_putColumns: stmtHndl = %p\n", stmtHndl );
    
    IIapi_clearAllErrors( (IIAPI_HNDL *)stmtHndl );
    
    if ( ! IIapi_validPColCount( stmtHndl, putColParm ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ("IIapi_putColumns: invalid count: %d (actual %d, index %d)\n",
	     (II_LONG)putColParm->pc_columnCount, 
	     (II_LONG)stmtHndl->sh_colCount,
	     (II_LONG)stmtHndl->sh_colIndex );

	if ( ! IIapi_localError( (IIAPI_HNDL *)stmtHndl, 
				 E_AP0010_INVALID_COLUMN_COUNT, 
				 II_SS21000_CARD_VIOLATION,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &putColParm->pc_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &putColParm->pc_genParm, 
			       (IIAPI_HNDL *)stmtHndl, IIAPI_ST_FAILURE );
	return;
    }
    
    if ( (err_code = IIapi_validDataValue( putColParm->pc_columnCount, 
    					   &stmtHndl->sh_colDescriptor[ 
						stmtHndl->sh_colIndex ],
					   putColParm->pc_columnData )) != OK )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_putColumns: column data conflicts with descriptor\n" );

	/*
	** We only issue a warning on data conflicts, but if the
	** warning can't be generated then abort the operation.
	*/
	if ( ! IIapi_localError( (IIAPI_HNDL *)stmtHndl, err_code, 
				 II_SS22500_INVALID_DATA_TYPE,
				 IIAPI_ST_WARNING ) )
	{
	    IIapi_appCallback( &putColParm->pc_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	    return;
    	}
    }
    
    stmtHndl->sh_colFetch = putColParm->pc_columnCount;

    IIapi_uiDispatch( IIAPI_EV_PUTCOLUMN_FUNC, 
		      (IIAPI_HNDL *)stmtHndl, (II_PTR)putColParm );

    return;
}
