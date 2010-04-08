/*
**  Copyright (c) 1995, 2001 Ingres Corporation
*/

#include <compat.h>
#include <gc.h>
#include <pc.h>
#include <me.h>
#include <st.h>
#include "gclocal.h"
#include "gcaclw95.h"
/*
**
**  Name: GCACLW95.C - IPC implementation for GCA and GCN private CL routines
**
**  Description:
**      The routines in this file are CL since the internals will vary
**      across O.S. environments.
**
**      They implement local IPC between GCA users (clients and servers).
**
**      The routines are PRIVATE to GCA and GCN.
**      They must never be called by other
**      facilities.  
**
**	These routines are designed to abstract gcacl from the 
**	specific IPC mechanism being used. Previously the IPC was all done
**	in GCACL; however, the need to have the same code run under both 
**	Windows 95 and Windows NT required that the actual mechanism be
** 	hidden from the gcacl. The result is that the routines in the 
**	gcacl don't care whether they are using named pipes, anonymous 
**	pipes, memory mapped files etc. They simply call routines in this
**	module which take care of the actual interprocess communications.
**	This avoids unecessary complication in gcacl.
**
**	The original implementation in gcacl.c used named pipes; however,
**	Windows 95 does not permit the creation of named pipes so this 
**	implementation employs anonymous pipes. Using anon pipes complicates
**	the situaltion substantially enough to justify creating a new file. The
**	functions in this file could be very easily modified to use named pipes
**	or any other IPC mechnism without having to alter gcacl.c
**
** History:
**	31-jul-95 (mls)
**		Created.
**	01-aug-95 (leighb)
**		Removed "Broken Pipe" SIprintf message.
**      01-aug-95 (mls)
**		Fixed connection counting.
**	02-aug-95 (mls)
**	    IIReadFile makes sure that asipc, and asipc->IpcInternal are valid
**	    before checking for broken pipes.
**	16-oct-95 (mls)
**	    Added code to make sure that gcc waits for client to close 
**	    connection first. Bug 71896.
**      06-Feb-96 (fanra01)
**              Changed Pipe_buffer_size from extern to GLOBALREF.
** 	06-Feb-96 (wonst02)
** 		Changed CreateAsc to create hMapping as inherited and to save 
**	    asipc->IpcInternal.hMapping for IPCsave and IPCrestore.
**      27-may-97 (mcgem01)
**          Cleaned up compiler warnings.
**	14-oct-1997 (canor01)
**	    Before closing all pipes in a connection, write a zero-length 
**	    message on receive pipes to wake up any pending receives in 
**	    helper threads.
**	06-apr-1998 (canor01)
**	    In OpenAsc(): Clean up handles on failure.  If shared memory 
**	    still exists, make sure the owning process is still alive.
**      10-Dec-98 (lunbr01) Bug 94183
**          Use PeekNamedPipe rather than shared memory counter (*ct) to check
**          if data is avail to read (for RECEIVES with a non-INFINITE
**          timeout parm only). The shared memory counter was occasionally
**          getting corrupted because IIReadFile and IIWriteFile would update
**          it at the same time; InterlockedExchange was not sufficient...
**          a mutex was needed before the current value of *ct is "gotten"
**          for the increment/decrement.  The result was that IIReadFile
**          would "think" that there was something to read but there wasn't;
**          he would then issue the blocked read and wait forever or until
**          a dbevent happened to come in. The symptom was an application hang.
**          Rather than adding mutexes, PeekNamedPipe was used and the 
**          shared memory counters eliminated.
**	08-feb-1999 (somsa01 for lunbr01)
**	    Save off the read ends of the SndStdChan pipes as we'll need
**	    to peek on this to ensure that the purge message doesn't
**	    get mangled. Duplicate the read end of the pipe right after
**	    the pipe is created because it will get closed after a front
**	    end client get ahold of it (DuplicateHandle) from the
**	    shared memory. Close the handle when we disassociate.
**	    These changes were taken from OI Desktop changes in March 97
**	    by mcgem01 and zhayu01.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	08-feb-2001 (somsa01)
**	    Use INVALID_HANDLE_VALUE rather than 0xFFFFFFFF when calling
**	    CreateFileMapping().
*/

