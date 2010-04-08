/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <gc.h>
# include <pc.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apilower.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>


/*
** Name: apiwait.c
**
** Description:
**	This file contains the IIapi_wait() function interface.
**
**      IIapi_wait  API wait function
**
** History:
**      01-oct-93 (ctham)
**          creation
**      22-apr-94 (ctham)
**          GCsync is changed to allow timeout as an input.  
**      22-apr-94 (ctham)
**          provide a new function IIapi_serviceOpQue() to remove the
**          IIapi_liDispatch(), IIapi_uiDispatch() and IIapi_wait()
**          from going through and dealing with queued events.
**	 3-Nov-94 (gordy)
**	    Added a counter to track nesting of GCsync() calls.  We may
**	    want to disallow nesting as GCsync() may not be re-entrant.
**	20-Jan-95 (gordy)
**	    Only block in GCsync() if GCA is active.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	 9-Mar-95 (gordy)
**	    Remove timeout from GCsync() until it is fixed.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	28-Jul-95 (gordy)
**	    Use fixed length types.  Fixed tracing
**	12-Oct-95 (gordy)
**	    Check and set the dispatch flag prior to re-entering the API.
**	    Otherwise, new API events will not get queued and may see
**	    handles which have not been updated to the correct state yet.
**	17-Jan-96 (gordy)
**	    Added environment handles.  Added global data structure.
**	 2-Oct-96 (gordy)
**	    Removed unused MINICODE ifdef's.
**	14-Nov-96 (gordy)
**	    GCsync() now takes timeout.  Serialization control data moved
**	    to thread-local-storage.
*/


/*
** Name: IIapi_wait
**
** Description:
**	This function provide an interface for the frontend application
**      to poll the next incoming event from the DBMS server.
**
**      waitParm  input and output parameters for IIapi_wait()
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
**      22-apr-94 (ctham)
**          GCsync is changed to allow timeout as an input.  
**      22-apr-94 (ctham)
**          provide a new function IIapi_serviceOpQue() to remove the
**          IIapi_liDispatch(), IIapi_uiDispatch() and IIapi_wait()
**          from going through and dealing with queued events.
**	 3-Nov-94 (gordy)
**	    Added a counter to track nesting of GCsync() calls.  We may
**	    want to disallow nesting as GCsync() may not be re-entrant.
**	11-Nov-94 (gordy)
**	    If we found a queued operation, return without calling
**	    GCsync().  Only block in GCsync() if there was nothing
**	    else to do.
**	20-Jan-95 (gordy)
**	    Only block in GCsync() if GCA is active.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 8-Mar-95 (gordy)
**	    Use IIapi_initialized() to test for init.
**	 9-Mar-95 (gordy)
**	    Remove timeout from GCsync() until it is fixed.
**	28-Jul-95 (gordy)
**	    Fixed tracing.
**	12-Oct-95 (gordy)
**	    Check and set the dispatch flag prior to re-entering the API.
**	    Otherwise, new API events will not get queued and may see
**	    handles which have not been updated to the correct state yet.
**	14-Nov-96 (gordy)
**	    GCsync() now takes timeout.  Serialization control data moved
**	    to thread-local-storage.
*/

II_EXTERN II_VOID II_FAR II_EXPORT
IIapi_wait( IIAPI_WAITPARM II_FAR *waitParm )
{
    IIAPI_THREAD	*thread;
    II_BOOL		api_active = FALSE;

    IIAPI_TRACE( IIAPI_TR_TRACE )
	( "IIapi_wait(%d): waiting for next event\n", PCtid() );
    
    /*
    ** Validate Input parameters
    */
    if ( waitParm == (IIAPI_WAITPARM *)NULL )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )( "IIapi_wait: null wait parameters\n" );
	return;
    }
    
    waitParm->wt_status = IIAPI_ST_SUCCESS;

    /*
    ** Make sure API is initialized
    */
    if ( ! IIapi_initialized() )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR ) 
	    ( "IIapi_wait: API is not initialized\n" );
	waitParm->wt_status = IIAPI_ST_NOT_INITIALIZED;
	return;
    }
    
    if ( ! (thread = IIapi_thread()) )
    {
	waitParm->wt_status = IIAPI_ST_OUT_OF_MEMORY;
	return;
    }

    /*
    ** We should only be called from the top most application 
    ** level, not from a callback (recursive or otherwise).  
    ** If the dispatch flag is set some other dispatcher is
    ** active and we have been called at the wrong time.
    */
    if ( ! IIapi_setDispatchFlag( thread ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_wait: called from a callback function\n" );
	waitParm->wt_status = IIAPI_ST_FAILURE;
	return;
    }

    /*
    ** See if there are any unprocessed API events.
    */
    api_active = IIapi_serviceOpQue( thread );

    /*
    ** If we did work in the API then we want to just 
    ** return and let the client determine if any 
    ** statements have completed.  Otherwise, give GCA 
    ** a chance to complete outstanding requests.  If 
    ** there were no API events and GCA is not active, 
    ** then the client should not have made this call.
    */
    if ( ! api_active )
	if ( ! IIapi_gcaActive( thread ) )
	{
	    IIAPI_TRACE( IIAPI_TR_INFO )( "IIapi_wait: nothing to do!\n" );
	    waitParm->wt_status = IIAPI_ST_WARNING;
	}
	else
	{
	    IIAPI_TRACE( IIAPI_TR_INFO )
		( "IIapi_wait: timeout = %d\n", waitParm->wt_timeout );
    
	    GCsync( waitParm->wt_timeout );

	    /*
	    ** Since we are holding the dispatch flag,
	    ** any GCA events which occured while blocked
	    ** got queued.  Process the events before
	    ** returning.
	    */
	    IIapi_serviceOpQue( thread );
	}

    IIapi_clearDispatchFlag( thread );

    return;
}
