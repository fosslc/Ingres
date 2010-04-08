/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <cm.h>
# include <er.h>
# include <st.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apiehndl.h>
# include <apierhnd.h>
# include <apidatav.h>
# include <apige2ss.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apierhnd
**
** Description:
**	This file defines the error handle management functions.
**
**	IIapi_createErrorHndl	Create an error handle.
**	IIapi_serverError	Create an server error.
**	IIapi_localError	Create a local error.
**      IIapi_getErrorHndl	Get error handle from error list.
**      IIapi_cleanErrorHndl	Clean error list.
**	IIapi_clearAllErrors	Clean errors from all accessibl handles.
**
** History:
**      30-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	19-Dec-94 (gordy)
**	    Fix sQLState for USER messages.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	13-Feb-95 (gordy)
**	    Enhanced tracing.
**	 9-Mar-95 (gordy)
**	    Silence compiler warnings.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	22-May-95 (gordy)
**	    Treat user messages as already formatted.
**	 9-Jun-95 (gordy)
**	    Added status parameter to IIapi_localError().
**	13-Jun-95 (gordy)
**	    Allocate data buffer before calling IIapi_cnvtGDV2DataValue().
**	28-Jul-95 (gordy)
**	    Use fixed length types.  Fixed tracing of pointers.
**	14-Sep-95 (gordy)
**	    GCD_GCA_ prefix changed to GCD_.
**	11-Oct-95 (gordy)
**	    Removed IIapi_initError() and IIapi_termError().  We do not
**	    use individual message files any longer.  The only other
**	    reason to call ERinit() is for semaphore routines.  Client
**	    side API does not need this and if we are in a server, then
**	    server code should be setting up ER for us.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 8-Mar-96 (gordy)
**	    Added support for processing GCA_ERROR messages which are
**	    are split across buffers.
**	15-Jul-96 (gordy)
**	    Allow for no error text.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	    Added IIapi_saveErrors().  Environment handles can now
**	    hold error handles.
**      21-Aug-98 (rajus01)
**          Added environment parameter to IIapi_cnvtGDVDescr().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	11-Dec-02 (gordy)
**	    Added tracing of DBMS error messages.
**	11-Jul-06 (gordy)
**	    Added IIapi_xaError().
**	14-Jul-2006 (kschendel)
**	    Remove u_i2 casts from MEcopy calls.
**	25-Mar-10 (gordy)
**	    Fixed condition where only the errors for the first failing
**	    statement in a batch are returned and errors in subsequent
**	    statements are lost.
*/



/*
** Local function
*/
static	II_VOID		deleteErrorHndl( IIAPI_ERRORHNDL *errorHndl );


/*
** Ingres mainline function which has no home
*/
II_EXTERN II_VOID ss_decode( char *sqlstate, i4 encoded_ss );




/*
** Name: IIapi_createErrorHndl
**
** Description:
**	This function creates a error handle.
**
**	handle
**	    Handle to which the error is added.
**
** Return value:
**      status     TRUE if errors are added to error list, FALSE otherwise.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      19-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	19-Dec-94 (gordy)
**	    Fix sQLState for USER messages.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	25-Mar-10 (gordy)
**	    Rather than checking for an empty error list to determine
**	    that the current error Queue pointer should be set, check
**	    to see if the error Queue currently points at the head of
**	    the error list.  The former condition only occurs when no
**	    errors have been generated.  The latter will be true for
**	    the same condition, but will also be true when errors have
**	    been generated and retrieved by the application.  If errors
**	    are then subsequently added, the error Queue needs to be
**	    set to the new errors.  This latter condition can occur
**	    during batch processing.
*/

