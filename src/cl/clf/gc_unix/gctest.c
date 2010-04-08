/*
**Copyright (c) 2004 Ingres Corporation
*/
#include 	<compat.h>
#include 	<gl.h>
#include 	<systypes.h>
#include 	<clconfig.h>
#include	<stdio.h>
#include	<clsigs.h>
#include	<st.h>
#include	<tr.h>
#include 	<gc.h>
#include	<me.h>
#include	<pc.h>
#include	<rusage.h>
#include	<bsi.h>
#include 	"gcarw.h"
#include 	"gcacli.h"

#include	<errno.h>
extern	int	errno;

#define	NUM_PACKETS	40
#define	MIN_PACKETS	4
#define	PACKET_SIZE	4096
#define	MAXPATHNAME	1024
#define	TIMEOUT		-1
#define SERVER		0
#define CLIENT		1
#define TRIES           4
#define N_CLIENTS	4
#define	TEST_DURATION	10	/* seconds */
#define	NUM_SIGNALS	100 	/* num of SIGUSR2's to send to server */

/*
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
NEEDLIBS = COMPAT MALLOCLIB 
**
** History:
**      12-Mar-1991 (gautam)
**		Created to replace 6.2 bstest 
**      16-Apr-1991 (gautam)
**		Corrected coding error that showed up on some boxes
**      10-May-1991 (gautam)
**		Added client_only/server_only functionality for debugging
**      13-Jan-1993 (brucek)
**		Added GCFLIB to NEEDLIBS.
**      18-Jan-1993 (edg)
**		Removed GCFLIB from NEEDLIBS.
**	04-Mar-1993 (edg)
**		Removed detection stuff so that we can get rid of GCdetect().
**      16-mar-1993 (smc)
**              Removed declaration of MEreqmem, it is now inherited from
**		<me.h>. Also, include <systype.h> after <compat.h>, not
**		before it.
**	27-apr-1993 (sweeney)
**		A call to fprintf was missing its final parameter, and was 
**		printing meaningless status values. Also, the -p port
**		flag works, so modify the usage message.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      25-Aug-1993 (fredv)
**          GCinitiate should be declared as STATUS, not VOID.
**      26-aug-93 (smc)
**              Removed anomalous declarations of external GC functions, 
**              now declared in gc.h.
**	11-May-98 (rajus01)
**	    Fixed MEfill() function call. The server issues GCdisconn() and
**	    GCrelease() for each client and finally issues GCterminate().
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

VOID    iiCLintrp();


/*
** local routines
*/
static	PTR	GC_alloc();
static	void	GC_free();
static 	VOID	usage();
static  VOID    fill_write_packet();
static  bool    verify_read_packet();
static	VOID	reg_server();
static	VOID	do_signal();
static	VOID	reap_child();
static	VOID	do_server();
static	VOID	do_client();
static	VOID	listen_callback( PTR );
static	VOID	connect_callback();
static	VOID	send_packet();
static	VOID	send_callback();
static	VOID	close_socket();
static	VOID	close_callback( PTR );
static	VOID	receive_packet();
static	VOID	receive_callback();

/*
** local data
*/
typedef struct	_conn_data
{
	/* connection control block */
	PTR	gc_cb;
	char	*write_packet ;
	char	*read_packet ;
 	int	n_read_packets;
 	int	n_write_packets;
 	int	n_bytes_read;
      	bool	recv_close_conn ; /* state machine done receiving */
	bool	send_close_conn ; /* state machine done sending */
    SVC_PARMS	send_parms;
    CL_SYS_ERR	send_sys_err;
    SVC_PARMS	recv_parms;
    CL_SYS_ERR	recv_sys_err;
    SVC_PARMS	conn_parms;
    CL_SYS_ERR	conn_sys_err;
} CONN_DATA;

