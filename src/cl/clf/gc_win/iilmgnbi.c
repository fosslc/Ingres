/*
** Copyright (c) 1987, 2009 Ingres Corporation
*/

/*
**  Name: GCLANMAN.C
**  Description:
**	Lan Manager NetBios protocol driver for Windows NT.
** 
**  History: *
** 
**	11-Nov-93 (edg)
**	    Windows NT implementation of protocol driver - lanman.
**	15-Nov-93 (edg)
**	    Fixed bugs.  Discovery: For some reason when we pass an NCB
**	    pointer to Netbios() that is from an area of memory allocated
**	    by MEreqmem() the call fails (retcode = 7, badbuf).  This
**	    may have something to do with the GlobalAlloc done in MEreqmem.
**	    Changed allocations of PCB's here to use malloc() which seems
**	    to work.
**	18-nov-93 (edg)
**	    Bug fix: wasn't filling in parm_list->buffer_lng when receive
**	    completed.  Also need to set INITIAL state for a request
**	    specific to the request type when getting new requests.
**	19-nov-93 (edg)
**	   structural improvements.
**	   Change tracing to allow only tracing this driver.
**      15-jul-95 (emmag)
**          Use a NULL Discretionary Access Control List (DACL) for
**          security, to give implicit access to everyone.
**      18-jul-95 (reijo01)
**          Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**              the proper values.
**      18-aug-95 (chech02, lawst01)
**          In function GClanman_async_thread - process is put to sleep for
**          1 msec if there are no executable requests in the Incoming queue.
**      12-Dec-95 (fanra01)
**          Extracted data for DLLs on windows NT.
**	19-mar-1996 (canor01)
**	    Allow connections with LANA number other than 0.
**	03-may-1996 (canor01)
**	    Allow for more than the default number of sessions.
**	    Remove names from namespace when disconnecting.
**	07-may-1996 (canor01)
**	    1) Use UCHAR_MAX for max number of sessions and max
**	    number of names (instead of 255). 2) Add a small timeout
**	    value for i/o completion checks--this cuts down on
**	    cpu usage by a large amount.  3) To keep names from
**	    growing beyond size limits, re-use old names. 4) For
**	    present, remove Netbios() call in function registered
**	    in atexit()--after connections have been made, this call
**	    will hang forever.
**      09-may-1996 (canor01)
**          Do not call GClanman_exit() as a function registered with
**          PCatexit().  Instead, call it when GCexec() processes the
**          SHUTDOWN event.
**	16-feb-98 (mcgem01)
**	    Read the port identifier from config.dat.  If a value is
**	    not found, use II_INSTALLATION, if all else fails default
**	    to the value for SystemVarPrefix, be it II or JAS.
**	23-Feb-1998 (thaal01)
**	   Make space for port_id string, stops gcc crashing on startup,
**	   sometimes.
**	01-Apr-98 ( rajus01 )
**	    Added GLOBALREF is_comm_svr and set it to TRUE in GCC_OPEN for
**	    communication server.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
** 	29-jun-2000 (somsa01)
**	    Assured that we stuff the machine name in front of the port id
**	    before registering the server's listen port. Also, the listen
**	    port id must be unique for each GCC that starts up, and for
**	    every client that comes up.
**	16-mar-2001 (somsa01)
**	    In GClanman_listen(), set node_id to ncb_callname.
**      11-dec-2002 (loera01)  SIR 109237
**          Set option flag in GCC_P_LIST to 0 (remote).
**	13-may-2004 (somsa01)
**	    In GClanman_init(), updated config.dat string used to retrieve
**	    port information such that we do not rely specifically on the GCC
**	    port.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      06-Aug-2009 (Bruce Lunsford) Sir 122426
**          Remove mutexing around calls to GCA service completion routine
**          as it is no longer necessary, since GCA is thread-safe...removes
**          calls to GCwaitCompletion + GCrestart. Should improve peformance.
**          Convert GCC completion queue mutex to a critical section
**          to improve performance (less overhead).
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime. This also involves using errno instead of
**	    GetLastError() for failures and _endthreadex() at end of thread.
*/

# include	<compat.h>
# include	<malloc.h>
# include 	<nb30.h>
# include	<er.h>
# include	<cm.h>
# include	<cs.h>
# include	<me.h>
# include	<qu.h>
# include	<pc.h>
# include	<nm.h>
# include	<lo.h>
# include	<pm.h>
# include	<tr.h>
# include	<st.h>
# include	<cv.h>
# include	<gcccl.h>
# include	<gc.h>

#define port parm_list->function_parms.connect.port_id
#define node parm_list->function_parms.connect.node_id

# include	"gclocal.h"
# include	"gcclocal.h"

#define GC_STACK_SIZE   0x8000

/*
* *  Forward and/or External function references.
*/

STATUS 		GClanman_init(GCC_PCE * pptr);
STATUS          GClanman(i4 function_code, GCC_P_PLIST * parm_list);
VOID     	GClanman_exit(void);
VOID            GClanman_async_thread(void *parms);
STATUS          GClanman_open(GCC_P_PLIST *parm_list);
VOID            GClanman_listen(void *parms);
extern VOID	GCcomplete_request( QUEUE * );

/*
** Globals and Statics: 
*/
static char	GCc_listen_port[64];
static char	GCc_client_name[128];
static char	MyName[NCBNAMSZ + 1];
static int	GCc_client_count = 0;
static bool	In_Shutdown = FALSE;
static NCB	Name_Ncb;
static NCB	General_Ncb;
static NCB	Listen_Ncb;
static int	lana_num = 0;

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

