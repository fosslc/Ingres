/*
** Copyright (c) 2009 Ingres Corporation. All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <pc.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apisql.h>
# include <apins.h>
# include <apichndl.h>
# include <apithndl.h>
# include <apishndl.h>
# include <apiehndl.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apidspth.c
**
** Description:
**	API event dispatcher.  API events can originate from three
**	different sources: API functions called by the application,
**	GCA callback functions, and the API itself.  The API will
**	generate its own events to simulate events from the other
**	sources.
**
**	API function events are dispatched by the Upper Interface
**	dispatcher while GCA events are dispatched by the Lower
**	Interface dispatcher.  The dispatchers serialize event
**	processing using the dispatch flag and the operations
**	queue.  Only a single instance of a dispatcher may be
**	active at a time.  Any other dispatcher which finds the
**	dispatch flag set must queue the event on the operations
**	queue.  Each active dispatcher will check the operations
**	queue after processing the event to process any events
**	which were queued during the processing.
**
**	The dispatch flag must be held to service the operations
**	queue (IIapi_serviceOpQue()).
**
**	IIapi_setDispatchFlag		Mark dispatching active.
**	IIapi_clearDispatchFlag		Mark dispatching inactive.
**	IIapi_serviceOpQue		Process events on operations queue.
**	IIapi_sm_init			Initialize state machines.
**	IIapi_uiDispatch		Upper interface dispatcher.
**	IIapi_liDispatch		Lower interface dispatcher.
**
**	Local routines:
**
**	createOpQueEntry		Place event on operations queue.
**	uiDispatch			Dispatch API functions.
**	liDispatch			Dispatch GCA events.
**	Dispatch			Dispatch event to a state machine.
**
** History:
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
**	13-Mar-97 (gordy)
**	    Added Jasmine and Name Server connection types.
**	 3-Jul-97 (gordy)
**	    State machine interface now made through initialization
**	    function which fill in the dispatch table entries.
**	    Added IIapi_sm_init().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	15-Jun-01 (gordy)
**	    Do not discontinue processing if failure occurs while
**	    dispatching an event.  There may be unrelated events
**	    on the queue which can be successfully processed.
**      28-jun-2002 (loera01) Bug 108147
**          Apply mutex protection to handles that access the dispatcher.
**	15-Mar-04 (gordy)
**	    Removed Jasmine support.
**	29-apr-2004 (abbjo03)
**	    On VMS, disable ASTs while calling QUinsert()/QUremove().
**	13-may-2004 (abbjo03)
**	    Fix the 29-apr change testing the result of sys$setast().
**      13-May-2007 (Ralph Loen)  SIR 118032
**          Removed the sys$setast() calls for VMS. They are no longer required
**          because GCsync() now maintains asynchronous I/O in a queue
**          rather than a fixed array.
**      15-May-2007 (Ralph Loen) SIR 118032
**          Removed lingering "#endif" from previous change.
**	23-Sep-2009 (wonst02) SIR 121201
**	    Moved tid=PCtid() so it is computed only when tracing.
*/

/*
** Operations Queue Entries.
*/

typedef II_BOOL (*IIAPI_OPDISPATCH)( IIAPI_EVENT event, 
			             IIAPI_HNDL *ev_hndl,
				     II_PTR parmBlock,
			             IIAPI_DELPARMFUNC delParmFunc );

typedef struct _IIAPI_OPQUEENT
{

    QUEUE		oq_queue;
    IIAPI_EVENT		oq_event;
    IIAPI_HNDL		*oq_handle;
    II_PTR		oq_parmBlock;
    IIAPI_DELPARMFUNC	oq_delParmFunc;
    IIAPI_OPDISPATCH	oq_dispatchFunc;

} IIAPI_OPQUEENT;

/*
** Defines the API state machine interface for each 
** connection and handle type.  These entries are
** filled in by the initialization routine for each
** state machine.
*/

GLOBALDEF IIAPI_SM	*IIapi_sm[ IIAPI_SMT_CNT ][ IIAPI_SMH_CNT ] ZERO_FILL;


