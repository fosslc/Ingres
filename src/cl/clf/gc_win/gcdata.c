/*
**  Copyright (c) 1995, 2009 Ingres Corporation
*/

/*
**      GC Data
**
**  Description
**      File added to hold all GLOBALDEFs in GC facility.
**      This is for use in DLLs only.
**
LIBRARY = IMPCOMPATLIBDATA
**
**  History:
**      12-Sep-95 (fanra01)
**          Created.
**	21-apr-96 (emmag)
**	    Add is_gcc which is required to cross branch mls's change
**	    from OpenROAD driver for W95 for use here.
**	24-apr-96 (tutto01)
**	    Added DECnet support.
**	23-sep-1996 (canor01)
**	    Added additional globals from gcacl.c
**	27-jun-97 (somsa01)
**	    Moved DECnet stuff to decnet.c . These definitions will now be
**	    contained in its own DLL, to be loaded in at ingstart time.
**	31-Mar-98 (rajus01)
**	    Added is_comm_svr.
**	26-Jul-98 (rajus01)
**	    Initialized new TIMEOUT event for GClisten() requests.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	29-sep-2003 (somsa01)
**	    Added tcp_ip as a valid protocol.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to have this file as part of IMPCOMPATLIBDATA
**      26-Jan-2006 (loera01) Bug 115671
**          Added GCWINTCP_log_rem_host to optionally disable invocation of 
**          gethostbyaddr().
**      24-Aug-2006 (loera01) Bug 115671
**          Added GCTCPIP_log_rem_host to optionally disable invocation of 
**          gethostbyaddr().
**      06-Feb-2007 (Ralph Loen) SIR 117590
**          Removed GCTCPIP_log_rem_host and GCWINTCP_log_rem_host.  
**          Gethostbyaddr() and getnameinfo() are no longer invoked for 
**          network listens.
**      26-Aug-2008 (lunbr01) Bug/Sir 119985
**          Change the default protocol from "wintcp" to "tcp_ip" by
**          moving "tcp_ip" to the top of the table.
**      06-Aug-2009 (Bruce Lunsford) Sir 122426
**          Remove global hCompletion mutex...no longer needed.
**          Change GccCompleteQ from a mutex to a critical section.
*/

#include <compat.h>
#include <wsipx.h>
#include <er.h>
#include <cs.h>
#include <me.h>
#include <nm.h>
#include <st.h>
#include <qu.h>
#include <pc.h>
#include <lo.h>
#include <pm.h>
#include <tr.h>
#include <gcccl.h>
#include <gc.h>
#include "gclocal.h"
#include "gcclocal.h"

STATUS 		GCnvlspx_init(GCC_PCE * , GCC_WINSOCK_DRIVER *);
STATUS		GCnvlspx_addr( char *, char *, SOCKADDR_IPX *, char * );

STATUS 		GCwintcp_init(GCC_PCE * , GCC_WINSOCK_DRIVER *);
STATUS		GCwintcp_addr( char *, char *, struct sockaddr_in *, char * );

STATUS 		GCtcpip_init(GCC_PCE * , GCC_WINSOCK_DRIVER *);
STATUS		GCtcpip_addr(char *, char *, struct sockaddr_in *, char *);

/*
**  Data from gcacl.c
*/

GLOBALDEF HANDLE    hPurgeEvent  = NULL;

GLOBALDEF ULONG Pipe_buffer_size = 0;
GLOBALDEF BOOL  is_gcc = FALSE;

GLOBALDEF i4      GC_trace = 0;

GLOBALDEF i4      Gc_reads = 0;		/* used by TMperfstat */
GLOBALDEF i4      Gc_writes = 0;	/* used by TMperfstat */

GLOBALDEF HANDLE hAsyncEvents[NUM_EVENTS] = {NULL, NULL, NULL, NULL, NULL};
GLOBALDEF HANDLE hSignalShutdown = NULL;

GLOBALDEF   BOOL        is_gcn = FALSE;
GLOBALDEF HANDLE    SyncEvent = NULL;
GLOBALDEF HANDLE    GCshutdownEvent; 
GLOBALDEF BOOL  is_comm_svr = FALSE;


/*
**  Data from gcfcl.c
*/

GLOBALDEF VOID (*GCevent_func[NUM_EVENTS])();

/*
**  Data from gcpinit.c
*/

GLOBALDEF i4 GCCL_trace = 0;

GLOBALDEF THREAD_EVENT_LIST IIGCc_proto_threads ZERO_FILL;
GLOBALDEF CRITICAL_SECTION GccCompleteQCritSect;

/*
**  Data from gcwinsck
*/

GLOBALDEF bool WSAStartup_called = FALSE;
GLOBALDEF i4 GCWINSOCK_trace = 0;

/*
**  Data from gcwinsock2
*/

GLOBALDEF i4 GCWINSOCK2_trace = 0;

/*
**  Data from nvlspx
*/

GLOBALDEF	WS_DRIVER WS_nvlspx =
{
	GCnvlspx_init,
	GCnvlspx_addr
};

GLOBALDEF i4 GCNVLSPX_trace = 0;

/*
**  Data from win_tcp.c
*/

GLOBALDEF	WS_DRIVER WS_wintcp =
{
	GCwintcp_init,
	GCwintcp_addr
};

GLOBALDEF i4 GCWINTCP_trace = 0;

/*
**  Data from tcp_ip.c
*/

GLOBALDEF	WS_DRIVER WS_tcpip =
{
	GCtcpip_init,
	GCtcpip_addr
};

GLOBALDEF i4 GCTCPIP_trace = 0;

/*
**  Data from gccptb
**  NOTE! Keep in same order as IIGCc_prot_init/exit in gccptbl.c!
*/

GLOBALDEF	GCC_PCT		IIGCc_gcc_pct =
{
	8,
  {
    {TCPIP_ID, "II", GCwinsock2, 32768, 64, 0, 0, (PTR)&WS_tcpip, (PTR)NULL},
    {WINTCP_ID, "II", GCwinsock, 4096, 64, 0, 0, (PTR)&WS_wintcp, (PTR)NULL},
    {LANMAN_ID, "II", GClanman, 2048, 64, 0, 0, (PTR)NULL, (PTR)NULL},
    {SPX_ID, "8265", GCwinsock, 1024, 64, 0, 0, (PTR)&WS_nvlspx, (PTR)NULL},
    { "", "", 0, 0, 0, 0, 0, (PTR)NULL, (PTR)NULL}
  }
};

/*
**  Data from iilmgnbi
*/

GLOBALDEF i4 GCLANMAN_trace = 0;

