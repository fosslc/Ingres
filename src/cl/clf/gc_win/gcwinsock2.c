/*
**  Copyright (c) 2009 Ingres Corporation
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
GLOBALREF bool			WSAStartup_called;
GLOBALREF HANDLE		hAsyncEvents[];
GLOBALREF CRITICAL_SECTION	GccCompleteQCritSect;
GLOBALREF THREAD_EVENT_LIST	IIGCc_proto_threads;
GLOBALREF BOOL			is_comm_svr;
GLOBALREF i4			GCWINSOCK2_trace;

/*
** The PER_IO_DATA is allocated per I/O request, containing information
** pertaining to the I/O request.
*/
typedef struct _PER_IO_DATA
{
    WSAOVERLAPPED	Overlapped;
    SOCKET		listen_sd;
    SOCKET		client_sd;
    int			OperationType;
#define		OP_ACCEPT	0x010	/* AcceptEx */
#define		OP_CONNECT	0x020	/* ConnectEx */
#define		OP_RECV		0x080	/* WSARecv */
#define		OP_SEND		0x100	/* WSASend */
#define		OP_DISCONNECT	0x200	/* Disconnect */
    WSABUF		wbuf;
    bool		block_mode;
    GCC_P_PLIST		*parm_list;
    GCC_LSN     	*lsn;              /* ptr to LSN struct for addr/port */
    struct addrinfo	*addrinfo_posted;  /* ptr to last posted lsn addr */
    int     		socket_ix_posted;  /* socket index last posted */
    char		AcceptExBuffer[(sizeof(SOCKADDR_STORAGE) + 16) * 2];
} PER_IO_DATA;

/*
**  The PCB is allocated on listen or connection request.  The format is
**  internal and specific to each protocol driver.
*/
typedef struct _PCB
{
    SOCKET		sd;
    i4			tot_snd;	    /* total to send */
    i4			tot_rcv;	    /* total to rcv */
    char		*snd_bptr;	    /* utility buffer pointer */
    char		*rcv_bptr;	    /* utility buffer pointer */
    struct addrinfo	*addrinfo_list;      /* ptr to addrinfo list for connect */
    struct addrinfo	*lpAddrinfo;        /* ptr to current addrinfo entry */
    GCC_ADDR		lcl_addr, rmt_addr;
    volatile LONG	AsyncIoRunning;
    volatile LONG	DisconnectAsyncIoWaits;
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
} PCB;

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
static bool				Winsock2_exit_set = FALSE;
static bool				is_win9x = FALSE;
static WSADATA  			startup_data;
static i4				GCWINSOCK2_timeout;
static bool				GCWINSOCK2_nodelay = FALSE;
static HANDLE				GCwinsock2CompletionPort = NULL;
static u_i2				GCWINSOCK2_recv_buffer_size;
static QUEUE				DisconnectQ_in;
static QUEUE				DisconnectQ;
CRITICAL_SECTION			DisconnectCritSect;
static HANDLE				hDisconnectEvent = NULL;
static HANDLE				hDisconnectThread = NULL;
static LPFN_ACCEPTEX			lpfnAcceptEx = NULL;
static LPFN_CONNECTEX			lpfnConnectEx = NULL;
static LPFN_GETACCEPTEXSOCKADDRS	lpfnGetAcceptExSockaddrs = NULL;

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