/*
** Local functions.
*/

static II_BOOL	createOpQueEntry( IIAPI_THREAD *thread, IIAPI_EVENT event,
				  IIAPI_HNDL *ev_hndl, II_PTR parmBlock,
				  IIAPI_DELPARMFUNC delParmFunc,
				  IIAPI_OPDISPATCH dispatchFunc );

static II_BOOL	uiDispatch( IIAPI_EVENT, 
			    IIAPI_HNDL *, II_PTR, IIAPI_DELPARMFUNC );

static II_BOOL	liDispatch( IIAPI_EVENT, 
			    IIAPI_HNDL *, II_PTR, IIAPI_DELPARMFUNC );

static II_BOOL	Dispatch( IIAPI_HNDL *, IIAPI_EVENT, IIAPI_HNDL *, II_PTR );


/*
** Name: IIapi_setDispatchFlag
**
** Description:
**	The upper and lower dispatchers need to coordinate between
**	themselves so that the stack won't keep growing because
**	a callback caused by one dispatcher is calling the other 
**	dispatcher.  The dispatch flag is set whenever a dispatcher
**	is invoked and cleared when the dispatcher is done.  When 
**	a dispatcher is invoked, it must check if it should proceed
**	(flag is cleared) or queue the event and return (flag is set).
**
**	This function sets the dispatch flag to TRUE if the flag
**	is currently unset.  If the flag is currently set, it
**	returns FALSE to inform the caller.
**
** Input:
**	thread		Thread specific data.
**
** Output:
**	None.
**
** Returns:
**      II_BOOL		TRUE if able to set dispatch flag, FALSE otherwise.
**
** History:
**	 2-Oct-96 (gordy)
**	    Reworked API event dispatching.
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
*/

II_EXTERN II_BOOL
IIapi_setDispatchFlag( IIAPI_THREAD *thread )
{
    II_BOOL	value = ! thread->api_dispatch;
    thread->api_dispatch = TRUE;
    return( value );
}


/*
** Name: IIapi_clearDispatchFlag
**
** Description:
**     This function clears the dispatch flag (see IIapi_setDispatchFlag).
**
** Input:
**	thread		Thread specific data.
**
** Output:
**	None.
**
** Returns:
**     none.
**
** History:
**	 2-Oct-96 (gordy)
**	    Reworked API event dispatching.
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
*/

II_EXTERN II_VOID
IIapi_clearDispatchFlag( IIAPI_THREAD *thread )
{
    thread->api_dispatch = FALSE;
    return;
}


/*
** Name: createOpQueEntry
**
** Description:
**	This function adds a new entry to the operation queue to be
**	dispatched by IIapi_serviceOpQue().
**
** Input:
**	thread		Thread specific data.
**	event		Event number.
**	ev_hndl		Event handle.
**	parmBlock	Event parameter block.
**	delParmFunc	Function to delete parameter block.
**	dispatchFunc	Function to dispatch the event.
**
** Ouput:
**	None.
**
** Returns:
**	II_BOOL		TRUE if success, FALSE if memory allocation error.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     place entry at end of queue.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
*/

static II_BOOL
createOpQueEntry
(
    IIAPI_THREAD	*thread,
    IIAPI_EVENT		event,
    IIAPI_HNDL		*ev_hndl,
    II_PTR		parmBlock,
    IIAPI_DELPARMFUNC	delParmFunc,
    IIAPI_OPDISPATCH	dispatchFunc
)
{
    IIAPI_OPQUEENT	*opQueEnt;
    STATUS		status;

    if ( ( opQueEnt = (IIAPI_OPQUEENT *)
	   MEreqmem( 0, sizeof (IIAPI_OPQUEENT), FALSE, &status ) ) == NULL )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "createOpQueEntry: memory allocation failed\n" );
	return( FALSE );
    }
    
    QUinit( &opQueEnt->oq_queue );
    opQueEnt->oq_event = event;
    opQueEnt->oq_handle = ev_hndl;
    opQueEnt->oq_parmBlock = parmBlock;
    opQueEnt->oq_delParmFunc = delParmFunc;
    opQueEnt->oq_dispatchFunc = dispatchFunc;
    
    /*
    ** Place the entry on the end of the queue.
    */
    QUinsert( (QUEUE *)opQueEnt, thread->api_op_q.q_prev );

    return( TRUE );
}


