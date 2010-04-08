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
# include <apichndl.h>
# include <apiehndl.h>
# include <apierhnd.h>
# include <apilower.h>
# include <apidatav.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apiehndl.c
**
** Description:
**	This file defines the Database Event handle management functions.
**
**	IIapi_createDbevCB	Create database event control block.
**	IIapi_processDbevCB	Process database event control blocks.
**      IIapi_createDbevHndl	Create database event handle.
**      IIapi_deleteDbevHndl	Delete database event handle.
**	IIapi_abortDbevHndl	Abort database event handle.
**      IIapi_isDbevHndl	Validate database event handle.
**      IIapi_getDbevHndl	Get database event handle.
**
**	Local Routines:
**
**	delete_dbevCB		Delete database event control block.
**	dispatch_dbev		Match database event to database event handles.
**
** History:
**      30-sep-93 (ctham)
**          creation
**      11-apr-94 (ctham)
**          fix IIapi_getEventHndl() to deal with empty event handle
**          queue.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     clean up error handle interface.
**	 31-may-94 (ctham)
**	     changed ce_event{Name,Owner} to ce_selectEvent{Name,Owner}.
**      31-may-94 (ctham)
**          use errorQue as the current error handle reference.
**	26-Oct-94 (gordy)
**	    General cleanup and minor fixes.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 9-Mar-95 (gordy)
**	    Silence compiler warnings.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	13-Jun-95 (gordy)
**	    Allocate data buffer before calling IIapi_cnvtGDV2DataValue().
**	13-Jul-95 (gordy)
**	    Add missing header files, cast for type correctness.
**	14-Sep-95 (gordy)
**	    GCD_GCA_ prefix changed to GCD_.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 8-Mar-96 (gordy)
**	    Added support for GCA_EVENT messages which are split
**	    across buffers.
**	15-Mar-96 (gordy)
**	    Two byte data lengths should be unsigned.
**	22-Apr-96 (gordy)
**	    Changed ch_msg_continued to flag bits.
**	15-Jul-96 (gordy)
**	    Fixed bug introduced when support for split messages added.
**	    Use correct count when allocating descriptor and value arrays.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	    Renamed just about everything to distinguish between
**	    Database Events and API events.
**	 3-Sep-98 (gordy)
**	    Added IIapi_abortDbevHndl().
**	26-Jan-99 (rajus01)
**	    Don't dispatch the event control block unless
**	    it is the first element of the control block 
**	    queue and is new.
**	23-Mar-99 (rajus01)
**	    Fixed a core dump problem that occured when GCA_EVENT messages
**	    are split.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**      28-jun-2002 (loera01) Bug 108147
**          Apply mutex protection to db event handle.
**	14-Jul-2006 (kschendel)
**	    Remove u_i2 casts in MEcopy calls.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/


static	II_VOID		delete_dbevCB( IIAPI_DBEVCB * );
static	II_VOID		dispatch_dbev( IIAPI_DBEVCB * );