STATUS		GCwinsock2_init(GCC_PCE * pptr);
STATUS          GCwinsock2_startup_WSA();
STATUS		GCwinsock2(i4 function_code, GCC_P_PLIST * parm_list);
VOID		GCwinsock2_exit(void);
STATUS		GCwinsock2_schedule_completion(REQUEST_Q *rq, GCC_P_PLIST *parm_list);
DWORD WINAPI	GCwinsock2_worker_thread(LPVOID CompletionPortID);
STATUS		GCwinsock2_open(GCC_P_PLIST *parm_list);
VOID 		GCwinsock2_open_setSPXport(SOCKADDR_IPX *spx_sockaddr, char *port_name);
STATUS		GCwinsock2_listen(void *parms);
VOID 		GCwinsock2_listen_complete(GCC_P_PLIST *parm_list, char *AcceptExBuffer);
STATUS  	GCwinsock2_connect(GCC_P_PLIST *parm_list, char *lpPerIoData);
VOID 		GCwinsock2_connect_complete(GCC_P_PLIST *parm_list);
VOID		GCwinsock2_disconnect_thread();
char *  	GCwinsock2_display_IPaddr(struct addrinfo *lpAddrinfo, char *IPaddr_out);
THREAD_EVENTS	*GCwinsock2_get_thread(char *);
STATUS		GCwinsock2_alloc_connect_events();
STATUS		GCwinsock2_add_connect_event(LPOVERLAPPED lpOverlapped, SOCKET sd);
bool		GCwinsock2_wait_completed_connect(LPOVERLAPPED *lpOverlapped, STATUS *status);
VOID CALLBACK	GCwinsock2_AIO_Callback(DWORD, DWORD, WSAOVERLAPPED *, DWORD);
bool		GCwinsock2_OP_RECV_complete(DWORD dwError, STATUS status, DWORD BytesTransferred, PER_IO_DATA *lpPerIoData);
VOID 		GCwinsock2_close_listen_sockets(GCC_LSN *lsn);


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
    
    /*
    ** Get trace variable
    */
    NMgtAt("II_WINSOCK2_TRACE", &ptr);
    if (!(ptr && *ptr) && PMget("!.winsock2_trace_level", &ptr) != OK)
	GCWINSOCK2_trace = 0;
    else
	GCWINSOCK2_trace = atoi(ptr);

    if (!WSAStartup_called)  
        if (GCwinsock2_startup_WSA() == FAIL)
            return FAIL;

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

    /*
    ** Get pointer to WinSock 2.2 protocol's control entry in table.
    */
    Tptr = (THREAD_EVENTS *)pptr->pce_dcb;
    wsd = &Tptr->wsd;

    /*
    ** Now call the protocol-specific init routine.
    */
    if ((*driver->init_func)(pptr, wsd) != OK)
    {
	GCTRACE(1)("GCwinsock2_init %s: Protocol-specific init function failed\n", proto);
	return(FAIL);
    }

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
	int		tid;

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
	** with Ingres variable or config.dat setting.  The default
	** size is whatever RCV_PCB_BUFF_SZ is #defined to in the PCB...
	** typically 8K or 32K.  This size does not include a 2-byte prefix
	** for the length of the message, which is always present.
	** If this variable is set to zero (0), then no overflow will be
	** allocated and the receives will be done using the 2-step stream
	** I/O method rather than the 1-step stream I/O method.  This variable
	** has no impact on block message protocols.
	** The variable was originally added to compare performance between
	** the original 2-step approach and the 1-step approach and could be
	** removed if one approach is found to be universally better.
	*/
	NMgtAt("II_WINSOCK2_RECV_BUFFER_SIZE", &ptr);
	if (!(ptr && *ptr) && PMget("!.winsock2_recv_buffer_size", &ptr) != OK)
	{
	    GCWINSOCK2_recv_buffer_size = RCV_PCB_BUFF_SZ;
	}
	else
	{
	    GCWINSOCK2_recv_buffer_size = (u_i2)(atoi(ptr));
	}
	GCTRACE(1)("GCwinsock2_init %s: RECV Overflow Buffer size=%d ... use %s logic\n", 
		    proto, GCWINSOCK2_recv_buffer_size,
		    GCWINSOCK2_recv_buffer_size ? "'1-step read all'" : "'2-step read length prefix first'");

	/*
	** Get # concurrent and worker threads variables to control
	** use of IO completion port or Alertable IO (AIO).  
	** These settings may also be useful for performance tuning
	** in situations where it is desired to limit processing
	** to less than the total # of CPUs on the machine.
	**
	** If concurrent threads is set to 0, then AIO will be used instead
	** of IO completion port. Sends and Receives for AIO run completely
	** in the caller's thread whereas, for IO completion port, their
	** completion is handled by the worker threads.
	**
	** If not set, then the default # of concurrent threads will be
	** created (=#CPUs) and IO completion port used.
	**
	** If not set, then the default # of worker threads will be created
	** (# concurrent + 2) + (#listen ports - 1)) [eg, on a dual-core
	** machine with both IPv6 and IPv4 ports ==> 5 worker threads].
	** If set to other than zero, then that number of worker threads
	** will be created (plus possibly a thread(s) for listen ports).
	*/
	GetSystemInfo(&SystemInfo);
	NMgtAt("II_WINSOCK2_CONCURRENT_THREADS", &ptr);
	if (!(ptr && *ptr) && PMget("!.winsock2_concurrent_threads", &ptr) != OK)
	{
	    NumConcurrentThreads =
		SystemInfo.dwNumberOfProcessors > MAX_WORKER_THREADS ?
		    MAX_WORKER_THREADS : SystemInfo.dwNumberOfProcessors;
	}
	else
	{
	    NumConcurrentThreads = atoi(ptr);
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
*/

STATUS
GCwinsock2(i4 function_code, GCC_P_PLIST *parm_list)
{
    STATUS		status = OK;
    THREAD_EVENTS	*Tptr = NULL;
    GCC_LSN		*lsn = NULL;
    GCC_WINSOCK_DRIVER	*wsd;
    PCB			*pcb = (PCB *)parm_list->pcb;
    char		*port = NULL;
    char		*node = NULL;
    int			i, rval;
    DWORD		dwBytes=0;
    DWORD		dwBytes_wanted;
    i4			len;
    char		*proto = parm_list->pce->pce_pid;
    WS_DRIVER		*driver;
    char		port_name[36];
    PER_IO_DATA  	*lpPerIoData = NULL;
    REQUEST_Q		*rq;
    struct addrinfo	hints, *lpAddrinfo;
    int 		tid;
    SECURITY_ATTRIBUTES sa;

    parm_list->generic_status = OK;
    CLEAR_ERR(&parm_list->system_status);

    /*
    ** Get entry for driver in Winsock protocol driver table 
    ** Tptr=THREAD_EVENTS=dcb (driver ctl block) and embedded struct wsd.
    */
    Tptr = (THREAD_EVENTS *)parm_list->pce->pce_dcb;
    wsd = &Tptr->wsd;

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
	        status = FAIL;
		GCTRACE(1)("GCwinsock2 %s: GCC_LISTEN %p No matching GCC_LSN cb found for port %s\n", 
			proto, parm_list, parm_list->pce->pce_port);
	        goto complete;
	    }

	    if (lpfnAcceptEx)
		return GCwinsock2_listen(parm_list);
	    else
	    {
		if (lsn->hListenThread =
			(HANDLE)_beginthreadex(&sa, GC_STACK_SIZE,
				     (LPTHREAD_START_ROUTINE)GCwinsock2_listen,
				     parm_list, 0, &tid))
		{
		    return(OK);
		}

		status = errno;
		SETWIN32ERR(&parm_list->system_status, status, ER_threadcreate);
		GCTRACE(1)("GCwinsock2 %s: GCC_LISTEN %p Unable to create listen thread, status=%d\n", 
			proto, parm_list, status);
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
	    ** Allocate the protocol control block.
	    */
	    pcb = (PCB *) MEreqmem(0, sizeof(PCB) + GCWINSOCK2_recv_buffer_size, FALSE, &status);
	    parm_list->pcb = (char *)pcb;
	    if (pcb == NULL)
	    {
		SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
		parm_list->generic_status = GC_CONNECT_FAIL;
        	GCTRACE(1)("GCwinsock2 %s: GCC_CONNECT %p Unable to allocate PCB (size=%d)\n",
                       proto, parm_list, sizeof(PCB));
		break;
	    }
	    ZeroMemory(pcb, sizeof(PCB));  /* Don't bother to zero buffer */
	    pcb->rcv_buf_len = sizeof(pcb->rcv_buffer) + GCWINSOCK2_recv_buffer_size;
	    pcb->b1 = pcb->b2 = pcb->rcv_buffer;

	    /*
	    ** Call the protocol-specific addressing function.
	    ** If an Ingres symbolic port was input, this routine
	    ** will convert it to a numeric port that can be used
	    ** in the network function calls.
	    */ 
	    if ((*driver->addr_func)(node, port, port_name) == FAIL)
	    {
		status = WSAGetLastError();
		SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		parm_list->generic_status = GC_CONNECT_FAIL;
        	GCTRACE(1)("GCwinsock2 %s: GCC_CONNECT %p Protocol-specific address function failed for node=%s port=%s, status=%d\n",
                       proto, parm_list, node, port, status);
		break;
	    }

	    GCTRACE(2)( "GCwinsock2 %s: GCC_CONNECT %p connecting to %s port %s(%s)\n", 
		proto, parm_list, node, port, port_name );

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
	        status = WSAGetLastError();
	        SETWIN32ERR(&parm_list->system_status, status, ER_getaddrinfo);
	        parm_list->generic_status = GC_CONNECT_FAIL;
	        GCTRACE(1)( "GCwinsock2 %s: GCC_CONNECT %p getaddrinfo() failed for node=%s port=%s(%s), status=%d\n",
			    proto, parm_list, node, port, port_name, status);
	        break;
	    }

	    /*
	    ** Ensure at least one address returned.
	    */
	    if (pcb->addrinfo_list == NULL)
	    {
	        SETWIN32ERR(&parm_list->system_status, status, ER_getaddrinfo);
	        parm_list->generic_status = GC_CONNECT_FAIL;
	        status = FAIL;
	        GCTRACE(1)( "GCwinsock2 %s: GCC_CONNECT %p getaddrinfo() returned no addresses for node=%s port=%s(%s)\n",
			    proto, parm_list, node, port, port_name);
	        break;
	    }

	    if (lpfnConnectEx)  /* If using Compl Port or alertable IO */
	    {
		/*
		** Allocate the PER_IO_DATA structure and initialize.
		*/
		lpPerIoData = (PER_IO_DATA *)MEreqmem(0, sizeof(PER_IO_DATA),
		  				      TRUE, &status);
		if (lpPerIoData == NULL)
		{
		    SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
		    parm_list->generic_status = GC_CONNECT_FAIL;
		    GCTRACE(1)("GCwinsock2 %s: GCC_CONNECT %p Unable to allocate PER_IO_DATA (size=%d)\n",
			    proto, parm_list, sizeof(PER_IO_DATA));
		    break;
		}

		lpPerIoData->OperationType = OP_CONNECT;
		lpPerIoData->parm_list = parm_list;
		lpPerIoData->block_mode = wsd->block_mode;

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
		GCTRACE(1)("GCwinsock2 %s: GCC_CONNECT %p All IP addrs failed to connect to %s port %s(%s)\n",
			proto, parm_list, node, port, port_name );
		goto complete;
	    }

	case GCC_SEND:
	    GCTRACE(2) ("GCwinsock2 %s: GCC_SEND %p\n", proto, parm_list);

	    if (!pcb) goto complete;

	    if (wsd->block_mode)
	    {
		pcb->snd_bptr = parm_list->buffer_ptr;
		pcb->tot_snd = parm_list->buffer_lng;
	    }
	    else
	    {
		u_i2	tmp_i2;
		u_char	*cptr;

		/*
		** Handle 1st 2 bytes which are a msg length field.
		*/
		pcb->snd_bptr = parm_list->buffer_ptr - 2;
		tmp_i2 = (u_i2)(pcb->tot_snd = (i4)(parm_list->buffer_lng + 2));
		cptr = (u_char *)&tmp_i2;
		pcb->snd_bptr[1] = cptr[1];
		pcb->snd_bptr[0] = cptr[0];
	    }

	    /*
	    ** Allocate the PER_IO_DATA structure and initialize.
	    */
	    lpPerIoData = (PER_IO_DATA *)MEreqmem(0, sizeof(PER_IO_DATA),
						  TRUE, &status);
	    if (lpPerIoData == NULL)
	    {
		SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
		parm_list->generic_status = GC_SEND_FAIL;
		GCTRACE(1)("GCwinsock2 %s: GCC_SEND %p Unable to allocate PER_IO_DATA (size=%d)\n",
			    proto, parm_list, sizeof(PER_IO_DATA));
		break;
	    }

	    lpPerIoData->OperationType = OP_SEND;
	    lpPerIoData->parm_list = parm_list;
	    lpPerIoData->block_mode = wsd->block_mode;
	    lpPerIoData->wbuf.buf = pcb->snd_bptr;
	    lpPerIoData->wbuf.len = pcb->tot_snd;

	    InterlockedIncrement(&pcb->AsyncIoRunning);
	    rval = WSASend( pcb->sd, &lpPerIoData->wbuf, 1, &dwBytes, 0,
			&lpPerIoData->Overlapped,
			GCwinsock2CompletionPort ? NULL : GCwinsock2_AIO_Callback );

	    if (rval == SOCKET_ERROR)
	    {
		status = WSAGetLastError();
		if (status != WSA_IO_PENDING)
		{
		    InterlockedDecrement(&pcb->AsyncIoRunning);
		    parm_list->generic_status = GC_SEND_FAIL;
		    SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		    GCTRACE(1)( "GCwinsock2 %s: GCC_SEND %p WSASend() failed for %d bytes, socket=0x%p, status=%d\n",
			     proto, parm_list, pcb->tot_snd, pcb->sd, status);
		    goto complete;
		}
	    }

	    GCTRACE(2)( "GCwinsock2 %s: GCC_SEND %p, try send %d bytes, did send %d bytes, status=%d\n",
			proto, parm_list, pcb->tot_snd, dwBytes, rval);

	    return(OK);

	case GCC_RECEIVE:
	    GCTRACE(2) ("GCwinsock2 %s: GCC_RECEIVE %p\n", proto, parm_list);

	    if (!pcb) goto complete;

	    if (wsd->block_mode)	/* message (block) read */
	    {
		pcb->rcv_bptr = parm_list->buffer_ptr;
		pcb->tot_rcv = parm_list->buffer_lng;
	    }
	    else if( pcb->rcv_buf_len <= sizeof(pcb->rcv_buffer)  )   /* stream 2-step read */
	    {
		/*
		** No receive buffer for overflow, so will first read the
		** 2 byte header that specifies what the message length is.
		** Then will do a 2nd receive to get the actual message.
		*/
		pcb->rcv_bptr = parm_list->buffer_ptr - 2;
		pcb->tot_rcv = 2;
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
		*/
		pcb->rcv_bptr = parm_list->buffer_ptr - 2;
		pcb->tot_rcv = min( parm_list->buffer_lng + 2, pcb->rcv_buf_len );

		/* Get what we can from the buffer */

		if( len = ( pcb->b2 - pcb->b1 ) )  /* #bytes in buffer */
		{
		    bool flag_skip_recv = FALSE;
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
			    GCTRACE(1)( "GCwinsock2 %s: GCC_RECEIVE %p failure - Invalid incoming message len = %d bytes, buffer = %d bytes, socket=0x%p\n",
		                proto, parm_list,
		                len2 - 2, 
		                parm_list->buffer_lng,
		                pcb->sd);
			    break;
			}

			if (len2 <= len)  /* Have a whole msg? */
			{
			    len = len2;
			    flag_skip_recv = TRUE;
			}
		    }  /* End if have at least 2 bytes in buffer */

		    GCTRACE(3)("GCwinsock2 %s: GCC_RECEIVE %p using %d - %s message\n",
			proto, parm_list, len,
			flag_skip_recv ? "whole" : "part" );

		    MEcopy( pcb->b1, len, pcb->rcv_bptr );
		    pcb->rcv_bptr += len;
		    pcb->tot_rcv -= len;
		    pcb->b1 += len;
		    /*
		    ** If we've copied out everything from the overflow
		    ** buffer, then reset the pointers to the overflow
		    ** buffer back to the beginning (indicating nothing
		    ** in the overflow buffer).
		    ** NOTE: b1 and b2 should be equal in this condition,
		    ** but we check for > also just to be safe.
		    */
		    if (pcb->b1 >= pcb->b2)
			pcb->b1 = pcb->b2 = pcb->rcv_buffer;
		    /*
		    ** Skip the receive if an entire message was
		    ** found in the overflow area from a prior receive.
		    ** Ie, the GCC_RECEIVE is done!
		    */
		    if (flag_skip_recv == TRUE)
		    {
			parm_list->buffer_lng  = len - 2;
			goto complete;
		    }
		}  /* End if have any bytes already in buffer. */
	    }  /* End if non-blocked mode (ie, stream mode) */

	    /*
	    ** Allocate the PER_IO_DATA structure and initialize.
	    */
	    lpPerIoData = (PER_IO_DATA *)MEreqmem(0, sizeof(PER_IO_DATA),
						  TRUE, &status);
	    if (lpPerIoData == NULL)
	    {
		SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
		parm_list->generic_status = GC_RECEIVE_FAIL;
		GCTRACE(1)("GCwinsock2 %s: GCC_RECEIVE %p Unable to allocate PER_IO_DATA (size=%d)\n",
			    proto, parm_list, sizeof(PER_IO_DATA));
		break;
	    }

	    lpPerIoData->OperationType = OP_RECV;
	    lpPerIoData->parm_list = parm_list;
	    lpPerIoData->block_mode = wsd->block_mode;
	    lpPerIoData->wbuf.buf = pcb->rcv_bptr;
	    lpPerIoData->wbuf.len = dwBytes_wanted = pcb->tot_rcv;

	    i = 0;
	    InterlockedIncrement(&pcb->AsyncIoRunning);
	    rval = WSARecv( pcb->sd, &lpPerIoData->wbuf, 1, &dwBytes, &i,
			&lpPerIoData->Overlapped,
			GCwinsock2CompletionPort ? NULL : GCwinsock2_AIO_Callback );

	    if (rval == SOCKET_ERROR)
	    {
		status = WSAGetLastError();
		if (status != WSA_IO_PENDING)
		{
		    InterlockedDecrement(&pcb->AsyncIoRunning);
		    parm_list->generic_status = GC_RECEIVE_FAIL;
		    SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		    GCTRACE(1)( "GCwinsock2 %s: GCC_RECEIVE %p WSARecv() failed for %d bytes, socket=0x%p, status=%d\n",
			     proto, parm_list, pcb->tot_rcv, pcb->sd, status);
		    goto complete;
		}
	    }

	    GCTRACE(4)( "GCwinsock2 %s: GCC_RECEIVE %p, want %d bytes got %d bytes\n",
			proto, parm_list, dwBytes_wanted, dwBytes );

	    return(OK);

	case GCC_DISCONNECT:
	    GCTRACE(2)("GCwinsock2 %s: GCC_DISCONNECT %p\n", proto, parm_list);

	    if (!pcb)
		break;

	    /*
	    ** OK to issue closesocket() here because it returns
	    ** immediately and does a graceful shutdown of the socket
	    ** in background (default behavior of closesocket()).
	    ** Previous code waited to issue it in worker_thread disconnect
	    ** processing which is unnecessary and slower...better to
	    ** get the disconnect started sooner, rather than later.
	    */
	    closesocket(pcb->sd);
	    GCTRACE(2)("GCwinsock2 %s: GCC_DISCONNECT %p Closed socket=0x%p\n",
			proto, parm_list, pcb->sd);

	    if (pcb->AsyncIoRunning > 0)
	    {
		/*
		** Async I/O is pending on this socket.  Must wait for
		** it to complete before calling disconnect completion
		** routine.
		*/

		/*
		** Schedule request to the Disconnect thread
		*/
		if ((rq = (REQUEST_Q *)MEreqmem(0, sizeof(*rq), TRUE, NULL)) == NULL)
		{
		    SETWIN32ERR(&parm_list->system_status, status, ER_close);
		    parm_list->generic_status = GC_DISCONNECT_FAIL;
		    GCTRACE(1)("GCwinsock2 %s: GCC_DISCONNECT %p Unable to allocate memory for rq\n",
			    proto, parm_list);
		    break;
		}

		rq->plist = parm_list;

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
	    	    GCTRACE(1)("GCwinsock2 %s: GCC_DISCONNECT %p PostQueuedCompletionStatus() failed for socket=0x%p, status=%d)\n", 
			    proto, parm_list, pcb->sd, status);
		    break;
		}

		return(OK);
	    } /* End if async I/O pending */

	    /*
	    ** No async I/O pending on socket, so OK to complete disconnect
	    ** processing immediately.
	    */
	    if (pcb->addrinfo_list)
		freeaddrinfo(pcb->addrinfo_list);

	    MEfree((PTR)pcb);

	    parm_list->pcb = NULL;
	    break;

	default:
	    return(FAIL);
    }

