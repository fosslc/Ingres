/*
** Copyright (c) 1995, 2010 Ingres Corporation
**
** Name: GCLOCAL.H
**
** Description:
**      Contains defines and/or variables that must be shared between the
**      GCA CL and GCC CL.
**
** History:
**      17-jun-95 (emmag)
**         Rewrite an asynchronous GC.
**      20-jul-95 (chech02)
**         Added security level.
**      04-Aug-95 (fanra01)
**         Bug #70280 #70304
**         Added a define for pipe I/O failure.  This value is used to indicate
**         that the association has failed and that the higher levels should
**         clean up resources.  The value is taken from UNIX BS_READ_ERR,
**         ( E_CL_MASK | E_BS_MASK | 0x07 ).
**      11-nov-95 (chech02)
**         Added support for Windows 95.
**	13-dec-1995 (canor01)
**	   Add IsPeer to ASSOC_IPC structure to note a peer connection.
**      04-Jan-96 (fanra01)
**         Changed reference to GCevent_func to standard not imported.  This
**         variable is only used in the GC.
**      08-Jan-96 (fanra01)
**         Changed GCevent_func to GLOBALREF as now imported from DLL.
**	02-feb-1996 (canor01)
**	   Add SyncEvent to hAsyncEvents structure.
**	30-jul-96 (fanra01)
**	   Moved the SL_LABEL to the end of the ASSOC_IPC structure
**	   for OpenROAD compatability.
**      05-dec-1996 (canor01)
**         Add padding at end of SL_LABEL in ASSOC_IPC structure.  SL_LABEL
**         can vary in size, but GCC code may use a fixed length.  On NT
**         that may cause a reference beyond end of segment.     
**	01-Apr-1998 (rajus01)
**	   Removed IsPeer. Added assoc_id.
**	26-Jul-1998 (rajus01)
**	   Added new TIMEOUT event for GClisten() requests on NT. ASYNC_IOCB
**	   now carries the information per association, uses flags to determine
**	   the state of the association.
**	07-nov-1998 (canor01)
**	   Add timeout structures for asynchronous receives that time out.
**      10-Dec-98 (lunbr01) Bug 94183
**          Eliminate use of *Ct fields (using PeekNamedPipe instead).
**	08-feb-1999 (somsa01 for lunbr01)
**	    Add ClientReadHandle to ASSOC_IPC structure for purging
**	    on Win95. Integrated from mcgem01 OI Desktop chg.
**	19-apr-1999 (somsa01)
**	    Added GC_IO_CREATED, set when the pipes are created.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      14-Sep-1999 (fanra01)
**          Add a thread identifier to the ASYNC_IOCB for listen completion.
**      01-Oct-1999 (fanra01)
**          Add flags, asyncIoPending counter, asyncIoComplete event handle
**          fields for handling disconnects.
**          flags field replaces listen_side and incorporates a disconn flag.
**      03-Dec-1999 (fanra01)
**          Bug 99392
**          Change the asyncIoPending counter from an unsigned type to signed
**          type.
**          Add an extra flag for net completions.
**          Add a prototype for GCasyncDisconn.
**      23-Oct-2001 (lunbr01) Bug 105793 & 106119
**          Replace assoc_id (actually was sem_id) with hPurgeSemaphore.
**      01-Jul-2005 (lunbr01) Bug 109115
**          Add flag to indicate that a connection has been saved (for
**          use by another program or process).
**      26-Aug-2005 (lunbr01) Bug 115107 + 109115
**          Add disconnect thread id handle and svc_parms to ASSOC_IPC.  
**          Remove asyncIoComplete field and GC_IPC_SAVED_CONN mask as they
**          are no longer needed.
**	29-Apr-2009 (Bruce Lunsford) Bug 122009
**	    Add hRequestThread to ASSOC_IPC for async Connect (GCrequest)
**	    processing.  Remove 2nd parm from GCconnectToStandardPipes().
**	    Remove GClistenToStandardPipes() as it was removed from gcacl.c.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers in new peer info (CONNECT)
**	    structure.
**	14-Jan-2010 (Bruce Lunsford) Sir 122536 addendum
**	    Change max message size (see buf[]) from 512 to 1024 to agree
**	    with Unix and original intent for Windows.  512 is not quite
**	    large enough for worst case maximum length user and host names.
**	    Add ClientSysUserName to ASSOC_IPC; for named pipes, this is the
**	    pipe owner.
*/