GLOBALREF ULONG Pipe_buffer_size;
SERVER_ENTRY	*server_list = NULL; 
i4 		nServers = 0;
GLOBALREF BOOL 	is_gcc;

HANDLE		RequestingEvent;
HANDLE		ListeningEvent;

SERVER_ENTRY 	*AddServer(char *, LPVOID, HANDLE, HANDLE, HANDLE );
VOID		RemoveServer( SERVER_ENTRY * );
SERVER_ENTRY 	*LookUpServer( char * );

/* 
** Structure for memory mapped file that contains shared data 
*/

struct _Asc_ipc_area 
{
	DWORD		nProcId;
	ASSOC_IPC	asipc;	/* temporary for establishing assocs. */
	ASSOC_INFO	assoc_info[MAX_ASSOCS];
};

/*
** CreateAsc( ) --- Create an association.
**
** Inputs:
**	asipc	ASSOC_IPC for association
**	name	name of server creating association.
** Output:
**	returns TRUE if succeeds.
**
** GClisten( ) calls CreateAsc( ) when a server wishes to create
** a new association. CrateAsc( ) allocates the resources required by 
** the association, and exposes shared information available to other processes.
** The only requirements are that it create four handles in the ASSOC_IPC:
** standard send, standard receive, expedited send, and expeditied receive.
** These handles are the only members created here used by gcacl. In 
** this implementation handles to anonymous pipes are used.
**
** If there are no existing connections to the server spicified in the 
** name parameter, CreateAsc( ) creates the server specific resources and
** adds the server to the internally maintained list of servers. A memory mapped
** mapped file is created for passing information between the client and 
** server. This file contains a temporary area for passing process id and
** handles which the client needs to copy to its own ASSOC_IPC. It also 
** contains and array of assoc_info structures. These structures contain
** information that must be shared per connection for the duration of each 
** association. A mutex is created to protect access to the temporary area.
** The temporary area is only used when a connectionis first established
** to copy handles to the pipes and other information to the requester's
** ASSOC_IPC.
**
** Two events are also created to arbitrate the connection process. When
** a server is ready for a connection, it signals a "listening event". 
** and waits for a client to set a requesting event. Before connecting 
** a client waits on the listening event before trying to connect. When
** the client tries to connect it sets a requesting event to tell the server
** that a client has connected.
**
** The names for the memory mapped file, the mutex, and the events are
** based on the server name.
**
** History:
**      10-Dec-98 (lunbr01) Bug 94183
**          Eliminate use of *Ct fields (using PeekNamedPipe instead).
*/
BOOL 	 
CreateAsc(ASSOC_IPC * asipc, char *name )
{
	HANDLE 		hMapping; /* for memory mapped file */
	static HANDLE	hMutex;	  /* for memory mapped file */
	ASSOC_IPC	*asipc_temp;
	ASSOC_INFO	*assoc_info;
	struct _Asc_ipc_area	*svr_ipc_area;
	SERVER_ENTRY	*server;
	short		i;
															
	SECURITY_ATTRIBUTES	sa;
	
	/* Set up security attributes */		
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	
	/* If there are no existing connections to this server. Open the
	** memory mapped file, events, and mutex for this server. 
	*/
	if( NULL == ( server = LookUpServer( name ) ) )	
	{
		char ReqEventName[MAXPNAME + 4 ];
		char LstEventName[MAXPNAME + 4 ];
		
		if ((hMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
				&sa,
				PAGE_READWRITE,
				0,
				sizeof( struct _Asc_ipc_area ),  
				name )) == NULL)
		{
			DWORD err = GetLastError( );
			return FALSE ;
		}
		
		if (NULL == (svr_ipc_area=
			(struct _Asc_ipc_area *)MapViewOfFile(hMapping,
						FILE_MAP_ALL_ACCESS,
						0,
						0,
						0 ) ) )
		{
			return FALSE;
		}
		
		/* Create an event that will be signaled when an 
		** association is requested.
		*/
		wsprintf( ReqEventName, "%s-%s", name, "req" );
		wsprintf( LstEventName, "%s-%s", name, "lst" );
		
		/* Create SERVER_ENTRY */
		server = AddServer( name,  svr_ipc_area, hMapping, 
					CreateEvent( &sa, FALSE, FALSE, 
					ReqEventName ), 
					CreateEvent( &sa, FALSE, FALSE, 
					LstEventName ) );
		
		/* Create a mutex to coordinate access to asipc_temp */
		hMutex = CreateMutex( &sa, FALSE, name );
		
		/* Create a proces handle so other processes can 
		   use DuplicateHandle */
		svr_ipc_area->nProcId =  GetCurrentProcessId( );
	}
	
	/* Get a pointer to the Memory mapped file for this server. */
	svr_ipc_area = server->lpMapping;
	
	if ( server->nConnections == MAX_ASSOCS ) /* no more assocs allowed */
		return FALSE ;
		
	asipc_temp = &( svr_ipc_area->asipc );
	assoc_info = (ASSOC_INFO *)&( svr_ipc_area->assoc_info );

	/* Make sure that it is safe to change temporary connection info.  
	** Copy the connection information the local ASSOC_IPC. 
	*/
	WaitForSingleObject( hMutex, INFINITE );	
	
	/* Find a valid id and set up the assoc_info. */
	for( i = 0; i < MAX_ASSOCS; i++ )
	{
		if ( assoc_info[i].refs == 0 )
			break;
	}
	asipc->IpcInternal.hMapping 	= hMapping; /* for save and restore */
	asipc->IpcInternal.AssocInfo	= &assoc_info[i];
	asipc_temp->IpcInternal.AscID	= asipc->IpcInternal.AscID = i;
	assoc_info[i].bPipesClosed = FALSE;
	assoc_info[i].refs++;

	/* Set the AssocInfo */

	assoc_info[i].out_buffer_size = Pipe_buffer_size;
	assoc_info[i].in_buffer_size = Pipe_buffer_size;

	if(!CreatePipe( &asipc->RcvStdChan, &asipc_temp->SndStdChan, &sa, 
				Pipe_buffer_size ) ||
	   !CreatePipe( &asipc_temp->RcvStdChan, &asipc->SndStdChan, &sa, 
				Pipe_buffer_size ) ||
	   !CreatePipe( &asipc->RcvExpChan, &asipc_temp->SndExpChan, &sa, 
				Pipe_buffer_size ) ||
	   !CreatePipe( &asipc_temp->RcvExpChan, &asipc->SndExpChan, &sa, 
				Pipe_buffer_size ) ) 
	{
		CloseAsc( asipc );
		CloseAsc( asipc_temp );
		return FALSE ;
	}

        /* We need to duplicate the client side of standard read handle
           asipc_temp->RcvStdChan for PeekNamedPipe() because it gets closed
           by the DUPLICATE_CLOSE_SOURCE option of DuplicateHandle() after a
           front end client connects to the server and duplicates it into
           their address space. */
        if (!DuplicateHandle(
                       GetCurrentProcess(),
                       asipc_temp->RcvStdChan,
                       GetCurrentProcess(),
                       &asipc->ClientReadHandle,
                       GENERIC_READ,
                       TRUE,
                       DUPLICATE_SAME_ACCESS
                       ) )
        {
                        CloseAsc( asipc );
                        CloseAsc( asipc_temp );
                        return FALSE ;
        }

        /* We need to duplicate the server side of standard read handle
           asipc->RcvStdChan for PeekNamedPipe() in the client because it
           gets closed by the DUPLICATE_CLOSE_SOURCE option of
           DuplicateHandle() after a front end client connects to the server
           and duplicates it into their address space. */
        if (!DuplicateHandle(
                       GetCurrentProcess(),
                       asipc->RcvStdChan,
                       GetCurrentProcess(),
                       &asipc_temp->ClientReadHandle,
                       GENERIC_READ,
                       TRUE,
                       DUPLICATE_SAME_ACCESS
                       ) )
        {
                        CloseAsc( asipc );
                        CloseAsc( asipc_temp );
                        return FALSE ;
        }

	ReleaseMutex( hMutex );

	/* 
	** Set up pointer to server and increment the number of connections.	
	*/
	asipc->IpcInternal.server = server;
	asipc->IpcInternal.server->nConnections++;
		
	return TRUE;
}