/*
** Static: These Event/Mutex handles only used within this module for
** thread synchronization.
*/
static HANDLE hMutexThreadInQ;
static HANDLE hEventThreadInQ;

static  THREAD_EVENTS *Tptr = NULL;

/*
** Dummy buffer -- seems like the ncb_buffer needs to be set for calls that
** really don't seem to require it.  Point the ncb_buffer here.
*/
char	Dummy_Buf[256];


/*
** The PCB is allocated on listen or connection request.  The format is
** internal and specific to each protocol driver.
*/
typedef struct _PCB
{
	PCB_STATE 	state;
	NCB		r_ncb;	/* for receives */
	NCB		s_ncb;  /* for sends */
} PCB;


/*
**  Defines of other constants.
*/
GLOBALREF i4 GCLANMAN_trace;
GLOBALREF bool is_comm_svr;
# define GCTRACE(n) if ( GCLANMAN_trace >= n ) (void)TRdisplay



/*
** Name: GClanman_init
** Description:
**	LANMAN inititialization function.  This routine is called from
**	GCpinit() -- the routine GCC calls to initialize protocol drivers.
**
**	This function does initialization specific to the protocol:
**	    Creates Events and Mutex's for the protocol
**	    Finds and saves a pointer to it's input event Q.
**	    Fires up the thread which will do asynch I/O
** History:
**	11-Nov-93 (edg)
**	    created.
**      15-jul-95 (emmag)
**          Use a NULL Discretionary Access Control List (DACL) for
**          security, to give implicit access to everyone.
**	23-Feb-1998 (thaal01)
**	    Make space for port_id, stops gcc crashing on startup, sometimes.
**	13-may-2004 (somsa01)
**	    Updated config.dat string used to retrieve port information such
**	    that we do not rely specifically on the GCC port.
**	06-Aug-2009 (Bruce Lunsford)  Sir 122426
**	    Change arglist pointer in _beginthreadex for async_thread from
**	    uninitialized "dummy" to NULL to eliminate compiler warning
**	    and possible startup problem.
*/
STATUS
GClanman_init(GCC_PCE * pptr)
{

	char            *ptr, *host, *server_id, *port_id;
	char		config_string[256];
	char            buffer[MAX_COMPUTERNAME_LENGTH + 1];
	int 		real_name_size = MAX_COMPUTERNAME_LENGTH + 1;
	i4		i;
	int		tid;
	HANDLE		hThread;
	int		status;
	SECURITY_ATTRIBUTES sa;
	char		port_id_buf[8];

	port_id = port_id_buf;

	iimksec (&sa);

	/*
	** Look for trace variable.
	*/
	NMgtAt( "II_LANMAN_TRACE", &ptr );
	if ( !(ptr && *ptr) && PMget("!.lanman_trace_level", &ptr) != OK )
	{
	    GCLANMAN_trace = 0;
	}
	else
	{
	    GCLANMAN_trace = atoi( ptr );
	}

	/*
	** Create MUTEX and EVENT for the input queue of this protocol
	** driver.
	*/
	if ( ( hMutexThreadInQ = CreateMutex(&sa, FALSE, NULL) ) == NULL )
	{
	    return FAIL;
	}

	GCTRACE(3)( "GClanman_init: MutexInQ Handle = %d\n", hMutexThreadInQ );

	if ( ( hEventThreadInQ = CreateEvent(&sa, FALSE, FALSE, NULL)) == NULL )
	{
	    CloseHandle( hMutexThreadInQ );
	    return FAIL;
	}

	GCTRACE(3)( "GClanman_init: EventInQ Handle = %d\n", hEventThreadInQ );
	

	GCTRACE(4)( "Start GClanman_init\n" );

        /*
        ** Get set up for the PMget call.
        */
        PMinit();
        if( PMload( NULL, (PM_ERR_FUNC *)NULL ) != OK )
                PCexit( FAIL );

	/*
	** Construct the network port identifier. 
	*/

        host = PMhost();
	server_id = PMgetDefault(3);
 	if (!server_id)
 		server_id = "*" ;
        STprintf( config_string, ERx("!.lanman.port"),
                  SystemCfgPrefix, host, server_id);

	/*
	** Search config.dat for a match on the string we just built.
	** If we don't find it, then use the value for II_INSTALLATION 
	** failing that, default to II.
	*/
        PMget( config_string, &port_id );
	if (port_id == NULL )
	{
		NMgtAt("II_INSTALLATION", &ptr);
		if (ptr != NULL && *ptr != '\0')
		{
			STcopy(ptr, port_id);
		}
		else 
		{ 
			STcopy(SystemVarPrefix, port_id);
		}
	}

	NMgtAt( "II_NETBIOS_NODE", &ptr );
	if ( !ptr || !*ptr  )
	{
	    /*
	    ** Get Computer Name into buffer.
	    */
	    *buffer = (char)NULL;
	    GetComputerName( buffer, &real_name_size );
	    if ( !*buffer )
	        STcopy( "NONAME", buffer );
	    ptr = buffer;
	}
	/*
	** MyName holds ID for outgoing connections.
	*/
	STpolycat( 2, ptr, "_", MyName );
	/*
	** Create listen port ID.
	*/
	STpolycat( 3, ptr, "_", port_id, GCc_listen_port );
	CVupper( MyName );
	CVupper( GCc_listen_port );
	STcopy( GCc_listen_port, pptr->pce_port );

	GCTRACE(2)("GClanman_init: port = %s\n", pptr->pce_port );

	/*
	** Go thru the the protocol threads event list and find the index
	** of the lanman thread.  Set the Global Tptr for easy reference
	** to the event q's for this protocols thread.
	*/
	for ( i = 0; i < IIGCc_proto_threads.no_threads; i++ )
	{
	     THREAD_EVENTS *p = &IIGCc_proto_threads.thread[i];

	     if ( !STcompare( LANMAN_ID, p->thread_name ) )
	     {
	         Tptr = p;
	         break;
	     }
	}
	if ( Tptr == NULL )
	{
	    CloseHandle( hEventThreadInQ );
	    CloseHandle( hMutexThreadInQ );
	    return FAIL;
	}

	/*
	** Finally we start the asynchronous I/O thread
	*/

	hThread = (HANDLE)_beginthreadex(&sa,
		       GC_STACK_SIZE,
		       (LPTHREAD_START_ROUTINE) GClanman_async_thread,
		       NULL,
		       (unsigned long)NULL,
		       &tid);
	if (hThread) 
	{
		CloseHandle(hThread);
	}
	else
	{
	    status = errno;
	    GCTRACE(1)("GClanman_init: Couldn't create thread errno = %d '%s'\n",
	    		status, strerror(status) );
	    return FAIL;
	}

	return OK;
}