/*
** Name: IIapi_createDbevCB
**
** Description:
**	This function creates an database event control block.
**	The event data is read from the connection receive queue.
**	
**
** Input:
**	connHndl	Connection handle of database event.
**
** Ouput:
**	none
**
** Returns:
**	VOID
**
** History:
**	25-Apr-95 (gordy)
**	    Created.
**	13-Jun-95 (gordy)
**	    Allocate data buffer before calling IIapi_cnvtGDV2DataValue().
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	 8-Mar-96 (gordy)
**	    Added support for GCA_EVENT messages which are split
**	    across buffers.
**	15-Mar-96 (gordy)
**	    Two byte data lengths should be unsigned.
**	22-Apr-96 (gordy)
**	    Changed ch_msg_continued to flag bits.
**	15-Jul-96 (gordy)
**	    Fixed bug introduced when support for split messages added.
**	    Use correct count when allocating descriptor and value arrays.
**	15-Aug-98 (rajus01)
**	    Added envHndl parameter to IIapi_cnvtGDV2Descr().
**	23-Mar-99 (rajus01)
**	    Fixed a core dump problem that occured when GCA_EVENT messages
**	    are split.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

II_EXTERN II_VOID
IIapi_createDbevCB( IIAPI_CONNHNDL *connHndl )
{
    IIAPI_ENVHNDL	*envHndl = connHndl->ch_envHndl;
    IIAPI_DBEVCB	*dbevCB;
    STATUS		status;
    
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_createDbevCB: create an database event control block\n" );
    
    if ( ! (dbevCB = (IIAPI_DBEVCB *)
	    MEreqmem( 0, sizeof(IIAPI_DBEVCB), FALSE, NULL )) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ("IIapi_createDbevCB: can't alloc database event control block\n");
	return;
    }
    
    /*
    ** Establish valid initial control block state.
    */
    dbevCB->ev_id.hi_hndlID = IIAPI_HI_DBEV;
    dbevCB->ev_connHndl = connHndl;
    dbevCB->ev_notify = -2;		/* Undispatched CB's are negative */

    dbevCB->ev_name = NULL;
    dbevCB->ev_owner = NULL;
    dbevCB->ev_database = NULL;
    dbevCB->ev_time.dv_value = NULL;
    dbevCB->ev_descriptor = NULL;
    dbevCB->ev_data = NULL;
    dbevCB->ev_count = 0;

    QUinit( (QUEUE *)&dbevCB->ev_id.hi_queue );
    QUinit( (QUEUE *)&dbevCB->ev_dbevHndlList );
    QUinsert( (QUEUE *)dbevCB, &connHndl->ch_dbevCBList );

    if ( (status = IIapi_readMsgEvent(connHndl, dbevCB)) != IIAPI_ST_SUCCESS )
    {
	delete_dbevCB( dbevCB );
	return;
    }

    /*
    ** This database event control block may be dispatchable 
    ** so call IIapi_processDbevCB() to check.
    */
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_createDbevCB: dbevCB %p created\n", dbevCB );

    dbevCB->ev_notify = -1;		/* Event message is complete */
    IIapi_processDbevCB( connHndl );
    return;
}




/*
** Name: delete_dbevCB
**
** Description:
**	This function deletes a database event control block.  
**	The database event handle queue is not checked so the 
**	caller must make sure that it is empty.
**
** Input:
**      dbevCB		Database event control block to be deleted.
**
** Output:
**	none
**
** Returns:
**      VOID
**
** History:
**	25-Apr-95 (gordy)
**	    Created.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	25-Mar-10 (gordy)
**	    Ensure NULL pointers are not dereferenced.
*/

static II_VOID
delete_dbevCB( IIAPI_DBEVCB *dbevCB )
{
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_deleteDbevCB: delete database event control block %p\n", 
	  dbevCB );
    
    QUremove( (QUEUE *)dbevCB );
    dbevCB->ev_id.hi_hndlID = ~IIAPI_HI_DBEV;

    if ( dbevCB->ev_data )
    {
	i4 i;

	for( i = 0; i < dbevCB->ev_count; i++ )
	    MEfree( (II_PTR)dbevCB->ev_data[ i ].dv_value );

	MEfree( (II_PTR)dbevCB->ev_data );
    }

    if ( dbevCB->ev_descriptor )  MEfree( (II_PTR)dbevCB->ev_descriptor );
    if ( dbevCB->ev_time.dv_value ) MEfree( (II_PTR)dbevCB->ev_time.dv_value );
    if ( dbevCB->ev_database )  MEfree( (II_PTR)dbevCB->ev_database );
    if ( dbevCB->ev_owner )  MEfree( (II_PTR)dbevCB->ev_owner );
    if ( dbevCB->ev_name )  MEfree( (II_PTR)dbevCB->ev_name );
    MEfree( (II_PTR)dbevCB );
    
    return;
}




/*
** Name: dispatch_dbev
**
** Description:
**	Matches a database event control block to database event 
**	handles waiting on the connection handle queue and issues 
**	new API events to activate the callbacks for the database
**	event handles.
**
** Input:
**	dbevCB		Database event control block to be dispatched.
**
** Output:
**	none
**
** Returns:
**	VOID
**
** History:
**	25-Apr-95 (gordy)
**	    Created.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	14-Nov-96 (gordy)
**	    Dispatch events using lower interface dispatcher.
*/