/*
** OpenAsc() --- Open an association to a named server
**
** Inputs:
**	asipc	ASSOC_IPC for association.
**	name	Name of the server to which to connect.
**	timeout	Time in milliseconds to wait for a connection or INFINITE.
** Returns:
**	OK		If connection successful
**	~OK or GC_???	If connection fails.
**		
** OpenAsc( ) is the client side equivalent to CreateAsc( ). If the client
** has no exising connections to the named server, a SERVER_ENTRY 
** is created for the server. 	OpenAsc opens handles to the Events, 
** and Memmory Mapped file created by CreateAsc( ). 
**		
** It then duplicates  handles to the pipess, and sets up the Asipc using
** the information contained in the memory mapped file's temporary asipc.
**
** History:
**	06-apr-1998 (canor01)
**	    Clean up handles on failure.  If shared memory still exists, make
**	    sure the owning process is still alive.
*/	
	
STATUS 	 
OpenAsc(ASSOC_IPC * asipc, char *name, DWORD timeout )
{
	HANDLE			hMapping;
	HANDLE			hMutex;
	HANDLE			hSrcProc;
	struct _Asc_ipc_area	*cli_ipc_area;
	SERVER_ENTRY		*server;

	/* 
	** If there are no connections to this server, set up a 
	** SERVER_ENTRY and open the required resources.	
	*/
	if ( NULL == ( server = LookUpServer( name) ) )
	{
	    char ReqEventName[MAXPNAME + 4 ];/*Event for requesting connection*/
	    char LstEventName[MAXPNAME + 4 ];/*Event to see server is lstning */
	    HANDLE revent, levent;
		
	    /* Create an event that will be signaled when an 
	    ** association is requested. */
	    wsprintf( ReqEventName, "%s-%s", name, "req" );
	    wsprintf( LstEventName, "%s-%s", name, "lst" );
		
	    if( NULL == (revent = OpenEvent( EVENT_ALL_ACCESS, 
						FALSE, ReqEventName ) ) )
		return ~OK;
			
	    if( NULL == (levent = OpenEvent( EVENT_ALL_ACCESS, 
						FALSE, LstEventName ) ) )
	    {
		CloseHandle( revent );
		return ~OK;
	    }
			
	    if( NULL == (hMapping = OpenFileMapping( FILE_MAP_ALL_ACCESS, 
						TRUE, name ) ) )
	    {
		CloseHandle( levent );
		CloseHandle( revent );
		return ~OK;
	    }
			
	    if ( NULL == ( cli_ipc_area = (struct _Asc_ipc_area *)
					MapViewOfFile(	hMapping,
					  	FILE_MAP_ALL_ACCESS,
		  				0,
		  				0,
		  				0 ) ) )
	    {
		CloseHandle( hMapping );
		CloseHandle( levent );
		CloseHandle( revent );
		return~OK;
	    }
	
	    if ( PCis_alive( cli_ipc_area->nProcId ) == FALSE )
	    {
		UnmapViewOfFile( hMapping );
		CloseHandle( hMapping );
		CloseHandle( levent );
		CloseHandle( revent );
		return ~OK;
	    }
		
		
	    /* Create SERVER_ENTRY */
	    server = AddServer( name,  cli_ipc_area, hMapping, revent, levent );
	}

	/* Get pointer to memory mapped file with shared data.	*/
	cli_ipc_area = server->lpMapping;
	
	/* wait for a server to listen. */
	if( WAIT_TIMEOUT == WaitForSingleObject( server->ListeningEvent, 
		timeout == GC_WAIT_FOREVER ? INFINITE : timeout ) )
	{
		return GC_TIME_OUT;
	}

	/*
	** Setup the ASSOC_IPC
	*/	
	hMutex = OpenMutex( MUTEX_ALL_ACCESS, TRUE, name );
	WaitForSingleObject( hMutex, INFINITE );
	
	asipc->IpcInternal.hMapping = server->hMapping;	/*for save and restore*/
	asipc->IpcInternal.AscID = cli_ipc_area->asipc.IpcInternal.AscID;
	asipc->IpcInternal.AssocInfo = &cli_ipc_area->assoc_info[asipc->IpcInternal.AscID];
	/* Increment Reference Count for AssocInfo */
	asipc->IpcInternal.AssocInfo->refs++;
       
	hSrcProc = OpenProcess( PROCESS_DUP_HANDLE, TRUE, cli_ipc_area->nProcId );

	/* Get the pipe handles. */
	if (!DuplicateHandle( hSrcProc,
				cli_ipc_area->asipc.SndStdChan,
				GetCurrentProcess( ),
				&asipc->SndStdChan,
				GENERIC_READ | GENERIC_WRITE,
				TRUE,
				DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS ) )
	{
		CloseAsc( asipc );
		return ~OK;

	}
	
	if (!DuplicateHandle( hSrcProc, 
				cli_ipc_area->asipc.RcvStdChan,
				GetCurrentProcess( ),
				&asipc->RcvStdChan,
				GENERIC_READ | GENERIC_WRITE,
				TRUE,
				DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS) )
	{
		CloseAsc( asipc );
		return ~OK;
	}
	
	if (!DuplicateHandle( hSrcProc, 
				cli_ipc_area->asipc.SndExpChan,
				GetCurrentProcess( ),
				&asipc->SndExpChan,
				GENERIC_READ | GENERIC_WRITE,
				TRUE,
				DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS ) )
	{
		CloseAsc( asipc );
		return ~OK;
	}
	
	if (!DuplicateHandle( hSrcProc, 
				cli_ipc_area->asipc.RcvExpChan,
				GetCurrentProcess( ),
				&asipc->RcvExpChan,
				GENERIC_READ | GENERIC_WRITE,
				TRUE,
				DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS ) ) 
	{
		CloseAsc( asipc );
		return ~OK;
	}

        if (!DuplicateHandle( hSrcProc,
                                cli_ipc_area->asipc.ClientReadHandle,
                                GetCurrentProcess( ),
                                &asipc->ClientReadHandle,
                                GENERIC_READ | GENERIC_WRITE,
                                TRUE,
                                DUPLICATE_CLOSE_SOURCE) )
        {
                CloseAsc( asipc );
                return ~OK;
        }
	
	/* Set the handles in the temporary area to NULL before 
	** releaseing mutex so no one else steals them.	
	*/
	cli_ipc_area->asipc.SndStdChan = NULL;
	cli_ipc_area->asipc.RcvStdChan = NULL;
	cli_ipc_area->asipc.SndExpChan = NULL;
	cli_ipc_area->asipc.RcvExpChan = NULL;
	cli_ipc_area->asipc.ClientReadHandle = NULL;

	/* Point AssocInfo to propper structure in shared memory. */
	asipc->IpcInternal.AssocInfo = &( cli_ipc_area->assoc_info[asipc->IpcInternal.AscID] );
	
        /* Set flag to indicate Pipe is open */
        asipc->IpcInternal.AssocInfo->bPipesClosed = FALSE;

	ReleaseMutex( hMutex );
	CloseHandle( hMutex );

	/* Tell the server that we are ready to request a connection. */
	SetEvent( server->RequestingEvent );
	
	server->nConnections++;
	asipc->IpcInternal.server = server;
								  
	return OK;
}
	
