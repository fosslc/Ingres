/*
** Copyright (c) 1987, 2010 Ingres Corporation
**
**
*/
/**
**  Name: GCWINSCK.C
**  Description:
**	The main driver for all protocols that go thru the winsock API.
**	The protocols accessed via the winsock API will all basically use
**	the same set of calls and mechanisms.  There are some slight
**	differences between adresses and such which get addressed in
**	specific routines for the protocol
** 
**  History: 
**	02-dec-93 (edg)
**	    Original -- basically revamped from the original gcwintcp.c
**	    implementation so that more than one protocol can use the
**	    same set of winsock code.  Currently supports both wintcp and
**	    novell spx.
**	06-dec-93 (edg)
**	    Got rid of gcwinsck.h and put GCC_WINSOCK_DRIVER into the
**	    THREAD_EVENTS structure.
**	    Fixed potential bug -- in async thread connect should be using
**	    ws_Addr from pcb, not wsd.
**      15-jul-95 (emmag)
**          Use a NULL Discretionary Access Control List (DACL) for
**          security, to give implicit access to everyone.
**      18-jul-95 (reijo01)
**          Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**              the proper values.
**      09-aug-95 (lawst01)
**          Added function call 'iimksec' to function GCwinsock to
**          initialize the SECURITY_ATTRIBUTES structure used by
**          CreateThread.
**      17-aug-95 (chech02)
**          Added cast of intermediate value to nat prior to assigning to
**          u_i2.
**      18-aug-95 (chech02,lawst01)
**          In function GCwinsock_async_thread - process is put to sleep for
**          1 msec if there are no executable requests in the Incoming queue.
**      25-aug-95 (chech02)
**          Initialize the pcb->ws_addr field - to get rid of occasional GPF
**          Set correct state in the pcb control block when the session is
**          disconected
**      12-Sep-95 (fanra01)
**          Extracted data for DLLs on Windows NT.
**	18-dec-1995 (canor01)
**	    For NVLSPX, print IPX network name and node name into the 
**	    errlog.log on a listen, not the socket port number.
**	02-apr-1996 (canor01)
**	    Do not process requests unless the socket is available.
**	    Instead of trying to read/write the socket and looping
**	    until it is available, gather the sockets of interest and
**	    call select() on them, then do the read/write only on the
**	    ones that were ready.  Loop at that point and gather new
**	    requests.
**	30-apr-1996 (tutto01)
**	    Added support for DECnet.  Fixed calls to GetLastError, which were
**	    not being done immediately following the calls they were diagnosing.
**	09-may-1996 (canor01)
**	    Do not call GCwinsock_exit() as a function registered with
**	    PCatexit().  Instead, call it when GCexec() processes the
**	    SHUTDOWN event.
**	28-mar-1997 (canor01)
**	    Check the in_shutdown flag at logical points in the
**	    processing loop to prevent the process from exiting while
**	    we are still in a system call (causing mysterious ACCVIOs).
**	14-jun-1997 (canor01)
**	    Move check for in_shutdown flag to within the main loop to
**	    catch shutdown before socket calls.
**	24-jun-1997 (canor01)
**	    Terminate protocol helper threads at process shutdown.
**	26-jun-1997 (somsa01 & lunbr01)
**	    In GCwinsock_async_thread(), moved testing of !rval before testing
**	    of wsd->block_mode when testing comeback from recv(). With the
**	    DECnet protocol, this was causing the processing of zero-length
**	    data or data that should not be looked at when the socket was not
**	    connected. In general, in the case of block mode protocols, the
**	    test for !rval and wsd->block_mode accomplish the same - we are
**	    done processing data from the client.
**	01-Apr-98 ( rajus01 )
**	    Added GLOBALREF is_comm_svr and set it to TRUE in GCC_OPEN for 
**	    Communication Server..
**	31-dec-1998 (mcgem01)
**	    Avoid passing a null pointer to the TerminateThread function
**	    when shutting down the listen thread for our three supported
**	    Winsock protocols by moving the TerminateThread call to a point
**	    where we know we have a valid Tptr.
**	16-mar-1999 (somsa01)
**	    In GCwinsock(), initialize generic_status to OK. This is to help
**	    GCC tracing to not cause SEGVs in the GCC server.
**	17-may-1999 (somsa01)
**	    In GCwinsock_listen(), initialize the listen node_id to NULL.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**
**      02-Nov-1999 (fanra01)
**          Close the resource leak for server side net.
**	02-may-2000 (somsa01)
**	    In GCwinsock_open(), do not set the SO_REUSEADDR option on the
**	    created socket, since each server needs a unique listen port.
**	28-jun-2000 (somsa01)
**	    Protocols other than win_tcp can only have one listen address
**	    each. Therefore, for those protocols turn on the SO_REUSEADDR
**	    option on the created socket. Also, since other protocols other
**	    than win_tcp can go through here, subport has been moved back
**	    to the protocol-specific implementation file.
**	04-jun-2001 (lunbr01)   Bug 104952
**	    Force complete with error any outstanding RECEIVEs or SENDs
**          when a DISCONNECT is processed on the same session. This
**          prevents trying to select() on an invalid socket and a poll loop
**          in iigcb (bridge) on the outbound session after a disconnect.
**          This also prevents a possible access vio on the pcb.
**	16-mar-2001 (somsa01)
**	    Added code to GCwinsock_listen() to set up the node id of the
**	    partner.
**      03-Apr-2002 (lunbr01)
**          Bug 106589, INGNET 90: Performance of send/receive is approx
**          4 times slower in OpenIngres than in Ingres 6.4 on Windows.
**          Fixed by sending internal dummy message to wake up async
**          thread that is sitting in select(), rather than waiting the
**          10msec minimum for the select() to timeout.
**      11-dec-2002 (loera01)  SIR 109237
**          Use getsockname() and getpeername() in GCwinsock_listen() to
**          determine whether the accepted peer is local or remote.
**      17-Apr-2003 (fanra01)
**          Bug 110091
**          Disable send/receive performance enhancement for Win9x and
**          WinMe platforms.
**	04-nov-2003 (somsa01)
**	    Changed Winsock version to 2.2.
**	09-feb-2004 (somsa01)
**	    In GCwinsock_open(), when working with an instance identifier as
**	    a port address, make sure we start with only the first two
**	    characters. The subport is calculated from within the driver's
**	    addr_func() function. Also, in GCwinsock_async_thread(),
**	    make sure we check for pcbInternal_listen being valid before
**	    doing anything with it.
**	20-feb-2004 (somsa01)
**	    Updates to account for multiple listen ports per protocol.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      14-Aug-2004 (lunbr01)
**          Bug 110337, Problem INGNET 125
**          Performance is much slower if more than 1 session is started,
**          even if 2nd session is inactive.  The fix is to not skip
**          the wake-up select() to the async thread for GCC_RECEIVE
**          requests (ie, remove a minor part of fix for bug 106589).
**      15-Feb-2005 (lunbr01)  Bug 113903, Problem EDJDBC 97
**          Do not clear the 3rd char of the input port when passed in
**          as XX# format.  Some callers, such as the JDBC server, pass in
**          an explicit symbolic port to use.
**      16-Aug-2005 (loera01) Bug 115050, Problem INGNET 179
**          In GCwinsock_async_thread(), added a Sleep(0) before the 
**          GCC_CONNECT operation so that the winsock thread gives up its 
**          timeslice to GCwinsock_listen(), in case a listen needs to be 
**          queued.  Added an exception fd mask in the connect select()
**          call so that errors would be reported right away instead of
**          pausing the thread for ten seconds.  In GCwinsock_listen(), 
**          don't retrieve the hostname via gethostbyaddr() if the connection 
**          is local.  Instead, copy the local hostname from GChostname().
**      26-Jan-06 (loera01) Bug 115671
**          Added GCWINTCP_log_rem_host to allow invocation of gethostbyaddr()
**          to be disabled if set to FALSE.
**      22-Jan-2007 (lunbr01) Bug 117523
**          Corrupted (invalid) incoming data with large length prefix 
**          can crash caller (eg, GCC or GCD).  Add check to ensure
**          caller's buffer length is not exceeded (ie, avoid buffer overrun).
**      06-Feb-2007 (Ralph Loen) SIR 117590
**          In GCwinsock_listen(), removed gethostbyaddr() and set the node_id
**          field in the GCC listen parms to NULL.
**      11-Oct-2007 (rajus01) Bug 118003; SD issue: 116789
**	    When DAS server is mistakenly configured to use same
**	    integer or alpha numeric ports for both flavors of tcp_ip 
**	    protocols (tcp_ip & wintcp) the driver loops while trying to 
**	    bind to a unique port causing DAS server to go into loop 
**	    upon a new JDBC connection filling the errlog.log with 
**	    E_GC4810_TL_FSM_STATE error message until the disk 
**	    becomes full.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**          Remove mutexing around calls to GCC service completion routine
**          as it is no longer necessary, since GCx servers are single-
**	    threaded anyway and I/O completions are invoked 1 at a time
**	    by the OS for a thread ...removes calls to
**	    GCwaitCompletion + GCrestart. Should improve peformance.
**	    Convert GCC completion queue mutex to a critical section
**	    to improve performance (less overhead).
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime. This also involves using errno instead of
**	    GetLastError() for failures.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Variable name of protocol-specific init_func was changed to
**	    prot_init_func.  Also sync up code with later winsock2 driver
**	    regarding initialization and cleanup of the winsock facility.
**	    In GCwinsock_exit(), add PCT entry as an input parm which is
**	    now available, which allows termination of specified protocol
**	    only, making the code cleaner.  Also change return value from
**	    VOID to STATUS since caller expects it.
*/

/* FD_SETSIZE is the maximum number of sockets that can be
** passed to a select() call.  This must be defined before
** winsock.h is included
*/
# define	FD_SETSIZE 1024

