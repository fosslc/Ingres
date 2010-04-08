/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <st.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apierhnd.h>
# include <apitrace.h>

/*
** Name: apishndl.c
**
** Description:
**	This file defines the statement handle management functions.
**
**      IIapi_createStmtHndl	Create statement handle.
**      IIapi_deleteStmtHndl	Delete statement handle.
**	IIapi_abortStmtHndl	Abort statement handle.
**      IIapi_isStmtHndl	Is statement handle valid?
**      IIapi_getStmtHndl	Get statement handle.
**
** History:
**      30-sep-93 (ctham)
**          creation
**      11-apr-94 (ctham)
**          fix IIapi_getStmtHndl() to deal with empty transaction and
**          statement handle queue.
**      12-apr-94 (ctham)
**          fix IIapi_getStmtHndl() to deal with incorrect stmtHndl.
**      20-apr-94 (ctham)
**          Must check for null before freeing cursor handle.
**          IIapi_deleteStmtHndl() is updated.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**      31-may-94 (ctham)
**          use errorQue as the current error handle reference.
**	     added hd_memTag to handle header for memory allocation.
**	26-Oct-94 (gordy)
**	    General cleanup and minor fixes.
**	31-Oct-94 (gordy)
**	    Cleaned up cursor processing.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	11-Jan-95 (gordy)
**	    Cleaning up descriptor handling.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	13-Jun-95 (gordy)
**	    Added support for buffering of GCA messages.
**	20-Jun-95 (gordy)
**	    Test cursor flag rather than handle since CURSOR UPDATE
**	    may also use the cursor handle.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	23-Feb-96 (gordy)
**	    Reworked copy info structures.
**	24-May-96 (gordy)
**	    Removed hd_memTag, replaced by new tagged memory support.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	31-Jan-97 (gordy)
**	    Delete het-net descriptor in statement handle (if present).
**	 3-Sep-98 (gordy)
**	    Added IIapi_abortStmtHndl().
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**      28-jun-2002 (loera01) Bug 108147
**          Apply mutex protection to statement handle.
**      28-jun-2002 (loera01) Bug 108148
**          Added check for stmtHndl->sh_cancelled in IIapi_abortStmtHndl()
**	25-Oct-06 (gordy)
**	    Save query flags as statement flags.
**	11-Aug-08 (gordy)
**	    Added additional column info.
**	25-Mar-10 (gordy)
**	    Add GCA packed byte stream message buffer and queue.
*/




/*
** Name: IIapi_createStmtHndl
**
** Description:
**	This function creates a statement handle.
**
** Return value:
**      stmtHndl   statement handle if created successfully, NULL
**                 otherwise.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      19-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**      31-may-94 (ctham)
**          use errorQue as the current error handle reference.
**	     added hd_memTag to handle header for memory allocation.
**	31-Oct-94 (gordy)
**	    Cleaned up cursor processing.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	24-May-96 (gordy)
**	    Removed hd_memTag, replaced by new tagged memory support.
**	25-Oct-06 (gordy)
**	    Query flags added at IIAPI_VERSION_6.
**	25-Mar-10 (gordy)
**	    Add GCA packed byte stream message buffer and queue.
**	    Moved query specific processing to interface functions.
*/

II_EXTERN IIAPI_STMTHNDL *
IIapi_createStmtHndl( IIAPI_CONNHNDL *connHndl, IIAPI_TRANHNDL *tranHndl )
{
    IIAPI_STMTHNDL	*stmtHndl;
    STATUS		status;
    
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_createStmtHndl: create a statement handle\n" );
    
    if ( ! (stmtHndl = (IIAPI_STMTHNDL *)
		       MEreqmem(0, sizeof(IIAPI_STMTHNDL), TRUE, &status)) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createStmtHndl: can't alloc statement handle\n" );
	return( NULL );
    }
    
    /*
    ** Initialize statement handle parameters, provide default values.
    */
    stmtHndl->sh_header.hd_id.hi_hndlID = IIAPI_HI_STMT_HNDL;
    stmtHndl->sh_header.hd_delete = FALSE;
    stmtHndl->sh_header.hd_smi.smi_state = IIAPI_IDLE;
    stmtHndl->sh_header.hd_smi.smi_sm =
			IIapi_sm[ connHndl->ch_type ][ IIAPI_SMH_STMT ];

    if ( MUi_semaphore( &stmtHndl->sh_header.hd_sem ) != OK )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createStmtHndl: can't create semaphore\n" );
	MEfree( (II_PTR)stmtHndl );
	return( NULL );
    }

    stmtHndl->sh_tranHndl = tranHndl;
    stmtHndl->sh_callback = FALSE;
    stmtHndl->sh_cancelled = FALSE;
   
    QUinit( &stmtHndl->sh_sndQueue );
    QUinit( (QUEUE *)&stmtHndl->sh_header.hd_id.hi_queue );
    QUinit( (QUEUE *)&stmtHndl->sh_header.hd_errorList );
    stmtHndl->sh_header.hd_errorQue = &stmtHndl->sh_header.hd_errorList;
    
    QUinsert( (QUEUE *)stmtHndl, &stmtHndl->sh_tranHndl->th_stmtHndlList );
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_createStmtHndl: stmtHndl %p created\n", stmtHndl );
    
    return( stmtHndl );
}




