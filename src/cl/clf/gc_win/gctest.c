#include <compat.h>
#include <systypes.h>
#include <clconfig.h>
#include <stdio.h>
#include <st.h>
#include <tr.h>
#include <gc.h>
#include <me.h>
#include <pc.h>
#include <errno.h>


#define NUM_PACKETS   40
#define MIN_PACKETS   4
#define PACKET_SIZE   4096
#define MAXPATHNAME   1024
#define TIMEOUT       -1
#define SERVER        0
#define CLIENT        1
#define TRIES         4
#define N_CLIENTS     4
#define TEST_DURATION 10
#define NUM_SIGNALS   100

/******************************************************************************
** Test for the UNIX 6.4 GCA/CL interface. Starts up a
** server and a variable number of clients that establish a connection
** and exchange messages (default 4K)  with each other.
**
** Option also exists to send SIGUSR2's to the server to test
** correct operation under signals, and test operation with
** premature disconnection by the client.
**
** Usage:
** $ gctest
**    starts up a default 4  clients
**
** $ gctest -help
**    prints out options
**
PROGRAM = gctest
**
NEEDLIBS = COMPAT 
** History:
**	25-jun-95 (emmag)
**	   Replaced error with errnum.
**	27-nov-1995 (canor01)
**	   free() does not return a value, so move it out of return statement.
**      27-may-97 (mcgem01)
**          Cleaned up compiler warnings.
**      03-Apr-98 (rajus01)
**          Removed references to GCA_ACB.
**	06-Jul-04 (drivi01)
**	    Added PROGRAM and NEEDLIBS for new build.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**
******************************************************************************/

/******************************************************************************
** local routines
******************************************************************************/
static PTR      GC_alloc();
static STATUS   GC_free();
static VOID	InitCON();
static VOID     usage();
static VOID     fill_write_packet();
static bool     verify_read_packet();
static VOID     reg_server();
DWORD WINAPI    do_server(LPVOID);
DWORD WINAPI    do_client(LPVOID);
static VOID     listen_callback();
static VOID     connect_callback();
static VOID     send_packet();
static VOID     send_callback();
static VOID     close_socket();
static VOID     close_callback();
static VOID     receive_packet();
static VOID     receive_callback();
static VOID	do_test();


/******************************************************************************
** local data
******************************************************************************/
typedef struct _conn_data {
    /* connection control block */
    PTR     	gc_cb;
    char    	*write_packet;
    char    	*read_packet;
    int     	n_read_packets;
    int     	n_write_packets;
    int     	n_bytes_read;
    bool    	recv_close_conn;	/* state machine done receiving */
    bool    	send_close_conn;	/* state machine done sending */
    SVC_PARMS	send_parms;
    CL_SYS_ERR	send_sys_err;
    SVC_PARMS	recv_parms;
    CL_SYS_ERR	recv_sys_err;
    SVC_PARMS	conn_parms;
    CL_SYS_ERR	conn_sys_err;
} CONN_DATA;

static CONN_DATA  *conn_data_blks;
static CL_SYS_ERR syserr;
static char       listen_port[MAXPATHNAME];
static char       *write_packet;
static int        num_packets          = NUM_PACKETS;
static int        num_signals          = NUM_SIGNALS;
static bool       verbose_it           = FALSE;
static int        timeout              = TIMEOUT;
static int        packet_size          = PACKET_SIZE;
static int        read_size            = PACKET_SIZE;
static int        write_size           = PACKET_SIZE;
static bool       signal_it            = FALSE;
static bool       error_it             = FALSE;
static int        exit_stat            = OK;
static int        reps                 = 0;
static int        tries                = TRIES;
static int        n_clients            = N_CLIENTS;
static int        num_open_connections = N_CLIENTS;
static int        num_lisns_posted     = 0;
static bool       client_dbg           = FALSE;
static bool       server_dbg           = FALSE;
static bool       server_only          = FALSE;
static bool       client_only          = FALSE;

__declspec (thread) DWORD dwType;
__declspec (thread) DWORD dwPid;