# include	<compat.h>
# include 	<winsock.h>
# include 	<wsipx.h>
# include	<er.h>
# include	<cm.h>
# include	<cs.h>
# include	<me.h>
# include	<qu.h>
# include	<pc.h>
# include	<lo.h>
# include	<pm.h>
# include	<tr.h>
# include	<st.h>
# include	<nm.h>
# include	<gcccl.h>
# include	<gc.h>
# include	"gclocal.h"
# include	"gcclocal.h"

typedef unsigned char *u_caddr_t;
typedef char   *caddr_t;

#define GC_STACK_SIZE   0x8000

/*
* *  Forward and/or External function references.
*/

STATUS 		GCwinsock_init(GCC_PCE * pptr);
STATUS          GCwinsock(i4 function_code, GCC_P_PLIST * parm_list);
STATUS   	GCwinsock_exit(GCC_PCE * pptr);
VOID            GCwinsock_async_thread(void *parms);
STATUS          GCwinsock_open(GCC_P_PLIST *parm_list);
VOID            GCwinsock_listen(void *parms);
extern VOID	GCcomplete_request( QUEUE * );
THREAD_EVENTS   *GCwinsock_get_thread( char * );
VOID            GCcomplete_conn(HANDLE event);

/*
** Globals: only need 1 listen socket descriptor
*/
GLOBALREF i4   WSAStartup_called;
static bool Winsock_exit_set = FALSE;
static bool is_win9x = FALSE;

/*
** Glabal references to externals: these handles are used by modules outside
** of this one which also need to synchronize  with the threads created here.
**
** Threads are all synchronized with the main struture: IIGCc_proto_threads --
** the main Event structure for protocol driver threads.
*/
GLOBALREF HANDLE hAsyncEvents[];
GLOBALREF CRITICAL_SECTION GccCompleteQCritSect;
GLOBALREF THREAD_EVENT_LIST IIGCc_proto_threads;
GLOBALREF BOOL is_comm_svr;

/*
** The PCB is allocated on listen or connection request.  The format is
** internal and specific to each protocol driver.
*/
typedef struct _PCB
{
	PCB_STATE	   state;

	SOCKET             sd;

	i4		   no_snd;  	    /* bytes already sent/rcvd */
	i4		   tot_snd;	    /* total to send/rcv */
	i4		   no_rcv;  	    /* bytes already sent/rcvd */
	i4		   tot_rcv;	    /* total to send/rcv */
	char		   *snd_bptr;	    /* utility buffer pointer */
	char		   *rcv_bptr;	    /* utility buffer pointer */
	char		   recv_msg_len[2]; /* used in GCC_RECEIVE to read
					    ** in the 1st 2 bytes which is
					    ** a length of message field.
					    */
	struct sockaddr	   *ws_addr;
} PCB;

/*
**  Defines of other constants.
*/
GLOBALREF i4 GCWINSOCK_trace;
# ifdef xDEBUG
# define GCTRACE( n )  if ( GCWINSOCK_trace >= n )(void)GCtrace
# else
# define GCTRACE( n )  if ( GCWINSOCK_trace >= n )(void)TRdisplay
# endif

GLOBALREF BOOL GCWINTCP_log_rem_host;

/*
**  Definition of static variables and forward static functions.
*/

/* Information required to catch association requests */
static int      in_shutdown = 0;

static WSADATA  startup_data;

static i4 GCWINSOCK_timeout;

static i4 GCWINSOCK_select = 10;