static II_VOID
dispatch_dbev( IIAPI_DBEVCB *dbevCB )
{
    IIAPI_DBEVHNDL	*dbevHndl;
    QUEUE		*q, *next;

    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "dispatch_dbev: dispatching database event control block %p\n", 
	  dbevCB );

    dbevCB->ev_notify = 0;

    /*
    ** We scan the handle queue in reverse order to get
    ** the oldest handles first.  Watch out for handles
    ** being removed from the queue during the scan.
    */
    for( 
	 q = dbevCB->ev_connHndl->ch_dbevHndlList.q_prev;
	 q != &dbevCB->ev_connHndl->ch_dbevHndlList;
	 q = next 
       )
    {
	next = q->q_prev;
	dbevHndl = (IIAPI_DBEVHNDL *)q;

	/*
	** Ignore cancelled database events.
	*/
	if ( dbevHndl->eh_cancelled )  continue;

	if ( 
	     ( ! dbevHndl->eh_dbevName  || 
	       STcompare(dbevHndl->eh_dbevName, dbevCB->ev_name) == 0 )  &&
	     ( ! dbevHndl->eh_dbevOwner  || 
	       STcompare(dbevHndl->eh_dbevOwner, dbevCB->ev_owner) == 0 ) 
	   )
	{
	    IIAPI_TRACE( IIAPI_TR_VERBOSE )
		( "dispatch_dbev: control block matched to handle %p\n",
		  dbevHndl );

	    /*
	    ** Move the database event handle to the 
	    ** database event control block queue.
	    */
	    QUremove( (QUEUE *)dbevHndl );
	    QUinsert( (QUEUE *)dbevHndl, &dbevCB->ev_dbevHndlList );
            dbevHndl->eh_parent = (II_PTR)dbevCB;
	    dbevHndl->eh_done = (dbevCB->ev_count <= 0);

	    /*
	    ** Dispatch an event to callback the database event handle.
	    */
	    dbevCB->ev_notify++;
	    IIapi_liDispatch( IIAPI_EV_EVENT_RCVD, 
			      (IIAPI_HNDL *)dbevHndl, NULL, NULL );
	}
    }

    /*
    ** If no matching database event handles were found, then
    ** the database event control block can be freed. We call
    ** IIapi_processDbevCB() (recursively!) to handle
    ** this and any other control blocks which may be
    ** dispatchable.
    */
    if ( ! dbevCB->ev_notify )
    {
	IIAPI_ENVHNDL       *envHndl = IIapi_getEnvHndl(
				       (IIAPI_HNDL *)dbevCB->ev_connHndl );
	IIAPI_USRPARM       *usrParm = &envHndl->en_usrParm;
	IIAPI_EVENTPARM     eventParm;

	IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "dispatch_dbev: No matching event handle for the control block.\n" );

	if ( usrParm->up_mask1 & IIAPI_UP_EVENT_FUNC )
	{

	    eventParm.ev_envHandle 	= 
	    		(envHndl == IIapi_defaultEnvHndl()) ? NULL : envHndl;
	    eventParm.ev_connHandle	= dbevCB->ev_connHndl;

	    if( ! ( eventParm.ev_eventName = STalloc( dbevCB->ev_name ) ) )
		IIAPI_TRACE( IIAPI_TR_ERROR )
	    	( "dispatch_dbev: can't allocate database event name\n" );
	    if( ! ( eventParm.ev_eventOwner = STalloc( dbevCB->ev_owner ) ) )
		IIAPI_TRACE( IIAPI_TR_ERROR )
	    	( "dispatch_dbev: can't allocate database event Owner\n");
	    if( ! ( eventParm.ev_eventDB = STalloc( dbevCB->ev_database ) ) )
		IIAPI_TRACE( IIAPI_TR_ERROR )
	    	( "dispatch_dbev: can't allocate database event db\n");

	    MEcopy( (PTR)&dbevCB->ev_time, sizeof(dbevCB->ev_time),
			    		(PTR)&eventParm.ev_eventTime );

	    IIAPI_TRACE( IIAPI_TR_INFO ) 
			("dispatch_dbev: Application event func callback..\n");

	    (*usrParm->up_event_func)( &eventParm );

	    if( eventParm.ev_eventName )
	    	MEfree( (II_PTR)eventParm.ev_eventName );
	    if( eventParm.ev_eventOwner )
	    	MEfree( (II_PTR)eventParm.ev_eventOwner );
	    if( eventParm.ev_eventDB )
	    	MEfree( (II_PTR)eventParm.ev_eventDB );
	}

	IIapi_processDbevCB( dbevCB->ev_connHndl );

    }

    return;
}