/******************************************************************************
** generic error routine - ignores SIGPIPE errors
******************************************************************************/
static VOID
error(str, svc_parms)
	char           *str;
	SVC_PARMS      *svc_parms;
{
	CONN_DATA *conn_data = (CONN_DATA *) svc_parms->closure;

	if ((error_it == FALSE) ||
	    (verbose_it == TRUE)) {
		fprintf(stderr,
		        "%d:\t%s error in %s: SYSERROR:%d;STATUS: 0x%x\n",
		        dwPid,
		        (dwType ? "Client" : "Server"),
		        str,
		        svc_parms->sys_err->errnum,
		        svc_parms->status);
		fprintf(stderr,
		        "\tn_read_pkts=%d,n_write_pkts=%d,reps=%d\n",
		        conn_data->n_read_packets,
		        conn_data->n_write_packets,
		        reps);
	}
	if (error_it == FALSE)
		exit(3);
}

/******************************************************************************
** Memory allocator functions
******************************************************************************/
static PTR
GC_alloc(int size)
{
	PTR    ptr;

	ptr = (PTR) malloc (size);
	if (ptr == NULL) {
		perror("malloc");
		PCexit(FAIL);
	}
	return (ptr);
}

static          STATUS
GC_free(ptr)
	PTR             ptr;
{
	free(ptr);
	return (OK);
}

/******************************************************************************
**
******************************************************************************/
static VOID
usage(name)
	char           *name;
{
	fprintf(stderr, "%s: usage:\n", name);
	fprintf(stderr, "\t[-n number_of_packets]\n");
	fprintf(stderr, "\t[-v(erbose)]\n");
	fprintf(stderr, "\t[-p port]\n");
	fprintf(stderr, "\t[-i receive_msg_size]\n");
	fprintf(stderr, "\t[-o send_msg_size (< receive_msg_size)]\n");
	fprintf(stderr, "\t[-b msg_packet_size]\n");
	fprintf(stderr, "\t[-c num_of_clients ]\n");
	fprintf(stderr, "\t[-client_only | server_only ]\n");
	fprintf(stderr, "\t[-errorit ( test premature disassociation)]\n");
	fprintf(stderr, "\t[-iterations num_of_iterations ]\n");
	fprintf(stderr, "\t[-x num_signals (num_of SIGUSR2's to server)]\n");
}

/******************************************************************************
** test the GC interface
******************************************************************************/
main(argc, argv)
	int  argc;
	char *argv[];
{
	int    n     = 0;
	char   *port = NULL;

	MEadvise(ME_INGRES_ALLOC);

	TRset_file(TR_T_OPEN, "stdio", 5, &syserr);

	while (++n < argc) {
		if (STcompare(argv[n], "-n") == 0)
			num_packets = atoi(argv[++n]);
		else if (STcompare(argv[n], "-v") == 0)
			verbose_it = TRUE;
		else if (STcompare(argv[n], "-p") == 0)
			port = argv[++n];
		else if (STcompare(argv[n], "-i") == 0)
			read_size = atoi(argv[++n]);
		else if (STcompare(argv[n], "-o") == 0)
			write_size = atoi(argv[++n]);
		else if (STcompare(argv[n], "-b") == 0)
			packet_size = atoi(argv[++n]);
		else if (STcompare(argv[n], "-iterations") == 0)
			tries = atoi(argv[++n]);
		else if (STcompare(argv[n], "-c") == 0)
			n_clients = atoi(argv[++n]);
		else if (STcompare(argv[n], "-errorit") == 0)
			error_it = TRUE;
		else if (STcompare(argv[n], "-server_only") == 0)
			server_only = TRUE;
		else if (STcompare(argv[n], "-client_only") == 0)
			client_only = TRUE;
		else if (STcompare(argv[n], "-x") == 0) {
			signal_it = TRUE;
			num_signals = atoi(argv[++n]);
		} else {
			usage(argv[0]);
			PCexit(FAIL);
		}
	}
	
	/*
	 * set read/write/num_packet sizes
	 */

	if (packet_size != PACKET_SIZE)
		read_size = write_size = packet_size;
	if (read_size < write_size)
		read_size = write_size;
	if (num_packets < MIN_PACKETS)
		num_packets = MIN_PACKETS;

	if (server_only &&
	    client_only) {
		usage(argv[0]);
		PCexit(FAIL);
	}

	if (server_only ||
	    client_only ) {
//		n_clients = 1;
		do_test(port);
		PCexit(0);
	}

	/*
	 * show them what we are doing
	 */

	if (verbose_it == TRUE) {
		fprintf(stderr,
		        "%d repetitions of %d packets; %d clients\n",
		        tries,
		        num_packets,
		        n_clients);
		fprintf(stderr,
		        "Packets: %d,  Write: %d, Read: %d\n",
		        num_packets,
		        write_size,
		        read_size);
	}

	do_test(port);
	PCexit(OK);
}