/*
** Name: IIapi_serviceOpQue
**
** Description:
**	Flushes the operations queue by dispatching each queued event.
**	Any events which get queued during dispatching will also be
**	dispatched.  The operations queue will be empty when this
**	function returns (unless a dispatching error occurs, in which
**	case recovery is not possible).
**
**	The dispatch flag must be successfully set before calling this
**	function.
**
** Input:
**	thread		Thread specific data.
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if event dispatched, FALSE if nothing to do.
**
** History:
**      22-apr-94 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	11-Nov-94 (gordy)
**	    Return indicator that there was a queued operation.
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
**	15-Jun-01 (gordy)
**	    Individual events may not be related, so don't abort
**	    if an error occurs dispatching a particular event.
*/

II_EXTERN II_BOOL
IIapi_serviceOpQue( IIAPI_THREAD *thread )
{
    IIAPI_OPQUEENT	*opQueEnt;
    II_BOOL		found_something = FALSE;

    /*
    ** Get each queued event and dispatch.
    */
    while (1)
    {
	if (!(opQueEnt = (IIAPI_OPQUEENT *)QUremove(thread->api_op_q.q_next)))
	    break;

	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "IIapi_serviceOpQue(%d): processing an operation on the queue\n",
	      PCtid() );
    
	(*opQueEnt->oq_dispatchFunc)( opQueEnt->oq_event, 
				      opQueEnt->oq_handle, 
				      opQueEnt->oq_parmBlock,
				      opQueEnt->oq_delParmFunc );
    
	found_something = TRUE;
	MEfree( (II_PTR)opQueEnt );
    }
   
    return( found_something );
}


/*
** Name: IIapi_sm_init
**
** Description:
**	Initializes the state machines for a specified type.
**
** Input:
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS		API status.
**
** History:
**	 3-Jul-97 (gordy)
**	    Created.
**	15-Mar-04 (gordy)
**	    Removed Jasmine support.
*/

II_EXTERN IIAPI_STATUS
IIapi_sm_init( II_UINT2 sm_type )
{
    IIAPI_STATUS status = IIAPI_ST_FAILURE;

    switch( sm_type )
    {
	case IIAPI_SMT_SQL : status = IIapi_sql_init();		break;
	case IIAPI_SMT_NS  : status = IIapi_ns_init();		break;
    }

    return( status );
}


/*
** Name: IIapi_uiDispatch
**
** Description:
**	Dispatch event associated with an API function 
**	invocation to the appropriate state machines.
**
**	API event processing is serialized to ensure an
**	event is fully processed before subsequent events
**	get dispatched.  The event will be queued if a
**	dispatcher is already active.
**
**	The event handle may be deleted during dispatching.
**
** Input:
**      event		API function event number.
**      parmBlock	API function parameter block.
**      ev_hndl		Handle associated with event.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 2-Oct-96 (gordy)
**	    Reworked API event dispatching.
*/