static	CONN_DATA	*conn_data_blks;
static	CL_SYS_ERR 	syserr;	
static	int	pid, server_pid, client_pid;
static	char	listen_port[MAXPATHNAME];
static	int	num_packets = NUM_PACKETS;
static	int	num_signals = NUM_SIGNALS;
static	bool	verbose_it = FALSE ;
static	int	timeout = TIMEOUT;
static	int	packet_size = PACKET_SIZE;
static	int	read_size =  PACKET_SIZE;
static	int	write_size =  PACKET_SIZE;
static	bool	type = CLIENT;
static	bool	signal_it = FALSE;
static	bool	error_it = FALSE;
static	int	exit_stat = OK;
static	int	reps = 0; 
static	int	tries = TRIES;
static	int	n_clients = N_CLIENTS;
static  int	num_open_connections = N_CLIENTS;
static  int	num_lisns_posted = 0;     /* one per client connection */
static	bool	client_dbg = FALSE;
static	bool	server_dbg = FALSE;
static	bool	server_only = FALSE;
static	bool	client_only = FALSE;

/* Memory allocator functions */
static PTR
GC_alloc( size)
int	size;
{
	PTR     ptr;
	STATUS	status;

	 ptr = MEreqmem( 0, size , TRUE, &status ) ;
	 return(ptr); 
}

static void
GC_free(ptr)
PTR	ptr;
{
	(void) MEfree( ptr );
}

/* generic error routine - ignores SIGPIPE errors */
static VOID
error(str, svc_parms)
char	*str;
SVC_PARMS	*svc_parms;
{
	CONN_DATA  *conn_data  = (CONN_DATA *)svc_parms->closure;

	if ((error_it == FALSE) || (verbose_it == TRUE))
	{
		fprintf(stderr,"%d: %s error in %s: ERRNO:%d;STATUS: 0x%x\n",
		 pid, (type ? "Client" : "Server" ), str, errno,
                 svc_parms->status);
		fprintf(stderr,"\tn_read_pkts=%d,n_write_pkts=%d,reps=%d\n",
		 conn_data->n_read_packets, conn_data->n_write_packets, reps);
	}
	if (error_it == FALSE)
		exit(3);
}

static VOID
usage(name)
char	*name;
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

/*
** test the GC interface
*/
main(argc, argv)
int	argc;
char	*argv[];
{
	int	n = 0;
	char	*port = NULL;

	PCpid(&pid);
	MEadvise(ME_INGRES_ALLOC);

	TRset_file(TR_T_OPEN, "stdio", 5, &syserr);

	while(++n < argc)
	{
		if(STcompare(argv[n], "-n") == 0)
			num_packets = atoi(argv[++n]);
		else if(STcompare(argv[n], "-v") == 0)
			verbose_it = TRUE;
		else if(STcompare(argv[n], "-p") == 0)
			port = argv[++n];
		else if(STcompare(argv[n], "-i") == 0)
			read_size = atoi(argv[++n]);
		else if(STcompare(argv[n], "-o") == 0)
			write_size = atoi(argv[++n]);
		else if(STcompare(argv[n], "-b") == 0)
			packet_size = atoi(argv[++n]);
		else if(STcompare(argv[n], "-iterations") == 0)
			tries = atoi(argv[++n]);
		else if(STcompare(argv[n], "-c") == 0)
			n_clients  = atoi(argv[++n]);
		else if(STcompare(argv[n], "-errorit") == 0)
			error_it = TRUE;
		else if(STcompare(argv[n], "-server_only") == 0)
			server_only = TRUE;
		else if(STcompare(argv[n], "-client_only") == 0)
			client_only = TRUE;
		else if(STcompare(argv[n], "-x") == 0)
		{
			signal_it = TRUE;
			num_signals = atoi(argv[++n]);
		}
		else
		{
			usage(argv[0]);
			PCexit(FAIL);
		}
	}

	GCinitiate( GC_alloc, GC_free );
	
	/* 
	** set read/write/num_packet sizes
	*/
	if (packet_size != PACKET_SIZE)
		read_size = write_size = packet_size;
	if (read_size  < write_size)
		read_size = write_size;
	if (num_packets < MIN_PACKETS)
		num_packets = MIN_PACKETS;

	if (server_only && client_only)
	{
		usage(argv[0]);
		PCexit(FAIL);
	}

	if (server_only)
	{
		n_clients = 1;
		PCpid(&server_pid);
		do_test(port);
		PCexit(0);
	}
	
	if (client_only)
	{
		n_clients = 1;
		type = CLIENT;
		PCpid(&client_pid);
		STcopy(port, listen_port);
		do_client(port);
		PCexit(0);
	}
	/*
	** show them what we are doing
	*/
	if (verbose_it == TRUE)
	{
		fprintf(stderr,"%d repetitions of %d packets; %d clients\n", tries, num_packets,  n_clients);
		fprintf(stderr,"%d: Packets: %d,  Write: %d, Read: %d\n",
		pid,  num_packets,  write_size, read_size);
	}


	/*
	** fork child to start test
	*/
	PCpid(&pid);
	switch(server_pid= fork() )
	{
	case -1:
		perror("fork");
		PCexit( FAIL );
		break;
	case 0:
		do_test(port);
		PCexit(OK);
		break;
	}

	/* fork a process to send SIGUSR2's to server if desired */
	if (signal_it == TRUE)
	{
		switch(n = fork() )
		{
		case -1:
			perror("fork");
			PCexit( FAIL );
			break;
		case 0:
			sleep(2);
			do_signal(server_pid);
			PCexit(OK);
		}
	}

	/* catch the children */
	reap_child();
	PCexit(OK);
}


