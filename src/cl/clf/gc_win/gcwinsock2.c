/*
**  Copyright (c) 2010 Ingres Corporation
**
*/

/*
**  Name: GCWINSOCK2.C
**  Description:
**	The main driver for all protocols that go thru the WinSock 2.2 API.
**	The protocols accessed via the WinSock 2.2 API will all basically
**	use the same set of calls and mechanisms. There are some slight
**	differences between adresses and such which get addressed in
**	specific routines for the protocol.
** 
**  History: 
**	01-oct-2003 (somsa01)
**	    Created.
**	29-jan-2004 (somsa01)
**	    Changed SecureZeroMemory() to ZeroMemory(). SecureZeroMemory()
**	    is only available on Windows Server 2003.
**	09-feb-2004 (somsa01)
**	    Removed extra declaration of GCwinsock2_exit(). Also, in
**	    GCwinsock2_open(), when working with an instance identifier as
**	    a port address, make sure we start with only the first two
**	    characters. The subport is calculated from within the driver's
**	    addr_func() function.
**	20-feb-2004 (somsa01)
**	    Updates to account for multiple listen ports per protocol.
**	08-oct-2004 (somsa01)
**	    In GCwinsock2(), if we're going to queue a completion routine
**	    execution, make sure we return OK from this function. Also, in
**	    GCC_DISCONNECT, make sure the pcb is valid. Also, in
**	    GCwinsock2_worker_thread(), make sure lpPerHandleData is valid
**	    in OP_DISCONNECT.
**      15-Feb-2005 (lunbr01)  Bug 113903, Problem EDJDBC 97
**          Do not clear the 3rd char of the input port when passed in
**          as XX# format.  Some callers, such as the JDBC server, pass in
**          an explicit symbolic port to use.
**      23-feb-2005 (lunbr01)  bug 113686, prob INGNET 160
**          E_GC4810_TL_FSM_STATE errors occur occasionally in GCD due to 
**          SEND (or RECV) completion routine being called AFTER disconnect
**          compl rtn. Add "call completion routine" requests at END of
**          queue rather than beginning to force FIFO processing of
**          completions.  Each occurrence of E_GC4810 results in approx 
**          9K memory leak.
**      29-feb-2005 (lunbr01)  bug 113987, prob INGNET 165
**          Pending async RECEIVES occasionally do not post complete after
**          the socket is closed during disconnect, which resulted in the
**          disconnect looping forever waiting for the IO to complete.
**          Limit the number of times disconnect will wait.  Symptom
**          is that subsequent connect requests may hang.
**      14-jul-2004 (loera01) Bug 114852
**          For GCwinsock2_worker_thread() and GCwinsock2_AIO_Callback(),
**          in the OP_RECV_MSG_LEN operation, fetch the second byte of
**          the message length if only one byte was sent.
**      16-Aug-2005 (loera01) Bug 115050, Problem INGNET 179
**          In GCwinsock2_listen(), don't retrieve the hostname
**          via gethostbyaddr() if the connection is local.  Instead,
**          copy the local hostname from GChostname().
**      26-Jan-2006 (Ralph Loen) Bug 115671
**          Added GCTCPIP_log_rem_host to optionally disable invocation of 
**          gethostbyaddr() in GCwinsock2_listen() and 
**          GCwinsock2_worker_thread().
**      28-Aug-2006 (lunbr01) Sir 116548
**          Add IPv6 support.
**      22-Jan-2007 (lunbr01) Bug 117523
**          Corrupted (invalid) incoming data with large length prefix 
**          can crash caller (eg, GCC or GCD).  Add check to ensure
**          caller's buffer length is not exceeded (ie, avoid buffer overrun).
**      06-Feb-2007 (Ralph Loen) SIR 117590
**          In GCwinsock2_listen_complete(), removed getnameinfo() and set 
**          the node_id field in the GCC listen parms to NULL.
**      03-Aug-2007 (Ralph Loen) SIR 118898
**          Add GCdns_hostname().  In GCwinsock2_init(), moved WSA
**          startup logic into GCwinsock2_startup_WSA(), so that both
**          GCdns_hostname() and GCwinsock2_init() could check and startup
**          WSA. 
**      11-Oct-2007 (rajus01) Bug 118003; SD issue: 116789
**	    When DAS server is mistakenly configured to use same
**	    integer or alpha numeric ports for both flavors of tcp_ip
**	    protocols (tcp_ip & wintcp) the driver loops while trying to
**	    bind to a unique port causing DAS server to go into loop
**	    upon a new JDBC connection filling the errlog.log with
**	    E_GC4810_TL_FSM_STATE error message until the disk
**	    becomes full.
**      11-Sep-2008 (lunbr01)
**	    Fix compiler warning in LISTEN CreateThread (NULL -> 0).
**	    Add Trace (level=1) statements for error conditions that don't
**	    already have them plus enhance/standardize format.
**	    This will simplify diagnosis of problems in this driver.
**	    Change socket displays (traces) from %d to %p since on Windows
**	    they are UINT_PTR and will be 64-bits long on 64-bit systems;
**	    hence, changes display of socket id from decimal to hex.
**	26-Nov-2008 (Bruce Lunsford) Bug 121285
**	    Redo disconnect logic: issue closesocket() earlier and perform
**	    sleep loop waiting for async I/O to complete on the socket
**	    in a dedicated disconnect thread (for all sessions) rather
**	    than a winsock worker thread for each session, which can tie
**	    up all the worker threads preventing I/O to post complete.
**	    Accvio can then occur after disconnect "gives up" after 1
**	    second, frees pcb and other control blocks that pending receive
**	    tries to access when it runs.
**	    Also prevent occasional crash in tracing after WSARecv()s.
**	02-Mar-2009 (Bruce Lunsford) Bug 121742
**	    Fix crashes while connecting on Windows 2000 (and Win 9x/ME).
**	    Also connections handled by routine "normal_connect" were 
**	    failing with error=0 when tracing is on.  Other hangs would
**	    occur with tracing on, which was resolved by ensuring
**	    GCTRACE not called in between function and WSAGetLastError()
**	    ...because GCTRACE overlays the last error (with zero).
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Various performance enhancements and associated fixes:
**	    1. Convert GCC completion queue mutex to a critical section
**	    to improve performance (less overhead).
**	    2. Add support for alertable IO (AIO):
**	       ...use if II_WINSOCK2_CONCURRENT_THREADS environment variable
**	       ...    or winsock2_concurrent_threads config.dat variable
**	       ...       is set to zero.
**	    Alertable IO (rather than current default of IO Completion
**	    Port) may provide better performance for single-threaded GC*
**	    servers by avoiding extra thread context switching for all IO
**	    completions. On the other hand, performance may be worsened
**	    on some multi-CPU machines because other CPU processors will,
**	    for the most part, not be utilized.
**	    3. New variables (concurrent & worker threads) also allow
**	    overriding defaults for IO Completion Port # of concurrent
**	    and worker threads.
**	    4. If using IO completion port, add 1 less extra thread than
**	    we used to for the listens since it is not needed.
**	    5. Call completion routine directly (for performance) rather
**	    than "scheduling" it whenever running in caller's thread.
**	    6. Modify receive logic (modeled after Unix gccbs.c) to do a
**	    single receive instead of 2 per request (previously recv'd
**	    2-byte length prefix and then recv'd the actual data).
**	    Added variable II_WINSOCK2_RECV_BUFFER_SIZE and config.dat
**	    winsock2_recv_buffer_size; if not set, defaults to 
**	    RCV_PCB_BUFF_SZ (#define in PCB...8 or 32K most likely).
**	    If set to zero, no overflow area allocated and the old
**	    approach of 2 WSARecv()'s per GCC_RECEIVE will be used.
**	    Code was extracted to new function GCwinsock2_OP_RECV_complete()
**	    and is shared by AIO and worker thread logic.
**	    7. Sync up code in GCwinsock2_AIO_callback() with code in
**	    GCwinsock2_worker_thread() so that common code can be shared,
**	    such as new receive function.
**	    8. Collapse PER_HANDLE_DATA into PCB; it's small and there
**	    is no need to allocate and free it separately...complicates
**	    code unnecessarily.  Its info belongs in the PCB.  However,
**	    GetQueuedCompletionStatus() requires the address of a 
**	    completion key (which was the pointer to the PER_HANDLE_DATA)
**	    so just made it a local dummy field.
**	    9. Check for presence of lpOverlapped from IO completions in 
**	    worker threads...may be NULL under certain error conditions.
**	    10. Convert CreateThread() to _beginthreadex() which is
**	    recommended by Microsoft when using C runtime. This also
**	    involves using errno instead of GetLastError() for failures.
**	    Add logic to terminate and close the Disconnect Thread (missing
**	    from fix for bug 121285).
**	    11. Add async connect logic for AIO using Events.
**	28-Oct-2009 (Bruce Lunsford) Sir 122814
**	    Added variable II_WINSOCK2_NODELAY and config.dat winsock2_nodelay.
**	    If set to "ON", socket option TCP_NODELAY is set on all
**	    connections.  This "may" improve performance as it disables
**	    the TCP Naegle algorithm, which can cause delays of up to 200
**	    milliseconds between sends.  Unix/Linux already sets this option
**	    except on OS-threaded inbound sessions (ie, in DBMS).
**	19-Nov-2009 (Bruce Lunsford) Bug 122940 + Sir 122426
**	    Failed connects and listens were not detected when using
**	    recently added async alertable I/O logic (item 11 above for
**	    sir 122426).  Added WSAGetOverlappedResult() and WSAGetLastError()
**	    when connect/listen completed to check results.  This required
**	    adding the socket to associated AIO structures.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers. Use cl-defined GC_HOSTNAME_MAX
**	    instead of gl-defined GCC_L_NODE in GCdns_hostname().
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add support for net drivers like "tcp_ip" as local IPC.
**	    Variable name of protocol-specific init_func was changed to
**	    prot_init_func. In GCwinsock2_exit(), get dcb from input parm
**	    PCT entry rather than calling GCwinsock2_get_thread(); since
**	    GCwinsock2_get_thread() is no longer used, removed it.
**	    Remove calls to WSAGetLastError() after getaddrinfo() failure
**	    as it returns a zero (0) status, which is confusing.  Just use
**	    the return value directly from getaddrinfo(); also add call to
**	    gai_strerror() to get message text for error during GCC_OPEN.
**	    Support port zero (0) listens for local IPC.
**
**	    Embed one PER_IO_DATA structure for both send and receive
**	    into the PCB.  This eliminates the overhead of allocating and
**	    freeing them for each send and receive request.  For some
**	    situations, such as immediate returns that skip subsequent
**	    callback, the single embedded PER_IO_DATA is insufficient,
**	    so additional ones are allocated as needed.  Allocation is
**	    now done in new function GCwinsock2_get_piod.  Deallocation
**	    is handled by GCwinsock2_rel_piod.
**
**	    If an error occurs on a request immediately, rather than
**	    scheduling a callback to the caller's completion routine
**	    and returning OK, instead just return FAIL thereby allowing
**	    the caller to detect the error right away.
**
**	    Change default mechanism for scheduling completion requests
**	    to one that supports a multithreaded environment; doesn't
**	    affect Alertable IO (AIO) sends/receives because they call
**	    completion routine directly since they always run in the
**	    caller's thread.  The old completion scheduling mechanism
**	    was fairly high overhead (EnterCritSect, alloc memory, add
**	    entry to completion Q, LeaveCritSect, set event ... and the
**	    reverse on the receiving (caller's) side).  If multiple threads
**	    were waiting on a callback, the old mechanism had no way to
**	    ensure that the correct thread woke up from the event posting.
**	    Use Windows QueueUserAPC functionality to replace it; much
**	    simpler, may improve performance, and can target completions
**	    to the correct thread.  Provide config variable
**	    II_WINSOCK2_EVENT_Q (=ON) to resort back to original mechanism.
**
**	    Set system_status to ER_close if send or receive failed due to
**	    connection having been closed (BytesTransferred == 0).
**
**	    Shutdown function GCwinsock2_exit() now returns STATUS rather
**	    than VOID since caller expects it.
**	    Occasional crash in iigcc shutdown that was more common with
**	    II_GC_PROT=tcp_ip was resolved by adding a short sleep after
**	    each pending listen socket is closed to allow waiting worker
**	    thread to process before freeing associated resources.
**
**	    To improve turnaround time and support peeks (receives with
**	    zero timeout values), if PCT option PCT_SKIP_CALLBK_IF_DONE
**	    is set, then caller supports getting results immediately (if
**	    request has completed) rather than waiting to get results when
**	    the completion exit is called back, which then won't be done.
**	    Similar for sends.  Currently, only GCA CL (gcarw.c) can handle
**	    immediate results with no callback.
**
**	    Add support for timeouts on receives.  Timeouts are required for
**	    calls from GCA CL in selected circumstances.  Most receives have
**	    no timeout (timeout=-1 for INFINITE) and is not even used on
**	    calls from GCC TL.  A new field, timeout, was added to the end
**	    of the input parmlist.  For backward compatibility
**	    to GCC TL, which doesn't pass a timeout value in the parm_list,
**	    only check timeout if PCE option PCT_ENABLE_TIMEOUTS is on.
**	    Unfortunately, Winsock2 async receives provide no mechanism for
**	    timing them out or cancelling them (except by closing the socket,
**	    which shuts down the whole connection).  The only timeout options
**	    that winsock provides, such as setsockopt(SO_RCVTIMEO) only work
**	    with sync calls.  Even WSACancelBlockingCall() only works for
**	    sync(blocking) calls and is not supported in winsock2.
**	    For performance, switching to sync/blocking calls is not viable
**	    without spinning them off to separate threads.  Another option
**	    is to use select(), which DOES have a timeout value, but this
**	    would defeat the whole purpose of the switch to winsock2 to use
**	    a common wait mechanism (WaitForMultipleObjects) throughout
**	    Ingres central wait points (wintcp uses winsock1 with select(),
**	    tcp_ip uses winsock2 with WaitFor...Objects).  Due to lack of
**	    native winsock2 timeout support, a "smoke and mirrors" scheme
**	    was used.  Receives with timeouts are issued in the normal way
**	    but are directed to receive into an internal work/overflow
**	    buffer area. If the timeout period is reached before the
**	    receive completes, the caller's completion routine is called
**	    with GC_TIME_OUT error.  In the meantime, if a new receive
**	    comes in, the pending receive to the OS is redirected to the
**	    new receive request; it is too late to change the target area
**	    or length of the receive which is why it was initially
**	    directed to the internal overflow area.  When the IO completes,
**	    the data is copied from the overflow buffer to the requester's
**	    receive buffer.  There is a new state field in the PER_IO_DATA
**	    structure used to keep track of the receive flow since there
**	    are a number of event sequences that can occur and have
**	    slightly different actions/results.  One special case is
**	    receives with timeout=0, which is essentially a "peek".
**	    The immediate return logic is used in this case and no
**	    timer needs to be set.  However, the IO callback still occurs
**	    and must be handled in a manner similar to "timeout > 0" cases.
**	    Windows "waitable timer" object is used to handle the timer
**	    duties. When the timer expires and the thread becomes "alertable",
**	    the OS calls the completion routine associated with the timer,
**	    GCwinsock2_Timer_Callback(), which then cleans up and calls
**	    the GCC_RECEIVE request completion routine.
**
**	    Change default size of receive overflow buffer from #defined
**	    value (=32768) to the packet size in the protocol table (PCT).
**	    Change default IO processing mode from IO completion port to
**	    alertable IO (AIO), which generally performs better and is
**	    supported for tcp_ip as local IPC.
**
**	    Added "id" to pcb and traced it; modeled after gcb->id in
**	    gcarw.c.  This simplifies analyzing traces.
**	    Change PCB typedef to PCB2 so that Windows debugger will
**	    pick it up properly; the one in iilmgnbi.c was being picked
**	    up instead.
**
**	    Add 2 new external functions, GCwinsock2_save() and
**	    GCwinsock2_restore() to support GCsave() and GCrestore()
**	    (and are called by those routines).  These functions are
**	    only used when running under GCA CL (ie, as local IPC).
*/

/* FD_SETSIZE is the maximum number of sockets that can be
** passed to a select() call.  This must be defined before
** winsock2.h is included
*/
#define	FD_SETSIZE 1024
#define WINSOCK_VERSION2

#include <winsock2.h>
#include <ws2tcpip.h>  /* IPv6 extensions */
#include <wspiapi.h>   /* Dynamic link for pre-IPv6 Windows */
#include <mswsock.h>
#include <compat.h>
#include <wsipx.h>
#include <er.h>
#include <cm.h>
#include <cs.h>
#include <cv.h>
#include <me.h>
#include <qu.h>
#include <pc.h>
#include <lo.h>
#include <pm.h>
#include <tr.h>
#include <st.h>
#include <nm.h>
#include <gcccl.h>
#include <gc.h>
#include "gclocal.h"
#include "gcclocal.h"

typedef unsigned char *u_caddr_t;
typedef char   *caddr_t;

#define GC_STACK_SIZE   0x8000

/*
** Global references to externals: these handles are used by modules outside
** of this one which also need to synchronize  with the threads created here.
**
** Protocol threads are all synchronized with the main struture:
** IIGCc_proto_threads -- the main Event structure for protocol driver threads.
*/
GLOBALREF i4			WSAStartup_called;
GLOBALREF HANDLE		hAsyncEvents[];
GLOBALREF CRITICAL_SECTION	GccCompleteQCritSect;
GLOBALREF THREAD_EVENT_LIST	IIGCc_proto_threads;
GLOBALREF BOOL			is_comm_svr;
GLOBALREF i4			GCWINSOCK2_trace;


/*
** PER_IO_DATA is the control block for each winsock I/O request.
** The overlapped structure MUST BE FIRST and is the only part used by the OS.
*/
typedef struct _PCB2 PCB2;    /* Forward reference */

typedef struct _PER_IO_DATA
{
    WSAOVERLAPPED	Overlapped;
    SOCKET		listen_sd;
    SOCKET		client_sd;
    i4 			OperationType;
#define		OP_ACCEPT	0x010	/* AcceptEx */
#define		OP_CONNECT	0x020	/* ConnectEx */
#define		OP_RECV		0x080	/* WSARecv */
#define		OP_SEND		0x100	/* WSASend */
#define		OP_DISCONNECT	0x200	/* Disconnect */
    i4 			flags;
#define		PIOD_DYN_ALLOC	0x0001	/* Dynamically allocated */
    DWORD		ThreadId;	/* Requester's thread id - set if
					** completion in other thread */
    WSABUF		wbuf;
    bool		block_mode;
    u_i1		state;		/* State of request/PER_IO_DATA: */
#define	PIOD_ST_AVAIL		0x00	/* - available for use */
#define	PIOD_ST_IO_PENDING 	0x01	/* - IO pending (no timeout) */
#define	PIOD_ST_IO_TMO_PENDING	0x02	/* - IO pending with timeout */
#define	PIOD_ST_SKIP_CALLBK	0x03	/* - skip compl rtn callback */
#define	PIOD_ST_TIMED_OUT 	0x04	/* - request timed out       */
#define	PIOD_ST_REDIRECT  	0x05	/* - redirect IO compl to new request */
    GCC_P_PLIST		*parm_list;
    PCB2		*pcb;		/* Stored here in case parm_list gone*/
    HANDLE		hWaitableTimer; /* Handle to timer */
    GCC_LSN     	*lsn;              /* ptr to LSN struct for addr/port */
    struct addrinfo	*addrinfo_posted;  /* ptr to last posted lsn addr */
    int     		socket_ix_posted;  /* socket index last posted */
    char		AcceptExBuffer[(sizeof(SOCKADDR_STORAGE) + 16) * 2];
} PER_IO_DATA;


/*
**  The PCB2 is allocated on listen or connection request.  The format is
**  internal and specific to each protocol driver.
*/
typedef struct _PCB2
{
    unsigned char	id;		/* connecton counter (info only): */
					/*  - evens inbound, odds outbound*/
    SOCKET		sd;
    i4			session_state;      /* state of connection */
#define			SS_CONNECTED	0   /* - setting up or connected */
#define			SS_DISCONNECTED	1   /* - other side disconnected */
    i4			tot_snd;	    /* total to send */
    i4			tot_rcv;	    /* total to rcv */
    char		*snd_bptr;	    /* utility buffer pointer */
    char		*rcv_bptr;	    /* utility buffer pointer */
    PER_IO_DATA		snd_PerIoData;	    /* send IO control block */
    PER_IO_DATA		rcv_PerIoData;	    /* recv IO control block */
    PER_IO_DATA		*last_rcv_PerIoData;    /* last recv IO control block */
    struct addrinfo	*addrinfo_list;     /* ptr to addrinfo list for connect */
    struct addrinfo	*lpAddrinfo;        /* ptr to current addrinfo entry */
    GCC_ADDR		lcl_addr, rmt_addr;
    /*
    ** Number of async IOs still pending.  Disconnect (and associated
    ** freeing of resources) should not occur until this is zero.  Normally,
    ** the value is 0-2, but with AIO and immediate results (GCA CL only),
    ** the value can get very high in a busy system/application because
    ** the thread rarely becomes alertable.  To alleviate this (though no
    ** limit is known or has been encountered), when the value exceeds the
    ** (very arbitrary) soft limit, begin returning results in callback rather
    ** than immediately, until the number goes back below the soft limit.
    */
    volatile LONG	AsyncIoRunning;
#define ASYNC_IO_RUNNING_SOFT_LIMIT 20

    volatile LONG	DisconnectAsyncIoWaits;
    bool		skipped_callbk_flag; /* If TRUE, a callback was skipped */
    char		*b1;		    /* start of receive buffer */
    char		*b2;		    /* end   of receive buffer */
    /*
    ** Determine I/O buffer size.  For stream I/O (tcp/ip) there must be room
    ** for the max block size plus two bytes for the block length.  Note
    ** that overhead bytes are provided in the buffer declaration
    ** while the base buffer size is contained in the macro.
    */
# define		RCV_PCB_BUFF_SZ	    32768
    u_i2		rcv_buf_len;	    /* Length of overflow buffer */
    char		rcv_buffer[2];      /* Variable length recv overflow
					    ** area used in GCC_RECEIVE.
					    ** The 1st 2 bytes contain the
					    ** length of the message field.
					    */
} PCB2;

/*
**  The SAVE_DATA structure is used to save and restore information
**  for a connection across processes.  It contains the protocol-specific
**  information needed for the connection to enable GCsave/GCrestore in 
**  the GCA CL to work with this driver.
*/
typedef struct _SAVE_DATA
{
    unsigned char	id;		/* = pcb->id ... for info only */
    SOCKET		sd;		/* socket descriptor */
} SAVE_DATA;

/*
**  Defines of other constants.
*/
#ifdef xDEBUG
#define GCTRACE(n)	if (GCWINSOCK2_trace >= n) (void)GCtrace
#else
#define GCTRACE(n)	if (GCWINSOCK2_trace >= n) (void)TRdisplay
#endif

/*
**  Definition of static variables and forward static functions.
*/
static i4				GCwinsock2_use_count = 0;
static bool				GCwinsock2_shutting_down = FALSE;
static bool				is_win9x = FALSE;
static i4				GCWINSOCK2_timeout;
static bool				GCWINSOCK2_nodelay = FALSE;
static bool				GCWINSOCK2_event_q = FALSE;
static HANDLE				GCwinsock2CompletionPort = NULL;
static HANDLE				hGCwinsock2Process       = NULL;
static i4	 			GCWINSOCK2_recv_buffer_size = -1;
static QUEUE				DisconnectQ_in;
static QUEUE				DisconnectQ;
CRITICAL_SECTION			DisconnectCritSect;
static HANDLE				hDisconnectEvent = NULL;
static HANDLE				hDisconnectThread = NULL;
static LPFN_ACCEPTEX			lpfnAcceptEx = NULL;
static LPFN_CONNECTEX			lpfnConnectEx = NULL;
static LPFN_GETACCEPTEXSOCKADDRS	lpfnGetAcceptExSockaddrs = NULL;


/* Function pointer for QueueUserAPC */
static DWORD (FAR WINAPI *pfnQueueUserAPC)(PAPCFUNC pfnAPC,
                                           HANDLE hThread,
                                           DWORD dwData) = NULL;

/***********************************************************************
** AIO (Alertable IO) async connect/listen variables
*/

#define MAX_PENDING_CONNECTS 20		/* Size of Connect arrays (# entries) */
static u_i4		numPendingConnects = 0;	      /* Curr # active */
static u_i4		highPendingConnects = 0;      /* High water # active */

/*
** struct AIO_CONN_INFO
** Description:
**	Structure for entries in array of connect and listen requests
**	that are pending completion.  Entries are 1:1 with those in
**	event array, which is separate because it must be contiguous
**	for use by WaitForMultipleObjectsEx().
*/
typedef struct _AIO_CONN_INFO
{
    LPOVERLAPPED lpOverlapped;
    SOCKET	 sd;
} AIO_CONN_INFO;

static HANDLE		*arrayConnectEvents = NULL;   /* Connect event array */
static AIO_CONN_INFO  	*arrayConnectInfo   = NULL;   /* Connect socket + overlap ptr array */

/*
** struct AIO_CONN_OVFLO_Q
** Description:
**	Structure for Q'd (overflow) AIO async conn/listen requests.
**	Should rarely be needed...only when no entries are available
**	in the predefined, fixed-size AIO arrays.  Connect requests are
**	put here in the Q until a slot opens up.
*/
typedef struct _AIO_CONN_OVFLO_Q
{
    QUEUE	AIO_Connect_q;
    HANDLE	hEvent;
    LPOVERLAPPED lpOverlapped;
    SOCKET	sd;
} AIO_CONN_OVFLO_Q;

static QUEUE		AIOConnectQ;	/* Overflow Q for AIO connect reqs */
CRITICAL_SECTION	AIOConnectCritSect;
static u_i4		numAIOConnectQ = 0;      /* Current # queued */
static u_i4		totalAIOConnectQ = 0;    /* Total # queued (since startup) */

/*
**     END of AIO connect/listen variables
***********************************************************************/

/*
**  Forward and/or External function references.
*/

STATUS		GCwinsock2_init(GCC_PCE *pptr);
STATUS          GCwinsock2_startup_WSA();
STATUS		GCwinsock2(i4 function_code, GCC_P_PLIST *parm_list);
STATUS		GCwinsock2_exit(GCC_PCE *pptr);
STATUS		GCwinsock2_save(char *buffer, i4 *buf_len, PCB2 *pcb);
STATUS		GCwinsock2_restore(char *buffer, GCC_PCE *pptr, PCB2 **lpPcb);
DWORD WINAPI	GCwinsock2_worker_thread(LPVOID CompletionPortID);
STATUS		GCwinsock2_open(GCC_P_PLIST *parm_list);
VOID 		GCwinsock2_open_setSPXport(SOCKADDR_IPX *spx_sockaddr, char *port_name);
STATUS		GCwinsock2_listen(void *parms);
VOID 		GCwinsock2_listen_complete(GCC_P_PLIST *parm_list, char *AcceptExBuffer);
STATUS  	GCwinsock2_connect(GCC_P_PLIST *parm_list, char *lpPerIoData);
VOID 		GCwinsock2_connect_complete(GCC_P_PLIST *parm_list);
VOID		GCwinsock2_disconnect_thread();
char *  	GCwinsock2_display_IPaddr(struct addrinfo *lpAddrinfo, char *IPaddr_out);
PCB2 *		GCwinsock2_get_pcb(GCC_PCE *pptr, bool outbound);
VOID		GCwinsock2_rel_pcb(PCB2 *pcb);
PER_IO_DATA *	GCwinsock2_get_piod(i4 OperationType, GCC_P_PLIST *parm_list);
VOID		GCwinsock2_rel_piod(PER_IO_DATA *lpPerIoData);
STATUS		GCwinsock2_alloc_connect_events();
STATUS		GCwinsock2_add_connect_event(LPOVERLAPPED lpOverlapped, SOCKET sd);
bool		GCwinsock2_wait_completed_connect(LPOVERLAPPED *lpOverlapped, STATUS *status);
VOID CALLBACK	GCwinsock2_AIO_Callback(DWORD, DWORD, WSAOVERLAPPED *, DWORD);
STATUS		GCwinsock2_schedule_completion(REQUEST_Q *rq, GCC_P_PLIST *parm_list, DWORD ThreadId);
VOID WINAPI	GCwinsock2_async_completion( GCC_P_PLIST *parm_list );
bool		GCwinsock2_OP_RECV_complete(DWORD dwError, STATUS *status, DWORD BytesTransferred, PER_IO_DATA *lpPerIoData);
bool		GCwinsock2_copy_ovflo_to_inbuf(PCB2 *pcb, GCC_P_PLIST *parm_list, bool block_mode, DWORD *BytesTransferred);
STATUS		GCwinsock2_set_timer(PER_IO_DATA *lpPerIoData, i4 timeout);
VOID CALLBACK	GCwinsock2_timer_callback(PER_IO_DATA *lpPerIoData, DWORD dwTimerLowValue, DWORD dwTimerHighValue);
VOID 		GCwinsock2_close_listen_sockets(GCC_LSN *lsn);
VOID		gc_tdump( char *buf, i4 len );


/*
**  Name: GCwinsock2_init
**  Description:
**	WinSock 2.2 inititialization function. This routine is called from
**	GCpinit() -- the routine GCC calls to initialize protocol drivers.
**
**	This function does initialization specific to the protocol:
**	    Initialize tracing variables
**	    Creates I/O completion port for the protocol (non-Windows 9x)
**	    Create worker threads for the I/O completion port (non-Windows 9x)
**	    Load WinSock extension functions (non-Windows 9x)
**
**  History:
**	01-oct-2003 (somsa01)
**	    Created.
**      28-Aug-2006 (lunbr01) 
**          Cannot call WSAGetLastError() if WSAStartup() fails, as with
**          other Windows Sockets calls, because WS2_32.DLL will not have
**          been loaded.
**      03-Aug-2007 (Ralph Loen) SIR 118898
**          Moved WSA startup logic into GCwinsock2_startup_WSA(), so that both
**          GCdns_hostname() and GCwinsock2_init() could check and startup
**          WSA.
**	11-Sep-2008 (lunbr01)
**	    Check/setup trace variable sooner to get more tracing out.
**	26-Nov-2008 (Bruce Lunsford) Bug 121285
**	    Add initialization of objects for disconnect thread.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Read thread variable to determine if using IO Completion Port
**	    or Alertable IO for async IO.
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime.
**	28-Oct-2009 (Bruce Lunsford) Sir 122814
**	    Added variable II_WINSOCK2_NODELAY and config.dat winsock2_nodelay.
**	    and set corresponding static variable GCWINSOCK2_nodelay
**	    accordingly.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Now that this driver can be used for local communications
**	    as well as remote (via GCx server), only do the initialization
**	    on the first time in.  WSAStartup_called field is now
**	    a "use count" rather than a true/false boolean.
**	    Locate QueueuUserAPC() for new thread-aware callback mechanism
**	    and provide config variable II_WINSOCK2_EVENT_Q (=ON) to
**	    resort back to original queue/GCC_COMPLETE event mechanism.
**	    Change default size of receive overflow buffer from #defined
**	    value (=32768) to the packet size in the protocol table (PCT).
**	    The #define value is now only used if PCT packet size is not
**	    greater than 0.  This reduces wasted memory in the case where
**	    the overflow was much bigger than the PCT packet size, without
**	    impacting performance (receive size is set to minimum of
**	    the 2 sizes, which now default to being equal).
**	    Change default IO processing from IO Completion Port to
**	    Alertable IO (AIO), which provides slightly better performance
**	    in most environments and is currently the only supported
**	    option for tcp_ip as a local (GCA CL) protocol (instead of
**	    pipes.  IO Completion Port logic can be enabled by setting
**	    II_WINSOCK2_CONCURRENT_THREADS to a non-zero numeric value.
*/