II_EXTERN II_VOID
IIapi_uiDispatch
(
    IIAPI_EVENT		event,
    IIAPI_HNDL		*ev_hndl,
    IIAPI_GENPARM	*parmBlock
)
{
    IIAPI_THREAD *thread;

    if ( ! (thread = IIapi_thread()) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_uiDispatch: can't obtain thread info, event ignored\n" );

	/*
	** Since we are only called from an API application interface
	** function, we can safely return the error through the
	** parameter block.
	*/
	IIapi_appCallback( parmBlock, NULL, IIAPI_ST_OUT_OF_MEMORY );
    }

    /*
    ** Check to see if event dispatching is active.
    */
    if ( IIapi_setDispatchFlag( thread ) )
    {
	/*
	** Dispatch current event and check for any
	** events which may have been queued during
	** dispatching.  Note that there is nothing
	** we can do if dispatching fails.  Each
	** routine below this point is responsible
	** for communicating any errors to the
	** application through callbacks.
	*/
	uiDispatch( event, ev_hndl, (II_PTR)parmBlock, NULL );
	IIapi_serviceOpQue( thread );
	IIapi_clearDispatchFlag( thread );
    }
    else
    {
	/*
	** Queue the event for later processing
	*/
	IIAPI_TRACE( IIAPI_TR_TRACE )
	    ("IIapi_uiDispatch: queueing event %s\n", IIAPI_PRINTEVENT(event));

	if ( ! createOpQueEntry( thread, event, ev_hndl, 
				 (II_PTR)parmBlock, NULL, uiDispatch ) )
	{
	    /*
	    ** Since we are only called from an API application interface
	    ** function, we can safely return the error through the
	    ** parameter block.
	    */
	    IIapi_appCallback( parmBlock, NULL, IIAPI_ST_OUT_OF_MEMORY );
	}
    }

    return;
}


/*
** Name: IIapi_liDispatch
**
** Description:
**	Dispatch event associated with a GCA callback
**	result to the appropriate state machines.
**
**	API event processing is serialized to ensure an
**	event is fully processed before subsequent events
**	get dispatched.  The event will be queued if a
**	dispatcher is already active.
**
**	The event handle may be deleted during dispatching.
**
** Input:
**      event		GCA result event number.
**      ev_hndl		Handle associated with event.
**      parmBlock	GCA result parameter block.
**      delParmFunc	Function to delete parmBlock.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 2-Oct-96 (gordy)
**	    Reworked API event dispatching.
**	15-Jun-01 (gordy)
**	    Delete the parameter block if event can't be queued.
*/

II_EXTERN II_VOID
IIapi_liDispatch 
(
    IIAPI_EVENT		event,
    IIAPI_HNDL		*ev_hndl,
    II_PTR		parmBlock,
    IIAPI_DELPARMFUNC	delParmFunc
)
{
    IIAPI_THREAD *thread;

    if ( ! (thread = IIapi_thread()) )  
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_liDispatch: can't obtain thread info, event ignored\n" );
	return;
    }

    /*
    ** Check to see if event dispatching is active.
    */
    if ( IIapi_setDispatchFlag( thread ) )
    {
	/*
	** Dispatch current event and check for any
	** events which may have been queued during
	** dispatching.  Note that there is nothing
	** we can do if dispatching fails.
	*/
	liDispatch( event, ev_hndl, parmBlock, delParmFunc );
	IIapi_serviceOpQue( thread );
	IIapi_clearDispatchFlag( thread );
    }
    else
    {
	/*
	** Queue the event for later processing
	*/
	IIAPI_TRACE( IIAPI_TR_TRACE )
	    ("IIapi_liDispatch: queueing event %s\n", IIAPI_PRINTEVENT(event));

	if ( ! createOpQueEntry( thread, event, ev_hndl, 
				 parmBlock, delParmFunc, liDispatch ) )
	{
	    /*
	    ** !!!!!FIX-ME:
	    ** We are only called from a GCA callback routine, 
	    ** so we need some mechanism to surface this error 
	    ** to the calling application.
	    */
	    if ( delParmFunc )  (*delParmFunc)( parmBlock );
	}
    }

    return;
}


/*
** Name: uiDispatch
**
** Description:
**	Dispatchs an event to all handles associated with 
**	the event handle.  Handles are dispatched top-down 
**	so that the states of parent handles can be validated
**	prior to the actual detailed processing of the event
**	handle.
**
**	This function expects to be called as a result of
**	an API function invocation.  Because of this, the
**	delParmFunc parameter is ignored (included so that
**	the parameter list matches that of liDispatch())
**	and the parmBlock must be an API function parameter
**	block (coercable to IIAPI_GENPARM *).
**
** Input:
**      event		API function event number.
**      ev_hndl		Handle associated with event.
**      parmBlock	API function parameter block.
**      delParmFunc	Ignored.
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if successful, FALSE if error.
**
** History:
**	 2-Oct-96 (gordy)
**	    Reworked API event dispatching.
*/