/* 
** signal server repeatedly with SIGUSR2's 
*/
static	VOID
do_signal(serv_pid)
int	serv_pid;
{
	long	num_usecs = TEST_DURATION * MICRO_PER_SEC;
	long	counter = 0;
	long	sleep_time = num_usecs/num_signals ;

	while (counter < num_usecs)
	{
		II_nap(sleep_time);
		kill(serv_pid,SIGUSR2);
		if (verbose_it)
			fprintf(stderr,"SIGUSR2 to server (%d)\n",serv_pid);
		counter += sleep_time;
	}
}

static	VOID
reap_child()
{
	int	exit_status = OK, status ;

	exit_status = OK;
	while( (pid=wait(&status)) != -1)
	{
		if(status != 0)
			exit_status = status;
	}
	if (verbose_it == TRUE)
		fprintf(stderr,"GCtest exit status: %d/0x%x\n", exit_status, exit_status);
	PCexit(exit_stat);
}

do_test(port)
char	*port;
{
	int     i;

	/*
	** Handle SIGPIPE, SIGUSR2 
	*/
	i_EXsetothersig( SIGPIPE, SIG_IGN );
	i_EXsetothersig( SIGUSR2, iiCLintrp );

	/*
	** Create a server and return the listen address.
	*/
	PCpid(&server_pid);
	pid = server_pid;
	reg_server(port);

	/*
	** now create a client
	*/
	if (server_only == FALSE)
 	 for (i = 0; i < n_clients; i++)
	  switch( (client_pid=fork()) )
	{
	case -1:	/* oops */
		perror("fork");
		PCexit(FAIL);
		break;
	case 0:		/* child */
		PCpid(&client_pid);
		if (verbose_it == TRUE)
			fprintf(stderr,"%d client starting up..\n",client_pid);
		type = CLIENT;
		do_client(port);
		PCexit(OK);
		break;
	}
	type = SERVER;
	do_server(port);
	PCexit(OK);
}


static VOID
do_client(port)
char	*port;
{
    CONN_DATA	*conn_data;
    SVC_PARMS  	*conn_svc_parms;
    char		*read_packet, *write_packet;
    int		i;
    STATUS		status;
	
    pid = client_pid;

    /*
    ** Alloc and init the conn_data structure
    ** Fill in write/read packet
    */
    conn_data = (CONN_DATA *)GC_alloc(sizeof(CONN_DATA));
    write_packet = (char *)GC_alloc(write_size);
    read_packet = (char *)GC_alloc(read_size);
    if (!write_packet || !read_packet)
    {
	perror("malloc");
	PCexit(FAIL);
    }
    fill_write_packet(write_packet, write_size); /* generate data */

    conn_data->write_packet = write_packet;
    conn_data->read_packet = read_packet;
    conn_svc_parms = &conn_data->conn_parms;

    for(reps=0; reps < tries; reps++)
    {
	/* Fill the conn_svc_parms */
	conn_svc_parms->partner_name = listen_port ;
	conn_svc_parms->status = OK ;
	conn_svc_parms->sys_err = &conn_data->conn_sys_err;
	conn_svc_parms->time_out = timeout;
	conn_svc_parms->gca_cl_completion = connect_callback;
	conn_svc_parms->closure = (PTR)conn_data;

	if (verbose_it == TRUE)
	    fprintf(stderr,"%d: GCrequest to %s\n", 
		    pid, conn_svc_parms->partner_name);
	GCrequest(conn_svc_parms);
	GCexec();

	if (verbose_it == TRUE)
	    fprintf(stderr,"%d Client: Done with %d iterations\n",
		   pid, reps);
    }
    GCterminate( conn_svc_parms);
}