STATUS
GCwinsock2_init(GCC_PCE * pptr)
{
    STATUS		status = OK;
    char		*ptr;
    GCC_WINSOCK_DRIVER	*wsd;
    THREAD_EVENTS	*Tptr;
    char		*proto = pptr->pce_pid;
    WS_DRIVER		*driver = (WS_DRIVER *)pptr->pce_driver;
    SECURITY_ATTRIBUTES	sa;
    OSVERSIONINFOEX	osver;
    u_i4		NumConcurrentThreads = 0;
    u_i4		NumWorkerThreads = 0;
    HANDLE		hDll = NULL;

    /*
    ** Get trace variable
    */
    NMgtAt("II_WINSOCK2_TRACE", &ptr);
    if (!(ptr && *ptr) && PMget("!.winsock2_trace_level", &ptr) != OK)
	GCWINSOCK2_trace = 0;
    else
	GCWINSOCK2_trace = atoi(ptr);

    GCTRACE(1)("GCwinsock2_init %s: Entered.  use_count(this driver,winsock)=(%d,%d)\n",
		proto, GCwinsock2_use_count, WSAStartup_called);

    GCwinsock2_use_count++;	/* Increment # times init called */

    if (GCwinsock2_startup_WSA() == FAIL)
	    return FAIL;

    hGCwinsock2Process = GetCurrentProcess();  /* Get pseudo handle for proces*/

    iimksec(&sa);

    /*
    ** Should use GVverifyOSversion here but it doesn't have the options 
    ** required.
    ** Could use GVosvers but would require explicit string comparisons for 95
    ** and 98.
    */
    osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((LPOSVERSIONINFO)&osver))
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
        if (GetVersionEx((LPOSVERSIONINFO)&osver))
        {
            is_win9x = ((osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
                ((osver.dwMajorVersion == 4) && (osver.dwMinorVersion >= 0)));
        }
        else
            is_win9x = TRUE;
    }

    /*
    ** Get timeout variable
    */
    NMgtAt("II_WINSOCK2_TIMEOUT", &ptr);
    if (!(ptr && *ptr) && PMget("!.winsock2_timeout", &ptr) != OK)
	GCWINSOCK2_timeout = 10;
    else
	GCWINSOCK2_timeout = atoi(ptr);

    /*
    ** Should TCP_NODELAY be set on sockets?
    */
    NMgtAt("II_WINSOCK2_NODELAY", &ptr);
    if ( ((ptr && *ptr) || (PMget("!.winsock2_nodelay", &ptr) == OK)) &&
	 (STcasecmp( ptr, "ON" ) == 0) )
    {
	GCWINSOCK2_nodelay = TRUE;
	GCTRACE(1)("GCwinsock2_init %s: TCP_NODELAY option is ON\n", proto);
    }
    else
	GCWINSOCK2_nodelay = FALSE;

    /*
    ** Get pointer to WinSock 2.2 protocol's control entry in table.
    */
    Tptr = (THREAD_EVENTS *)pptr->pce_dcb;
    wsd = &Tptr->wsd;

    /*
    ** Now call the protocol-specific init routine.
    */
    if ((*driver->prot_init_func)(pptr, wsd) != OK)
    {
	GCTRACE(1)("GCwinsock2_init %s: Protocol-specific init function failed\n", proto);
	return(FAIL);
    }
    
    /*****************************************************************
    ** The remainder of the code below should only be executed once
    ** (1st time in), so exit if we've been here before.
    *****************************************************************/
    if (GCwinsock2_use_count > 1)  
	return OK;

    /*
    ** Get a handle to the kernel32 dll and get the proc address for
    ** QueueUserAPC, which is used in optional thread-aware callback
    ** mechanism.
    */
    if ((hDll = LoadLibrary( TEXT("kernel32.dll") )) != NULL)
    {
	pfnQueueUserAPC = (DWORD (FAR WINAPI *)(PAPCFUNC pfnAPC,
			   			HANDLE hThread,
			   			DWORD dwData))
			   GetProcAddress(hDll, TEXT("QueueUserAPC"));
	if (pfnQueueUserAPC == NULL)
	{
	    GCTRACE(1)("GCwinsock2_init %s: Function QueueUserAPC not found\n", proto);
	}
	FreeLibrary( hDll );
    }
    else
    {
	    GCTRACE(1)("GCwinsock2_init %s: LoadLibrary kernel32 failed %d\n",
                       proto, GetLastError());
    }

    /*
    ** The option II_WINSOCK2_EVENT_Q (or config.dat winsock2_event_q) forces
    ** the legacy callback mechanism to be used instead of QueueUserAPC().
    ** This option is more for comparing performance than expected to really be
    ** needed by any situation/user.  Presumably could be removed someday
    ** if not really needed.  Unless this option is configured, all
    ** request completions that are queued back to the caller will use
    ** QueueUserAPC() instead of the legacy mechanism that queued the
    ** completions to an internal queue (required synchronization) and
    ** then set the GCC_COMPLETE event.  NOTE that this has no effect on
    ** situations where we call the completion routine back directly,
    ** such as for alertable IO (AIO).
    */
    NMgtAt("II_WINSOCK2_EVENT_Q", &ptr);
    if ( ((ptr && *ptr) || (PMget("!.winsock2_event_q", &ptr) == OK)) &&
	 (STcasecmp( ptr, "ON" ) == 0) )
    {
	GCWINSOCK2_event_q = TRUE;
	GCTRACE(1)("GCwinsock2_init %s: EVENT_Q option is ON\n", proto);
    }
    else
	GCWINSOCK2_event_q = FALSE;

    /*
    ** If this is not Windows 9x, create the I/O completion port to handle
    ** completion of asynchronous events, along with its worker threads.
    ** (Unless Alertable IO has been requested.)
    ** Also, load the WinSock extension functions.
    */
    if (!is_win9x)
    {
	SYSTEM_INFO	SystemInfo;
	u_i4		i;
	SOCKET		sd;
	GUID		GuidAcceptEx = WSAID_ACCEPTEX;
	GUID		GuidConnectEx = WSAID_CONNECTEX;
	GUID		GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD		dwBytes;
	HANDLE		ThreadHandle;
	u_i4		tid;

	/*
	** Create a dummy socket.
	*/
	if ((sd = WSASocket(wsd->sock_fam, wsd->sock_type, wsd->sock_proto,
			    NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
	    GCTRACE(1)("GCwinsock2_init %s: Couldn't create socket, status = %d\n",
			proto, WSAGetLastError());
	    return(FAIL);
	}

	WSAIoctl(sd, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx,
		 sizeof(GuidAcceptEx), &lpfnAcceptEx, sizeof(lpfnAcceptEx),
		 &dwBytes, NULL, NULL);

	WSAIoctl(sd, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx,
		 sizeof(GuidConnectEx), &lpfnConnectEx, sizeof(lpfnConnectEx),
		 &dwBytes, NULL, NULL);

	WSAIoctl(sd, SIO_GET_EXTENSION_FUNCTION_POINTER,
		 &GuidGetAcceptExSockaddrs, sizeof(GuidGetAcceptExSockaddrs),
		 &lpfnGetAcceptExSockaddrs, sizeof(lpfnGetAcceptExSockaddrs),
		 &dwBytes, NULL, NULL);

	closesocket(sd);
	GCTRACE(1)("GCwinsock2_init %s: Closed dummy socket=0x%p ... no longer needed\n", proto, sd);

	/*
	** The size of the receive overflow buffer can be overridden
	** with Ingres variable II_WINSOCK2_RECV_BUFFER_SIZE or
	** config.dat parm winsock2_recv_buffer_size.
	** If neither is defined, then the default size is the
	** packet size from the protocol table; if not defined
	** there, use RCV_PCB_BUFF_SZ, which is #defined in the PCB...
	** typically 8K or 32K.  This size does not include a 2-byte prefix
	** for the length of the message, which is always present.
	** If this variable is set to zero (0), then no overflow will be
	** allocated and the receives will be done using the 2-step stream
	** I/O method rather than the 1-step stream I/O method.  This variable
	** has no impact on block message protocols.
	** The variable was originally added to compare performance between
	** the old 2-step approach and the new 1-step approach and could be
	** removed if one approach is found to be universally better, and
	** if no other benefit seen in being able to specify a specific size.
	*/
	NMgtAt("II_WINSOCK2_RECV_BUFFER_SIZE", &ptr);
	if (!(ptr && *ptr) && PMget("!.winsock2_recv_buffer_size", &ptr) != OK)
	{
	    GCWINSOCK2_recv_buffer_size = -1;  /* not overridden...use default*/
	}
	else
	{
	    GCWINSOCK2_recv_buffer_size = (atoi(ptr));
	}
	GCTRACE(1)("GCwinsock2_init %s: RECV Overflow Buffer size=%d %s ... use %s logic\n", 
		    proto, GCWINSOCK2_recv_buffer_size,
		    (GCWINSOCK2_recv_buffer_size == -1) ? "(use PCT pkt size)" : "",
		    GCWINSOCK2_recv_buffer_size ? "'1-step read all'" : "'2-step read length prefix first'");

	/*
	** Get # concurrent and worker threads variables to control
	** use of IO completion port or Alertable IO (AIO).  
	** These settings may also be useful for performance tuning
	** in situations where it is desired to limit processing
	** to less than the total # of CPUs on the machine.
	**
	** If concurrent threads is set to 0 or not set, then AIO
	** will be used instead of IO completion port. Sends and Receives
	** for AIO run completely in the caller's thread whereas, for
	** IO completion port, their completion is handled by the worker
	** threads.
	**
	** If set to a negative value, then IO completion port logic will
	** be used and the default # of worker threads created will be:
	** (# concurrent + 2) + (#listen ports - 1)) [eg, on a dual-core
	** machine with both IPv6 and IPv4 ports ==> 5 worker threads].
	** If set to more than zero, then that number of worker threads
	** will be created (plus possibly a thread(s) for listen ports).
	*/
	GetSystemInfo(&SystemInfo);
	NMgtAt("II_WINSOCK2_CONCURRENT_THREADS", &ptr);
	if (!(ptr && *ptr) && PMget("!.winsock2_concurrent_threads", &ptr) != OK)
	{
	    NumConcurrentThreads = 0;
	}
	else
	{
	    NumConcurrentThreads = atoi(ptr);
	    if (NumConcurrentThreads < 0)
		NumConcurrentThreads =
		    SystemInfo.dwNumberOfProcessors > MAX_WORKER_THREADS ?
		        MAX_WORKER_THREADS : SystemInfo.dwNumberOfProcessors;
	    else
	    if (NumConcurrentThreads > MAX_WORKER_THREADS)
		NumConcurrentThreads = MAX_WORKER_THREADS;
	}

	NMgtAt("II_WINSOCK2_WORKER_THREADS", &ptr);
	if (!(ptr && *ptr) && PMget("!.winsock2_worker_threads", &ptr) != OK)
	    NumWorkerThreads = NumConcurrentThreads + 2;
	else
	    NumWorkerThreads = atoi(ptr);
	if (NumWorkerThreads > MAX_WORKER_THREADS)
	    NumWorkerThreads = MAX_WORKER_THREADS;
	else
	if (NumWorkerThreads < NumConcurrentThreads)
	    NumWorkerThreads = NumConcurrentThreads;

	/*
	** Document (in trace file) the options chosen.
	** Also force blocking connects/listens if AIO with worker threads=0.
	*/
	if (NumConcurrentThreads > 0)
	{
	    GCTRACE(1)("GCwinsock2_init %s: Using IO Completion port: # Concurrent/Total Worker threads=(%d/%d) on CPU with %d processors.\n",
		    proto, NumConcurrentThreads, NumWorkerThreads,
		    SystemInfo.dwNumberOfProcessors);
	}
	else /* if num concurrent threads set to zero */
	{
	    GCTRACE(1)("GCwinsock2_init %s: Using Alertable IO from caller's thread since winsock2_concurrent_threads set to zero.\n",
		    proto);
	    if (NumWorkerThreads > 0)
	    {
		GCTRACE(1)("  ... Using ASYNC connects with %d worker thread(s).\n", NumWorkerThreads);
		/* Init array of events and overlapped ptrs */
		if ((status = GCwinsock2_alloc_connect_events()) != OK)
		{
		    GCTRACE(1)("GCwinsock2_init %s: Couldn't create event array for AIO connects, status = %d\n",
			    proto, status);
		    return(status);
		}
		QUinit(&AIOConnectQ);
		InitializeCriticalSection( &AIOConnectCritSect );
	    }
	    else
	    {
		GCTRACE(1)("  ... Using SYNC(blocking) connects in main thread.\n");
		/* Force blocking connects/listens */
		lpfnAcceptEx  = NULL, lpfnConnectEx = NULL;
	    }
	}
	    
	if (NumConcurrentThreads) /* Using IO Completion Port(vs Alertable IO)*/
	{
	    GCwinsock2CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
					  NULL, 0, NumConcurrentThreads);
	    if (!GCwinsock2CompletionPort)
	    {
		GCTRACE(1)("GCwinsock2_init %s: Couldn't create I/O completion port, status = %d\n",
			    proto, GetLastError());
		return(FAIL);
	    }
	}

	/*
	** Create specified # of worker threads for IO Completion Port
	** (or for AIO with ASYNC connects/listens).
	** More may be added later in GCC_OPEN based on number of addresses
	** being listened on to avoid unlikely, but possible, tie-up of
	** all worker threads in pending listen completion status (OP_ACCEPT).
	*/
	for (i = 0; i < NumWorkerThreads; i++)
	{
	    ThreadHandle = (HANDLE)_beginthreadex(
			    &sa, GC_STACK_SIZE,
			    (LPTHREAD_START_ROUTINE)GCwinsock2_worker_thread,
			    GCwinsock2CompletionPort, 0, &tid);
	    if (ThreadHandle)
	    {
		GCTRACE(3)("GCwinsock2_init %s: Worker Thread [%d] tid = %d, Handle = %d\n",
			   proto, i, tid, ThreadHandle);
		Tptr->hWorkerThreads[i] = ThreadHandle;
	    }
	    else
	    {
		GCTRACE(1)("GCwinsock2_init %s: Couldn't create worker thread, errno = %d '%s'\n",
			   proto, errno, strerror(errno) );
		if (GCwinsock2CompletionPort)
		    CloseHandle(GCwinsock2CompletionPort);
		return(FAIL);
	    }
	} /* end for each worker thread */

	Tptr->NumWorkerThreads = i;

	/*
	** Initialize objects for Disconnect processing:
	** queue, critical section, event, and thread.
	*/
	QUinit(&DisconnectQ);
	QUinit(&DisconnectQ_in);

	InitializeCriticalSection( &DisconnectCritSect );

	if ( (hDisconnectEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
	{
	    GCTRACE(1)("GCwinsock2_init: Couldn't create Disconnect event, status = %d\n",
			   GetLastError());
	    if (GCwinsock2CompletionPort) CloseHandle(GCwinsock2CompletionPort);
	    return(FAIL);
	}

	hDisconnectThread = (HANDLE)_beginthreadex(
			&sa, GC_STACK_SIZE,
			(LPTHREAD_START_ROUTINE)GCwinsock2_disconnect_thread,
			NULL, 0, &tid);
	if (hDisconnectThread)
	{
	    GCTRACE(3)("GCwinsock2_init: Disconnect Thread tid = %d, Handle = %d\n",
		   tid, hDisconnectThread);
	}
	else
	{
	    GCTRACE(1)("GCwinsock2_init: Couldn't create Disconnect thread, errno = %d '%s'\n",
		   errno, strerror(errno) );
	    if (GCwinsock2CompletionPort) CloseHandle(GCwinsock2CompletionPort);
	    return(FAIL);
	}

    }

    return(OK);
}


/*
**  Name: GCwinsock2
**  Description:
**	Main entry point for the WinSock 2.2 protocol driver. This
** 	driver is essentially just a dispatcher -- it runs in the primary
**	GCC thread and mostly just allows the I/O completion port to handle
**	aynchronous operations (Windows NT platforms) or completion routines
**	to handle asynchronous operations (Windows 9x). It may also start a
**	listen thread if it is a LISTEN request.
**
**	The following functions are handled:
**	    GCC_OPEN	- call GCwinsock2_open
**	    GCC_LISTEN  - post a listen using GCwinsock2_listen
**	    GCC_CONNECT - satisfy a synch / asynch connect request
**	    GCC_SEND    - satisfy an asynch send request
**	    GCC_RECEIVE - satisfy an asynch recieve request
**	    GCC_DISCONN - satisfy an asynch disconnect request
**
**  History:
**	16-oct-2003 (somsa01)
**	    Created.
**	20-feb-2004 (somsa01)
**	    Updated to account for multiple listen ports per protocol.
**	08-oct-2004 (somsa01)
**	    If we're going to queue a completion routine execution, make
**	    sure we return OK from this function. Also, in GCC_DISCONNECT,
**	    make sure the pcb is valid.
**      27-feb-2005 (lunbr01)
**          Fix trace stmts following WSASend and WSARecv by initializing
**          dwBytes to 0 and correcting descriptions on WSASend trace.
**      11-Sep-2008 (lunbr01)
**          Fix compiler warning in LISTEN CreateThread (NULL -> 0).
**          Add trace statements for all error conditions.
**	26-Nov-2008 (Bruce Lunsford) Bug 121285
**	    Redo disconnect logic:
**	    1. Issue closesocket() earlier--here in GCC_DISCONNECT in main 
**	       thread rather than in IO completion routine OP_DISCONNECT
**	       in worker thread.
**	    2. Only invoke background disconnect logic if there is 
**	       currently I/O to wait on.
**	    3. Interface to background disconnect logic has changed.
**	       Previously did a PostQueuedCompletionStatus() call to
**	       OP_DISCONNECT operation type, which would get assigned
**	       to one of the worker threads.  Now, we queue a request
**	       and signal a dedicated disconnect thread.
**	    Call new standalone function GCwinsock2_schedule_completion
**	    instead of having code inline.
**	    Prevent occasional crash in tracing after WSARecv() by not
**	    referencing fields in control blocks that may be gone due
**	    to async I/O completing first on other (worker) threads.
**	02-Mar-2009 (Bruce Lunsford) Bug 121742
**	    Fix crash while connecting on Windows 2000 (and Win 9x/ME).
**	    Allocate lpPerHandleData for non-W9x systems even if
**	    lpfnConnectEx() doesn't exist (as on W2000).
**	    Ensure GCTRACE() not invoked between winsock calls
**	    and WSAGetLastError() because it overlays the returned
**	    status (such as WSAEWOULDBLOCK) with zero, which can cause
**	    hangs if tracing is on.  Also means no longer need to check 
**	    for status!=OK with SOCKET_ERROR.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Handle both AIO and IO Completion Port logic for SENDs and
**	    RECVs, including setup for 1-step or 2-step RECVs.
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime.
**	    Add async connect logic for AIO using Events.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Don't call WSAGetLastError() after getaddrinfo() failure as it
**	    returns a zero (0) status, which is confusing.  Just use the
**	    return value directly from getaddrinfo().
**
**	    Create new functions GCwinsock2_get_pcb() and ...rel_pcb()
**	    to handle allocating/initializing and releasing/freeing a PCB,
**	    respectively.
**	    PER_IO_DATA structures for send and receive are now embedded
**	    in the PCB and no longer need to be allocated and freed for
**	    each send/receive request.
**
**	    If caller supports it, when an operation completes immediately,
**	    return with the results and skip the subsequent callback to the
**	    completion routine; this condition is indicated by returning
**	    status=FAIL but generic_status=OK. Support for this feature
**	    by the caller is indicated with flag PCT_SKIP_CALLBK_IF_DONE
**	    being set in the PCE pce_options field.  Currently the feature
**	    is used by GCA CL to support "peeks" (receives with timeout=0)
**	    and to improve performance.
**
**	    Add support for timeouts on receives. For backward compatibility
**	    to GCC TL which doesn't pass a timeout value in the parm_list,
**	    only check timeout if PCE option PCT_ENABLE_TIMEOUTS is on.
*/

STATUS
GCwinsock2(i4 function_code, GCC_P_PLIST *parm_list)
{
    STATUS		status = OK;
    bool		retval = TRUE;
    THREAD_EVENTS	*Tptr = NULL;
    GCC_LSN		*lsn = NULL;
    GCC_WINSOCK_DRIVER	*wsd;
    u_i4		pce_options;
    PCB2		*pcb = (PCB2 *)parm_list->pcb;
    char		*port = NULL;
    char		*node = NULL;
    int			i, rval;
    DWORD		dwBytes=0;
    DWORD		dwBytes_wanted;
    i4			len, len_prefix, timeout;
    char		*proto = parm_list->pce->pce_pid;
    WS_DRIVER		*driver;
    char		port_name[36];
    PER_IO_DATA  	*lpPerIoData = NULL;
    REQUEST_Q		*rq;
    struct addrinfo	hints, *lpAddrinfo;
    int 		tid;
    SECURITY_ATTRIBUTES sa;
    DWORD		CurrentThreadId = GetCurrentThreadId();

    parm_list->generic_status = OK;
    CLEAR_ERR(&parm_list->system_status);

    /*
    ** Get entry for driver in Winsock protocol driver table 
    ** Tptr=THREAD_EVENTS=dcb (driver ctl block) and embedded struct wsd.
    */
    Tptr = (THREAD_EVENTS *)parm_list->pce->pce_dcb;
    wsd = &Tptr->wsd;
    len_prefix = wsd->block_mode ? 0 : 2;  /* Streams mode has 2 byte len prfx*/
    pce_options = parm_list->pce->pce_options;
    timeout = (pce_options & PCT_ENABLE_TIMEOUTS ) ? parm_list->timeout : -1;


    GCTRACE(5)("GCwinsock2 %s: %p Entered: function=%d thread_id=%d\n", proto, parm_list, function_code, CurrentThreadId);

    /*
    ** Set error based on function code and determine whether we got a
    ** valid function.
    */
    switch (function_code)
    {
	case GCC_OPEN:
	    is_comm_svr = TRUE;
	    GCTRACE(2)("GCwinsock2 %s: GCC_OPEN %p\n", proto, parm_list);
	    return GCwinsock2_open(parm_list);

	case GCC_LISTEN:
	    iimksec(&sa);

	    GCTRACE(2)("GCwinsock2 %s: GCC_LISTEN %p\n", proto, parm_list);

	    /*
	    ** Use the proper listen port.
	    */
	    lsn = wsd->lsn;
	    while ( lsn &&
		STcompare(parm_list->pce->pce_port, lsn->pce_port) != 0)
	    {
	        lsn = lsn->lsn_next;
	    }

	    if (!lsn)
	    {
		SETWIN32ERR(&parm_list->system_status, status, ER_listen);
	        parm_list->generic_status = GC_LISTEN_FAIL;
		GCTRACE(1)("GCwinsock2 %s: GCC_LISTEN %p No matching GCC_LSN cb found for port %s\n", 
			proto, parm_list, parm_list->pce->pce_port);
	        goto complete;
	    }

	    /*
	    ** Since LISTENs are always done in a different thread
	    ** from the originator, pass on the original caller's thread
	    ** handle to the listen processing functions so that the
	    ** callback to the caller's completion routine can be done in
	    ** the original thread.  Without the handle, can still use LISTEN
	    ** event instead at callback time as long as only 1 thread is
	    ** waiting on it, which is usually the case.
	    ** Create a REQUEST_Q block to contain this info along with
	    ** the pointer to the parmlist.
	    */
	    rq = (REQUEST_Q *)MEreqmem(0, sizeof(*rq), TRUE, NULL);
	    if (rq == NULL)
	    {
		SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
	        parm_list->generic_status = GC_LISTEN_FAIL;
		GCTRACE(1)("GCwinsock2%s: GCC_LISTEN %p ERROR: Unable to allocate rq structure\n", proto, parm_list);
		return(FAIL);
	    }
	    rq->plist = parm_list;
	    rq->ThreadId = CurrentThreadId;

	    if (lpfnAcceptEx)
		return GCwinsock2_listen(rq);
	    else
	    {
		if (lsn->hListenThread =
			(HANDLE)_beginthreadex(&sa, GC_STACK_SIZE,
				     (LPTHREAD_START_ROUTINE)GCwinsock2_listen,
				     rq, 0, &tid))
		{
		    return(OK);
		}

		status = errno;
		SETWIN32ERR(&parm_list->system_status, status, ER_threadcreate);
		GCTRACE(1)("GCwinsock2 %s: GCC_LISTEN %p Unable to create listen thread, status=%d\n", 
			proto, parm_list, status);
		MEfree((PTR)rq);
		break;
	    }

	case GCC_CONNECT:

	    GCTRACE(2)("GCwinsock2 %s: GCC_CONNECT %p\n", proto, parm_list);
	    port = parm_list->function_parms.connect.port_id;
	    node = parm_list->function_parms.connect.node_id;
            GCTRACE(2)("GCwinsock2 %s: GCC_CONNECT %p node=%s port=%s\n",
                       proto, parm_list, node, port);

	    driver = (WS_DRIVER *)wsd->pce_driver;

	    /*
	    ** Obtain a protocol control block.
	    */
	    if ( ((PCB2 *)parm_list->pcb = pcb = GCwinsock2_get_pcb(parm_list->pce, TRUE)) == NULL )
	    {
		SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
		parm_list->generic_status = GC_CONNECT_FAIL;
		break;
	    }

	    /*
	    ** Call the protocol-specific addressing function.
	    ** If an Ingres symbolic port was input, this routine
	    ** will convert it to a numeric port that can be used
	    ** in the network function calls.
	    */ 
	    if ((status = (*driver->addr_func)(node, port, port_name)) == FAIL)
	    {
		SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		parm_list->generic_status = GC_CONNECT_FAIL;
        	GCTRACE(1)("GCwinsock2 %s %d: GCC_CONNECT %p Protocol-specific address function failed for node=%s port=%s, status=%d\n",
                       proto, pcb->id, parm_list, node, port, status);
		break;
	    }

	    GCTRACE(2)( "GCwinsock2 %s %d: GCC_CONNECT %p connecting to %s port %s(%s)\n", 
		proto, pcb->id, parm_list, node, port, port_name );

	    /*
	    ** Set up criteria for getting list of IPv* addrs to connect to.
	    ** This will be used as the main input parm to getaddrinfo().
	    */
	    memset(&hints, 0, sizeof(hints));
	    hints.ai_family   = wsd->sock_fam;
	    hints.ai_socktype = wsd->sock_type;
	    hints.ai_protocol = wsd->sock_proto;

	    /*
	    ** Get a list of qualifying addresses for the target hostname,
	    ** which may be a mix of IPv6 & IPv4, and may be more than 1
	    ** of each, particularly for IPv6.
	    */
	    status = getaddrinfo(node, port_name, &hints, &pcb->addrinfo_list);
	    if (status != 0)
	    {
	        /*
	        ** Use status from getaddrinfo() directly; do not call
	        ** WSAGetLastError() as it returns a zero status, which
	        ** is confusing if there is an error.
	        */
	        SETWIN32ERR(&parm_list->system_status, status, ER_getaddrinfo);
	        parm_list->generic_status = GC_CONNECT_FAIL;
	        GCTRACE(1)( "GCwinsock2 %s %d: GCC_CONNECT %p getaddrinfo() failed for node=%s port=%s(%s), status=%d\n",
			    proto, pcb->id, parm_list, node, port, port_name, status);
	        break;
	    }

	    /*
	    ** Ensure at least one address returned.
	    */
	    if (pcb->addrinfo_list == NULL)
	    {
	        SETWIN32ERR(&parm_list->system_status, status, ER_getaddrinfo);
	        parm_list->generic_status = GC_CONNECT_FAIL;
	        GCTRACE(1)( "GCwinsock2 %s %d: GCC_CONNECT %p getaddrinfo() returned no addresses for node=%s port=%s(%s)\n",
			    proto, pcb->id, parm_list, node, port, port_name);
	        break;
	    }

	    if (lpfnConnectEx)  /* If using Compl Port or alertable IO */
	    {
		/*
		** Allocate the PER_IO_DATA structure and initialize.
		*/
		lpPerIoData = GCwinsock2_get_piod(OP_CONNECT, parm_list);
		if (lpPerIoData == NULL)
		{
		    parm_list->generic_status = GC_CONNECT_FAIL;
		    break;
		}
		lpPerIoData->ThreadId = CurrentThreadId;

	    }  /* Endif lpfnConnectEx */

	    /*
	    ** Walk through the list of addresses returned and
	    ** connect to each until one is successful.
	    ** NOTE that for certain types of errors, we bail
	    ** out of the loop immediately and return with an error.
	    */
	    pcb->sd = INVALID_SOCKET;
	    for (lpAddrinfo = pcb->addrinfo_list;
	         lpAddrinfo;
	         lpAddrinfo = lpAddrinfo->ai_next)
	    {
		pcb->lpAddrinfo = lpAddrinfo;

		if (GCwinsock2_connect (parm_list, (char *)lpPerIoData) == OK)
		    if (lpfnConnectEx)    /* Async connect ... in progress */
		        return(OK);
		    else                  /* Sync connect ... connected */
		        goto complete;
                if (parm_list->generic_status == GC_CONNECT_FAIL)  /* fatal error */
                    goto complete;

	    }  /* end of loop through addresses */

	    if (!lpAddrinfo)  /* If exited because reached the end of addrs */
	    {
		SETWIN32ERR(&parm_list->system_status, status, ER_connect);
		parm_list->generic_status = GC_CONNECT_FAIL;
		GCTRACE(1)("GCwinsock2 %s %d: GCC_CONNECT %p All IP addrs failed to connect to %s port %s(%s)\n",
			proto, pcb->id, parm_list, node, port, port_name );
		goto complete;
	    }

	case GCC_SEND:
	    if (!pcb) goto complete;

	    GCTRACE(2) ("GCwinsock2 %s %d: GCC_SEND %p PendingIOs=%d\n",
		proto, pcb->id, parm_list, pcb->AsyncIoRunning);

	    pcb->snd_bptr = parm_list->buffer_ptr - len_prefix;
	    pcb->tot_snd  = parm_list->buffer_lng + len_prefix;
	    if (len_prefix > 0)
	    {
		u_i2	tmp_i2;
		u_char	*cptr;

		/*
		** Handle 1st 2 bytes which are a msg length field.
		** NOTE!! It is a REQUIREMENT that the caller provides
		** space preceding the message that can be used
		** by the protocol driver.  For streams-based protocols
		** like TCP/IP, this is used to prefix a 2-byte message
		** length field.
		*/
		tmp_i2 = (u_i2)(pcb->tot_snd);
		cptr = (u_char *)&tmp_i2;
		pcb->snd_bptr[1] = cptr[1];
		pcb->snd_bptr[0] = cptr[0];
	    }

	    /*
	    ** Initialize the PER_IO_DATA structure.
	    ** If the default one is still in use, then get a new one.
	    */
	    lpPerIoData = &pcb->snd_PerIoData;
	    if (lpPerIoData->state != PIOD_ST_AVAIL)
	    {
		lpPerIoData = GCwinsock2_get_piod(OP_SEND, parm_list);
		if (lpPerIoData == NULL)
		{
		    parm_list->generic_status = GC_SEND_FAIL;
		    break;
		}
	    }
	    else  /* reuse existing (default) one */
	    {
		ZeroMemory(&lpPerIoData->Overlapped, sizeof(OVERLAPPED));
		lpPerIoData->state	= PIOD_ST_IO_PENDING;
		lpPerIoData->parm_list	= parm_list;
		lpPerIoData->block_mode = wsd->block_mode;
	    }
	    lpPerIoData->wbuf.buf = pcb->snd_bptr;
	    lpPerIoData->wbuf.len = pcb->tot_snd;
	    if (GCwinsock2CompletionPort)
		lpPerIoData->ThreadId = CurrentThreadId;

	    InterlockedIncrement(&pcb->AsyncIoRunning);
	    rval = WSASend( pcb->sd, &lpPerIoData->wbuf, 1, &dwBytes, 0,
			&lpPerIoData->Overlapped,
			GCwinsock2CompletionPort ? NULL : GCwinsock2_AIO_Callback );

	    if (rval)
	    {
		status = WSAGetLastError();
		if (status == WSA_IO_PENDING)
		{
		    GCTRACE(2)( "GCwinsock2 %s %d: GCC_SEND %p, try send %d bytes, WSASend() IO_PENDING\n",
			proto, pcb->id, parm_list, lpPerIoData->wbuf.len );
		    return(OK);
		}
		else
		{
		    GCTRACE(1)( "GCwinsock2 %s %d: GCC_SEND %p WSASend() failed for %d bytes on socket=0x%p, status=%d\n",
			     proto, pcb->id, parm_list, pcb->tot_snd, pcb->sd, status);
		    pcb->tot_snd = 0;
		    parm_list->generic_status = GC_SEND_FAIL;
		    SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		    InterlockedDecrement(&pcb->AsyncIoRunning);
		    GCwinsock2_rel_piod(lpPerIoData);
		    return FAIL; /* GCC_SEND failed */
		}
	    }
	    else   /* Successful immediate completion */
	    {
		/*
		** If caller requires results to be provided to its callback
		** completion routine (rather than now at return), or if said
		** callback can't be blocked/suppressed,
		** just return OK now and let the normal callback pass back
		** the results.  GCC TL, for instance, requires callback, but
		** GCA CL (gcarw.c) doesn't (checks for results upon return).
		** Return results now and block/skip the callback ONLY IF flag
		** PCT_SKIP_CALLBK_IF_DONE is set.
		**
		** A special case to also wait for callback is when 0 bytes
		** were sent, indicating the other side disconnected.
		** The callback logic is already set up to handle this as an
		** error and there is no harm in waiting until callback.
		** Also makes it consistent with GCC_RECEIVE processing.
		**
		** For now, immediate returns for IO Completion Port logic is
		** is not supported because the IO callback runs in separate
		** worker threads and so may have already run.  Hence,
		** it is difficult to reliably suppress the callback of the
		** caller's completion routine...a timing issue.
		*/
		if ( !(pce_options & PCT_SKIP_CALLBK_IF_DONE) ||
		     GCwinsock2CompletionPort ||
		     pcb->AsyncIoRunning > ASYNC_IO_RUNNING_SOFT_LIMIT ||
		     !dwBytes )
		{
		    GCTRACE(2)( "GCwinsock2 %s %d: GCC_SEND %p, try send %d bytes, did send %d bytes, results in callback\n",
			proto, pcb->id, parm_list, pcb->tot_snd, dwBytes);
		    return(OK);
		}
	    }

	    /*
	    ** WSASend() completed immediately - successfully.
	    ** Set return status to FAIL, which actually means "all done".
	    ** The OS will still call our IO callback routine even
	    ** though the request completed immediately.  Flag the IO
	    ** request so that our IO callback routine, when invoked
	    ** by the OS, will not callback the caller's completion routine.
	    */
	    GCTRACE(2)( "GCwinsock2 %s %d: GCC_SEND %p, try send %d bytes, did send %d bytes\n",
			proto, pcb->id, parm_list, pcb->tot_snd, dwBytes );
	    lpPerIoData->state = PIOD_ST_SKIP_CALLBK;
	    pcb->skipped_callbk_flag = TRUE;
	    return(FAIL);


	case GCC_RECEIVE:
	    if (!pcb) goto complete;

	    GCTRACE(2) ("GCwinsock2 %s %d: GCC_RECEIVE %p PendingIOs=%d PLIST: buf=0x%p len=%d\n",
			proto, pcb->id, parm_list, pcb->AsyncIoRunning,
			parm_list->buffer_ptr, parm_list->buffer_lng);
	    GCTRACE(5) ("GCwinsock2 %s %d: GCC_RECEIVE %p PCB: bptr=0x%p buffer=x%p for len=%d Ovflo[0x%p-0x%p]\n",
			proto, pcb->id, parm_list,
			pcb->rcv_bptr, pcb->rcv_buffer, pcb->rcv_buf_len,
			pcb->b1, pcb->b2);

	    /*
	    ** DBMS server tends to reissue expedited receives no matter what
	    ** has happened in order to keep an expedited receive pending.
	    ** If the session was already disconnected by the client,
	    ** then there is no point in reissuing a receive and getting
	    ** the same error (again).  Ignore the request.
	    */
	    if (pcb->session_state == SS_DISCONNECTED)
		return(OK);

	    /*
	    ** Sanity check prior receive request state.  Also, if an IO is 
	    ** pending for a previously timed out request, then redirect
	    ** the pending IO to this new request.
	    */
	    if (pcb->last_rcv_PerIoData)
	    {
		PER_IO_DATA *last_PerIoData = pcb->last_rcv_PerIoData;
		switch (last_PerIoData->state)
		{
		case PIOD_ST_AVAIL:
		case PIOD_ST_SKIP_CALLBK:
		    break;		/* OK to proceed */

		case PIOD_ST_TIMED_OUT:
		    /*
		    ** Hijack prior receive, which was timed out but the IO
		    ** is still pending.
		    */
		    last_PerIoData->parm_list = parm_list;
		    switch (timeout)
		    {
		    case -1: 	/* no timeout (INFINITE)  - IO Pending */
			GCTRACE(4)( "GCwinsock2 %s %d: GCC_RECEIVE %p, want %d bytes, use existing WSARecv() IO_PENDING\n",
			        proto, pcb->id, parm_list, last_PerIoData->wbuf.len );
			last_PerIoData->state     = PIOD_ST_REDIRECT;
			break;

		    case 0:	/* "peek" failed - still no data received */
			GCTRACE(4)( "GCwinsock2 %s %d: GCC_RECEIVE %p, want %d bytes, Timed out (timeout=0)\n",
			        proto, pcb->id, parm_list, last_PerIoData->wbuf.len );
			parm_list->generic_status = GC_TIME_OUT;
	    		last_PerIoData->state = PIOD_ST_TIMED_OUT;
			return(FAIL);

		    default: 		/* timeout > 0 - set timer */
			GCTRACE(4)( "GCwinsock2 %s %d: GCC_RECEIVE %p, want %d bytes, use existing WSARecv() IO_PENDING with timeout=%d\n",
			        proto, pcb->id, parm_list, last_PerIoData->wbuf.len, timeout );
			GCwinsock2_set_timer(last_PerIoData, timeout);
			break;
		    }  /* end switch on timeout */

		    return(OK);		/* Return as if WSARecv() issued */

		default:		/* Shouldn't be in that state */
		    parm_list->generic_status = GC_RECEIVE_FAIL;
		    GCTRACE(1)( "GCwinsock2 %s %d: GCC_RECEIVE %p failure - Invalid state (%d) of last GCC_RECEIVE %p, socket=0x%p\n",
		                proto, pcb->id, parm_list,
		                last_PerIoData->state,
		                last_PerIoData->parm_list,
		                pcb->sd);
		    goto complete;
		} /* End switch on state */
	    } /* End if last_rcv_PerIoData exists */

	    /*
	    ** Set buffer pointer and length for receive.
	    **
	    ** NOTE!! It is a REQUIREMENT that the caller provides
	    ** space preceding the message buffer that can be used
	    ** by the protocol driver.  
	    **
	    ** For streams-based protocols like TCP/IP, this is used
	    ** to prefix a 2-byte length field to each message. The caller's
	    ** completion routine is not invoked until the entire message,
	    ** based on the incoming length field, has been received,
	    ** which may require multiple WSARecv()s.
	    ** The caller only sees the message, not the prefix, since
	    ** the prefix is placed ahead of the message buffer of the caller.
	    ** Excess incoming data, if any, is saved in an internal
	    ** receive buffer area (ie, for overflow).
	    **
	    ** For block-mode protocols, no message prefix is used.
	    ** Whatever is received is passed directly back to the caller.
	    ** Hence, no such thing as excess.
	    ** Block mode is used by Novell SPX protocol and by GCA CL
	    ** calling this driver for TCP_IP.  GCA CL has his own
	    ** length information embedded within the message and keeps
	    ** track of when a full message has been received as well as
	    ** excess/overflow.
	    */
	    pcb->rcv_bptr = parm_list->buffer_ptr - len_prefix;
	    pcb->tot_rcv  = parm_list->buffer_lng + len_prefix;

	    if (len_prefix > 0)
	    {
		if( pcb->rcv_buf_len <= sizeof(pcb->rcv_buffer) )  /* stream 2-step read */
		{
		    /*
		    ** No receive buffer for overflow, so will first read only
		    ** the length prefix that contains the message length.
		    ** Then will do a 2nd receive to get the actual message.
		    */
		    pcb->tot_rcv = len_prefix;
		}
		else     /* stream 1-step read with overflow buffer */
		{
		    /*
		    ** Setup bounds of read area.
		    **
		    ** Caller should restrict requests to the limit defined
		    ** in the protocol table.  We allow longer requests, but
		    ** are limited by the size of the overflow buffer.  By
		    ** restricting reads to the size of the overflow buffer,
		    ** we ensure that a buffer overrun won't occur.  This
		    ** means multiple reads will be required to receive
		    ** requests larger than the declared limit.
		    ** NOTE: The default size of the overflow buffer is
		    ** now set to equal the protocol table packet size.
		    */
		    pcb->tot_rcv = min( parm_list->buffer_lng + len_prefix, pcb->rcv_buf_len );
		}
	    }

	    /*
	    ** Get what we can from the overflow buffer.  If a whole
	    ** message is there, use it instead of issuing the
	    ** WSARecv() below.
	    */
	    if( len = ( pcb->b2 - pcb->b1 ) )  /* #bytes in ovflo buffer */
	    {
		/*
		** Turn off timeout for this receive (in case it happens to
		** have been set by the caller).  Since we have data already
		** available in the overflow/receive buffer, we consider
		** the receive immediately satisfied by that data.  Hence,
		** no timeout can occur, even if we only have a partial
		** message and need to issue another WSARecv() to get the rest.
		*/
		timeout = -1;

		/*
		** Skip the receive if an entire message was found
		** in the overflow area from a prior receive.
		** Ie, the GCC_RECEIVE is done!
		*/
		if (GCwinsock2_copy_ovflo_to_inbuf(pcb, parm_list, wsd->block_mode, &len) == TRUE)
		{   /* got whole message */
		    if (pce_options & PCT_SKIP_CALLBK_IF_DONE)
			return(FAIL);  /* No need to call compl rtn...immed rtns OK */
		    else
		        goto complete; /* Call completion rtn */
		}
	    }  /* End if have any bytes already in (overflow) buffer. */

	    /*
	    ** Initialize a PER_IO_DATA structure.
	    ** If the default one is still in use, then get a new one.
	    */
	    lpPerIoData = &pcb->rcv_PerIoData;
	    if (lpPerIoData->state != PIOD_ST_AVAIL)
	    {
		lpPerIoData = GCwinsock2_get_piod(OP_RECV, parm_list);
		if (lpPerIoData == NULL)
		{
		    parm_list->generic_status = GC_RECEIVE_FAIL;
		    break;
		}
	    }
	    else  /* reuse existing (default) one */
	    {
		ZeroMemory(&lpPerIoData->Overlapped, sizeof(OVERLAPPED));
		lpPerIoData->state	= PIOD_ST_IO_PENDING;
		lpPerIoData->parm_list  = parm_list;
		lpPerIoData->block_mode = wsd->block_mode;
	    }
	    if (GCwinsock2CompletionPort)
		lpPerIoData->ThreadId = CurrentThreadId;
	    pcb->last_rcv_PerIoData = lpPerIoData;   /* save ptr to it */

	    /*
	    ** If a timeout was specified ( >=0  or !=-1), the overflow
	    ** buffer is used to receive the data rather than the caller's
	    ** buffer.  This is because we have no convenient way to
	    ** time out or cancel a receive once we've issued it below.
	    ** When a timeout occurs, the caller is notified of the 
	    ** error (GC_TIME_OUT), but the WSARecv() IO remains pended.
	    ** If a new GCC_RECEIVE request comes in, it is hooked up
	    ** to the pended IO.  Since the new GCC_RECEIVE request will
	    ** likely have a different target buffer address and length
	    ** from the timed out request, and because neither value can
	    ** be changed once WSARecv() has been issued, we instead use
	    ** an intermediate area (the receive buffer/overflow area) to
	    ** receive the data; this area persists for the duration of
	    ** the connection.  When the IO completes, then the data
	    ** must be copied from the intermediate area into the caller's
	    ** buffer; this is a slight performance hit, but unavoidable.
	    ** Fortunately, timeouts on receives are the exception, not
	    ** the "norm".
	    */
	    if (timeout >= 0)
	    {
		lpPerIoData->state = PIOD_ST_IO_TMO_PENDING;
		pcb->rcv_bptr = pcb->b2;
		pcb->tot_rcv = pcb->rcv_buf_len - (pcb->b2 - pcb->b1);
		GCTRACE(3)("GCwinsock2 %s %d: GCC_RECEIVE %p with timeout=%d, recv into ovflo at 0x%p (offset=%d) for len=%d\n",
			proto, pcb->id, parm_list, timeout,
			pcb->rcv_bptr,
			pcb->rcv_bptr - pcb->rcv_buffer,
			pcb->tot_rcv);
	    }

	    /*
	    ** Finish setting up PER_IO_DATA with address and length
	    ** of area to receive into.
	    */
	    lpPerIoData->wbuf.buf = pcb->rcv_bptr;
	    lpPerIoData->wbuf.len = dwBytes_wanted = pcb->tot_rcv;

	    GCTRACE(5)("GCwinsock2 %s %d: GCC_RECEIVE %p WSARecv() args: PIOD=0x%p &wbuf=0x%p, .buf=0x%p, .len=%d, Overlapped=0x%p\n",
			proto, pcb->id, parm_list,
			lpPerIoData,
			&lpPerIoData->wbuf,
			lpPerIoData->wbuf.buf,
			lpPerIoData->wbuf.len,
			&lpPerIoData->Overlapped);

	    InterlockedIncrement(&pcb->AsyncIoRunning);
	    i = 0;
	    rval = WSARecv( pcb->sd, &lpPerIoData->wbuf, 1, &dwBytes, &i,
			&lpPerIoData->Overlapped,
			GCwinsock2CompletionPort ? NULL : GCwinsock2_AIO_Callback );

	    if (rval)
	    {
		status = WSAGetLastError();
		if (status == WSA_IO_PENDING)
		{
		    switch (timeout)
		    {
		    case -1: 	/* no timeout (INFINITE)  - IO Pending */
			GCTRACE(4)( "GCwinsock2 %s %d: GCC_RECEIVE %p, want %d bytes, WSARecv() IO_PENDING\n",
			        proto, pcb->id, parm_list, dwBytes_wanted );
			return(OK);

		    case 0:	/* "peek" failed - not immediate completion */
			GCTRACE(4)( "GCwinsock2 %s %d: GCC_RECEIVE %p, want %d bytes, timed out (timeout=0)\n",
			        proto, pcb->id, parm_list, dwBytes_wanted );
			parm_list->generic_status = GC_TIME_OUT;
	    		lpPerIoData->state = PIOD_ST_TIMED_OUT;
			return(FAIL);

		    default: 		/* timeout > 0 - set timer */
			GCTRACE(4)( "GCwinsock2 %s %d: GCC_RECEIVE %p, want %d bytes, WSARecv() IO_PENDING with timeout=%d\n",
			        proto, pcb->id, parm_list, dwBytes_wanted, timeout );
			GCwinsock2_set_timer(lpPerIoData, timeout);
			return(OK);
		    }  /* end switch on timeout */
		}  /* end if IO_PENDING */
	    }  /* end if non-zero return from WSARecv() */
	    else   /* Successful immediate completion */
	    {
		/*
		** If caller requires results to be provided to its callback
		** completion routine (rather than now at return), or if said
		** callback can't be blocked/suppressed,
		** just return OK now and let the normal callback pass back
		** the results.  GCC TL, for instance, requires callback, but
		** GCA CL (gcarw.c) doesn't (checks for results upon return).
		** Return results now and block/skip the callback ONLY IF flag
		** PCT_SKIP_CALLBK_IF_DONE is set.
		**
		** A special case to also wait for callback is when 0 bytes
		** were received, indicating the other side disconnected.
		** This makes disconnects in the DBMS server "cleaner" since
		** it allows time for a just-received GCA_RELEASE message to
		** get processed before the DBMS gets notified that his
		** "always pending" expedited receive failed.
		**
		** For now, immediate returns for IO Completion Port logic is
		** is not supported because the IO callback runs in separate
		** worker threads and so may have already run.  Hence,
		** it is difficult to reliably suppress the callback of the
		** caller's completion routine...a timing issue.
		*/
		if ( !(pce_options & PCT_SKIP_CALLBK_IF_DONE) ||
		     GCwinsock2CompletionPort ||
		     pcb->AsyncIoRunning > ASYNC_IO_RUNNING_SOFT_LIMIT ||
		     !dwBytes )
		{
		    GCTRACE(2)( "GCwinsock2 %s %d: GCC_RECEIVE %p, want %d bytes got %d, results in callback\n",
			proto, pcb->id, parm_list, dwBytes_wanted, dwBytes );
		    return(OK);
		}
	    }

	    /*
	    ** WSARecv() completed immediately, either with an error or
	    ** successfully.  Call common Receive completion logic.
	    ** If it returns TRUE, then the receive is complete, so set return
	    ** status to FAIL, meaning "all done" -> results are available
	    ** now and caller's completion routine will not be called back.
	    **
	    ** For successful completions, the results are available now
	    ** to the caller. However, the OS will still call our IO
	    ** callback routine, so flag the IO request so that our
	    ** IO callback routine, when invoked by the OS, will not
	    ** callback the caller's completion routine.
	    **
	    ** For errors, the parm_list->generic_status contains an
	    ** error code.  The OS won't make a callback so free
	    ** the lpPerIoData, which is normally freed in our callback
	    ** routine.
	    */
            if ( (GCwinsock2_OP_RECV_complete(rval, &status, dwBytes,
 lpPerIoData)) == TRUE )
	    {
		if (parm_list->generic_status != OK )  /* Error...*/
		{
		    /*
		    ** While errors can be returned immediately, deferring
		    ** them  until callback helps the DBMS with disconnect;
		    ** otherwise, expedited recv will likely be posted
		    ** before DBMS has a chance to start mainline code
		    ** GCA_DISASSOCIATE processing.  Benign, but annoying,
		    ** error messages can show up in the errlog.log from
		    ** disconnect otherwise.
		    */
		    return(OK); 	/* defer error until callback */
		    /* Alternative - return error now */
		    //InterlockedDecrement(&pcb->AsyncIoRunning);
		    //GCwinsock2_rel_piod(lpPerIoData);
		}
		else  /* successful...return results now, ignore later callbk */
		{
		    lpPerIoData->state = PIOD_ST_SKIP_CALLBK;
		    pcb->skipped_callbk_flag = TRUE;
		}
		return(FAIL);
	    }
	    /*
	    ** GCwinsock2_OP_RECV_complete() returned FALSE, indicating
	    ** processing NOT yet complete...caller will get results
	    ** when his completion routine is called back.  Hence, return
	    ** TRUE as with normal WSA_IO_PENDING case above.
	    ** This situation could occur if it is ever decided to support
	    ** stream mode (rather than just block mode) for immediate
	    ** completions.  GCwinsock2_OP_RECV_complete() would need to
	    ** be fixed to not reuse same lpPerIoData if it decides to
	    ** reissue a WSARecv() to get the remainder of a message.
	    */
            return(OK);  /* another WSARecv() was issued...not done yet */


	case GCC_DISCONNECT:
	    if (!pcb) goto complete;

	    GCTRACE(2)("GCwinsock2 %s %d: GCC_DISCONNECT %p PendingIOs=%d\n",
			proto, pcb->id, parm_list, pcb->AsyncIoRunning);

	    /*
	    ** OK to issue closesocket() here because it returns
	    ** immediately and does a graceful shutdown of the socket
	    ** in background (default behavior of closesocket()).
	    ** Previous code waited to issue it in worker_thread disconnect
	    ** processing which is unnecessary and slower...better to
	    ** get the disconnect started sooner, rather than later.
	    */
	    closesocket(pcb->sd);
	    GCTRACE(2)("GCwinsock2 %s %d: GCC_DISCONNECT %p Closed socket=0x%p\n",
			proto, pcb->id, parm_list, pcb->sd);

	    /*
	    ** If we have skipped any callbacks to the caller's completion
	    ** routine, the corresponding IO callbacks may not have all
	    ** yet completed.  Before we disconnect, make sure they get a
	    ** chance to complete (even though they are basically NO-OPs);
	    ** this is to make sure that accvio's don't occur if this
	    ** thread goes away (eg, at server shutdown or DBMS session
	    ** disconnect).
	    ** NOTE: The WaitForMultipleObjectsEx() calls in GCexec, etc
	    ** process events in the wait list before (ie, higher priority)
	    ** than async IO completions.  This can cause these skipped
	    ** callbacks to queue up if the system is busy with various
	    ** events getting driven (such as connects and disconnects).
	    */
	    if (pcb->skipped_callbk_flag == TRUE)
		SleepEx(0, TRUE);

	    if (pcb->AsyncIoRunning > 0)
	    {
		/*
		** Async I/O is pending on this socket.  Must wait for
		** it to complete before calling disconnect completion
		** routine.  NOTE that we may have been called by the
		** completion routine of the last pending GCC_RECEIVE
		** or GCC_SEND, in which case the I/O pending count will
		** be 1 now but will go to zero as soon as we return from
		** the send/recv completion routine.  Hence, the disconnect
		** thread, when it wakes up, will likely find the count
		** already set to 0 and immediately call the disconnect
		** completion routine.  While this is somewhat of a waste
		** of resources in that it would probably be OK to go
		** ahead and call the disconnect completion routine from
		** here, it is not "in general" a safe thing to do and
		** checking for that special case may cause more overhead
		** than it's worth.  Slight delays in disconnect processing
		** are not felt to be a concern.
		*/
		GCTRACE(2)("GCwinsock2 %s %d: GCC_DISCONNECT %p Pending I/Os=%d not zero so Q to disconnect thread for completion\n",
			proto, pcb->id, parm_list, pcb->AsyncIoRunning);

		/*
		** Schedule request to the Disconnect thread
		*/
		if ((rq = (REQUEST_Q *)MEreqmem(0, sizeof(*rq), TRUE, NULL)) == NULL)
		{
		    SETWIN32ERR(&parm_list->system_status, status, ER_close);
		    parm_list->generic_status = GC_DISCONNECT_FAIL;
		    GCTRACE(1)("GCwinsock2 %s %d: GCC_DISCONNECT %p Unable to allocate memory for rq\n",
			    proto, pcb->id, parm_list);
		    break;
		}

		rq->plist = parm_list;
		rq->ThreadId = CurrentThreadId;

		/*
		** Get critical section object for Disconnect input Q.
		*/
		EnterCriticalSection( &DisconnectCritSect );

		/*
		** Insert the disconnect request into the Disconnect input Q.
		*/
		QUinsert(&rq->req_q, &DisconnectQ_in);

		/*
		** Release critical section object for Disconnect input Q.
		*/
		LeaveCriticalSection( &DisconnectCritSect );

		/*
		** Raise the disconnect event to wake up Disconnect thread.
		*/
		if (!SetEvent(hDisconnectEvent))
		{
		    status = GetLastError();
		    GCTRACE(1)("GCwinsock2 GCC_DISCONNECT, SetEvent error = %d\n", status);
		    SETWIN32ERR(&parm_list->system_status, status, ER_close);
		    parm_list->generic_status = GC_DISCONNECT_FAIL;
	    	    GCTRACE(1)("GCwinsock2 %s %d: GCC_DISCONNECT %p PostQueuedCompletionStatus() failed for socket=0x%p, status=%d)\n", 
			    proto, pcb->id, parm_list, pcb->sd, status);
		    break;
		}

		return(OK);
	    } /* End if async I/O pending */

	    /*
	    ** No async I/O pending on socket, so OK to complete disconnect
	    ** processing immediately.
	    */
	    GCwinsock2_rel_pcb(pcb);
	    parm_list->pcb = NULL;

	    /*
	    ** If caller supports immediate return with results and skipping
	    ** the callback to the completion routine, then return now.
	    */
	    if (pce_options & PCT_SKIP_CALLBK_IF_DONE)
		return(FAIL);  /* signals that disconnect done...not pending */
	    break;

	default:
	    return(FAIL);
    }

complete:

    /*
    ** Drive the completion routine.
    */
    (*parm_list->compl_exit)( parm_list->compl_id );

    /*
    ** Return OK since we just called the completion routine.
    ** FAIL means immediate error, no callback.
    */
    return(OK);
}


/*
**  Name: GCwinsock2_open
**  Description:
**	Open the listen socket for WinSock 2.2. Called from GCwinsock2(). This
**	routine should only be called once at server startup.
**
**  History:
**	16-oct-2003 (somsa01)
**	    Created.
**	09-feb-2004 (somsa01)
**	    When working with an instance identifier as a port address, make
**	    sure we start with only the first two characters. The subport is
**	    calculated from within the driver's addr_func() function.
**	20-feb-2004 (somsa01)
**	    Updated to account for multiple listen ports per protocol.
**      15-Feb-2005 (lunbr01)  Bug 113903, Problem EDJDBC 97
**          Do not clear the 3rd char of the input port when passed in
**          as XX# format.  Some callers, such as the JDBC server, pass in
**          an explicit symbolic port to use.
**      28-Aug-2006 (lunbr01) Sir 116548
**          Add IPv6 support.  Add call to getaddrinfo() to get list of
**          IP addresses that we should listen on--may be an IPv4 &
**          0-n IPv6 addresses.
**      11-Sep-2008 (lunbr01)
**          Add trace statements for all error conditions.
**	02-Mar-2009 (Bruce Lunsford) Bug 121742
**	    Make IO completion port usage (and hence worker threads)
**	    dependent on existence of GCwinsock2CompletionPort rather 
**	    than OS version.
**      06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Call GetLastError() instead of WSAGetLastError() after calling
**	    CreateEvent() and other non-winsock related functions.
**	    If using IO completion port, add 1 less extra thread
**	    than we used to for the listens since it is not needed.
**	    Also, add = to ck against MAX_WORKER_THREADS.
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Don't call WSAGetLastError() after getaddrinfo() failure as it
**	    returns a zero (0) status, which is confusing.  Just use the
**	    return value directly from getaddrinfo().  Also add call to
**	    gai_strerror() to get message text for error.
**	    Port will be zero (0) for local IPC listens; this will cause
**	    the bind to return an available non-zero port.  Grab the one
**	    assigned to the 1st address info entry returned by getaddrinfo()
**	    and then use it for any subsequent address info entries (if any).
**	    Also, the generated port must be returned to the caller rather
**	    than zero as the port number.
*/

STATUS
GCwinsock2_open(GCC_P_PLIST *parm_list)
{
    int			addr_reuse_on = 1;
    STATUS		status = OK;
    GCC_LSN		*lsn, *lsn_ptr;
    char		*proto = parm_list->pce->pce_pid;
    GCC_WINSOCK_DRIVER	*wsd;
    WS_DRIVER		*driver = (WS_DRIVER *)parm_list->pce->pce_driver;
    THREAD_EVENTS	*Tptr;
    char		*port  = parm_list->function_parms.open.port_id;
    char		port_id[GCC_L_PORT];
    int			i, num_port_tries;
#define			MAX_PORT_TRIES 100  /* if reach this, we're looping! */
    struct addrinfo	hints, *lpAddrinfo;
    struct sockaddr_storage ws_addr;
    int			ws_addr_len = sizeof(ws_addr);
    struct sockaddr_in *ws_addr_in_ptr = (struct sockaddr_in *)&ws_addr;
    char        	hoststr[NI_MAXHOST], servstr[NI_MAXSERV];
    SECURITY_ATTRIBUTES	sa;
    u_i2		port_num, port_num_assigned, sin_port_assigned = 0;

    parm_list->function_parms.open.lsn_port = NULL;

    /*
    ** See whether this is a "no listen" protocol.
    */
    if(parm_list->pce->pce_flags & PCT_NO_PORT)
	goto complete_open;

    Tptr = (THREAD_EVENTS *)parm_list->pce->pce_dcb;
    wsd = &Tptr->wsd;

    /*
    ** Create the GCC_LSN structure for this listen port.
    */
    lsn = (GCC_LSN *)MEreqmem(0, sizeof(GCC_LSN), TRUE, NULL);
    if (!lsn)
    {
	parm_list->generic_status = GC_OPEN_FAIL;
	SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
	status = FAIL;
	GCTRACE(1)( "GCwinsock2_open %s: %p Unable to allocate memory for GCC_LSN (size=%d)\n",
		proto, parm_list, sizeof(GCC_LSN));
	goto complete_open;
    }

    if ( (lsn->hListenEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
    {
	status = GetLastError();
	parm_list->generic_status = GC_OPEN_FAIL;
	SETWIN32ERR(&parm_list->system_status, status, ER_create);
	GCTRACE(1)( "GCwinsock2_open %s: %p Unable to create listen event, status=%d)\n",
		proto, parm_list, status);
	goto complete_open;
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
    ** Set up criteria for getting list of IPv* addrs to listen on.
    ** This will be used as the main input parm to getaddrinfo().
    */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = wsd->sock_fam;
    hints.ai_socktype = wsd->sock_type;
    hints.ai_protocol = wsd->sock_proto;
    hints.ai_flags    = AI_PASSIVE;

    /*
    ** Determine port to listen on.
    */
    if (STcompare(port, "II"))
    {
	STcopy(port, port_id);
    }
    else
    {
	char	*value;

	NMgtAt("II_INSTALLATION", &value);

	if (value && *value)
	    STcopy(value, port_id);
	else if (*parm_list->pce->pce_port != EOS)
	    STcopy(parm_list->pce->pce_port, port_id);
	else
	    STcopy(port, port_id);
    }

    /*
    ** Continue attempting to bind to a unique port id until we are
    ** successfull, or until we have retried an unreasonable number 
    ** of times.  If the latter occurs, it is likely because a specific
    ** numeric port was requested, in which case it won't be incremented,
    ** and someone else is listening on it.
    */
    for (num_port_tries = 0;;num_port_tries++)
    {
	if (num_port_tries > MAX_PORT_TRIES)
	{
	    parm_list->generic_status = GC_OPEN_FAIL;
	    SETWIN32ERR(&parm_list->system_status, status, ER_bind);
	    GCTRACE(1)( "GCwinsock2_open %s: %p error - MAX_PORT_TRIES exceeded (=%d)\n", 
			proto, parm_list, num_port_tries);
	    goto complete_open;
	}

	/*
	** Call the protocol-specific adressing function.
	** Pass NULL in for node which will be taken to mean the address is
	** local host.  On 2nd and subsequent calls, this routine will 
	** increment the port number by 1 if a symbolic port was requested.
	*/
	status = (*driver->addr_func)(NULL, port_id, lsn->port_name);
	if (status == FAIL)
	{
	    parm_list->generic_status = GC_OPEN_FAIL;
	    SETWIN32ERR(&parm_list->system_status, WSAEADDRINUSE, ER_badadar);
	    GCTRACE(1)( "GCwinsock2_open %s: %p addr() failed on port_id=%s, status=%d\n",
			proto, parm_list, port_id, status);
	    goto complete_open;
	}
	GCTRACE(1)( "GCwinsock2_open %s: %p addr() - port=%s(%s)\n",
			proto, parm_list, port_id, lsn->port_name);

	port_num = atoi(lsn->port_name);

	/*
	** If this is a "retry" attempt, there will already be an
	** addrinfo_list and associated structures from prior attempt; if so,
	** free up the memory allocated to them because we want to start 
	** with fresh, clean structures.  We could probably use the 
	** existing ones and just scan and replace the port field 
	** wherever needed, but why take a chance messing with these 
	** system structures, particularly when supporting different 
	** protocols (IPv4 and IPv6) and hence different addr structs.
	*/
	GCwinsock2_close_listen_sockets(lsn);

	/*
	** Get a list of qualifying addresses...may be a mix of IPv6 & IPv4.
	*/
	status = getaddrinfo(NULL, lsn->port_name, &hints, &lsn->addrinfo_list);
	if (status != 0)
	{
	    /*
	    ** Use status from getaddrinfo() directly; do not call
	    ** WSAGetLastError() as it returns a zero status, which
	    ** is confusing if there is an error.  Also, gai_strerror
	    ** is safe to call at this point even though it is not
	    ** thread-safe as this GCC_OPEN code is only called by
	    ** initialization.
	    */
	    parm_list->generic_status = GC_OPEN_FAIL;
	    SETWIN32ERR(&parm_list->system_status, status, ER_getaddrinfo);
	    GCTRACE(1)( "GCwinsock2_open %s: %p getaddrinfo() failed on port=%s, status=%d(%s)\n",
			proto, parm_list, lsn->port_name,
			status, gai_strerror(status));
	    goto complete_open;
	}

	/*
	** Ensure at least one address returned.
	*/
	if (lsn->addrinfo_list == NULL)
	{
	    parm_list->generic_status = GC_OPEN_FAIL;
	    SETWIN32ERR(&parm_list->system_status, status, ER_getaddrinfo);
	    status = FAIL;
	    GCTRACE(1)( "GCwinsock2_open %s: %p getaddrinfo() returned no addresses for port=%s\n",
			proto, parm_list, lsn->port_name);
	    goto complete_open;
	}

	/*
	** Count number of addresses returned.
	*/
	lsn->num_sockets = 0;
	for (lpAddrinfo = lsn->addrinfo_list; 
	     lpAddrinfo;
             lpAddrinfo = lpAddrinfo->ai_next)
	{
            lsn->num_sockets++;
	}
	GCTRACE(1)( "GCwinsock2_open %s: %p getaddrinfo() succeeded on port=%s, num addresses=%d\n",
			proto, parm_list, lsn->port_name, lsn->num_sockets);

	/*
	** Allocate space for the listen sockets
	*/
	lsn->listen_sd = (SOCKET *)MEreqmem( 0, sizeof(SOCKET) * lsn->num_sockets,
						TRUE, NULL );
	if (lsn->listen_sd == NULL)
	{
	    parm_list->generic_status = GC_OPEN_FAIL;
	    SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
	    GCTRACE(1)( "GCwinsock2_open %s: %p Unable to allocate memory for (%d) sockets\n",
		    proto, parm_list, lsn->num_sockets);
	    goto complete_open;
	}

	/*
	** Create, bind and listen on sockets for each address in the array.
	** If at least one is successful, then consider successful and 
	** start up.  NOTE that we may have to go through the loop 
	** several times until a unique port id is found; this is primarily
	** to handle multiple comm server instances(processes). 
	*/
	lsn->num_good_sockets = 0;
	for (i = 0, lpAddrinfo = lsn->addrinfo_list; 
	     i < lsn->num_sockets; 
	     i++, lpAddrinfo = lpAddrinfo->ai_next)
	{
	    /* if port was zero, use port assigned to 1st good @ for other @s */
	    if( port_num == 0 )
		((struct sockaddr_in *)(lpAddrinfo->ai_addr))->sin_port = sin_port_assigned;

	    /*
	    ** Create the socket.
	    */
	    lsn->listen_sd[i] = INVALID_SOCKET;
	    if ((lsn->listen_sd[i] = WSASocket(lpAddrinfo->ai_family, 
                                               lpAddrinfo->ai_socktype, 
                                               lpAddrinfo->ai_protocol,
					       NULL, 0,
					       WSA_FLAG_OVERLAPPED)
		) == INVALID_SOCKET)
	    {
		status = WSAGetLastError();
		parm_list->generic_status = GC_OPEN_FAIL;
		SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		GCTRACE(1)( "GCwinsock2_open %s: %p WSASocket() failed for listen socket[%d], status=%d. family=%d, socktype=%d, protocol=%d\n",
			proto, parm_list, i, status,
			lpAddrinfo->ai_family,
			lpAddrinfo->ai_socktype,
			lpAddrinfo->ai_protocol);
		goto complete_open;
	    }

	    /*   Non-IPv* special check.
	    ** Not sure if following still needed (still support IPX?)
	    ** but leaving in "just in case".
	    */
	    if ((lpAddrinfo->ai_family != AF_INET) &&
	        (lpAddrinfo->ai_family != AF_INET6))

	    {
	        /* Set the socket options to allow address reuse. */
	        if ((status = setsockopt(lsn->listen_sd[i], SOL_SOCKET,
			         SO_REUSEADDR, (u_caddr_t)&addr_reuse_on,
			         sizeof(addr_reuse_on))) != 0)
	        {
		    status = WSAGetLastError();
		    parm_list->generic_status = GC_OPEN_FAIL;
		    SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		    GCTRACE(1)( "GCwinsock2_open %s: %p setsockopt() for SO_REUSEADDR failed on listen socket[%d]=%p, status=%d\n",
			    proto, parm_list, i, lsn->listen_sd[i], status);
		    goto complete_open;
	        }
	    }  /*  End of Non-Internet Protocol special check  */

	    /*
	    ** Bind the socket to the address/port.
	    */
	    if (bind(lsn->listen_sd[i], 
		     lpAddrinfo->ai_addr, 
		     (int)lpAddrinfo->ai_addrlen) != 0) 
	    {
	        status = WSAGetLastError();
	        if (status == WSAEADDRINUSE)
	        {
		    GCTRACE(1)( "GCwinsock2_open %s: %p bind() port=%s already used...try next port\n", 
			    proto, parm_list, lsn->port_name);
		    break;
	        }
	        parm_list->generic_status = GC_OPEN_FAIL;
	        SETWIN32ERR(&parm_list->system_status, status, ER_bind);
		GCTRACE(1)( "GCwinsock2_open %s: %p bind() port=%s failed, status=%d\n", 
			proto, parm_list, lsn->port_name, status);
	        goto complete_open;
	    }

	    /*
	    ** If input port was zero, the bind() will have given us a new
	    ** port assigned by the system.  Save the port assigned to the
	    ** first address to use for subsequent addresses in list.
 	    ** Get the assigned port by using getsockname().
 	    */
	    if( ( port_num == 0 ) && ( sin_port_assigned == 0 ) )
	    {
		if (getsockname(lsn->listen_sd[i],
	    		    (struct sockaddr *)&ws_addr,
	    		    &ws_addr_len) != 0) 
		{
		    status = WSAGetLastError();
		    parm_list->generic_status = GC_OPEN_FAIL;
		    SETWIN32ERR(&parm_list->system_status, status, ER_getsockname);
		    GCTRACE(1)( "GCwinsock2_open %s: %p getsockname() failed on listen socket[%d]=%p, status=%d\n", 
			    proto, parm_list, i, lsn->listen_sd[i], status);
		    goto complete_open;
		}
		sin_port_assigned = ws_addr_in_ptr->sin_port;
		((struct sockaddr_in *)(lpAddrinfo->ai_addr))->sin_port = sin_port_assigned;
		port_num_assigned = ntohs( sin_port_assigned );
		STprintf( lsn->port_name, "%d", port_num_assigned );
	    }

	    if (GCwinsock2CompletionPort)
	    {
		/*
		** Associate listening socket to completion port.
		*/
		if (!CreateIoCompletionPort((HANDLE)lsn->listen_sd[i],
					    GCwinsock2CompletionPort,
					    (ULONG_PTR)0, 0))
		{
		    status = GetLastError();
		    parm_list->generic_status = GC_OPEN_FAIL;
		    SETWIN32ERR(&parm_list->system_status, status, ER_createiocompport);
		    GCTRACE(1)( "GCwinsock2_open %s: %p CreateIoCompletionPort() failed on listen socket[%d]=%p, status=%d\n", 
			    proto, parm_list, i, lsn->listen_sd[i], status);
		    goto complete_open;
		}
    	    }

	    /*
	    ** SPX specific setup 
	    ** This assumes not mixing other protocols in addrinfo list.
	    */
	    if (lpAddrinfo->ai_family == AF_IPX)
		GCwinsock2_open_setSPXport((SOCKADDR_IPX *)lpAddrinfo->ai_addr, lsn->port_name);

	    /*
	    ** OK, now listen...
	    */
	    if (listen(lsn->listen_sd[i], SOMAXCONN) != 0) 
	    {
		status = WSAGetLastError();
		parm_list->generic_status = GC_OPEN_FAIL;
		SETWIN32ERR(&parm_list->system_status, status, ER_listen);
		GCTRACE(1)( "GCwinsock2_open %s: %p listen failed for port %s, status=%d\n", 
			proto, parm_list, lsn->port_name, status );
		goto complete_open;
	    } 

	    GCTRACE(1)( "GCwinsock2_open %s: %p listening at %s\n",
		    proto, parm_list, lsn->port_name );
	    lsn->num_good_sockets++;

	    /*
	    ** Trace the address this socket is bound to.
	    ** ...but don't bother if proper tracing level not on.
	    */
	    if ((GCWINSOCK2_trace >= 1) &&
	        (getnameinfo(lpAddrinfo->ai_addr,
                             (socklen_t)lpAddrinfo->ai_addrlen,
                             hoststr,
                             NI_MAXHOST,
                             servstr,
                             NI_MAXSERV,
                             NI_NUMERICHOST | NI_NUMERICSERV
                            ) == 0))
	    {
	        GCTRACE(1)( "GCwinsock2_open %s: %p socket=0x%p bound to address %s and port %s\n",
            		proto, parm_list, lsn->listen_sd[i], hoststr, servstr);
	    }

	}  /*** End of loop through the addresses ***/

	/*
	** If someone (another GCC process for instance) is already 
	** listening on our port, try again->port will be incremented by 1
	** by moving port to port_id and calling GCtcpip_addr() at loop start.
	*/

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

	/*
	** Did we get at least one good socket?  If not, error!
	*/
	if (lsn->num_good_sockets == 0)
	{
	    parm_list->generic_status = GC_OPEN_FAIL;
	    SETWIN32ERR(&parm_list->system_status, status, ER_socket);
	    GCTRACE(1)( "GCwinsock2_open %s: %p listen failed - no good sockets\n", 
			proto, parm_list);
	    goto complete_open;
	}

	/*
	** !!SUCCESS!!  We have a good listen on at least one of the sockets
	** in the address list...probably all of them.
	*/

	if (GCwinsock2CompletionPort)
	{
	/*
	** Add more worker threads...
	** to avoid unlikely, but possible, tie-up of IO Completion Port
	** worker threads in a pending listen complete status (OP_ACCEPT
	** entered and waiting in WFSO on next Listen request).
	** This could occur if multiple connects come in concurrently
	** for different addresses (IPv6 and IPv4 for instance); only
	** one net listen completes immediately while the other(s) are
	** put in the wait status until the LISTEN is reissued by the caller.
	** Hence, create one less than total # of good listen sockets
	** to handle this rare, but possible, concurrency situation.
	** Generally, most of the threads will be idle, and we only
	** create these threads once during initialization, so the
	** overhead is not significant.
	*/
	iimksec(&sa);
	for (i = 1; i < lsn->num_good_sockets; i++)
	{
	    HANDLE	ThreadHandle;
	    int		tid;

	    if (Tptr->NumWorkerThreads >= MAX_WORKER_THREADS)
		break;

	    ThreadHandle = (HANDLE)_beginthreadex(
			    &sa, GC_STACK_SIZE,
			    (LPTHREAD_START_ROUTINE)GCwinsock2_worker_thread,
			    GCwinsock2CompletionPort, 0, &tid);
	    if (ThreadHandle)
	    {
		GCTRACE(3)("GCwinsock2_open %s: %p Worker Thread [%d] tid = %d, Handle = %d\n",
			   proto, parm_list,
			   Tptr->NumWorkerThreads, tid, ThreadHandle);
		Tptr->hWorkerThreads[Tptr->NumWorkerThreads] = ThreadHandle;
		Tptr->NumWorkerThreads++;
	    }
	    else
	    {
		GCTRACE(1)("GCwinsock2_open %s: %p Couldn't create worker thread, errno = %d\n",
			   proto, parm_list, errno, strerror(errno) );
		CloseHandle(GCwinsock2CompletionPort);
		return(FAIL);
	    }
	} /* End foreach good listen, add worker thread */
	} /* End if GCwinsock2CompletionPort */

	break;

    }  /*** End of loop until unique port found ***/

    parm_list->generic_status = OK;

    /* get name for output */

    STcopy(port_id, port);
    parm_list->function_parms.open.lsn_port = lsn->port_name;
    STcopy(port_id, parm_list->pce->pce_port);
    STcopy(port_id, lsn->pce_port);

complete_open:
    /*
    ** Drive the completion routine.
    */
    if (parm_list->compl_exit)
        (*parm_list->compl_exit)(parm_list->compl_id);

    /* The driver should only return single error status. An error
    ** may be returned via callback or from the protocol driver routine
    ** not both. The wintcp driver seems to follow this protocol as
    ** expected by the upper layers. 
    ** Return OK from the protocol driver. Don't return the error status
    ** again.  
    */ 
    
    return(OK);
}


/*
**  Name: GCwinsock2_open_setSPXport
**  Description:
**	This routine adjusts the SPX port.  The code was split out from
**	GCwinsock2_open for clarity and separation from other protocols.
**
**  History:
**	28-aug-2006 (lunbr01)
**	    Created.
*/

VOID
GCwinsock2_open_setSPXport(SOCKADDR_IPX *spx_sockaddr, char *port_name)
{

    i4	i, j;

    for (i = 0, j = 0; i < 4; i++)
    {
	i4  byt;

	byt = spx_sockaddr->sa_netnum[i] & 0xFF;
	if ((byt & 0x0F) <= 9)
	    port_name[j+1] = (char)((byt & 0x0F) + '0');
	else
	    port_name[j+1] = (char)((byt & 0x0F) - 10 + 'A');
	byt >>= 4;
	if (byt <= 9)
	    port_name[j] = (char)(byt + '0');
	else
	    port_name[j] = (char)(byt - 10 + 'A');
	j += 2;
    }

    port_name[j++] = '.';
    for (i = 0; i < 6; i++)
    {
	i4	byt;

	byt = spx_sockaddr->sa_nodenum[i] & 0xFF;
	if ((byt & 0x0F) <= 9)
	    port_name[j+1] = (char)((byt & 0x0F) + '0');
	else
	    port_name[j+1] = (char)((byt & 0x0F) - 10 + 'A');
	byt >>= 4;
	if (byt <= 9)
	    port_name[j] = (char)(byt + '0');
	else
	    port_name[j] = (char)(byt - 10 + 'A');
	j += 2;
    }

    port_name[j] = EOS;
}


/*
**  Name: GCwinsock2_listen
**  Description:
**	This is the listen routine for Winsock 2.2. It runs an
**	asyncronous / synchronous accept() on the listen socket. This
**	routine is invoked each time GCwinsock() gets a request to repost
**	the listen.
**
**  History:
**	28-oct-2003 (somsa01)
**	    Created.
**	20-feb-2004 (somsa01)
**	    Updated to account for multiple listen ports per protocol.
**      16-Aug-2005 (loera01) Bug 115050, Problem INGNET 179
**          Don't retrieve the hostname via gethostbyaddr() if the 
**          connection is local.  Instead, copy the local hostname 
**          from GChostname().
**      26-Jan-2006 (Ralph Loen) Bug 115671
**          Added GCTCPIP_log_rem_host to optionally disable invocation of
**          gethostbyaddr().
**      28-Aug-2006 (lunbr01) Sir 116548
**          Add IPv6 support.
**      11-Sep-2008 (lunbr01)
**          Add trace statements for all error conditions.
**	26-Nov-2008 (Bruce Lunsford) Bug 121285
**	    Call new standalone function GCwinsock2_schedule_completion
**	    instead of having code inline.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Call new function GCwinsock2_get_pcb() rather than directly
**	    allocating and initializing a PCB.
**	    Keep track of requestor's thread id for later completion.
**	    At end in complete_listen, only return status from schedule
**	    completion, not status from this function; the reason is
**	    that successfully scheduling a completion and then returning
**	    a non-OK return here could cause caller confusion.
*/

STATUS
GCwinsock2_listen(VOID *rq_in)
{
    REQUEST_Q		*rq = (REQUEST_Q *)rq_in;
    GCC_P_PLIST		*parm_list = rq->plist;
    char		*proto = parm_list->pce->pce_pid;
    int			i, i_stop;
    SOCKET		sd;
    PCB2		*pcb;
    int			status = OK;
    int			status_sc;
    GCC_LSN		*lsn;
    GCC_WINSOCK_DRIVER	*wsd;
    THREAD_EVENTS	*Tptr;
    struct addrinfo	*lpAddrinfo;
    struct sockaddr_storage peer;
    int			peer_len = sizeof(peer);
    DWORD		dwBytes;
    PER_IO_DATA		*lpPerIoData;

    GCTRACE(2)("GCwinsock2_listen %s: %p \n", proto, parm_list);

    Tptr = (THREAD_EVENTS *)parm_list->pce->pce_dcb;
    wsd = &Tptr->wsd;

    /*
    ** Use the proper listen port.
    */
    lsn = wsd->lsn;
    while (lsn && STcompare(parm_list->pce->pce_port, lsn->pce_port) != 0)
	lsn = lsn->lsn_next;

    if (!lsn)
    {
	SETWIN32ERR(&parm_list->system_status, status, ER_listen);
	parm_list->generic_status = GC_LISTEN_FAIL;
	GCTRACE(1)("GCwinsock2_listen %s: %p No matching listen port(%s) in wsd->lsn at %p\n",
		proto, parm_list, parm_list->pce->pce_port, wsd->lsn);
	goto complete_listen;
    }

    lsn->parm_list = parm_list; /* Save for use by IO worker thread(s) */

    /*
    ** Initialize some listen parameters to NULL.
    */
    parm_list->function_parms.listen.node_id = NULL;
    parm_list->function_parms.listen.lcl_addr = NULL;
    parm_list->function_parms.listen.rmt_addr = NULL;

    /* 
    ** Initialize peer connection status to zero (remote).
    */
    parm_list->options = 0;

    /*
    ** Obtain a Protcol Control Block specific to this driver/port
    */
    if ( ((PCB2 *)parm_list->pcb = pcb = GCwinsock2_get_pcb(parm_list->pce, FALSE)) == NULL )
    {
	SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
	parm_list->generic_status = GC_LISTEN_FAIL;
	goto complete_listen;
    }

    if (!lpfnAcceptEx) goto normal_accept;

  /* 
  ** Set up a listen for each addr in addrinfo on the first time in.
  ** Thereafter, only one address will need to have the network listen
  ** reinvoked each time -- the one that satisfied and posted complete
  ** the last GCC_LISTEN request for this port.
  ** The tricky part about Listen processing is that more than one 
  ** incoming network connect request could have occurred (max of one 
  ** per address), but only one of them can service (post) each
  ** input GCC_LISTEN request.
  */
  if (lsn->addrinfo_posted)  /* if not first time in */
  {     /* Process only the address last completed/posted */
    lpAddrinfo=lsn->addrinfo_posted;
    i = lsn->socket_ix_posted;
    i_stop = i+1;
  }
  else  /* Process all the addresses ... first time in */
  {
    lpAddrinfo=lsn->addrinfo_list;
    i = 0;
    i_stop = lsn->num_sockets;
  }
  for (;
       i < i_stop;
       i++, lpAddrinfo = lpAddrinfo->ai_next)
  {
    /*
    ** Allocate a PER_IO_DATA structure and initialize.
    ** NOTE: parm_list set to NULL because may be gone when AcceptEx completes.
    */
    lpPerIoData = GCwinsock2_get_piod(OP_ACCEPT, NULL);
    if (lpPerIoData == NULL)
    {
	parm_list->generic_status = GC_LISTEN_FAIL;
	goto complete_listen;
    }

    lpPerIoData->block_mode = wsd->block_mode;
    lpPerIoData->listen_sd = lsn->listen_sd[i];
    lpPerIoData->lsn       = lsn;
    lpPerIoData->addrinfo_posted = lpAddrinfo;
    lpPerIoData->socket_ix_posted = i;
    lpPerIoData->ThreadId = rq->ThreadId;

    /*
    ** Create the client socket for the accepted connection.
    */
    if ((lpPerIoData->client_sd = WSASocket(lpAddrinfo->ai_family, 
					    lpAddrinfo->ai_socktype,
					    lpAddrinfo->ai_protocol, 
					    NULL, 0, WSA_FLAG_OVERLAPPED)
	) == INVALID_SOCKET)
    {
	status = WSAGetLastError();
	parm_list->generic_status = GC_LISTEN_FAIL;
	SETWIN32ERR(&parm_list->system_status, status, ER_socket);
	GCTRACE(1)("GCwinsock2_listen %s: %p ai[%d]=%p Create client socket failed with status %d. family=%d, socktype=%d, protocol=%d\n",
		proto, parm_list, i, lpAddrinfo, status,
		lpAddrinfo->ai_family,
		lpAddrinfo->ai_socktype,
		lpAddrinfo->ai_protocol);
	goto complete_listen;
    }

    if (!GCwinsock2CompletionPort)
    {
	status = GCwinsock2_add_connect_event(&lpPerIoData->Overlapped,
			                      lpPerIoData->client_sd);
	if (status != OK)
	{
	    SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
	    parm_list->generic_status = GC_LISTEN_FAIL;
	    GCTRACE(1)( "GCwinsock2_listen %s: %p GCwinsock2_add_connect_event() failed with status %d, socket=0x%p\n", 
		    proto, parm_list, status, lpPerIoData->client_sd);
	    goto complete_listen;
	}
    }

    if (!lpfnAcceptEx(lpPerIoData->listen_sd, lpPerIoData->client_sd, 
		      lpPerIoData->AcceptExBuffer, 0,
		      sizeof(SOCKADDR_STORAGE) + 16,
		      sizeof(SOCKADDR_STORAGE) + 16, &dwBytes,
		      &lpPerIoData->Overlapped))
    {
	status = WSAGetLastError();
	if (status != WSA_IO_PENDING)
	{
	    parm_list->generic_status = GC_LISTEN_FAIL;
	    SETWIN32ERR(&parm_list->system_status, status, ER_accept);
	    GCTRACE(1)("GCwinsock2_listen %s: %p ai[%d]=%p AcceptEx() failed with status %d. listen/client socket=0x%p/0x%p,\n",
		proto, parm_list, i, lpAddrinfo, status,
		lpPerIoData->listen_sd,
		lpPerIoData->client_sd);
	    GCTRACE(1)("GCwinsock2_listen . . . family=%d, socktype=%d, protocol=%d\n",
		lpAddrinfo->ai_family,
		lpAddrinfo->ai_socktype,
		lpAddrinfo->ai_protocol);
	    goto complete_listen;
	}
    }

    /*
    ** Do not run the completion routine, as the completion routine
    ** for the AcceptEx() call will do that for us when it is time.
    */

  } /* End of loop through listen addresses */

    /*
    ** The AcceptEx() on each Listen address has been issued.  These IOs
    ** will pend in the OS.  When a remote user connects, the IO will 
    ** complete and be given to a worker thread, which 
    ** will process the incoming connection as much as possible, but then
    ** will wait on the Listen event which is set here.  NOTE that only one
    ** worker thread will be awakened by the event; he will then pick up
    ** the GCC_LISTEN parm_list saved in the lsn block and post the
    ** completion routine for GCC_LISTEN.  The reason we can afford
    ** for the worker thread to wait on this event is because the wait
    ** time should be minimal as LISTENs are immediately redispatched
    ** by the caller (GCC for instance).  If this was not true, it would
    ** tie up worker threads, which need to be kept free to process
    ** completed network IO.
    */
    if (!SetEvent(lsn->hListenEvent))
    {
	status = GetLastError();
	parm_list->generic_status = GC_LISTEN_FAIL;
	SETWIN32ERR(&parm_list->system_status, status, ER_sevent);
	GCTRACE(1)("GCwinsock2_listen %s: %p SetEvent error = %d\n",
		proto, parm_list, status);
	goto complete_listen;
    }

    return(OK);

normal_accept:
    /*
    ** If we are here, then we are in a spawned thread that handles
    ** the listen for this port; this code is only used if IO Completion
    ** ports and the AcceptEx function are not supported, which is the
    ** case on Win95/98 and some network service providers (SPX perhaps?).
    ** Using a normal accept() will block, so if we have multiple addresses
    ** for this port, then we need to spawn a subthread for each address,
    ** or don't allow more than one address.  For now, limit to one address
    ** since there is no apparent need for multiple addresses per port
    ** capabilities when normal accepts must be used.
    */

    /*
    ** Use only the first if more than one address being listened on.  
    ** If determined to be a requirement later, can enhance at that point.
    /*
    if (lsn->num_sockets > 1)
    {
	GCTRACE(1)( "GCwinsock2_listen %s: %p WARNING--Multiple listen addresses returned, only listening on first\n since Winsock AcceptEx() function unavailable.  port=%s, #addrs=%d\n",
		   proto, parm_list, port, lsn->num_sockets);
    }

    /*
    ** Accept connections on the listen socket.
    */
    sd = accept(lsn->listen_sd[0], (struct sockaddr *)&peer, &peer_len);
    if (sd == INVALID_SOCKET) 
    {
	status = WSAGetLastError();
	SETWIN32ERR(&parm_list->system_status, status, ER_accept);
	parm_list->generic_status = GC_LISTEN_FAIL;
	GCTRACE(1)("GCwinsock2_listen %s: %p accept() failed with status %d. listen socket=0x%p\n",
		proto, parm_list, status, lsn->listen_sd[0]);
	goto complete_listen;
    }

    pcb->sd = sd;

    GCwinsock2_listen_complete(parm_list, NULL);

complete_listen:
    /*
    ** Drive the completion routine.
    */
    status_sc = GCwinsock2_schedule_completion(rq, NULL, 0);

    return(status_sc);
}


/*
**  Name: GCwinsock2_listen_complete
**  Description:
**	This is the listen completion routine for Winsock 2.2.
**	It is called when a socket has been accepted successfully
**	by ANY means: blocking accept or asyncronous (IO Completion Port).
**	This contains all the common code needed to finish setting up
**	the information related to an incoming connection.  Previously,
**	the same or similar code was in several places.
**
**  History:
**	19-sep-2006 (lunbr01)
**	    Created as part of IPv6 support to consolidate redundant code.
**      06-Feb-2007 (Ralph Loen) SIR 117590
**          Removed getnameinfo() and set the node_id field in the GCC
**          listen parms to NULL.
**      11-Sep-2008 (lunbr01)
**          Add trace statements for all error conditions.
**	02-Mar-2009 (Bruce Lunsford) Bug 121742
**	    Fix crash while connecting on Windows 2000 (and Win 9x/ME);
**	    allocate area in stack for Local and Remote sockaddr's for
**	    calls to getsockname() and getpeername().
**	28-Oct-2009 (Bruce Lunsford) Sir 122814
**	    If configured, set socket option TCP_NODELAY on inbound
**	    connections to potentially improve performance by disabling
**	    TCP Naegle algorithm, which can cause delays of up to 200 msec
**	    between sends.  Unix/Linux already sets the option except on
**	    OS-threaded inbound sessions (ie, in DBMS).
*/

VOID
GCwinsock2_listen_complete(GCC_P_PLIST *parm_list, char *AcceptExBuffer)
{
    PCB2		*pcb = (PCB2 *)parm_list->pcb;
    char		*proto = parm_list->pce->pce_pid;
    STATUS		retval;
    char		hostname[NI_MAXHOST]={0};
    SOCKADDR_STORAGE	LocalSockaddr, RemoteSockaddr;
    SOCKADDR		*lpLocalSockaddr=(SOCKADDR *)&LocalSockaddr;
    SOCKADDR		*lpRemoteSockaddr=(SOCKADDR *)&RemoteSockaddr;
    int			LocalSockaddrLen  = sizeof(SOCKADDR_STORAGE);
    int			RemoteSockaddrLen = sizeof(SOCKADDR_STORAGE);
    int			i;

    if (AcceptExBuffer && lpfnGetAcceptExSockaddrs)
    {
	lpfnGetAcceptExSockaddrs(AcceptExBuffer, 0,
			         sizeof(SOCKADDR_STORAGE) + 16,
			         sizeof(SOCKADDR_STORAGE) + 16,
			         (LPSOCKADDR *)&lpLocalSockaddr,
			         &LocalSockaddrLen,
			         (LPSOCKADDR *)&lpRemoteSockaddr,
			         &RemoteSockaddrLen);
    }
    else
    {
	getsockname(pcb->sd, lpLocalSockaddr,  &LocalSockaddrLen);
	getpeername(pcb->sd, lpRemoteSockaddr, &RemoteSockaddrLen);
    }


    if (((struct sockaddr_in6 *)lpLocalSockaddr)->sin6_family == AF_INET6)
    {
        if( MEcmp( &((struct sockaddr_in6 *)(lpLocalSockaddr))->sin6_addr,
            &((struct sockaddr_in6 *)(lpRemoteSockaddr))->sin6_addr,
                sizeof(((struct sockaddr_in6 *)(lpLocalSockaddr))->sin6_addr)) 
                    == 0 )
            parm_list->options |= GCC_LOCAL_CONNECT;
    }
    else
    {
        if( MEcmp( &((struct sockaddr_in *)(lpLocalSockaddr))->sin_addr,
            &((struct sockaddr_in *)(lpRemoteSockaddr))->sin_addr,
                sizeof(((struct sockaddr_in *)(lpLocalSockaddr))->sin_addr)) 
                    == 0 )
            parm_list->options |= GCC_LOCAL_CONNECT;
    }

    GCTRACE(2)( "GCwinsock2_listen_complete %s %d: %p Connection is %s on socket=0x%p\n",
		proto, pcb->id, parm_list,
                parm_list->options & GCC_LOCAL_CONNECT ? "local" : "remote",
		pcb->sd );

    parm_list->function_parms.listen.node_id = NULL;

    /*  Turn on KEEPALIVE and (optionally) NODELAY */
    i = 1;
    setsockopt(pcb->sd, SOL_SOCKET, SO_KEEPALIVE, (caddr_t)&i, sizeof(i));
    if (GCWINSOCK2_nodelay)
	setsockopt(pcb->sd, IPPROTO_TCP, TCP_NODELAY, (caddr_t)&i, sizeof(i));

    /*
    ** Set socket blocking status: 0=blocking, 1=non blocking.
    */
    i = 1;
    if (ioctlsocket(pcb->sd, FIONBIO, &i) == SOCKET_ERROR)
    {
	retval = WSAGetLastError();
	SETWIN32ERR(&parm_list->system_status, retval, ER_ioctl);
	parm_list->generic_status = GC_LISTEN_FAIL;
	GCTRACE(1)("GCwinsock2_listen_complete %s %d: %p ioctlsocket() set non-blocking failed with status %d, socket=0x%p\n",
		proto, pcb->id, parm_list, retval, pcb->sd);
	return;
    }
}


/*
**  Name: GCwinsock2_connect
**  Description:
**	This routine will set up and issue a connect request for
**	the requested target IP address (in addrinfo).
** 
**  History:
**	19-sep-2006 (lunbr01)
**	    Created as part of IPv6 support.
**      11-Sep-2008 (lunbr01)
**          Add trace statements for all error conditions.
**	02-Mar-2009 (Bruce Lunsford) Bug 121742
**	    Connections handled by routine "normal_connect" were failing
**	    with error=0 when tracing is on; ensure GCTRACE() not invoked
**	    between winsock calls and WSAGetLastError() because it overlays
**	    the returned status (such as WSAEWOULDBLOCK) with zero.
**	    Also must associate the socket with the IO completion port
**	    for W2K for the SENDs and RECEIVEs, even if ConnectEx() not
**	    available.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Keep track of requestor's thread id for later completion.
*/
STATUS
GCwinsock2_connect(GCC_P_PLIST *parm_list, char *inPerIoData)
{
    STATUS		status = OK;
    int			i, rval;
    char		*proto = parm_list->pce->pce_pid;
    char		*port = parm_list->function_parms.connect.port_id;
    char		*node = parm_list->function_parms.connect.node_id;
    PCB2		*pcb = (PCB2 *)parm_list->pcb;
    SOCKET		sd;
    struct addrinfo	*lpAddrinfo = pcb->lpAddrinfo;
    PER_IO_DATA		*lpPerIoData = (PER_IO_DATA *)inPerIoData;
    u_i2		port_num = ntohs( ((struct sockaddr_in *)(lpAddrinfo->ai_addr))->sin_port );
    struct sockaddr_storage ws_addr;
    int			ws_addr_len = sizeof(ws_addr);
    char                hoststr[NI_MAXHOST] = {0};

    /*
    ** If not first try, clean up from last iteration.
    ** The final iteration will get cleaned up in GCC_DISCONNECT.
    */
    if (pcb->sd != INVALID_SOCKET)
    {
	closesocket(pcb->sd);
	GCTRACE(2)( "GCwinsock2_connect %s %d: %p Closed prior (failed) socket %p\n", 
		proto, pcb->id, parm_list, pcb->sd );
    }

    /*
    ** Create the socket
    */
    if ((pcb->sd = WSASocket(lpAddrinfo->ai_family, 
                        lpAddrinfo->ai_socktype, 
                        lpAddrinfo->ai_protocol,
		        NULL, 0,
		        WSA_FLAG_OVERLAPPED)
	) == INVALID_SOCKET)
    {
	status = WSAGetLastError();
	SETWIN32ERR(&parm_list->system_status, status, ER_socket);
	parm_list->generic_status = GC_CONNECT_FAIL;
	GCTRACE(1)( "GCwinsock2_connect %s %d: %p WSASocket() failed with status %d. family=%d, socktype=%d, protocol=%d\n", 
		proto, pcb->id, parm_list,
		status,
		lpAddrinfo->ai_family,
		lpAddrinfo->ai_socktype,
		lpAddrinfo->ai_protocol);
	return(FAIL);
    }

    sd = pcb->sd;

    /*
    ** Set socket blocking status.  0=blocking, 1=non blocking.
    */
    i = 1;
    if (ioctlsocket(sd, FIONBIO, &i) == SOCKET_ERROR)
    {
	status = WSAGetLastError();
	SETWIN32ERR(&parm_list->system_status, status, ER_ioctl);
	parm_list->generic_status = GC_CONNECT_FAIL;
	GCTRACE(1)( "GCwinsock2_connect %s %d: %p ioctlsocket() set non-blocking failed with status %d, socket=0x%p\n", 
		proto, pcb->id, parm_list, status, sd);
	return(FAIL);
    }

    GCTRACE(2)( "GCwinsock2_connect %s %d: %p connecting to %s(%s) port %s(%d)\n",
	    proto, pcb->id, parm_list, node,
	    GCwinsock2_display_IPaddr(lpAddrinfo,hoststr),
	    port, port_num );
/* FIX: Add GCwinsock2_display_IPaddr() to other GCTRACE calls below if it works */

    /*
    ** Associate client socket to completion port.
    */
    if ((GCwinsock2CompletionPort)
    &&  (!CreateIoCompletionPort((HANDLE)sd,
			        GCwinsock2CompletionPort,
			        (ULONG_PTR)0, 0)))
    {
	status = WSAGetLastError();
	SETWIN32ERR(&parm_list->system_status, status,
		    ER_createiocompport);
	parm_list->generic_status = GC_CONNECT_FAIL;
	GCTRACE(1)( "GCwinsock2_connect %s %d: %p CreateIoCompletionPort() failed with status %d, socket=0x%p\n", 
		proto, pcb->id, parm_list, status, sd);
	return(FAIL);
    }

    if (!lpfnConnectEx) goto normal_connect;

    if (!GCwinsock2CompletionPort)
    {
	status = GCwinsock2_add_connect_event(&lpPerIoData->Overlapped, sd);
	if (status != OK)
	{
	    SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
	    parm_list->generic_status = GC_CONNECT_FAIL;
	    GCTRACE(1)( "GCwinsock2_connect %s %d: %p GCwinsock2_add_connect_event() failed with status %d, socket=0x%p\n", 
		    proto, pcb->id, parm_list, status, sd);
	    return(FAIL);
	}
    }

    /*
    ** Bind the socket.  This is required by ConnectEx(),
    ** but not normal connect().  
    ** Bind to port=0, addr=ADDR_ANY(=0x00000000); zeroing the
    ** contents of the sockaddr struct (regardless of protocol)
    ** accomplishes this without having to deal with the diff formats.
    */
    ZeroMemory(&ws_addr, sizeof(struct sockaddr_storage));
    ws_addr.ss_family = lpAddrinfo->ai_family;
    if (bind(sd, (struct sockaddr *)&ws_addr, ws_addr_len) != 0)
    {
	status = WSAGetLastError();
	SETWIN32ERR(&parm_list->system_status, status, ER_bind);
	parm_list->generic_status = GC_CONNECT_FAIL;
	GCTRACE(1)("GCwinsock2_connect %s %d: %p bind() error = %d\n",
		    proto, pcb->id, parm_list, status);
	return(FAIL);
    }

    /*
    ** Connect to the remote target asynchronously.
    */
    if (!lpfnConnectEx(sd, lpAddrinfo->ai_addr, lpAddrinfo->ai_addrlen, NULL, 0,
			NULL, &lpPerIoData->Overlapped))
    {
	status = WSAGetLastError();
	if (status == WSA_IO_PENDING || status == OK)
	    return(OK);
	if ((status == WSAECONNREFUSED) ||
	    (status == WSAENETUNREACH)  ||
	    (status == WSAETIMEDOUT))
	    return(FAIL);
	SETWIN32ERR(&parm_list->system_status, status, ER_connect);
	parm_list->generic_status = GC_CONNECT_FAIL;
	GCTRACE(1)("GCwinsock2_connect %s %d: %p ConnectEx() error = %d\n",
		    proto, pcb->id, parm_list, status);
	return(FAIL);
    }
    return(OK);

normal_connect:

    rval = connect(sd, lpAddrinfo->ai_addr, lpAddrinfo->ai_addrlen);

    if (rval == SOCKET_ERROR)
    {
	status = WSAGetLastError();
	GCTRACE(2)("GCwinsock2_connect %s %d: %p connect() SOCKET_ERROR status = %d\n",
		    proto, pcb->id, parm_list, status);
	if (status == WSAEWOULDBLOCK)
	{
	    fd_set		check_fds;
	    struct timeval	tm;

	    /*
	    ** State is completing. Use select() -- when socket
	    ** is writeable the connect has completed.
	    */
	    status = OK;
	    tm.tv_sec = GCWINSOCK2_timeout;
	    tm.tv_usec = 0;
	    FD_ZERO(&check_fds);
	    FD_SET(sd, &check_fds);

	    rval = select((int)1, NULL, &check_fds, NULL, &tm);
	    if (rval == SOCKET_ERROR)
	    {
		status = WSAGetLastError();
		if (status != WSAEINPROGRESS)
		{
	            GCTRACE(1)("GCwinsock2_connect %s %d: %p select() error = %d\n",
			        proto, pcb->id, parm_list, status);
		    return(FAIL);
		}

		status = OK;
	    }
	    else if (rval == 0)
	    {
		/* Connection timed out */
	        GCTRACE(1)("GCwinsock2_connect %s %d: %p timeout after %d secs\n",
			    proto, pcb->id, parm_list, tm.tv_sec);
		return(FAIL);
	    }
	}
	else  /* Not WSAEWOULDBLOCK status, real failure to connect */
	{
	    GCTRACE(1)("GCwinsock2_connect %s %d: %p connect() error = %d\n",
		    proto, pcb->id, parm_list, status);
	    return(FAIL);
	}
    }  /* end if connect returned a SOCKET_ERROR */

    GCTRACE(2)("GCwinsock2_connect %s %d: %p connect() completed; call ...connect_complete()\n",
	    proto, pcb->id, parm_list);

    GCwinsock2_connect_complete(parm_list);

    return(status);
}


/*
**  Name: GCwinsock2_connect_complete
**  Description:
**	This is the connect completion routine for Winsock 2.2.  It is
**	called when a socket has successfully connected to a remote node 
**	by ANY means: blocking connect or asyncronous (IO Completion Port).
**	This contains all the common code needed to finish setting up
**	the information related to an outgoing connection.
**
**  History:
**	19-sep-2006 (lunbr01)
**	    Created as part of IPv6 support to consolidate redundant code.
**      11-Sep-2008 (lunbr01)
**          Add trace statements for all error conditions.
**	28-Oct-2009 (Bruce Lunsford) Sir 122814
**	    If configured, set socket option TCP_NODELAY on outbound
**	    connections to potentially improve performance by disabling
**	    TCP Naegle algorithm, which can cause delays of up to 200 msec
**	    between sends.  Unix/Linux already sets the option except on
**	    OS-threaded inbound sessions (ie, in DBMS).
*/

VOID
GCwinsock2_connect_complete(GCC_P_PLIST *parm_list)
{
    PCB2		*pcb = (PCB2 *)parm_list->pcb;
    char		*proto = parm_list->pce->pce_pid;
    struct addrinfo	*lpAddrinfo = pcb->lpAddrinfo;
    STATUS		retval;
    struct sockaddr_storage  ws_addr;
    int			ws_addr_len = sizeof(ws_addr);
    int			i;

    /*
    ** Set up the local and remote address information.
    */
    getsockname(pcb->sd, (struct sockaddr *)&ws_addr, &ws_addr_len);
    if (getnameinfo((struct sockaddr *)&ws_addr, ws_addr_len,
                    pcb->lcl_addr.node_id,
                    sizeof(pcb->lcl_addr.node_id),
                    pcb->lcl_addr.port_id,
                    sizeof(pcb->lcl_addr.port_id),
                    NI_NUMERICHOST | NI_NUMERICSERV) != 0)
    {
	retval = WSAGetLastError();
	parm_list->generic_status = GC_CONNECT_FAIL;
	SETWIN32ERR(&parm_list->system_status, retval, ER_getnameinfo);
	GCTRACE(1)("GCwinsock2_connect_complete %s %d: %p getnameinfo(local) error = %d\n",
		   proto, pcb->id, parm_list, retval);
	return;
    }

    if (getnameinfo(lpAddrinfo->ai_addr, lpAddrinfo->ai_addrlen,
                    pcb->rmt_addr.node_id,
                    sizeof(pcb->rmt_addr.node_id),
                    pcb->rmt_addr.port_id,
                    sizeof(pcb->rmt_addr.port_id),
                    NI_NUMERICHOST | NI_NUMERICSERV) != 0)
    {
	retval = WSAGetLastError();
	parm_list->generic_status = GC_CONNECT_FAIL;
	SETWIN32ERR(&parm_list->system_status, retval, ER_getnameinfo);
	GCTRACE(1)("GCwinsock2_connect_complete %s %d: %p getnameinfo(remote) error = %d\n",
		   proto, pcb->id, parm_list, retval);
	return;
    }

    /*  Turn on KEEPALIVE and (optionally) NODELAY */
    i = 1;
    setsockopt(pcb->sd, SOL_SOCKET, SO_KEEPALIVE, (u_caddr_t)&i, sizeof(i));
    if (GCWINSOCK2_nodelay)
	setsockopt(pcb->sd, IPPROTO_TCP, TCP_NODELAY, (u_caddr_t)&i, sizeof(i));

    return;

}


/*
**  Name: GCwinsock2_disconnect_thread
**  Description:
**	This routine is started as a separate background thread which
**	handles disconnects that have asyncronous I/O (receives and sends) 
**	pending.  The disconnects are put on a queue by the mainline
**	thread and then an event set to wake up this thread.  This
**	function will check each session's pending I/O count and
**	when it drops to zero, post the completion routine for the
**	disconnect request.  After all the pending disconnects have
**	been checked, if any still have pending I/O and there are no
**	new disconnects to process from the input queue, the thread
**	thread goes into a short sleep to allow more I/O to complete
**	and then starts the cycle all over again.  The thread does
**	not terminate until the process terminates.
**
**	There is a point at which we "give up" waiting for the
**	I/Os to complete and go ahead and schedule the disconnect
**	completion.  This is intended to prevent the possibility
**	of orphan sessions in the disconnect Q hanging around
**	indefinitely and tying up sessions in the GC* server.
**	See NOTE below related to DISCONNECT_ASYNC_WAITS_MAX_LOOPS.
**
**  History:
**	26-Nov-2008 (Bruce Lunsford) Bug 121285
**	    Created to handle disconnects with pending I/O in a dedicated
**	    thread rather than tieing up one or more completion I/O
**	    worker threads. Replaces OP_DISCONNECT in worker thread logic.
**	02-Mar-2009 (Bruce Lunsford) Bug 121742
**	    Ensure GCTRACE() not invoked between OS or winsock calls
**	    and [WSA]GetLastError() because tracing overlays the returned
**	    status (such as WSAEWOULDBLOCK) with zero.
*/

VOID
GCwinsock2_disconnect_thread()
{
    DWORD	wait_stat;
    DWORD	wait_time = INFINITE;
    STATUS	status=OK;
    QUEUE 	*q;
/*
** NOTE: The max time to wait for async I/Os to complete was bumped from
** its old value of 1 sec (100 msec/sleep * 10 sleeps) to 10 seconds.
** This is to be "extra safe" since not waiting long enough will likely
** cause the GC* server to crash with an AccVio when the pending I/O completes.
** In testing with the new code (closesocket() invoked earlier and not using
** worker threads for the disconnect sleep loop), the # of times needed to
** sleep was usually 0 and never exceeded 1 (ie, 1/10 of a second), so setting
** it to 100 should be more than adequate.
** Making it too long will delay disconnect completion...unlikely to be
** noticeable though since disconnects are handled concurrently and
** asynchronously on both ends of the pipe...so the other end should not
** be waiting.  However, it does tie up a connection in the GC* server
** until the disconnect is posted complete.
** An even safer alternative (but it seems a bit extreme) would be to 
** set the total max wait/sleep time to the network TCP/IP default
** TIME_WAIT time of 120 seconds.
*/
#define	DISCONNECT_ASYNC_WAITS_SLEEP		100  /* millisec  to sleep */
#define	DISCONNECT_ASYNC_WAITS_MAX_LOOPS	100  /* max times to sleep */

    GCTRACE(4)( "GCwinsock2_disconnect_thread: started.\n" );

    while (TRUE)
    {
	/*
	** Check if any new incoming disconnect requests.
	**
	** Main thread GCC_DISCONNECT will set the disconnect event
	** when it has added more disconnects to the "in" Q to process.
	** On the 1st time in and anytime there are no disconnects to process,
	** just WAIT until the disconnect event is signalled to pick up
	** the new disconnect requests.
	** Otherwise, if there are still disconnects being processed (ie,
	** not completed), don't WAIT but just check to see if any new
	** disconnects have come in (handled with wait_time set to zero).
	*/
	GCTRACE(4)("GCwinsock2_disconnect_thread: %s event for new disconnects\n",
		    wait_time ? "wait on" : "check");
	wait_stat = WaitForSingleObject( hDisconnectEvent, wait_time );
	switch(wait_stat)
	{
	    case WAIT_OBJECT_0:  /* Event signalled */
		GCTRACE(4)("GCwinsock2_disconnect_thread: WAIT_OBJECT_0, handle=%d\n",
			    hDisconnectEvent );
		/*
		** Move all disconnect requests from incoming Q to internal Q.
		** Must protect from simultaneous adds of disconnect requests to
		** incoming Q by main thread (using critical section).
		*/
		EnterCriticalSection( &DisconnectCritSect );
		for ( q = DisconnectQ_in.q_next;
	  	      q != &DisconnectQ_in;
		      q = DisconnectQ_in.q_next )
		{
		    QUremove( q );
		    QUinsert( q, &DisconnectQ );
		}
		LeaveCriticalSection( &DisconnectCritSect );
		break;

	    case WAIT_TIMEOUT:  /* Event timed out...not signalled */
		GCTRACE(4)("GCwinsock2_disconnect_thread: WAIT_TIMEOUT, handle=%d\n",
			    hDisconnectEvent );
		/*
		** Timeout could not occur if the wait time was INFINITE;
		** therefore, wait time must have been zero, meaning
		** we were just checking for more work (new incoming requests)
		** after having processed the disconnect Q and still having
		** some entries in the Q.  It is time to sleep a little to
		** let some I/Os complete before looping through the 
		** disconnect Q again.
		*/
		Sleep(DISCONNECT_ASYNC_WAITS_SLEEP);
		break;

	    default:
        	GCTRACE(1)("GCwinsock2_disconnect_thread: wait failed %d\n",
                    	GetLastError() );
		break;
	} /* End switch on wait_stat */

	/*
	** Loop through all disconnect requests in disconnect Q and schedule
	** completion routine for each session that no longer has pending
	** I/O.
	*/
	for ( q = DisconnectQ.q_next;
	      q != &DisconnectQ; )
	{
	    REQUEST_Q *rq = (REQUEST_Q *)q;
	    GCC_P_PLIST *parm_list = rq->plist;
	    PCB2 *pcb = (PCB2 *)parm_list->pcb;
	    q = q->q_next;		/* Bump/save next Q entry address */
	    GCTRACE(5)( "GCwinsock2_disconnect_thread %s check disconnect: parm_list=%p, pcb=%d(0x%p)\n",
			 parm_list->pce->pce_pid, 
			 parm_list, pcb->id, pcb );
	    if (pcb)
	    {
		/*
		** If I/O on this socket is still pending,
		** don't schedule disconnect yet, UNLESS we've
		** waited (slept) long enough.
		*/
		GCTRACE(5)( "GCwinsock2_disconnect_thread %s check I/Os complete: parmlist=%p, pcb=%d(0x%p), Pending I/Os=%d\n",
			     parm_list->pce->pce_pid, 
			     parm_list, pcb->id, pcb,
			     pcb->AsyncIoRunning );
		if (wait_stat == WAIT_TIMEOUT) /* Means we slept above */
		    pcb->DisconnectAsyncIoWaits++;
		if (pcb->AsyncIoRunning > 0)
		{
		    if (pcb->DisconnectAsyncIoWaits <= DISCONNECT_ASYNC_WAITS_MAX_LOOPS )
			continue;  /* Don't disconnect this session yet */
		    else
			GCTRACE(1)( "GCwinsock2_disconnect_thread %s WARNING: disconnect completion scheduled with pending I/Os\n (parmlist%p, pcb=%d(0x%p), Pending I/Os=%d\n",
			         parm_list->pce->pce_pid, 
			         parm_list, pcb->id, pcb,
			         pcb->AsyncIoRunning );
		}

		/*
		** No IOs pending or max wait/sleep time exceeded...
		** so start disconnecting the session.
		*/
		GCwinsock2_rel_pcb(pcb);
	    } /* End if pcb exists */

	    /*
	    ** Drive completion routine for disconnect
	    */
	    GCTRACE(5)( "GCwinsock2_disconnect_thread %s schedule completion: parm_list=%p, pcb=%p\n",
			 parm_list->pce->pce_pid, 
			 parm_list, pcb );
	    parm_list->pcb = NULL;
	    QUremove(&rq->req_q);  /* Remove from disconnect Q */
	    status = GCwinsock2_schedule_completion(rq, NULL, 0);

	} /* End for each request in disconnect Q */

	if ( DisconnectQ.q_next == &DisconnectQ )
	{
	    /*
	    ** Disconnect Q is now empty; go to top and wait for more 
	    ** disconnects to come in.
	    */
	    wait_time = INFINITE;
	}
	else
	{
	    /*
	    ** We've gone through all the disconnects in the Q, but there
	    ** are still some left with pending I/O.  Before sleeping and
	    ** then going thru the Q again, check to see (without WAITing)
	    ** if more incoming disconnects are ready to be processed.
	    */
	    wait_time = 0;
	}

    } /* End while TRUE...loop back to top. */

}


/*
**  Name: GCwinsock2_display_IPaddr
**  Description:
**	This routine will convert the IP address in the sockaddr
**	structure to a display format and return it as a string.
**	To avoid leaking memory, the 2nd input parm must specify
**	an area to place the display form of the IP address, which
**	is also then used as the return value of the function.
**	The output area must be large enough to fit the largest
**	value including string termination character (that is, 
**	length should be NI_MAXHOST or longer).
** 
**  History:
**	19-sep-2006 (lunbr01)
**	    Created as part of IPv6 support.
*/
char *
GCwinsock2_display_IPaddr(struct addrinfo *lpAddrinfo, char *IPaddr_out)
{
    if (getnameinfo(lpAddrinfo->ai_addr,
                    (socklen_t)lpAddrinfo->ai_addrlen,
                    IPaddr_out,
                    NI_MAXHOST,
                    NULL,
                    0,
                    NI_NUMERICHOST) != 0)
	STcopy("Bad IP Address", IPaddr_out);
    return(IPaddr_out);
}


/*
**  Name: GCwinsock2_exit
**  Description:
**	This function exits from the Winsock 2.2 protocol, cleaning up.
**
**  History:
**	30-oct-2003 (somsa01)
**	    Created.
**	20-feb-2004 (somsa01)
**	    Updated to account for multiple listen ports per protocol.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Add logic to terminate and close the Disconnect Thread
**	    (missing from fix for bug 121285). Free resources for
**	    event array (if exists) used for async AIO connects/listens.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add PCT entry as an input parm since we may be called from
**	    gcacl, which has a different PCT than GCC; ie, can't use
**	    GCwinsock2_get_thread() to find it since it only accesses
**	    the global GCC PCT.  Don't release shared resources until
**	    all users of the driver have called this exit function.
**	    Return STATUS rather than VOID since caller expects it (and
**	    always did).
*/
STATUS
GCwinsock2_exit(GCC_PCE * pptr)
{
    int			i = 1, j;
    GCC_WINSOCK_DRIVER	*wsd;
    THREAD_EVENTS	*Tptr;
    char		*proto = pptr->pce_pid;

    GCTRACE(1)("GCwinsock2_exit %s: Entered.  use_count(this driver,winsock)=(%d,%d)\n",
		proto, GCwinsock2_use_count, WSAStartup_called);

    /*
    ** Decrement "use count"s for this driver and for winsock in general
    ** (the latter is also used by previous winsock 1.1 driver).
    */
    GCwinsock2_use_count--;
    WSAStartup_called--;
    GCwinsock2_shutting_down = TRUE;

    /*
    ** Get pointer to WinSock 2.2 protocol's control entry in table.
    */
    Tptr = (THREAD_EVENTS *)pptr->pce_dcb;

    /*
    ** Close listen sockets and various associated threads for this
    ** driver instance.
    */
    {
	GCC_LSN	*lsn;

	wsd = &Tptr->wsd;
	lsn = wsd->lsn;
	if (lsn)
	    GCTRACE(2)("GCwinsock2_exit %s: Close Listen sockets and threads.\n", proto);
	while (lsn)
	{
	    GCwinsock2_close_listen_sockets(lsn);
	    if (lsn->hListenThread)
	    {
		TerminateThread(lsn->hListenThread, 0);
		CloseHandle(lsn->hListenThread);
	    }

	    lsn = lsn->lsn_next;
	}

	if (Tptr->NumWorkerThreads)
	    GCTRACE(2)("GCwinsock2_exit %s: Close worker threads.\n", proto);
	for (j = 0; j < Tptr->NumWorkerThreads; j++)
	{
	    TerminateThread(Tptr->hWorkerThreads[j], 0);
	    CloseHandle(Tptr->hWorkerThreads[j]);
	}
    }

    /*
    ** Don't cleanup common/shared resources until the use count goes to zero.
    */
    if (GCwinsock2_use_count > 0)
	return(OK);

    /*
    ** Close the IO Completion Port, if created/used.
    */
    if (GCwinsock2CompletionPort)
    {
	CloseHandle(GCwinsock2CompletionPort);
	GCwinsock2CompletionPort = 0;
    }

    /*
    ** Close Disconnect thread.
    */
    if (hDisconnectThread)
    {
	GCTRACE(2)("GCwinsock2_exit %s: Close Disconnect thread.\n", proto);
	TerminateThread(hDisconnectThread, 0);
	CloseHandle(hDisconnectThread);
	hDisconnectThread = 0;
    }

    /*
    ** Cleanup arrays (if exists) for AIO async connects.
    ** Close event handles and free memory.
    */
    if (arrayConnectEvents)
    {
	GCTRACE(2)("GCwinsock2_exit %s: Cleanup/free AIO async connect array.\n", proto);
	for (i = 0; i < MAX_PENDING_CONNECTS; i++)
	{
	    CloseHandle(arrayConnectEvents[i]);
	}

	MEfree((PTR)arrayConnectEvents);
	arrayConnectEvents = NULL;
	MEfree((PTR)arrayConnectInfo);
	arrayConnectInfo = NULL;
	GCTRACE(1)("GCwinsock2_exit: AIO pending connect/listens: current=%d, max=%d, highwater=%d\n",
		    numPendingConnects, MAX_PENDING_CONNECTS, highPendingConnects);
	GCTRACE(1)("GCwinsock2_exit: AIO queued  connect/listens: current=%d, total(since startup)=%d\n",
		    numAIOConnectQ, totalAIOConnectQ);
    }

    /*
    ** Don't cleanup/shutdown winsock until no driver instance/thread
    ** is using it.  Ie, last one out the door turns out the lights.
    ** NOTE that winsock is a shared resource/facility with the prior
    ** winsock 1.1 driver (gcwinsck.c) and hence used by WINTCP protocol.
    */
    if (WSAStartup_called == 0)
	WSACleanup();

    return(OK);
}


/*
**  Name: GCwinsock2_save
**  Description:
**	This function is called externally during GCsave operation
**	to save protocol-specific information needed to "restore"
**	the session under a different process.  The new (other) process
**	will call complementary function GCwinsock2_restore() to
**	establish communications on the same socket/connection set
**	up by the original process.
**
**  Inputs:
**	buffer 		Pointer to buffer into which to save information
**	buf_len_in	Pointer to length of buffer area.
**
**  Outputs: None
**	*buf_len_in	Length of data saved into buffer.
**
**  Returns:
**	OK    if success
**	FAIL  if error occurred...such as buffer not large enough
**
**  History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created for GCsave local IPC processing.
*/
STATUS
GCwinsock2_save(char *buffer, i4 *buf_len_in, PCB2 *pcb)
{
    SAVE_DATA	save_data;
    i4		buf_len = *buf_len_in;

    GCTRACE(2)("GCwinsock2_save %d: Entered.\n", pcb->id);

    /*
    ** Verify buffer large enough for data to save into.
    */
    if (sizeof(save_data) > buf_len)
    {
	GCTRACE(1)("GCwinsock2_save %d: ERROR! Input save area (%d) not large enough for data to save (%d)\n",
		    pcb->id, buf_len, sizeof(save_data) );
	return(FAIL);
    }

    /*
    ** Reject request if IO Completion Ports being used -- not supported.
    ** IO Completion Port sockets cannot be transferred to another
    ** process because of the following Windows limitations:
    ** 1. An I/O completion port and its handle are associated with the
    **    process that created it and is not shareable between processes.
    ** 2. A handle can be associated with only 1 I/O completion port
    **    and the handle remains associated with that I/O completion
    **    port until it is closed.
    ** It may be that duplicating the socket handle and then associating
    ** it in the new process, or some variation thereof, might work,
    ** though the Microsoft doc warns against this.  May try to get
    ** this working sometime in the future.  For now, just reject the
    ** save and document that IO Completion Ports do not support
    ** GCA_SAVE/GCA_RESTORE.  Not rejecting the request causes the
    ** application to hang in the new process; the next socket request
    ** is successful (sent data or received data), but the callback
    ** into the worker thread completion routine never occurs.
    ** The workaround is to use Alertable IO, which DOES work, and
    ** is now the default.
    */
    if (GCwinsock2CompletionPort)
    {
	GCTRACE(0)("GCwinsock2_save %d: ERROR! Connection cannot be tranferred to spawned process with I/O Completion Port;\n - use Alertable I/O (unset II_WINSOCK2_CONCURRENT_THREADS or set to zero, or\n - use named pipes for local IPC (unset II_GC_PROT)\nRejecting GCA_SAVE request.\n",
		    pcb->id );
	return(FAIL);
    }

    /*
    ** Note if any IOs pending.
    */
    if (pcb->AsyncIoRunning > 0)
    {
	GCTRACE(1)("GCwinsock2_save %d: WARNING! %d IOs pending on connection\n",
		    pcb->id, pcb->AsyncIoRunning );
    }

    save_data.id = pcb->id;
    save_data.sd = pcb->sd;
    MEcopy( &save_data, sizeof(save_data), buffer );
    *buf_len_in = sizeof(save_data);  /* Tell caller how much data was saved */

    return(OK);
}

/*
**  Name: GCwinsock2_restore
**  Description:
**	This function is called externally during GCrestore operation
**	to restore protocol-specific information from a prior "save"
**	for a session under a different process.
**
**  Inputs:
**	buffer 		Pointer to buffer from which to get saved information
**	pptr		Pointer to Protocol Control Table (PCT) entry
**	lpPcb		Pointer to address in which to return PCB2 address
**
**  Outputs:
**	lpPcb		Address of newly allocated/initialized PCB2
**
**  Returns:
**	OK    if success
**	FAIL  if error occurred...such as unable to allocate PCB2
**
**  History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created for GCrestore local IPC processing.
*/
STATUS
GCwinsock2_restore(char *buffer, GCC_PCE *pptr, PCB2 **lpPcb)
{
    SAVE_DATA	save_data;
    PCB2	*pcb;
    char	*proto = pptr->pce_pid;

    GCTRACE(2)("GCwinsock2_restore %s: Entered.\n", proto);

    MEcopy( buffer, sizeof(save_data), &save_data );

    /*
    ** Obtain a Protcol Control Block specific to this driver/port
    */
    if ( (pcb = GCwinsock2_get_pcb(pptr, TRUE)) == NULL )
	return(FAIL);

    pcb->id = save_data.id;
    pcb->sd = save_data.sd;
    *lpPcb = pcb;

    GCTRACE(2)("GCwinsock2_restore %s %d: Exited. socket=0x%p\n",
		proto, pcb->id, pcb->sd);

    return(OK);
}
    

/*
**  Name: GCwinsock2_get_pcb
**  Description:
**	Get an available PCB for the caller and initialize it.  PCB is the
**	protocol connection block for the protocol driver (1 per connection).
**	Currently, a new one is always allocated and then freed at disconnect.
**	In the future, may want to keep unused PCBs on a free list and
**	obtain them from there rather than reallocating each time.
**
**	Changed default size of receive overflow buffer from #defined
**	value (=32768) to the packet size in the protocol table (PCT).
**	The #define value is now only used if PCT packet size is not
**	greater than 0 (and if value wasn't overridden with Ingres variable
**	or config parm). This reduces wasted memory in the case where
**	the overflow size was much bigger than the PCT packet size,
**	since the caller's requested size is normally no larger than the
**	PCT packet size and, when the WSARecv() is issued, the actual
**	receive size used is set to the minimum of the caller's
**	requested size and the overflow size.
**
**  Inputs:
**	pptr	Pointer to Protocol Control Table (PCT) entry
**
**  Outputs: None
**
**  Returns:
**	Address to a PCB
**	NULL if unable to allocate a PCB	         
**
**  History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
*/
PCB2 *
GCwinsock2_get_pcb(GCC_PCE *pptr, bool outbound)
{
    PCB2	*pcb;
    STATUS	status;
    u_i2	recv_buffer_size;
    static i4	outbound_counter   = -1;  /* just for id'ing */
    static i4	inbound_counter    =  0;
    u_i1	pcb_id;

    /*
    ** Determine size of receive overflow buffer, which is allocated as
    ** part of the PCB.  It can be overridden with Ingres or environment
    ** variable II_WINSOCK2_RECV_BUFFER_SIZE or config.dat parm
    ** winsock2_recv_buffer_size, which has been saved in static variable
    ** GCWINSOCK2_recv_buffer_size.  If not overridden, it defaults to
    ** the protocol control table (PCT) packet size. In the unlikely
    ** event that the PCT packet size is not specified or is not available,
    ** then it defaults to hard-coded #define RCV_PCB_BUFF_SZ (=32K).
    */
    if (GCWINSOCK2_recv_buffer_size != -1)
	recv_buffer_size = (u_i2)GCWINSOCK2_recv_buffer_size;
    else
    if (pptr && pptr->pce_pkt_sz > 0)
	recv_buffer_size = (u_i2)pptr->pce_pkt_sz;
    else
	recv_buffer_size = RCV_PCB_BUFF_SZ;

    /*
    **  Allocate a new PCB
    */
    pcb = (PCB2 *) MEreqmem(0, sizeof(PCB2) + recv_buffer_size, FALSE, &status);
    if (pcb == NULL)
    {
	GCTRACE(1)("GCwinsock2_get_pcb: Unable to allocate PCB (size=%d)\n",
		    sizeof(PCB2) + recv_buffer_size);
	return(pcb);
    }
    pcb_id = (u_i1)(outbound ? (outbound_counter += 2) : (inbound_counter += 2));


    GCTRACE(5)("GCwinsock2_get_pcb: Allocated PCB %d (size=%d) at 0x%p\n",
		pcb_id, sizeof(PCB2) + recv_buffer_size, pcb);

    /*
    **  Initialize the PCB
    */
    ZeroMemory(pcb, sizeof(PCB2));  /* Don't bother to zero buffer */
    pcb->id = pcb_id;
    pcb->snd_PerIoData.OperationType = OP_SEND;
    pcb->snd_PerIoData.pcb       = pcb;

    pcb->rcv_PerIoData.OperationType = OP_RECV;
    pcb->rcv_PerIoData.pcb       = pcb;
    pcb->rcv_buf_len = sizeof(pcb->rcv_buffer) + recv_buffer_size;
    pcb->b1 = pcb->b2 = pcb->rcv_buffer;

    return(pcb);
}


/*
**  Name: GCwinsock2_rel_pcb
**  Description:
**	Release PCB - Protocol Connection Block.  Currently, we always
**	free it and all attached memory. 
**	In the future, may want to keep unused PCBs on a free list and
**	obtain them from there rather than reallocating each time.
**
**  Inputs:  Pointer to PCB to be released/freed
**
**  Outputs: None, though memory for PCB will no longer be available.
**
**  Returns: None
**
**  History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
*/
VOID
GCwinsock2_rel_pcb(PCB2 *pcb)
{
    if (!pcb)	/* Should never happen...but just in case */
    {
	GCTRACE(1)("GCwinsock2_rel_pcb: WARNING! PCB input ptr NULL!\n");
	return;
    }

    GCTRACE(5)("GCwinsock2_rel_pcb: Freeing PCB %d at 0x%p for socket=0x%p\n",
		pcb->id, pcb, pcb->sd);

    /*
    ** Do some basic validation that the PCB is in a state that is OK to free.
    ** This is more for diagnosis/debugging than anything.  We will still
    ** free the PCB, but at least provide a trace level 1 warning if
    ** anything seems amiss.  Only do the checking if tracing is on since
    ** there is no point in incurring the additional (admittedly slight)
    ** overhead if we're not going to display the warning.
    */
    if (GCWINSOCK2_trace >= 1)
    {
	if (pcb->AsyncIoRunning != 0)
	{
	    GCTRACE(5)("GCwinsock2_rel_pcb: WARNING! Freeing PCB %d at 0x%p for socket=0x%p with %d IO(s) pending!\n",
			pcb->id, pcb, pcb->sd, pcb->AsyncIoRunning);
	}

	if ( (pcb->snd_PerIoData.state != PIOD_ST_AVAIL) ||
	     (pcb->rcv_PerIoData.state != PIOD_ST_AVAIL) )
	{
	    GCTRACE(5)("GCwinsock2_rel_pcb: WARNING! Freeing PCB %d at 0x%p for socket=0x%p with an active snd/rcv state(%d/%d)!\n",
			pcb->id, pcb, pcb->sd,
			pcb->snd_PerIoData.state,
			pcb->rcv_PerIoData.state);
	}
    }

    /*
    **  Free memory and close object handles linked to the PCB
    **  and then free the PCB itself.
    */

    if (pcb->addrinfo_list)
	freeaddrinfo(pcb->addrinfo_list);
    if (pcb->snd_PerIoData.hWaitableTimer)
	CloseHandle(pcb->snd_PerIoData.hWaitableTimer);
    if (pcb->rcv_PerIoData.hWaitableTimer)
	CloseHandle(pcb->rcv_PerIoData.hWaitableTimer);

    MEfree((PTR)pcb);

    return;
}


/*
**  Name: GCwinsock2_get_piod
**  Description:
**	Get an available PER_IO_DATA for the caller and initialize it.
**	PER_IO_DATA is the control block for each Winsock2 IO request.
**	Currently, this routine always allocates a new one in memory.
**	It is up to the caller to free it.  NOTE that flags value
**	PIOD_DYN_ALLOC is set to distinguish these blocks from those
**	that are embedded in other structures (such as the PCB) and
**	shouldn't be freed directly.
**
**	While not strictly critical to the operation of this function
**	as it currently stands, the input parms are used to initialize
**	some of the common fields within the structure.
**	The OperationType is an input parm that could, in the future,
**	be used to allocate and initialize different types/sizes of
**	PER_IO_DATA structures.
**
**	In the future, may want to keep unused PER_IO_DATAs on a free
**	list and obtain them from there rather than reallocating each time.
**
**  Inputs:
**	OperationType   OP_RECV, OP_SEND, etc
**	GCC_P_PLIST	parmlist for GCC_RECEIVE, GCC_SEND, etc
**
**  Outputs: None
**
**  Returns:
**	Address to a PER_IO_DATA structure
**	NULL if unable to allocate a PER_IO_DATA	         
**
**  History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
*/
PER_IO_DATA *
GCwinsock2_get_piod(i4 OperationType, GCC_P_PLIST *parm_list)
{
    PER_IO_DATA	*lpPerIoData;
    PCB2        *pcb = (parm_list ? (PCB2 *)parm_list->pcb : NULL);
    STATUS	status;

    /*
    ** Allocate a new PER_IO_DATA structure
    */
    lpPerIoData = (PER_IO_DATA *)MEreqmem(0, sizeof(PER_IO_DATA),
  				      TRUE, &status);
    if (lpPerIoData == NULL)
    {
	GCTRACE(1)("GCwinsock2_get_piod: %p Unable to allocate PER_IO_DATA (size=%d) for optype=%d\n",
		    parm_list, sizeof(PER_IO_DATA), OperationType);
	if (parm_list)
	    SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
	return(lpPerIoData);
    }

    GCTRACE(5)("GCwinsock2_get_piod: %p Allocated PER_IO_DATA (size=%d) at 0x%p for optype=%d\n",
		parm_list, sizeof(PER_IO_DATA), lpPerIoData, OperationType);

    /*
    **  Initialize the PER_IO_DATA
    */
    lpPerIoData->OperationType	= OperationType;
    lpPerIoData->state		= PIOD_ST_IO_PENDING;
    lpPerIoData->flags		= PIOD_DYN_ALLOC;
    lpPerIoData->parm_list	= parm_list;
    lpPerIoData->pcb		= pcb;
    if (parm_list)
    {
	GCC_WINSOCK_DRIVER *wsd = &((THREAD_EVENTS *)parm_list->pce->pce_dcb)->wsd;
        lpPerIoData->block_mode = wsd->block_mode;
    }

    return(lpPerIoData);
}

/*
**  Name: GCwinsock2_rel_piod
**  Description:
**	Release PER_IO_DATA object.
**	If pre-allocated, mark it as available.
**	If dynamically allocated, free it (the memory).
**
**	In the future, may want to put released PER_IO_DATAs on a free
**	list for use by GCwinsock2_get_piod().
**
**  Inputs:
**	lpPerIoData     Ptr to PerIoData to be released
**
**  Outputs: None
**
**  Returns: None
**
**  History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
*/
VOID
GCwinsock2_rel_piod(PER_IO_DATA *lpPerIoData)
{
    i4		OperationType = lpPerIoData->OperationType;
    PCB2        *pcb = lpPerIoData->pcb;
    GCC_P_PLIST *parm_list = lpPerIoData->parm_list;

    GCTRACE(5)("GCwinsock2_rel_piod: %p %s PER_IO_DATA at 0x%p for optype=%d, state=%d\n",
		parm_list,
		(lpPerIoData->flags & PIOD_DYN_ALLOC) ? "Freeing" : "Releasing",
		lpPerIoData,
		OperationType,
		lpPerIoData->state);
    if (lpPerIoData->flags & PIOD_DYN_ALLOC)
    {
	if ( (OperationType == OP_RECV) &&
	     ((pcb = lpPerIoData->pcb) != NULL) &&
	     (lpPerIoData == pcb->last_rcv_PerIoData) )
		pcb->last_rcv_PerIoData = NULL;
	if (lpPerIoData->hWaitableTimer)
	    CloseHandle(lpPerIoData->hWaitableTimer);
	MEfree((PTR)lpPerIoData);
    }
    else
    {
	lpPerIoData->state = PIOD_ST_AVAIL;
    }
}


/*
**  Name: GCwinsock2_alloc_connect_events
**  Description:
**	For AIO logic (ie, not IO Completion Ports), asynchronous connects
**	and listens are handled via events.  This routine allocates
**	an array of events and an associated array of Overlapped
**	structure pointers (initially set to NULL).
**
**	Since the maximum # of concurrent connects is unknown,
**	a reasonably large # of array entries is allocated.
**	While the calling server's inbound_limit+outbound_limit
**	(for gcc at least) would provide an upper bound,
**	it is not readily available to the CL and is likely much
**	larger than is needed for the array, which only needs to
**	handle concurrent, pending connection requests; ie, once
**	connected, its slot in the array is available for reuse.
**
**  Inputs:  None...except:
**	     Static arrays for events and overlapped structure pointers.
**
**  Outputs: None...except:
**	     Static arrays for events and overlapped structures
**	     are allocated.
**
**  Returns:
**	OK   - Successful
**	!OK  - Error
**
**  History:
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Created.
**      19-Nov-2009 (Bruce Lunsford) Bug 122940 + Sir 122426
**	    Failed connects and listens were not detected when using
**	    recently added async alertable I/O logic (sir 122426).
**	    Changed arrayConnectOverlaps to arrayConnectInfo which
**	    contains required addition of the socket descriptor.
*/

STATUS
GCwinsock2_alloc_connect_events()
{
    STATUS	status;
    u_i4	ixEvent;

    /*
    ** If already allocated (shouldn't happen), just return.
    */
    if (arrayConnectEvents || arrayConnectInfo)
    {
	GCTRACE(1)("GCwinsock2_alloc_connect_events: Array of AIO connect events/overlap ptrs already allocated at (0x%p, 0x%p)\n",
		    arrayConnectEvents, arrayConnectInfo);
	return(OK);
    }

    /*
    ** Allocate arrays.
    */

    GCTRACE(1)("GCwinsock2_alloc_connect_events: Entered. Allocating %d entries in arrays\n", MAX_PENDING_CONNECTS);

    arrayConnectEvents = (HANDLE *)MEreqmem(0, MAX_PENDING_CONNECTS * sizeof(HANDLE), TRUE, &status);
    if (arrayConnectEvents == NULL)
    {
	GCTRACE(1)("GCwinsock2_alloc_connect_events: Unable to allocate arrayConnectEvents (%d handles)\n",
		    MAX_PENDING_CONNECTS);
	return(status);
    }

    arrayConnectInfo = (AIO_CONN_INFO *)MEreqmem(0, MAX_PENDING_CONNECTS * sizeof(AIO_CONN_INFO), TRUE, &status);
    if (arrayConnectInfo == NULL)
    {
	GCTRACE(1)("GCwinsock2_alloc_connect_events: Unable to allocate arrayConnectInfo (%d entries)\n",
		    MAX_PENDING_CONNECTS);
	return(status);
    }

    /*
    ** Create an event for each new entry in the event array.
    ** The events are predefined here and then reused, as needed,
    ** for future connect and listen requests.
    */
    for (ixEvent = 0; ixEvent < MAX_PENDING_CONNECTS; ixEvent++)
    {
	if ( (arrayConnectEvents[ixEvent] = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
	{
	    status = GetLastError();
	    GCTRACE(1)("GCwinsock2_alloc_connect_events: Unable to create connect event[%d], status=%d)\n", ixEvent, status);
	    return(status);
	}
    }

    return(OK);
}

/*
**  Name: GCwinsock2_add_connect_event
**  Description:
**	For AIO logic (ie, not IO Completion Ports), asynchronous
**	connects and listens are handled via events.  This routine
**	obtains an available event for caller from an array of events.
**	The returned event can be used in the ConnectEx() or AcceptEx()
**	call and then waited on pending completion of associated
**	connects/listens...see wait in GCwinsock_wait_completed_connect().
**
**	In the hopefully rare occurrence that no events are available
**	for use (ie, already being used), a new event must be created.
**	Queue the new event until a slot in the wait array frees up.
**
**  Inputs:
**      lpOverlapped   Pointer to overlapped structure.
**	sd             socket descriptor for connect/listen
**
**  Outputs:
**      lpOverlapped->hEvent   Event handle to use for connect/listen.
**	Static array for event-related overlapped pointers is updated
**	     with caller's lpOverlapped value, indicating event is
**	     "in-use" and who is using it.
**
**  Returns:
**	OK   - Successful
**	!OK  - Error
**
**  History:
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Created.
**      19-Nov-2009 (Bruce Lunsford) Bug 122940 + Sir 122426
**	    Failed connects and listens were not detected when using
**	    recently added async alertable I/O logic (sir 122426).
**	    Save socket descriptor (new input parm and new field in
**	    AIO structures arrayConnectInfo and conn_q.
*/

STATUS
GCwinsock2_add_connect_event(LPOVERLAPPED lpOverlapped, SOCKET sd)
{
    STATUS	status;
    u_i4	ixEvent, nowPending, nowQueued;
    AIO_CONN_OVFLO_Q  *conn_q;

    /*
    ** Keep track of current and highest ever concurrent pending connects+
    ** listens.  ** For informational purposes only **
    */
    nowPending = InterlockedIncrement(&numPendingConnects);
    if ( nowPending > highPendingConnects )
	highPendingConnects = nowPending;

    /*
    ** Find available (unused) event in array, which is determined by
    ** a NULL value in the associated lpOverlapped pointer array.
    */
    for (ixEvent=0; ixEvent < MAX_PENDING_CONNECTS; ixEvent++)
    {
	if (arrayConnectInfo[ixEvent].lpOverlapped == NULL) break;
    }
	
    /*
    **  !OVERFLOW LOGIC! - If no available(unused) events found
    */
    if (ixEvent >= MAX_PENDING_CONNECTS)
    {
	/*
	** Create a new event but save it into the overflow queue
	** for later processing (move to wait array) when a connect
	** or listen completes and a slot in the array frees up.
	** NOTE: This should rarely occur...means array not big enough
	** to handle concurrent connect activity.  Caller is provided an
	** event so that the ConnectEx() or AcceptEx() can be issued,
	** but no one will initially be waiting on the event to complete.
	*/
	totalAIOConnectQ++;  /* Keep track of # Q'd...info for performance */
	if ((conn_q = (AIO_CONN_OVFLO_Q *)MEreqmem(0, sizeof(*conn_q), TRUE, &status)) == NULL)
	{
	    GCTRACE(1)("GCwinsock2_add_connect_event: Unable to allocate memory for Q for Overlapped=0x%p, status=%d\n",
			lpOverlapped, status);
	    return(status);
	}
	if ( (conn_q->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
	{
	    status = GetLastError();
	    GCTRACE(1)("GCwinsock2_add_connect_event: Unable to create connect event for Q for Overlapped=0x%p, status=%d\n",
			lpOverlapped, status);
	    return(status);
	}
	lpOverlapped->hEvent = conn_q->hEvent;
	conn_q->lpOverlapped = lpOverlapped;
	conn_q->sd           = sd;
	EnterCriticalSection( &AIOConnectCritSect );
	/* Place at end to cause FIFO processing. */
	QUinsert(&conn_q->AIO_Connect_q, AIOConnectQ.q_prev);
	nowQueued = numAIOConnectQ++;
	LeaveCriticalSection( &AIOConnectCritSect );
	GCTRACE(1)("GCwinsock2_add_connect_event: AIO Conn Array full, request Q'd(%d) for Overlapped=0x%p\n",
		   nowQueued, lpOverlapped);
	return(OK);
    }

    /*
    ** Copy the available event to the Overlapped structure, which is
    ** what the Windows ConnectEx() and AcceptEx() functions use.
    ** The event in the array and the overlapped structures are
    ** linked together by putting them into their respective arrays at the
    ** same position.
    */
    arrayConnectInfo[ixEvent].lpOverlapped = lpOverlapped;
    arrayConnectInfo[ixEvent].sd           = sd;
    lpOverlapped->hEvent = arrayConnectEvents[ixEvent];
    return(OK);
}

/*
**  Name: GCwinsock_wait_completed_connect
**  Description:
**	For AIO logic (ie, not IO Completion Ports), asynchronous
**	connects are handled via events.  This routine waits for any
**	connection event (one per pending connect and listen request)
**	to be signaled by the OS (from original ConnectEx() or
**	AcceptEx() call); the event is signalled when the request
**	is complete.  This routine determines which connect/listen
**	request has completed and passes back to the caller
**	a pointer to the Overlapped area, from which the address
**	to even more useful info in the PerIoData structure can
**	be obtained.
**
**	This routine is called from a special worker thread for
**	connects in lieu of GetQueuedCompletionStatus().  This
**	allows sharing of the worker thread code that handles
**	connects.  If using the AIO logic for sends and receives,
**	ConnectEx() and AcceptEx() are used for async connects.
**	However, since these functions only work with IO Completion
**	Ports or Events, the latter must be used since AIO and
**	IO Completion Port logic on a socket is mutually exclusive.
**	That is, one cannot use an IO Completion Port to connect and
**	then use AIO for sends and receives (the OS complains).
**
**  Inputs:
**      &lpOverlapped   Pointer to location where pointer to overlapped
**			structure is to be returned.
**
**  Outputs:
**      lpOverlapped    Pointer to overlapped structure.  Set to NULL
**			if none found.
**	status		Pointer to status from completed connect/listen.
**
**  Returns:
**       TRUE	if found a completed connect/listen request.
**	 FALSE	if error (such as abandoned WAIT)
**
**  History:
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Created.
**      19-Nov-2009 (Bruce Lunsford) Bug 122940 + Sir 122426
**	    Failed connects and listens were not detected when using
**	    recently added async alertable I/O logic (sir 122426).
**	    Added calls to WSAGetOverlappedResult() and WSAGetLastError()
**	    when connect/listen completed to check results.  This required
**	    adding the socket to associated AIO structures.
*/

bool
GCwinsock2_wait_completed_connect(LPOVERLAPPED *lppOverlapped, STATUS *status)
{
    bool	retval = TRUE;
    DWORD	BytesTransferred = 0;
    DWORD	Flags;
    DWORD 	dwObj, oops;
    u_i4	ixEvent, nowPending;
    SOCKET	sd;

    *status = OK;

    GCTRACE(3)("GCwinsock2_wait_completed_connect: Entered. connects+listens event array size=%d, a(lpOverlapped)=0x%p\n",
		 MAX_PENDING_CONNECTS,
		 lppOverlapped);

    *lppOverlapped = NULL;

    while (TRUE)
    {
	/*
	** Wait for a pending async connect or listen to complete.
	*/
	dwObj = WaitForMultipleObjectsEx(MAX_PENDING_CONNECTS, 
		                         arrayConnectEvents,
		                         FALSE, INFINITE, TRUE);
	if ( (dwObj >=  WAIT_OBJECT_0) &&
	     (dwObj <= (WAIT_OBJECT_0 + MAX_PENDING_CONNECTS - 1)) )
	{
	    ixEvent = dwObj - WAIT_OBJECT_0; 
	    break;
	}
	else
	if ( (dwObj >=  WAIT_ABANDONED_0) &&
	     (dwObj <= (WAIT_ABANDONED_0 + MAX_PENDING_CONNECTS - 1)) )
	{
	    ixEvent = dwObj - WAIT_ABANDONED_0; 
	    retval = FALSE;
	    break;
	}
	/*
	** The rest of the wait objects below normally shouldn't happen.
	** Just trace them and continue (ie, don't return to caller).
	*/
	else
	if (dwObj == WAIT_IO_COMPLETION)
	{
	    GCTRACE(3)("GCwinsock2_wait_completed_connect: WAIT_IO_COMPLETION\n");
	    continue;
	}
	else
	if (dwObj == WAIT_FAILED)
	{
            oops = GetLastError();
            GCTRACE(1)( "GCwinsock2_wait_completed_connect - Wait Failed ret = %d\n", oops );
            continue;
	}
	else
	{
            GCTRACE(1)( "GCwinsock2_wait_completed_connect - Unexpected Wait Object = %d\n", dwObj );
            continue;
	}
    } /* End while(TRUE) */

    /*
    ** Return the address of the overlapped structure that corresponds
    ** to the signaled event; save into input parm pointer.
    ** Unless there is an entry in the overflow Q (rare),
    ** mark the signaled event as available (unused) by clearing it
    ** in the overlapped structure and then removing its corresponding
    ** overlap pointer from the Overlapped array.  If there is an entry
    ** in the overflow Q, move it from the Q to the just freed-up entry
    ** in the array.
    */
    *lppOverlapped = arrayConnectInfo[ixEvent].lpOverlapped;
    sd             = arrayConnectInfo[ixEvent].sd;
    (*lppOverlapped)->hEvent = 0;

    /*
    ** Get results (success or fail) of the connect or listen operation.
    ** If succeeded, WSAGetOverlappedResult() will return a nonzero value.
    ** If failed, it will return FALSE (=0).
    ** Set retval appropriately (FALSE for failure, TRUE for success).
    ** In the case of a failure, also get the status of the operation;
    ** this must be done here right after the call to WSAGetOverlappedResult()
    ** because any tracing or other system functions executed after that
    ** may overlay the status that WSAGetLastError() returns.
    */
    if (WSAGetOverlappedResult(sd, *lppOverlapped, &BytesTransferred, FALSE, &Flags) == FALSE)
    {
	retval = FALSE;
        *status = WSAGetLastError();
    }

    GCTRACE(2)("GCwinsock2_wait_completed_connect: Event[%d], lpOverlapped=0x%p\n",
		ixEvent,
		*lppOverlapped);

    /*
    ** See if any events are waiting in the overflow Q (should rarely happen).
    ** If so, since we're freeing up a slot in the wait array, replace it
    ** with an entry from the overflow Q.
    */
    if ( numAIOConnectQ )  /* If anything in AIO connect overflow Q */
    {
	u_i4 nowQueued;
	AIO_CONN_OVFLO_Q  *conn_q;

	/* Remove an entry from the Q */
	EnterCriticalSection( &AIOConnectCritSect );
	conn_q = (AIO_CONN_OVFLO_Q *)AIOConnectQ.q_next;
	if ( (QUEUE *)conn_q != &AIOConnectQ )  /* Make sure Q not now empty */ 
	{
	    QUremove( &conn_q->AIO_Connect_q );
	    nowQueued = numAIOConnectQ--;
	}
	LeaveCriticalSection( &AIOConnectCritSect );

	if ( (QUEUE *)conn_q != &AIOConnectQ )  /* If we got a conn overflow entry */
	{
	    CloseHandle(arrayConnectEvents[ixEvent]);  /* Close the old event */
	    arrayConnectEvents            [ixEvent] = conn_q->hEvent;
	    arrayConnectInfo[ixEvent].lpOverlapped  = conn_q->lpOverlapped;
	    arrayConnectInfo[ixEvent].sd            = conn_q->sd;
	    GCTRACE(1)("GCwinsock2_wait_completed_connect: Moved AIO Conn event/Overlapped(0x%p) from overflow Q to array[%d], #remaining on Q=%d\n",
			conn_q->lpOverlapped,
			ixEvent,
			nowQueued);
	    MEfree((PTR)conn_q);
	    goto wait_completed_connect_exit;
	}  /* End if moved entry from overflow Q to array */
    } /* End if anything in AIO connect overflow Q */

    arrayConnectInfo[ixEvent].lpOverlapped = NULL;
    arrayConnectInfo[ixEvent].sd           = 0;
    nowPending = InterlockedDecrement(&numPendingConnects);  /* for informational purposes */

wait_completed_connect_exit:

    GCTRACE(2)("GCwinsock2_wait_completed_connect: Exiting retval %d status=%d #pending=%d\n",
		retval,
		*status,
		nowPending);

    return(retval);

}


/*
**  Name: GCwinsock2_worker_thread
**  Description:
**	This routine services the completion port.
**
**  History:
**	29-oct-2003 (somsa01)
**	    Created.
**	08-oct-2004 (somsa01)
**	    Make sure lpPerHandleData is valid in OP_DISCONNECT.
**      23-feb-2005 (lunbr01)  bug 113686, prob INGNET 160
**          E_GC4810_TL_FSM_STATE errors occur occasionally in GCD due to 
**          SEND (or RECV) completion routine being called AFTER disconnect
**          compl rtn. Add "call completion routine" requests at END of
**          queue rather than beginning to force FIFO processing of
**          completions.
**      29-feb-2005 (lunbr01)  bug 113987, prob INGNET 165
**          Pending async RECEIVES occasionally do not post complete after
**          the socket is closed during disconnect, which resulted in the
**          disconnect looping forever waiting for the IO to complete.
**          Limit the number of times disconnect will wait.  Symptom
**          is that subsequent connect requests may hang.
**      14-jul-2004 (loera01) Bug 114852
**          In the OP_RECV_MSG_LEN operation, fetch the second byte of
**          the message length if only one byte was sent.
**      26-Jan-2006 (Ralph Loen) Bug 115671
**          Added GCTCPIP_log_rem_host to optionally disable invocation of
**          gethostbyaddr().
**      22-Jan-2007 (lunbr01) Bug 117523
**          Corrupted (invalid) incoming data with large length prefix 
**          can crash caller (eg, GCC or GCD).  Add check to ensure
**          caller's buffer length is not exceeded (ie, avoid buffer overrun).
**      11-Sep-2008 (lunbr01)
**          Add trace statements for all error conditions.
**	26-Nov-2008 (Bruce Lunsford) Bug 121285
**	    Call new standalone function GCwinsock2_schedule_completion
**	    instead of having code inline.
**	    Also prevent occasional crash in tracing after WSARecv()s by not
**	    referencing fields in control blocks that may be gone due
**	    to async I/O completing first on other (worker) threads.
**	02-Mar-2009 (Bruce Lunsford) Bug 121742
**	    Ensure GCTRACE() not invoked between OS or winsock calls
**	    and [WSA]GetLastError() because tracing overlays the returned
**	    status (such as WSAEWOULDBLOCK) with zero.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Sync up code here with code in GCwinsock2_AIO_Callback()
**	    so that we can share code, where feasible. Replaced OP_RECV
**	    and OP_RECV_MSG_LEN code with single call to new shared code
**	    GCwinsock2_OP_RECV_complete(), which handles both 1-step and
**	    2-step receives.
**	    Check for presence of lpOverlapped from IO completions in 
**	    worker threads...may be NULL under certain error conditions.
**	19-Nov-2009 (Bruce Lunsford) Bug 122940 + Sir 122426
**	    Do not call GetLastError() for AIO when retval is non-zero
**	    because it was already called and stored in input parm "status"
**	    by GCwinsock2_wait_completed_connect().
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add check for shutting down to avoid misleading trace output.
**	    Add trace at entry + exit with blank lines before/after for
**	    easier tracking of IO completion processing in trace output.
**	    Also add thread id (tid) to traces.
**	    Use thread id from incoming PER_IO_DATA structure to
**	    call GCwinsock2_schedule_completion(), which now requires
**	    the original caller's thread id.
**	    Set system_status to ER_close if send failed due to
**	    connection having been closed (BytesTransferred == 0);
**	    can be used by caller to detect closed socket.
*/

DWORD WINAPI
GCwinsock2_worker_thread(LPVOID CompletionPortID)
{
    HANDLE		CompletionPort;
    DWORD		BytesTransferred;
    LPOVERLAPPED	lpOverlapped;
    GCC_P_PLIST		*parm_list;
    GCC_LSN    		*lsn;
    bool		retval;
    STATUS		status;
    PCB2		*pcb = NULL;
    PER_IO_DATA		*lpPerIoData;
    bool		DecrementIoCount;
    char		*proto = "";
    ULONG_PTR		dummyCompletionKey;
    u_i4  		tid;
    DWORD		ThreadId;

    CompletionPort = (HANDLE)CompletionPortID;
    tid = GetCurrentThreadId();

    GCTRACE(3)("GCwinsock2_worker_thread: Started.  Worker Thread tid = %d\n",
		tid);

    while(TRUE)
    {
	DecrementIoCount = TRUE;
	if (CompletionPort)
	    retval = GetQueuedCompletionStatus(CompletionPort,
					       &BytesTransferred,
					       &dummyCompletionKey,
					       &lpOverlapped, INFINITE);
	else
	    retval = GCwinsock2_wait_completed_connect(&lpOverlapped,
			                               &status);

	if (retval)
	    status = 0;
	else if (CompletionPort)
	    status = GetLastError();

	if (!lpOverlapped)
	{
	    if (GCwinsock2_shutting_down)  /* if shutting down,       */
		return(OK);		   /*   just exit the thread. */
	    GCTRACE(1)("GCwinsock2_worker_thread(%s): Entered with no Overlapped area and failure status %d ...skip it!\n",
		        tid, status);
	    continue;
	}

	lpPerIoData = CONTAINING_RECORD(lpOverlapped, PER_IO_DATA, Overlapped);
	ThreadId  = lpPerIoData->ThreadId;
	parm_list = lpPerIoData->parm_list;
	pcb       = lpPerIoData->pcb;

	/*
	** If we already processed this I/O when we first issued it, then
	** skip most of the processing below.  NOTE that because the caller
	** has already received the results, the parmlist is probably
	** gone, and hence cannot be used.
	*/
	if (lpPerIoData->state == PIOD_ST_SKIP_CALLBK)
	{
	    if ( (lpPerIoData->OperationType == OP_RECV) ||
		 (lpPerIoData->OperationType == OP_SEND) )
		DecrementIoCount = TRUE;
	    else
		DecrementIoCount = FALSE;
	    goto worker_thread_err_exit;
	}

        if (parm_list)  /* No parm list OK for LISTEN -> OP_ACCEPT */
	{
	    proto = parm_list->pce->pce_pid;
	    if (!pcb)   /* if wasn't found above in PerIoData, try parm_list. */
		pcb = (PCB2 *)parm_list->pcb;
	    if (!pcb)
	    {
		GCTRACE(1)("GCwinsock2_worker_thread(%d) %s: %p Entered with no PCB\n",
			            tid, proto, parm_list);
		DecrementIoCount = FALSE;
	        goto worker_thread_err_exit;
	    }
	}
	else
	{
	    proto = "";   /* Until proto located below */
	    pcb = NULL;   /* Until pcb   located below */
	}


	GCTRACE(5)("\nGCwinsock2_worker_thread(%d) %s %d: %p Entered with retval/status=%d/%d...\n ... for optype=%d on socket=0x%p with PendingIOs=%d\n",
		tid, proto,
		pcb ? pcb->id : 0,
		parm_list, retval, status,
		lpPerIoData->OperationType,
		pcb ? pcb->sd : 0,
		pcb ? pcb->AsyncIoRunning : 0);

	/*
	** If this IO had an active timer associated with it, cancel it.
	*/
	if ( (lpPerIoData->state == PIOD_ST_IO_TMO_PENDING) &&
	     (lpPerIoData->hWaitableTimer) )
	{
	    GCTRACE(3)("GCwinsock2_worker_thread %s: %p Cancelling Timer\n",
			proto, parm_list);
	    if ( CancelWaitableTimer(lpPerIoData->hWaitableTimer) == 0 )
	    {
		DWORD tmp_status = GetLastError();
		GCTRACE(1)("GCwinsock2_worker_thread %s: %p WARNING CancelWaitableTimer() failed, status=%d\n",
			    proto, parm_list, tmp_status);
	    }
	}

	/*
	**   Process completion based on Operation Type
	*/

	switch(lpPerIoData->OperationType)
	{
	    case OP_ACCEPT:
		DecrementIoCount = FALSE;
		lsn = lpPerIoData->lsn;

		/*
		** Don't start processing the incoming connection request
		** until we have exclusive access to a Listen request.
		** This shouldn't take long as caller will reissue listens
		** as fast as he can.
		** Could optimize this slightly more by moving WFSO further
		** down in processing, but then need to be sure not to 
		** use PCB before WFSO; this approach required fewer changes
		** and is more straightforward.
		** One problem with this approach is that, though unlikely,
		** it is possible to tie up all of the worker threads here.
		** One escape mechanism that could be, but wasn't, taken
		** for that possibility would be to set the timeout 
		** value to a reasonable value so we can bail out
		** of this pending listen completion and free up the 
		** worker thread for other activities (send, receive, etc). 
		** An alternative approach was taken, which was to start 
		** up enough extra worker threads to handle a listen pending 
		** completion on every address for every port being listened 
		** on, which may be overkill and wastes threads.  The only
		** performance impact is the extra virtual memory required
		** for each thread (default 1 MB stack heap).  The advantage
		** of the extra thread approach is that we would never
		** fail an incoming connection for a timeout, whose value
		** would be arbitrary.
		*/
		WaitForSingleObject(lsn->hListenEvent, INFINITE);
		parm_list = lsn->parm_list;
		proto = parm_list->pce->pce_pid;
		pcb = (PCB2 *)parm_list->pcb;
		pcb->sd = lpPerIoData->client_sd;
		lsn->addrinfo_posted  = lpPerIoData->addrinfo_posted;
		lsn->socket_ix_posted = lpPerIoData->socket_ix_posted;

		if (!retval)
		{
		    GCTRACE(1)("GCwinsock2_worker_thread(%d) %s %d: %p OP_ACCEPT Failed with status %d, socket=0x%p\n",
			    tid, proto, pcb->id, parm_list, status, pcb->sd);
		    if (status == ERROR_OPERATION_ABORTED)
			goto worker_thread_err_exit;
		    SETWIN32ERR(&parm_list->system_status, status, ER_accept);
		    parm_list->generic_status = GC_LISTEN_FAIL;
		    break;
		}

		setsockopt(pcb->sd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
			   (char *)&lpPerIoData->listen_sd,
			   sizeof(lpPerIoData->listen_sd));

		GCwinsock2_listen_complete(parm_list, lpPerIoData->AcceptExBuffer);
		if (parm_list->generic_status == GC_LISTEN_FAIL)
		    break;

		/*
		** Associate accepted client socket to completion port.
		*/
		if ((GCwinsock2CompletionPort)
		&&  (!CreateIoCompletionPort((HANDLE)pcb->sd,
					    GCwinsock2CompletionPort,
					    (ULONG_PTR)0, 0)))
		{
		    status = GetLastError();
		    parm_list->generic_status = GC_LISTEN_FAIL;
		    SETWIN32ERR(&parm_list->system_status, status,
				ER_createiocompport);
		    GCTRACE(1)("GCwinsock2_worker_thread(%d) %s %d: %p OP_ACCEPT CreateIoCompletionPort() failed with status %d, socket=0x%p\n",
			    tid, proto, pcb->id, parm_list, status, pcb->sd);
		    break;
		}

		break;

	    case OP_CONNECT:
		DecrementIoCount = FALSE;
		if (!retval)
		{
		    GCTRACE(1)("GCwinsock2_worker_thread(%d) %s %d: %p OP_CONNECT Failed with status %d, socket=0x%p\n",
			    tid, proto, pcb->id, parm_list, status, pcb->sd);
		    if (status == ERROR_OPERATION_ABORTED)
			goto worker_thread_err_exit;

op_connect_next_addr:
	            pcb->lpAddrinfo = pcb->lpAddrinfo->ai_next;
	            if (pcb->lpAddrinfo)   /* If more addresses, try next one */
		    {
		        if (GCwinsock2_connect (parm_list, (char *)lpPerIoData) == OK)
		            continue;  /* Async connect issued */
		        else           /* Async connect failed */
                            if (parm_list->generic_status == GC_CONNECT_FAIL)
		                break; /* fatal error */
		            else
		                goto op_connect_next_addr;
		    }
		    else  /* No more target IP addresses...connect failed */
		    {
		        SETWIN32ERR(&parm_list->system_status, status, ER_connect);
		        parm_list->generic_status = GC_CONNECT_FAIL;
		        GCTRACE(1)("GCwinsock2_worker_thread(%d) %s %d: %p OP_CONNECT All IP addrs failed\n",
			    tid, proto, pcb->id, parm_list);
		        break;
		    }
		}

		setsockopt(pcb->sd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT,
			   NULL, 0);

		GCwinsock2_connect_complete(parm_list);

		break;

	    case OP_SEND:
		if (!retval)
		{
		    GCTRACE(1)("GCwinsock2_worker_thread(%d) %s %d: %p OP_SEND Failed with status %d, socket=0x%p\n",
			    tid, proto, pcb->id, parm_list, status, pcb->sd);
		    pcb->tot_snd = 0;
		    if (status == ERROR_OPERATION_ABORTED)
			goto worker_thread_err_exit;
		    parm_list->generic_status = GC_SEND_FAIL;
		    SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		    break;
		}

		if (!BytesTransferred)
		{
		    /* The session has been disconnected */
		    GCTRACE(1)("GCwinsock2_worker_thread(%d) %s %d: %p OP_SEND Failed- 0 bytes trfrd indicating session disconnected, socket=0x%p\n",
			    tid, proto, pcb->id, parm_list, pcb->sd);
		    pcb->session_state = SS_DISCONNECTED;
		    pcb->tot_snd = 0;
		    parm_list->generic_status = GC_SEND_FAIL;
		    SETWIN32ERR(&parm_list->system_status, 0, ER_close);
		    break;
		}

		break;

	    case OP_RECV:
		if ( (GCwinsock2_OP_RECV_complete((DWORD)!retval, &status, BytesTransferred, lpPerIoData)) == FALSE )
		    continue;  /* another WSARecv() was issued...not done yet */
		if (status == ERROR_OPERATION_ABORTED)
		    goto worker_thread_err_exit;
		break;

	    default:
		continue;
	} /* End switch on OperationType */

	/*
	** Release PER_IO_DATA before running completion routine so that
	** it can be reused if we are called back with another request
	** from within the completion routine.  The PER_IO_DATA is no
	** longer needed at this point, so it is safe to release.
	** This also helps with reuse of pre-allocated PER_IO_DATA(s)
	** by making them available a bit sooner.
	*/
	GCwinsock2_rel_piod(lpPerIoData);
	lpPerIoData = NULL;

	/*
	** Drive the completion routine.
	*/
	status = GCwinsock2_schedule_completion(NULL, parm_list, ThreadId);

worker_thread_err_exit:
	if (lpPerIoData)
	    GCwinsock2_rel_piod(lpPerIoData);
	if (DecrementIoCount)
	    InterlockedDecrement(&pcb->AsyncIoRunning);
	GCTRACE(5)("GCwinsock2_worker_thread(%d) %s: %p Exited (back to sleep - wait for next IO completion)\n\n",
		tid, proto, parm_list);
    } /* End while TRUE */

    /*
    ** Return a value for compiler's sake; in actuality, we should never
    ** reach this as we stay in the while loop until the thread is 
    ** terminated at shutdown.
    */
    return(OK);
}


/*
**  Name: GCwinsock2_AIO_Callback
**  Description:
**	This callback function is executed upon Asynchronous I/O completion.
**	It is used on Windows 9x machines where completion ports cannot
**	be used and on later Windows platforms if AIO was selected (config
**	parm) rather than IO Completion Port.
**
**  History:
**	29-oct-2003 (somsa01)
**	    Created.
**      14-jul-2004 (loera01) Bug 114852
**          In the OP_RECV_MSG_LEN operation, fetch the second byte of
**          the message length if only one byte was sent.
**      11-Sep-2008 (lunbr01)
**          Add trace statements for all error conditions.
**	26-Nov-2008 (Bruce Lunsford) Bug 121285
**	    Call new standalone function GCwinsock2_schedule_completion
**	    instead of having code inline.
**	02-Mar-2009 (Bruce Lunsford) Bug 121742
**	    Ensure GCTRACE() not invoked between OS or winsock calls
**	    and [WSA]GetLastError() because tracing overlays the returned
**	    status (such as WSAEWOULDBLOCK) with zero.
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Move check for disconnect (zero bytes received) above (rather
**	    than after) check for "not all bytes received" to fix loop
**	    continually WSARecv'ing 0 bytes (occurred at disconnect).
**	    Call completion routine directly (for performance) rather
**	    than "scheduling" it.
**	    Sync up code here with code in GCwinsock2_worker_thread()
**	    so that we can share code, where feasible. Replaced OP_RECV
**	    and OP_RECV_MSG_LEN code with single call to new shared code
**	    GCwinsock2_OP_RECV_complete(), which handles both 1-step and
**	    2-step receives.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add trace to exit and a blank line before entry and after exit
**	    for easier tracking of IO completion processing in trace output.
*/

VOID CALLBACK
GCwinsock2_AIO_Callback(DWORD dwError, DWORD BytesTransferred, WSAOVERLAPPED *lpOverlapped, DWORD dwFlags)
{
    PER_IO_DATA		*lpPerIoData;
    GCC_P_PLIST		*parm_list;
    char		*proto = "";
    PCB2		*pcb;
    STATUS		status = 0;

    lpPerIoData = CONTAINING_RECORD(lpOverlapped, PER_IO_DATA, Overlapped);
    parm_list = lpPerIoData->parm_list;
    pcb       = lpPerIoData->pcb;

    /*
    ** If we already processed this I/O when we first issued it, then
    ** skip most of the processing below.  NOTE that because the caller
    ** has already received the results, the parmlist is probably
    ** gone, and hence cannot be used.
    */
    if (lpPerIoData->state == PIOD_ST_SKIP_CALLBK)
	goto AIO_Callback_err_exit;

    proto = parm_list->pce->pce_pid;

    if (dwError != OK)
	status = WSAGetLastError();

    GCTRACE(5)("\nGCwinsock2_AIO_Callback %s %d: %p Entered with error/status=%d/%d...\n   ... for optype=%d on socket=0x%p with PendingIOs=%d\n",
		proto, pcb->id, parm_list, dwError, status,
		lpPerIoData->OperationType, pcb->sd,
		pcb->AsyncIoRunning);

    /*
    ** If this IO had an active timer associated with it, cancel it.
    */
    if ( (lpPerIoData->state == PIOD_ST_IO_TMO_PENDING) &&
	 (lpPerIoData->hWaitableTimer) )
    {
	GCTRACE(3)("GCwinsock2_AIO_Callback %s: %p Cancelling Timer\n",
		    proto, parm_list);
	if ( CancelWaitableTimer(lpPerIoData->hWaitableTimer) == 0 )
	{
	    DWORD tmp_status = GetLastError();
	    GCTRACE(1)("GCwinsock2_AIO_Callback %s: %p WARNING CancelWaitableTimer() failed, status=%d\n",
			proto, parm_list, tmp_status);
	}
    }

    /*
    **   Process completion based on Operation Type
    */
    switch(lpPerIoData->OperationType)
    {
	case OP_SEND:
	    if (dwError != OK)
	    {
		GCTRACE(1)("GCwinsock2_AIO_Callback %s %d: %p OP_SEND Failed with status %d, socket=0x%p\n",
			    proto, pcb->id, parm_list, status, pcb->sd);
		pcb->session_state = SS_DISCONNECTED;
		pcb->tot_snd = 0;
		if (status == ERROR_OPERATION_ABORTED)
		    goto AIO_Callback_err_exit;
		parm_list->generic_status = GC_SEND_FAIL;
		SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		break;
	    }

	    if (!BytesTransferred)
	    {
		/* The session has been disconnected */
		GCTRACE(1)("GCwinsock2_AIO_Callback %s %d: %p OP_SEND Failed- 0 bytes trfrd indicating session disconnected, socket=0x%p\n",
			    proto, pcb->id, parm_list, pcb->sd);
		pcb->tot_snd = 0;
		parm_list->generic_status = GC_SEND_FAIL;
		SETWIN32ERR(&parm_list->system_status, 0, ER_close);
	    }

	    break;

	case OP_RECV:
	    if ( (GCwinsock2_OP_RECV_complete(dwError, &status, BytesTransferred, lpPerIoData)) == FALSE )
		return;  /* another WSARecv() was issued...not done yet */
	    if (status == ERROR_OPERATION_ABORTED)
		goto AIO_Callback_err_exit;
	    break;

	default:
	    return;
    }

    /*
    ** Release PER_IO_DATA before running completion routine so that
    ** it can be reused if we are called back with another request
    ** from within the completion routine.  The PER_IO_DATA is no
    ** longer needed at this point, so it is safe to release.
    ** This also helps with reuse of pre-allocated PER_IO_DATA(s)
    ** by making them available a bit sooner.
    */
    GCwinsock2_rel_piod(lpPerIoData);	/* Release PER_IO_DATA first */
    lpPerIoData = NULL;

    /*
    ** Drive the completion routine.
    */
    (*parm_list->compl_exit)( parm_list->compl_id );

AIO_Callback_err_exit:
    if (lpPerIoData)
	GCwinsock2_rel_piod(lpPerIoData);
    InterlockedDecrement(&pcb->AsyncIoRunning);
    GCTRACE(5)("GCwinsock2_AIO_Callback %s: %p Exited\n\n",
		proto, parm_list);
}



/*
**  Name: GCwinsock2_schedule_completion
**  Description:
**	Schedule callback of the completion routine for the request.
**	This is an alternative to calling the completion routine
**	directly.  It must be used when the thread that is handling
**	the completion is different from the thread that originated
**	the request; the completion routine is required to be invoked
**	from the thread of the original request.
**
**	Two(2) options are supported:
**	1. Queue a UserAPC to the target thread's and invoke the
**	   completion routine from there.
**	2. (Legacy) Queue a request element to the GCC completion
**	   queue (global) and set event to trigger routine in main
**	   thread to run the completion routine.  This only works
**	   reliably for GC* servers which are single-threaded and
**	   only wait on the completion queue event from the main
**	   thread.  This option has no thread awareness, which would
**	   be needed in any multi-threaded environment, like the DBMS.
**	   Another drawback of this approach over option 1 is the
**	   extra overhead since this option involves acquiring and
**	   releasing a sync object, setting an event, ...all system
**	   functions.  One potential advantage is that the completion
**	   routine will be invoked from the requester thread in a
**	   "normal" thread context whereas, with option 1, the
**	   completion routine is invoked in a UserAPC callback mode.
**
**	The following are scenarios that must call this function:
**        1. Completion Port IO, which runs in worker threads.
**        2. Alertable IO async connects and listens, which run in worker
**	     threads.
**	  3. Disconnects that had been queued to the disconnect thread
**	     awaiting completion of all pending IOs.
**
**  Inputs:
**	One and only one of the following inputs should be provided.
**	Supply the rq if one already exists, else one will be built
**	from the parm list:
**      rq_in      	Ptr to request queue entry.
**			If NULL, one will be created if needed.
**      parm_list_in	Ptr to function parm list.
**			If NULL, rq_in required and must contain parm list
**			in rq_in->plist.
**			If not NULL, and if rq_in is not NULL, then
**			parm_list_in should match rq_in->plist (not verified).
**	ThreadId_in	Thread id of original requestor.
**			If NULL, get from rq_in if present, else use current
**			thread.
**
**  Outputs: none
**
**  Returns:
**	OK if successful
**	FAIL or Windows status if unsuccessful (unable to schedule callback)
**
**  History:
**	26-Nov-2008 (Bruce Lunsford) Bug 121285
**	    Split out into standalone function since code is now needed
**	    in 5 places.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Use QueueUserAPC() to queue completions to the target thread
**	    instead of prior mechanism, which allocated memory for request,
**	    queued it to global completion Q (synchronized) and set
**	    GCC_COMPLETE event.  New mechanism is thread-aware and simpler--
**	    probably faster as well.  Add target thread to input parm list;
**	    ThreadId_in takes precedence, but if not specified there, then
**	    use the one in the rq_in.  If zero, then get a handle to the
**	    current thread.
*/
STATUS
GCwinsock2_schedule_completion(REQUEST_Q *rq_in, GCC_P_PLIST *parm_list_in, DWORD ThreadId_in)
{
    STATUS	status = OK;
    STATUS	temp_status;
    bool	retval = TRUE;
    REQUEST_Q	*rq = rq_in;
    GCC_P_PLIST *parm_list = parm_list_in ? parm_list_in : (rq ? rq->plist : parm_list_in);
    DWORD	TargetThreadId = ThreadId_in ? ThreadId_in : (rq ? rq->ThreadId : 0);
    DWORD	CurrentThreadId = GetCurrentThreadId();

    GCTRACE(5)("GCwinsock2_schedule_completion Entered: rq=%p, parm_list=%p, target thread id=%d\n",
		rq, parm_list, TargetThreadId);

    /*
    ** Sanity check the parm_list.
    */
    if (!parm_list || !parm_list->compl_exit)
    {
	GCTRACE(1)("GCwinsock2_schedule_completion ERROR: parmlist or compl exit NULL!...exit; rq=%p, parm_list=%p, target thread id=%d\n",
		rq, parm_list, TargetThreadId);
	return(FAIL);
    }

    /*
    ** If target thread id is zero, use our own thread id as the target.
    */
    if (!TargetThreadId)
	TargetThreadId = CurrentThreadId;

    /*
    ** If target and current are equal, it might make sense to call the
    ** completion routine directly from here.  Currently, however, we
    ** always queue the request.
    */

    /*
    ** If QueueUserAPC() exists and has not been disabled by setting
    ** II_WINSOCK2_EVENT_Q=ON, queue a UserAPC to the original
    ** calling thread to invoke the completion routine.
    */
    if ( (!GCWINSOCK2_event_q) && (pfnQueueUserAPC != NULL) )
    {   
	HANDLE hThread;
	/*
	** A thread handle is required to queue it with QueueUserAPC.
	*/
        hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, TargetThreadId);
	if (hThread == NULL)
	{
	    temp_status = GetLastError();
	    GCTRACE(1)("GCwinsock2_schedule_completion: WARNING: OpenThread() error=%d on thread id=%d for parm_list=%p\n",
			temp_status, TargetThreadId, parm_list);
	    goto legacy_callback;
	}

	/*
	** If an rq has been allocated already (such as for async
	** completing disconnects), free it since this code doesn't
	** use it.
	*/
	if ( rq_in != NULL )
	    MEfree ((PTR)rq_in);

	GCTRACE(5)("GCwinsock2_schedule_completion: Queue UserAPC for parm_list=%p, target thread id=%d\n",
		    parm_list, TargetThreadId);
	pfnQueueUserAPC((PAPCFUNC)(GCwinsock2_async_completion),
			hThread, 
                        (DWORD)parm_list);
	CloseHandle(hThread);
	return(OK);
    }

legacy_callback:
    /*
    ** Legacy callback mechanism - requires GCC_COMPLETE event/processing.
    ** - Only used if II_WINSOCK2_EVENT_Q set to ON.
    ** Create a request queue entry if not provided as input parm.
    */
    if ( rq_in == NULL )
    {
	rq = (REQUEST_Q *)MEreqmem(0, sizeof(*rq), TRUE, NULL);
	if (rq == NULL)
	{
	    GCTRACE(1)("GCwinsock2_schedule_completion ERROR: Unable to allocate rq for parm_list=%p\n", parm_list);
	    return(FAIL);
	}
	rq->plist = parm_list;
	rq->ThreadId = TargetThreadId;  /* Currently just informational */
    }

    GCTRACE(5)("GCwinsock2_schedule_completion: Queue rq=%p to GCC_COMPLETE for parm_list=%p\n",
		rq, rq->plist);
    /*
    ** Get critical section for completion Q.
    */
    EnterCriticalSection( &GccCompleteQCritSect );

    /*
    ** Now insert the completed request into the completed Q.
    ** Place at end to cause FIFO processing.
    */
    QUinsert(&rq->req_q, IIGCc_proto_threads.completed_head.q_prev);

    /*
    ** Exit/leave critical section for completion Q.
    */
    LeaveCriticalSection( &GccCompleteQCritSect );

    /*
    ** Raise the completion event to wake up GCexec.
    */
    if (!SetEvent(hAsyncEvents[GCC_COMPLETE]))
    {
	status = GetLastError();
	GCTRACE(1)("GCwinsock2_schedule_completion SetEvent error = %d\n", status);
    }

    return(status);
}


/******************************************************************************
**
** Name: GCwinsock2_async_completion
**
** Description:
**	This function calls the completion routine for a request that 
**	completed in a thread other than the original thread of the
**	request.  This function is itself run as a completion routine
**	for a Windows User APC that is queued to the original thread by
**	GCwinsock2_schedule_completion().
**      The thread must be in an alertable wait state for this function
**      to be executed.
**	TIP: There might be a temptation to eliminate this function
**	     and queue the UserAPC directly to the actual requester's
**	     completion routine since, as seen below, this routine does
**	     nothing more than turn around and call the requester's
**	     completion routine.  DON'T BOTHER! ... the call mechanism
**	     for this Windows UserAPC routine is WINAPI which is #defined
**	     to __stdcall, which is different from the default call
**	     mechanism which is used by all the Ingres programs.  If that
**	     were to ever get in sync, then this middle-man function could
**	     be eliminated.
**
** Inputs:
**      parm_list	Ptr to function parm list.
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**          Created.  Modeled after gcacl.c GCasyncCompletion().
**
******************************************************************************/
VOID WINAPI
GCwinsock2_async_completion( GCC_P_PLIST *parm_list )
{
    char	*proto = parm_list->pce->pce_pid;
    GCTRACE(3)( "\nGCwinsock2_async_completion %s: %p Entry point.\n",
		proto, parm_list );

    (*parm_list->compl_exit)( parm_list->compl_id );

    GCTRACE(3)( "GCwinsock2_async_completion: Exit point.\n\n" );
    return;
}

/*
**  Name: GCwinsock2_OP_RECV_complete
**  Description:
**	This is a shared RECEIVE completion routine for asynchronous I/O
**	called both from Alertable I/O and I/O Completion Port logic.
**	It has 2 modes of operation:  
**	1. 2-step receive:
**	   First, a receive for just the 2-byte message length prefix is
**	   done, followed by a receive for the rest of the message.
**	   This is the original mode (but new code) used by this driver.
**	   Used if a receive overflow buffer was not allocated, which is 
**	   controlled by setting config parm winsock2_recv_buffer_size
**	   or Ingres variable II_WINSOCK2_RECV_BUFFER_SIZE to zero.
**	2. 1-step receive:
**	   A receive is done for the full message, including the length
**	   prefix, based on the available space in the receive and overflow
**	   buffers.  If more than the one message is received, then the
**	   excess is copied to the receive overflow buffer to await the
**	   next GCC_RECEIVE request.
**	   This is the newer, default mode used by this driver and is
**	   modeled after the Unix implementation.  It "should" provide
**	   better performance as the # of RECV requests issued to the OS
**	   is cut approximately in half.
**
**  Inputs:
**      dwError         Error indicator received by callback mechanism
**			for WSARecv.
**      &status         Ptr to error code from WSAGetLastError() after WSARecv.
**      BytesTransferred  Number of bytes received into target buffer.
**	lpPerIoData	Control block associated with the I/O - contains
**			the Overlapped area as well as data specific to
**			this protocol.
**
**  Outputs:
**	status		May be overridden to ERROR_OPERATION_ABORTED.
**
**  Returns:
**                      TRUE  if receive is completed.  Ie, either:
**			  - error in receive, or
**			  - entire message received
**                      FALSE if receive is incomplete. Ie, another
**			  receive issued...for rest of the message.
**
**  History:
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Created.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Check for timeout errors; if WSAETIMEDOUT returned from OS,
**	    then return GC_TIME_OUT in parm_list->generic_status.
**	    Set system_status to ER_close if receive failed due to
**	    connection having been closed (BytesTransferred == 0);
**	    can be used by caller to detect closed socket.  Also
**	    clear pcb->tot_rcv if error occurs, as done elsewhere.
*/
bool 
GCwinsock2_OP_RECV_complete(DWORD dwError, STATUS *lpstatus, DWORD BytesTransferred_in, PER_IO_DATA *lpPerIoData)
{
    STATUS		status = *lpstatus;
    DWORD		BytesTransferred = BytesTransferred_in;
    GCC_P_PLIST		*parm_list;
    char		*proto;
    PCB2		*pcb;
    int			i;
    DWORD		dwBytes;
    DWORD		dwBytes_wanted;
    i4			len;
    i4			len_prefix = lpPerIoData->block_mode ? 0 : 2;

    parm_list = lpPerIoData->parm_list;
    proto = parm_list->pce->pce_pid;
    pcb = (PCB2 *)parm_list->pcb;

    if (dwError != OK)
    {
	GCTRACE(1)("GCwinsock2_OP_RECV_complete %s %d: %p Failed with status %d in state=%d on socket=0x%p\n",
		    proto, pcb->id, parm_list, status, lpPerIoData->state, pcb->sd);
	pcb->tot_rcv = 0;
	if (status == ERROR_OPERATION_ABORTED)
	    return TRUE; /* GCC_RECEIVE is done */

	if (lpPerIoData->state == PIOD_ST_TIMED_OUT)
	{
	    /*
	    ** The requester has already been notified of the timeout.
	    ** Nothing left to do except "eat" the IO completion.
	    */
	    *lpstatus = ERROR_OPERATION_ABORTED;
	    return TRUE; /* GCC_RECEIVE is done */
	}

	/*
	** WSAETIMEDOUT is unlikely to ever actually occur since receives
	** are currently all asynchronous and cannot time out.  However,
	** we check for it anyway for completeness.
	*/
	if (status == WSAETIMEDOUT)
	    parm_list->generic_status = GC_TIME_OUT;
	else
	    parm_list->generic_status = GC_RECEIVE_FAIL;
	SETWIN32ERR(&parm_list->system_status, status, ER_socket);
	return TRUE; /* GCC_RECEIVE is done */
    }

    GCTRACE(2)( "GCwinsock2_OP_RECV_complete %s %d: %p Want %d bytes got %d bytes\n",
		proto, pcb->id, parm_list, pcb->tot_rcv, BytesTransferred );

    if (!BytesTransferred)
    {
	/* The session has been disconnected */
	GCTRACE(2)("GCwinsock2_OP_RECV_complete %s %d: %p Failed- 0 bytes trfrd indicating session disconnected, socket=0x%p\n",
		    proto, pcb->id, parm_list, pcb->sd);
	pcb->session_state = SS_DISCONNECTED;
	pcb->tot_rcv = 0;
	if (lpPerIoData->state == PIOD_ST_TIMED_OUT)
	{
	    /*
	    ** The requester has already been notified of the timeout.
	    ** Nothing left to do except "eat" the IO completion.
	    */
	    *lpstatus = ERROR_OPERATION_ABORTED;
	    return TRUE; /* GCC_RECEIVE is done */
	}
	parm_list->generic_status = GC_RECEIVE_FAIL;
	SETWIN32ERR(&parm_list->system_status, 0, ER_close);
	return TRUE; /* GCC_RECEIVE is done */
    }

    /*
    ** Check if data was received into overflow buffer area rather
    ** than requester's buffer; if so, it must be copied over...
    ** whatever will fit.  Data gets read into the overflow area
    ** on receives with a timeout value (not -1) specified.
    */
    if ( (lpPerIoData->state == PIOD_ST_IO_TMO_PENDING) ||
	 (lpPerIoData->state == PIOD_ST_REDIRECT) )
    {
	pcb->b2 += BytesTransferred;	/* Adjust ptr to end of data read */
	/*
	** Redirect target from overflow to caller's buffer.
	*/
	pcb->rcv_bptr = parm_list->buffer_ptr - len_prefix;
	pcb->tot_rcv  = parm_list->buffer_lng + len_prefix;
	/*
	** Copy data from overflow to caller's buffer.
	** NOTES re. function GCwinsock2_copy_ovflo_to_inbuf():
	** 1. It adjusts pcb->tot_rcv & rcv_bptr.
	** 2. It may increase BytesTransferred because, theoretically, there
	**    may have already been data in the overflow prior to this receive.
	**    All that will fit into caller's buffer is copied.
	*/
	GCwinsock2_copy_ovflo_to_inbuf(pcb, parm_list, lpPerIoData->block_mode, &BytesTransferred);
	if (parm_list->generic_status != OK)
	    return TRUE; /* GCC_RECEIVE is done */
	lpPerIoData->state = PIOD_ST_IO_PENDING;
    }
    else	/* data was read directly into caller's buffer */
    {
	pcb->tot_rcv -= BytesTransferred;
	pcb->rcv_bptr += BytesTransferred;
    }

    /*
    ** If this is a block mode protocol, once the
    ** recv completes successfully, we should have
    ** recv'd all the bytes we're going to so we're
    ** done.
    */
    if (lpPerIoData->block_mode)
    {
	parm_list->buffer_lng = BytesTransferred;
	return TRUE; /* GCC_RECEIVE is done */
    }

    /* The remainder of this code below only runs for non-block mode */

    if (pcb->rcv_bptr < parm_list->buffer_ptr)
    {
	/*
	** Haven't received the all of the 2-byte length header yet.
	** We need to issue another recv to get the rest of the
	** message (or at least the header).
	*/

	ZeroMemory(&lpPerIoData->Overlapped, sizeof(OVERLAPPED));
	lpPerIoData->wbuf.buf = pcb->rcv_bptr;
	lpPerIoData->wbuf.len = dwBytes_wanted = pcb->tot_rcv;

	i = 0;
	status = WSARecv( pcb->sd, &lpPerIoData->wbuf, 1, &dwBytes, &i,
			  &lpPerIoData->Overlapped,
			  GCwinsock2CompletionPort ? NULL : GCwinsock2_AIO_Callback );
	if (status == SOCKET_ERROR)
	{
	    status = WSAGetLastError();
	    if (status != WSA_IO_PENDING)
	    {
		parm_list->generic_status = GC_RECEIVE_FAIL;
		SETWIN32ERR(&parm_list->system_status, status,
			    ER_socket);
		GCTRACE(1)("GCwinsock2_OP_RECV_complete %s %d: %p WSARecv() failed with status %d, socket=0x%p\n",
		        proto, pcb->id, parm_list, status, pcb->sd);
		return TRUE; /* GCC_RECEIVE is done */
	    }
	}
	GCTRACE(4)( "GCwinsock2_OP_RECV_complete %s %d: %p want %d bytes got %d bytes\n",
		    proto, pcb->id, parm_list, dwBytes_wanted, dwBytes );

	return FALSE;  /* GCC_RECEIVE is not yet done...need more data. */
    }  /* End if still need rest of 2-byte length header */

    /*
    ** We've gotten (at least) the complete message length header. Now check
    ** the hdr to see if it is valid (ie, msg will fit within caller's buffer).
    ** Then see if we also have the complete message ... if not,
    ** another receive will need to be issued.
    ** If we have more than the message, copy the excess to the
    ** overflow buffer in preparation for a subsequent receive.
    */
    len = ((u_char *)parm_list->buffer_ptr)[-2] +
	  ((u_char *)parm_list->buffer_ptr)[-1] * 256 - 2;
    if (len < 0 || len > parm_list->buffer_lng)
    {
	parm_list->generic_status = GC_RECEIVE_FAIL;
	GCTRACE(1)( "GCwinsock2_OP_RECV_complete %s %d: %p failure - Invalid incoming message len = %d bytes, buffer = %d bytes, socket=0x%p\n",
	            proto, pcb->id, parm_list,
	            len, 
	            parm_list->buffer_lng,
	            pcb->sd);
	/* Dump 1st 1024 bytes (arbitrary max) of invalid incoming message */
	if( GCWINSOCK2_trace )
	    gc_tdump(&parm_list->buffer_ptr[-4],
		     (BytesTransferred > 1024) ? 1024 : BytesTransferred);
	return TRUE; /* GCC_RECEIVE is done */
    }
    parm_list->buffer_lng  = len;

    len = pcb->rcv_bptr - ( parm_list->buffer_ptr + parm_list->buffer_lng );

    /*
    ** A negative length indicates the number of bytes
    ** remaining in the current packet to be read.
    **
    ** Reinvoke for short read.
    */
    if( len < 0 )
    {
	pcb->tot_rcv = -len;
	ZeroMemory(&lpPerIoData->Overlapped, sizeof(OVERLAPPED));
	lpPerIoData->wbuf.buf = pcb->rcv_bptr;
	lpPerIoData->wbuf.len = dwBytes_wanted = pcb->tot_rcv;

	i = 0;
	status = WSARecv( pcb->sd, &lpPerIoData->wbuf, 1, &dwBytes, &i,
		      &lpPerIoData->Overlapped,
		      GCwinsock2CompletionPort ? NULL : GCwinsock2_AIO_Callback );
	if (status == SOCKET_ERROR)
	{
	    status = WSAGetLastError();
	    if (status != WSA_IO_PENDING)
	    {
		parm_list->generic_status = GC_RECEIVE_FAIL;
		SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		GCTRACE(1)("GCwinsock2_OP_RECV_complete %s %d: %p WSARecv() #2 failed with status %d, socket=0x%p\n",
		        proto, pcb->id, parm_list, status, pcb->sd);
		return TRUE; /* GCC_RECEIVE is done */
	    }
	}
	GCTRACE(4)( "GCwinsock2_OP_RECV_complete %s %d: %p Want %d bytes got %d, msg len %d remaining %d\n",
		proto, pcb->id, parm_list, dwBytes_wanted, dwBytes,
		parm_list->buffer_lng, -len );
	return FALSE;  /* GCC_RECEIVE is not yet done...need more data. */
    }  /* End if len < 0 */

    /*
    ** A positive length indicates excess data received
    ** beyond the current packet.
    **
    ** Copy any excess
    */
    if( len > 0 )
    {
	GCTRACE(3)("GCwinsock2_OP_RECV_complete %s %d: %p saving %d\n",
	    proto, pcb->id, parm_list, len );
	MEcopy( pcb->rcv_bptr - len, len, pcb->rcv_buffer );
	pcb->b1 = pcb->rcv_buffer;
	pcb->b2 = pcb->rcv_buffer + len;
    }

    return TRUE; /* GCC_RECEIVE is done */
}


/*
**  Name: GCwinsock2_copy_ovflo_to_inbuf
**  Description:
**	Copy what we can from the overflow buffer into the
**	requester's receive buffer.
**
**	Data can reside in the overflow area due to one of the
**	following prior events:
**	1. Data came in after a receive timed out.
**	2. Data from more than 1 message was received together;
**	   anything past 1st message was saved in overflow.  This can
**	   only occur with stream mode and not with block mode.
**
**  Inputs:
**      pcb    	        Protocol Connection Block, which includes receive
**			overflow buffer area and related pointers.
**	    ->b1 to b2     (area to copy from)
**	    ->rcv_bptr     (area to copy to)
**	    ->tot_rcv      (len of area to copy to)
**      parm_list       Caller's input parameter list for the receive.
**	block_mode      Block/message or stream (with 2-byte prefix) mode
**	&BytesTransferred Ptr to output # of bytes copied
**
**  Outputs:
**	parm_list->generic_status  Set to GC_RECEIVE_FAIL if error
**	parm_list->buffer_lng      Set to "logical" message length (not
**		                   including prefix) if found whole msg.
**	pcb->rcv_bptr and tot_rcv  Ptr and len of target (adjusted by
**		                   # of bytes copied into it)
**	BytesTransferred           # of bytes copied
**
**  Returns:
**                      TRUE  if whole   message copied or ERROR
**                      FALSE if partial message copied...ie, need more data.
**
**  History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
*/
bool
GCwinsock2_copy_ovflo_to_inbuf(PCB2 *pcb, GCC_P_PLIST *parm_list, bool block_mode, DWORD *BytesTransferred)
{
    char	*proto = parm_list->pce->pce_pid;
    bool	got_whole_msg = FALSE;
    i4		len;
    i4		excess = 0;
    i4		len_prefix = block_mode ? 0 : 2;

    len = pcb->b2 - pcb->b1;  /* #bytes in ovflo buffer */

    if (block_mode)	  /* block mode => use whatever will fit */
    {
	got_whole_msg = TRUE;
	if (len > parm_list->buffer_lng)
	{
	    excess = len - parm_list->buffer_lng;
	    len = parm_list->buffer_lng;
	}
    }
    else                  /* streams mode => ck if got entire msg */
    {
	/* If we have the NDU header, use the length */
	if( len >= 2 )
	{
	    i4 len2 = ((u_char *)pcb->b1)[0] +
	              ((u_char *)pcb->b1)[1] * 256;
	    /*
	    ** Ensure buffered msg has valid length...must be at
	    ** least 2 for NDU hdr & must fit in caller's buffer.
	    */
	    if (len2 < 2 || len2 > parm_list->buffer_lng + 2)
	    {
		parm_list->generic_status = GC_RECEIVE_FAIL;
		GCTRACE(1)( "GCwinsock2_copy_ovflo_to_inbuf %s %d: GCC_RECEIVE %p failure - Invalid incoming message len = %d bytes, buffer = %d bytes, socket=0x%p\n",
		             proto, pcb->id, parm_list,
		             len2 - 2, 
		             parm_list->buffer_lng,
		             pcb->sd);
		*BytesTransferred = 0;
		return(TRUE);
	    }

	    excess = len - len2;	/* calc excess/under */
	    if (excess >= 0)  /* Have a whole msg? */
	    {                 /* - whole msg (or more) */
		got_whole_msg = TRUE;
		len = len2;             /* only take 1st msg  */
	    }
	}  /* End if have at least 2 bytes in buffer */
    } /* End if streams mode */

    GCTRACE(3)("GCwinsock2_copy_ovflo_to_inbuf %s %d: GCC_RECEIVE %p using %d - %s message, excess=%d buffered\n",
		proto, pcb->id, parm_list, len,
		got_whole_msg ? "whole" : "part",
		excess );

    MEcopy( pcb->b1, len, pcb->rcv_bptr );
    pcb->rcv_bptr += len;
    pcb->tot_rcv -= len;
    pcb->b1 += len;
    *BytesTransferred = len;

    /*
    ** If we've copied out everything from the overflow
    ** buffer, then reset the pointers to the overflow
    ** buffer back to the beginning (indicating nothing
    ** in the overflow buffer).
    ** NOTE: b1 and b2 should be equal in this condition,
    ** but we check for > also just to be safe.
    ** NOTE: Another option here would be to always copy
    ** the excess data, if any, to the start of the buffer;
    ** this would maximize the buffer area, but require a
    ** perhaps unneeded copying of data.
    */
    if (pcb->b1 >= pcb->b2)
	pcb->b1 = pcb->b2 = pcb->rcv_buffer;

    /*
    ** Tell caller if an entire message was found in (and
    ** copied from) the overflow area (from prior receive(s)).
    ** Allows caller to skip any further WSARecv()'s...
    ** Ie, the GCC_RECEIVE is done!
    */
    if (got_whole_msg == TRUE)
    {
	parm_list->buffer_lng  = len - len_prefix;
    }

    return(got_whole_msg);
}


/*
**  Name: GCwinsock2_set_timer
**  Description:
**	This function creates (if not already created) a waitable timer
**	and then sets the timer for the specified amount of time.
**
**  Inputs:
**	lpPerIoData	Control block associated with the I/O - contains
**			the Overlapped area as well as data specific to
**			this protocol.
**	timeout		Timeout period in milliseconds.
**
**  Outputs:
**	lpPerIoData->hWaitableTimer  Handle to Waitable Timer for request
**		                     (created if non-existent)
**
**  Returns:
**                      OK     if timer set successfully
**                      status of failed OS timer function
**
**  History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
*/
STATUS
GCwinsock2_set_timer(PER_IO_DATA *lpPerIoData, i4 timeout)
{
    STATUS	status = OK;
    GCC_P_PLIST	*parm_list = lpPerIoData->parm_list;
    char	*proto = parm_list->pce->pce_pid;
    PCB2	*pcb   = (PCB2 *)parm_list->pcb;

    LARGE_INTEGER	li;
    LONGLONG		nanosec;

    /*
    ** Create a waitable timer for the request if one does not
    ** already exist.
    */
    if (!lpPerIoData->hWaitableTimer)
    {
	SECURITY_ATTRIBUTES sa;
	iimksec (&sa);
	sa.bInheritHandle = FALSE;
	lpPerIoData->hWaitableTimer = CreateWaitableTimer(&sa, FALSE, NULL);
	if (!lpPerIoData->hWaitableTimer)
	{
	    status = GetLastError();
	    GCTRACE(1)("GCwinsock2_set_timer %s %d: %p CreateWaitableTimer() failed, status=%d\n",
			proto, pcb->id, parm_list, status);
	    return(status);
	}
    }

    /*
    ** Set the timer to specified timeout value.
    */

    nanosec = (LONGLONG) timeout * 10000;

    /*
    ** Negate the time so that SetWaitableTimer knows
    ** we want relative time instead of absolute time.
    ** This negative value tells the function to go off 
    ** at a time relative to the time when we call the function.
    */
    nanosec=-nanosec;

    /*
    ** Copy the relative time from a __int64 into a LARGE_INTEGER.
    */
    li.LowPart = (DWORD) (nanosec & 0xFFFFFFFF );
    li.HighPart = (LONG) (nanosec >> 32 );

    if ( SetWaitableTimer(lpPerIoData->hWaitableTimer, &li, 0, GCwinsock2_timer_callback, lpPerIoData, FALSE) == 0)
    {
	status = GetLastError();
	GCTRACE(1)("GCwinsock2_set_timer %s %d: %p SetWaitableTimer() failed for %d msecs, status=%d\n",
			proto, pcb->id, parm_list, timeout, status);
	return(status);
    }

    lpPerIoData->state = PIOD_ST_IO_TMO_PENDING;

    GCTRACE(3)("GCwinsock2_set_timer %s %d: %p timeout(millisec)=%d\n",
		proto, pcb->id, parm_list, timeout);
    return(status);
}


/*
**  Name: GCwinsock2_timer_callback
**  Description:
**	This function is called by the OS if a waitable timer expires.
**	It then calls back, with a GC_TIME_OUT error, the completion routine
**	for the request that timed out.  NOTE that although this timer
**	was started for a request that had a timeout ( >0 ), the
**	associated actual IO request is not touched in any way and
**	will complete independently later, at which time it will be
**	either ignored or linked up with a new request if one has come in.
**	The PerIoData state is merely set to PIOD_ST_TIMED_OUT at this
**	time to note that the original request (not the IO) has timed out.
**	In other words, the original request is timed out, but not the IO
**	because there is no way to cancel it.
**
**  Inputs:
**	lpPerIoData	Control block associated with the I/O - contains
**			the Overlapped area as well as data specific to
**			this protocol.
**	dwTimerLowValue	Low order portion of timeout period
**	dwTimerHighValue High order portion of timeout period
**
**  Outputs: None
**
**  Returns: None
**
**  History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.
*/
VOID CALLBACK
GCwinsock2_timer_callback(PER_IO_DATA *lpPerIoData, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
    GCC_P_PLIST	*parm_list = lpPerIoData->parm_list;
    char	*proto = parm_list->pce->pce_pid;
    PCB2	*pcb   = (PCB2 *)parm_list->pcb;

    GCTRACE(3)("GCwinsock2_timer_callback %s %d: %p Timer Expired - call completion with GC_TIME_OUT error\n",
		proto, pcb->id, parm_list);

    lpPerIoData->state = PIOD_ST_TIMED_OUT;

    /*
    ** Drive the completion routine with timeout error.
    */
    parm_list->generic_status = GC_TIME_OUT;
    (*parm_list->compl_exit)( parm_list->compl_id );

    return;
}


/*
**  Name: GCwinsock2_close_listen_sockets
**  Description:
**	This function closes all of the listen sockets and frees 
**	associated memory.
**
**  History:
**      28-Aug-2006 (lunbr01) Sir 116548
**          Add IPv6 support.
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add Sleep for minumum duration after closing each socket
**	    to allow any triggered reactions to complete before returning
**	    to the caller.  This avoids potential shutdown problems
**	    where the triggered events don't occur until later and 
**	    expected resources have already been freed.  For example,
**	    with AIO async connects/listens, the GCwinsock2_exit() code
**	    could terminate the worker threads and free the connect/listen
**	    arrays before or while a worker thread is woken up to process
**	    the closed listen socket, which results in an AccVio.
**	    This problem could have been encountered prior to this Sir.
*/
VOID
GCwinsock2_close_listen_sockets(GCC_LSN *lsn)
{
    int 	i;
#define	AFTER_CLOSE_SOCKET_WAIT_TIME	50  /* milliseconds to sleep */

    if (lsn->addrinfo_list)
    {
	freeaddrinfo(lsn->addrinfo_list);
	lsn->addrinfo_list = NULL;
	if (lsn->listen_sd)
	{
	    for (i=0; i < lsn->num_sockets; i++)  /* close open sockets */
	    {
	        if (lsn->listen_sd[i] != INVALID_SOCKET)
	        {
		    setsockopt(lsn->listen_sd[i], SOL_SOCKET, 
			       SO_DONTLINGER,
			       (u_caddr_t)&i, sizeof(i));
	            closesocket(lsn->listen_sd[i]);
		    GCTRACE(1)( "GCwinsock2_close_listen_sockets: socket=0x%p closed\n",
            		lsn->listen_sd[i]);
		    Sleep(AFTER_CLOSE_SOCKET_WAIT_TIME);
	        }
	    }
	    lsn->num_sockets = 0;
	    MEfree((PTR)lsn->listen_sd);
	    lsn->listen_sd = NULL;
	}
    }
}


/*
**  Name: GCdns_hostname()
**
**  Description:
**
**      GCdns_hostname() -- get fully-qualified host name
**
**      This function differs from GChostname() and PMhost() in that it
**      fetches the true, fully qualified network name of the local host--
**      provided that such a name exists.  If a fully qualified network
**      name cannot be found, an empty string is returned.  If a fully 
**      qualified network name is found, but the output buffer is too 
**      small, the host name is truncated and an error is returned.
**
**  Inputs:
**      hostlen         Byte length of host name buffer.
**      ourhost         Allocated buffer at least "hostlen" in size.
**
**  Outputs:
**      ourhost         Fully qualified host name - null terminated string.
**
**  Returns:
**                      OK
**                      GC_NO_HOST
**                      GC_HOST_TRUNCATED
**                      GC_INVALID_ARGS
**  Exceptions:
**                      N/A
**
**  History:
**    03-Aug-2007 (Ralph Loen) SIR 118898
**      Created.
**    17-Dec-2009 (Bruce Lunsford) Sir 122536
**      Add support for long identifiers. Use cl-defined GC_HOSTNAME_MAX
**      (=256 on Windows) instead of gl-defined GCC_L_NODE (=64).
*/

STATUS
GCdns_hostname( char *ourhost, i4 hostlen )
{
    struct hostent *hp, *hp1;
    struct addrinfo *result=NULL;
    int err;
    size_t size;
    struct sockaddr *s;
    char hostname[GC_HOSTNAME_MAX+1]="\0";
    u_i4 i = 0;
    char *ipaddr;
    int addr;
    STATUS status = OK;
    i2 len;

    /*
    ** Evaluate input arguments.
    */
    
    if ((ourhost == NULL) || (hostlen <= 0) )
    {
        status = GC_INVALID_ARGS;
        goto exit_routine; 
    }

    ourhost[0] = '\0';

    status = GCwinsock2_startup_WSA();

    /*
    ** gethostname() is the most straightforward means of getting
    ** the host name.
    */
    
    hostname[0] = '\0';
    if (gethostname(hostname, GC_HOSTNAME_MAX))
        goto deprecated_funcs; 

    /*
    ** Sometimes gethostname() returns the fully qualified host name, so
    ** there is nothing more required.
    */
    if (STindex(hostname, ".", 0))
    {
        len = STlength(hostname);
        if ((len > hostlen-1) || (len > GC_HOSTNAME_MAX))
        {
            STlcopy(hostname, ourhost, hostlen-1);
            status = GC_HOST_TRUNCATED;
        }
        else
            STcopy(hostname, ourhost);
        goto exit_routine;
    }

    /*
    ** Try getaddrinfo() and getnameinfo() first.
    */
    if (err = getaddrinfo(hostname, NULL, NULL, &result))
        goto deprecated_funcs;

    for( i=0; result; result = result->ai_next, i++ )
    {
        if (result->ai_canonname)
        {
            if (STindex(result->ai_canonname,".",0))
            {
                len = STlength(result->ai_canonname);
                if ((len > hostlen-1) || (len > GC_HOSTNAME_MAX))
                {
                    STlcopy(result->ai_canonname, ourhost, hostlen-1);
                    status = GC_HOST_TRUNCATED;
                }
                else
                    STcopy (result->ai_canonname, ourhost);
                goto exit_routine;
            }
        }
        s = result->ai_addr;
        size = result->ai_addrlen;
        err = getnameinfo(s, 
            size,
            hostname, sizeof(hostname),         
            NULL, 
            0, 
            0);

        if (!err)
        {
            if (STindex(hostname, ".", 0))
            {
                len = STlength(hostname);
                if ((len > hostlen-1) || (len > GC_HOSTNAME_MAX))
                {
                    STlcopy(hostname, ourhost, hostlen-1);
                    status = GC_HOST_TRUNCATED;
                }
                else
                    STcopy(hostname, ourhost);
                goto exit_routine;
            }
        }
    } /* for( i=0; result; result = result->ai_next, i++ ) */

deprecated_funcs:
    /*
    ** If the preferred approach, using getnameinfo() and getaddrinfo(),
    ** doesn't produce a fully qualified host name, fall back on
    ** the deprecated gethostname() and gethostbyname() functions.
    */        
    hp  = gethostbyname(hostname);
    if (hp != NULL) 
    {
        if (STindex(hp->h_name, ".", 0))
        {
            len = STlength(hp->h_name);
            if ((len > hostlen-1) || (len > GC_HOSTNAME_MAX))
            {
                STlcopy(hp->h_name, ourhost, hostlen-1);
                status = GC_HOST_TRUNCATED;
            }
            else
                STcopy(hp->h_name, ourhost);
            goto exit_routine;
        }
        /*
        ** If the result of gethostbyname() didn't return anything useful,
        ** search through the address list and see if something turns up
        ** from gethostbyaddr().
        */
        i = 0;
        while ( hp->h_addr_list[i] != NULL)
        {
            ipaddr = inet_ntoa( *( struct in_addr*)( hp-> h_addr_list[i])); 
            if (ipaddr)
            {
                addr = inet_addr(ipaddr);
                hp1 = gethostbyaddr((const char *)&addr, sizeof(struct in_addr),
                    AF_INET);
                if (hp1)
                {
                    if (STindex(hp1->h_name, ".", 0))
                    {
                        len = STlength(hp1->h_name);
                        if ((len > hostlen-1) || (len > GC_HOSTNAME_MAX) )
                        {
                            STlcopy(hp1->h_name, ourhost, hostlen-1);
                            status = GC_HOST_TRUNCATED;
                        }
                        else
                            STcopy(hp1->h_name, ourhost);
                        goto exit_routine;
                    } 
                }
            } /* if (ipaddr) */              
           i++;
        } /* while ( hp->h_addr_list[i] != NULL) */
    } /* if (hp != NULL) */

exit_routine:
    if (result)
        freeaddrinfo(result);
    if (!STlength(ourhost))
        status = GC_NO_HOST;
    else
        CVlower(ourhost);
    return status;
}


/*
**  Name: GCwinsock2_startup_WSA()
**
**  Description:
**
**      GCwinsock2_startup_WSA() -- Start up Winsock facility
**
**      This function must be called before any other winsock functions
**      are called.  It calls WSAStartup() which initializes the winsock
**	facility.
**
**  Inputs:
**      none
**
**  Outputs:
**      none
**
**  Returns:
**                      OK
**                      FAIL
**  Exceptions:
**                      N/A
**
**  History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add function flower box header.
**	    Change startup_data from static to local variable as it is not
** 	    referenced anywhere except in this function.  WSAStartup_called
**	    is now a "use count" rather than a true/false boolean.
*/
STATUS
GCwinsock2_startup_WSA()
{
    WORD	version;
    WSADATA	startup_data;
    int	err;

    /*
    ** Keep a driver instance/thread "use count" and only call WSAStartup()
    ** on 1st call (or if use_count had dropped to 0 indicating WSACleanup()
    ** had been called). 
    */
    WSAStartup_called++;
    if (WSAStartup_called > 1)
	return(OK);

    version = MAKEWORD(2,2);
    err = WSAStartup(version, &startup_data);
    if (err != 0)
    {
        GCTRACE(1)("GCwinsock2_startup_WSA: Couldn't find usable WinSock DLL, status = %d\n", err);
        return(FAIL);
    }

    /*
    ** Confirm that the WinSock DLL supports 2.2.
    ** Note that if the DLL supports versions greater
    ** than 2.2 in addition to 2.2, it will still return
    ** 2.2 in wVersion since that is the version we
    ** requested.
    */
    if (LOBYTE(startup_data.wVersion) != 2 ||
        HIBYTE(startup_data.wVersion) != 2)
    {
        WSACleanup();
        GCTRACE(1)("GCwinsock2_startup_WSA: Couldn't find usable WinSock DLL\n");
        return(FAIL);
    }
    
    return (OK);
}
/*
**  Name:	gc_tdump	- Trace a buffer.
**
**  Description:
**	Prints out the buffer in hex and ASCII for debugging purposes.
**
**  Inputs:
**	buf		Pointer to buffer
**	len		Buffer length
**
**  Outputs:
**	None.
**
**  History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Cloned from gcx_tdump in gcc tl, with a couple of minor mods
**	    to be equivalent to odbc_hexdump().
*/
VOID					 
gc_tdump( char *buf, i4  len )
{
    char *hexchar = "0123456789ABCDEF";
    char hex[ 49 ]; /* 3 * 16 + \0 */
    char asc[ 17 ]; 
    u_i2 i;

    for( i = 0 ; len-- > 0; buf++ )
    {
	hex[3*i] = hexchar[ (*buf >> 4) & 0x0F ];
	hex[3*i+1] = hexchar[ *buf & 0x0F ];
	hex[3*i+2] =  ' ';
	asc[i++] = CMprint(buf) && !(*buf & 0x80) ? *buf : '.' ;

	if( i == 16 || !len )
	{
	    hex[3*i] = asc [i] = '\0';
	    TRdisplay("%48s    %s\n", hex, asc);
	    i = 0;
	}
    }

    return;
}