static II_BOOL
uiDispatch
(
    IIAPI_EVENT		event,
    IIAPI_HNDL		*ev_hndl,
    II_PTR		parmBlock,
    IIAPI_DELPARMFUNC	delParmFunc
)
{
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_TRANHNDL	*tranHndl;
    IIAPI_STMTHNDL	*stmtHndl;
    IIAPI_DBEVHNDL	*dbevHndl;
    II_BOOL		success;
    
    if ( event > IIAPI_EVENT_MAX  )
    {
	/*
	** This is an internal error which (in a perfect world) 
	** should not happen.  We can return a general failure 
	** since we are always called with an API function 
	** parameter block.
	*/
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_uiDispatch: INTERNAL ERROR - invalid event number %d\n",
	      event );

	IIapi_appCallback( (IIAPI_GENPARM *)parmBlock, NULL, IIAPI_ST_FAILURE );
	return( FALSE );
    }
    
    IIAPI_TRACE( IIAPI_TR_TRACE )
	("IIapi_uiDispatch: dispatching event %s\n", IIAPI_PRINTEVENT(event));
    
    /*
    ** Get the various handles now in case
    ** the event handle is deleted by its
    ** state manager.
    */
    connHndl = IIapi_getConnHndl( ev_hndl );
    tranHndl = IIapi_getTranHndl( ev_hndl );
    stmtHndl = IIapi_getStmtHndl( ev_hndl );
    dbevHndl = IIapi_getDbevHndl( ev_hndl );

    /*
    ** Dispatch the event to each handle derived
    ** from the event handle.  Note that while we
    ** dispatch handles top-down, we can delete
    ** handles as we go since the related handles
    ** have been extracted above, and no handles
    ** are flagged for deletion when child handles
    ** are present.
    */
    if ( connHndl )
    {
	success = Dispatch((IIAPI_HNDL *)connHndl, event, ev_hndl, parmBlock);
	if ( connHndl->ch_header.hd_delete )  IIapi_deleteConnHndl( connHndl );
	if ( ! success )  return( FALSE );
    }
    
    if ( tranHndl )
    {
	success = Dispatch((IIAPI_HNDL *)tranHndl, event, ev_hndl, parmBlock);
	if ( tranHndl->th_header.hd_delete )  IIapi_deleteTranHndl( tranHndl );
	if ( ! success )  return( FALSE );
    }
    
    if ( stmtHndl )
    {
	success = Dispatch((IIAPI_HNDL *)stmtHndl, event, ev_hndl, parmBlock);
	if ( stmtHndl->sh_header.hd_delete )  IIapi_deleteStmtHndl( stmtHndl );
	if ( ! success )  return( FALSE );
    }
    
    if ( dbevHndl )
    {
	success = Dispatch((IIAPI_HNDL *)dbevHndl, event, ev_hndl, parmBlock);
	if ( dbevHndl->eh_header.hd_delete )  IIapi_deleteDbevHndl( dbevHndl );
	if ( ! success )  return( FALSE );
    }
    
    return( TRUE );
}


/*
** Name: liDispatch
**
** Description:
**	Dispatchs an event to all handles associated with 
**	the event handle.  Handles are dispatched bottom-up
**	so that event specific handling (query results) can
**	be processed prior to more general processing
**	(connection failure, transaction aborts).
**
**	The actual datatype of the parameter block depends
**	on the GCA event.  A function may be provided to
**	delete the parameter block once dispatching is
**	completed.
**
** Input:
**      event		GCA result event number.
**      ev_hndl		Handle associated with event.
**      parmBlock	GCA result parameter block.
**      delParmFunc	Function to delete parameter block.
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if successful, FALSE if error.
**
** History:
**	 2-Oct-96 (gordy)
**	    Reworked API event dispatching.
*/