complete:

    /*
    ** Drive the completion routine.
    */
    status = GCwinsock2_schedule_completion(NULL, parm_list);

    return(status);
}


/*
**  Name: GCwinsock2_schedule_completion
**  Description:
**	Schedule (drive) the completion routine for the request.
**
**  Inputs:
**	One and only one of the following inputs should be provided.
**	Supply the rq if one already exists, else one will be built
**	from the parm list:
**      rq         	Ptr to request queue entry.
**			If NULL, one will be created.
**      parm_list	Ptr to function parm list.
**			Only required if rq==NULL.
**
**  Outputs: none
**
**  Returns:
**	OK if successful
**	FAIL or Windows status if unsuccessful
**
**  History:
**	26-Nov-2008 (Bruce Lunsford) Bug 121285
**	    Split out into standalone function since code is now needed
**	    in 5 places.
*/
STATUS
GCwinsock2_schedule_completion(REQUEST_Q *rq_in, GCC_P_PLIST *parm_list)
{
    STATUS	status = OK;
    REQUEST_Q	*rq = rq_in;

    GCTRACE(5)("GCwinsock2_schedule_completion Entered: rq=%p, parm_list=%p\n",
		rq, parm_list);
    /*
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
    }

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
    char        	hoststr[NI_MAXHOST], servstr[NI_MAXSERV];
    SECURITY_ATTRIBUTES	sa;

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
	    GCTRACE(1)( "GCwinsock2_open %s: %p addr_func() failed on port_id=%s, status=%d\n",
			proto, parm_list, port_id, status);
	    goto complete_open;
	}
	GCTRACE(1)( "GCwinsock2_open %s: %p addr_func() - port=%s(%s)\n",
			proto, parm_list, port_id, lsn->port_name);

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
	    status = WSAGetLastError();
	    parm_list->generic_status = GC_OPEN_FAIL;
	    SETWIN32ERR(&parm_list->system_status, status, ER_getaddrinfo);
	    GCTRACE(1)( "GCwinsock2_open %s: %p getaddrinfo() failed on port=%s, status=%d\n",
			proto, parm_list, lsn->port_name, status);
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
 	    ** Grab the name, in case we used a port number of 0 and got a
 	    ** random address...
 	    */
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
    STcopy(port_id, port);
    parm_list->function_parms.open.lsn_port = lsn->port_name;
    STcopy(port_id, parm_list->pce->pce_port);
    STcopy(port_id, lsn->pce_port);

complete_open:
    /*
    ** Drive the completion routine.
    */
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
*/

STATUS
GCwinsock2_listen(VOID *parms)
{
    GCC_P_PLIST		*parm_list = (GCC_P_PLIST *)parms;
    char		*proto = parm_list->pce->pce_pid;
    int			i, i_stop;
    SOCKET		sd;
    PCB			*pcb;
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
    ** Allocate Protcol Control Block specific to this driver/port
    */
    if ((pcb = (PCB *)MEreqmem(0, sizeof(PCB) + GCWINSOCK2_recv_buffer_size, FALSE, &status)) == NULL)
    {
	SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
	parm_list->generic_status = GC_LISTEN_FAIL;
	GCTRACE(1)("GCwinsock2_listen %s: %p Unable to allocate PCB (size=%d)\n",
		proto, parm_list, sizeof(PCB));
	goto complete_listen;
    }

    parm_list->pcb = (char *)pcb;  
    ZeroMemory(pcb, sizeof(PCB));  /* Don't bother to zero buffer */
    pcb->rcv_buf_len = sizeof( pcb->rcv_buffer ) + GCWINSOCK2_recv_buffer_size;
    pcb->b1 = pcb->b2 = pcb->rcv_buffer;

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
    ** Allocate the PER_IO_DATA structure and initialize.
    */
    lpPerIoData = (PER_IO_DATA *)MEreqmem(0, sizeof(PER_IO_DATA), TRUE,
					  &status);
    if (lpPerIoData == NULL)
    {
	SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
	parm_list->generic_status = GC_LISTEN_FAIL;
	GCTRACE(1)("GCwinsock2_listen %s: %p ai[%d]=%p Unable to allocate lpPerIoData (size=%d)\n",
		proto, parm_list, i, lpAddrinfo, sizeof(PER_IO_DATA));
	goto complete_listen;
    }

    lpPerIoData->OperationType = OP_ACCEPT;
    lpPerIoData->parm_list = NULL;  /* parmlist may be gone when AcceptEx completes */
    lpPerIoData->block_mode = wsd->block_mode;
    lpPerIoData->listen_sd = lsn->listen_sd[i];
    lpPerIoData->lsn       = lsn;
    lpPerIoData->addrinfo_posted = lpAddrinfo;
    lpPerIoData->socket_ix_posted = i;

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
    status_sc = GCwinsock2_schedule_completion(NULL, parm_list);

    return(status_sc ? status_sc : status);
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
    PCB			*pcb = (PCB *)parm_list->pcb;
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

    GCTRACE(2)( "GCwinsock2_listen_complete %s: %p Connection is %s\n",
		proto, parm_list,
                parm_list->options & GCC_LOCAL_CONNECT ? "local" : "remote" );

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
	GCTRACE(1)("GCwinsock2_listen_complete %s: %p ioctlsocket() set non-blocking failed with status %d, socket=0x%p\n",
		proto, parm_list, retval, pcb->sd);
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
*/
STATUS
GCwinsock2_connect(GCC_P_PLIST *parm_list, char *inPerIoData)
{
    STATUS		status = OK;
    int			i, rval;
    char		*proto = parm_list->pce->pce_pid;
    char		*port = parm_list->function_parms.connect.port_id;
    char		*node = parm_list->function_parms.connect.node_id;
    PCB			*pcb = (PCB *)parm_list->pcb;
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
	GCTRACE(2)( "GCwinsock2_connect %s: %p Closed prior (failed) socket %p\n", 
		proto, parm_list, pcb->sd );
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
	GCTRACE(1)( "GCwinsock2_connect %s: %p WSASocket() failed with status %d. family=%d, socktype=%d, protocol=%d\n", 
		proto, parm_list,
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
	GCTRACE(1)( "GCwinsock2_connect %s: %p ioctlsocket() set non-blocking failed with status %d, socket=0x%p\n", 
		proto, parm_list, status, sd);
	return(FAIL);
    }

    GCTRACE(2)( "GCwinsock2_connect %s: %p connecting to %s(%s) port %s(%d)\n",
	    proto, parm_list, node,
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
	GCTRACE(1)( "GCwinsock2_connect %s: %p CreateIoCompletionPort() failed with status %d, socket=0x%p\n", 
		proto, parm_list, status, sd);
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
	    GCTRACE(1)( "GCwinsock2_connect %s: %p GCwinsock2_add_connect_event() failed with status %d, socket=0x%p\n", 
		    proto, parm_list, status, sd);
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
	GCTRACE(1)("GCwinsock2_connect %s: %p bind() error = %d\n",
		    proto, parm_list, status);
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
	GCTRACE(1)("GCwinsock2_connect %s: %p ConnectEx() error = %d\n",
		    proto, parm_list, status);
	return(FAIL);
    }
    return(OK);

normal_connect:

    rval = connect(sd, lpAddrinfo->ai_addr, lpAddrinfo->ai_addrlen);

    if (rval == SOCKET_ERROR)
    {
	status = WSAGetLastError();
	GCTRACE(2)("GCwinsock2_connect %s: %p connect() SOCKET_ERROR status = %d\n",
		    proto, parm_list, status);
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
	            GCTRACE(1)("GCwinsock2_connect %s: %p select() error = %d\n",
			        proto, parm_list, status);
		    return(FAIL);
		}

		status = OK;
	    }
	    else if (rval == 0)
	    {
		/* Connection timed out */
	        GCTRACE(1)("GCwinsock2_connect %s: %p timeout after %d secs\n",
			    proto, parm_list, tm.tv_sec);
		return(FAIL);
	    }
	}
	else  /* Not WSAEWOULDBLOCK status, real failure to connect */
	{
	    GCTRACE(1)("GCwinsock2_connect %s: %p connect() error = %d\n",
		    proto, parm_list, status);
	    return(FAIL);
	}
    }  /* end if connect returned a SOCKET_ERROR */

    GCTRACE(2)("GCwinsock2_connect %s: %p connect() completed; call ...connect_complete()\n",
	    proto, parm_list);

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
    PCB			*pcb = (PCB *)parm_list->pcb;
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
	GCTRACE(1)("GCwinsock2_connect_complete %s: %p getnameinfo(local) error = %d\n",
		   proto, parm_list, retval);
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
	GCTRACE(1)("GCwinsock2_connect_complete %s: %p getnameinfo(remote) error = %d\n",
		   proto, parm_list, retval);
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
	    PCB *pcb = (PCB *)parm_list->pcb;
	    q = q->q_next;		/* Bump/save next Q entry address */
	    GCTRACE(5)( "GCwinsock2_disconnect_thread %s check disconnect: parm_list=%p, pcb=%p\n",
			 parm_list->pce->pce_pid, 
			 parm_list, pcb );
	    if (pcb)
	    {
		/*
		** If I/O on this socket is still pending,
		** don't schedule disconnect yet, UNLESS we've
		** waited (slept) long enough.
		*/
		GCTRACE(5)( "GCwinsock2_disconnect_thread %s check I/Os complete: parmlist=%p, pcb=%p, Pending I/Os=%d\n",
			     parm_list->pce->pce_pid, 
			     parm_list, pcb,
			     pcb->AsyncIoRunning );
		if (wait_stat == WAIT_TIMEOUT) /* Means we slept above */
		    pcb->DisconnectAsyncIoWaits++;
		if (pcb->AsyncIoRunning > 0)
		{
		    if (pcb->DisconnectAsyncIoWaits <= DISCONNECT_ASYNC_WAITS_MAX_LOOPS )
			continue;  /* Don't disconnect this session yet */
		    else
			GCTRACE(1)( "GCwinsock2_disconnect_thread %s WARNING: disconnect completion scheduled with pending I/Os\n (parmlist%p, pcb=%p, Pending I/Os=%d\n",
			         parm_list->pce->pce_pid, 
			         parm_list, pcb,
			         pcb->AsyncIoRunning );
		}

		/*
		** No IOs pending or max wait/sleep time exceeded...
		** so start disconnecting the session.
		*/
		if (pcb->addrinfo_list)
		    freeaddrinfo(pcb->addrinfo_list);
		MEfree((PTR)pcb);
	    } /* End if pcb exists */

	    /*
	    ** Drive completion routine for disconnect
	    */
	    GCTRACE(5)( "GCwinsock2_disconnect_thread %s schedule completion: parm_list=%p, pcb=%p\n",
			 parm_list->pce->pce_pid, 
			 parm_list, pcb );
	    parm_list->pcb = NULL;
	    QUremove(&rq->req_q);  /* Remove from disconnect Q */
	    status = GCwinsock2_schedule_completion(rq, NULL);

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
**	This function exists from the Winsock 2.2 protocol, cleaning up.
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
*/
VOID
GCwinsock2_exit()
{
    int			i = 1, j;
    GCC_WINSOCK_DRIVER	*wsd;
    THREAD_EVENTS	*Tptr;

    /*
    ** Have we already been called once?
    */
    if (Winsock2_exit_set)
	return;
    Winsock2_exit_set = TRUE;

    /*
    ** Close listen sockets and various associated threads.
    */
    if ((Tptr = GCwinsock2_get_thread(TCPIP_ID)) != NULL)
    {
	GCC_LSN	*lsn;

	wsd = &Tptr->wsd;
	lsn = wsd->lsn;
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

	for (j = 0; j < Tptr->NumWorkerThreads; j++)
	{
	    TerminateThread(Tptr->hWorkerThreads[j], 0);
	    CloseHandle(Tptr->hWorkerThreads[j]);
	}

	if (GCwinsock2CompletionPort) CloseHandle(GCwinsock2CompletionPort);
    }

    /*
    ** Close Disconnect thread.
    */
    if (hDisconnectThread)
    {
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

    WSACleanup();
}

/*
**  Name: GCwinsock2_get_thread
**  Description:
**	Return pointer to appropriate entry in IIGCc_wsproto_tab.
**	This table is indexed the same as IIGCc_proto_threads.
**
**  History:
**	16-oct-2003 (somsa01)
**	    Created.
*/

THREAD_EVENTS *
GCwinsock2_get_thread(char *proto_name)
{
    int i;

    /*
    ** Go thru the the protocol threads event list and find the index
    ** of the winsock thread.  Set the Global Tptr for easy reference
    ** to the event q's for this protocols thread.
    */
    for (i = 0; i < IIGCc_proto_threads.no_threads; i++)
    {
	THREAD_EVENTS *p = &IIGCc_proto_threads.thread[i];

	if (!STcompare(proto_name, p->thread_name))
	    return p;
    }

    /*
    ** Failed
    */
    return NULL;
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
**      19-Nov-2009 (Bruce Lunsford) Bug 122940 + Sir 122679
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
**      19-Nov-2009 (Bruce Lunsford) Bug 122940 + Sir 122679
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
    PCB			*pcb = NULL;
    PER_IO_DATA		*lpPerIoData;
    bool		DecrementIoCount;
    char		*proto = "";
    ULONG_PTR		dummyCompletionKey;

    CompletionPort = (HANDLE)CompletionPortID;

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
	    GCTRACE(1)("GCwinsock2_worker_thread %s: Entered with no Overlapped area and failure status %d\n",
		        proto, status);
	    continue;
	}

	lpPerIoData = CONTAINING_RECORD(lpOverlapped, PER_IO_DATA, Overlapped);
	parm_list = lpPerIoData->parm_list;
        if (parm_list)  /* No parm list OK for LISTEN -> OP_ACCEPT */
	{
	    proto = parm_list->pce->pce_pid;
	    pcb = (PCB *)parm_list->pcb;
	    if (!pcb)
	    {
		GCTRACE(1)("GCwinsock2_worker_thread %s: %p Entered with no PCB\n",
			            proto, parm_list);
	        goto worker_thread_err_exit;
	    }
	}

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
		pcb = (PCB *)parm_list->pcb;
		pcb->sd = lpPerIoData->client_sd;
		lsn->addrinfo_posted  = lpPerIoData->addrinfo_posted;
		lsn->socket_ix_posted = lpPerIoData->socket_ix_posted;

		if (!retval)
		{
		    GCTRACE(1)("GCwinsock2_worker_thread %s: %p OP_ACCEPT Failed with status %d, socket=0x%p\n",
			    proto, parm_list, status, pcb->sd);
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
		    GCTRACE(1)("GCwinsock2_worker_thread %s: %p OP_ACCEPT CreateIoCompletionPort() failed with status %d, socket=0x%p\n",
			    proto, parm_list, status, pcb->sd);
		    break;
		}

		break;

	    case OP_CONNECT:
		DecrementIoCount = FALSE;
		if (!retval)
		{
		    GCTRACE(1)("GCwinsock2_worker_thread %s: %p OP_CONNECT Failed with status %d, socket=0x%p\n",
			    proto, parm_list, status, pcb->sd);
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
		        GCTRACE(1)("GCwinsock2_worker_thread %s: %p OP_CONNECT All IP addrs failed\n",
			    proto, parm_list);
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
		    GCTRACE(1)("GCwinsock2_worker_thread %s: %p OP_SEND Failed with status %d, socket=0x%p\n",
			    proto, parm_list, status, pcb->sd);
		    if (status == ERROR_OPERATION_ABORTED)
			goto worker_thread_err_exit;
		    parm_list->generic_status = GC_SEND_FAIL;
		    SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		    break;
		}

		if (!BytesTransferred)
		{
		    /* The session has been disconnected */
		    pcb->tot_snd = 0;
		    parm_list->generic_status = GC_SEND_FAIL;
		    GCTRACE(1)("GCwinsock2_worker_thread %s: %p OP_SEND Failed- 0 bytes trfrd indicating session disconnected, socket=0x%p\n",
			    proto, parm_list, pcb->sd);
		    break;
		}

		break;

	    case OP_RECV:
		if ( (GCwinsock2_OP_RECV_complete((DWORD)!retval, status, BytesTransferred, lpPerIoData)) == FALSE )
		    continue;  /* another WSARecv() was issued...not done yet */
		if (status == ERROR_OPERATION_ABORTED)
		    goto worker_thread_err_exit;
		break;

	    default:
		continue;
	} /* End switch on OperationType */

	/*
	** Drive the completion routine.
	*/
	status = GCwinsock2_schedule_completion(NULL, parm_list);