II_EXTERN IIAPI_ERRORHNDL *
IIapi_createErrorHndl( IIAPI_HNDL *handle )
{
    IIAPI_ERRORHNDL	*errorHndl;
    STATUS		status;
    
    if ( ! ( errorHndl = (IIAPI_ERRORHNDL *)
	     MEreqmem( 0, sizeof(IIAPI_ERRORHNDL), TRUE, &status ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createErrorHndl: can't alloc error handle\n" );
	return( NULL );
    }
	
    QUinit( &errorHndl->er_id.hi_queue );
    errorHndl->er_id.hi_hndlID = IIAPI_HI_ERROR;

    if ( handle->hd_errorQue == &handle->hd_errorList )
	handle->hd_errorQue = (QUEUE *)errorHndl;

    /*
    ** Insert at end of queue.
    */
    QUinsert( (QUEUE *)errorHndl, handle->hd_errorList.q_prev );
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_createErrorHndl: created error handle %p on handle %p\n", 
	  errorHndl, handle );

    return( errorHndl );
}




/*
** Name: serverErrorText
**
** Description:
**	Extracts and formats the messsage text from
**	the GCA error parameters.
**
** History:
**	23-Jan-95 (gordy)
**	    Created.
**	22-May-95 (gordy)
**	    Treat user messages as already formatted.
**	28-Jul-95 (gordy)
**	    Used fixed length types.
*/

static II_BOOL
serverErrorText( IIAPI_ERRORHNDL *errorHndl )
{
    STATUS	status;
    char	*ptr, *end;
    i4		i, len;

    /*
    ** If the first parm is a string, we use it as the message text.
    */
    if ( errorHndl->er_serverInfo->svr_parmCount  &&
	 errorHndl->er_serverInfo->svr_parmDescr[0].ds_dataType == 
							    IIAPI_CHA_TYPE )
    {
	ptr = errorHndl->er_serverInfo->svr_parmValue[0].dv_value;
	end = ptr + errorHndl->er_serverInfo->svr_parmValue[0].dv_length;

	if ( ! ( errorHndl->er_serverInfo->svr_severity & GCA_ERFORMATTED )  &&
	     ! ( errorHndl->er_serverInfo->svr_severity & GCA_ERMESSAGE ) )
	{
	    /*
	    ** For unformatted messages we remove
	    ** the leading timestamp and name.  We
	    ** look for the '_' in the name (should
	    ** occur in the first 30 characters)
	    ** then move forward to the first word
	    ** following the name.
	    */
	    for( i = 30; i  &&  ptr < end  &&  *ptr != '_'; i-- )
		CMnext( ptr );

	    if ( *ptr != '_' )
		ptr = errorHndl->er_serverInfo->svr_parmValue[0].dv_value;
	    else
	    {
		/*
		** Move past the name and upto the start of the text.
		*/
		while( ptr < end  &&  ! CMwhite( ptr ) )  CMnext( ptr );
		while( ptr < end  &&  CMwhite( ptr ) )  CMnext( ptr );

		if ( ptr >= end )
		    ptr = errorHndl->er_serverInfo->svr_parmValue[0].dv_value;
	    }
	}

	len = errorHndl->er_serverInfo->svr_parmValue[0].dv_length -
	    (ptr - (char *)errorHndl->er_serverInfo->svr_parmValue[0].dv_value);

	if ( ! ( errorHndl->er_message = 
		 MEreqmem( 0, len + 1, FALSE, &status ) ) )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "serverErrorText: can't alloc error text\n" );
	    return( FALSE );
	}

	MEcopy( ptr, len, errorHndl->er_message );
	errorHndl->er_message[ len ] = '\0';
    }
    else
    {
	errorHndl->er_message = STalloc( "Message text not available!" );

	if ( ! errorHndl->er_message )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "serverErrorText: can't alloc error text\n" );
	    return( FALSE );
	}

    }

    return( TRUE );
}



