/*
** Copyright (c) 2004, 2006 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apitname.h>
# include <apisphnd.h>
# include <apierhnd.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apithndl.c
**
** Description:
**	This file defines the transaction handle management functions.
**
**      IIapi_createTranHndl	Create transaction handle.
**      IIapi_deleteTranHndl	Delete transaction handle.
**	IIapi_findTranHndl	Find transaction handle.
**	Iiapi_abortTranHndl	Abort transaction handle.
**      IIapi_isTranHndl	Is transaction handle valid?
**      IIapi_getTranHndl	Get transaction handle.
**
** History:
**      30-sep-93 (ctham)
**          creation
**      11-apr-94 (ctham)
**          fix IIapi_getTranHndl() to deal with empty transaction handle
**          queue.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**      31-may-94 (ctham)
**          use errorQue as the current error handle reference.
**	     added hd_memTag to handle header for memory allocation.
**	21-Oct-94 (gordy)
**	    Fix savepoint processing.
**	31-Oct-94 (gordy)
**	    Removed unused member th_end.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	14-Jun-95 (gordy)
**	    Distributed transaction handles must be maintained
**	    on the associated transaction name queue.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	24-May-96 (gordy)
**	    Removed hd_memTag, replaced by new tagged memory support.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	13-Mar-97 (gordy)
**	    Delete all related statement and savepoint handles
**	    when deleting a transaction handle.
**	 3-Sep-98 (gordy)
**	    Added IIapi_abortTranHndl().
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	21-Mar-01 (gordy)
**	    Disassociate transaction from distributed transaction
**	    name prior to making callback.
**      28-jun-2002 (loera01) Bug 108147
**          Apply mutex protection to transaction handle.
**	11-Jul-06 (gordy)
**	    Added IIapi_findTranHndl().
*/




/*
** Name: IIapi_createTranHndl
**
** Description:
**	This function creates a transaction handle.
**
**      tranName   transaction name
**      connHndl   connection handle
**
** Return value:
**      tranHndl   transaction handle if created successfully, NULL
**                 otherwise.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      19-oct-93 (ctham)
**          creation
**      31-may-94 (ctham)
**          use errorQue as the current error handle reference.
**	     added hd_memTag to handle header for memory allocation.
**	21-Oct-94 (gordy)
**	    Removed unused member th_savePtHndl.
**	31-Oct-94 (gordy)
**	    Removed unused member th_end.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	14-Jun-95 (gordy)
**	    Place transaction handle on transaction name queue.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	24-May-96 (gordy)
**	    Removed hd_memTag, replaced by new tagged memory support.
*/

II_EXTERN IIAPI_TRANHNDL*
IIapi_createTranHndl 
(
    IIAPI_TRANNAME	*tranName,
    IIAPI_CONNHNDL	*connHndl
)
{
    IIAPI_TRANHNDL	*tranHndl;
    STATUS		status;
    
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_createTranHndl: create a transaction handle\n" );
    
    if ( ( tranHndl = (IIAPI_TRANHNDL *)
	   MEreqmem( 0, sizeof(IIAPI_TRANHNDL), TRUE, &status ) ) == NULL )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createTranHndl: can't alloc transaction handle\n" );
	return( NULL );
    }
    
    /*
    ** Initialize transaction handle parameters, provide default values.
    */
    tranHndl->th_header.hd_id.hi_hndlID = IIAPI_HI_TRAN_HNDL;
    tranHndl->th_header.hd_delete = FALSE;
    tranHndl->th_header.hd_smi.smi_state = IIAPI_IDLE;
    tranHndl->th_header.hd_smi.smi_sm =
			IIapi_sm[ connHndl->ch_type ][ IIAPI_SMH_TRAN ];

    if ( MUi_semaphore( &tranHndl->th_header.hd_sem ) != OK )
    {
        IIAPI_TRACE( IIAPI_TR_FATAL )
            ( "IIapi_createTranHndl: can't create semaphore\n" );
        MEfree( (PTR)tranHndl );
        return (NULL);
    }
    tranHndl->th_connHndl = connHndl;
    tranHndl->th_callback = FALSE;

    QUinit( (QUEUE *)&tranHndl->th_tranNameQue );
    QUinit( (QUEUE *)&tranHndl->th_stmtHndlList );
    QUinit( (QUEUE *)&tranHndl->th_savePtHndlList );
    QUinit( (QUEUE *)&tranHndl->th_header.hd_id.hi_queue );
    QUinit( (QUEUE *)&tranHndl->th_header.hd_errorList );
    tranHndl->th_header.hd_errorQue = &tranHndl->th_header.hd_errorList;
    
    QUinsert( (QUEUE *)tranHndl, &tranHndl->th_connHndl->ch_tranHndlList );
    
    if ( tranName )
    {
	tranHndl->th_tranName = tranName;
	QUinsert( &tranHndl->th_tranNameQue, &tranName->tn_tranHndlList );
    }

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_createTranHndl: tranHndl %p created\n", tranHndl );
    
    return( tranHndl );
}