static VOID
connect_callback(closure)
PTR closure;
{
    CONN_DATA	*conn_data = (CONN_DATA *)closure;
    SVC_PARMS	*svc_parms = &conn_data->conn_parms;
    SVC_PARMS	*send_svc_parms = &conn_data->send_parms;
    SVC_PARMS	*recv_svc_parms = &conn_data->recv_parms;

    if(svc_parms->status != OK)
    {
	    error("connect_callback", svc_parms);
	    return;
    }

    if (verbose_it == TRUE)
	    fprintf(stderr,"%d: GCconnect(): connected thru %s\n", 
		    pid, svc_parms->partner_name);

    conn_data->n_read_packets =  0;
    conn_data->n_write_packets =  0;
    conn_data->recv_close_conn =  FALSE ;
    conn_data->send_close_conn =  FALSE ;

    send_svc_parms->gc_cb = svc_parms->gc_cb;
    recv_svc_parms->gc_cb = svc_parms->gc_cb;
    send_svc_parms->closure = (PTR)conn_data;
    recv_svc_parms->closure = (PTR)conn_data;
    send_svc_parms->partner_name = svc_parms->partner_name ;
    recv_svc_parms->partner_name = svc_parms->partner_name ;
    send_svc_parms->sys_err = &conn_data->send_sys_err;
    recv_svc_parms->sys_err = &conn_data->recv_sys_err;

    send_packet(send_svc_parms);
    receive_packet(recv_svc_parms);
}

static VOID
reg_server(port)
char		*port;
{
    SVC_PARMS	*svc_parms;
    CONN_DATA	conn_data;

    MEfill( sizeof( conn_data ), 0, (PTR)&conn_data );
    svc_parms = &conn_data.conn_parms;
    svc_parms->closure = (PTR)&conn_data;
    svc_parms->sys_err = &conn_data.conn_sys_err;

    /*
    ** GCregister is synchronous
    */
    GCregister(svc_parms);

    if(svc_parms->status != OK)
    {
	error("reg_server", svc_parms);
	return;
    }
    STcopy(svc_parms->listen_id, listen_port);

    if(verbose_it == TRUE || server_only == TRUE)
	fprintf(stderr,"%d: Registered as %s\n", pid, svc_parms->listen_id);
    return;
}

