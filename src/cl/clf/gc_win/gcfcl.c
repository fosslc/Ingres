/******************************************************************************
** Copyright (c) 1987, 2010 Ingres Corporation
******************************************************************************/

# include       <compat.h>
# include	<tr.h>
# include	<pc.h>
# include	<gcccl.h>
# include	<gc.h>
# include	<st.h>
# include	"gclocal.h"


/******************************************************************************
**  Forward and/or External function references.
******************************************************************************/
FUNC_EXTERN BOOL GVosvers(char *OSVersionString);

/******************************************************************************
**  Defines of other constants.
******************************************************************************/

GLOBALREF i4 GC_trace;
# define GCTRACE( n )	if( GC_trace >= n ) (void)TRdisplay

/******************************************************************************
**  Definition of static variables and forward static functions.
******************************************************************************/
GLOBALREF VOID (*GCevent_func[NUM_EVENTS])();


/******************************************************************************
**
**  Name: GCFCL.C - CL routines used by the GCF communication servers
**
**  Description:
**
**      This module contains all of the system-specific routines associated
**      with the GCF communication servers.  There are two intended purposes of
**      this CL module.  The first is to isolate the CL client from
**      details of either a protocol itself or its impelementation in a
**      specific system environment (and possibly by a specific vendor).
**      Protocol access is offered in a generic way which relieves the mainline
*       client of the necessity of knowledge of protocol detail.  This also
**      makes possible the implementation of additional protocols without
**      requiring modifications to mainline code.  The second is to provide
**      access to the system-specific environment at appropriate points in the
**      execution control sequence to allow the functional abstraction of the
**      Communication Server to be implemented in a reasonable way in any
**      system environment.
**
**      The server execution management routine GCexec() is responsible for
**      controlling normal execution of the communication server.  It is
**      invoked after completion of initialization, and retains control for the
**      entire duration of server execution.  It returns only when the server
**      is to shut down.  It performs the functions necessary in a particular
**      environment to allow and handle the occurrence of asynchronous external
**      events.
**
**      The shutdown signal routine GCshut() is invoked to request server
**      shutdown.  Shutdown may be indicated as graceful or immediate.  As a
**      result of this invocation, the server execution management routine will
**      complete execution and return to its invoker.
**
**  Intended Uses:
**
**      These routines are intended for use exclusively by the the GCF
**      communication servers, designated GCC and GCN.  These routines are not
**      externally available.  They are used by GCC to control execution of the
**      communication server in a particular environment and to perform network
**      communication functions in support of client/server connections.
**
**
**  Assumptions:
**      Server CL:
**      GCexec() - The Communication Server execution mangagement routine;
**      GCshut() - The Communication Server shutdown routine;
**
**      The interface associated with each is described below.
**
** History:
**      12-Sep-95 (fanra01)
**          Extracted data for DLLs on Windows NT.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	28-jun-2001 (somsa01)
**	    For OS version, use GVosvers().
**
******************************************************************************/



