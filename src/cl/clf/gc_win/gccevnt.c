/******************************************************************************
**
** Copyright (c) 1987, 2009 Ingres Corporation
**
******************************************************************************/
/*
** Name: GCCEVNT.C
** Description:
**	This module handles events that have been completed by the
**	protocol drivers in the comm server and is a sort of glue between
**	how gca handles synchronisity vs. how the protocol drivers handle it.
**
**	DESCRIPTION OF GCA, PROTOCOL DRIVER THREADS AND THREAD EVENTS
**	--------------------------------------------------------------
**	Protocol Driver Model
**
**	This model is based on how Windows TCP/IP was implemented.  For the
**	addition of new protocols, despite there being likely differences in
**	the Network CLI, it's probable that the same model can be used.
**
**	The overall scheme on NT for a driver is this: The driver runs
**	2 threads -- a synchronous listen thread, and an asynchronous
**	request thread.  The synchronous thread is continously started/ended
**	throughout the life of the GCC, whereas the other thread is started
**	once and runs continuously throught the life of GCC.
**
**	Following is a description of the model by function:
**
**	Initialization (GCwintcp_init) -- This function is called from
**	GCpinit (which in turn is called by GCC mainline).  It should
**	handle any protocol-specific initializations, create the
**	necessary events and mutex's for the driver, and finally kick
**	off the thread that handles asynchronous net I/O.
**	
**	Dispatcher (GCwintcp) -- This function dispatches GCC_OPEN,
**	GCC_LISTEN, GCC_SEND, GCC_CONNECT, GCC_RECEIVE and GCC_DISCONNECT
**	requests from GCC mainline to the protocol driver.  Messages for
**	all but GCC_OPEN and LISTEN requests are dispatched to the asynch
**	thread.  GCC_OPEN just runs once and simply calls the protocol's
**	open function.  GCC_LISTEN will cause the dispatcher to start a
** 	listen thread.
**
**	Open (GCwintcp_open) -- This function, called only once at startup,
**	establishes a "listen posture" for incoming connections and for
**	subsequent GCC_LISTEN requests.  For TCP/IP this involves creating
**	the listen socket, binding and issuing the listen call.
**
**	Listen Thread (GCwintcp_listen) -- This thread is started by the
** 	dispatcher on a GCC_LISTEN request.  It just waits for a connection
**	to come in on whatver its listen channel happens to be (i.e. in
**	TCP/IP, accept() blocks on the listen socket).  When a connection
**	comes in this thread returns (goes away) after setting the completion
**	event.  Presumably, GCC mainline reissues a listen after this occurs.
**
**	Async Thread (GCwintcp_async_thread) -- This thread is started in the
**	protocol driver's init routine and never returns until the server is
**	shut down.  It does the big chunk of the NET I/O work -- connects,
**	sends, receives and disconnects.  It takes requests out of it's
**	input Q, puts them into its in process Q, runs them asynchronously,
**	and finally puts them into the server completion Q when complete.
**
**	How The Events Work
**	
**	The Windows NT local GCA communication mechanism is built on native
**	NT Named Pipes.  Along with this is used a native NT asynchronous
**	mechanism using Events in gcexec() and gcsync().  This is all
**	fine and dandy except in GCC which requires that GCexec() drive
**	*both* its local GCA asynchronous I/O and Network I/O via the
**	protocol drivers.
**
**	Unfortunately, it seems not all network protocol asynchronous I/O
**	can be driven by the native NT Event mechanism: witness the 1st
**	implemented Network driver, WINTCP, which uses winsock-specific
**	calls (berkely sockets based) to drive asynch I/O via select().
**	Therefore, a "gluing" mechanism was devised whcih should be able
**	to bridge any async mechanism for a given protocol driver to 
**	the native NT mechanism used by GCexec().
**
**	The structure, THREAD_EVENT_LIST, provides the glue.  This structure
**	contains a list of THREAD_EVENT structres -- 1 for each network
**	protocol driver, and a completion Q of completed I/O requests.
**	The THREAD_EVENT structure contains an input Q for incoming requests,
**	and an in-process Q for requests currently being executed.
**
**	The sequence of events for asynchronous I/O is this:
**	    
**	1) Request from upper-layer GCC comes thru to the protocol driver's
**	   dispatch routine.  (Nearly all of GCC, including GCexec() runs in
**	   the primary thread.  The primary thread *must* handle all callbacks
**	   because Named Pipe Events won't be signaled properly if >1
**	   thread does I/O on them).
**	2) The dispatch routine puts the request on the incoming Q of its
**	   THREAD_EVENT structure and then raises an Event which will signal
**	   the asynchronous request handler thread that there's an incoming
**	   request.
**	3) The asyncnhronous request handling thread picks off the request
**	   from its incoming Q and puts it into its in-process Q.
**	4) The thread processes the request until it completes (success or
**	   failure).
**	5) When complete, the asynchronous threads pulls the request off of
**	   its in-process Q and puts it into the global Completion Q.  It
**	   then signals a GCC completion Event.
**	6) GCexec() will wake from a GCC completion Event, pull the request
**	   off of the completion Q, and drive the callback.
** History:
**	05-nov-93 (edg)
**	    created.
**	19-nov-93 (edg)
**	    Modified GCget_incoming_requests to initialize pcb state.
**	    Added function GCcomplete_request() .
**	    Added tracing.
**      06-Feb-96 (fanra01)
**          Changed extern GCCL_trace to GLOBALREF.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      06-Aug-2009 (Bruce Lunsford) Sir 122426
**          Remove mutexing around calls to GCC service completion routine
**          as it is no longer necessary, since GCx servers are single-
**	    threaded anyway and I/O completions are invoked 1 at a time
**	    by the OS for a thread ...removes calls
**          to GCwaitCompletion + GCrestart. Should improve peformance.
**          Convert GCC completion queue mutex to a critical section
**          to improve performance (less overhead).
*/
#include <compat.h>
#include <ex.h>
#include <gcccl.h>
#include <gc.h>
#include <me.h>
#include <nm.h>
#include <st.h>
#include <pc.h>
#include <qu.h>
#include <tr.h>
#include "gcclocal.h"
#include "gclocal.h"