/* Name: IIapi_processDbevCB
** 
** Description:
**	Scans the database event control blocks to see if 
**	any need dispatching or may be freed.
**
**	Control blocks exist on the queue in the following
**	four states: new, partially dispatched, dispatched,
**	and complete.  To assure that similar database events 
**	do not use the same database event handle, a new 
**	control block can not be dispatched until all preceding 
**	control blocks have been fully dispatched.  This routine 
**	guarantees that this requirement is met.  New control 
**	blocks are only dispatched when all preceding control 
**	blocks have been freed or are dispatched.  Control 
**	blocks which have completed processing are freed.
**
**	Partially dispatched control blocks become fully
**	dispatched when all the associated callbacks have
**	been made.  Dispatched control blocks complete
**	when all associated database event handles have 
**	been closed.  These actions are performed in the 
**	database event subsystem and this routine is then 
**	called clean up the queue.
**
**	This routine is also called when a new control
**	block is added to the queue (to see if it is
**	dispatchable) and from the dispatcher if the
**	database event fails to match any database event 
**	handles (the control block can be freed).
**
** Input:
**	connHndl	Connection handle
**
** Output:
**	none
**
** Returns:
**	VOID
**
** History:
**	25-Apr-95 (gordy)
**	    Created.
**	26-Jan-99 (rajus01)
**	    Don't dispatch the event control block unless
**	    it is the first element of the control block 
**	    queue and is new.
*/

II_EXTERN II_VOID
IIapi_processDbevCB( IIAPI_CONNHNDL *connHndl )
{
    QUEUE		*cb, *next_cb; 

    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_processDbevCB: processing database event control blocks\n" );

    /*
    ** Scan queue in reverse order to get oldest
    ** control blocks first.  Watch out for deletion
    ** of control blocks during the loop.  Control
    ** blocks for which ev_notify == 0 are fully
    ** dispatched and can either be freed (database
    ** event handle queue is empty) or skipped and 
    ** left on queue (waiting for database event 
    ** handles to be closed).  Stop at end of queue 
    ** or we reach any non-dispatched control block.
    */
    for( 
	 cb = connHndl->ch_dbevCBList.q_prev;
	 cb != &connHndl->ch_dbevCBList && ! ((IIAPI_DBEVCB *)cb)->ev_notify; 
	 cb = next_cb 
       )
    {
	next_cb = cb->q_prev;

	if ( IIapi_isQueEmpty( &((IIAPI_DBEVCB *)cb)->ev_dbevHndlList ) )
	    delete_dbevCB( (IIAPI_DBEVCB *)cb );
    }

    /*
    ** A new control block (ev_notify < 0) is
    ** dispatchable if it is at the head of the
    ** queue and if the event message is completely
    ** received (ev_notify == -1).
    */
    if ( ! IIapi_isQueEmpty( &connHndl->ch_dbevCBList ) )
    {
	cb = connHndl->ch_dbevCBList.q_prev;
	if (( (IIAPI_DBEVCB *)cb )->ev_notify == -1 ) 
	    dispatch_dbev( (IIAPI_DBEVCB *)cb );
    }

    return;
}




/*
** Name: IIapi_createDbevHndl
**
** Description:
**	This function creates a database event handle.
**
**      catchEventParm		Input argument of IIapi_catchEvent.
**
** Return value:
**      dbevHndl	Database event handle if successful, NULL otherwise.
**
** History:
**      19-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 31-may-94 (ctham)
**	     changed ce_event{Name,Owner} to ce_selectEvent{Name,Owner}.
**      31-may-94 (ctham)
**          use errorQue as the current error handle reference.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
*/