/*
** Name: IIapi_deleteTranHndl
**
** Description:
**	This function deletes a transaction handle.
**
**      tranHndl  transaction handle to be deleted
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
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	26-Oct-94 (gordy)
**	    Always reset handle ID for deleted handles.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	14-Jun-95 (gordy)
**	    Remove transaction handle from transaction name queue.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	13-Mar-97 (gordy)
**	    Delete all statement and savepoint handles related to
**	    the transaction handle.
**       9-Aug-99 (rajus01)
**          Check hd_delete flag before removing the handle from
**          tranHndl queue. ( Bug #98303 )
*/

II_EXTERN II_VOID
IIapi_deleteTranHndl( IIAPI_TRANHNDL *tranHndl )
{
    QUEUE *q;

    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_deleteTranHndl: delete a transaction handle %p\n", 
	  tranHndl );

    if( ! tranHndl->th_header.hd_delete )
        QUremove( (QUEUE *)tranHndl );

    if ( tranHndl->th_tranName )  QUremove( &tranHndl->th_tranNameQue );

    for(
         q = tranHndl->th_stmtHndlList.q_next;
	 q != &tranHndl->th_stmtHndlList;
	 q = tranHndl->th_stmtHndlList.q_next
       )
	IIapi_deleteStmtHndl( (IIAPI_STMTHNDL *)q );

    for(
         q = tranHndl->th_savePtHndlList.q_next;
	 q != &tranHndl->th_savePtHndlList;
	 q = tranHndl->th_savePtHndlList.q_next
       )
	IIapi_deleteSavePtHndl( (IIAPI_SAVEPTHNDL *)q );

    IIapi_cleanErrorHndl( &tranHndl->th_header );

    MUr_semaphore( &tranHndl->th_header.hd_sem ); 
    tranHndl->th_header.hd_id.hi_hndlID = ~IIAPI_HI_TRAN_HNDL;
    MEfree( (II_PTR)tranHndl );

    return;
}



/*
** Name: IIapi_findTranHndl
**
** Description:
**	Search for an active transaction handle associated with a
**	particular transaction ID.  NULL is returned if no match
**	is found.
**
** Input:
**	connHndl		Connection handle.
**	tranID			Transaction ID.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_TRANHNDL *	Transaction handle, or NULL.
**
** History:
**	11-Jun-06 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_TRANHNDL *
IIapi_findTranHndl( IIAPI_CONNHNDL *connHndl, IIAPI_TRAN_ID *tranID )
{
    QUEUE	*q;

    for(
         q = connHndl->ch_tranHndlList.q_next;
	 q != &connHndl->ch_tranHndlList;
	 q = q->q_next
       )
    {
        IIAPI_TRANHNDL *tranHandle = (IIAPI_TRANHNDL *)q;

	if ( ! tranHandle->th_tranName  ||
	     tranHandle->th_tranName->tn_tranID.ti_type != tranID->ti_type ) 
	    continue;

	switch( tranID->ti_type )
	{
	case IIAPI_TI_IIXID :
	{
	    IIAPI_II_TRAN_ID *iiTranID1 = &tranID->ti_value.iiXID.ii_tranID;
	    IIAPI_II_TRAN_ID *iiTranID2 = 
		&tranHandle->th_tranName->tn_tranID.ti_value.iiXID.ii_tranID;

	    if ( 
	         iiTranID1->it_highTran == iiTranID2->it_highTran  &&
		 iiTranID1->it_lowTran == iiTranID2->it_lowTran
	       )
	    	return( tranHandle );
	    break;
	}
	case IIAPI_TI_XAXID :
	{
	    IIAPI_XA_TRAN_ID *xaTranID1 = &tranID->ti_value.xaXID.xa_tranID;
	    IIAPI_XA_TRAN_ID *xaTranID2 =
	    	&tranHandle->th_tranName->tn_tranID.ti_value.xaXID.xa_tranID;

	    if (
	         xaTranID1->xt_formatID == xaTranID2->xt_formatID  &&
		 xaTranID1->xt_gtridLength == xaTranID2->xt_gtridLength  &&
		 xaTranID1->xt_bqualLength == xaTranID2->xt_bqualLength  &&
		 ! MEcmp( (PTR)xaTranID1->xt_data, (PTR)xaTranID2->xt_data, 
		          xaTranID1->xt_gtridLength+xaTranID1->xt_bqualLength )
	       )
		return( tranHandle );
	    break;
	}
	}
    }

    return( NULL );
}



/*
** Name: IIapi_abortTranHndl
**
** Description:
**	Aborts any operation active on the provided transaction
**	handle and associated statement handles.
**
** Input:
**	tranHndl	Transaction handle.
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
**	21-Mar-01 (gordy)
**	    Disassociate transaction from distributed transaction
**	    name prior to making callback.
*/

