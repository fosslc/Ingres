/*
** Name: GCCLOCAL.H
** Description:
**	Contains struct typedefs and #defines used locally for GC CL functions
**	needed by the Comm Server.
**
**	See description in gcevnt.c for use of these structures.
** History:
**	03-Nov-93 (edg)
**	    Created for Windows NT.
**	19-Nov-93 (edg)
**	    Added PCB_STATE structure.  Should be included as first member
**	    in a protocol driver's PCB if that protocol driver wishes to
**	    use some of the utility routines.
**	02-dec-93 (edg)
**	    Winsock driver restructuring -- put the event and mutex handles
**	    for the thread's Q's into THREAD_EVENTS.
**	05-dec-93 (edg)
**	    Added GCC_WINSOCK_DRIVER structure.  This structure is now also
**	    a member of THREAD_EVENTS.
**	10-jan-94 (edg)
**	    Added gcc cl internal func refernces and global refs.
**	23-apr-96 (tutto01)
**	    Added definitions and prototypes for DECnet.
**	09-may-1996 (canor01)
**	    Added definitions for GCwinsock_exit() and GClanman_exit().
**	24-jun-1997 (canor01)
**	    Add handle for async i/o thread to THREAD_EVENTS structure.
**	01-jul-97 (somsa01)
**	    Moved DECnet stuff into decnet.c . DECnet is now contained in its
**	    own DLL, and it is loaded in at ingstart time.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      03-Apr-2002 (lunbr01) Bug 106589
**          Add pcbInternal_listen & _conn to THREAD_EVENTS for internal socket.
**	01-oct-2003 (somsa01)
**	    Added TCP_IP (Winsock 2.2) functions.
**	20-feb-2004 (somsa01)
**	    Separated listen specific stuff from GCC_WINSOCK_DRIVER into
**	    a new structure, GCC_LSN. This is because, per protocol, there
**	    could be multiple listen ports.
**	28-Aug-2006 (lunbr01) Sir 116548
**	    Add IPv6 support.  Update GCC_LSN structure to add hListenEvent.
**          Change single listen address and socket to array ptrs for
**          Winsock2 ONLY and misc other Winsock2 ONLY fields.
*/

/*
*/
typedef struct _GCC_LSN
{
#ifdef WINSOCK_VERSION2
    struct addrinfo	*addrinfo_list;		/* ptr to addrinfo for listen */
    SOCKET		*listen_sd;      	/* ptr to listen sockets */
    int  		num_sockets;		/* # addresses(1 socket each) */
    int  		num_good_sockets;	/* # good sockets (listen OK) */
    HANDLE		hListenEvent;		/* handle for Listen Event */
    GCC_P_PLIST    	*parm_list;		/* Listen request parm list */
    struct addrinfo	*addrinfo_posted;  	/* ptr to last posted lsn addr*/
    int 		socket_ix_posted;     	/* socket index last posted */
#else
    struct sockaddr	*ws_addr;		/* ptr to sockaddr for listen */
    SOCKET		listen_sd;      	/* listen socket descriptor */
#endif
    char		pce_port[GCC_L_PORT];	/* untranslated port id */
    char		port_name[36];  	/* ascii listen port id */
    HANDLE		hListenThread;		/* handle for Listen thread */
    struct _GCC_LSN	*lsn_next;		/* ptr to next listen block */
} GCC_LSN;

/*
** Name: GCC_WINSOCK_DRIVER
** Description:
**	This structue is used
**	by all protocol drivers that use the winsock API.  The general
**	winsock driver is in gcwinsck.c.  This structure holds additional
**	info that's necessary for a particular driver's use of the
**	winsock API and also contains some info for thread control.
**
** History:
**	02-dec-93 (edg)
**	    original.
**	20-feb-2004 (somsa01)
**	    Updated to use GCC_LSN structure.
*/
typedef struct _GCC_WINSOCK_DRIVER
{
	i4		addr_len;	/* length of sockaddr struct */    
	int		sock_fam;	/* address family of socket */
	int		sock_type;	/* socket type: STREAM, SEQPACKET..*/
	int		sock_proto;	/* protocol for socket */
	bool		block_mode;	/* block or byte stream type */
	PTR		pce_driver;
	GCC_LSN		*lsn;		/* Listen block, could be a list */
} GCC_WINSOCK_DRIVER;

/*
** Name: WS_DRIVER
** Description:
**	Structure containing function pointers to some protocol specific
**	functions used by winsock driver.
*/
typedef struct _WS_DRIVER
{
	STATUS	(*init_func)();
	STATUS	(*addr_func)();
} WS_DRIVER;