/******************************************************************************
**
******************************************************************************/
VOID
do_test(port)
	char *port;
{
	int    i, wTry;
	HANDLE *ahThread;
	DWORD  ThreadId;
	DWORD  dwNum;
	
	/*
	 * One write packet suffices since it is read-only (huh?)
	 */

	write_packet = (char *) GC_alloc(write_size);
	fill_write_packet(write_packet, write_size);

	/*
	 * Thread stuff
	 */

	ahThread = (HANDLE *) malloc((n_clients + 1) * sizeof(HANDLE));

	/*
	 * now create server
	 */

	for (wTry=0; wTry<tries; wTry++) {
		dwNum = 0;
		if (!client_only) {
			ahThread[dwNum++] = CreateThread(NULL,
			                                 0,
			                                 do_server,
			                                 port,
			                                 0,
			                                 &ThreadId);
			Sleep(1000);
		}

		/*
		 * now create all clients
		 */

		if (!server_only) {
			GCnsid(GC_FIND_NSID,
			       listen_port,
			       sizeof(listen_port),
			       &syserr);

			for (i = 0; i < n_clients; i++) {
				if (verbose_it == TRUE)
					fprintf(stderr,
					        "client #%d starting up..\n",
					        i);
				ahThread[dwNum++] = CreateThread(NULL,
				                                 0,
				                                 do_client,
				                                 port,
				                                 0,
				                                 &ThreadId);
				Sleep(1500);
			}
		}

		WaitForMultipleObjects(dwNum,
		                       ahThread,
		                       TRUE,
		                       INFINITE);

		for(i=0; i<dwNum; i++) {
			CloseHandle(ahThread[i]);
		}
	}
}

/******************************************************************************
**
******************************************************************************/
DWORD WINAPI
do_client(LPVOID lpParam)
{
	char      *port = (char *) lpParam;
	CONN_DATA *conn_data;
	SVC_PARMS *conn_svc_parms;

	dwType = CLIENT;
	dwPid  = GetCurrentThreadId();

	if (client_dbg == TRUE) {
		;
	}

	/******************************************
	 * Alloc and init the conn_data structure *
	 ******************************************/

	conn_data      = (CONN_DATA *) GC_alloc(sizeof(CONN_DATA));
	InitCON( conn_data );
	conn_svc_parms = &conn_data->conn_parms;

	/********************
	 * Initialize GCACL *
	 ********************/

	GCinitiate(GC_alloc, GC_free);

	/******************************
	 * Try to connect to a server *
	 ******************************/

	if (verbose_it == TRUE)
		fprintf(stderr,
		        "%d: GCrequest to %s\n",
		        dwPid,
		        conn_svc_parms->partner_name);

	GCrequest(conn_svc_parms);
	GCexec();

	/**********************
	 * All done - bye-bye *
	 **********************/

	if (verbose_it == TRUE)
		fprintf(stderr,
		        "%d Client: Done with %d iterations\n",
		        dwPid,
		        reps);

	GCterminate(conn_svc_parms);

	GC_free(conn_data);

	Sleep(1000);
}