/*
** Name: IIapi_serverError
**
** Description:
**	This function creates a error handle and populates it
**	from server error info.
**
** Input:
**	handle		Handle to which the error is added
**      errInfo		Server error information.
**
** Return value:
**	IIAPI_STATUS	API Status.
**
** History:
**      19-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	19-Dec-94 (gordy)
**	    Fix sQLState for USER messages.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	13-Jun-95 (gordy)
**	    Allocate data buffer before calling IIapi_cnvtGDV2DataValue().
**	28-Jul-95 (gordy)
**	    Use fixed length types.
**	 8-Mar-96 (gordy)
**	    Added support for processing GCA_ERROR messages which are
**	    are split across buffers.
**	15-Jul-96 (gordy)
**	    Allow for no error text.
**	11-Dec-02 (gordy)
**	    Added tracing of DBMS error messages.
**	25-Mar-10 (gordy)
**	    Converted to build error handle from pre-extracted server info.
*/

II_EXTERN IIAPI_STATUS
IIapi_serverError( IIAPI_HNDL *handle, IIAPI_SVR_ERRINFO *errInfo )
{
    IIAPI_ENVHNDL	*envHndl;
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_ERRORHNDL	*errorHndl;
    II_LONG		protocolLevel = 0;
    IIAPI_STATUS	status;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_serverError: create an error handle from server info\n" );
    
    if ( ! (connHndl = IIapi_getConnHndl( handle )) )
	envHndl = IIapi_defaultEnvHndl();
    else
    {
	envHndl = connHndl->ch_envHndl;
	protocolLevel = connHndl->ch_partnerProtocol;
    }

    /*
    ** Allocate the error handle, server information block,
    ** server parm descriptor and server parm value.
    */
    if ( ! ( errorHndl = IIapi_createErrorHndl( handle ) ) )
	return( IIAPI_ST_OUT_OF_MEMORY );
	
    if ( ! (errorHndl->er_serverInfo = (IIAPI_SVR_ERRINFO *)
		MEreqmem( 0, sizeof(IIAPI_SVR_ERRINFO), FALSE, NULL )) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_serverError: can't alloc server info\n" );

	deleteErrorHndl( errorHndl );
	return( IIAPI_ST_OUT_OF_MEMORY );
    }
	
    /*
    ** By carefully controlling how the error handle
    ** is populated, clean-up can be done by simply
    ** calling deleteErrorHndl().
    */
    errorHndl->er_serverInfo->svr_parmCount = 0;
    errorHndl->er_serverInfo->svr_parmDescr = NULL;
    errorHndl->er_serverInfo->svr_parmValue = NULL;
    errorHndl->er_serverInfoAvail = TRUE;

    if ( errInfo->svr_parmCount )
    {
	errorHndl->er_serverInfo->svr_parmDescr = (IIAPI_DESCRIPTOR *)
		MEreqmem( 0, errInfo->svr_parmCount * sizeof(IIAPI_DESCRIPTOR),
			  FALSE, NULL );

	if ( ! errorHndl->er_serverInfo->svr_parmDescr )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_serverError: can't alloc server parm descr\n" );

	    deleteErrorHndl( errorHndl );
	    return( IIAPI_ST_OUT_OF_MEMORY );
	}
	
	errorHndl->er_serverInfo->svr_parmValue = (IIAPI_DATAVALUE *)
		MEreqmem( 0, errInfo->svr_parmCount * sizeof(IIAPI_DATAVALUE),
			  FALSE, NULL );

	if ( ! errorHndl->er_serverInfo->svr_parmValue ) 
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_serverError: can't alloc server parm value\n" );

	    deleteErrorHndl( errorHndl );
	    return( IIAPI_ST_OUT_OF_MEMORY );
	}
    }

    if ( errInfo->svr_severity & GCA_ERMESSAGE )
	errorHndl->er_type = IIAPI_GE_MESSAGE;
    else  if ( errInfo->svr_severity & GCA_ERWARNING )
	errorHndl->er_type = IIAPI_GE_WARNING;
    else
	errorHndl->er_type = IIAPI_GE_ERROR;
	
    if ( errorHndl->er_type == IIAPI_GE_MESSAGE )
	MEcopy( II_SS5000G_DBPROC_MESSAGE, 
		II_SQLSTATE_LEN, errorHndl->er_SQLSTATE );
    else  if ( protocolLevel < GCA_PROTOCOL_LEVEL_60 )
	MEcopy( IIapi_gen2IntSQLState( errInfo->svr_id_error, 
				       errInfo->svr_local_error ),
		II_SQLSTATE_LEN, errorHndl->er_SQLSTATE );
    else
	ss_decode( errorHndl->er_SQLSTATE, errInfo->svr_id_error );
	
    errorHndl->er_SQLSTATE[ II_SQLSTATE_LEN ] = '\0';
    errorHndl->er_errorCode = errInfo->svr_local_error;

    errorHndl->er_serverInfo->svr_id_error = errInfo->svr_id_error;
    errorHndl->er_serverInfo->svr_local_error = errInfo->svr_local_error;
    errorHndl->er_serverInfo->svr_id_server = errInfo->svr_id_server;
    errorHndl->er_serverInfo->svr_server_type = errInfo->svr_server_type;
    errorHndl->er_serverInfo->svr_severity = errInfo->svr_severity;
	
    for( 
	 errorHndl->er_serverInfo->svr_parmCount = 0;
	 errorHndl->er_serverInfo->svr_parmCount < errInfo->svr_parmCount; 
	 errorHndl->er_serverInfo->svr_parmCount++ 
       )
    {
	i4 idx = errorHndl->er_serverInfo->svr_parmCount;

	errorHndl->er_serverInfo->svr_parmValue[idx].dv_value =
	    MEreqmem( 0, errInfo->svr_parmDescr[idx].ds_length, FALSE, NULL );

	if ( ! errorHndl->er_serverInfo->svr_parmValue[idx].dv_value )
	{
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_serverError: can't alloc error parm value\n" );

	    deleteErrorHndl( errorHndl );
	    return( IIAPI_ST_OUT_OF_MEMORY );
	}

	MEcopy( &errInfo->svr_parmDescr[idx], sizeof( IIAPI_DESCRIPTOR ),
		&errorHndl->er_serverInfo->svr_parmDescr[idx] );

	errorHndl->er_serverInfo->svr_parmValue[idx].dv_null = 
					errInfo->svr_parmValue[idx].dv_null;
	errorHndl->er_serverInfo->svr_parmValue[idx].dv_length = 
					errInfo->svr_parmValue[idx].dv_length;
	MEcopy( errInfo->svr_parmValue[idx].dv_value,
		errInfo->svr_parmDescr[idx].ds_length,
		errorHndl->er_serverInfo->svr_parmValue[idx].dv_value );
    }

    if ( ! serverErrorText( errorHndl ) )
    {
	deleteErrorHndl( errorHndl );
	return( IIAPI_ST_OUT_OF_MEMORY );
    }

    if ( errorHndl->er_type == IIAPI_GE_ERROR )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "DBMS Error: (0x%x) %s\n", 
	      errorHndl->er_errorCode, errorHndl->er_message );
    }
    else  if ( errorHndl->er_type == IIAPI_GE_WARNING )
    {
	IIAPI_TRACE( IIAPI_TR_WARNING )
	    ( "DBMS Warning: (0x%x) %s\n", 
	      errorHndl->er_errorCode, errorHndl->er_message );
    }
    else
    {
	IIAPI_TRACE( IIAPI_TR_INFO )
	    ( "DBMS Message: (0x%x) %s\n", 
	      errorHndl->er_errorCode, errorHndl->er_message );
    }
    
    return( IIAPI_ST_SUCCESS );
}