/******************************************************************************
**  Forward and/or External function references.
******************************************************************************/
GLOBALREF i4 GCCL_trace;
GLOBALREF CRITICAL_SECTION      GccCompleteQCritSect;
# define GCTRACE(n) if ( GCCL_trace >= n ) (void)TRdisplay




/*
** Name: GCc_callback_driver
** Description:
**	This routine is called whenever GCexec has been signaled that an
**	event has completed from a protocol driver.  It takes the request
**	off of the completed request q and drives the callback.
** History:
**	05-nov-93 (edg)
**	    created.
**	10-jan-94 (edg)
**	    rearrangement of global references to header.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Convert GCC completion queue mutex to a critical section
**	    to improve performance (less overhead).  Also add trace
**	    to know when/if called.
*/
VOID
GCc_callback_driver()
{
    QUEUE *q;
    GCTRACE(5)("GCc_callback_driver: Entered\n");

    /*
    ** Lock the Q of completed requests
    */
    EnterCriticalSection( &GccCompleteQCritSect );

    q = IIGCc_proto_threads.completed_head.q_next;
    while ( q != &IIGCc_proto_threads.completed_head )
    {
        REQUEST_Q *rq = (REQUEST_Q *)q;
	GCC_P_PLIST *parm_list = rq->plist;

	q = q->q_next;

	/*
	** Remove request from q and free memory
	*/
	MEfree( (PTR)QUremove( (QUEUE *)rq ) );

	/*
	** drive completion.
	*/
	(*parm_list->compl_exit)( parm_list->compl_id );

    }

    /*
    ** We're done, unlock the Q.
    */
    LeaveCriticalSection( &GccCompleteQCritSect );
}

/*
** Name: GCget_incoming_reqs
** Description:
**	Takes requests off of a thread's input q and puts them into the
**	in-process q.  returns the number of req's placed in the process
**	q.
** History:
**	05-nov-93 (edg)
**	    created.
**	19-nov-93 (edg)
**	    Now initializes the appropriate state for the request in the
**	    PCB.  The PCB must have a PCB_STATE struct as 1st member.
*/
int
GCget_incoming_reqs( THREAD_EVENTS *tptr, HANDLE hMutexInQ )
{
   int no_reqs = 0;
   QUEUE *q;

   WaitForSingleObject( hMutexInQ, INFINITE );
   q = tptr->incoming_head.q_next;
   while( q != &tptr->incoming_head )
   {
       QUEUE *nq = q->q_next;
       REQUEST_Q *rq = (REQUEST_Q *)q;
       GCC_P_PLIST *parm_list = rq->plist;
       PCB_STATE *pcb_state = (PCB_STATE *)parm_list->pcb;

       GCTRACE(3)("GCget_incoming: PARM = %x, PCB = %x\n",
       		   parm_list, pcb_state );

       if ( pcb_state != NULL )
       {
           switch( parm_list->function_invoked )
	   {
	       case GCC_CONNECT:
	           pcb_state->conn = INITIAL;
	    	   break;
	       case GCC_SEND:
	           pcb_state->send = INITIAL;
	    	   break;
	       case GCC_RECEIVE:
	           pcb_state->recv = INITIAL;
	    	   break;
	       case GCC_DISCONNECT:
	           pcb_state->disc = INITIAL;
	    	   break;
	       default:
	    	   break;
	    }
 	}

       (void)QUremove( q );
       (void)QUinsert( q, &tptr->process_head );

       q = nq;
       no_reqs++;
   }

   ReleaseMutex( hMutexInQ );

   return no_reqs;
}


/*
** Name: GCcomplete_request
** Description:
**	Takes a pointer to QUEUE, puts it in the completed Q and raises
**	the completion event for GCexec().
** History:
**	19-nov-93 (edg)
**	    created.
**      06-Aug-2009 (Bruce Lunsford) Sir 122426
**          Convert GCC completion queue mutex to a critical section
**          to improve performance (less overhead).
*/
VOID
GCcomplete_request( QUEUE *q )
{
    /*
    ** Remove request from its in-process Q.
    */
    (void)QUremove( q );

    /*
    ** Add request to completed Q
    */
    EnterCriticalSection( &GccCompleteQCritSect );
    QUinsert( q, &IIGCc_proto_threads.completed_head );
    LeaveCriticalSection( &GccCompleteQCritSect );

    if ( !SetEvent( hAsyncEvents[GCC_COMPLETE] ) )
    {
       GCTRACE(1)("GCcomplete_request: SetEvent Error = %d\n",
       		   GetLastError() );
    }

    return;
}