/*
** Name: GCwinsock_init
** Description:
**	WINSOCK inititialization function.  This routine is called from
**	GCpinit() -- the routine GCC calls to initialize protocol drivers.
**
**	This function does initialization specific to the protocol:
**	    Creates Events and Mutex's for the protocol
**	    Finds and saves a pointer to it's input event Q.
**	    Fires up the thread which will do asynch I/O
** History:
**	05-Nov-93 (edg)
**	    created.
**	28-sep-1995 (canor01)
**	    Set default socket timeout value to 10 seconds.  Allow 
**	    override by II_WINSOCK_TIMEOUT or "!.winsock.timeout"
**  17-Apr-2003 (fanra01)
**      Add initialization of is_win9x to disable wake up send on 
**      Win 9x and Win Me platforms.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Move call of WSAStartup() from GCwinsock_open() to here
**	    since this routine and GCwinsock_term() are symmetric in
**	    that they are called the same # of times...once for each
**	    driver instance...at startup vs shutdown.  This makes it
**	    easier to track # of driver instances using winsock, which
**	    can only be started up the first time and only cleaned up
**	    when the last driver instance exits.
**	    WSAStartup_called field is now a "use count" rather than a
**	    true/false boolean and is used to know, not only when to
**	    "startup" winsock, but also when to "cleanup".  Improve tracing.
*/
STATUS
GCwinsock_init(GCC_PCE * pptr)
{
	char           *ptr;
	int		tid;
	HANDLE		hThread;
	STATUS		status;
	GCC_WINSOCK_DRIVER *wsd;
	THREAD_EVENTS   *Tptr;
	char		*proto = pptr->pce_pid;
	WS_DRIVER	*driver = (WS_DRIVER *)pptr->pce_driver;
	SECURITY_ATTRIBUTES sa;
    OSVERSIONINFOEX osver;

    /*
    ** Should use GVverifyOSversion here but it doesn't have the options 
    ** required.
    ** Could use GVosvers but would require explicit string comparisons for 95
    ** and 98.
    */
    osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx( (LPOSVERSIONINFO)&osver ))
    {
        /*
        ** Platform Ids are
        ** VER_PLATFORM_WIN32s
        ** VER_PLATFORM_WIN32_WINDOWS
        **      4.0     = Win95
        **      4.x     = Win98 & WinMe
        ** VER_PLATFORM_WIN32_NT
        ** VER_PLATFORM_WIN32_CE
        */
        is_win9x = ((osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
            ((osver.dwMajorVersion == 4) && (osver.dwMinorVersion >= 0)));
    }
    else
    {
        osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if(GetVersionEx( (LPOSVERSIONINFO)&osver ))
        {
            is_win9x = ((osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
                ((osver.dwMajorVersion == 4) && (osver.dwMinorVersion >= 0)));
        }
        else
        {
            is_win9x = TRUE;
        }
    }

	iimksec (&sa);

	/*
	** Get trace variable
	*/
	NMgtAt( "II_WINSOCK_TRACE", &ptr );
	if ( !(ptr && *ptr) && PMget("!.winsock_trace_level", &ptr) != OK )
	{
	    GCWINSOCK_trace = 0;
	}
	else
	{
	    GCWINSOCK_trace = atoi( ptr );
	}
    
	GCTRACE(1)("GCwinsock_init %s: Entered.  winsock use_count=%d\n",
		    proto, WSAStartup_called);

	/*
	** NT March Beta and June Beta have different version #s that
	** work. So call WSAStartup with both if necessary.
	*/
	WSAStartup_called++;
	if ( WSAStartup_called <= 1 )
	{
	    WORD version = MAKEWORD(2,2);
	    int err = WSAStartup(version, &startup_data);
	    if (err != 0) 
		return FAIL;

	    /*
	    ** Confirm that the WinSock DLL supports 2.2.
	    ** Note that if the DLL supports versions greater
	    ** than 2.2 in addition to 2.2, it will still return
	    ** 2.2 in wVersion since that is the version we
	    ** requested.
	    */
	    if (LOBYTE(startup_data.wVersion) != 2 ||
		HIBYTE(startup_data.wVersion) != 2)
		return FAIL;
	}

	/*
	** Get timeout variable
	*/
	NMgtAt( "II_WINSOCK_TIMEOUT", &ptr );
	if ( !(ptr && *ptr) && PMget("!.winsock_timeout", &ptr) != OK )
	{
	    GCWINSOCK_timeout = 10;
	}
	else
	{
	    GCWINSOCK_timeout = atoi( ptr );
	}

	/*
	** Get select variable. This is the delay used when testing
	** which sockets are ready for read/write operations.  In
	** theory, if all sockets are busy, the thread will wait for
	** this length of time and not process any new messages that
	** may have been added to the queue.  However, setting this
	** to zero causes massive CPU utilization by iigcc as the
	** thread loops continuously, retrying the sockets over and
	** over.
	** The time is in microseconds.
	*/
	NMgtAt( "II_WINSOCK_SELECT", &ptr );
	if ( !(ptr && *ptr) )
	{
	    GCWINSOCK_select = 10;
	}
	else
	{
	    GCWINSOCK_select = atoi( ptr );
	}

	/*
	** Get pointer to Winsock proto's control entry in table.
	*/
	if ( (Tptr = GCwinsock_get_thread( pptr->pce_pid )) == NULL )
	    return FAIL;
	wsd = &Tptr->wsd;

	/*
	** Now call the protocol-specific init routine.
	*/
	if ( (*driver->prot_init_func)( pptr, wsd ) != OK )
	    return FAIL;

	/*
	** Create MUTEX and EVENT for the input queue of this protocol
	** driver.
	*/
	if ( ( Tptr->hMutexThreadInQ = CreateMutex(&sa, FALSE, NULL) ) == NULL )
	{
	    return FAIL;
	}
	GCTRACE(3)( "GCwinsock_init: MutexInQ Handle = %d\n", Tptr->hMutexThreadInQ );
	if ( ( Tptr->hEventThreadInQ = CreateEvent(&sa, FALSE, FALSE, NULL) ) == NULL )
	{
	    CloseHandle( Tptr->hMutexThreadInQ );
	    return FAIL;
	}
	GCTRACE(3)( "GCwinsock_init: EventInQ Handle = %d\n", Tptr->hEventThreadInQ );
	if ( ( Tptr->hEventInternal_conn = CreateEvent(&sa, FALSE, FALSE, NULL) ) == NULL )
	{
	    CloseHandle( Tptr->hMutexThreadInQ );
	    CloseHandle( Tptr->hEventThreadInQ );
	    return FAIL;
	}
	GCTRACE(3)( "GCwinsock_init: EventInternal_conn Handle = %d\n", Tptr->hEventInternal_conn );
	
	/*
	** Finally we start the asynchronous I/O thread
	*/

	hThread = (HANDLE)_beginthreadex(&sa,
		       GC_STACK_SIZE,
		       (LPTHREAD_START_ROUTINE) GCwinsock_async_thread,
		       Tptr,
		       (unsigned long)NULL,
		       &tid);
	if (hThread) 
	{
		Tptr->hAIOThread = hThread;
	}
	else
	{
	    status = errno;
	    GCTRACE(1)("GCwinsock_init: Couldn't create thread errno = %d '%s'\n",
	    		status, strerror(status) );
	    return FAIL;
	}

	return OK;
}



/*
** Name: GCwinsock
** Description:
**	Main entry point for the window's NT winsock protocol driver.  This
** 	driver is essentially just a dispatcher -- it runs in the primary
**	GCC thread and mostly just Q's things to do to the constantly running
**	aynchronous request thread.  It may also start a listen thread if
**	it is a LISTEN request.
**
**	The following functions are handled:
**	    GCC_OPEN	- call GCwinsock_open
**	    GCC_LISTEN  - start listen thread
**	    GCC_SEND    - Q request for asynch thread
**	    GCC_RECEIVE - Q request for asynch thread
**	    GCC_CONNECT - Q request for asynch thread
**	    GCC_DISCONN - Q request for asynch thread
** History:
**	03-Nov-93 (edg)
**	    Original.
**	16-mar-1999 (somsa01)
**	    Initialize generic_status to OK.
**      03-Apr-2002 (lunbr01)
**          Bug 106589, INGNET 90: Performance of send/receive is approx
**          4 times slower in OpenIngres than in Ingres 6.4 on Windows.
**          Fixed by sending internal dummy message to wake up async
**          thread that is sitting in select(), rather than waiting the
**          10msec minimum for the select() to timeout. During initialization,
**          set up internal socket to send wakeup messages on.
**	20-feb-2004 (somsa01)
**	    Updated to handle multiple listen ports per protocol.
**      14-Aug-2004 (lunbr01)
**          Bug 110337, Problem INGNET 125
**          Performance is much slower if more than 1 session is started,
**          even if 2nd session is inactive.  The fix is to not skip
**          the wake-up select() to the async thread for GCC_RECEIVE
**          requests (ie, remove a minor part of fix for bug 106589).
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Remove mutexing around calls to GCA service completion routine
**	    as it is no longer necessary, since GCA is thread-safe...removes
**	    calls to GCwaitCompletion + GCrestart. Should improve peformance.
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime.
*/
STATUS
GCwinsock( i4 function_code, GCC_P_PLIST * parm_list)
{
	STATUS          generror = 0;
	STATUS          status = 0;
	int             tid;
	HANDLE          hThread;
    	REQUEST_Q 	*rq;
	GCC_LSN		*lsn;
	GCC_WINSOCK_DRIVER *wsd;
	THREAD_EVENTS   *Tptr;
	char		*proto = parm_list->pce->pce_pid;
	SECURITY_ATTRIBUTES sa;
        char            dummy_buffer='!';
        VOID            (*save_compl_exit)(PTR compl_id);
        GCC_P_PLIST     *conn_parm_list;
        char            LOOPBACK[]="127.0.0.1";

	iimksec (&sa);

	parm_list->generic_status = OK;
	CLEAR_ERR(&parm_list->system_status);
	/*
	** set error based on function code and determine whether we got a
	** valid function.
	*/
	switch (function_code) {
	case GCC_OPEN:
	        is_comm_svr = TRUE;
		GCTRACE(2) ("GCwinsock %s: Function = OPEN\n", proto );
		return GCwinsock_open( parm_list );

	case GCC_LISTEN:
		GCTRACE(2) ("GCwinsock %s: Function = LISTEN\n", proto );
		generror = GC_LISTEN_FAIL;
	        if ( (Tptr = GCwinsock_get_thread( 
			parm_list->pce->pce_pid )) == NULL )
	            return FAIL;

		/*
		** Use the proper listen port.
		*/
		lsn = Tptr->wsd.lsn;
		while ( lsn &&
			STcompare(parm_list->pce->pce_port, lsn->pce_port) != 0)
		{
		    lsn = lsn->lsn_next;
		}

		if (!lsn)
		    return FAIL;

		/* First time in, set up the internal connection:
		** 1. Call ourselves (recursive) to do a LISTEN w/ no
		**    callback (completion) routine.
		** 2. Call ourselves (recursive) to do a CONNECT w/ a special
		**    callback (completion) routine.  This logic connects to
		**    the LISTEN socket setup in step 1.  The completion rtn
		**    ensures we don't continue until the connect is complete.
                */
		if (Tptr->pcbInternal_listen == NULL) 
		{
		    save_compl_exit = parm_list->compl_exit;
		    parm_list->compl_exit = NULL;
		    Tptr->pcbInternal_listen = (char *)1;
		    status = GCwinsock (GCC_LISTEN, parm_list);
		    parm_list->compl_exit = save_compl_exit;
		    if (status != OK)
		        return FAIL;
		    /* Allocate a connect parmlist */
    		    if ( (conn_parm_list = (GCC_P_PLIST *)MEreqmem(0, sizeof(GCC_P_PLIST), TRUE, &status ) ) == NULL )
    		        return FAIL;
		    conn_parm_list->function_parms.connect.node_id = LOOPBACK;
		    conn_parm_list->function_parms.connect.port_id = 
		         parm_list->function_parms.listen.port_id;
		    GCTRACE(2) ("GCwinsock %s: Internal CONNECT node=%s port=%s\n",
	 	               proto,
	 	               conn_parm_list->function_parms.connect.node_id,
	 	               conn_parm_list->function_parms.connect.port_id );
		    conn_parm_list->pce = parm_list->pce;
		    conn_parm_list->function_invoked = (i4) GCC_CONNECT;
		    conn_parm_list->compl_exit = GCcomplete_conn;
		    conn_parm_list->compl_id   = (PTR)(&(Tptr->hEventInternal_conn));
	GCTRACE(1)("GCconn_connect: Tptr=%8x, plist->compl_id=%8x ->%8x,%8x,%8x hEvent=%8x\n",
             Tptr,
             conn_parm_list->compl_id, 
             *(conn_parm_list->compl_id), 
             (*conn_parm_list->compl_id), 
             (*(HANDLE *)conn_parm_list->compl_id), 
             Tptr->hEventInternal_conn);
		    status = GCwinsock (GCC_CONNECT, conn_parm_list);
		    GCTRACE(2) ("GCwinsock %s: Internal CONNECT return. status=%d\n", 
	 	               proto,
		               status );
		    if (status != OK)
		        return FAIL;
	            WaitForSingleObject( Tptr->hEventInternal_conn, INFINITE );
		    GCTRACE(2) ("GCwinsock %s: Internal CONNECT completed.  pcb=0x%8x\n",
	 	               proto,
	                       conn_parm_list->pcb);
		    Tptr->pcbInternal_conn = conn_parm_list->pcb;
		}

		/*
		** Spawn off a thread to handle the listen request
		*/
		hThread = (HANDLE)_beginthreadex(&sa,
			       GC_STACK_SIZE,
			       (LPTHREAD_START_ROUTINE) GCwinsock_listen,
			       parm_list,
			       (unsigned long)NULL,
			       &tid);
		if (hThread) 
		{
			lsn->hListenThread = hThread;
			return (OK);
		}
		status = errno;
		SETWIN32ERR(&parm_list->system_status, status, ER_create);
		goto err_exit;
		break;

	case GCC_CONNECT:
	    GCTRACE(2) ("GCwinsock %s: Function = CONNECT\n", proto );
            GCTRACE(2) ("GCwinsock %s: CONNECT node=%s port=%s\n",
                       proto,
                       parm_list->function_parms.connect.node_id,
                       parm_list->function_parms.connect.port_id );
	    generror = GC_CONNECT_FAIL;
	    break;
	case GCC_SEND:
	    GCTRACE(2) ("GCwinsock %s: Function = SEND\n", proto );
	    generror = GC_SEND_FAIL;
	    break;
	case GCC_RECEIVE:
	    GCTRACE(2) ("GCwinsock %s: Function = RECEIVE\n", proto );
	    generror = GC_RECEIVE_FAIL;
	    break;
	case GCC_DISCONNECT:
	    GCTRACE(2) ("GCwinsock %s: Function = DISCONNECT\n", proto );
	    generror = GC_DISCONNECT_FAIL;
	    break;

	default:
	    return FAIL;
	}			/* end switch */
	/*
	** CONNECT, SEND, RECEIVE and DISCONNECT are all dispatched
	** to the asynch thread.
   	** Now allocate a request q structure, stick it into incoming q,
   	** and raise the INCOMING REQUEST event.
   	*/
	if ( (Tptr = GCwinsock_get_thread( parm_list->pce->pce_pid )) == NULL )
	    return FAIL;
	wsd = &Tptr->wsd;

	GCTRACE(2)("GCwinsock %s: Q'ing request ...\n", proto);
    if ( (rq = (REQUEST_Q *)MEreqmem(0, sizeof(*rq), TRUE, &generror ) ) != NULL )
    {
        fd_set write_fds;
        rq->plist = parm_list;
	    /*
	    ** get mutex for completion Q
	    */
	    GCTRACE(2)("GCwinsock %s: wait for input mutex ...\n", proto);
	    WaitForSingleObject( Tptr->hMutexThreadInQ, INFINITE );

	    /*
	    ** Now insert the completed request into the inconming Q.
	    */
	    GCTRACE(2)("GCwinsock %s: inserting incoming req ...\n", proto);
	    QUinsert( &rq->req_q, &Tptr->incoming_head );

	    /*
	    ** release mutex for completion Q
	    */
	    GCTRACE(2)("GCwinsock %s: releasing Mutex incoming req ...\n", proto);
	    ReleaseMutex( Tptr->hMutexThreadInQ );

	    /*
	    ** raise the incoming event to wake up the thread.
	    */
	    GCTRACE(2)("GCwinsock %s: Setting event ...\n", proto);
	    if ( !SetEvent( Tptr->hEventThreadInQ ) )
	    {
	   	    status = GetLastError();
		    SETWIN32ERR(&parm_list->system_status, status, ER_sevent);
	   	    GCTRACE(1)("GCwinsock, SetEvent error = %d\n", 
		    		status );
	    }
        
	    if ((is_win9x == FALSE) &&
                (Tptr->pcbInternal_conn != NULL))
	    {
            /*
            ** Send a msg to wake up the thread in case it's waiting on a
            ** select for a receive.
            */
            int retcod;
            struct	timeval timeout;
         
            GCTRACE(2)("GCwinsock %s: Wakeup msg to async thread\n", proto);
            timeout.tv_sec  = 0;
            timeout.tv_usec = 0;
            FD_ZERO (&write_fds);
            FD_SET (((PCB *)Tptr->pcbInternal_conn)->sd, &write_fds);
            /*
            ** Changed timeout to timeval of 0 as NULL could cause a block
            */
            retcod = select (1, NULL, &write_fds, NULL, &timeout);
            switch(retcod)
            {
                case 0:
                    GCTRACE(1)("GCwinsock %s: Wakeup timed out\n", proto);
                    break;
                case 1:
                    send (((PCB *)Tptr->pcbInternal_conn)->sd, &dummy_buffer,1,0);
                    GCTRACE(2)("GCwinsock %s: Wakeup msg sent\n", proto);
                    break;
                case SOCKET_ERROR:
                default:
                    retcod = WSAGetLastError();
                    GCTRACE(1)("GCwinsock %s: Wakeup error %d\n", proto, retcod);
                    break;
            }
	    }

	    return OK;

   	}
    else
    {
        /*
		** MEreqmem failed
		*/
		SETWIN32ERR(&parm_list->system_status, generror, ER_alloc);
    }

	/*
	 * * Drive the completion routine on error
	 */
err_exit:
	parm_list->generic_status = generror;
	(*parm_list->compl_exit) (parm_list->compl_id);
	return OK;
}

/*
** Name: GCwinsock_listen
** Description:
**	This is the listen thread for winsock.  It runs a syncronous accept()
**	on the listen socket.  When complete it Q's the completetion to
**	the completed event Q.  When accept completes, this thread returns.
**	A new one will be created when GCwinsock() gets the request to
**	repost the listen.
** History:
**	05-nov-93 (edg)
**	    created.
**	17-may-1999 (somsa01)
**	    Initialize the listen node_id to NULL.
**      02-Nov-1999 (fanra01)
**          Close the thread handle for listen.
**	16-mar-2001 (somsa01)
**	    Added code to GCwinsock_listen() to set up the node id of the
**	    partner.
**      11-dec-2002 (loera01)  SIR 109237
**          Use getsockname() and getpeername() to determine whether the 
**          accepted peer is local or remote.
**	20-feb-2004 (somsa01)
**	    Updated to account for multiple listen ports per protocol.
**      16-Aug-2005 (loera01) Bug 115050, Problem INGNET 179
**          Don't retrieve the hostname via gethostbyaddr() if the 
**          connection is local.  Instead, copy the local hostname from 
**          GChostname().
**      06-Feb-2007 (Ralph Loen) SIR 117590
**          Removed gethostbyaddr() and set the node_id field in the GCC 
**          listen parms to NULL.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Convert GCC completion queue mutex to a critical section
**	    to improve performance (less overhead).
**	    Since _beginthreadex() is now used to start this thread,
**	    use _endthreadex() to end it.  Also eliminate unreferenced
**	    variable node_id.
**      
*/
VOID
GCwinsock_listen( VOID *parms )
{
    GCC_P_PLIST		*parm_list = (GCC_P_PLIST *)parms;
    int			i;
    SOCKET		sd;
    PCB			*pcb;
    int			status = OK;
    REQUEST_Q		*rq;
    GCC_LSN		*lsn = NULL;
    GCC_WINSOCK_DRIVER	*wsd;
    THREAD_EVENTS	*Tptr;
    struct sockaddr	peer;
    int			peer_len = sizeof(struct sockaddr);
    int			ipeer_len = sizeof(struct sockaddr_in);
    struct sockaddr_in 	*ipeer = (struct sockaddr_in *)&peer;
    i4                  sock_inaddr, peer_inaddr;

    if ( (Tptr = GCwinsock_get_thread( parm_list->pce->pce_pid )) == NULL )
    {
        status = FAIL;
	goto sys_err;
    }
    wsd = &Tptr->wsd;

    /*
    ** Use the proper listen port.
    */
    lsn = wsd->lsn;
    while (lsn && STcompare(parm_list->pce->pce_port, lsn->pce_port) != 0)
	lsn = lsn->lsn_next;

    if (!lsn)
	goto sys_err;

    /*
    ** Initialize the listen node_id to NULL.
    */
    parm_list->function_parms.listen.node_id = NULL;

    /* 
    ** Initialize peer connection status to zero (remote).
    */
    parm_list->options = 0;
    /*
    ** Accept connections on the listen socket
    */
    sd = accept( lsn->listen_sd, &peer, &peer_len );
    if (sd == INVALID_SOCKET) 
    {
	status = WSAGetLastError();
	goto sys_err;
    }

    /*
    ** Allocate Protcol Control Block specific to this driver and put into
    ** parm list
    */
    pcb = (PCB *) MEreqmem(0, sizeof(PCB), TRUE, &status);
    if (pcb == NULL) 
    {
	goto sys_err;
    }
    pcb->ws_addr = NULL;
    pcb->sd = sd;
    parm_list->pcb = (char *)pcb;

    /*
    ** Compare the IP addresses of the server and client.  If they
    ** match, the connection is local.
    */
    getsockname( sd, ipeer, &ipeer_len );
    MEcopy(&ipeer->sin_addr, sizeof(i4), &sock_inaddr);
    getpeername( sd, ipeer, &ipeer_len );
    MEcopy(&ipeer->sin_addr, sizeof(i4), &peer_inaddr);
    if (sock_inaddr == peer_inaddr)
       parm_list->options |= GCC_LOCAL_CONNECT;

    GCTRACE(1)( "GCwinsock_listen, Connection is %s\n", parm_list->options & 
        GCC_LOCAL_CONNECT ? "local" : "remote" );

    parm_list->function_parms.listen.node_id = NULL;

    i = 1;
    (void) setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (caddr_t)&i, sizeof(i));

    /*
    ** Set socket blocking status.  0=blocking, 1=non blocking.  Non blocking
    ** caused WSAEMSGSIZE error using the DECnet protocol.  Need to investigate.
    */
    i = 1;
    if (wsd->sock_fam == AF_DECnet)  i = 0;
    if ( ioctlsocket( sd, FIONBIO, &i ) == SOCKET_ERROR )
    {
	status = WSAGetLastError();
	goto sys_err;
    }

sys_err:
    if ((lsn != NULL) &&
        (lsn->hListenThread != NULL))
    {
        CloseHandle(lsn->hListenThread);
        lsn->hListenThread = NULL;
    }

    if (status != OK)
    {
        SETWIN32ERR(&parm_list->system_status, status, ER_ioctl);
	parm_list->generic_status = GC_LISTEN_FAIL;
    }

    if ((PCB *)Tptr->pcbInternal_listen == (PCB *)1)  /* if internal listen */
    {
        (PCB *)Tptr->pcbInternal_listen = pcb;        /*   save its PCB     */
	_endthreadex(0);
        return;                                       /*   exit             */
    }

    /*
    ** Now allocate a request q structure, stick it into complete q, and
    ** raise the GCC_COMPLETE event.
    */
    if ( (rq = (REQUEST_Q *)MEreqmem(0, sizeof(*rq), TRUE, NULL ) ) != NULL )
    {
        rq->plist = parm_list;
	/*
	** Get critical section for completion Q.
	*/
	EnterCriticalSection( &GccCompleteQCritSect );

	/*
	** Now insert the completed request into the completed Q.
	*/
	QUinsert( &rq->req_q, &IIGCc_proto_threads.completed_head );

	/*
	** Exit/leave critical section for completion Q.
	*/
	LeaveCriticalSection( &GccCompleteQCritSect );

	/*
	** raise the completion event to wake up GCexec.
	*/
	if ( !SetEvent( hAsyncEvents[GCC_COMPLETE] ) )
	{
	   /*
	   ** ruh roh.  We're screwed if this event can't be signaled.
	   */
	   status = GetLastError();
	   GCTRACE(1)("GCwinsock_listen, SetEvent error = %d\n", status );
	}

    }
    else
    {
        /*
	** ruh-roh.  MEreqmem failed.  Selious tlouble.  Not sure what to
	** do about it at this point since if it failed we can't notify
	** the completion routine.  For now, just return (exit thread)
	** which will probably have the effect of blocking all incoming
	** connections.
	*/
    }
    _endthreadex(0);
}



