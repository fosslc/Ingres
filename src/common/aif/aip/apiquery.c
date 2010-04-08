/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <st.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apitname.h>
# include <apierhnd.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apiquery.c
**
** Description:
**	This file contains the IIapi_query() function interface.
**
**      IIapi_query  API query function
**
** History:
**      01-oct-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          Let the dispatcher do the deletion of transaction handle.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      17-may-94 (ctham)
**          use II_EXPORT for functions which will cross the interface
**          between API and the application.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	16-Feb-95 (gordy)
**	    Added qy_parameters and test for required parameters.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	 9-Jun-95 (gordy)
**	    Added status parameter to IIapi_localError().
**	20-Jun-94 (gordy)
**	    Ensure query text is present when required.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	13-Mar-97 (gordy)
**	    Query type validation no based on connection type.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
**	11-Oct-05 (gordy)
**	    Clear errors on transaction handle (if provided).
**	25-Mar-10 (gordy)
**	    Changes to allow batch processing to share statement handle.
*/


/*
** Valid query type ranges 
** based on connection type.
*/
static struct
{

    IIAPI_QUERYTYPE	qt_min;
    IIAPI_QUERYTYPE	qt_max;

} api_queries[] =
{
    { IIAPI_QT_BASE,		IIAPI_QT_TOP },		/* SQL */
    { IIAPI_QT_TOP + 1, 	IIAPI_QT_TOP + 1 },	/* Unused */
    { IIAPI_QT_QUERY,		IIAPI_QT_QUERY }	/* Name Server */
};