/******************************************************************************
**
******************************************************************************/
static          VOID
connect_callback(closure)
	PTR 	closure;
{
	CONN_DATA *conn_data      = (CONN_DATA *) closure;
	SVC_PARMS *svc_parms 	  = &conn_data->conn_parms;
	SVC_PARMS *send_svc_parms = &conn_data->send_parms;
	SVC_PARMS *recv_svc_parms = &conn_data->recv_parms;
	

	fprintf(stderr,
	       "%d> [0x%lx] connect_callback\n",
	       dwPid,
	       svc_parms);

	if (svc_parms->status != OK) {
		error("connect_callback", svc_parms);
		return;
	}

	if (verbose_it == TRUE)
		fprintf(stderr,
		        "%d:\tGCconnect(): connected thru %s\n",
		        dwPid,
		        svc_parms->partner_name);
	

	send_svc_parms->partner_name = svc_parms->partner_name;
	recv_svc_parms->partner_name = svc_parms->partner_name;
	send_svc_parms->gc_cb 	     = svc_parms->gc_cb;
	recv_svc_parms->gc_cb 	     = svc_parms->gc_cb;

	send_packet(send_svc_parms);
	receive_packet(recv_svc_parms);

	fprintf(stderr,
	       "%d< [0x%lx] connect_callback\n",
	       dwPid,
	       svc_parms);
}

/******************************************************************************
**
******************************************************************************/
static          VOID
reg_server(char *port)
{
	SVC_PARMS 	*svc_parms;
	CONN_DATA	conn_data;

	MEfill( (PTR)&conn_data, 0, sizeof( conn_data ) );
	svc_parms               = &conn_data.conn_parms;
	svc_parms->partner_name = port;
	svc_parms->closure	=(PTR)&conn_data;
	svc_parms->sys_err 	= &conn_data.conn_sys_err;
	svc_parms->svr_class    = "IINMSVR";

	GCregister(svc_parms);

	if (svc_parms->status != OK) {
		error("reg_server", svc_parms);
		return;
	}

	STcopy(svc_parms->listen_id, listen_port);

	if (verbose_it == TRUE ||
	    server_only == TRUE)
		fprintf(stderr,
		        "%d: Registered as %s\n",
		        dwPid,
		        svc_parms->listen_id);

	return;
}

/******************************************************************************
**
******************************************************************************/
DWORD WINAPI
do_server(LPVOID lpParam)
{
	char      *port = (char *) lpParam;
	CONN_DATA *conn_data;
	SVC_PARMS *conn_svc_parms;
	int       i;

	dwType = SERVER;
	dwPid  = GetCurrentThreadId();

	if (server_dbg == TRUE) {
		;
	}

	/********************
	 * Initialize GCACL *
	 ********************/

	GCinitiate(GC_alloc, GC_free);

	/********************************
	 * Register ourself as a server *
	 ********************************/

	reg_server(port);

	/***********************************************************
	 * Allocate an array of connection blocks - one per client *
	 ***********************************************************/

	conn_data_blks = (CONN_DATA *) GC_alloc(sizeof(CONN_DATA) * n_clients);

	/********************
	 * Init global data *
	 ********************/

	num_open_connections = n_clients;
	num_lisns_posted     = 0;

	/************************************
	 * Initialize the connection blocks *
	 ************************************/

	for (i = 0; i < n_clients; i++) {
		conn_data = (CONN_DATA *) &conn_data_blks[i];
		InitCON( conn_data );
		conn_svc_parms = &conn_data->conn_parms;
	}

	/*******************
	 * Start listening *
	 *******************/

	conn_svc_parms = &conn_data_blks[num_lisns_posted].conn_parms;
	if (verbose_it == TRUE)
		fprintf(stderr,
		        "%d: GClisten to %s\n",
		        dwPid,
		        conn_svc_parms->listen_id);

	GClisten(conn_svc_parms);
	GCexec();

	/*************************
	 * Close all connections *
	 *************************/

	for (i = 0; i < n_clients; i++) {
		conn_data      = (CONN_DATA *) & conn_data_blks[i];
		conn_svc_parms = &conn_data->conn_parms;
		GCterminate(conn_svc_parms);
	}

	GC_free(conn_data_blks);
}