/*
** Name: IIapi_localError
**
** Description:
**	This function creates a error handle and populates it
**	from a local API information.
**
**	handle		Handle to which the error is added.
**	SQLState	SQLSTATE for error.
**	errorCode	Local error code.
**	status		API status of error condition.
**
** Return value:
**      status     TRUE if errors are added to error list, FALSE otherwise.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      19-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	19-Dec-94 (gordy)
**	    Fix sQLState for USER messages.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 9-Jun-95 (gordy)
**	    Added status parameter.
*/

II_EXTERN II_BOOL
IIapi_localError
(
    IIAPI_HNDL	*handle,
    II_LONG		errorCode,
    char		*SQLState,
    IIAPI_STATUS	status
)
{
    IIAPI_ERRORHNDL	*errorHndl;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_localError: create an error handle from local API info\n" );
    
    /*
    ** Allocate the error handle and message text.
    */
    if ( ! ( errorHndl = IIapi_createErrorHndl( handle ) ) )
	return( FALSE );
	
    if ( ! ( errorHndl->er_message = STalloc( ERget( errorCode ) ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_localError: can't allocate message for code 0x%s\n",
	      errorCode );

	deleteErrorHndl( errorHndl );
	return( FALSE );
    }

    switch( status )
    {
	case IIAPI_ST_SUCCESS:
	case IIAPI_ST_MESSAGE:
	    errorHndl->er_type = IIAPI_GE_MESSAGE;
	    break;

	case IIAPI_ST_WARNING:
	case IIAPI_ST_NO_DATA:
	    errorHndl->er_type = IIAPI_GE_WARNING;
	    break;

	default :
	    errorHndl->er_type = IIAPI_GE_ERROR;
	    break;
    }

    errorHndl->er_errorCode = errorCode;
    STcopy( SQLState, errorHndl->er_SQLSTATE );
    errorHndl->er_serverInfoAvail = FALSE;

    return( TRUE );
}




/*
** Name: IIapi_xaError
**
** Description:
**	This function creates a error handle and populates it
**	from a local API information.
**
** Input:
**	handle		Handle to which the error is added.
**	xaError		XA error code.
**
** Output:
**
** Returns:
**      II_BOOL		TRUE successful, FALSE otherwise.
**
** History:
**	11-Jul-06 (gordy)
**	    Created.
*/

II_EXTERN II_BOOL
IIapi_xaError
(
    IIAPI_HNDL	*handle,
    II_LONG	xaError
)
{
    IIAPI_ERRORHNDL	*errorHndl;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_xaError: create an error handle for XA error\n" );
    
    /*
    ** Allocate the error handle.
    */
    if ( ! ( errorHndl = IIapi_createErrorHndl( handle ) ) )  return( FALSE );
	
    errorHndl->er_type = IIAPI_GE_XAERR;
    errorHndl->er_errorCode = xaError;
    errorHndl->er_message = NULL;
    MEfill( sizeof(errorHndl->er_SQLSTATE), '\0', (PTR)errorHndl->er_SQLSTATE );
    errorHndl->er_serverInfoAvail = FALSE;

    IIAPI_TRACE( IIAPI_TR_ERROR )( "XA Error: %d\n", xaError );
    return( TRUE );
}




/*
** Name: deleteErrorHndl
**
** Description:
**	This function deletes a error handle.
**
**      errorHndl  error handle to be deleted
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	28-Jul-95 (gordy)
**	    Used fixed length types.  Fixed tracing of pointers.
*/

static II_VOID
deleteErrorHndl( IIAPI_ERRORHNDL *errorHndl )
{
    i4		    i;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "deleteErrorHndl: delete an error handle %p\n", errorHndl );
    
    QUremove( (QUEUE *)errorHndl );
    errorHndl->er_id.hi_hndlID = ~IIAPI_HI_ERROR;
    
    if ( errorHndl->er_serverInfoAvail )
    {
	if ( errorHndl->er_serverInfo->svr_parmValue )
	{
	    for( i = 0; i < errorHndl->er_serverInfo->svr_parmCount; i++ )
		MEfree( errorHndl->er_serverInfo->svr_parmValue[i].dv_value );

	    MEfree( (II_PTR)errorHndl->er_serverInfo->svr_parmValue );
	}

	if ( errorHndl->er_serverInfo->svr_parmDescr )
	    MEfree( (II_PTR)errorHndl->er_serverInfo->svr_parmDescr );

	MEfree( (II_PTR)errorHndl->er_serverInfo );
    }

    if ( errorHndl->er_message )  MEfree( (II_PTR)errorHndl->er_message );
    MEfree( (II_PTR)errorHndl );

    return;
}