/*
** CloseAsc --- Close association.
**		
** Inputs: 
**	asipc	ASSOC_IPC for connection.
** Returns:
**	none.
**
** CloseAsc( ) closes the association for a connection. If it is the last
** connection to a partricular server it removes that server from the list
** of servers.
**
** History:
**	14-oct-1997 (canor01)
**	    Before closing all pipes in a connection, write a zero-length 
**	    message on receive pipes to wake up any pending receives in 
*/

VOID
CloseAsc(ASSOC_IPC * asipc)
{
        int i = 0;

	/*
	** If we are a gcc let the client disconnect first. If ten seconds 
	** elapse then disconnect anyways.
	*/ 
	while( is_gcc && asipc->IpcInternal.AssocInfo->bPipesClosed == FALSE 
			&& i++ < 200 )
		PCsleep(50);

	if (asipc->SndStdChan)
        CloseHandle(asipc->SndStdChan);
	if (asipc->RcvStdChan)
	{
        WriteFile(asipc->RcvStdChan, "", 1, &i, NULL);
        Sleep(0);
        CloseHandle(asipc->RcvStdChan);
	}
	if (asipc->SndExpChan)
        CloseHandle(asipc->SndExpChan);
	if (asipc->RcvExpChan)
	{
        WriteFile(asipc->RcvExpChan, "", 1, &i, NULL);
        Sleep(0);
        CloseHandle(asipc->RcvExpChan);
	}
	if (asipc->ClientReadHandle)
	    CloseHandle(asipc->ClientReadHandle);
	
	/* Mark pipes as being closed. */
	if ( asipc->IpcInternal.AssocInfo )
	{
	    asipc->IpcInternal.AssocInfo->bPipesClosed = TRUE;
	    asipc->IpcInternal.AssocInfo->refs--; /* decrement reference ct */ 
	    asipc->IpcInternal.AssocInfo = NULL; 
	}
	
	asipc->SndStdChan = NULL;
	asipc->RcvStdChan = NULL;
	asipc->SndExpChan = NULL;
	asipc->RcvExpChan = NULL;
	asipc->ClientReadHandle = NULL;
	
	if ( asipc->IpcInternal.server )	
	{
		asipc->IpcInternal.server->nConnections--;
		if ( 0 == asipc->IpcInternal.server->nConnections )
			 RemoveServer( asipc->IpcInternal.server );
	}

	return;
}
/*
** GetAscInfo --- Returns pipe sizes.
**		
** Inputs:
**	asipc	ASSOC_IPC for connection.
**
** Outputs:
**	outbuf_size		size of send channels.
**	inbuf_size		size of receive channels.
**
** This function serves as a substitute for GetNamedPipeInfo that was
** previously used in gcacl.
*/