#include <cs.h>
#include <gl.h>
#include <sl.h>

/*
** Defines for Asynchronous Events.
*/

#define NUM_EVENTS      5

#define LISTEN          0
#define SHUTDOWN        1
#define GCC_COMPLETE    2
#define SYNC_EVENT      3
#define TIMEOUT     	4	/* Timeout event */

#define MAXPIPENAME     80

/*
** Defines for WIN32 API call failures
*/

#define NP_IO_FAIL      0x1FE07

/*
** Global references
*/
GLOBALREF VOID (*GCevent_func[])();

GLOBALREF HANDLE hCompletion;
GLOBALREF HANDLE hMutexGccCompleteQ;
GLOBALREF HANDLE hAsyncEvents[];

/*
** The SERVER_ENTRY structure is used to keep track of multiple servers by
** name. It contains information that must be maintained on a per server
** basis. The lpMapping and hMapping are the pointer and handle to the 
** memory mapped file that the IPC functions use for sharing data. The
** RequestingEvent fields is used for the client to  signal a server that 
** it is requesting an association. The ListeningEvent is signaled when
** the server is ready to create a new association.
**
** This structure may not be required for some implementations: for example,
** an implementation using named pipes would not require this structure. 
*/

typedef struct _SERVER_ENTRY
{
	char				*name;
	i4 					nConnections;
	LPVOID				lpMapping;
	HANDLE				hMapping;
	HANDLE				RequestingEvent;
	HANDLE				ListeningEvent;
	struct _SERVER_ENTRY	  *next;
} SERVER_ENTRY;

/* 
** The ASSOC_INFO structures contains per association information that must
** be shared between processes. 
** 
** This structure also might not be necessary in other implemetations.
*/

typedef struct ASSOC_INFO_ 
{
	short		refs;				// Reference count to determine if available
	BOOL		bPipesClosed;
	DWORD		out_buffer_size;
	DWORD		in_buffer_size;
} ASSOC_INFO;

/*
** The IPC_INT structure contains internal information that the IPC functions
** require in the ASSOC_IPC. This information is only used by the IPC functions
** not the gcacl.
*/

typedef struct IPC_INT_
{
	DWORD				AscID;	
	SERVER_ENTRY		*server;			// Pointer to server entry for assoc.
	ASSOC_INFO			*AssocInfo;
	HANDLE				hMapping;			// required for save and restore.

} IPC_INT;

/******************************************************************************
** Name: CONNECT - Local IPC peer info
**
** Description:
**	This structure contains information exchanged during connection
**	establishment.
**
** History:
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Added version 2 and GC_RQ_ASSOC2 to allow longer variable
**	    length names.  Made CONNECT a union of peer info msg formats
**	    and 512 byte buffer representing current maximum length.
**	    Made Version 2 compatible with Unix (see GC_RQ_ASSOC2) for
**	    consistency and, more importantly, to allow direct connects
**	    between Windows and Unix/Linux on compatible hardware (Intel)
**	    for planned future enhancement to enable TCP/IP as a local
**	    IPC protocol.
**	14-Jan-2010 (Bruce Lunsford) Sir 122536 addendum
**	    Change max message size (see buf[]) from 512 to 1024 to agree
**	    with Unix and original intent for Windows.  512 is not quite
**	    large enough for worst case maximum length user and host names.
**
******************************************************************************/

typedef struct _CONNECT_orig
{
	PID   pid;
	ULONG flag;
	char  userid[127];
	char  account[127];
	char  access_point[127];
} CONNECT_orig;

typedef struct _GC_RQ_ASSOC2
{
    i4  	length;
    i4  	id;
    i4  	version;
    u_i4	flags;
    i4  	host_name_len;		/* These lengths include EOS */
    i4  	user_name_len;
    i4  	term_name_len;

    /*
    ** This is a variable length structure.  Host name, user name,
    ** and terminal name immediately follow in order indicated.
    */

} GC_RQ_ASSOC2;

typedef union _CONNECT
{
    CONNECT_orig	v_orig;
    GC_RQ_ASSOC2	v2;

    /*
    ** The following buffer provides storage space for variable length
    ** data associated with GC_RQ_ASSOC2.
    */
    u_i1		buffer[ 1024 ];

} CONNECT;