/*
** Name: IIapi_query
**
** Description:
**	This function provide an interface for the frontend application
**      to invoke a query at the DBMS server.  Please refer to the API user
**      specification for function description.
**
**      queryParm  input and output parameters for IIapi_query()
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
**          Let the dispatcher do the deletion of transaction handle.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      17-may-94 (ctham)
**          use II_EXPORT for functions which will cross the interface
**          between API and the application.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	 31-may-94 (ctham)
**	     validate transaction handle.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	10-Feb-95 (gordy)
**	    Make callback for all errors.
**	16-Feb-95 (gordy)
**	    Added qy_parameters and test for required parameters.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	 9-Jun-95 (gordy)
**	    Added status parameter to IIapi_localError().
**	20-Jun-94 (gordy)
**	    Ensure query text is present when required.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	13-Mar-97 (gordy)
**	    Query type validation no based on connection type.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
**	11-Oct-05 (gordy)
**	    Clear errors on transaction handle (if provided).
**	21-Oct-2005 (schka24)
**	    Reposition cast so that older Sun C compiler doesn't spazz.
**	25-Mar-10 (gordy)
**	    Query specific parts of statement handle now initialized
**	    locally.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_query( IIAPI_QUERYPARM II_FAR *queryParm )
{
    IIAPI_ENVHNDL	*envHndl;
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_TRANHNDL	*tranHndl;
    IIAPI_STMTHNDL	*stmtHndl;
    IIAPI_HNDL		*handle;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_query: starting a query\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! queryParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_query: null query parameters\n" );
	return;
    }
    
    queryParm->qy_genParm.gp_completed = FALSE;
    queryParm->qy_genParm.gp_status = IIAPI_ST_SUCCESS;
    queryParm->qy_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)queryParm->qy_connHandle;
    handle = (IIAPI_HNDL *)queryParm->qy_tranHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_query: API is not initialized\n" );
	IIapi_appCallback( &queryParm->qy_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_query: invalid connection handle\n" );
	IIapi_appCallback( &queryParm->qy_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    if ( handle  &&  ! IIapi_isTranName( (IIAPI_TRANNAME *)handle )  &&
                    (! IIapi_isTranHndl( (IIAPI_TRANHNDL *)handle )  ||
	             IIAPI_STALE_HANDLE( handle )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_query: invalid transaction handle\n" );
	IIapi_appCallback( &queryParm->qy_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }

    IIAPI_TRACE( IIAPI_TR_INFO )
	("IIapi_query: connHndl = %p, tranHndl = %p, queryType = %d\n",
	 queryParm->qy_connHandle, queryParm->qy_tranHandle,
	 queryParm->qy_queryType);
    
    IIAPI_TRACE( IIAPI_TR_INFO )
	( "IIapi_query: queryText = %s\n",
	  queryParm->qy_queryText ? queryParm->qy_queryText : "NULL" );
    
    IIapi_clearAllErrors(
	( (handle && IIapi_isTranHndl( (IIAPI_TRANHNDL *)handle ))
	  ? handle : (IIAPI_HNDL *) connHndl ) );
    
    if ( queryParm->qy_queryType < api_queries[ connHndl->ch_type ].qt_min  ||
	 queryParm->qy_queryType > api_queries[ connHndl->ch_type ].qt_max )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_query: invalid query type %d\n", 
	      queryParm->qy_queryType );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0011_INVALID_PARAM_VALUE, 
				 II_SS22003_NUM_VAL_OUT_OF_RANGE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &queryParm->qy_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &queryParm->qy_genParm, 
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	return;
    }
    
    if ( queryParm->qy_queryType != IIAPI_QT_EXEC_PROCEDURE  &&
	 queryParm->qy_queryType != IIAPI_QT_EXEC_REPEAT_QUERY  &&
	 ! queryParm->qy_queryText )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_query: query requires query text\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0011_INVALID_PARAM_VALUE, 
				 II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &queryParm->qy_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &queryParm->qy_genParm, 
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	return;
    }

    if ( ( queryParm->qy_queryType == IIAPI_QT_CURSOR_DELETE   ||
	   queryParm->qy_queryType == IIAPI_QT_CURSOR_UPDATE   ||
	   queryParm->qy_queryType == IIAPI_QT_EXEC_PROCEDURE  ||
	   queryParm->qy_queryType == IIAPI_QT_EXEC_REPEAT_QUERY )  &&
	 ! queryParm->qy_parameters )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_query: query requires parameters, none indicated\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0011_INVALID_PARAM_VALUE, 
				 II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &queryParm->qy_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &queryParm->qy_genParm, 
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	return;
    }

    if ( ( ! handle  ||  IIapi_isTranName( (IIAPI_TRANNAME *)handle ) )  &&
	 ! IIapi_isQueEmpty( (QUEUE *)&connHndl->ch_tranHndlList ) )
    {
	/*
	** Query is attempting to start a transaction with a transaction
	** already active.  Currently, only one transaction can be active 
	** in a connection.
	*/
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ("IIapi_query: multiple transaction per connection not allowed\n");

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0003_ACTIVE_TRANSACTIONS, 
				 II_SS25000_INV_XACT_STATE,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &queryParm->qy_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &queryParm->qy_genParm, 
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	return;
    }
	
    /*
    ** Create a transaction handle if we don't already have one.
    */
    if ( ( ! handle  ||  IIapi_isTranName( (IIAPI_TRANNAME *)handle ) ) )
    {
	if ( ! ( tranHndl = IIapi_createTranHndl(
				(IIAPI_TRANNAME *)queryParm->qy_tranHandle,
				(IIAPI_CONNHNDL *)queryParm->qy_connHandle ) ) )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_query: createTranHndl failed\n" );
	    IIapi_appCallback( &queryParm->qy_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	    return;
	}
    }
    else
    {
	tranHndl = (IIAPI_TRANHNDL *)handle;
	IIapi_clearAllErrors( (IIAPI_HNDL *)tranHndl );
    }
    
    /*
    ** Create a statement handle for this query.
    */
    if ( ! (stmtHndl = IIapi_createStmtHndl( connHndl, tranHndl )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_query: createStmtHndl failed\n" );

	IIapi_appCallback( &queryParm->qy_genParm, NULL, 
			   IIAPI_ST_OUT_OF_MEMORY );
	goto freeTranHandle;
    }
    
    if ( queryParm->qy_queryText  &&
	 ! (stmtHndl->sh_queryText = STalloc( queryParm->qy_queryText )) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_query: can't alloc query text\n" );

	IIapi_appCallback( &queryParm->qy_genParm, NULL, 
			   IIAPI_ST_OUT_OF_MEMORY );
	goto freeStmtHandle;
    }

    stmtHndl->sh_queryType = queryParm->qy_queryType;
    envHndl = IIapi_getEnvHndl( (IIAPI_HNDL *)connHndl );

    if ( envHndl->en_version >= IIAPI_VERSION_6 )
    	stmtHndl->sh_flags |= queryParm->qy_flags & IIAPI_QF_ALL_FLAGS;

    if ( queryParm->qy_parameters )  stmtHndl->sh_flags |= IIAPI_SH_PARAMETERS;

    queryParm->qy_stmtHandle = (II_PTR)stmtHndl;
    queryParm->qy_tranHandle = (II_PTR)tranHndl;
    
    IIapi_uiDispatch( IIAPI_EV_QUERY_FUNC, 
		      (IIAPI_HNDL *)stmtHndl, (II_PTR)queryParm );
    return;

  freeStmtHandle:

    IIapi_deleteStmtHndl( stmtHndl );

  freeTranHandle:

    /*
    ** If the transaction handle is not the input handle,
    ** then we allocated a new handle above and we must
    ** now delete it.
    */
    if ( tranHndl != (IIAPI_TRANHNDL *)handle )
	IIapi_deleteTranHndl( tranHndl );

    return;
}