II_EXTERN IIAPI_DBEVHNDL*
IIapi_createDbevHndl( IIAPI_CATCHEVENTPARM *catchEventParm )
{
    IIAPI_DBEVHNDL	*dbevHndl;
    IIAPI_CONNHNDL	*connHndl;
    STATUS		status;
    
    connHndl = (IIAPI_CONNHNDL *)catchEventParm->ce_connHandle;

    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_createDbevHndl: create a database event handle\n" );
    
    if ( ! ( dbevHndl = (IIAPI_DBEVHNDL *)
	     MEreqmem( 0, sizeof(IIAPI_DBEVHNDL), TRUE, &status ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createDbevHndl: can't alloc database event handle\n" );
	goto dbevHandleAllocFail;
    }
    
    /*
    ** Initialize database event handle parameters, provide default values.
    */
    dbevHndl->eh_header.hd_id.hi_hndlID = IIAPI_HI_DBEV_HNDL;
    dbevHndl->eh_header.hd_delete = FALSE;
    dbevHndl->eh_header.hd_smi.smi_state = IIAPI_IDLE;
    dbevHndl->eh_header.hd_smi.smi_sm =
			IIapi_sm[ connHndl->ch_type ][ IIAPI_SMH_DBEV ];
    if ( MUi_semaphore( &dbevHndl->eh_header.hd_sem ) != OK )
    {
        IIAPI_TRACE( IIAPI_TR_FATAL )
            ( "IIapi_DbevHndl: can't create semaphore\n" );
	goto dbevNameAllocFail;
    }

    dbevHndl->eh_parent = (II_PTR)connHndl;
    dbevHndl->eh_cancelled = FALSE;
    dbevHndl->eh_done = FALSE;
    dbevHndl->eh_dbevName = NULL;
    dbevHndl->eh_dbevOwner = NULL;
    dbevHndl->eh_callback = FALSE;

    if ( catchEventParm->ce_selectEventName  &&
         ! ( dbevHndl->eh_dbevName = 
	     STalloc( catchEventParm->ce_selectEventName ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createDbevHndl: can't allocate database event name\n" );
	goto dbevNameAllocFail;
    }
    
    if ( catchEventParm->ce_selectEventOwner  &&
         ! ( dbevHndl->eh_dbevOwner =
	     STalloc( catchEventParm->ce_selectEventOwner ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createDbevHndl: can't allocate database event owner\n" );
	goto dbevOwnerAllocFail;
    }
    
    QUinit( (QUEUE *)&dbevHndl->eh_header.hd_id.hi_queue );
    QUinit( (QUEUE *)&dbevHndl->eh_header.hd_errorList );
    dbevHndl->eh_header.hd_errorQue = &dbevHndl->eh_header.hd_errorList;

    /*
    ** Database event Handles begin life on a queue of the 
    ** connection handle.  When a matching database event 
    ** arrives, the database event handle is moved to a 
    ** queue of the database event control block.
    */
    QUinsert( (QUEUE *)dbevHndl, &connHndl->ch_dbevHndlList );
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_createDbevHndl: dbevHndl %p created\n", dbevHndl );
    
    return( dbevHndl );

 dbevOwnerAllocFail:

    MEfree( (II_PTR)dbevHndl->eh_dbevName );

 dbevNameAllocFail:

    MEfree( (II_PTR)dbevHndl );

 dbevHandleAllocFail:

    return( NULL );
}




/*
** Name: IIapi_deleteDbevHndl
**
** Description:
**	This function deletes a database event handle.
**
**      dbevHndl	Database event handle to be deleted
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
**	26-Oct-94 (gordy)
**	    Always reset the handle ID for deleted Handles.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**       9-Aug-99 (rajus01)
**          Check hd_delete flag before removing the handle from
**          dbevHndl queue. ( Bug #98303 )
*/

II_EXTERN II_VOID
IIapi_deleteDbevHndl( IIAPI_DBEVHNDL *dbevHndl )
{
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_deleteDbevHndl: delete dtabase event handle %p\n", dbevHndl );
    
    if ( dbevHndl->eh_dbevOwner )  MEfree( (II_PTR)dbevHndl->eh_dbevOwner );
    if ( dbevHndl->eh_dbevName )   MEfree( (II_PTR)dbevHndl->eh_dbevName );

    IIapi_cleanErrorHndl( &dbevHndl->eh_header );
    dbevHndl->eh_header.hd_id.hi_hndlID = ~IIAPI_HI_DBEV_HNDL;
    MUr_semaphore( &dbevHndl->eh_header.hd_sem );

    /*
    ** Active and cancelled event handles are sitting on a
    ** connection handle queue.  Processed event handles are 
    ** sitting on an event control block queue.  If this is 
    ** the last event handle on the event control block, the
    ** event control block should be deleted.  This can be 
    ** done by calling IIapi_processDbevCB() after the event 
    ** handle has been removed.  Even if the parent is a
    ** connection handle, we still want to remove the event
    ** handle from the queue.
    */
    if( ! dbevHndl->eh_header.hd_delete )
        QUremove( (QUEUE *)dbevHndl );

    if ( ! IIapi_isConnHndl( (IIAPI_CONNHNDL *)dbevHndl->eh_parent ) )
    {
	IIAPI_DBEVCB *dbevCB = (IIAPI_DBEVCB *)dbevHndl->eh_parent;

	if ( IIapi_isQueEmpty( &dbevCB->ev_dbevHndlList ) )
	    IIapi_processDbevCB( dbevCB->ev_connHndl );
    }

    MEfree( (II_PTR)dbevHndl );
    
    return;
}



/*
** Name: IIapi_abortDbevHndl
**
** Description:
**	Aborts any operation active on the provided database event handle.
**
** Input:
**	dbevHndl	Database event handle.
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
IIapi_abortDbevHndl
( 
    IIAPI_DBEVHNDL	*dbevHndl, 
    II_LONG		errorCode, 
    char		*SQLState,
    IIAPI_STATUS	status
)
{
    /*
    ** Place the database event handle in the abort state.
    */
    dbevHndl->eh_header.hd_smi.smi_state =
			dbevHndl->eh_header.hd_smi.smi_sm->sm_abort_state;

    /*
    ** Abort current operation (if any).
    */
    if ( dbevHndl->eh_callback )
    {
	if ( ! IIapi_localError( (IIAPI_HNDL *)dbevHndl, 
				 errorCode, SQLState, status ) )
	    status = IIAPI_ST_OUT_OF_MEMORY;

	IIapi_appCallback( dbevHndl->eh_parm, (IIAPI_HNDL *)dbevHndl, status );
	dbevHndl->eh_callback = FALSE;
    }

    return;
}




/*
** Name: IIapi_isDbevHndl
**
** Description:
**     This function returns TRUE if the input is a valid database event
**     handle.
**
**     dbevHndl		Database event handle to be validated
**
** Return value:
**     status		TRUE if database event handle is valid, FALSE otherwise.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	26-Oct-94 (gordy)
**	    Remove tracing: caller can decide if trace is needed.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
*/

II_EXTERN II_BOOL
IIapi_isDbevHndl( IIAPI_DBEVHNDL *dbevHndl )
{
    return( dbevHndl && 
	    dbevHndl->eh_header.hd_id.hi_hndlID == IIAPI_HI_DBEV_HNDL );
}




/*
** Name: IIapi_getDbevHndl
**
** Description:
**	This function returns the database event handle in the generic handle
**      parameter.
**
** Return value:
**      dbevHndl	Database event handle
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      11-apr-94 (ctham)
**          fix IIapi_getEventHndl() to deal with empty event handle
**          queue.
**	26-Oct-94 (gordy)
**	    Allow for NULL handles.  Since multiple event handles
**	    can be active in a connection, don't just return the first.
**	20-Dec-94 (gordy)
**	    Cleaning up common handle processing.
*/

II_EXTERN IIAPI_DBEVHNDL *
IIapi_getDbevHndl( IIAPI_HNDL *handle )
{
    if ( handle )
    {
	switch( handle->hd_id.hi_hndlID )
	{
	    case IIAPI_HI_DBEV_HNDL:
		break;

	    default:
		handle = NULL;
		break;
	}
    }
    
    return( (IIAPI_DBEVHNDL *)handle );
}