/*
** Name: GCwinsock_open
** Description:
**	Open the listen socket for WINSOCK.   Called from GCwinsock().  This
**	routine should only be called once at server startup.
** History:
**	04-nov-93 (edg)
**	    created.
**	02-may-2000 (somsa01)
**	    Do not set the SO_REUSEADDR option on the created socket, since
**	    each server needs a unique listen port.
**	28-jun-2000 (somsa01)
**	    Protocols other than win_tcp can only have one listen address
**	    each. Therefore, for those protocols turn on the SO_REUSEADDR
**	    option on the created socket.
**	09-feb-2004 (somsa01)
**	    When working with an instance identifier as a port address, make
**	    sure we start with only the first two characters. The subport is
**	    calculated from within the driver's addr_func() function.
**	20-feb-2004 (somsa01)
**	    Updated to account for multiple listen ports per protocol.
**      15-Feb-2005 (lunbr01)  Bug 113903, Problem EDJDBC 97
**          Do not clear the 3rd char of the input port when passed in
**          as XX# format.  Some callers, such as the JDBC server, pass in
**          an explicit symbolic port to use.  Also, for clarity, use
**          the "open", not "connect" port_id (didn't actually cause a
**          problem since these are union structs with port_id at same
**          location in each structure). Also update pce port so it 
**          displays subport in startup message ("Network open complete...").
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Remove mutexing around calls to GCA service completion routine
**	    as it is no longer necessary, since GCA is thread-safe...removes
**	    calls to GCwaitCompletion + GCrestart. Should improve peformance.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Move call of WSAStartup() from this routine to GCwinsock_init()
**	    to better keep track of when to cleanup winsock.  Added a trace
**	    at entry.
*/
STATUS
GCwinsock_open( GCC_P_PLIST *parm_list )
{
	int turn_on = 1; /* Used to indicate address reuse option. */
	STATUS	status;
	GCC_LSN *lsn, *lsn_ptr;
        GCC_WINSOCK_DRIVER *wsd;
	char               *proto = parm_list->pce->pce_pid;
	WS_DRIVER *driver = (WS_DRIVER *)parm_list->pce->pce_driver;
	THREAD_EVENTS *Tptr;
	char *port  = parm_list->function_parms.open.port_id;
	char port_id[GCC_L_PORT];

	parm_list->function_parms.open.lsn_port = NULL;

	GCTRACE(1)("GCwinsock_open %s: Entered.\n", proto);

        if ( (Tptr = GCwinsock_get_thread( parm_list->pce->pce_pid )) == NULL )
        {
            status = FAIL;
	    parm_list->generic_status = GC_OPEN_FAIL;
	    goto err_exit;
        }
	wsd = &Tptr->wsd;

	/*
	** Create the GCC_LSN structure for this listen port.
	*/
	lsn_ptr = wsd->lsn;
	lsn = (GCC_LSN *)MEreqmem(0, sizeof(GCC_LSN), TRUE, NULL);
	if (!lsn)
	{
	    parm_list->generic_status = GC_OPEN_FAIL;
	    status = FAIL;
	    goto err_exit;
	}

	lsn->ws_addr = (struct sockaddr *)MEreqmem( 0, wsd->addr_len,
						    TRUE, NULL );
	if (!lsn->ws_addr)
	{
	    parm_list->generic_status = GC_OPEN_FAIL;
	    status = FAIL;
	    goto err_exit;
	}

	lsn_ptr = wsd->lsn;
	if (!lsn_ptr)
	    wsd->lsn = lsn;
	else
	{
	    while (lsn_ptr->lsn_next)
		lsn_ptr = lsn_ptr->lsn_next;
	    lsn_ptr->lsn_next = lsn;
	}

	/*
	** create the socket.
	*/
	if ( (lsn->listen_sd = 
	      socket(wsd->sock_fam, wsd->sock_type, wsd->sock_proto) ) 
			== INVALID_SOCKET) 
	{
		status = WSAGetLastError();
		parm_list->generic_status = GC_OPEN_FAIL;
		SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		goto err_exit;
	}

	if (STcompare(port, "II"))
	{
	    STcopy(port, port_id);
	}
	else
	{
	    char    *value;

	    NMgtAt("II_INSTALLATION", &value);

	    if (value && *value)
		STcopy(value, port_id);
	    else if (*parm_list->pce->pce_port != EOS)
		STcopy(parm_list->pce->pce_port, port_id);
	    else
		STcopy(port, port_id);
	}

	for (;;)
	{
	    /*
	    ** Continue attempting to bind to a unique port id until we are
	    ** successfull.
	    */

	    /*
	    ** call the protocol-specific adressing function.
	    ** Pass NULL in for node which will be taken to mean the address is
	    ** local host.
	    */
	    status = (*driver->addr_func)(NULL, port_id, lsn->ws_addr,
					  lsn->port_name);
	    if ( status == FAIL )
	    {
		parm_list->generic_status = GC_OPEN_FAIL;
		SETWIN32ERR(&parm_list->system_status, WSAEADDRINUSE,
			    ER_bind);
		goto err_exit;
	    }

	    if (wsd->sock_fam != AF_INET)
	    {
		/* Set the socket options to allow address reuse. */
		if ((status = setsockopt(lsn->listen_sd, SOL_SOCKET,
					 SO_REUSEADDR, (u_caddr_t)&turn_on,
					 sizeof(turn_on))) != 0)
		{
		    status = WSAGetLastError();
		    parm_list->generic_status = GC_OPEN_FAIL;
		    SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		    goto err_exit;
		}
	    }

	    /*
	    ** Bind the socket and get ready to listen for a connection request.
	    */
	    if (bind(lsn->listen_sd, lsn->ws_addr, wsd->addr_len) != 0) 
	    {
		status = WSAGetLastError();
		/*	
		** Re-try for alphanumeric ports.
		*/
		if (status == WSAEADDRINUSE && !CMdigit(port) )
		{
		    status = OK;
		    STcopy(port, port_id);
		    continue;
		}
                /*
                ** Re-try for numeric ports with a "+" indicator.
                */

                if (status == WSAEADDRINUSE && CMdigit(port) &&
                     STindex( port, "+", 0 ))
                {
                    status = OK;
                    STcopy(port, port_id);
                    continue;
                }


		parm_list->generic_status = GC_OPEN_FAIL;
		SETWIN32ERR(&parm_list->system_status, status, ER_bind);
		goto err_exit;
	    }
	    else
		break;
	}

	STcopy(port_id, parm_list->pce->pce_port);
	STcopy(port_id, lsn->pce_port);

	/* Grab the name, in case we used a port number of 0 and got a
	** random address...
	*/
	if (getsockname(lsn->listen_sd, lsn->ws_addr, &wsd->addr_len) != 0) 
	{
		status = WSAGetLastError();
		parm_list->generic_status = GC_OPEN_FAIL;
		SETWIN32ERR(&parm_list->system_status, status, ER_getsockname);
		goto err_exit;
	}

	/* SPX */
	if ( wsd->sock_fam == AF_IPX )
	{
		i4 i, j;
		for ( i = 0, j = 0; i < 4; i++ )
		{
			i4 byt;
			byt = ((SOCKADDR_IPX*)(lsn->ws_addr))->sa_netnum[i] & 0xFF;
			if ( (byt & 0x0F) <= 9 )
				lsn->port_name[j+1] = (byt & 0x0F) + '0';
			else
				lsn->port_name[j+1] = (byt & 0x0F) - 10 + 'A';
			byt >>= 4;
			if ( byt <= 9 )
				lsn->port_name[j] = byt + '0';
			else
				lsn->port_name[j] = byt - 10 + 'A';
			j += 2;
		}
		lsn->port_name[j++] = '.';
		for ( i = 0; i < 6; i++ )
		{
			i4 byt;
			byt = ((SOCKADDR_IPX*)(lsn->ws_addr))->sa_nodenum[i] & 0xFF;
			if ( (byt & 0x0F) <= 9 )
				lsn->port_name[j+1] = (byt & 0x0F) + '0';
			else
				lsn->port_name[j+1] = (byt & 0x0F) - 10 + 'A';
			byt >>= 4;
			if ( byt <= 9 )
				lsn->port_name[j] = byt + '0';
			else
				lsn->port_name[j] = byt - 10 + 'A';
			j += 2;
		}
		lsn->port_name[j] = EOS;
	}

	/*
	** OK.Now listen...
	*/
	if (listen(lsn->listen_sd, SOMAXCONN) != 0) 
	{
		status = WSAGetLastError();
		parm_list->generic_status = GC_OPEN_FAIL;
		SETWIN32ERR(&parm_list->system_status, status, ER_listen);
		goto err_exit;
	} 
	else 
	{
		parm_list->generic_status = OK;
		STcopy(port_id, port);
		parm_list->function_parms.open.lsn_port = lsn->port_name;
	}

err_exit:
	/*
	** Should completion be driven here?
	*/
	(*parm_list->compl_exit) (parm_list->compl_id);
	return OK;
}