static	VOID
do_server( port)
char	*port;
{
    CONN_DATA	*conn_data;
    SVC_PARMS  	*conn_svc_parms;
    char	*read_packet, *write_packet;
    int		i;
    STATUS	status;

    /*
    ** Alloc and init the conn_data_blks structures
    ** One write packet suffices since it is read-only
    */
    write_packet = (char *)GC_alloc(write_size);
    if (!write_packet )
    {
	perror("malloc");
	PCexit(FAIL);
    }
    fill_write_packet(write_packet, write_size); /* generate data */

    conn_data_blks = (CONN_DATA *)GC_alloc(sizeof(CONN_DATA) * n_clients); 
    conn_data = conn_data_blks;
    for (i = 0; i < n_clients; i++)
    {
	conn_data = (CONN_DATA *)&conn_data_blks[i];
	read_packet = (char *)GC_alloc(read_size);
	if(read_packet == NULL)
	{
	    perror("malloc");
	    PCexit(FAIL);
	}
	conn_data->write_packet = write_packet;
	conn_data->read_packet = read_packet;

	/* Fill the conn_svc_parms */
	conn_svc_parms = &conn_data->conn_parms;
	conn_svc_parms->status = OK ;
	conn_svc_parms->sys_err = &conn_data->conn_sys_err;
	conn_svc_parms->time_out = timeout;
	conn_svc_parms->gca_cl_completion = listen_callback;
	conn_svc_parms->closure = (PTR) conn_data;
	conn_svc_parms->listen_id = listen_port;
    }
				
    /*
    ** listen and drive GCA events
    */
    for(reps=0; reps<tries ; reps++)
    {
	num_open_connections = n_clients;
	num_lisns_posted = 0;

	conn_data = conn_data_blks;
	conn_svc_parms = &conn_data->conn_parms;
	conn_svc_parms->status = OK ;
	conn_svc_parms->sys_err = &conn_data->conn_sys_err;
	conn_svc_parms->time_out = timeout;
	conn_svc_parms->gca_cl_completion = listen_callback;
	conn_svc_parms->closure = (PTR)conn_data;
	conn_svc_parms->listen_id = listen_port;
	
	if (verbose_it == TRUE)
	    fprintf(stderr,"%d: GClisten to %s\n", 
		   pid, conn_svc_parms->listen_id);
	GClisten(conn_svc_parms);
	GCexec();

	if (verbose_it == TRUE)
	    fprintf(stderr,"%d: Server : %d iterations\n", 
		   pid, reps);
    }

    conn_data = conn_data_blks;
    conn_svc_parms = &conn_data->conn_parms;

    for (i = 0 ; i < n_clients; i++)
    {
	conn_data = (CONN_DATA *)&conn_data_blks[i];
	conn_svc_parms = &conn_data->conn_parms;
	conn_svc_parms->sys_err = &conn_data->conn_sys_err;
	GCdisconn( conn_svc_parms);
	GCrelease( conn_svc_parms);
    }

    GCterminate( conn_svc_parms );
}

static VOID
listen_callback(PTR closure)
{
    CONN_DATA	*conn_data  = (CONN_DATA *)closure;
    SVC_PARMS	*svc_parms = &conn_data->conn_parms;
    SVC_PARMS	*send_svc_parms = &conn_data->send_parms;
    SVC_PARMS	*recv_svc_parms = &conn_data->recv_parms;
    SVC_PARMS	*conn_svc_parms;

    if(svc_parms->status != OK)
    {
	exit_stat = svc_parms->status;
	error("listen_callback", svc_parms);
    }

    svc_parms->partner_name = "CLIENT" ;

    if (verbose_it == TRUE)
	fprintf(stderr,"%d GClisten: connected thru %s\n", 
	       pid, svc_parms->partner_name);

    num_lisns_posted++;

    conn_data->recv_close_conn = FALSE;
    conn_data->send_close_conn = FALSE;
    conn_data->n_read_packets = 0; 
    conn_data->n_write_packets = 0;

    send_svc_parms->gc_cb = svc_parms->gc_cb;
    recv_svc_parms->gc_cb = svc_parms->gc_cb;
    send_svc_parms->closure = (PTR)conn_data;
    recv_svc_parms->closure = (PTR)conn_data;
    send_svc_parms->partner_name = svc_parms->partner_name ;
    recv_svc_parms->partner_name = svc_parms->partner_name ;
    send_svc_parms->sys_err = &conn_data->send_sys_err;
    recv_svc_parms->sys_err = &conn_data->recv_sys_err;

    if (num_lisns_posted < n_clients )
    {
	/* 
	** Post listen for another client connection 
	*/
	if (verbose_it == TRUE)
	    fprintf(stderr,"%d: Server RE-POSTING listen\n", pid);

	conn_data = (CONN_DATA *)&conn_data_blks[num_lisns_posted];

	conn_data->recv_close_conn = FALSE;
	conn_data->send_close_conn = FALSE;
	conn_data->n_read_packets = 0; 
	conn_data->n_write_packets = 0;

	conn_svc_parms = &conn_data->conn_parms;
	conn_svc_parms->status = OK ;
	conn_svc_parms->sys_err = &conn_data->conn_sys_err;
	conn_svc_parms->time_out = timeout;
	conn_svc_parms->gca_cl_completion = listen_callback;
	conn_svc_parms->closure = (PTR)conn_data;
	conn_svc_parms->listen_id = listen_port;
	
	GClisten(conn_svc_parms);
    }

    receive_packet(recv_svc_parms);
    send_packet(send_svc_parms);
}

