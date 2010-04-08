/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apitname.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apirelxi.c
**
** Description:
**	This file contains the IIapi_releaseXID() function interface.
**
**      IIapi_releaseXID  API release transaction ID function
**
** History:
**      01-oct-93 (ctham)
**          creation
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	 6-Mar-95 (gordy)
**	    Stardardize on transaction ID rather than transaction name.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	14-Jun-95 (gordy)
**	    Do not update input only parameters.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 2-Oct-96 (gordy)
**	    Removed unused MINICODE ifdef's.
*/




/*
** Name: IIapi_releaseXID
**
** Description:
**	This function provide an interface for the frontend application
**      to release a globally unique transaction ID with GCA-API for
**      when the ID is no longer needed by any transactions.  Please
**      refer to the API user specification for function description.
**
**      relXIDParm  input and output parameters for IIapi_releaseXID()
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
**	23-Jan-95 (gordy)
**	    Clean up error handling.
**	 6-Mar-95 (gordy)
**	    Stardardize on transaction ID rather than transaction name.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	14-Jun-95 (gordy)
**	    Do not update input only parameters.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_releaseXID( IIAPI_RELXIDPARM II_FAR *relXIDParm )
{
    IIAPI_TRANNAME	*tranName;
    
    IIAPI_TRACE(IIAPI_TR_TRACE)
	("IIapi_releaseXID: releasing transaction ID with API\n");

    /*
    ** Validate Input parameters
    */
    if ( ! relXIDParm )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_releaseXID: null releaseXID parameters\n" );
	return;
    }

    relXIDParm->rl_status = IIAPI_ST_SUCCESS;
    tranName = (IIAPI_TRANNAME *)relXIDParm->rl_tranIdHandle;
    
    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_releaseXID: API is not initialized\n" );
	relXIDParm->rl_status = IIAPI_ST_NOT_INITIALIZED;
	return;
    }
    
    if ( ! IIapi_isTranName( tranName ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_releaseXID: invalid transaction handle\n" );
	relXIDParm->rl_status = IIAPI_ST_INVALID_HANDLE;
	return;
    }

    if ( ! IIapi_isQueEmpty( &tranName->tn_tranHndlList ) )
    {
	IIAPI_TRACE(IIAPI_TR_ERROR)
	    ("IIapi_releaseXID: can't delete with active transactions\n");
	relXIDParm->rl_status = IIAPI_ST_FAILURE;
	return;
    }

    IIAPI_TRACE( IIAPI_TR_INFO ) 
	( "IIapi_releaseXID: tranHndl = %p\n", tranName );

    /*
    ** Delete transaction name
    */
    IIapi_deleteTranName( tranName );

    return;
}