/*
** Name: GCwinsock_async_thread
** Description:
**	This thread handles all the asynchronous I/O for a protocol driver.
** 	It will be woken up when GCwinsock() places a request on it's input
**	Q.  Then, this thread will move the request from the input Q to its
**	processing Q and continue to process the request until complete.
**	When complete, the request is finally moved to the completion
**	Q.
** History:
**	04-Nov-93 (edg)
**	    Written.
**	28-sep-1995 (canor01)
**	    Check for timeout (return value of zero) on select().
**	    Use default timeout value instead of zero.
**	02-apr-1996 (canor01)
**	    Do not process requests unless the socket is available.
**	    Instead of trying to read/write the socket and looping
**	    until it is available, gather the sockets of interest and
**	    call select() on them, then do the read/write only on the
**	    ones that were ready.  Loop at that point and gather new
**	    requests.
**	26-jun-1997 (somsa01 & lunbr01)
**	    Moved testing of !rval before testing of wsd->block_mode when
**	    testing comeback from recv(). With the DECnet protocol, this was
**	    causing the processing of zero-length data or data that should
**	    not be looked at when the socket was not connected. In general,
**	    in the case of block mode protocols, the test for !rval and
**	    wsd->block_mode accomplish the same - we are done processing 
**	    recieved data from the client.
**	04-jun-2001 (lunbr01)
**	    Force complete with error any outstanding RECEIVEs or SENDs
**          when a DISCONNECT is processed on the same session. This
**          prevents trying to select() on an invalid socket and a poll loop
**          in iigcb (bridge) on the outbound session after a disconnect.
**          This also prevents a possible access vio on the pcb.
**	09-feb-2004 (somsa01)
**	    Make sure we check for pcbInternal_listen being valid before
**	    doing anything with it.
**      16-Aug-2005 (loera01) Bug 115050, Problem INGNET 179
**          Added a Sleep(0) before the GCC_CONNECT operation so that the
**          winsock thread gives up its timeslice to GCwinsock_listen(), in
**          case a listen needs to be queued.  Added an exception fd mask in 
**          the connect select() call so that errors would be reported right 
**          away instead of pausing the thread for ten seconds.
**      22-Jan-2007 (lunbr01) Bug 117523
**          Corrupted (invalid) incoming data with large length prefix 
**          can crash caller (eg, GCC or GCD).  Add check to ensure
**          caller's buffer length is not exceeded (ie, avoid buffer overrun).
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Since _beginthreadex() is now used to start this thread,
**	    use _endthreadex() to end it.
*/
VOID
GCwinsock_async_thread( VOID * parms)
{
	THREAD_EVENTS   *Tptr = (THREAD_EVENTS *)parms;
        GCC_WINSOCK_DRIVER *wsd = &Tptr->wsd;
	int             i;
	SOCKET          sd;
	int             rval;
	int             nbytes;
	int             status = OK;
	fd_set		write_fds;
	fd_set		read_fds;
	fd_set		check_fds;
	fd_set		except_fds;
	int		write_fds_cnt;
	int		read_fds_cnt;
	int		total_fds_cnt;
	struct	timeval tm;
	struct	timeval select_tm;
	DWORD		wait_stat;
	int		processing_requests = 0;
	int		pending_requests = 0;
	QUEUE		*q;
	WS_DRIVER	*driver = (WS_DRIVER *)wsd->pce_driver;
	char		port_name[36];
	char		*proto = Tptr->thread_name;
        char            dummy_recv_buffer;

	/*
	** set timer values so that select won't block.
	*/
	tm.tv_sec = GCWINSOCK_timeout;
	tm.tv_usec = 0;
	select_tm.tv_sec  = GCWINSOCK_select / 1000000;
	select_tm.tv_usec = GCWINSOCK_select % 1000000;

	GCTRACE(4)("WSOCK THREAD %s: started.\n", proto);
top:
	/*
	** Wait for a request to come in from the primary gcc thread....
	*/
	GCTRACE(4)("WSOCK THREAD %s: waiting for event ... \n", proto);
	wait_stat = WaitForSingleObject( Tptr->hEventThreadInQ, INFINITE );
	GCTRACE(3)("WSOCK THREAD %s: wait returned %d, handle = %d\n", proto,
		    wait_stat, Tptr->hEventThreadInQ );
	if (wait_stat == WAIT_FAILED)
	{
	    GCTRACE(1)("WSOCK THREAD %s: wait failed %d\n", proto,
	    		GetLastError() );
	}

	/*
	** Now get get the incoming requests and add up how many requests
	** we're processing.
	*/
	processing_requests = GCget_incoming_reqs( Tptr, Tptr->hMutexThreadInQ );
	GCTRACE(2)("WSOCK THREAD %s: Got %d new requests to process\n",proto,
		    processing_requests);

	/*
	** Loop until there's no more requests being processed.
	*/

	while( processing_requests )
	{
	    FD_ZERO(&write_fds);
	    FD_ZERO(&read_fds);
	    write_fds_cnt = 0;
	    read_fds_cnt = 0;

            pending_requests = 0;
	    /*
	    ** Now loop thru the inprocess request list.
	    */
	    for ( q = Tptr->process_head.q_next;
	    	  q != &Tptr->process_head;
		  q = q->q_next )
	    {
	        REQUEST_Q *rq = (REQUEST_Q *)q;
		GCC_P_PLIST *parm_list = rq->plist;
		PCB *pcb = (PCB *)parm_list->pcb;
		char *port  = parm_list->function_parms.connect.port_id;
		char *node  = parm_list->function_parms.connect.node_id;

	        if ( in_shutdown )
		{
		    _endthreadex(0);
		    return;
		}

		parm_list->generic_status = OK;
		CLEAR_ERR(&parm_list->system_status);

		switch (parm_list->function_invoked) 
		{
		    case GCC_SEND:
			if ( (pcb) && ( ++write_fds_cnt < FD_SETSIZE ) )
                            FD_SET( pcb->sd, &write_fds );
			break;
			
		    case GCC_RECEIVE:
                        GCTRACE(4)("WSOCK THREAD %s: process RECEIVE\n", proto);
			if ( (pcb) && ( ++read_fds_cnt < FD_SETSIZE ) )
                            FD_SET( pcb->sd, &read_fds );
			break;

		    /******************************************************
		    ** Handle CONNECT
		    *******************************************************/
		    case GCC_CONNECT:
               
                    /*
                    ** Give up this thread's time slice in case a listen
                    ** needs to be queued on the listen thread.
                    */
                    Sleep(0);   

		    GCTRACE(4)("WSOCK THREAD %s: process CONNECT\n", proto);
		    if ( pcb == NULL || pcb->state.conn == INITIAL )
		    {
		        /*
			** Allocate the protocol control block.
			*/
		  	pcb = (PCB *) MEreqmem(0, sizeof(PCB), TRUE, &status);
		    	parm_list->pcb = (char *)pcb;
			if (pcb == NULL) 
			{
			    SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
			    parm_list->generic_status = GC_CONNECT_FAIL;
			    break;
		   	}

			/*
			** Allocate sockaddr structure
			*/
			pcb->ws_addr = (struct sockaddr *) 
			    MEreqmem(0, wsd->addr_len, TRUE, &status );
			if (pcb->ws_addr == NULL) 
			{
			    SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
			    parm_list->generic_status = GC_CONNECT_FAIL;
			    break;
		   	}

		    	/*
			** Create the socket
			*/
			if ((sd = socket(wsd->sock_fam, wsd->sock_type, 
					 wsd->sock_proto)) 
				    == SOCKET_ERROR) 
			{
			    status = WSAGetLastError();
			    pcb->state.conn = COMPLETED;
			    SETWIN32ERR(&parm_list->system_status, status, ER_socket);
			    parm_list->generic_status = GC_CONNECT_FAIL;
			    break;
			}

			pcb->sd = sd;

			if ( (*driver->addr_func)
			   ( node, port, pcb->ws_addr, port_name ) == FAIL )
			{
			    status = WSAGetLastError();
			    pcb->state.conn = COMPLETED;
			    SETWIN32ERR(&parm_list->system_status, status, ER_socket);
			    parm_list->generic_status = GC_CONNECT_FAIL;
			    break;
			}

			/*
			** Set socket blocking status.  0=blocking, 1=non bl.
			** Non-blocking caused WSAEMSGSIZE errors using the
			** DECnet protocol.  Need to investigate.
			*/
			i = 1;
			if (wsd->sock_fam == AF_DECnet)  i = 0;
    			if ( ioctlsocket( sd, FIONBIO, &i ) == SOCKET_ERROR )
    			{
			        status = WSAGetLastError();
			        pcb->state.conn = COMPLETED;
			        SETWIN32ERR(&parm_list->system_status, status, ER_ioctl);
			        parm_list->generic_status = GC_CONNECT_FAIL;
			        break;
    			}

			rval = connect( sd, pcb->ws_addr, wsd->addr_len );
			if ( rval == SOCKET_ERROR ) status = WSAGetLastError();

		        GCTRACE(2)("WSOCK THREAD %s: connect rval = %d\n",
				    proto, rval);

			if ( rval == SOCKET_ERROR )
		        {
			    if ( status == WSAEWOULDBLOCK )
			    {
			        pcb->state.conn = COMPLETING;
			    }
			    else
			    {
		                GCTRACE(1)("WSOCK THREAD %s: connect error = %d\n",
				    proto, status);
			        pcb->state.conn = COMPLETED;
			        SETWIN32ERR(&parm_list->system_status, status, ER_socket);
			        parm_list->generic_status = GC_CONNECT_FAIL;
			        break;
			    }
			}
			else
			{
			   /*
			   ** Connect completed already!
			   */
			   pcb->state.conn = COMPLETED;
			}
			i = 1;
			(void) setsockopt( sd, 
					   SOL_SOCKET, 
					   SO_KEEPALIVE, 
					   (u_caddr_t) &i, 
					   sizeof(i));
		    } /* end if state is initial */
		    else
		    {
		        /*
		        ** State is completing.  Use select() -- when socket
			** is writeable the connect has completed.
		        */
			FD_ZERO( &check_fds );
			FD_SET( pcb->sd, &check_fds );
			FD_ZERO( &except_fds );
			FD_SET( pcb->sd, &except_fds );

			rval = select( (int)2, NULL, &check_fds, &except_fds, &tm );
			if ( rval  == SOCKET_ERROR ) status = WSAGetLastError();

		        GCTRACE(4)("WSOCK THREAD %s: connect select = %d\n",
				    proto, rval );

			if ( rval == SOCKET_ERROR )
			{
			    if ( status != WSAEINPROGRESS && status !=
                                 WSAEWOULDBLOCK )
			    {
			        pcb->state.conn = COMPLETED;
			        SETWIN32ERR(&parm_list->system_status, 
                                    status, ER_socket);
			        parm_list->generic_status = GC_CONNECT_FAIL;
			        break;
			    }
			}
			else if ( rval  > 0 )
			{
                            DWORD optval;
                            int optlen = sizeof(DWORD);

                            if (FD_ISSET (pcb->sd, &except_fds))
                            {
                                if (getsockopt(pcb->sd, SOL_SOCKET, SO_ERROR, 
                                    (char *)&optval,&optlen) != SOCKET_ERROR) 
                                status = optval;
		                if ( status != WSAEINPROGRESS && status !=
                                    WSAEWOULDBLOCK )
			        {
			            pcb->state.conn = COMPLETED;
			            SETWIN32ERR(&parm_list->system_status, 
                                        status, ER_socket);
			            parm_list->generic_status = GC_CONNECT_FAIL;
			            break;
                                }
                            }
                            else
                                 pcb->state.conn = COMPLETED;
			}
			else if ( rval == 0 )
			{
			    /* Connection timed out */
			    pcb->state.conn = COMPLETED;
			    parm_list->generic_status = GC_CONNECT_FAIL;
			    break;
			}
		    }
		    break;


		    /*******************************************************
		    ** Handle DISCONNECT
		    *******************************************************/
		    case GCC_DISCONNECT:

		    GCTRACE(4)("WSOCK THREAD %s: process DISCONNECT\n",
		    		proto);

		    if ( pcb ) 
		    {
                        QUEUE *q2;             
	                for ( q2 = Tptr->process_head.q_next;
	    	              q2 != &Tptr->process_head;
		              q2 = q2->q_next )
	                {
	                    REQUEST_Q *q2_rq = (REQUEST_Q *)q2;
		            GCC_P_PLIST *q2_parm_list = q2_rq->plist;
                            if ( ((PCB *)q2_parm_list->pcb == pcb) && (q2 != q) ) 
                                switch (q2_parm_list->function_invoked)
                                {
                                   case GCC_SEND:
                                     if (FD_ISSET (pcb->sd, &write_fds))
                                     {
                                       FD_CLR (pcb->sd, &write_fds);
                                       --write_fds_cnt;
                                     }
				     SETWIN32ERR(&q2_parm_list->system_status, 
				                 status, ER_socket);
				     q2_parm_list->generic_status = GC_SEND_FAIL;
                                     q2_parm_list->pcb = NULL;
                                     break;
                                   case GCC_RECEIVE:
                                     if (FD_ISSET (pcb->sd, &read_fds))
                                     {
                                       FD_CLR (pcb->sd, &read_fds);
                                       --read_fds_cnt;
                                     }
				     SETWIN32ERR(&q2_parm_list->system_status, 
				                 status, ER_socket);
				     q2_parm_list->generic_status = GC_RECEIVE_FAIL;
                                     q2_parm_list->pcb = NULL;
                                     break;
                                }  /* end switch on function_invoked */
                        } /* end for each request */
			(void) closesocket(pcb->sd);
			if ( pcb->ws_addr )
			{
			    MEfree( (PTR)pcb->ws_addr );
				pcb->ws_addr= NULL;
			}
			MEfree( (PTR)pcb);
		    }
		    parm_list->pcb = NULL;
		    break;

		} /* end switch */

	    } /* end for process q loop */
	    if ( in_shutdown )
	    {
		_endthreadex(0);
		return;
	    }
        
	    GCTRACE(4)("WSOCK THREAD %s: read_fds_cnt = %d\n", proto, read_fds_cnt );
	    if (read_fds_cnt > 0)
	    { 
	        read_fds_cnt++;
		if (Tptr->pcbInternal_listen)
		    FD_SET (((PCB *)Tptr->pcbInternal_listen)->sd, &read_fds);
	    }
        
	    total_fds_cnt = write_fds_cnt + read_fds_cnt;
            GCTRACE(4)("WSOCK THREAD %s: total_fds_cnt = %d\n", proto, total_fds_cnt );
	    if ( total_fds_cnt > 0 )
	    {
                GCTRACE(4)("WSOCK THREAD %s: select ... timeout=%d.%d\n",
                           proto, select_tm.tv_sec, select_tm.tv_usec);
		rval = select( (int)total_fds_cnt, 
				&read_fds, &write_fds, NULL, &select_tm );

		GCTRACE(4)("WSOCK THREAD %s: select = %d\n", proto, rval );

		/* First take care of the internal socket, if selected. 
		** This means there is a new input request.  If this was
		** the only thing selected, then we'll decrement rval (to 0)
		** so we don't waste time looking for activity on the 
		** requests already being processed.
		*/
		if ( rval > 0  && Tptr->pcbInternal_listen)
		{
	            if (FD_ISSET (((PCB *)Tptr->pcbInternal_listen)->sd, &read_fds))
		    {
			recv(((PCB *)Tptr->pcbInternal_listen)->sd, &dummy_recv_buffer, 1, 0);
		        rval--;
		    }
		}

		if ( rval > 0 )
		{
	    	    /*
	    	    ** Now loop thru the inprocess request list.
	    	    */
	    	    for ( q = Tptr->process_head.q_next;
	    	  	  q != &Tptr->process_head;
		  	  q = q->q_next )
	    	    {
	        	REQUEST_Q *rq = (REQUEST_Q *)q;
			GCC_P_PLIST *parm_list = rq->plist;
			PCB *pcb = (PCB *)parm_list->pcb;
			char *port  = parm_list->function_parms.connect.port_id;
			char *node  = parm_list->function_parms.connect.node_id;

                        if (pcb)
			if ( ( parm_list->function_invoked == GCC_SEND &&
			       FD_ISSET( pcb->sd, &write_fds ) ) ||
			     ( parm_list->function_invoked == GCC_RECEIVE &&
			       FD_ISSET( pcb->sd, &read_fds ) ) )
			switch (parm_list->function_invoked) 
			{
			    /***************************************************
			    ** Handle SEND
			    ***************************************************/
			    case GCC_SEND:
			    GCTRACE(4)("WSOCK THREAD %s: process SEND\n",proto);
	
			    if ( pcb->state.send == INITIAL )
			    {
			        u_i2 tmp_i2;
				u_char *cptr;
	
				if ( wsd->block_mode )
				{
			            pcb->snd_bptr = parm_list->buffer_ptr;
			            pcb->tot_snd = parm_list->buffer_lng;
				}
				else
				{
			            /*
				    ** handle 1st 2 bytes which are a msg 
				    ** length field.
				    */
			            pcb->snd_bptr = parm_list->buffer_ptr - 2;
			            tmp_i2 = (u_i2)(pcb->tot_snd 
					   = (i4)(parm_list->buffer_lng + 2));
				    cptr = (u_char *)&tmp_i2;
				    pcb->snd_bptr[1] = cptr[1];
				    pcb->snd_bptr[0] = cptr[0];
				}
				pcb->no_snd = 0;
			    }
	
			    /*
			    ** do the send
			    */
			    nbytes = pcb->tot_snd - pcb->no_snd;
			    rval = send(pcb->sd, pcb->snd_bptr, nbytes, 0);
			    if (rval == SOCKET_ERROR) status = WSAGetLastError();
	
			    GCTRACE(2)(
             "WSOCK THREAD %s: try send %d, act sent %d bytes, tot bytes= %d\n",
					    proto, nbytes, rval, pcb->tot_snd );
	
			    if (rval == SOCKET_ERROR) 
			    {
				if ( status == WSAEWOULDBLOCK )
				{
			    	    GCTRACE(3)(
					"WSOCK THREAD %s: send WOULDBLOCK\n",
					proto );
				    pcb->state.send = COMPLETING;
				}
				else
				{
			    	    GCTRACE(1)(
			        	"WSOCK THREAD %s: send error %d \n", 
					proto,status );
				    pcb->state.send = COMPLETED;
				    SETWIN32ERR(&parm_list->system_status, 
						status, ER_socket);
				    parm_list->generic_status = GC_SEND_FAIL;
				}
			    }
			    else /* number of bytes sent returned in rval */
			    {
			        pcb->no_snd += rval;
				pcb->snd_bptr += rval;
				if ( pcb->no_snd == pcb->tot_snd )
				{
				    pcb->state.send = COMPLETED;
				}
				else
				{
				    pcb->state.send = COMPLETING;
				}
			    }
	
			    break;

	
			    /***************************************************
			    ** Handle RECEIVE
			    ***************************************************/
			    case GCC_RECEIVE:
	
			    GCTRACE(4)("WSOCK THREAD %s: process RECEIVE\n", proto);


			    if ( pcb->state.recv == INITIAL )
			    {
				if ( wsd->block_mode )
				{
			            pcb->rcv_bptr = parm_list->buffer_ptr;
			            pcb->tot_rcv = parm_list->buffer_lng;
				    pcb->state.recv = COMPLETING;
				}
				else
				{
			    	    /*
				    ** We first want to get the 2 bytes that 
				    ** tell us what the real message length is.
				    */
			            pcb->rcv_bptr = pcb->recv_msg_len;
			            pcb->tot_rcv = 2;
				    pcb->state.recv = RECV_MSG_LEN;
				}
				pcb->no_rcv = 0;
			    }

			    nbytes = pcb->tot_rcv - pcb->no_rcv;
			    rval = recv(pcb->sd, pcb->rcv_bptr, nbytes, 0);
			    if (rval == SOCKET_ERROR) status = WSAGetLastError();
			    GCTRACE(4)(
				"WSOCK THREAD %s: recv: want %d got %d\n",
			    	proto, nbytes, rval);
	
			    if (rval == SOCKET_ERROR) 
			    {
				if ( status == WSAEWOULDBLOCK )
				{
	                          pending_requests++;
				}
				else
				{
			    	   GCTRACE(1)(
					"WSOCK THREAD %s: recv error %d\n",
			    		proto, status );
				    pcb->state.recv = COMPLETED;
				    SETWIN32ERR(&parm_list->system_status, 
						status, ER_socket);
				    parm_list->generic_status = GC_RECEIVE_FAIL;
				}
			    }
			    else /* number of bytes sent returned in rval */
			    {
			        GCTRACE(2)(
				    "WSOCK THREAD %s: RECV'D: want %d got %d\n",
			    	    proto, nbytes, rval);

	                        /* the session has been disconnected */
	                        if ( ! rval )
	                        {
	                           pcb->tot_rcv = 0;
				   pcb->state.recv = COMPLETED;
				   parm_list->generic_status = GC_RECEIVE_FAIL;
	                           pending_requests++;
	                           break;
	                        }
	
				/*
				** if this is a block mode protocol, once the
				** recv completes successfully, we should have
				** recv'd all the bytes we're going to so we're
				** done.
				*/
				if ( wsd->block_mode )
				{
				    parm_list->buffer_lng = rval;
				    pcb->state.recv = COMPLETED;
				    break;
				} 
                    
			 	/*
				** This part only runs for non-block mode ....
				** ... gets the 2 byte length field 1st.
				*/
			        pcb->no_rcv += rval;
				pcb->rcv_bptr += rval;
				if ( pcb->no_rcv == pcb->tot_rcv )
				{
				    if ( pcb->state.recv == RECV_MSG_LEN )
				    {
				        /*
					** We've gotten the complete message 
					** length, now fill in some variables.
					** Ensure incoming msg will fit in buffer.
					*/
					pcb->tot_rcv =
					 ( (u_char)pcb->recv_msg_len[1] << 8 ) +
					 (u_char)pcb->recv_msg_len[0] - 2;
					if ( pcb->tot_rcv < 0 ||
					     pcb->tot_rcv > parm_list->buffer_lng )
					{
					    GCTRACE(1)(
						"WSOCK THREAD %s: recv error  - Invalid incoming message len = %d bytes, buffer = %d bytes\n",
				    		proto,
				    		pcb->tot_rcv,
				    		parm_list->buffer_lng );
					    pcb->state.recv = COMPLETED;
					    parm_list->generic_status = GC_RECEIVE_FAIL;
					    break;
					}
					parm_list->buffer_lng = pcb->tot_rcv;
					pcb->no_rcv = 0;
			        	pcb->rcv_bptr = parm_list->buffer_ptr;
					pcb->state.recv = COMPLETING;
				    }
				    else
				        pcb->state.recv = COMPLETED;
				}
			    }

			    break;
			}
		    }
		}
	    }

	    if ( in_shutdown )
	    {
		_endthreadex(0);
		return;
	    }

	    /*
	    ** Now go thru the inprocess Q and look for any requests that
	    ** have been completed.  This will be indicated by one of:
	    ** parm_list->pcb == NULL (bad connect or after disconnect) or
	    ** pcb->state == COMPLETED.
	    */

	    GCTRACE(4)("WSOCK THREAD %s: processing completed. . . \n",
	    		proto);

	    q = Tptr->process_head.q_next;
	    while( q != &Tptr->process_head )
	    {
	        REQUEST_Q *rq = (REQUEST_Q *)q;
	        GCC_P_PLIST *pl = rq->plist;
		PCB *pcb = (PCB *)pl->pcb;
		bool completed = FALSE;

		switch ( pl->function_invoked )
		{
		    case GCC_CONNECT:
		        if ( pcb == NULL || pcb->state.conn == COMPLETED )
			    completed = TRUE;
		        break;
		    case GCC_SEND:
		        if ( pcb == NULL || pcb->state.send == COMPLETED )
			    completed = TRUE;
		        break;
		    case GCC_RECEIVE:
		        if ( pcb == NULL || pcb->state.recv == COMPLETED )
			    completed = TRUE;
		        break;
		    case GCC_DISCONNECT:
		        if ( pcb == NULL || pcb->state.disc == COMPLETED )
			    completed = TRUE;
		        break;
		}

		if ( completed )
		{
		    QUEUE *nq = q->q_next;

	    	    GCTRACE(3)("WSOCK THREAD %s:  Complete! PCB = %x PARM = %x \n",
		    		proto, pcb, pl);

                    if (rq->plist->compl_exit == GCcomplete_conn)
                        GCcomplete_conn( q);
                    else
		    GCcomplete_request( q );

		    q = nq;
		    processing_requests--;

	    	    GCTRACE(3)("WSOCK THREAD %s: processed completed \n",
		    		proto);
	    	    GCTRACE(3)("                 : total now = %d \n",
		    		processing_requests);

		} /* end if req completed */
		else
		{
		    q = q->q_next;
		}

	    } /* end for -- look for complete req */

	    if ( in_shutdown )
	    {
		_endthreadex(0);
		return;
	    }

	    /*
	    ** Do a quick, non-blocking check to see if any new requests
	    ** came in during processing.
	    */
	    GCTRACE(4)("WSOCK THREAD %s: quick look for new reqs \n",
	    		proto);
	    if ( WaitForSingleObject( Tptr->hEventThreadInQ, 0 ) == WAIT_OBJECT_0 )
	    {
		processing_requests += 
		    GCget_incoming_reqs( Tptr, Tptr->hMutexThreadInQ ); 
	    }

            GCTRACE(2)("WSOCK THREAD %s: After quicklook, processing=%d, pending=%d\n",
                        proto, processing_requests, pending_requests);

	    GCTRACE(4)("WSOCK THREAD %s: process reqs now = %d\n",
	    		proto, processing_requests);

         if (processing_requests && pending_requests == processing_requests)
         {
            i4 Sleeptime = 1;
            GCTRACE(1)("WSOCK THREAD %s: Sleep! processing=%d, pending=%d\n",
                        proto, processing_requests, pending_requests);
            Sleep (Sleeptime);
         }

	} /* end while processing requests */

	if (in_shutdown)
	{
		_endthreadex(0);
		return;
	}

	/*
	** we're done for now, go back to the top and sleep.
	*/
	GCTRACE(3)("WSOCK THREAD %s: No more reqs, going back to top\n",
		    proto );
	goto top;
}