/*
** Name: GClanman
** Description:
**	Main entry point for the window's NT lan manager protocol driver.  This
** 	driver is essentially just a dispatcher -- it runs in the primary
**	GCC thread and mostly just Q's things to do to the constantly running
**	aynchronous request thread.  It may also start a listen thread if
**	it is a LISTEN request.
**
**	The following functions are handled:
**	    GCC_OPEN	- call GClanman_open
**	    GCC_LISTEN  - start listen thread
**	    GCC_SEND    - Q request for asynch thread
**	    GCC_RECEIVE - Q request for asynch thread
**	    GCC_CONNECT - Q request for asynch thread
**	    GCC_DISCONN - Q request for asynch thread
** History:
**	11-Nov-93 (edg)
**	    Original.
**      06-Aug-2009 (Bruce Lunsford) Sir 122426
**          Remove mutexing around calls to GCA service completion routine
**          as it is no longer necessary, since GCA is thread-safe...removes
**          calls to GCwaitCompletion + GCrestart. Should improve peformance.
**	    Convert CreateThread() to _beginthreadex() which is recommended
**	    when using C runtime.
*/
STATUS
GClanman( i4 function_code, GCC_P_PLIST * parm_list)
{
	STATUS          generror = 0;
	int             status = 0;
	int             tid;
	HANDLE          hThread;
    	REQUEST_Q 	*rq;
	SECURITY_ATTRIBUTES sa;

	iimksec (&sa);

	CLEAR_ERR(&parm_list->system_status);

	/*
	** set error based on function code and determine whether we got a
	** valid function.
	*/
	switch (function_code) {
	case GCC_OPEN:
		is_comm_svr = TRUE;
		GCTRACE(2) ("GClanman: Function = OPEN\n" );
		return GClanman_open( parm_list );

	case GCC_LISTEN:
		GCTRACE(2) ("GClanman: Function = LISTEN\n" );
		generror = GC_LISTEN_FAIL;
                /*
                ** For Lanman, the peer is always remote.
                */
                parm_list->options = 0;

		/*
		** Spawn off a thread to handle the listen request
		*/
		hThread = (HANDLE)_beginthreadex(&sa,
			       GC_STACK_SIZE,
			       (LPTHREAD_START_ROUTINE) GClanman_listen,
			       parm_list,
			       (unsigned long)NULL,
			       &tid);
		if (hThread) 
		{
			CloseHandle(hThread);
			return (OK);
		}
		status = errno;
		SETWIN32ERR(&parm_list->system_status, status, ER_create);
		goto err_exit;
		break;

	case GCC_CONNECT:
	    GCTRACE(2) ("GClanman: Function = CONNECT\n" );
	    generror = GC_CONNECT_FAIL;
	    break;
	case GCC_SEND:
	    GCTRACE(2) ("GClanman: Function = SEND\n" );
	    generror = GC_SEND_FAIL;
	    break;
	case GCC_RECEIVE:
	    GCTRACE(2) ("GClanman: Function = RECEIVE\n" );
	    generror = GC_RECEIVE_FAIL;
	    break;
	case GCC_DISCONNECT:
	    GCTRACE(2) ("GClanman: Function = DISCONNECT\n" );
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
	GCTRACE(2)("GClanman: Q'ing request ...\n");

    	if ( (rq = (REQUEST_Q *)MEreqmem(0, sizeof(*rq), TRUE, NULL ) ) != NULL )
    	{
            rq->plist = parm_list;
	    /*
	    ** get mutex for completion Q
	    */
	    GCTRACE(2)("GClanman: wait for input mutex ...\n");
	    WaitForSingleObject( hMutexThreadInQ, INFINITE );

	    /*
	    ** Now insert the completed request into the inconming Q.
	    */
	    GCTRACE(2)("GClanman: inserting incoming req ...\n");
	    QUinsert( &rq->req_q, &Tptr->incoming_head );

	    /*
	    ** release mutex for completion Q
	    */
	    GCTRACE(2)("GClanman: releasing Mutex incoming req ...\n");
	    ReleaseMutex( hMutexThreadInQ );

	    /*
	    ** raise the incoming event to wake up the thread.
	    */
	    GCTRACE(2)("GClanman: Setting event ...\n");
	    if ( !SetEvent( hEventThreadInQ ) )
	    {
	   	    status = GetLastError();
		    SETWIN32ERR(&parm_list->system_status, status, ER_sevent);
	   	    GCTRACE(1)("GClanman, SetEvent error = %d\n", 
		    		status );
	    }

	    return OK;

   	}
    	else
    	{
	        /*
		** MEreqmem failed
		*/
		SETWIN32ERR(&parm_list->system_status, errno, ER_alloc);
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
** Name: GClanman_listen
** Description:
**	This is the listen thread for lanman.  It runs a syncronous accept()
**	on the listen socket.  When complete it Q's the completetion to
**	the completed event Q.  When accept completes, this thread returns.
**	A new one will be created when GClanman() gets the request to
**	repost the listen.
** History:
**	11-nov-93 (edg)
**	    created.
**	29-jun-2000 (somsa01)
**	    Use GCc_listen_port for the ncb_name.
**	16-mar-2001 (somsa01)
**	    Set node_id to ncb_callname.
**      06-Aug-2009 (Bruce Lunsford) Sir 122426
**          Convert GCC completion queue mutex to a critical section
**          to improve performance (less overhead).
**	    Since _beginthreadex() is now used to start this thread,
**	    use _endthreadex() to end it.
*/
VOID
GClanman_listen( VOID *parms )
{
    GCC_P_PLIST		*parm_list = (GCC_P_PLIST *)parms;
    PCB			*pcb;
    STATUS		status = OK;
    REQUEST_Q		*rq;
    SECURITY_ATTRIBUTES	sa;

    iimksec (&sa);

    /*
    ** Initialize the listen node_id to NULL.
    */
    parm_list->function_parms.listen.node_id = NULL;

    /*
    ** Initialize fields of the Listen_Ncb
    */
    memset( &Listen_Ncb, 0, sizeof(NCB) );
    Listen_Ncb.ncb_buffer = parm_list->buffer_ptr;
    Listen_Ncb.ncb_length = (USHORT)parm_list->buffer_lng;
    Listen_Ncb.ncb_command = NCBLISTEN;
    Listen_Ncb.ncb_lana_num = lana_num;
    *Listen_Ncb.ncb_callname = (unsigned char)NULL;
    STmove( "*", ' ', NCBNAMSZ, Listen_Ncb.ncb_callname );
    *Listen_Ncb.ncb_name = (unsigned char)NULL;
    STcopy( GCc_listen_port, Listen_Ncb.ncb_name );


    /*
    ** Now we can do the NCBLISTEN request.  Block until it completes.
    */
    Netbios( &Listen_Ncb );
    if ( Listen_Ncb.ncb_retcode != NRC_GOODRET )
    {
	status = (int)Listen_Ncb.ncb_retcode;
	goto sys_err;
    }

    /*
    ** Allocate Protcol Control Block specific to this driver and put into
    ** parm list
    */
    pcb = (PCB *) malloc( sizeof(PCB) );
    if (pcb == NULL) 
    {
	status = errno;
	goto sys_err;
    }
    memset( pcb, 0, sizeof( *pcb ) );
    parm_list->pcb = (char *)pcb;

    /*
    ** Set node_id to the node name of the partner.
    */
    parm_list->function_parms.listen.node_id = STalloc(Listen_Ncb.ncb_callname);

    /*
    ** Now assign the pcb's send and receive NCB's the local session number
    ** returned by listen for further communications.
    */
    pcb->s_ncb.ncb_lsn = pcb->r_ncb.ncb_lsn = Listen_Ncb.ncb_lsn;

    /*
    ** Now create handles for the read and write event handles in the
    ** NCB.  These are created with manual reset as cautioned by the 
    ** programmer's guide so a ResetEvent MUST be done on them.
    */
    if ((pcb->s_ncb.ncb_event = CreateEvent( &sa, TRUE, FALSE, NULL ))== NULL)
    {
        status = GetLastError();
	goto sys_err;
    }
    if ((pcb->r_ncb.ncb_event = CreateEvent( &sa, TRUE, FALSE, NULL ))== NULL)
    {
        status = GetLastError();
	CloseHandle( pcb->s_ncb.ncb_event );
	pcb->s_ncb.ncb_event = NULL;
	goto sys_err;
    }

sys_err:
    if (status != OK) 
    {
        SETWIN32ERR(&parm_list->system_status, status, ER_create);
	parm_list->generic_status = GC_LISTEN_FAIL;
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
	** Exit/leave critical section for completion Q
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
	   GCTRACE(1)("GClanman_listen, SetEvent error = %d\n", status );
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
** Name: GClanman_open
** Description:
**	Open the listen channel for LANMAN.   Called from GClanman().  This
**	routine should only be called once at server startup.
** History:
**	11-nov-93 (edg)
**	    created.
**	03-may-1996 (canor01)
**	    Allow for more than the default number of sessions.
**	29-jun-2000 (somsa01)
**	    Use GCc_listen_port for the ncb_name, making sure that we
**	    update GCc_listen_port if we need a unique one.
**      06-Aug-2009 (Bruce Lunsford) Sir 122426
**          Remove mutexing around calls to GCA service completion routine
**          as it is no longer necessary, since GCA is thread-safe...removes
**          calls to GCwaitCompletion + GCrestart. Should improve peformance.
*/
STATUS
GClanman_open( GCC_P_PLIST *parm_list )
{
	STATUS	status;
	LANA_ENUM	lanas;
	i4		subport = 0;

	/*
	** Find available adapters
	*/
	memset( &Listen_Ncb, 0, sizeof(NCB) );
	Listen_Ncb.ncb_buffer = &lanas;
	Listen_Ncb.ncb_length = sizeof(LANA_ENUM);
	Listen_Ncb.ncb_command = NCBENUM;
	Netbios( &Listen_Ncb );
	if ( lanas.length == 0 )
	{
		status = (STATUS)Listen_Ncb.ncb_retcode;
		GCTRACE(1)("GClanman_open: No LANA adapters %d\n",
			    status );
		parm_list->generic_status = GC_OPEN_FAIL;
		SETWIN32ERR(&parm_list->system_status, status, ER_listen);
		goto err_exit;
	}
	/* use first available LANA */
	lana_num = lanas.lana[0];

	/*
	** Use Listen_Ncb to 1st do a reset to start things out ...
	*/
	memset( &Listen_Ncb, 0, sizeof(NCB) );
	Listen_Ncb.ncb_buffer = Dummy_Buf;
	Listen_Ncb.ncb_length = sizeof(Dummy_Buf);
	Listen_Ncb.ncb_lana_num = lana_num;
	Listen_Ncb.ncb_callname[0] = 255; /* max number of sessions */
	Listen_Ncb.ncb_callname[2] = 255; /* max number of names */
	Listen_Ncb.ncb_command = NCBRESET;
	Netbios( &Listen_Ncb );

	/*
	** Add Name we'll be listening on.
	*/
	memset( &Listen_Ncb, 0, sizeof(NCB) );
	Listen_Ncb.ncb_buffer = Dummy_Buf;
	Listen_Ncb.ncb_length = sizeof(Dummy_Buf);
	Listen_Ncb.ncb_lana_num = lana_num;
	Listen_Ncb.ncb_command = NCBADDNAME;

	for (;;)
	{
	    if (subport)
		STprintf(Listen_Ncb.ncb_name, "%s%d", GCc_listen_port, subport);
	    else
		STcopy( GCc_listen_port, Listen_Ncb.ncb_name );

	    Netbios( &Listen_Ncb );

	    if (Listen_Ncb.ncb_retcode == NRC_GOODRET)
		break;
	    else if (Listen_Ncb.ncb_retcode == NRC_DUPNAME && (++subport) < 8)
		continue;
	    else
	    {
		status = (STATUS)Listen_Ncb.ncb_retcode;
		GCTRACE(1)("GClanman_open: Error ADDNAME returned %d\n",
			    status );
		parm_list->generic_status = GC_OPEN_FAIL;
		SETWIN32ERR(&parm_list->system_status, status, ER_listen);
		goto err_exit;
	    }
	}

	/*
	** Set listen port in parm list.
	*/
	STcopy(Listen_Ncb.ncb_name, GCc_listen_port);
	if (subport)
	{
	    STprintf(parm_list->pce->pce_port, "%s%d",
		     parm_list->function_parms.open.port_id, subport);
	}
	parm_list->function_parms.open.lsn_port = GCc_listen_port;

err_exit:
	/*
	** Should completion be driven here?
	*/
	(*parm_list->compl_exit) (parm_list->compl_id);
	return OK;
}


/*
** Name: GClanman_async_thread
** Description:
**	This thread handles all the asynchronous I/O for a protocol driver.
** 	It will be woken up when GClanman() places a request on it's input
**	Q.  Then, this thread will move the request from the input Q to its
**	processing Q and continue to process the request until complete.
**	When complete, the request is finally moved to the completion
**	Q.
** History:
**	04-Nov-93 (edg)
**	    Written.
**	29-jun-2000 (somsa01)
**	    Use GCc_listen_port for the server ncb_name. Also, make sure
**	    that we update GCc_client_name if we need a unique one.
**      06-Aug-2009 (Bruce Lunsford) Sir 122426
**	    Since _beginthreadex() is now used to start this thread,
**	    use _endthreadex() to end it.
*/
VOID
GClanman_async_thread( VOID * parms)
{
	int             status = OK;
	char            callname[NCBNAMSZ+1];
	DWORD		wait_stat;
	HANDLE		hSave;
	int		processing_requests = 0;
	int		pending_requests = 0;
	QUEUE		*q;
	SECURITY_ATTRIBUTES sa;

	iimksec (&sa);

	GCTRACE(4)("LMAN THREAD: started.\n");
top:
	/*
	** Wait for a request to come in from the primary gcc thread....
	*/

	GCTRACE(4)("LMAN THREAD: waiting for event ... \n");

	wait_stat = WaitForSingleObject( hEventThreadInQ, INFINITE );

	GCTRACE(3)("LMAN THREAD: wait returned %d, handle = %d\n", 
		    wait_stat, hEventThreadInQ );

	/*
	** If wait failed, chances are it's a major hosure.  Continue on any
	** way -- there's a possibility that something useful may get done.
	*/
	if (wait_stat == WAIT_FAILED)
	{
	    GCTRACE(1)("LMAN THREAD: wait failed %d\n", 
	    		GetLastError() );
	}

	/*
	** Now get get the incoming requests and add up how many requests
	** we're processing.
	*/
	processing_requests = GCget_incoming_reqs( Tptr, hMutexThreadInQ );

	GCTRACE(2)("LMAN THREAD: Got %d new requests to process\n",
		    processing_requests);

	/*
	** Loop until there's no more requests being processed.
	*/
	while( processing_requests )
	{
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

		parm_list->generic_status = OK;
		CLEAR_ERR(&parm_list->system_status);

		switch (parm_list->function_invoked) 
		{

		    /******************************************************
		    ** Handle CONNECT
		    *******************************************************/
		    case GCC_CONNECT:

		    GCTRACE(4)("LMAN THREAD: process CONNECT\n");
		    if ( pcb == NULL || pcb->state.conn == INITIAL )
		    {
		        GCTRACE(3)("LMAN THREAD: initial CONNECT\n");
		        /*
			** Allocate the protocol control block.
			*/
		  	pcb = (PCB *) malloc( sizeof(PCB) );
		    	parm_list->pcb = (char *)pcb;
			if (pcb == NULL) 
			{
			    status = errno;
			    SETWIN32ERR(&parm_list->system_status, status, ER_alloc);
			    pcb->state.conn = COMPLETED;
			    parm_list->generic_status = GC_CONNECT_FAIL;
			    break;
		   	}
			memset( pcb, 0, sizeof( *pcb ) );
		        GCTRACE(3)("LMAN THREAD: CONNECT allocated pcb\n");
			/*
			** Create send/recv event handles for ncb.
			*/
    			if ((pcb->s_ncb.ncb_event = 
				CreateEvent( &sa, TRUE, FALSE, NULL ))== NULL)
    			{
        		    status = GetLastError();
			    pcb->state.conn = COMPLETED;
			    SETWIN32ERR(&parm_list->system_status, status, ER_create);
			    parm_list->generic_status = GC_CONNECT_FAIL;
			    break;
    		        }
       		 	if ((pcb->r_ncb.ncb_event = 
				CreateEvent( &sa, TRUE, FALSE, NULL ))== NULL)
    			{
        		    status = GetLastError();
			    CloseHandle( pcb->s_ncb.ncb_event );
			    pcb->state.conn = COMPLETED;
			    SETWIN32ERR(&parm_list->system_status, status, ER_create);
			    parm_list->generic_status = GC_CONNECT_FAIL;
			    break;
    			}
		        GCTRACE(3)("LMAN THREAD: CONNECT created events\n");

			pcb->state.conn = INITIAL;

		    } /* end if pcb NULL */

		    /*
		    ** If the PCB state is not INITIAL, just break because
		    ** we're just waiting for connect to complete.
		    */
		    if ( pcb->state.conn != INITIAL )
		        break;

		    /*
		    ** Use the send ncb in pcb for the connect -- add name.
		    */
		    pcb->s_ncb.ncb_command = NCBADDNAME;
		    pcb->s_ncb.ncb_buffer = Dummy_Buf;
		    pcb->s_ncb.ncb_length = sizeof(Dummy_Buf);
		    pcb->s_ncb.ncb_lana_num = lana_num;

		    for (;;)
		    {
			STprintf( GCc_client_name, "%s%-d", 
				  MyName, GCc_client_count++ );
			STcopy( GCc_client_name, pcb->s_ncb.ncb_name );

			GCTRACE(3)("LMAN THREAD: CONNECT doing ADDNAME %s\n",
				   pcb->s_ncb.ncb_name );

			/*
			** Copy to local NCB struct -- Netbios seems to fark
			** up if we don't.
			*/
			memcpy( &Name_Ncb, &pcb->s_ncb, sizeof( Name_Ncb ) );

			Netbios( &Name_Ncb );

			if (Name_Ncb.ncb_retcode == NRC_GOODRET)
			    break;
			else if (Name_Ncb.ncb_retcode == NRC_DUPNAME)
			    continue;
			else
			{
			    status = (STATUS)Name_Ncb.ncb_retcode;
			    CloseHandle( Name_Ncb.ncb_event );
			    pcb->s_ncb.ncb_event = NULL;
			    CloseHandle( pcb->r_ncb.ncb_event );
			    pcb->r_ncb.ncb_event = NULL;
			    pcb->state.conn = COMPLETED;
			    SETWIN32ERR(&parm_list->system_status, status,
					ER_netbios);
			    parm_list->generic_status = GC_CONNECT_FAIL;
			    break;
			}
		    }

		    if (parm_list->generic_status == GC_CONNECT_FAIL)
			break;

		    /*
		    ** just in case ...
		    */
		    ResetEvent( pcb->s_ncb.ncb_event );

		    /*
		    ** OK, now make the call
		    */
		    hSave = pcb->s_ncb.ncb_event;    /* save handle */
		    memset( &pcb->s_ncb, 0, sizeof(NCB) );
		    pcb->s_ncb.ncb_event = hSave;    /* restore handle */
		    pcb->s_ncb.ncb_buffer = parm_list->buffer_ptr;    
		    pcb->s_ncb.ncb_length = (WORD)parm_list->buffer_lng;    
		    pcb->s_ncb.ncb_command = NCBCALL | ASYNCH;
		    pcb->s_ncb.ncb_lana_num = lana_num;
		    STcopy( GCc_client_name, pcb->s_ncb.ncb_name );
		    STpolycat( 3, parm_list->function_parms.connect.node_id,
		    	       "_", parm_list->function_parms.connect.port_id,
			       callname );
		    CVupper( callname );

		    
		    /*
		    ** Loopback check to prevent mangling the name (?)
		    */
		    if ( STcompare( parm_list->function_parms.connect.port_id,
		    		    GCc_listen_port ) == 0 )
		    {
		        STcopy( GCc_listen_port, pcb->s_ncb.ncb_callname );
		    }
		    else
		    {
		        STcopy( callname, pcb->s_ncb.ncb_callname );
		    }

		    GCTRACE(3)("LMAN THREAD: CONNECT doing CALL to %s\n",
		    		pcb->s_ncb.ncb_callname );

		    if ( Netbios( &pcb->s_ncb ) != NRC_GOODRET )
		    {
		        status = (STATUS)pcb->s_ncb.ncb_retcode;
			CloseHandle( pcb->s_ncb.ncb_event );
			pcb->s_ncb.ncb_event = NULL;
			CloseHandle( pcb->r_ncb.ncb_event );
			pcb->r_ncb.ncb_event = NULL;
			pcb->state.conn = COMPLETED;
			SETWIN32ERR(&parm_list->system_status, status, ER_netbios);
			parm_list->generic_status = GC_CONNECT_FAIL;
			break;
		    }

		    GCTRACE(3)("LMAN THREAD: Async CALL OK\n" );

		    pcb->state.conn = COMPLETING;

		    break;


		    /*******************************************************
		    ** Handle SEND
		    *******************************************************/
		    case GCC_SEND:

		    GCTRACE(4)("LMAN THREAD: process SEND\n");

		    if ( pcb->state.send != INITIAL )
		    {
		        break;
		    }
		    pcb->s_ncb.ncb_buffer = parm_list->buffer_ptr;    
		    pcb->s_ncb.ncb_length = (WORD)parm_list->buffer_lng;    
		    pcb->s_ncb.ncb_lana_num = lana_num;
		    pcb->s_ncb.ncb_command = NCBSEND | ASYNCH;
		    if ( Netbios( &pcb->s_ncb ) != NRC_GOODRET )
		    {
		        status = (STATUS)pcb->s_ncb.ncb_retcode;
			pcb->state.send = COMPLETED;
			SETWIN32ERR(&parm_list->system_status, status, ER_netbios);
			parm_list->generic_status = GC_SEND_FAIL;
		    }

		    pcb->state.send = COMPLETING;

		    break;


		    /*******************************************************
		    ** Handle RECEIVE
		    *******************************************************/
		    case GCC_RECEIVE:

		    GCTRACE(4)("LMAN THREAD: process RECEIVE\n");

		    if ( pcb->state.recv != INITIAL )
		    {
                        pending_requests++;
		        break;
		    }
		    pcb->r_ncb.ncb_buffer = parm_list->buffer_ptr;    
		    pcb->r_ncb.ncb_length = (WORD)parm_list->buffer_lng;    
		    pcb->r_ncb.ncb_lana_num = lana_num;
		    pcb->r_ncb.ncb_command = NCBRECV | ASYNCH;

		    if ( Netbios( &pcb->r_ncb ) != NRC_GOODRET )
		    {
		        status = (STATUS)pcb->r_ncb.ncb_retcode;
			pcb->state.recv = COMPLETED;
			SETWIN32ERR(&parm_list->system_status, status, ER_netbios);
			parm_list->generic_status = GC_RECEIVE_FAIL;
		    }

		    pcb->state.recv = COMPLETING;

		    break;

		    /*******************************************************
		    ** Handle DISCONNECT
		    *******************************************************/
		    case GCC_DISCONNECT:

		    GCTRACE(4)("LMAN THREAD: process DISCONNECT\n");

		    if ( pcb && pcb->state.disc == INITIAL ) 
		    {
		        pcb->s_ncb.ncb_buffer = parm_list->buffer_ptr;    
		        pcb->s_ncb.ncb_length = (WORD)parm_list->buffer_lng;    
		        pcb->s_ncb.ncb_command = NCBHANGUP | ASYNCH;
		    	pcb->s_ncb.ncb_lana_num = lana_num;
			if ( pcb->s_ncb.ncb_lsn == 0 )
			    pcb->s_ncb.ncb_lsn = pcb->r_ncb.ncb_lsn;
		        if ( Netbios( &pcb->s_ncb ) != NRC_GOODRET )
		        {
		            status = (STATUS)pcb->s_ncb.ncb_retcode;
		    	    pcb->state.disc = COMPLETED;
			    SETWIN32ERR(&parm_list->system_status, status, ER_netbios);
			    parm_list->generic_status = GC_DISCONNECT_FAIL;
			    break;
		        }
		        pcb->state.disc = COMPLETING;
		    }

		    break;

		} /* end switch */

	    } /* end for process q loop */

	    /*
	    ** Now go thru the inprocess Q and look for any requests that
	    ** have been completed.  This will be indicated by one of:
	    ** parm_list->pcb == NULL (bad connect or after disconnect) or
	    ** pcb->state == COMPLETED, or WaitForSingleObject indicates
	    ** completion.
	    */

	    GCTRACE(4)("LMAN THREAD: processing completed. . . \n");

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
		        if ( pcb == NULL || pcb->state.conn == COMPLETED || 
			     WaitForSingleObject( pcb->s_ncb.ncb_event, 0) ==
			         WAIT_OBJECT_0 )
			{
			    if (pcb) 
			    {
			        ResetEvent( pcb->s_ncb.ncb_event );
			        pcb->r_ncb.ncb_lsn = pcb->s_ncb.ncb_lsn;
				if ( pcb->s_ncb.ncb_lsn == 0 ||
				     pcb->s_ncb.ncb_retcode != NRC_GOODRET )
				{
				    pl->generic_status = GC_CONNECT_FAIL;
				    status = (STATUS)pcb->s_ncb.ncb_retcode;
				    SETWIN32ERR( &pl->system_status, status , ER_revent);
				    CloseHandle( pcb->s_ncb.ncb_event );
				    CloseHandle( pcb->r_ncb.ncb_event );
				    free( pcb );
				    pl->pcb = NULL;
				}
			    }

			    completed = TRUE;
			}
		        break;
		    case GCC_SEND:
		        if ( pcb == NULL || pcb->state.send == COMPLETED || 
			     WaitForSingleObject( pcb->s_ncb.ncb_event, 0) ==
			         WAIT_OBJECT_0 )
			{
			    ResetEvent( pcb->s_ncb.ncb_event );
			    if ( pcb->s_ncb.ncb_lsn == 0 ||
			         pcb->s_ncb.ncb_retcode != NRC_GOODRET )
			    {
				    pl->generic_status = GC_SEND_FAIL;
				    status = (STATUS)pcb->s_ncb.ncb_retcode;
				    SETWIN32ERR( &pl->system_status, status , ER_revent);
			    }
			    else
			    {
			        GCTRACE(2)(
				"LMAN THREAD: Send COMP pl len %d pcb len %d\n",
			            pl->buffer_lng, pcb->s_ncb.ncb_length);
					
			        pl->buffer_lng = pcb->s_ncb.ncb_length;
			    }
			    completed = TRUE;
			}
		        break;
		    case GCC_RECEIVE:
		        if ( pcb == NULL || pcb->state.recv == COMPLETED || 
			     WaitForSingleObject( pcb->r_ncb.ncb_event, 0) ==
			         WAIT_OBJECT_0 )
			{
			    ResetEvent( pcb->r_ncb.ncb_event );
			    if ( pcb->s_ncb.ncb_lsn == 0 ||
			         pcb->r_ncb.ncb_retcode != NRC_GOODRET )
			    {
				    pl->generic_status = GC_RECEIVE_FAIL;
				    status = (STATUS)pcb->r_ncb.ncb_retcode;
				    SETWIN32ERR( &pl->system_status, status , ER_revent);
			    }
			    else
			    {
			        pl->buffer_lng = pcb->r_ncb.ncb_length;
			    }
			    completed = TRUE;
			}
		        break;
		    case GCC_DISCONNECT:
		        if ( pcb == NULL || pcb->state.disc == COMPLETED || 
			     WaitForSingleObject( pcb->s_ncb.ncb_event, 0) ==
			         WAIT_OBJECT_0 )
			{
			    if (pcb) 
			    {
				if ( pcb->s_ncb.ncb_lsn == 0 ||
				     pcb->s_ncb.ncb_retcode != NRC_GOODRET )
				{
				    pl->generic_status = GC_DISCONNECT_FAIL;
				    status = (STATUS)pcb->s_ncb.ncb_retcode;
				    SETWIN32ERR( &pl->system_status, status , ER_revent);
				}
				pcb->s_ncb.ncb_command = NCBDELNAME;
				Netbios( &pcb->s_ncb );
				CloseHandle( pcb->s_ncb.ncb_event );
				CloseHandle( pcb->r_ncb.ncb_event );
				free( pcb );
				pl->pcb = NULL;
			    }
			    completed = TRUE;
			}
		        break;
		} /* end switch */

		if ( completed )
		{
		    QUEUE *nq = q->q_next;

	    	    GCTRACE(3)("LMAN THREAD:  Complete! PCB = %x PARM = %x \n",
		    		pcb, pl);

		    GCcomplete_request( q );

		    q = nq;
		    processing_requests--;

	    	    GCTRACE(3)("LMAN THREAD: processed completed \n");
	    	    GCTRACE(3)("                 : total now = %d \n",
		    		processing_requests);

		} /* end if req completed */
		else
		{
		    q = q->q_next;
		}

	    } /* end for -- look for complete req */

	    /*
	    ** Do a quick, non-blocking check to see if any new requests
	    ** came in during processing.
	    */
	    GCTRACE(4)("LMAN THREAD: quick look for new reqs \n");
	    if ( WaitForSingleObject( hEventThreadInQ, 0 ) == WAIT_OBJECT_0 )
	    {
		processing_requests += GCget_incoming_reqs( Tptr, 
					   hMutexThreadInQ );
	    }
	    GCTRACE(4)("LMAN THREAD: process reqs now = %d\n",
	    		processing_requests);

            if (processing_requests && pending_requests == processing_requests)
            {
                i4 Sleeptime = 1;
                Sleep(Sleeptime);
            }

	} /* end while processing requests */

	if (In_Shutdown)
	{
		_endthreadex(0);
		return;
	}

	/*
	** we're done for now, go back to the top and sleep.
	*/
	GCTRACE(3)("LMAN THREAD: No more reqs, going back to top\n" );
	goto top;
}



/*
** Name: GClanman_exit
** Description:
**
**
*/
VOID
GClanman_exit()
{
	if ( In_Shutdown )
	    return;

	In_Shutdown = TRUE;

    memset( &Name_Ncb, 0, sizeof( NCB ) );
    Name_Ncb.ncb_command = NCBDELNAME;
    STcopy( GCc_listen_port, Name_Ncb.ncb_name );
    Name_Ncb.ncb_lana_num = lana_num;

    (void) Netbios( &Name_Ncb );
}