VOID    
GetAscInfo(ASSOC_IPC *asipc, i4  *outbuf_size, i4  *inbuf_size )
{
	*outbuf_size = asipc->IpcInternal.AssocInfo->out_buffer_size;
	*inbuf_size = asipc->IpcInternal.AssocInfo->in_buffer_size;
	return;
}

/* DetectServer --- determines if the specified server exists.
**
** Inputs:
**	name	Name of server to be detected.
** Returns:
**	TRUE	If server is successfully detected.
**
** This function is used by GCdetect to see if there is a server. IF
** we were using named pipes we would use WaitNamedPipe; however, since
** we are not using named pipes we simply check to see if the MemoryMappedFile
** for the server exists.
*/
BOOL	
DetectServer( char *name )
{
	HANDLE	hMapping;
	if( NULL == ( hMapping = OpenFileMapping( FILE_MAP_ALL_ACCESS, 
							TRUE, name ) ) )
		return FALSE;
	CloseHandle( hMapping );
	return TRUE;
}
/* IIReadfile --- reads data from specified channel
** Inputs:
**	asipc		ASSOC_IPC for connection.
**	hChan		Handle to channel from which to read.
**	nNumberToRead	number of bytes to read.
**	lpNUmberRead	number of bytes actually read.
**	LPOVERLAPPED	for compatiblity with win32 ReadFile.
**	timeout		timeout.
**			
** IIReadfile attempts to read from the pipe. It returns OK if the read succeeds
** ~OK if it fails, or GC_TIME_OUT if it times out.
**
** IIReadfile and IIWritefile must maintain a count of how many bytes are in
** the channel in order to implement the timeout functionality because anonymous
** pipes don't provide a wait function like named pipes. Database events 
** require the timeout functionality.
**
** History:
**      04-Sep-97 (fanra01)
**          Add return of OS error if read 0 bytes.
**      10-Dec-98 (lunbr01) Bug 94183
**          Use PeekNamedPipe rather than shared memory counter (*ct) to check
**          if data is avail to read (for RECEIVES with a non-INFINITE
**          timeout parm only). The shared memory counter was occasionally
**          getting corrupted because IIReadFile and IIWriteFile would update
**          it at the same time; InterlockedExchange was not sufficient...
**          a mutex was needed before the current value of *ct is "gotten"
**          for the increment/decrement.  The result was that IIReadFile
**          would "think" that there was something to read but there wasn't;
**          he would then issue the blocked read and wait forever or until
**          a dbevent happened to come in. The symptom was an application hang.
**          Rather than adding mutexes, PeekNamedPipe was used and the 
**          shared memory counters eliminated.
*/
ULONG	
IIReadFile( ASSOC_IPC *asipc,
	    HANDLE hChan, 
	    LPVOID lpBuffer, 
	    DWORD nNumberToRead, 
	    LPDWORD lpNumberRead, 
	    LPOVERLAPPED lpOverlapped, 
	    DWORD timeout )
{
	DWORD ct = 0;   
	int i = 0;
	int intervals = 0;
	BOOL rval = FALSE;
	BOOL tout = TRUE;
        DWORD dwError = 0;

	timeout = timeout == GC_WAIT_FOREVER ? INFINITE : timeout; 

	*lpNumberRead = 0;
	
	if ( timeout != INFINITE )
	{
	    rval = PeekNamedPipe(hChan, NULL, 0, NULL, &ct, NULL);
	    if ( rval != TRUE )
	    {
	        dwError = GetLastError();
		return rval ? OK : dwError;
	    }
	    if ( ct == 0 )
	    {
		if ( timeout == 0 )
			return GC_TIME_OUT;
		
		intervals = timeout == INFINITE ? 1:timeout/IPC_TIMEOUT_SLEEP ;	
			
		for( i = 0; i < intervals ; i++ )
		{
			PCsleep ( IPC_TIMEOUT_SLEEP );
	    		rval = PeekNamedPipe(hChan, NULL, 0, NULL, &ct, NULL);
	    		if ( rval != TRUE )
	    		{
	       			dwError = GetLastError();
				return rval ? OK : dwError;
	    		}
			if ( ct != 0 )
			{
				tout = FALSE;
				break;
			}
		}
		if ( tout )
			return GC_TIME_OUT;
			
	    } /* end if ct (bytes in read pipe) == 0 */
	}
	rval = ReadFile( hChan, lpBuffer, nNumberToRead, 
			 	lpNumberRead, lpOverlapped );
        dwError = GetLastError();
        if (rval)
        {               /* did successful data retrieval */  
            if (asipc)
            {
                /* set bPipesClosed to false - may have been set to TRUE 
                ** by the 'Server'.
	        */
                if (asipc->IpcInternal.AssocInfo)
                    asipc->IpcInternal.AssocInfo->bPipesClosed = FALSE;
            }
        }               /* did successful data retrieval */
	if( rval && 
	    (!asipc || !asipc->IpcInternal.AssocInfo ||
	    asipc->IpcInternal.AssocInfo->bPipesClosed) ) 
	{
		return ERROR_BROKEN_PIPE;
	}
	return rval ? OK : dwError;
	
}	