/*
** Name: GCwinsock_exit
** Description:
**
** History:
**	20-feb-2004 (somsa01)
**	    Updated to account for multiple listen ports per protocol.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add PCT entry as an input parm which is now available.
**	    Rather than terminating all valid protocols supported by
**	    this program on first call in (only), terminate the protocol
**	    in the input parm.  Also change return value from VOID to
**	    STATUS since caller expects it.  WSAStartup_called field is now
**	    a "use count" rather than a true/false boolean and is used
**	    to know, not only when to "startup" winsock, but also when
**	    to "cleanup".
*/
STATUS
GCwinsock_exit(GCC_PCE * pptr)
{
	int             i = 1;
	GCC_LSN *lsn;
	GCC_WINSOCK_DRIVER *wsd;
	THREAD_EVENTS *Tptr;
	STATUS status;
	char   *proto = pptr->pce_pid;

	GCTRACE(1)("GCwinsock_exit %s: Entered.  winsock use_count=%d\n",
		    proto, WSAStartup_called);

	in_shutdown = 1;

	/*
	** Get pointer to WinSock 2.2 protocol's control entry in table.
	*/
	Tptr = (THREAD_EVENTS *)pptr->pce_dcb;

	/*
	** Close listen sockets.
	*/
        {
            wsd = &Tptr->wsd;
	    lsn = wsd->lsn;
	    while (lsn)
	    {
		(void) setsockopt(lsn->listen_sd, SOL_SOCKET, SO_DONTLINGER,
				  (u_caddr_t)&i, sizeof(i));
		(void) closesocket(lsn->listen_sd);
		TerminateThread(lsn->hListenThread, 0);
		lsn = lsn->lsn_next;
	    }

            status = TerminateThread( Tptr->hAIOThread, 0 );
        }

	/*
	** Don't cleanup/shutdown winsock until no driver instance/thread
	** is using it.  Ie, last one out the door turns out the lights.
	*/
	WSAStartup_called--;
	if (WSAStartup_called == 0)
	    WSACleanup();

	return(OK);
}

