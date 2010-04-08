/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apichndl.h>
# include <apiihndl.h>
# include <apitrace.h>

/*
** Name: apiihndl.c
**
** Description:
**	This file defines the GCA identifier handle management functions:
**
**	IIapi_createIdHndl	Add ID handle
**	IIapi_deleteIdHndl	Delete ID handle
**	IIapi_isIdHndl		Verify ID handle
**
** History:
**	20-Dec-94 (gordy)
**	    Created.
**	16-Feb-95 (gordy)
**	    Added unique ID functions.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	14-Sep-95 (gordy)
**	    GCD_GCA_ prefix changed to GCD_.
**	17-Jan-96 (gordy)
**	    Added environment handles.  
**	 2-Oct-96 (gordy)
**	    Added new tracing levels.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/


/*
** Name: IIapi_createIdHndl
**
** Description:
**	This function appends a GCA identifier handle for
**	cursors, procedures and repeat queries.
**
** Return value:
**	ptr	Address of handle, NULL if failure.
**
** History:
**	20-Dec-94 (gordy)
**	    Created.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

II_EXTERN IIAPI_IDHNDL *
IIapi_createIdHndl( II_LONG type, GCA_ID *gcaID )
{
    IIAPI_IDHNDL	*handle;
    STATUS		status;
    
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_createIdHndl: create GCA identifier handle\n" );

    if ( ! ( handle = (IIAPI_IDHNDL *)
	     MEreqmem( 0, sizeof(IIAPI_IDHNDL), FALSE, &status ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_createIdHndl: can't allocate handle\n" );
	return( NULL );
    }

    QUinit( &handle->id_id.hi_queue );
    handle->id_id.hi_hndlID = IIAPI_HI_IDENTIFIER;
    handle->id_type = type;
    MEcopy( (II_PTR)gcaID, sizeof( GCA_ID ), (II_PTR)&handle->id_gca_id );

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	("IIapi_createIdHndl: created GCA identifier handle %p type %d\n",
	 handle, handle->id_type );

    return( handle );
}




/*
** Name: IIapi_deleteIdHndl
**
** Description:
**	This function deletes a GCA identifier handle.
**
** Return value:
**      none.
**
** History:
**	20-Dec-94 (gordy)
**	    Created.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
*/

II_EXTERN II_VOID
IIapi_deleteIdHndl( IIAPI_IDHNDL *idHndl )
{
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	("IIapi_deleteIdHndl: delete GCA identifier handle %p\n", idHndl);

    idHndl->id_id.hi_hndlID = ~IIAPI_HI_IDENTIFIER;
    MEfree( (II_PTR)idHndl );

    return;
}



/*
** Name: IIapi_isIdHndl
**
** Description:
**     This function returns TRUE if the input is a valid 
**     GCA identifier handle of the desired type.
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
**	20-Dec-94 (gordy)
**	    Created.
*/

II_EXTERN II_BOOL
IIapi_isIdHndl( IIAPI_IDHNDL *idHndl, II_LONG type )
{
    return( idHndl  &&  idHndl->id_id.hi_hndlID == IIAPI_HI_IDENTIFIER  &&
	    ( type <= 0  ||  idHndl->id_type == type ) );
}


