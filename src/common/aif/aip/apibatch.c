/*
** Copyright (c) 2010 Ingres Corporation
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
** Name: apibatch.c
**
** Description:
**	This file contains the IIapi_batch() function interface.
**
**      IIapi_batch  API batch query function
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/


static	II_VOID		initStmtHndl( IIAPI_STMTHNDL * );


/*
** Name: IIapi_batch
**
** Description:
**	This function provides an interface for the frontend application
**      to invoke a group, or batch, of queries at the DBMS server.  
**	Please refer to the API user specification for function description.
**
**      batchParm  input and output parameters for IIapi_batch()
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_batch( IIAPI_BATCHPARM II_FAR *batchParm )
{
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_TRANHNDL	*tranHndl;
    IIAPI_STMTHNDL	*stmtHndl;
    IIAPI_HNDL		*handle;
    
    IIAPI_TRACE( IIAPI_TR_TRACE )( "IIapi_batch: adding a batch query\n" );
    
    /*
    ** Validate Input parameters
    */
    if ( ! batchParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_batch: null batch parameters\n" );
	return;
    }
    
    batchParm->ba_genParm.gp_completed = FALSE;
    batchParm->ba_genParm.gp_status = IIAPI_ST_SUCCESS;
    batchParm->ba_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)batchParm->ba_connHandle;
    handle = (IIAPI_HNDL *)batchParm->ba_tranHandle;
    stmtHndl = (IIAPI_STMTHNDL *)batchParm->ba_stmtHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_batch: API is not initialized\n" );
	IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			   IIAPI_ST_NOT_INITIALIZED );
	return;
    }
    
    /*
    ** Validate connection handle.
    */
    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_batch: invalid connHndl %p\n", connHndl );
	IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }
    
    /*
    ** Validate transaction handle.
    */
    if ( handle  &&  ! IIapi_isTranName( (IIAPI_TRANNAME *)handle )  &&
                    (! IIapi_isTranHndl( (IIAPI_TRANHNDL *)handle )  ||
	             IIAPI_STALE_HANDLE( handle )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_batch: invalid tranHndl %p\n", handle );
	IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			   IIAPI_ST_INVALID_HANDLE );
	return;
    }

    /*
    ** Validate statement handle.
    */
    if ( stmtHndl )
    {
    	if ( ! IIapi_isStmtHndl( (IIAPI_STMTHNDL *)stmtHndl )  ||
	     IIAPI_STALE_HANDLE( stmtHndl ) )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_batch: invalid stmtHndl %p\n", stmtHndl );
	    IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			       IIAPI_ST_INVALID_HANDLE );
	    return;
	}

	if ( ! handle  ||  IIapi_isTranName( (IIAPI_TRANNAME *)handle) )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_batch: transaction handle required\n" );
	    IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			       IIAPI_ST_INVALID_HANDLE );
	    return;
	}

    	if ( handle != (IIAPI_HNDL *)stmtHndl->sh_tranHndl )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_batch: wrong tranHndl %p for stmtHndl %p\n",
		  handle, stmtHndl );
	    IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			       IIAPI_ST_INVALID_HANDLE );
	    return;
	}

	if ( connHndl != IIapi_getConnHndl( (IIAPI_HNDL *)stmtHndl ) )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_batch: wrong connHndl %p for stmtHndl %p\n",
		  connHndl, stmtHndl );
	    IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			       IIAPI_ST_INVALID_HANDLE );
	    return;
	}
    }

    IIAPI_TRACE( IIAPI_TR_INFO )
	( "IIapi_batch: connHndl = %p, tranHndl = %p, stmtHndl = %p, queryType = %d\n",
	  batchParm->ba_connHandle, batchParm->ba_tranHandle,
	  batchParm->ba_stmtHandle, batchParm->ba_queryType );
    
    IIAPI_TRACE( IIAPI_TR_INFO )
	( "IIapi_batch: queryText = %s\n",
	  batchParm->ba_queryText ? batchParm->ba_queryText : "<NULL>" );
    
    IIapi_clearAllErrors( stmtHndl ? (IIAPI_HNDL *)stmtHndl :
	( (handle && IIapi_isTranHndl( (IIAPI_TRANHNDL *)handle ))
	  ? handle : (IIAPI_HNDL *)connHndl ) );
    
    /*
    ** Check that batch processing is supported on the connection.
    */
    if ( connHndl->ch_partnerProtocol < GCA_PROTOCOL_LEVEL_68 )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_batch: batch not supported at protocol level %d\n", 
	      connHndl->ch_partnerProtocol );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP001F_BATCH_UNSUPPORTED, 
				 II_SS50008_UNSUPPORTED_STMT,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &batchParm->ba_genParm, 
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	return;
    }

    /*
    ** Check restrictions on query type and associated info.
    */
    if ( connHndl->ch_type != IIAPI_SMT_SQL  ||
	 (batchParm->ba_queryType != IIAPI_QT_QUERY  &&
	  batchParm->ba_queryType != IIAPI_QT_EXEC   &&
	  batchParm->ba_queryType != IIAPI_QT_EXEC_PROCEDURE) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_batch: invalid query type %d for connection type %d\n", 
	      batchParm->ba_queryType, connHndl->ch_type );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0011_INVALID_PARAM_VALUE, 
				 II_SS50008_UNSUPPORTED_STMT,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &batchParm->ba_genParm, 
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	return;
    }

    if ( batchParm->ba_queryType != IIAPI_QT_EXEC_PROCEDURE  &&
         ! batchParm->ba_queryText )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_batch: query requires query text\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0011_INVALID_PARAM_VALUE, 
				 II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &batchParm->ba_genParm, 
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	return;
    }

    if ( batchParm->ba_queryType == IIAPI_QT_EXEC_PROCEDURE  &&
         ! batchParm->ba_parameters )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_batch: query requires parameters, none indicated\n" );

	if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				 E_AP0011_INVALID_PARAM_VALUE, 
				 II_SS5000R_RUN_TIME_LOGICAL_ERROR,
				 IIAPI_ST_FAILURE ) )
	    IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	else
	    IIapi_appCallback( &batchParm->ba_genParm, 
			       (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	return;
    }

    /*
    ** Allocate a transaction handle if one was not provided.
    */
    if ( ! handle  ||  IIapi_isTranName( (IIAPI_TRANNAME *)handle ) )
    {
	/*
	** Check to see if there is an active transaction.
	*/
	if ( ! IIapi_isQueEmpty( (QUEUE *)&connHndl->ch_tranHndlList ) )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		("IIapi_batch: connection has active transaction.\n");

	    if ( ! IIapi_localError( (IIAPI_HNDL *)connHndl, 
				     E_AP0003_ACTIVE_TRANSACTIONS, 
				     II_SS25000_INV_XACT_STATE,
				     IIAPI_ST_FAILURE ) )
		IIapi_appCallback( &batchParm->ba_genParm, NULL, 
				   IIAPI_ST_OUT_OF_MEMORY );
	    else
		IIapi_appCallback( &batchParm->ba_genParm, 
				   (IIAPI_HNDL *)connHndl, IIAPI_ST_FAILURE );
	    return;
	}

	if ( ! ( tranHndl = IIapi_createTranHndl(
				(IIAPI_TRANNAME *)batchParm->ba_tranHandle,
				(IIAPI_CONNHNDL *)batchParm->ba_connHandle ) ) )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_batch: createTranHndl failed\n" );
	    IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			       IIAPI_ST_OUT_OF_MEMORY );
	    return;
	}
    }
    else
    {
	/*
	** Use existing transaction handle.
	*/
	tranHndl = (IIAPI_TRANHNDL *)handle;
	IIapi_clearAllErrors( (IIAPI_HNDL *)tranHndl );
    }
    
    /*
    ** Allocate a new statement handle or initialize existing handle.
    */
    if ( stmtHndl )
	initStmtHndl( stmtHndl );
    else  if ( ! (stmtHndl = IIapi_createStmtHndl( connHndl, tranHndl )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_batch: createStmtHndl failed\n" );

	IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			   IIAPI_ST_OUT_OF_MEMORY );
	goto freeTranHandle;
    }

    /*
    ** Update statement handle with batch query information.
    */
    if ( batchParm->ba_queryText  &&
	 ! (stmtHndl->sh_queryText = STalloc( batchParm->ba_queryText )) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_batch: can't alloc query text\n" );

	IIapi_appCallback( &batchParm->ba_genParm, NULL, 
			   IIAPI_ST_OUT_OF_MEMORY );
	goto freeStmtHandle;
    }
    
    stmtHndl->sh_queryType = batchParm->ba_queryType;
    if ( batchParm->ba_parameters )  stmtHndl->sh_flags |= IIAPI_SH_PARAMETERS;

    /*
    ** The current set of external batch/query flags
    ** are not applicable to batch processing, but
    ** we pass them along anyway.
    */
    stmtHndl->sh_flags |= batchParm->ba_flags & IIAPI_QF_ALL_FLAGS;

    batchParm->ba_stmtHandle = (II_PTR)stmtHndl;
    batchParm->ba_tranHandle = (II_PTR)tranHndl;
    
    IIapi_uiDispatch( IIAPI_EV_BATCH_FUNC, 
		      (IIAPI_HNDL *)stmtHndl, (II_PTR)batchParm );
    return;

  freeStmtHandle:

    /*
    ** If the statement handle is not the input handle,
    ** then we allocated a new handle above and we must
    ** now delete it.
    */
    if ( stmtHndl != (IIAPI_STMTHNDL *)batchParm->ba_stmtHandle )
	IIapi_deleteStmtHndl( stmtHndl );

  freeTranHandle:

    /*
    ** If the transaction handle is not the input handle,
    ** then we allocated a new handle above and we must
    ** now delete it.
    */
    if ( tranHndl != (IIAPI_TRANHNDL *)batchParm->ba_tranHandle )
	IIapi_deleteTranHndl( tranHndl );

    return;
}