static VOID
send_packet(svc_parms )
SVC_PARMS	*svc_parms;
{
    CONN_DATA  *conn_data  = (CONN_DATA *)svc_parms->closure;

    /*
    ** send a packet
    */
    if (verbose_it == TRUE)
	fprintf(stderr,"%d: %s sending packet....\n", pid,
		((type == CLIENT)? "Client":"Server"));

    svc_parms->flags.flow_indicator = GC_NORMAL;
    svc_parms->svc_buffer = conn_data->write_packet;
    svc_parms->svc_send_length  =  write_size ;
    svc_parms->time_out =  timeout ;
    svc_parms->gca_cl_completion = send_callback;
    GCsend(svc_parms);
}

static VOID
send_callback(closure)
PTR closure;
{
    CONN_DATA  *conn_data  = (CONN_DATA *)closure;
    SVC_PARMS  *svc_parms = &conn_data->send_parms;

    if(svc_parms->status != OK)
    {
	error("send_callback", svc_parms);
	conn_data->send_close_conn = TRUE;
	close_socket(svc_parms);
	return;
    }
    else
    {
	conn_data->n_write_packets++;
    }

    if ((conn_data->n_write_packets == num_packets) ||
	( (conn_data->n_write_packets == num_packets-2) && (error_it == TRUE)
		&& (type == CLIENT) ) )
    {
	if (verbose_it == TRUE)
	    fprintf(stderr,"%d: %s done sending %d packets to %s\n",
		    pid, (type ? "Client" : "Server"), 
		    conn_data->n_write_packets, 
		    svc_parms->partner_name);
	conn_data->send_close_conn = TRUE;
	conn_data->n_write_packets = 0;
	if (conn_data->recv_close_conn ==  TRUE)
	    close_socket(svc_parms);
	return;
    }
    else	
    {
	send_packet(svc_parms);
    }
}

static VOID
receive_packet(svc_parms )
SVC_PARMS       *svc_parms;
{
    CONN_DATA  *conn_data  = (CONN_DATA *)svc_parms->closure;

    /*
    ** receive a packet 
    ** read_size is always >= write_size 
    */
    MEfill(read_size, 0, conn_data->read_packet);
    conn_data->n_bytes_read = 0;

    svc_parms->flags.flow_indicator = GC_NORMAL;
    svc_parms->svc_buffer = conn_data->read_packet ;
    svc_parms->reqd_amount  =  read_size;
    svc_parms->time_out =  timeout ;
    svc_parms->gca_cl_completion = receive_callback;

    GCreceive(svc_parms);
}

static VOID
receive_callback(closure)
PTR closure;
{
    CONN_DATA	*conn_data  = (CONN_DATA *)closure;
    SVC_PARMS	*svc_parms = &conn_data->recv_parms;

    if(svc_parms->status != OK)
    {
	if( conn_data->n_read_packets < num_packets )
	    error("receive_callback", svc_parms);
	conn_data->recv_close_conn = TRUE;
	close_socket(svc_parms);
	return;
    }

    /* received packet */
    if(svc_parms->rcv_data_length )
    {
	conn_data->n_bytes_read += svc_parms->rcv_data_length;

	if ( conn_data->n_bytes_read != write_size)
	{
	    /* buffer reads */
	    svc_parms->svc_buffer = conn_data->read_packet 
				    + conn_data->n_bytes_read ;
	    svc_parms->reqd_amount  =  write_size 
				      - conn_data->n_bytes_read;
	    svc_parms->gca_cl_completion = receive_callback;
	    GCreceive(svc_parms);
	    return;
	}
	conn_data->n_read_packets++;
	if (verify_read_packet(conn_data->read_packet, 
		svc_parms->rcv_data_length,svc_parms) != TRUE)
	{
	    fprintf(stderr,"recv_callback: error-verify_packet\n");
	    exit(3);
	}

	if(verbose_it == TRUE)
	{
	    fprintf(stderr,"%d:%s received msg %d: (%d)\n", pid, 
		  (type ? "Client" : "Server"),
		   conn_data->n_read_packets, 
		   svc_parms->rcv_data_length);
	}
    }
    else
    {
	fprintf(stderr," recv_callback: error - 0 bytes recvd\n");
    }

    if ((conn_data->n_read_packets == num_packets) ||
	( (conn_data->n_read_packets == num_packets-1) && (error_it == TRUE)
		&& (type == CLIENT) ) )
    {
	if (verbose_it == TRUE)
	    fprintf(stderr,"%d: %s done receiving  %d packets\n", 
		    pid,(type ? "Client" : "Server"),
		    conn_data->n_read_packets);
	conn_data->n_read_packets = 0;
	conn_data->recv_close_conn = TRUE ;
	if (conn_data->send_close_conn == TRUE)
	    close_socket(svc_parms);
	return;
    }
    receive_packet(svc_parms);
}