/*
** Name: IIapi_deleteStmtHndl
**
** Description:
**	This function deletes a statement handle.
**
**      stmtHndl  statement handle to be deleted
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
**      20-apr-94 (ctham)
**          Must check for null before freeing cursor handle.
**          IIapi_deleteStmtHndl() is updated.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	26-Oct-94 (gordy)
**	    Always reset handle ID for deleted handles.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	11-Jan-95 (gordy)
**	    Cleaning up descriptor handling.
**	13-Jun-95 (gordy)
**	    Added support for buffering of GCA messages.
**	20-Jun-95 (gordy)
**	    Test cursor flag rather than handle since CURSOR UPDATE
**	    may also use the cursor handle.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	23-Feb-96 (gordy)
**	    Reworked copy info structures.
**	31-Jan-97 (gordy)
**	    Delete het-net descriptor in statement handle (if present).
**       9-Aug-99 (rajus01)
**          Check hd_delete flag before removing the handle from
**          stmtHndl queue. ( Bug #98303 )
**	11-Aug-08 (gordy)
**	    Free column info.
**	25-Mar-10 (gordy)
**	    Add GCA packed byte stream message buffer and queue.
*/

II_EXTERN II_VOID
IIapi_deleteStmtHndl( IIAPI_STMTHNDL *stmtHndl )
{
    QUEUE	*que;
    II_LONG	i;

    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_deleteStmtHndl: delete a statement handle %p\n", stmtHndl );
    
    if( ! stmtHndl->sh_header.hd_delete )
        QUremove( (QUEUE *)stmtHndl );

    if ( stmtHndl->sh_flags & IIAPI_SH_CURSOR )  
	IIapi_deleteIdHndl(stmtHndl->sh_cursorHndl);

    if ( stmtHndl->sh_queryText )  
	MEfree( (II_PTR)stmtHndl->sh_queryText );

    if ( stmtHndl->sh_copyMap.cp_fileName )  
	MEfree((II_PTR)stmtHndl->sh_copyMap.cp_fileName);

    if ( stmtHndl->sh_copyMap.cp_logName )  
	MEfree( (II_PTR)stmtHndl->sh_copyMap.cp_logName );

    if ( stmtHndl->sh_cpyDescriptor )
	MEfree( stmtHndl->sh_cpyDescriptor );

    if ( stmtHndl->sh_parmDescriptor )
    {
	for( i = 0; i < stmtHndl->sh_parmCount; i++ )
	    if ( stmtHndl->sh_parmDescriptor[i].ds_columnName )
		MEfree( (II_PTR)stmtHndl->sh_parmDescriptor[i].ds_columnName );
	
	MEfree( (II_PTR)stmtHndl->sh_parmDescriptor );
    }
    
    if ( stmtHndl->sh_colDescriptor )
    {
	/*
	** Note: this also frees sh_copyMap.cp_dbmsDescr.
	*/
	for( i = 0; i < stmtHndl->sh_colCount; i++ )
	    if ( stmtHndl->sh_colDescriptor[i].ds_columnName )
		MEfree( (II_PTR)stmtHndl->sh_colDescriptor[i].ds_columnName );
	
	MEfree( (II_PTR)stmtHndl->sh_colDescriptor );
    }
    
    if ( stmtHndl->sh_colInfo )
	MEfree( (II_PTR)stmtHndl->sh_colInfo );

    if ( stmtHndl->sh_copyMap.cp_fileDescr )
    {
	for( i = 0; i < stmtHndl->sh_copyMap.cp_fileCount; i++ )
	{
	    if ( stmtHndl->sh_copyMap.cp_fileDescr[i].fd_name )
		MEfree( (PTR)stmtHndl->sh_copyMap.cp_fileDescr[i].fd_name );

	    if ( stmtHndl->sh_copyMap.cp_fileDescr[i].fd_delimValue )
		MEfree((PTR)stmtHndl->sh_copyMap.cp_fileDescr[i].fd_delimValue);

	    if ( stmtHndl->sh_copyMap.cp_fileDescr[i].fd_nullValue.dv_value )
		MEfree( (PTR)stmtHndl->sh_copyMap.cp_fileDescr[i]
						    .fd_nullValue.dv_value );
	}
	
	MEfree( (II_PTR)stmtHndl->sh_copyMap.cp_fileDescr );
    }
    
    while( (que = QUremove( stmtHndl->sh_sndQueue.q_next )) )
	IIapi_freeMsgBuffer( (IIAPI_MSG_BUFF *)que );

    if ( stmtHndl->sh_sndBuff )  IIapi_freeMsgBuffer( stmtHndl->sh_sndBuff );
    if ( stmtHndl->sh_rcvBuff )  IIapi_freeMsgBuffer( stmtHndl->sh_rcvBuff);
    IIapi_cleanErrorHndl( &stmtHndl->sh_header );

    MUr_semaphore( &stmtHndl->sh_header.hd_sem );
    stmtHndl->sh_header.hd_id.hi_hndlID = ~IIAPI_HI_STMT_HNDL;
    MEfree( (II_PTR)stmtHndl );

    return;
}