static II_BOOL
liDispatch 
(
    IIAPI_EVENT		event,
    IIAPI_HNDL		*ev_hndl,
    II_PTR		parmBlock,
    IIAPI_DELPARMFUNC	delParmFunc
)
{
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_TRANHNDL	*tranHndl;
    IIAPI_STMTHNDL	*stmtHndl;
    IIAPI_DBEVHNDL	*dbevHndl;
    II_BOOL		success;

    if ( event > IIAPI_EVENT_MAX  )
    {
	/*
	** This is an internal error which (in a perfect world) 
	** should not happen.  !!!!!FIX-ME: We are only called 
	** from a GCA callback routine, so we need some mechanism 
	** to surface this error to the calling application.
	*/
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_liDispatch: INTERNAL ERROR - invalid event number %d\n",
	      event );
	goto failure;
    }

    IIAPI_TRACE( IIAPI_TR_TRACE )
	("IIapi_liDispatch: dispatching event %s\n", IIAPI_PRINTEVENT(event));
    
    /*
    ** Get the various handles now in case
    ** the event handle is deleted by its
    ** state manager.
    */
    connHndl = IIapi_getConnHndl( ev_hndl );
    tranHndl = IIapi_getTranHndl( ev_hndl );
    stmtHndl = IIapi_getStmtHndl( ev_hndl );
    dbevHndl = IIapi_getDbevHndl( ev_hndl );

    /*
    ** Dispatch the event to each handle derived
    ** from the event handle.  Note that we can 
    ** delete handles as we go since the related 
    ** handles have been extracted above (just be
    ** sure that we don't propogate a deleted
    ** handle as the event handle).
    */
    if ( dbevHndl )
    {
	success = Dispatch((IIAPI_HNDL *)dbevHndl, event, ev_hndl, parmBlock);

	if ( dbevHndl->eh_header.hd_delete )  
	{
	    if ( ev_hndl == (IIAPI_HNDL *)dbevHndl )  
		ev_hndl = (IIAPI_HNDL *)connHndl;
	    IIapi_deleteDbevHndl( dbevHndl );
	}

	if ( ! success )  goto failure;
    }

    if ( stmtHndl )
    {
	success = Dispatch((IIAPI_HNDL *)stmtHndl, event, ev_hndl, parmBlock);

	if ( stmtHndl->sh_header.hd_delete )  
	{
	    if ( ev_hndl == (IIAPI_HNDL *)stmtHndl )  
		ev_hndl = (IIAPI_HNDL *)tranHndl;
	    IIapi_deleteStmtHndl( stmtHndl );
	}

	if ( ! success )  goto failure;
    }
    
    if ( tranHndl )
    {
	success = Dispatch((IIAPI_HNDL *)tranHndl, event, ev_hndl, parmBlock);

	if ( tranHndl->th_header.hd_delete )  
	{
	    if ( ev_hndl == (IIAPI_HNDL *)tranHndl ) 
		ev_hndl = (IIAPI_HNDL *)connHndl;
	    IIapi_deleteTranHndl( tranHndl );
	}

	if ( ! success )  goto failure;
    }
    
    if ( connHndl )
    {
	success = Dispatch((IIAPI_HNDL *)connHndl, event, ev_hndl, parmBlock);

	if ( connHndl->ch_header.hd_delete )  
	{
	    if ( ev_hndl == (IIAPI_HNDL *)connHndl )  ev_hndl = NULL;
	    IIapi_deleteConnHndl( connHndl );
	}

	if ( ! success )  goto failure;
    }
    
    if ( delParmFunc )  (*delParmFunc)( parmBlock );

    return( TRUE );

 failure:

    /*
    ** Some internal error has occured which could
    ** not be handled by the state managers.
    */
    if ( delParmFunc ) (*delParmFunc)( parmBlock );

    return( FALSE );
}