/*
** Name: GCwinsock_get_thread
** Description:
**	Return pointer to appropriate entry in IIGCc_wsproto_tab.
**	This table is indexed the same as IIGCc_proto_threads.
*/
THREAD_EVENTS *
GCwinsock_get_thread( char *proto_name )
{
	int i;

	/*
	** Go thru the the protocol threads event list and find the index
	** of the winsock thread.  Set the Global Tptr for easy reference
	** to the event q's for this protocols thread.
	*/
	for ( i = 0; i < IIGCc_proto_threads.no_threads; i++ )
	{
	     THREAD_EVENTS *p = &IIGCc_proto_threads.thread[i];

	     if ( !STcompare( proto_name, p->thread_name ) )
	     {
	         return p;
	     }
	}

	/*
	** failed
	*/
   	return NULL;
}
/*
** Name: GCcomplete_conn
** Description:
**	Called when connect to internal socket during initialization is 
**      complete.  This routine must then set an event to wake up the 
**      requestor of the connect.
*/
VOID
GCcomplete_conn( QUEUE *q )
{
	REQUEST_Q *rq = (REQUEST_Q *)q;
	GCC_P_PLIST *parm_list = rq->plist;
	HANDLE hEventInternal_conn = (*(HANDLE *)parm_list->compl_id);
	GCTRACE(2)("GCcomplete_conn: plist->compl_id=%8x, hEvent=%8x\n",
             parm_list->compl_id, 
             hEventInternal_conn);
	MEfree( (PTR)QUremove( q ) ); 
	if ( !SetEvent( hEventInternal_conn ))
	{
	   GCTRACE(1)("GCcomplete_conn: SetEvent Error = %d\n",
	               GetLastError() );
	}
}