/*
** Name: initStmtHndl
**
** Description:
**	Initialize a statement handle to a state similar to that
**	established by IIapi_createStmtHndl().  This prepares the
**	statement handle to process an additional batch request.
**
** Input:
**	stmtHndl	Statement handle.
**
** Output:
**	None.
**
** Returns:
**	VOID.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

static II_VOID
initStmtHndl( IIAPI_STMTHNDL *stmtHndl )
{
    stmtHndl->sh_flags &= ~(IIAPI_SH_PARAMETERS | IIAPI_QF_ALL_FLAGS);

    if ( stmtHndl->sh_queryText )  MEfree( stmtHndl->sh_queryText );
    stmtHndl->sh_queryText = NULL;

    if ( stmtHndl->sh_parmDescriptor )
    {
	i4 i;

	for( i = 0; i < stmtHndl->sh_parmCount; i++ )
	    if ( stmtHndl->sh_parmDescriptor[i].ds_columnName )
	    	MEfree( (II_PTR)stmtHndl->sh_parmDescriptor[i].ds_columnName );

	MEfree( (II_PTR)stmtHndl->sh_parmDescriptor );
    	stmtHndl->sh_parmDescriptor = NULL;
    }

    stmtHndl->sh_parmCount = 0;
    stmtHndl->sh_parmIndex = 0;
    stmtHndl->sh_parmSend = 0;
    return;
}

