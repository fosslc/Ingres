/*
** Copyright (c) 2004, 2008 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apishndl.h>
# include <apiehndl.h>
# include <apierhnd.h>
# include <apidspth.h>
# include <apivalid.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apigetco.c
**
** Description:
**	This file contains the IIapi_getColumns() function interface.
**
**      IIapi_getColumns  API get columns function
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
**	 31-may-94 (ctham)
**	     Make sure all columns have allocated memory.
**	10-Nov-94 (gordy)
**	    Added ability to return multiple rows.
**	20-Dec-94 (gordy)
**	    Clean up common handle processing.
**	11-Jan-95 (gordy)
**	    Cleaning up descriptor handling.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	 9-Mar-95 (gordy)
**	    Silence compiler warnings.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	 9-Jun-95 (gordy)
**	    Added status parameter to IIapi_localError().
**	13-Jun-95 (gordy)
**	    Application may now request any number of columns.
**	    Set the request count in the statement handle.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	19-Mar-97 (gordy)
**	    Initialize the data value lengths to zero.  Blob
**	    processing relies on this initial value.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Dec-02 (gordy)
**	    Initialize null indicator to FALSE for non-nullable columns.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
**	11-Aug-08 (gordy)
**	    Added column info array.
*/




/*
** Name: IIapi_getColumns
**
** Description:
**	This function provide an interface for the frontend application
**      retrieve tuple data from the DBMS server.  Please refer to the API user
**      specification for function description.
**
**      getColParm  input and output parameters for IIapi_getColumns()
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
**	 31-may-94 (ctham)
**	     Make sure all columns have allocated memory.
**	10-Nov-94 (gordy)
**	    Added ability to return multiple rows.
**	20-Dec-94 (gordy)
**	    Clean up common handle processing.
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
**	13-Jun-95 (gordy)
**	    Application may now request any number of columns.
**	    Set the request count in the statement handle.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	19-Mar-97 (gordy)
**	    Initialize the data value lengths to zero.  Blob
**	    processing relies on this initial value.
**	11-Dec-02 (gordy)
**	    Initialize null indicator to FALSE for non-nullable columns.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
**	11-Aug-08 (gordy)
**	    Initialize the column info array.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_getColumns( IIAPI_GETCOLPARM II_FAR *getColParm )
{
    IIAPI_HNDL	*handle;
    i4		i;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_getColumns: retrieving columns from server\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! getColParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getColumns: null getColumns parameters\n" );
	return;
    }
    
    getColParm->gc_genParm.gp_completed = FALSE;
    getColParm->gc_genParm.gp_status = IIAPI_ST_SUCCESS;
    getColParm->gc_genParm.gp_errorHandle = NULL;
    getColParm->gc_rowsReturned = 0;
    getColParm->gc_moreSegments = FALSE;
    handle = (IIAPI_HNDL *)getColParm->gc_stmtHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getColumns: API is not initialized\n" );
	IIapi_appCallback( &getColParm->gc_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ( ! IIapi_isStmtHndl( (IIAPI_STMTHNDL *)handle )  &&  
           ! IIapi_isDbevHndl( (IIAPI_DBEVHNDL *)handle ) )  ||
	 IIAPI_STALE_HANDLE( handle ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getColumns: invalid handle\n" );
	IIapi_appCallback( &getColParm->gc_genParm, 
			   NULL, IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    IIAPI_TRACE( IIAPI_TR_INFO )( "IIapi_getColumns: handle = %p\n", handle );
    
    IIapi_clearAllErrors( handle );
    
    if ( ! IIapi_validGColCount( (IIAPI_STMTHNDL *)handle, getColParm ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getColumns: invalid count: %d (actual %d, index %d)\n",
	      (II_LONG)getColParm->gc_columnCount, 
	      (II_LONG)(((IIAPI_STMTHNDL *)handle)->sh_colCount), 
	      (II_LONG)(((IIAPI_STMTHNDL *)handle)->sh_colIndex) );

	if ( ! IIapi_localError( handle, E_AP0010_INVALID_COLUMN_COUNT, 
				 II_SS21000_CARD_VIOLATION, IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &getColParm->gc_genParm, 
			       NULL, IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &getColParm->gc_genParm, 
			       handle, IIAPI_ST_FAILURE );
	return;
    }
    
    if ( ! getColParm->gc_columnData )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getColumns: no column data array\n" );

	if ( ! IIapi_localError( handle, E_AP0013_INVALID_POINTER, 
				 II_SS22005_ASSIGNMENT_ERROR,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &getColParm->gc_genParm, 
			       NULL, IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &getColParm->gc_genParm, 
			       handle, IIAPI_ST_FAILURE );
	return;
    }
    
    /*
    ** Make sure all columns have allocated memory.
    ** We also need to make sure the data value length
    ** is initialized to zero.  Blob segment processing
    ** relies on this initial value.  The null indicator
    ** does not get set elsewhere for non-nullable 
    ** columns, so for the indicator FALSE initially.
    */
    for( i = 0; i < getColParm->gc_rowCount * getColParm->gc_columnCount; i++ )
	if ( getColParm->gc_columnData[ i ].dv_value )
	{
	    getColParm->gc_columnData[ i ].dv_length = 0;
	    getColParm->gc_columnData[ i ].dv_null = FALSE;
	}
	else
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		("IIapi_getColumns: no data value for column in data array\n");

	    if ( ! IIapi_localError( handle, E_AP0013_INVALID_POINTER, 
				     II_SS22005_ASSIGNMENT_ERROR,
				     IIAPI_ST_FAILURE ) )
		IIapi_appCallback( &getColParm->gc_genParm, 
				   NULL, IIAPI_ST_OUT_OF_MEMORY );
	    else
		IIapi_appCallback( &getColParm->gc_genParm, 
				   handle, IIAPI_ST_FAILURE );
	    return;
	}

    /*
    ** Initialize the request in the statement handle.
    */
    if ( IIapi_isStmtHndl( (IIAPI_STMTHNDL *)handle ) )
    {
	IIAPI_STMTHNDL *stmtHndl = (IIAPI_STMTHNDL *)handle;

	/*
	** Allocate the column info array if it hasn't been allocated.
	*/
        if ( ! stmtHndl->sh_colInfo )
	{
	    STATUS	status;

	    stmtHndl->sh_colInfo = (IIAPI_COLINFO *)
	    	MEreqmem( 0, sizeof(IIAPI_COLINFO) * stmtHndl->sh_colCount, 
			  TRUE, &status );

	    if ( ! stmtHndl->sh_colInfo )
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		    ("IIapi_getColumns: couldn't allocate column info array\n");

		IIapi_appCallback( &getColParm->gc_genParm, 
				   NULL, IIAPI_ST_OUT_OF_MEMORY );
	        return;
	    }
	}

	/*
	** Initialize the column info entries at the start of a new row.  
	** Note that we may have already begun processing the first 
	** column if it is a LOB in which case the more segments flag 
	** will be set.
	*/
	if ( stmtHndl->sh_colIndex == 0  &&  
	     ! (stmtHndl->sh_flags & IIAPI_SH_MORE_SEGMENTS) )
	{
	    i2 i;

	    for( i = 0; i < stmtHndl->sh_colCount; i++ )
	    	stmtHndl->sh_colInfo[ i ].ci_flags = 0;
	}

	stmtHndl->sh_colFetch = getColParm->gc_columnCount;
    }

    IIapi_uiDispatch( IIAPI_EV_GETCOLUMN_FUNC, handle, (II_PTR)getColParm );

    return;
}
