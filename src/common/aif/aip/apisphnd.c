/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <st.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apithndl.h>
# include <apisphnd.h>
# include <apitrace.h>

/*
** Name: apisphnd.c
**
** Description:
**	This file defines the save point handle management functions.
**
**      IIapi_createSavePtHndl create an savePoint handle
**      IIapi_deleteSavePtHndl delete an savePoint handle
**      IIapi_isSavePtHndl     is savePoint handle 
**
** History:
**      23-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 9-Mar-95 (gordy)
**	    Silenced compiler warnings.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 2-Oct-96 (gordy)
**	    Added new tracing levels.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
*/




/*
** Name: IIapi_createSavePtHndl
**
** Description:
**	This function creates a save point handle.
**
**      savePtParm argument of IIapi_savePoint()
**
** Return value:
**      status     TRUE if save point handle is created successfully,
**                 FALSE otherwise.
**
** Re-entrancy:
**	This function updates the save point handle list within
**      a transaction.
**
** History:
**      23-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
*/

II_EXTERN II_BOOL
IIapi_createSavePtHndl( IIAPI_SAVEPTPARM *savePtParm )
{
    IIAPI_SAVEPTHNDL	*savePtHndl;
    IIAPI_TRANHNDL	*tranHndl = (IIAPI_TRANHNDL *)savePtParm->sp_tranHandle;
    STATUS		status;
    
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_createSavePtHndl: create a save point handle\n" );
    
    if ( ! ( savePtHndl = (IIAPI_SAVEPTHNDL *)
	     MEreqmem( 0, sizeof(IIAPI_SAVEPTHNDL), FALSE, &status ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createSavePtHndl: can't allocate save point handle\n" );
	return( FALSE );
    }
    
    savePtHndl->sp_id.hi_hndlID = IIAPI_HI_SAVEPOINT;
    
    if ( ! (savePtHndl->sp_savePtName = STalloc( savePtParm->sp_savePoint )) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createSavePtHndl: can't allocate save point name\n" );
	MEfree( (II_PTR)savePtHndl );
	return( FALSE );
    }
    
    QUinit( &savePtHndl->sp_id.hi_queue );
    QUinsert( &savePtHndl->sp_id.hi_queue, &tranHndl->th_savePtHndlList );
    
    savePtParm->sp_savePointHandle = (II_PTR)savePtHndl;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	("IIapi_createSavePtHndl: savePtHndl = %p for tranHndl = %p\n",
	 savePtHndl, tranHndl );

    return( TRUE );
}




/*
** Name: IIapi_deleteSavePtHndl
**
** Description:
**	This function deletes a save point handle.
**
**      savePtHndl  save point handle to be deleted
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
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
*/

II_EXTERN II_VOID
IIapi_deleteSavePtHndl( IIAPI_SAVEPTHNDL *savePtHndl )
{
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_deleteSavePtHndl: delete save point handle %p\n", 
	  savePtHndl );
    
    QUremove( &savePtHndl->sp_id.hi_queue );
    savePtHndl->sp_id.hi_hndlID = ~IIAPI_HI_SAVEPOINT;
    MEfree( (II_PTR)savePtHndl->sp_savePtName );
    MEfree( (II_PTR)savePtHndl );

    return;
}




/*
** Name: IIapi_isSavePtHndl
**
** Description:
**     This function validates the input save point handle.
**
**     savePtHndl  save point handle
**
** Return value:
**     status      TRUE is the save point handle is valid, FALSE
**                 otherwise.
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
**	    Fixing queue processing.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
*/

II_EXTERN II_BOOL
IIapi_isSavePtHndl( IIAPI_SAVEPTHNDL *savePtHndl, IIAPI_TRANHNDL *tranHndl )
{
    IIAPI_SAVEPTHNDL  *sp;

    if ( savePtHndl  &&  savePtHndl->sp_id.hi_hndlID == IIAPI_HI_SAVEPOINT )
    { 
	for( 
	      sp = (IIAPI_SAVEPTHNDL *)tranHndl->th_savePtHndlList.q_next;
	      sp != (IIAPI_SAVEPTHNDL *)&tranHndl->th_savePtHndlList;
	      sp = (IIAPI_SAVEPTHNDL *)sp->sp_id.hi_queue.q_next
	   )
	{
	    if ( savePtHndl == sp )  return( TRUE );
	}
    }

    return( FALSE );
}