#define GC_RQ_ASSOC_ID	0x47435251	/* 'GCRQ' (ascii big-endian) */

#define GC_ASSOC_V2	2		/* +Names variable length */

#define GC_RQ_REMOTE	0x01		/* Flags: Direct remote connection */


/******************************************************************************
** Name: ASSOC_IPC - Local IPC association management structure
**
** Description:
**      This structure is used only within gcacl.c. It stores all
**  the information necessary to communicate via named pipes.
**  The information is on a per-association basis.
**
******************************************************************************/

typedef struct _ASSOC_IPC
{
        HANDLE    SndStdChan;
        char      SndStdName[MAXPIPENAME];
        HANDLE    RcvStdChan;
        char      RcvStdName[MAXPIPENAME];
        HANDLE    SndExpChan;
        char      SndExpName[MAXPIPENAME];
        HANDLE    RcvExpChan;
        char      RcvExpName[MAXPIPENAME];
        HANDLE    hRequestThread; 
        SVC_PARMS *Request_svc_parms;
        HANDLE    hDisconnThread; 
        i4        asyncIoPending;
        SVC_PARMS *Disconn_svc_parms;
        CONNECT   connect;
        i4        flags;
# define GC_IPC_LISTEN  1
# define GC_IPC_DISCONN 2
# define GC_IPC_NETCOMPL 4
        IPC_INT   IpcInternal;
        HANDLE    ClientReadHandle;
	HANDLE 	  hPurgeSemaphore;
        UINT      timer_event;
        ASYNC_TIMEOUT *async_timeout;
	char      *ClientSysUserName; /* For listen, client username per OS */
        SL_LABEL  sec_label;
        char      sec_label_pad[SL_MAX_EXTERNAL - sizeof(SL_LABEL)]; 
} ASSOC_IPC;

/*****************************************************************************
**
** Name: ASYNC_IOCB - Asyncronous I/O Control Block.
**
******************************************************************************/

typedef struct _ASYNC_IOCB 
{
	OVERLAPPED Overlapped;
	SVC_PARMS  *svc_parms;
	ASSOC_IPC  *asipc;
	i4         flags;

#define	GC_IO_READFILE	0x0001
#define	GC_IO_LISTEN	0x0002
#define	GC_IO_TIMEOUT	0x0004
#define	GC_IO_CREATED	0x0008
        DWORD      lstnthread;
} ASYNC_IOCB;


/*
** This structure is  used to store user name/label mappings from PM.
** This assumes a small number of users/labels so uses a simple  linked
** list currently. This is necessary since currently PMget can't be used
** directly within the GCA listen completion routine.
*/
typedef struct _user_label  {
        char *user_name;
        char *security_label;
        struct _user_label  *next;
} USER_LABEL;

/*
** External function definitions/prototypes.
*/
FUNC_EXTERN VOID   GClisten2();
FUNC_EXTERN VOID   GClisten_timeout();
FUNC_EXTERN VOID   GCwaitCompletion();
FUNC_EXTERN VOID   GCrestart();
FUNC_EXTERN VOID   GCRequestRoutine(SVC_PARMS *);
FUNC_EXTERN VOID   GCListenRoutine(SVC_PARMS *);
FUNC_EXTERN VOID   GCSendRoutine(SVC_PARMS *);
FUNC_EXTERN VOID   GCReceiveRoutine(SVC_PARMS *);
FUNC_EXTERN VOID   GCcloseAllPipes(ASSOC_IPC *);
FUNC_EXTERN VOID   WINAPI GCCompletionRoutine(DWORD, DWORD, LPOVERLAPPED);
FUNC_EXTERN DWORD  GCcreateStandardPipes(ASSOC_IPC *);
FUNC_EXTERN DWORD  GClistenToStandardPipes(ASSOC_IPC *);
FUNC_EXTERN DWORD  GCconnectToStandardPipes(ASSOC_IPC *);
FUNC_EXTERN HANDLE GCconnectToServer(ASSOC_IPC *, char *);
FUNC_EXTERN VOID   WINAPI GCasyncDisconn(SVC_PARMS *);
FUNC_EXTERN VOID   GCdisconnectAllPipes(ASSOC_IPC *);