/*
** Name: Dispatch
**
** Description:
**	Dispatches an API event to a given state machine and
** 	associated handle (instance).  Calls the state machine 
**	evaluation function to determine if the event should
**	be dispatched to the state machine.  If the evaluator
**	returns the state machine output info, the indicated
**	state transition is made and the actions executed.
**
** Input:
**	smi		State machine instance.
**	event		API event number.
**	ev_hndl		Event handle.
**	sm_hndl		State machine handle.
**	parmBlock	Event parameter block.
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if successful, FALSE otherwise.
**
** History:
**	 2-Oct-96 (gordy)
**	    Reworked API event dispatching.
**	17-Feb-2009 (wonst02) SIR 121201
**	    Moved tid=PCtid() so it is computed only when tracing.
*/

static II_BOOL
Dispatch
( 
    IIAPI_HNDL	*sm_hndl, 
    IIAPI_EVENT	event, 
    IIAPI_HNDL	*ev_hndl, 
    II_PTR	parmBlock 
)
{
    IIAPI_SMI		*smi = &sm_hndl->hd_smi;
    IIAPI_SM		*sm = smi->smi_sm;
    IIAPI_SM_OUT	*smo, obuff;
    IIAPI_STATE		prev_state;
    i4			tid = 0;
    i4			action;
    II_BOOL		success=TRUE;

    if ( smi->smi_state >= sm->sm_state_cnt )
    {
	/*
	** This is an internal error which (in 
	** a perfect world) should not happen.  
	*/
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "Dispatch INTERNAL ERROR: %s [%p] invalid state %d\n",
	      sm->sm_id, (PTR)sm_hndl, smi->smi_state );
	    return( FALSE );
    }

    /*
    ** Evaluate the state machine actions
    ** if any, for the current event.
    */
    MUp_semaphore( &sm_hndl->hd_sem );
    smo = (*sm->sm_evaluate)( event, smi->smi_state, 
			      ev_hndl, sm_hndl, parmBlock, &obuff );

    if ( ! smo )
    {
	IIAPI_TRACE( IIAPI_TR_VERBOSE )
	    ( "Dispatch(%d): %s [%p] event ignored\n", 
	      PCtid(), sm->sm_id, (PTR)sm_hndl );
    }
    else
    {
	/*
	** Make the state transition.
	*/
	IIAPI_TRACE( IIAPI_TR_TRACE )
	    ( "Dispatch(%d): %s [%p] %s --> %s, %d action(s)\n",
	      tid = PCtid(), sm->sm_id, (PTR)sm_hndl, 
	      IIAPI_PRINT_ID( smi->smi_state, 
			      sm->sm_state_cnt, sm->sm_state_id ),
	      IIAPI_PRINT_ID( smo->smo_next_state, 
			      sm->sm_state_cnt, sm->sm_state_id ),
	      smo->smo_action_cnt );

	prev_state = smi->smi_state;
	smi->smi_state = smo->smo_next_state;

	/*
	** Execute each action.
	*/
	for( action = 0; action < smo->smo_action_cnt; action++ )
	{
	    IIAPI_TRACE( IIAPI_TR_INFO )
		( "Dispatch(%d): action[%d] %s\n", 
		  (tid ? tid : (tid = PCtid())), action + 1,
		  IIAPI_PRINT_ID( smo->smo_actions[ action ],
				  sm->sm_action_cnt, sm->sm_action_id ) );

	    if ( ! (*sm->sm_action)( smo->smo_actions[ action ], 
				     ev_hndl, sm_hndl, parmBlock ) )
	    {
		/*
		** A fatal error has occured such that the
		** requested event could not be completed
		** nor the error handled by normal event
		** processing.  Abort action execution,
		** restore the original state and return
		** the error condition.
		*/
		IIAPI_TRACE( IIAPI_TR_VERBOSE )
		    ( "Dispatch(%d): %s [%p] action failed\n", 
		      (tid ? tid : (tid = PCtid())), sm->sm_id, (PTR)sm_hndl );
		smi->smi_state = prev_state;
		success = FALSE;
                break;
	    }
	}
    }

    MUv_semaphore( &sm_hndl->hd_sem );
    return( success );
}

