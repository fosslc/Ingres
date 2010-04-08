/*
** Copyright (c) 2004, 2006 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apienhnd.h>
# include <apitname.h>
# include <apithndl.h>
# include <apitrace.h>

/*
** Name: apitname.c
**
** Description:
**	This file defines the transaction name handle management functions.
**
**      IIapi_createTranName   Create transaction name handle
**      IIapi_deleteTranName   Delete transaction name handle
**      IIapi_isTranName       is transaction name handle valid
**
** History:
**      30-sep-93 (ctham)
**          creation
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	17-Jan-96 (gordy)
**	    Added environment handles.  Added global data structure.
**	 2-Oct-96 (gordy)
**	    Added new tracing levels.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
*/


/*
** Name: IIapi_createTranName
**
** Description:
**	This function creates a transaction name handle.
**
**      tranID      Transaction ID to be registered.
**
** Return value:
**      tranName    transaction name handle if created successfully,
**                  NULL otherwise.
**
** Re-entrancy:
**	This function updates tranNameList which is shared by API users.
**
** History:
**      19-oct-93 (ctham)
**          creation
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	10-Jul-06 (gordy)
**	    Changed parameter to permit greater access.
*/

II_EXTERN IIAPI_TRANNAME*
IIapi_createTranName( IIAPI_TRAN_ID *tranID, IIAPI_ENVHNDL *envHndl )
{
    IIAPI_TRANNAME	*tranName;
    STATUS		status;
    
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_createTranName: create a transaction name handle, env = %p\n",
	  envHndl );

    if ( ! ( tranName = (IIAPI_TRANNAME *)
	     MEreqmem( 0, sizeof(IIAPI_TRANNAME), TRUE, &status ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createTranName: can't alloc transaction name handle\n" );
	return( NULL );
    }

    /*
    ** Initialize transaction name handle parameters, provide default values.
    */
    QUinit( &tranName->tn_id.hi_queue );
    tranName->tn_id.hi_hndlID = IIAPI_HI_TRANNAME;
    STRUCT_ASSIGN_MACRO( *tranID, tranName->tn_tranID );
    QUinit( &tranName->tn_tranHndlList );

    /* !!!!! FIX-ME: check for uniqueness of ID */

    MUp_semaphore( &envHndl->en_semaphore );
    tranName->tn_envHndl = envHndl;
    QUinsert( (QUEUE *)tranName, &envHndl->en_tranNameList );
    MUv_semaphore( &envHndl->en_semaphore );

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	("IIapi_createTranName: Transaction Name handle %p created\n",tranName);
    
    return( tranName );
}




/*
** Name: IIapi_deleteTranName
**
** Description:
**	This function deletes a transaction name handle.
**
**      tranName  transaction name handle to be deleted
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function update tranNameList which is shared by API users.
**
** History:
**      20-oct-93 (ctham)
**          creation
**	31-may-94 (ctham)
**	    clean up error handle interface.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	17-Jan-96 (gordy)
**	    Added environment handles.
*/

II_EXTERN II_VOID
IIapi_deleteTranName( IIAPI_TRANNAME *tranName )
{
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	("IIapi_deleteTranName: delete transaction name handle %p\n",tranName);

    MUp_semaphore( &tranName->tn_envHndl->en_semaphore );
    QUremove( (QUEUE *)tranName );
    MUv_semaphore( &tranName->tn_envHndl->en_semaphore );
    
    tranName->tn_id.hi_hndlID = ~IIAPI_HI_TRANNAME;
    MEfree( (II_PTR)tranName );

    return;
}




/*
** Name: IIapi_isTranName
**
** Description:
**     This function returns TRUE if the input is a valid transaction
**     name handle.
**
**     tranName  transaction name handle to be validated
**
** Return value:
**     status    TRUE if transaction name handle is valid, FALSE otherwise.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
*/

II_EXTERN II_BOOL
IIapi_isTranName( IIAPI_TRANNAME *tranName )
{
    return( tranName  &&  tranName->tn_id.hi_hndlID == IIAPI_HI_TRANNAME );
}