/******************************************************************************
**
******************************************************************************/
static          VOID
listen_callback(closure)
	PTR  closure;
{
	CONN_DATA *conn_data      = (CONN_DATA *)closure; 
	SVC_PARMS *svc_parms 	  = &conn_data->conn_parms;
	SVC_PARMS *send_svc_parms = &conn_data->send_parms;
	SVC_PARMS *recv_svc_parms = &conn_data->recv_parms;
	SVC_PARMS *conn_svc_parms;

	fprintf(stderr,
	       "%d> [0x%lx] listen_callback\n",
	       dwPid,
	       svc_parms);

	if (svc_parms->status != OK) {
		exit_stat = svc_parms->status;
		error("listen_callback", svc_parms);
	}

	/*********************
	 * Get client's name *
	 *********************/

//	svc_parms->partner_name = "CLIENT";
	svc_parms->partner_name      = svc_parms->account_name;
	send_svc_parms->partner_name = svc_parms->partner_name;
	recv_svc_parms->partner_name = svc_parms->partner_name;


	if (verbose_it == TRUE)
		fprintf(stderr,
		        "%d:\tGClisten: connected thru %s\n",
		        dwPid,
		        svc_parms->partner_name);

	/*********************************************
	 * Post listen for another client connection *
	 *********************************************/

	if (++num_lisns_posted < n_clients) {
		if (verbose_it == TRUE)
			fprintf(stderr,
			        "%d:\tServer RE-POSTING listen\n",
			        dwPid);

		conn_svc_parms = &conn_data_blks[num_lisns_posted].conn_parms;
		GClisten(conn_svc_parms);
	}

	/*************************************
	 * Initiate sends/receives to client *
	 *************************************/

	receive_packet(recv_svc_parms);
	send_packet(send_svc_parms);

	fprintf(stderr,
	       "%d< [0x%lx] listen_callback\n",
	       dwPid,
	       svc_parms);
}

/******************************************************************************
**
******************************************************************************/
static          VOID
send_packet(svc_parms)
	SVC_PARMS *svc_parms;
{
	CONN_DATA *conn_data = (CONN_DATA *) svc_parms->closure;

	if (verbose_it == TRUE)
		fprintf(stderr,
		        "%d: %s sending packet....\n",
		        dwPid,
		        ((dwType == CLIENT) ? "Client" : "Server"));

	GCsend(svc_parms);
}

/******************************************************************************
**
******************************************************************************/
static          VOID
send_callback(closure)
	PTR closure;
{
	CONN_DATA *conn_data = (CONN_DATA *)closure;
	SVC_PARMS *svc_parms = &conn_data->send_parms;

	fprintf(stderr,
	       "%d> [0x%lx] send_callback\n",
	       dwPid,
	       svc_parms);

	if (svc_parms->status != OK) {
		error("send_callback", svc_parms);
		conn_data->send_close_conn = TRUE;
		close_socket(svc_parms);
		return;
	} else {
		conn_data->n_write_packets++;
	}

	if ((conn_data->n_write_packets == num_packets) ||
	    ((conn_data->n_write_packets == num_packets - 2) &&
	     (error_it == TRUE) &&
	     (dwType == CLIENT))) {
		if (verbose_it == TRUE)
			fprintf(stderr,
			        "%d:\t%s done sending %d packets to %s\n",
			        dwPid,
			        (dwType ? "Client" : "Server"),
			        conn_data->n_write_packets,
			        svc_parms->partner_name);
		conn_data->send_close_conn = TRUE;
		conn_data->n_write_packets = 0;
		if (conn_data->recv_close_conn == TRUE)
			close_socket(svc_parms);
		return;
	} else {
		send_packet(svc_parms);
	}

	fprintf(stderr,
	       "%d< [0x%lx] send_callback\n",
	       dwPid,
	       svc_parms);
}

/******************************************************************************
**
******************************************************************************/
static          VOID
receive_packet(SVC_PARMS *recv_svc_parms)
{
	CONN_DATA      *conn_data = (CONN_DATA *) recv_svc_parms->closure;

	MEfill(read_size, 0, conn_data->read_packet);
	conn_data->n_bytes_read = 0;

	GCreceive(recv_svc_parms);
}