worker_thread_err_exit:
	if (DecrementIoCount)
	    InterlockedDecrement(&pcb->AsyncIoRunning);
	MEfree((PTR)lpPerIoData);
    } /* End while TRUE */
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
*/

VOID CALLBACK
GCwinsock2_AIO_Callback(DWORD dwError, DWORD BytesTransferred, WSAOVERLAPPED *lpOverlapped, DWORD dwFlags)
{
    PER_IO_DATA		*lpPerIoData;
    GCC_P_PLIST		*parm_list;
    char		*proto = "";
    PCB			*pcb;
    STATUS		status = 0;

    lpPerIoData = CONTAINING_RECORD(lpOverlapped, PER_IO_DATA, Overlapped);
    parm_list = lpPerIoData->parm_list;
    proto = parm_list->pce->pce_pid;
    pcb = (PCB *)parm_list->pcb;

    if (dwError != OK)
	status = WSAGetLastError();
    GCTRACE(5)("GCwinsock2_AIO_Callback %s: %p Entered with error/status=%d/%d for optype=%d on socket=0x%p\n",
        proto, parm_list, dwError, status, lpPerIoData->OperationType, pcb->sd);

    switch(lpPerIoData->OperationType)
    {
	case OP_SEND:
	    if (dwError != OK)
	    {
		GCTRACE(1)("GCwinsock2_AIO_Callback %s: %p OP_SEND Failed with status %d, socket=0x%p\n",
			    proto, parm_list, status, pcb->sd);
		if (status == ERROR_OPERATION_ABORTED)
		    goto AIO_Callback_err_exit;
		parm_list->generic_status = GC_SEND_FAIL;
		SETWIN32ERR(&parm_list->system_status, status, ER_socket);
		break;
	    }

	    if (!BytesTransferred)
	    {
		/* The session has been disconnected */
		pcb->tot_snd = 0;
		parm_list->generic_status = GC_SEND_FAIL;
		GCTRACE(1)("GCwinsock2_AIO_Callback %s: %p OP_SEND Failed- 0 bytes trfrd indicating session disconnected, socket=0x%p\n",
			    proto, parm_list, pcb->sd);
	    }

	    break;

	case OP_RECV:
	    if ( (GCwinsock2_OP_RECV_complete(dwError, status, BytesTransferred, lpPerIoData)) == FALSE )
		return;  /* another WSARecv() was issued...not done yet */
	    if (status == ERROR_OPERATION_ABORTED)
		goto AIO_Callback_err_exit;
	    break;

	default:
	    return;
    }

    /*
    ** Drive the completion routine.
    */
    /* status = GCwinsock2_schedule_completion(NULL, parm_list); */
    (*parm_list->compl_exit)( parm_list->compl_id );

AIO_Callback_err_exit:
    InterlockedDecrement(&pcb->AsyncIoRunning);
    MEfree((PTR)lpPerIoData);
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
** Inputs:
**      dwError         Error indicator received by callback mechanism
**			for WSARecv.
**      status          Error code from WSAGetLastError() after WSARecv.
**      BytesTransferred  Number of bytes received into target buffer.
**	lpPerIoData	Control block associated with the I/O - contains
**			the Overlapped area as well as data specific to
**			this protocol.
**
** Outputs:		None
**
** Returns:
**                      TRUE  if receive is completed.  Ie, either:
**			  - error in receive, or
**			  - entire message received
**                      FALSE if receive is incomplete. Ie, another
**			  receive issued...for rest of the message.
**
**  History:
**	06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Created.
*/
bool 
GCwinsock2_OP_RECV_complete(DWORD dwError, STATUS status, DWORD BytesTransferred, PER_IO_DATA *lpPerIoData)
{
    GCC_P_PLIST		*parm_list;
    char		*proto;
    PCB			*pcb;
    int			i;
    DWORD		dwBytes;
    DWORD		dwBytes_wanted;
    i4			len;

    parm_list = lpPerIoData->parm_list;
    proto = parm_list->pce->pce_pid;
    pcb = (PCB *)parm_list->pcb;

    if (dwError != OK)
    {
	GCTRACE(1)("GCwinsock2_OP_RECV_complete %s: %p Failed with status %d, socket=0x%p\n",
		    proto, parm_list, status, pcb->sd);
	if (status == ERROR_OPERATION_ABORTED)
	    return TRUE; /* GCC_RECEIVE is done */
	parm_list->generic_status = GC_RECEIVE_FAIL;
	SETWIN32ERR(&parm_list->system_status, status, ER_socket);
	return TRUE; /* GCC_RECEIVE is done */
    }

    GCTRACE(2)( "GCwinsock2_OP_RECV_complete %s: %p Want %d bytes got %d bytes\n",
		proto, parm_list, pcb->tot_rcv, BytesTransferred );

    if (!BytesTransferred)
    {
	/* The session has been disconnected */
	pcb->tot_rcv = 0;
	parm_list->generic_status = GC_RECEIVE_FAIL;
	GCTRACE(2)("GCwinsock2_OP_RECV_complete %s: %p Failed- 0 bytes trfrd indicating session disconnected, socket=0x%p\n",
		    proto, parm_list, pcb->sd);
	return TRUE; /* GCC_RECEIVE is done */
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

    pcb->tot_rcv -= BytesTransferred;
    pcb->rcv_bptr += BytesTransferred;

    if (pcb->rcv_bptr < parm_list->buffer_ptr)
    {
	/*
	** Haven't received the all of the 2-byte length header yet.
	** We need to issue another recv to get the rest of the
	** message (or at least the header).
	*/

	ZeroMemory(&lpPerIoData->Overlapped, sizeof(OVERLAPPED));
	lpPerIoData->OperationType = OP_RECV;
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
		GCTRACE(1)("GCwinsock2_OP_RECV_complete %s: %p WSARecv() failed with status %d, socket=0x%p\n",
		        proto, parm_list, status, pcb->sd);
		return TRUE; /* GCC_RECEIVE is done */
	    }
	}
	GCTRACE(4)( "GCwinsock2_OP_RECV_complete %s: %p want %d bytes got %d bytes\n",
		    proto, parm_list, dwBytes_wanted, dwBytes );

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
	GCTRACE(1)( "GCwinsock2_OP_RECV_complete %s: %p failure - Invalid incoming message len = %d bytes, buffer = %d bytes, socket=0x%p\n",
	            proto, parm_list,
	            len, 
	            parm_list->buffer_lng,
	            pcb->sd);
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
	lpPerIoData->OperationType = OP_RECV;
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
		GCTRACE(1)("GCwinsock2_OP_RECV_complete %s: %p WSARecv() #2 failed with status %d, socket=0x%p\n",
		        proto, parm_list, status, pcb->sd);
		return TRUE; /* GCC_RECEIVE is done */
	    }
	}
	GCTRACE(4)( "GCwinsock2_OP_RECV_complete %s: %p Want %d bytes got %d, msg len %d remaining %d\n",
		proto, parm_list, dwBytes_wanted, dwBytes,
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
	GCTRACE(3)("GCwinsock2_OP_RECV_complete %s: %p saving %d\n",
	    proto, parm_list, len );
	MEcopy( pcb->rcv_bptr - len, len, pcb->rcv_buffer );
	pcb->b1 = pcb->rcv_buffer;
	pcb->b2 = pcb->rcv_buffer + len;
    }

    return TRUE; /* GCC_RECEIVE is done */
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
*/
VOID
GCwinsock2_close_listen_sockets(GCC_LSN *lsn)
{
    int 	i;
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
** Inputs:
**      hostlen         Byte length of host name buffer.
**      ourhost         Allocated buffer at least "hostlen" in size.
**
** Outputs:
**      ourhost         Fully qualified host name - null terminated string.
**
** Returns:
**                      OK
**                      GC_NO_HOST
**                      GC_HOST_TRUNCATED
**                      GC_INVALID_ARGS
** Exceptions:
**                      N/A
**
**  History:
**    03-Aug-2007 (Ralph Loen) SIR 118898
**      Created.
**    17-Dec-2009 (Bruce Lunsford) Sir 122536
**      Add support for long identifiers. Use cl-defined GC_HOSTNAME_MAX
**      (=256 on Windows) instead of gl-defined GCC_L_NODE (=64).
*/

STATUS GCdns_hostname( char *ourhost, i4 hostlen )
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

    if (!WSAStartup_called)
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
STATUS GCwinsock2_startup_WSA()
{
    WORD	version;
    int	err;

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
    
    WSAStartup_called = TRUE;
    return (OK);
}