/******************************************************************************
** Name: GCexec() - drive GCA events
**
** Description:
**      Drive I/O completion for async GCA.  Return when GCshut is called.
**
**      GCexec drives the event handling mechanism in several GCA
**      applications.  The application typically posts a few initial
**      asynchronous GCA CL requests (and in the case of the Comm
**      Server, a few initial GCC CL requests), and then passes control
**      to GCexec.  GCexec then allows asynchronous completion of
**      events to occur, calling the callback routines listed for the
**      asynchronous requests; these callback routines typically post
**      more asynchronous requests.  This continues until one of the
**      callback routines calls GCshut.
**
**      If the OS provides the event handling mechanism, GCexec
**      presumably needs only to enable asynchronous callbacks and then
**      block until GCshut is called.  If the CL must implement the
**      event handling mechanism, then GCexec is the entry point to its
**      main loop.
**
**      The event mechanism under GCexec must not interrupt one
**      callback routine to run another: once a callback routine is
**      called, the event mechanism must wait until the call returns
**      before driving another callback.  That is, callback routines
**      run atomically.
**
** Side effects:
**      All events driven.
**
** History:
**	28-Oct-93 (edg)
**	    Added Tracing support via GCTRACE( nat )
**	05-nov-93 (edg)
**	    Must now handle a 3rd type of aynchronous event for GCC:
**	    completion of protocol driver I/O.
**	19-nov-93 (edg)
**	    Now use a function pointer array, GCevent_func, to make function
**	    call when GCexec is awakened by an event.  This should eliminate
**	    having to link in unessary stuff in some servers (i.e gcn
**	    needing to link in protocol driver stuff from gcc );
**	02-feb-1996 (canor01)
**	    Add SyncEvent to hAsyncEvents array to signal synchronous IO
**	    completion.
**	09-may-1996 (canor01)
**	    Call the GCevent_func[SHUTDOWN] function on receipt of the
**	    SHUTDOWN event (if the function exists).
**	26-Jul-1998 (rajus01)
**	    Added new TIMEOUT event check for GClisten() requests.
**	31-mar-1999 (somsa01)
**	    Properly set the number of events when passed to
**	    WaitForMultipleObjectsEx() on Windows 95.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Include of gcccl.h added since now needed by gclocal.h.
**
******************************************************************************/
VOID
GCexec()
{
	DWORD dwObj;
	DWORD oops;
	SECURITY_ATTRIBUTES sa;
	char VersionString[256];
	INT Is_w95 = 0, eventnum;

	iimksec (&sa);
	sa.bInheritHandle = FALSE;

	GVosvers(VersionString);
	Is_w95 = (STstrindex(VersionString, "Microsoft Windows 9",
			     0, FALSE) != NULL) ? TRUE : FALSE;

	eventnum = NUM_EVENTS;
	if (Is_w95)
	    eventnum--;

	while(1) {

		GCTRACE(4)( "GCexec - Waiting for something to happen...\n" );

		dwObj = WaitForMultipleObjectsEx(eventnum,
		                                 hAsyncEvents,
		                                 FALSE,
		                                 INFINITE,
		                                 TRUE);

		switch(dwObj) {

		case WAIT_OBJECT_0 + LISTEN:
		        (*GCevent_func[LISTEN])();
			break;

		case WAIT_OBJECT_0 + SHUTDOWN:
			if ( GCevent_func[SHUTDOWN] != NULL )
		        	(*GCevent_func[SHUTDOWN])();
			return;

		case WAIT_OBJECT_0 + GCC_COMPLETE:
		        (*GCevent_func[GCC_COMPLETE])();
			break;

		case WAIT_OBJECT_0 + SYNC_EVENT:
			GCTRACE(3)( "GCexec - Sync Event Completed\n" );
			break;

		case WAIT_OBJECT_0 + TIMEOUT:
		        (*GCevent_func[TIMEOUT])();
			break;

		case WAIT_IO_COMPLETION:
			GCTRACE(3)( "GCexec - I/O Completed\n" );
			break;

		case WAIT_ABANDONED_0 + LISTEN:
		case WAIT_ABANDONED_0 + SHUTDOWN:
		case WAIT_ABANDONED_0 + GCC_COMPLETE:
			GCTRACE(3)( "GCexec - Wait Abandoned\n" );
			break;

		default:
			oops = GetLastError();
			GCTRACE(2)( "GCexec - Unexpected ret = %d\n", oops );
			break;
		}
	}
}

/******************************************************************************
** Name: GCshut() - stop GCexec
**
** Description:
**      Terminate GCexec.
**
**      GCshut causes GCexec to return.  When an application uses
**      GCexec to drive callbacks of asynchronous GCA CL and GCC CL
**      requests, it terminates asynchronous activity by calling
**      GCshut.  When control returns to GCexec, it itself returns.
**      The application then presumably exits.
**
**      If the application calls GCshut before passing control to
**      GCexec, GCexec should return immediately.  This can occur if
**      the completion of asynchronous requests occurs before the
**      application calls GCexec.
**
******************************************************************************/
VOID
GCshut()
{
	SetEvent(hAsyncEvents[SHUTDOWN]);
}