/* IIWritefile --- writes data to specified channel
**
** Inputs:
**	asipc		ASSOC_IPC fro connection.
**	hChan		Handle to channel from which to read.
**	nNumberToWrite	number of bytes to write.
**	lpNUmberRead	number of bytes actually written.
**	LPOVERLAPPED	for compatiblity with win32 WriteFile.
**			
**	Writes to specified channel.
**
** History:
**      04-Sep-97 (fanra01)
**          Modified return type.
**      10-Dec-98 (lunbr01)
**          Eliminate use (increment) of shared memory counter (*ct).
*/
ULONG
IIWriteFile( ASSOC_IPC *asipc, HANDLE hChan, LPVOID lpBuffer, 
			DWORD nNumberToWrite, LPDWORD	lpNumberWritten, 
			LPOVERLAPPED lpOverlapped )
{
	BOOL bRval;
	STATUS status=OK;

	bRval=WriteFile( hChan, lpBuffer, nNumberToWrite, lpNumberWritten, 
				lpOverlapped );

	if (!bRval)
		status = GetLastError();

	return status;
	
}
/* AscWait --- called by server to wait for an association.
**
** Input:
**	asipc	ASSOC_IPC for association.
**	tiemout	timeout.
** Returns:
**	WAIT_TIMEOUT if timeout occurs.
**
*/	
DWORD	
AscWait( ASSOC_IPC *asipc, int timeout )
{
	/* Tell the clients that we're ready for an association */
	SetEvent( asipc->IpcInternal.server->ListeningEvent ); 
	
	/* Now wait for a client to request an association.  */
	return WaitForSingleObject( asipc->IpcInternal.server->RequestingEvent, 
				timeout == GC_WAIT_FOREVER ? INFINITE :
				timeout );
}
/*
** IPCsave and IPCrestore
**
** Inputs:
**	asipc	ASSOC_IPC for connection to save or restore.
**	buff	ULONG * to store information to or restore information from.
** Returns:
**	Number of ULONGs used to store information.
**
**	IPCsave and IPCrestore are called by their counterparts in gcacl to
** 	save IPC specific information regarding an association. The counterparts
**	in gcacl pass the unused portion of their buffers to these functions.
**		
*/	
i4 		IPCsave( ASSOC_IPC *asipc, ULONG *buff )
{
	buff[0] = (ULONG)asipc->IpcInternal.AscID;
	buff[1] = (ULONG)asipc->IpcInternal.hMapping;
	return 2 * sizeof( ULONG );
}