II_EXTERN II_VOID
IIapi_abortTranHndl
( 
    IIAPI_TRANHNDL	*tranHndl,
    II_LONG		errorCode, 
    char		*SQLState,
    IIAPI_STATUS	status
)
{
    IIAPI_STMTHNDL	*stmtHndl;

    /*
    ** Place the transaction handle in the abort state.
    */
    tranHndl->th_header.hd_smi.smi_state =
			tranHndl->th_header.hd_smi.smi_sm->sm_abort_state;

    /*
    ** Abort associated statement handles.
    */
    for(
	 stmtHndl = (IIAPI_STMTHNDL *)tranHndl->th_stmtHndlList.q_next;
	 stmtHndl != (IIAPI_STMTHNDL *)&tranHndl->th_stmtHndlList;
	 stmtHndl = (IIAPI_STMTHNDL *)stmtHndl->sh_header.hd_id.hi_queue.q_next
       )
	IIapi_abortStmtHndl( stmtHndl, errorCode, SQLState, status );

    /*
    ** If this was a distributed transaction, 
    ** the associated distributed transaction 
    ** name should be releasable during the 
    ** callback.  Since the transaction handle 
    ** has not actually been deleted, it is 
    ** still associated with the distributed 
    ** transaction name.  Normally, cases such
    ** as this are serialized by the dispatch
    ** operations queue, but IIapi_releaseXID()
    ** is not a dispatched request.  So we need
    ** to drop the association prior to making
    ** the callback.
    */
    if ( tranHndl->th_tranName )  
    {
	QUremove( &tranHndl->th_tranNameQue );
	tranHndl->th_tranName = NULL;
    }

    /*
    ** Abort current operation (if any).
    */
    if ( tranHndl->th_callback )
    {
	if ( ! IIapi_localError( (IIAPI_HNDL *)tranHndl, 
				 errorCode, SQLState, status ) )
	    status = IIAPI_ST_OUT_OF_MEMORY;

	IIapi_appCallback( tranHndl->th_parm, (IIAPI_HNDL *)tranHndl, status );
	tranHndl->th_callback = FALSE;
    }

    return;
}



/*
** Name: IIapi_isTranHndl
**
** Description:
**     This function returns TRUE if the input is a valid transaction
**     handle.
**
**     tranHndl  transaction handle to be validated
**
** Return value:
**     status    TRUE if transaction handle is valid, FALSE otherwise.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**	26-Oct-94 (gordy)
**	    Remove tracing: caller can decide if tracing is needed.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
*/

II_EXTERN II_BOOL
IIapi_isTranHndl( IIAPI_TRANHNDL *tranHndl )
{
    return( tranHndl  && 
	    tranHndl->th_header.hd_id.hi_hndlID == IIAPI_HI_TRAN_HNDL );
}




/*
** Name: IIapi_getTranHndl
**
** Description:
**	This function returns the transaction handle in the generic handle
**      parameter.
**
** Return value:
**      tranHndl = transaction handle
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      11-apr-94 (ctham)
**          fix IIapi_getTranHndl() to deal with empty transaction handle
**          queue.
**	26-Oct-94 (gordy)
**	    Allow for NULL handles.  Can't get a transaction handle
**	    from a connection handle.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
*/

II_EXTERN IIAPI_TRANHNDL*
IIapi_getTranHndl( IIAPI_HNDL *handle )
{
    if ( handle )
    {
	switch (handle->hd_id.hi_hndlID)
	{
	    case IIAPI_HI_TRAN_HNDL:
		break;

	    case IIAPI_HI_STMT_HNDL:
		handle = (IIAPI_HNDL *)
			 ((IIAPI_STMTHNDL *)handle)->sh_tranHndl;
		break;

	    default:
		handle = NULL;
		break;
	}
    }
    
    return( (IIAPI_TRANHNDL *)handle );
}