/*
** Name: getErrorHndl
**
** Description:
**     This function retrieves first error handle from the
**     error handle list.
**
**     handle    the handle containing the error handle list.
**
** Return value:
**     errorHndl      first error handle in the error list, NULL if the
**                    error list is empty.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
*/

static IIAPI_ERRORHNDL *
getErrorHndl( IIAPI_HNDL *handle )
{
    IIAPI_ERRORHNDL   *errorHndl = NULL;
    
    if ( handle )
    {
	if ( handle->hd_errorQue == &handle->hd_errorList )
	    return( (IIAPI_ERRORHNDL *)NULL );

	errorHndl = (IIAPI_ERRORHNDL *)handle->hd_errorQue;
	handle->hd_errorQue = handle->hd_errorQue->q_next;
    }
    
    return( errorHndl );
}




/*
** Name: IIapi_getErrorHndl
**
** Description:
**	This function retrieves first error handle from the
**	error handle list.  If the error list is empty, parent
**	handles will also be searched.
**
**	handle    the handle containing the error handle list.
**
** Return value:
**     errorHndl      first error handle in the error list, NULL if
**                    no error handle is found.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	25-Apr-95 (gordy)
**	    Connection handles no longer obtainable from event handles.
**	 2-Oct-96 (gordy)
**	    Environment handles can now hold error handles.  Reversed
**	    previous change.
*/