BOOL 
IPCrestore( ASSOC_IPC *asipc, ULONG *buff )
{
	struct _Asc_ipc_area	*ipc_area;
	asipc->IpcInternal.AscID = buff[0];
	asipc->IpcInternal.hMapping = (HANDLE)buff[1];
	
	/* hMapping is inheritable so no need for DuplicateHandle */
	if ( NULL == ( ipc_area = ( struct _Asc_ipc_area *)MapViewOfFile(	
						asipc->IpcInternal.hMapping,
						FILE_MAP_ALL_ACCESS,
						0,
						0,
						0 ) ) )
	{
		DWORD err = GetLastError( );
		return FALSE;													  
	}									

	asipc->IpcInternal.AssocInfo = 
		&( ipc_area->assoc_info[asipc->IpcInternal.AscID] );

	return TRUE;
}		

/*
** The following functions are used internally to maintain a list of servers
** that are currently being used by a process. This avoids mapping files and
** opening events multiple times.
**
** These functions should not be called outsied of this module.
*/

static 
SERVER_ENTRY *
LookUpServer( char *name )
{
	SERVER_ENTRY	*cur = server_list;
	int i = 0;
	
	while ( i++ < nServers)
	{
		if( !STcompare( name, cur->name ) )
			return cur;
		cur = cur->next;
	}
	return NULL; /* no match */
}	