/*
** Name: IIapi_abortStmtHndl
**
** Description:
**	Aborts any operation active on the provided statement handle.
**
** Input:
**	stmtHndl	Statement handle.
**	errorCode	Error code.
**	SQLState	SQLSTATE for error.
**	status		API status of error condition.
**
** Output:
**	None.
**
** Returns:
**	Void.
**
** History:
**	 3-Sep-98 (gordy)
**	    Created.
*/

II_EXTERN II_VOID
IIapi_abortStmtHndl
( 
    IIAPI_STMTHNDL	*stmtHndl, 
    II_LONG		errorCode, 
    char		*SQLState,
    IIAPI_STATUS	status
)
{
    /*
    ** Place statement handle in the abort state.
    */
    stmtHndl->sh_header.hd_smi.smi_state = 
			stmtHndl->sh_header.hd_smi.smi_sm->sm_abort_state;

    if ( stmtHndl->sh_flags & IIAPI_SH_CURSOR )
    {
	/*
	** Aborted cursors do not need to be closed
	** (they are closed automatically by the abort)
	** so delete the cursor handle.
	*/
	IIapi_deleteIdHndl( stmtHndl->sh_cursorHndl );
	stmtHndl->sh_flags &= ~IIAPI_SH_CURSOR;
    }

    /*
    ** Abort current operation (if any).
    */
    if ( stmtHndl->sh_callback )
    {
	if ( ! IIapi_localError( (IIAPI_HNDL *)stmtHndl, 
				 errorCode, SQLState, status ) )
	    status = IIAPI_ST_OUT_OF_MEMORY;

	IIapi_appCallback( stmtHndl->sh_parm, (IIAPI_HNDL *)stmtHndl, status );
	stmtHndl->sh_callback = FALSE;
    }
    if ( stmtHndl->sh_cancelled )
    {
        if ( ! IIapi_localError( (IIAPI_HNDL *)stmtHndl,
                                 errorCode, SQLState, status ) )
            status = IIAPI_ST_OUT_OF_MEMORY;

        IIapi_appCallback( stmtHndl->sh_cancelParm, (IIAPI_HNDL *)stmtHndl, status );
        stmtHndl->sh_cancelled = FALSE;
    }

    return;
}



/*
** Name: IIapi_isStmtHndl
**
** Description:
**     This function returns TRUE if the input is a valid statement
**     handle.
**
**     stmtHndl  statement handle to be validated
**
** Return value:
**     status    TRUE is the statement handle is valid, FALSE otherwise.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	26-Oct-94 (gordy)
**	    Removed tracing: caller can decide if trace is needed.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
*/

II_EXTERN II_BOOL
IIapi_isStmtHndl( IIAPI_STMTHNDL *stmtHndl )
{
    return( stmtHndl  &&  
	    stmtHndl->sh_header.hd_id.hi_hndlID == IIAPI_HI_STMT_HNDL );
}




/*
** Name: IIapi_getStmtHndl
**
** Description:
**	This function returns the statement handle in the generic handle
**      parameter.
**
** Return value:
**      stmtHndl = statement handle
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      11-apr-94 (ctham)
**          fix IIapi_getStmtHndl() to deal with empty transaction and
**          statement handle queue.
**      12-apr-94 (ctham)
**          fix IIapi_getStmtHndl() to deal with incorrect stmtHndl.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	26-Oct-94 (gordy)
**	    Allow for NULL handles.  Since multiple statement
**	    handles can be active in a transaction/connection,
**	    don't just return the first handle.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
*/

II_EXTERN IIAPI_STMTHNDL *
IIapi_getStmtHndl( IIAPI_HNDL *handle )
{
    if ( handle )
    {
	switch( handle->hd_id.hi_hndlID )
	{
	    case IIAPI_HI_STMT_HNDL:
		break;

	    default:
		handle = NULL;
		break;
	}
    }
    
    return( (IIAPI_STMTHNDL *)handle );
}