II_EXTERN IIAPI_ERRORHNDL *
IIapi_getErrorHndl( IIAPI_HNDL *handle )
{
    IIAPI_ERRORHNDL   *errorHndl = NULL;
    
    while( handle )
    {
	if ( (errorHndl = getErrorHndl( handle )) )  
	{
	    IIAPI_TRACE( IIAPI_TR_VERBOSE )
		( "IIapi_getErrorHndl: found %p on %p\n", errorHndl, handle );
	    break;
	}

	if ( IIapi_isDbevHndl( (IIAPI_DBEVHNDL *)handle ) )
	    handle = (IIAPI_HNDL *)IIapi_getConnHndl( handle );
	else  if ( IIapi_isStmtHndl( (IIAPI_STMTHNDL *)handle ) )
	    handle = (IIAPI_HNDL *)IIapi_getTranHndl( handle );
	else  if ( IIapi_isTranHndl( (IIAPI_TRANHNDL *)handle ) )
	    handle = (IIAPI_HNDL *)IIapi_getConnHndl( handle );
	else  if ( IIapi_isConnHndl( (IIAPI_CONNHNDL *)handle ) )
	    handle = (IIAPI_HNDL *)IIapi_getEnvHndl( handle );
	else
	    break;
    }

    return( errorHndl );
}


/*
** Name: IIapi_cleanErrorHndl
**
** Description:
**     This function removes all error handles from the error handle list
**
**     handle   the handle containg the error handle list.
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
*/

II_EXTERN II_VOID
IIapi_cleanErrorHndl( IIAPI_HNDL *handle )
{
    IIAPI_ERRORHNDL	*errorHndl;
    II_BOOL		errors = FALSE;    

    for( errorHndl = (IIAPI_ERRORHNDL *)handle->hd_errorList.q_next;
	 errorHndl != (IIAPI_ERRORHNDL *)&handle->hd_errorList;
	 errorHndl = (IIAPI_ERRORHNDL *)handle->hd_errorList.q_next )
    {
	deleteErrorHndl( errorHndl );
	errors = TRUE;
    }

    handle->hd_errorQue = &handle->hd_errorList;

    if ( errors )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ("IIapi_cleanErrorHndl: removed error handles from %p\n", handle);
    }

    return;
}