static 
SERVER_ENTRY *
AddServer( char *name, LPVOID lpMapping, HANDLE hMapping,
	HANDLE	levent, HANDLE revent )
{
	SERVER_ENTRY *server;
	
	server = (SERVER_ENTRY *) MEreqmem(0, sizeof( SERVER_ENTRY ), 
						TRUE, NULL );
	server->name = ( char * ) MEreqmem(0, strlen( name ) + 1, 
						TRUE, NULL );
	
	strcpy( server->name, name );
	
	server->lpMapping = lpMapping;
	server->hMapping = hMapping;
	server->RequestingEvent = revent;
	server->ListeningEvent = levent;
	server->nConnections = 0;
	
	
	/* add to server list */
	server->next = server_list;
	server_list = server;
	nServers++;
	
	return server;
}	

static 
VOID	
RemoveServer( SERVER_ENTRY *target )
{
	SERVER_ENTRY *cur, *prev;

	/* The last server in the list is always the name server which never
	** gets removed.
	*/
	if ( NULL == target->next )
		return;
	
	cur = server_list;
	prev = NULL;
	
	while( cur != target )
	{
		prev = cur;
		cur = cur->next;
	}
	if ( prev )
		prev->next = cur->next;
	else
		server_list = cur->next;
		
	UnmapViewOfFile( cur->lpMapping );
	CloseHandle( cur->hMapping );
	CloseHandle( cur->RequestingEvent );
	CloseHandle( cur->ListeningEvent );
	
	if( cur->name )
		MEfree( cur->name );
	
	MEfree( (char *) cur );
	nServers--;
	
}	