static VOID
close_socket(svc_parms)
SVC_PARMS       *svc_parms;
{
    CONN_DATA	*conn_data = (CONN_DATA *)svc_parms->closure;
    SVC_PARMS	*con_svc_parms = &conn_data->conn_parms;

    con_svc_parms->time_out =  timeout ;
    con_svc_parms->gca_cl_completion = close_callback;
    con_svc_parms->closure = (PTR) conn_data;
    con_svc_parms->sys_err = &conn_data->conn_sys_err;

    if (verbose_it ==  TRUE)
    {
	fprintf(stderr,"%d %s closing  connection\n", 
		pid, (type ? "Client" : "Server"));
    }
    GCdisconn(con_svc_parms);
}

static VOID
close_callback(PTR closure)
{
    CONN_DATA	*conn_data = (CONN_DATA *)closure;
    SVC_PARMS	*svc_parms = &conn_data->conn_parms;

    if(svc_parms->status != OK)
    {
	exit_stat = svc_parms->status ; 
	error("close_callback", svc_parms);
    }
    if (verbose_it == TRUE)
	fprintf(stderr,"%d:%s done closing connection:status %d/0x%x\n",
		pid, (type ? "Client" : "Server"), exit_stat, exit_stat);
    if (type == CLIENT)
    {
	/* GCshut client */
	if (verbose_it == TRUE)
	    fprintf(stderr,"%d: Client GCshut()\n",pid);
	GCshut();
    }
    else 
    {
	/* GCshut server only when all threads are done */
	num_open_connections--;
	if (num_open_connections == 0)
	{
	    if (verbose_it == TRUE)
		fprintf(stderr,"%d: Server GCshut()\n",pid);
	    GCshut();
	}
    }
    GCrelease(svc_parms);
}

static  VOID
fill_write_packet(write_packet, wr_size )
PTR	write_packet;
int	wr_size;
{
	int	n;
	int	*iptr = (int *)write_packet;

	if (wr_size % 4)
	{
		fprintf(stderr,"Bad write_size (should be multiple of 4 )\n");
		exit(3);
	}
	for ( n = 0; n < wr_size/4;)
	{
		*iptr = n * 4;
		n++; iptr++;
	}
}

static  bool
verify_read_packet(read_packet, r_size,  svc_parms)
PTR	read_packet;
int	r_size;
SVC_PARMS	*svc_parms;
{
	CONN_DATA  *conn_data  = (CONN_DATA *)svc_parms->closure;
	int	n;
	int	*iptr = (int *)read_packet;

	if (r_size % 4)
	{
		fprintf(stderr,"Bad read_size (should be multiple of 4 )\n");
		exit(3);
	}
	for ( n = 0; n < r_size/4; n++)
	{
		if (*iptr != n * 4)
		{
			fprintf(stderr,"verify_read_packet: Error data %d instead  of %d - packet # %d\n", *iptr,  n * 4, conn_data->n_read_packets);
			return FALSE;
		}
		iptr++;
	}
	return TRUE;
}