/******************************************************************************
**
******************************************************************************/
static          VOID
receive_callback(closure)
	PTR closure;
{
	CONN_DATA *conn_data = (CONN_DATA *) closure;
	SVC_PARMS *recv_svc_parms = &conn_data->recv_parms;

	fprintf(stderr,
	       "%d> [0x%lx] receive_callback\n",
	       dwPid,
	       recv_svc_parms);

	if (recv_svc_parms->status != OK) {
		if (conn_data->n_read_packets < num_packets)
			error("receive_callback", recv_svc_parms);
		conn_data->recv_close_conn = TRUE;
		close_socket(recv_svc_parms);
		return;
	}

	/*
	 * received packet
	 */

	if (recv_svc_parms->rcv_data_length) {
		conn_data->n_bytes_read += recv_svc_parms->rcv_data_length;

		if (conn_data->n_bytes_read != write_size) {
			recv_svc_parms->svc_buffer = conn_data->read_packet +
				                         conn_data->n_bytes_read;
			recv_svc_parms->reqd_amount = write_size -
				                          conn_data->n_bytes_read;
			GCreceive(recv_svc_parms);
			return;
		}

		conn_data->n_read_packets++;
		if (verify_read_packet(conn_data->read_packet,
		                       recv_svc_parms->rcv_data_length,
		                       recv_svc_parms) != TRUE) {
			fprintf(stderr,
			        "\trecv_callback: error-verify_packet\n");
			exit(3);
		}

		if (verbose_it == TRUE) {
			fprintf(stderr,
			        "%d:\t%s received msg %d: (%d)\n",
			        dwPid,
			        (dwType ? "Client" : "Server"),
			        conn_data->n_read_packets,
			        recv_svc_parms->rcv_data_length);
		}
	} else {
		fprintf(stderr,
		        "\trecv_callback: error - 0 bytes recvd\n");
	}

	if ((conn_data->n_read_packets == num_packets) ||
	    ((conn_data->n_read_packets == num_packets - 1) &&
	     (error_it == TRUE) &&
	     (dwType == CLIENT))) {
		if (verbose_it == TRUE)
			fprintf(stderr,
			        "%d:\t%s done receiving  %d packets\n",
			        dwPid,
			        (dwType ? "Client" : "Server"),
			        conn_data->n_read_packets);
		conn_data->n_read_packets  = 0;
		conn_data->recv_close_conn = TRUE;
		if (conn_data->send_close_conn == TRUE)
			close_socket(recv_svc_parms);
		return;
	} else {
		receive_packet(recv_svc_parms);
	}

	fprintf(stderr,
	       "%d< [0x%lx] receive_callback\n",
	       dwPid,
	       recv_svc_parms);
}

/******************************************************************************
**
******************************************************************************/
static          VOID
close_socket(svc_parms)
	SVC_PARMS      *svc_parms;
{
	CONN_DATA *conn_data = ( CONN_DATA * )svc_parms->closure;
	SVC_PARMS *con_svc_parms = &conn_data->conn_parms;

	con_svc_parms->time_out          = timeout;
	con_svc_parms->gca_cl_completion = close_callback;

	if (verbose_it == TRUE) {
		fprintf(stderr,
		        "%d %s closing  connection\n",
		        dwPid,
		        (dwType ? "Client" : "Server"));
	}

	GCdisconn(con_svc_parms);
}

/******************************************************************************
**
******************************************************************************/
static VOID
close_callback(closure)
	PTR 	closure;
{
	CONN_DATA *conn_data = (CONN_DATA *) closure;
	SVC_PARMS *svc_parms = &conn_data->conn_parms;

	fprintf(stderr,
	       "%d> [0x%lx] close_callback\n",
	       dwPid,
	       svc_parms);

	if (svc_parms->status != OK) {
		exit_stat = svc_parms->status;
		error("close_callback", svc_parms);
	}

	if (verbose_it == TRUE)
		fprintf(stderr,
		        "%d:\t%s done closing connection:status %d/0x%x\n",
		        dwPid,
		        (dwType ? "Client" : "Server"),
		        exit_stat,
		        exit_stat);

	if (dwType == CLIENT) { /* GCshut client */
		if (verbose_it == TRUE)
			fprintf(stderr,
			        "%d:\tClient GCshut()\n",
			        dwPid);
		GCshut();
	} else { /* GCshut server only when all threads are done */
		num_open_connections--;
		if (num_open_connections == 0) {
			if (verbose_it == TRUE)
				fprintf(stderr,
				        "%d:\tServer GCshut()\n",
				        dwPid);
			GCshut();
		}
	}

	GCrelease(svc_parms);

	fprintf(stderr,
	       "%d< [0x%lx] close_callback\n",
	       dwPid,
	       svc_parms);
}