/*
** Name: IIapi_clearAllErrors
**
** Description:
**	Calls IIapi_cleanErrorHndl() for the given handle,
**	and all handles accessible through the handle.
**
** History:
**	23-Jan-95 (gordy)
**	    Created.
**	25-Apr-95 (gordy)
**	    Connection handles no longer obtainable from event handles.
**	 2-Oct-96 (gordy)
**	    Environment handles can now hold error handles.  Reversed
**	    previous change.
*/

II_EXTERN II_VOID
IIapi_clearAllErrors( IIAPI_HNDL *handle )
{
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_clearAllErrors: removing error handles\n" );

    if ( IIapi_isDbevHndl( (IIAPI_DBEVHNDL *)handle ) )
    {
	IIapi_cleanErrorHndl( handle );
	handle = (IIAPI_HNDL *)IIapi_getConnHndl( handle );
    }

    if ( IIapi_isStmtHndl( (IIAPI_STMTHNDL *)handle ) )
    {
	IIapi_cleanErrorHndl( handle );
	handle = (IIAPI_HNDL *)IIapi_getTranHndl( handle );
    }

    if ( IIapi_isTranHndl( (IIAPI_TRANHNDL *)handle ) )
    {
	IIapi_cleanErrorHndl( handle );
	handle = (IIAPI_HNDL *)IIapi_getConnHndl( handle );
    }

    if ( IIapi_isConnHndl( (IIAPI_CONNHNDL *)handle ) )
    {
	IIapi_cleanErrorHndl( handle );
	handle = (IIAPI_HNDL *)IIapi_getEnvHndl( handle );
    }

    if ( IIapi_isEnvHndl( (IIAPI_ENVHNDL *)handle ) )
	IIapi_cleanErrorHndl( handle );

    return;
}




/*
** Name: errorStatus
**
** Description:
**
**     handle   the handle containg the error handle list.
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	23-Jan-95 (gordy)
**	    Created.
**	11-Jul-06 (gordy)
**	    Added XA error support.
*/

static IIAPI_STATUS
errorStatus( IIAPI_HNDL *handle )
{
    IIAPI_ERRORHNDL	*errorHndl;
    IIAPI_STATUS	r_stat = IIAPI_ST_SUCCESS;
    IIAPI_STATUS	status;
    
    for( errorHndl = (IIAPI_ERRORHNDL *)handle->hd_errorQue;
	 errorHndl != (IIAPI_ERRORHNDL *)&handle->hd_errorList;
	 errorHndl = (IIAPI_ERRORHNDL *)errorHndl->er_id.hi_queue.q_next )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "IIapi_errorStatus: found %p on %p\n", errorHndl, handle );

	switch( errorHndl->er_type )
	{
	    case IIAPI_GE_ERROR : 
	    case IIAPI_GE_XAERR :
		/*
		** This is a shortcut since we
		** know this is the worst status
		** we can return.
		*/
		return( IIAPI_ST_ERROR );

	    case IIAPI_GE_WARNING :	status = IIAPI_ST_WARNING; break;
	    case IIAPI_GE_MESSAGE :	status = IIAPI_ST_MESSAGE; break;
	    default :			status = IIAPI_ST_SUCCESS; break;
	}

	r_stat = IIAPI_WORST_STATUS( r_stat, status );
    }

    return( r_stat );
}




/*
** Name: IIapi_errorStatus
**
** Description:
**	Searches the error list of handle, and all
**	handles accessible through handle, and returns
**	the most critical error status found.
**
** History:
**	23-Jan-95 (gordy)
**	    Created.
**	25-Apr-95 (gordy)
**	    Connection handles no longer obtainable from event handles.
**	 2-Oct-96 (gordy)
**	    Environment handles can now hold error handles.
*/