/*
** struct THREAD_EVENTS
** Description:
**	Structure which holds event Qs for a single thread (protcol driver).
**	Each thread has an incoming event q and an in-process q.
**
**	Access to incoming queue must be MUTEX'd.
*/
#define	MAX_WORKER_THREADS	256

typedef struct _THREAD_EVENTS
{
    char	*thread_name;	/* essantially protocol name */
    QUEUE	incoming_head;	/* head of incoming requests */
    QUEUE	process_head;	/* head of requests being processed */
    HANDLE	hMutexThreadInQ;/* Mutex handle to lock incoming Q */
    HANDLE	hEventThreadInQ;/* Event to signal asynchronous thread */
    GCC_WINSOCK_DRIVER wsd;	/* control info for any winsock protocol */
    HANDLE      hAIOThread;     /* handle for Async IO thread */
    PTR         pcbInternal_listen;  /* PCB for internal socket - listen */
    PTR         pcbInternal_conn;    /* PCB for internal socket - connect */
    HANDLE	hEventInternal_conn;/* Event to signal internal socket complete*/
    i4		NumWorkerThreads;	/* Number of worker threads */
    HANDLE	hWorkerThreads[MAX_WORKER_THREADS];  /* Worker thread handles */
} THREAD_EVENTS;


/*
** struct THREAD_EVENT_LIST
** Description:
**	This is the control/communication structure between the primary
**	GCC thread (runs GCexec and all callbacks into GCC mainline
**	code) and protocol driver threads (each protocol driver will
**	have up to 2 threads running: 1 synchronous listen thread, and
**	1 thread that processes all other requests in its in-processe
**	event Q asynchronously -- only the 1 async thread needs an
**	event Q in this structure).
**
**	Access to completed queue must be MUTEX'd.
*/
typedef struct _THREAD_EVENT_LIST
{
    i4 		  no_threads;
    THREAD_EVENTS *thread;		/* alloc'd list for each thread */
    QUEUE	  completed_head;	/* head of completed req q */
} THREAD_EVENT_LIST;


/*
** struct REQUEST_Q
** Description:
**	Structure for Q'd protocol driver requests that will go into either
**	the incoming or processing q's for a thread, or into the completed
**	q for processing of completions by the primary thread (running GCexec)
*/
typedef struct _REQUEST_Q
{
    QUEUE	req_q;
    GCC_P_PLIST *plist;
} REQUEST_Q;

# define WINTCP_ID	"WINTCP"
# define TCPIP_ID	"TCP_IP"
# define LANMAN_ID	"LANMAN"
# define SPX_ID		"NVLSPX"
# define DECNET_ID	"DECNET"

/*
** struct PCB_STATE
** Description:
**	Contains state information for asynchronous protocol threads for
**	CONNECT, SEND, RECEIVE and DISCONNECT requests.
*/
typedef struct _PCB_STATE
{
    int		conn;
    int		send;
    int		recv;
    int		disc;
} PCB_STATE;

/*
** Defines for states
*/
# define	INITIAL		0
# define	RECV_MSG_LEN	1
# define	COMPLETING	2
# define	COMPLETED	3

/*
** External function declarations
*/

FUNC_EXTERN	STATUS	GCwinsock();
FUNC_EXTERN	STATUS	GCwinsock_init();
FUNC_EXTERN	VOID	GCwinsock_exit();
FUNC_EXTERN	STATUS	GCwinsock2();
FUNC_EXTERN	STATUS	GCwinsock2_init();
FUNC_EXTERN	VOID	GCwinsock2_exit();
FUNC_EXTERN	STATUS	GCnvlspx_init();
FUNC_EXTERN	STATUS	GCwintcp_init();
FUNC_EXTERN	STATUS	GCtcpip_init();

FUNC_EXTERN	STATUS	GClanman();
FUNC_EXTERN	STATUS	GClanman_init();
FUNC_EXTERN	VOID	GClanman_exit();

FUNC_EXTERN	int GCget_incoming_reqs( THREAD_EVENTS *, HANDLE );
FUNC_EXTERN	VOID GCcomplete_request( QUEUE * );

GLOBALREF	WS_DRIVER WS_wintcp;
GLOBALREF	WS_DRIVER WS_tcpip;
GLOBALREF	WS_DRIVER WS_nvlspx;

GLOBALREF	THREAD_EVENT_LIST IIGCc_proto_threads;