/******************************************************************************
** Initalize CONN_DATA
******************************************************************************/
static VOID InitCON( conn_data )
	      CONN_DATA *conn_data; 
{

	SVC_PARMS   *send_svc_parms,
	            *recv_svc_parms,
	            *con_svc_parms;

	char        *read_packet = NULL;


	if (conn_data) {

		conn_data->write_packet    = write_packet;
		conn_data->read_packet     = (char *) GC_alloc(read_size);
		conn_data->n_read_packets  = 0;
		conn_data->n_write_packets = 0;
		conn_data->recv_close_conn = FALSE;
		conn_data->send_close_conn = FALSE;

		read_packet = conn_data->read_packet;

		send_svc_parms 		= &conn_data->send_parms;
		send_svc_parms->closure = (PTR)conn_data;
		send_svc_parms->sys_err = &conn_data->send_sys_err;
		send_svc_parms->flags.flow_indicator = GC_NORMAL;
		send_svc_parms->svc_buffer           = write_packet;
		send_svc_parms->svc_send_length      = write_size;
		send_svc_parms->time_out             = timeout;
		send_svc_parms->gca_cl_completion    = send_callback;

		recv_svc_parms 			= &conn_data->recv_parms;
		recv_svc_parms->closure 	= (PTR)conn_data;
		recv_svc_parms->sys_err 	= &conn_data->recv_sys_err;
		recv_svc_parms->flags.flow_indicator = GC_NORMAL;
		recv_svc_parms->svc_buffer           = read_packet;
		recv_svc_parms->reqd_amount          = read_size;
		recv_svc_parms->time_out             = timeout;
		recv_svc_parms->gca_cl_completion    = receive_callback;

		con_svc_parms  = &conn_data->conn_parms;
		con_svc_parms->sys_err = &conn_data->conn_sys_err;
		con_svc_parms->status            = OK;
		con_svc_parms->time_out          = timeout;
		con_svc_parms->gca_cl_completion = 
			dwType == SERVER? listen_callback: connect_callback;
		con_svc_parms->listen_id         = listen_port;
		con_svc_parms->partner_name      = listen_port;
	}

	return;
}

/******************************************************************************
**
******************************************************************************/
static VOID
fill_write_packet(write_packet, wr_size)
	PTR write_packet;
	int wr_size;
{
	int n;
	int *iptr = (int *) write_packet;

	if (wr_size % 4) {
		fprintf(stderr,
		        "Bad write_size (should be multiple of 4 )\n");
		exit(3);
	}

	for (n = 0; n < wr_size / 4;) {
		*iptr = n * 4;
		n++;
		iptr++;
	}
}

/******************************************************************************
**
******************************************************************************/
static bool
verify_read_packet(read_packet, r_size, svc_parms)
	PTR       read_packet;
	int       r_size;
	SVC_PARMS *svc_parms;
{
	CONN_DATA *conn_data = (CONN_DATA *) svc_parms->closure;
	int       *iptr      = (int *) read_packet;
	int       n;

	if (r_size % 4) {
		fprintf(stderr,
		        "Bad read_size (should be multiple of 4 )\n");
		exit(3);
	}
	for (n = 0; n < r_size / 4; n++) {
		if (*iptr != n * 4) {
			fprintf(stderr,
			        "verify_read_packet: Error data %d instead  of %d - packet # %d\n",
			        *iptr,
			        n * 4,
			        conn_data->n_read_packets);
			return FALSE;
		}
		iptr++;
	}
	return TRUE;
}