II_EXTERN IIAPI_STATUS
IIapi_errorStatus( IIAPI_HNDL *handle )
{
    IIAPI_STATUS	r_stat = IIAPI_ST_SUCCESS; 
    IIAPI_STATUS	status;

    if ( IIapi_isDbevHndl( (IIAPI_DBEVHNDL *)handle ) )
    {
	status = errorStatus( handle );
	r_stat = IIAPI_WORST_STATUS( r_stat, status );
	handle = (IIAPI_HNDL *)IIapi_getConnHndl( handle );
    }

    if ( IIapi_isStmtHndl( (IIAPI_STMTHNDL *)handle ) )
    {
	status = errorStatus( handle );
	r_stat = IIAPI_WORST_STATUS( r_stat, status );
	handle = (IIAPI_HNDL *)IIapi_getTranHndl( handle );
    }

    if ( IIapi_isTranHndl( (IIAPI_TRANHNDL *)handle ) )
    {
	status = errorStatus( handle );
	r_stat = IIAPI_WORST_STATUS( r_stat, status );
	handle = (IIAPI_HNDL *)IIapi_getConnHndl( handle );
    }

    if ( IIapi_isConnHndl( (IIAPI_CONNHNDL *)handle ) )
    {
	status = errorStatus( handle );
	r_stat = IIAPI_WORST_STATUS( r_stat, status );
	handle = (IIAPI_HNDL *)IIapi_getEnvHndl( handle );
    }

    if ( IIapi_isEnvHndl( (IIAPI_ENVHNDL *)handle ) )
    {
	status = errorStatus( handle );
	r_stat = IIAPI_WORST_STATUS( r_stat, status );
    }

    return( r_stat );
}


/*
** Name: IIapi_saveErrors
**
** Description:
**	Saves active errors on a handle by moving them to the parent handle.
** 
** Input:
**	handle		Handle for which error handles are to be saved.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_HNDL *	New error handle, NULL if none.
**
** History:
**	 2-Oct-96 (gordy)
**	    Created.
**	25-Mar-10 (gordy)
**	    Rather than checking for an empty error list to determine
**	    that the current error Queue pointer should be set, check
**	    to see if the error Queue currently points at the head of
**	    the error list.  The former condition only occurs when no
**	    errors have been generated.  The latter will be true for
**	    the same condition, but will also be true when errors have
**	    been generated and retrieved by the application.  If errors
**	    are then subsequently added, the error Queue needs to be
**	    set to the new errors.  This latter condition can occur
**	    during batch processing.
*/

II_EXTERN IIAPI_HNDL *
IIapi_saveErrors( IIAPI_HNDL *handle )
{
    IIAPI_ERRORHNDL	*errorHndl;
    IIAPI_HNDL		*er_hndl = NULL;
    II_BOOL		moved = FALSE;

    if ( IIapi_isConnHndl( (IIAPI_CONNHNDL *)handle ) )
	er_hndl = (IIAPI_HNDL *)IIapi_getEnvHndl( handle );
    else  if ( IIapi_isTranHndl( (IIAPI_TRANHNDL *)handle ) )
	er_hndl = (IIAPI_HNDL *)IIapi_getConnHndl( handle );
    else  if ( IIapi_isStmtHndl( (IIAPI_STMTHNDL *)handle ) )
	er_hndl = (IIAPI_HNDL *)IIapi_getTranHndl( handle );
    else  if ( IIapi_isDbevHndl( (IIAPI_DBEVHNDL *)handle ) )
	er_hndl = (IIAPI_HNDL *)IIapi_getConnHndl( handle );

    if ( er_hndl )
	while( handle->hd_errorQue != &handle->hd_errorList )
	{
	    errorHndl = (IIAPI_ERRORHNDL *)handle->hd_errorQue;
	    handle->hd_errorQue = handle->hd_errorQue->q_next;
	    QUremove( (QUEUE *)errorHndl );

	    if ( er_hndl->hd_errorQue == &er_hndl->hd_errorList )
		er_hndl->hd_errorQue = (QUEUE *)errorHndl;

	    QUinsert( (QUEUE *)errorHndl, er_hndl->hd_errorList.q_prev );
	    moved = TRUE;
	}

    if ( moved )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ("IIapi_saveErrors: errors for %p saved on %p\n", handle, er_hndl);
    }

    return( er_hndl );
}
