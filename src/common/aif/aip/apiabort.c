/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apiehndl.h>
# include <apierhnd.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apiabort.c
**
** Description:
**	This file contains the IIapi_abort() function interface.
**
**      IIapi_abort		API abort function
**
** History:
**	09-Aug-98 (rajus01)
**	    Created.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

/*
** Name: IIapi_abort
**
** Description:
**	This function provide an interface for the frontend application
**	to provide flexibility to abort the connection to the server.
**	Please refer to the API user specification for function description.
**
**	abortParm	input and output parameters for IIapi_abort()
**
** Return value:
**	none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	09-Aug-98 (rajus01)
**	    Created.
**      31-May-05 (gordy)
**          Don't permit new requests on handles marked for deletion.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_abort( IIAPI_ABORTPARM II_FAR *abortParm )
{
    IIAPI_CONNHNDL	*connHndl;

    /*
    ** Validate Input parameters
    */
    if ( ! abortParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
  		   ( "IIapi_abort: null function parameters\n" );
	return;
    }

    abortParm->ab_genParm.gp_completed = FALSE;
    abortParm->ab_genParm.gp_status = IIAPI_ST_SUCCESS;
    abortParm->ab_genParm.gp_errorHandle = NULL;
    connHndl = (IIAPI_CONNHNDL *)abortParm->ab_connHandle;

    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_abort: API is not initialized\n" );
	IIapi_appCallback( &abortParm->ab_genParm, NULL,
				   IIAPI_ST_NOT_INITIALIZED );
	return;
    }

    if ( ! IIapi_isConnHndl( connHndl )  ||  IIAPI_STALE_HANDLE( connHndl ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_abort: invalid connection handle\n" );
	IIapi_appCallback( &abortParm->ab_genParm, NULL,
				   IIAPI_ST_INVALID_HANDLE );
	return;
    }

    IIAPI_TRACE( IIAPI_TR_INFO )
	    ( "IIapi_abort: connHndl = %p\n", connHndl);

    IIapi_clearAllErrors( (IIAPI_HNDL *)connHndl );

    IIapi_uiDispatch( IIAPI_EV_ABORT_FUNC,
			(IIAPI_HNDL *)connHndl, (II_PTR)abortParm );
    return;
}
