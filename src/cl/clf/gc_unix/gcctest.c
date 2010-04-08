/*
**Copyright (c) 2004 Ingres Corporation
*/
#include 	<systypes.h>
#include 	<compat.h>
#include 	<gl.h>
#include 	<clconfig.h>
#include	<stdio.h>
#include	<clsigs.h>
#include	<st.h>
#include	<tr.h>
#include 	<gc.h>
#include 	<gcccl.h>
#include	<me.h>
#include	<pc.h>

#include	<errno.h>

#define	NUM_PACKETS	4
#define	MIN_PACKETS	4
#define	PACKET_SIZE	4096
#define	MAXPATHNAME	1024
#define	TIMEOUT		-1
#define SERVER		0
#define CLIENT		1
#define TRIES		1
#define N_CLIENTS	1
#define PADDING		16

/*
** Test for the GCC/CL interface. This test is written to test out  
** the different protocol driver support for Ingres/NET.
** 
** Usage:
** Start up a "listen" gcctest on host machine with
**   $ gcctest  -type server -protocol protocol_name
** 
** Start up client using
**   $ gcctest  -type client -host host_machine_name -port server_port_name 
**           -protocol protocol_name 
**   
**   $ gcctest  -help 
**           gives usage instructions
**
PROGRAM = gcctest
**
NEEDLIBS = COMPAT MALLOCLIB INGNETLIB 
**
** History:
**      12-Mar-1992 (gautam)
**		Created 
**      09-Apr-1992 (dianeh)
**    Added INGNETLIB to NEEDLIBS.
**      13-sep-96 (gautam)
**    Added an iiCLpoll() call in send_callback for IBM RS/6000 SNA 
**    LU6.2 multiple-sessions
**      13-Jan-93 (brucek)
**    Added GCFLIB to NEEDLIBS.
**      18-Jan-93 (edg)
**    Removed GCFLIB from NEEDLIBS.
**
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	24-nov-93 (robf)
**	    added check for listen returning node name.
**	    fix AV in send_callback by passing correct parameters to 
**	    iiCLpoll() (should be pointer, not value)
**	29-may-1997 (canor01)
**	    Clean up compiler warnings.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


VOID	GCexec();
/*
** external data
*/
GLOBALREF GCC_PCT       IIGCc_gcc_pct;
/*
** local data
*/
typedef struct	_conn_data
{
	/* connection control block */
	GCC_P_PLIST 	*lsn_parms ;
	GCC_P_PLIST 	*send_parms ;
	GCC_P_PLIST 	*rcv_parms ;
	GCC_PCE		*pce ;
	GCC_PCT		*pct ;
	CL_ERR_DESC     conn_system_status;
	STATUS     	conn_generic_status ; 
	char		*write_packet ;
	char		*read_packet ;
 	int		n_read_packets;
 	int		n_write_packets;
 	int		n_bytes_read;
	bool		recv_close_conn ; /* state machine done receiving */
	bool		send_close_conn ; /* state machine done sending */
} CONN_DATA;

/*
** local routines
*/
static	PTR	GCC_alloc(int size);
static	STATUS	GCC_free(PTR p);
static  VOID    error(char *str, CONN_DATA *conn_data, GCC_P_PLIST *parms);
static  VOID    do_ds(CONN_DATA *conn_bk);
static 	VOID	usage(char *name);
static  GCC_PCE *get_pce(CONN_DATA *conn_bk, char *protocol);
static	VOID 	server_listen(CONN_DATA *conn_data);
static  VOID    fill_write_packet(PTR write_packet, int wr_size);
static  bool    verify_read_packet(PTR read_packet, int r_size,  
				    CONN_DATA *conn_data);
static	VOID	do_server(char *port);
static	VOID	do_client(char *port);
static	VOID	listen_callback(CONN_DATA *conn_data);
static	VOID	connect_callback(CONN_DATA *conn_data);
static	VOID	send_packet(CONN_DATA *conn_data);
static	VOID	send_callback(CONN_DATA *conn_data);
static	VOID	close_conn(CONN_DATA *conn_data);
static	VOID	close_callback(CONN_DATA *conn_data);
static	VOID	receive_packet(CONN_DATA *conn_data);
static	VOID	receive_callback(CONN_DATA *conn_data);

static	CONN_DATA	*conn_data_blks;
static  CL_SYS_ERR      syserr;
static	int	pid, server_pid, client_pid;
static	char	host[MAXPATHNAME];
static	char	port[MAXPATHNAME];
static  char	protocol[128] = TCP_IP ;
static	int	num_packets = NUM_PACKETS;
static	bool	verbose_it = FALSE ;
static	int	timeout = TIMEOUT;
static	int	packet_size = PACKET_SIZE;
static	int	read_size =  PACKET_SIZE;
static	int	write_size =  PACKET_SIZE;
static	bool	type = CLIENT;
static	char	whoami[20];
static	char	type_string[20];
static	int	exit_stat = OK;
static	int	reps = 0; 
static	int	tries = TRIES;
static	int	n_clients = N_CLIENTS;
static  int	num_open_connections = N_CLIENTS;
static  int	num_lisns_posted = 0;     /* one per client connection */
static	bool	client_dbg = FALSE;
static	bool	server_dbg = FALSE;

/* Memory allocator functions */
static PTR
GCC_alloc(int size)
{
	PTR     ptr;
	STATUS	status;

	 ptr = MEreqmem( 0, size , TRUE, &status ) ;
	 return(ptr); 
}

static STATUS
GCC_free(PTR ptr)
{
	return( MEfree( ptr ) );
}

/* generic error routine  */
static VOID
error(char *str, CONN_DATA *conn_data, GCC_P_PLIST *parms)
{

	fprintf(stderr,"%d: %s error in %s: ERRNO:%d;STATUS: 0x%x\n",
	  pid, (type ? "Client" : "Server" ), str, errno, 
	  parms->generic_status);
	fprintf(stderr,"\tn_read_pkts=%d,n_write_pkts=%d,reps=%d\n",
	  conn_data->n_read_packets, conn_data->n_write_packets, reps);
}

static VOID
usage(char *name)
{
	int i ;
	GCC_PCE		*pce = (GCC_PCE *)0;

	fprintf(stderr, "%s: usage:\n", name);
	fprintf(stderr, "\t[-n number_of_packets]\n");
	fprintf(stderr, "\t[-type client/server]\n");
	fprintf(stderr, "\t[-v(erbose)]\n");
	fprintf(stderr, "\t[-port port ]\n");
	fprintf(stderr, "\t[-host hostname (for connect requests)]\n");
	fprintf(stderr, "\t[-i receive_msg_size]\n");
	fprintf(stderr, "\t[-o send_msg_size (< receive_msg_size)]\n");
	fprintf(stderr, "\t[-b msg_packet_size]\n");
	fprintf(stderr, "\t[-c num_of_clients ]\n");
	fprintf(stderr, "\t[-iterations num_of_iterations ]\n");
	fprintf(stderr, "\t[-protocol protocol_name (default is TCP_IP)]\n");
	fprintf(stderr, "\t\tprotocols are\n\t\t");

	pce = &IIGCc_gcc_pct.pct_pce[0] ;
	while( *pce->pce_pid != 0)
	{
		fprintf(stderr, " %s", pce->pce_pid);
		pce++ ;
	}
	fprintf(stderr,"\n");
}

/*
** test a GCC protocol driver 
*/
main(argc, argv)
int	argc;
char	*argv[];
{
	int	n = 0;

	PCpid(&pid);
	MEadvise(ME_INGRES_ALLOC);

	TRset_file(TR_T_OPEN, "stdio", 5, &syserr);

	while(++n < argc)
	{
		if(STcompare(argv[n], "-n") == 0)
			num_packets = atoi(argv[++n]);
		else if(STcompare(argv[n], "-v") == 0)
			verbose_it = TRUE;
		else if(STcompare(argv[n], "-port") == 0)
			STcopy( argv[++n], port);
		else if(STcompare(argv[n], "-host") == 0)
			STcopy( argv[++n], host);
		else if (STcompare(argv[n], "-protocol") == 0)
			STcopy( argv[++n], protocol);
		else if(STcompare(argv[n], "-i") == 0)
			read_size = atoi(argv[++n]);
		else if(STcompare(argv[n], "-o") == 0)
			write_size = atoi(argv[++n]);
		else if(STcompare(argv[n], "-b") == 0)
			packet_size = atoi(argv[++n]);
		else if(STcompare(argv[n], "-iterations") == 0)
			tries = atoi(argv[++n]);
		else if(STcompare(argv[n], "-type") == 0)
			STcopy( argv[++n], type_string);
		else if(STcompare(argv[n], "-c") == 0)
			n_clients  = atoi(argv[++n]);
		else
		{
			usage(argv[0]);
			PCexit(FAIL);
		}
	}
	/* 
	** set read/write/num_packet sizes
	*/
	if (packet_size != PACKET_SIZE)
		read_size = write_size = packet_size;
	if (read_size  < write_size)
		read_size = write_size;
	if (num_packets < MIN_PACKETS)
		num_packets = MIN_PACKETS;

	if (!STcompare(type_string, "server"))
	{
		type = SERVER ;
		STcopy("Server", whoami);
	}
	else
	{
		type = CLIENT ;
		STcopy("Client", whoami);
	}

	/*
	** show them what we are doing
	*/
	if (verbose_it == TRUE)
	{
		fprintf(stderr,"gcctest: protocol %s\n",  protocol);
		fprintf(stderr,"%s: %d repetitions of %d packets; %d clients\n", 
			whoami, tries, num_packets,  n_clients);
		fprintf(stderr,"%d: Packets: %d,  Write: %d, Read: %d\n",
		pid,  num_packets,  write_size, read_size);
	}

	/*
	** start test depending on whether this is a client or a server
	*/
	switch(type)
	{
	case CLIENT:
		do_client(port);
		PCexit( OK );
		break;
	case SERVER:
		do_server(port);
		PCexit(OK);
		break;
	}

	PCexit(OK);
}

static VOID
do_client(char *port)
{
	CONN_DATA	*conn_data;
	char		*read_packet, *write_packet;
	int		i;
	STATUS		status;
	
	client_pid = pid;

	if (client_dbg == TRUE)
	{
		/* future support for debugging */
		;
	}

	/*
	** Alloc and init the conn_data structure
	** Fill in write/read packet
	*/
	conn_data = (CONN_DATA *)GCC_alloc(sizeof(CONN_DATA));
	do_ds(conn_data);

	/*
	** GCC initialization routines
	*/
	GCcinit(&conn_data->conn_generic_status, &conn_data->conn_system_status); 
	GCpinit(&conn_data->pct, &conn_data->conn_generic_status, &syserr); 
	if ((conn_data->pce = get_pce(conn_data, protocol)) == 0)
	{
		fprintf(stderr,"%s:Cannot get %s  pce - error \n", pid, protocol);
		exit(2);
	}
	conn_data->send_parms->pce = conn_data->pce ;
	conn_data->rcv_parms->pce = conn_data->pce ;

	write_packet = (char *)GCC_alloc(write_size + PADDING);
	read_packet = (char *)GCC_alloc(read_size + PADDING);
	if (!write_packet || !read_packet)
	{
		perror("malloc");
		PCexit(FAIL);
	}
	fill_write_packet(write_packet + PADDING, write_size); /* generate data */

	conn_data->write_packet = write_packet;
	conn_data->read_packet = read_packet;

	for(reps=0; reps < tries; reps++)
	{
		GCC_P_PLIST 	*parms  = conn_data->send_parms;
		GCC_PCE		*pce = conn_data->pce ;

		/* 
		** You don't want the client doing a GCC_OPEN, so 
		** fill in the connect parms. 
		*/
		parms->compl_exit = (VOID(*)(PTR))connect_callback ;
		parms->compl_id = (PTR)conn_data ;
		parms->function_invoked = GCC_CONNECT ;
		parms->function_parms.connect.port_id = port ;
		parms->function_parms.connect.node_id = host;

		(*pce->pce_protocol_rtne)(GCC_CONNECT, parms);
		GCexec();
	}
}

static VOID
connect_callback(CONN_DATA *conn_data)
{
	if (conn_data->send_parms->generic_status)
	{
		error("connect_callback", conn_data, conn_data->send_parms);
		conn_data->recv_close_conn =  TRUE ;
		conn_data->send_close_conn =  TRUE ;
		close_conn(conn_data);
		return;
	}

	if (verbose_it == TRUE)
		fprintf(stderr,"%d: GCC_CONNECT: connected\n", pid);

	conn_data->n_read_packets =  0;
	conn_data->n_write_packets =  0;
	conn_data->recv_close_conn =  FALSE ;
	conn_data->send_close_conn =  FALSE ;

	conn_data->rcv_parms->pcb = conn_data->send_parms->pcb ;

	receive_packet(conn_data); 
	send_packet(conn_data);
}

static	VOID
do_server(char *port)
{
	CONN_DATA	*conn_data;
	GCC_P_PLIST 	*parms  ;
	GCC_PCE		*pce ;
	char		*read_packet, *write_packet;
	int		i;
	STATUS		status;

	if (server_dbg == TRUE)
	{
		/* future support for debugging */
		;
	}

	/*
	** Alloc and init the conn_data_blks structures
	** One write packet suffices since it is read-only
	*/
	write_packet = (char *)GCC_alloc(write_size + PADDING);
	if (!write_packet )
	{
		perror("malloc");
		PCexit(FAIL);
	}
	fill_write_packet(write_packet + PADDING, write_size); /* generate data */

	conn_data_blks = (CONN_DATA *)GCC_alloc(sizeof(CONN_DATA) * n_clients); 

	for (i = 0; i < n_clients; i++)
	{
		conn_data = (CONN_DATA *)&conn_data_blks[i];
		read_packet = (char *)GCC_alloc(read_size + PADDING);
		if(read_packet == NULL)
		{
			perror("malloc");
			PCexit(FAIL);
		}
		conn_data->read_packet = read_packet ;
		conn_data->write_packet = write_packet ;
		do_ds(conn_data);
	}

	conn_data = (CONN_DATA *)&conn_data_blks[0];
	GCcinit(&conn_data->conn_generic_status, &conn_data->conn_system_status); 
	GCpinit(&conn_data->pct, &conn_data->conn_generic_status, &syserr); 
	if ((pce = get_pce(conn_data, protocol)) == 0)
	{
		fprintf(stderr,"%d: Cannot get %s pce - error \n", pid, protocol);
		exit(2);
	}
	conn_data->pce = pce ;
	conn_data->send_parms->pce = conn_data->pce ;
	conn_data->rcv_parms->pce = conn_data->pce ;
	conn_data->lsn_parms->pce = conn_data->pce ;

	/* 
	** Put in the pce for the other conn_blks
	*/
	for (i = 1; i < n_clients; i++)
	{
		conn_data = (CONN_DATA *)&conn_data_blks[i];
		conn_data->pce = pce ;
		conn_data->send_parms->pce = conn_data->pce ;
		conn_data->rcv_parms->pce = conn_data->pce ;
		conn_data->lsn_parms->pce = conn_data->pce ;
	}

	/*
	** We have 4 conn_data's but since only
	** one GCC_OPEN suffices, we do that now
	*/
	conn_data = (CONN_DATA *)&conn_data_blks[0];
	parms  = conn_data->lsn_parms;
	pce    = conn_data->pce ;

	/* Fill in the OPEN parms. For now, the LU62 tp_name
	** etc. are found through INGRES environment variables
	*/
	parms->compl_exit = (VOID(*)(PTR))server_listen ;
	parms->compl_id = (PTR)conn_data_blks ;
	parms->function_invoked = GCC_OPEN ;

	if (*port)
		parms->function_parms.open.port_id  = port ;

	status = (*pce->pce_protocol_rtne)(GCC_OPEN, parms);
		
	if (verbose_it == TRUE)
		fprintf(stderr,"%d do_server: GCC_OPEN \n", pid);

	GCexec();
}

/*
** This is the server's GCC_OPEN callback. In here, we  do a GCC_LISTEN 
** waiting for connections from clients
*/
static	VOID
server_listen(CONN_DATA *conn_data)
{
	GCC_P_PLIST 	*parms  = conn_data->lsn_parms;
	GCC_PCE		*pce = conn_data->pce ;
	int	i = 0 ;

	if(parms->generic_status != OK)
	{
		error("server_listen", conn_data, parms);
		close_conn(conn_data);
		return;
	}
	
	if (parms->function_parms.open.lsn_port)
		fprintf(stderr,"%s: server_listen at port %s\n", whoami, 
			parms->function_parms.open.lsn_port);

	num_open_connections = n_clients;

	/* Fill in the LISTEN parms
	*/
	parms->compl_exit = (VOID(*)(PTR))listen_callback ;
	parms->compl_id = (PTR)conn_data ;
	parms->function_parms.listen.node_id=NULL;
	parms->function_invoked = GCC_LISTEN ;

	(*pce->pce_protocol_rtne)(GCC_LISTEN, parms);
	if (verbose_it == TRUE)
		fprintf(stderr,"%d server_listen: GCC_LISTEN \n", pid);

}


static VOID
listen_callback(CONN_DATA *conn_data)
{
	GCC_PCE		*pce = conn_data->pce ;
	GCC_P_PLIST 	*parms  = conn_data->lsn_parms;

	if(parms->generic_status != OK)
	{
		error("listen_callback", conn_data, parms);
		conn_data->recv_close_conn = TRUE;
		conn_data->send_close_conn = TRUE;
		close_conn(conn_data);
	}

	if (verbose_it == TRUE)
	{
		fprintf(stderr,"%d GClisten: connected\n", pid);
		if(parms->function_parms.listen.node_id)
			fprintf(stderr,"%d GClisten: partner name is '%s'\n",
				pid, parms->function_parms.listen.node_id);
	}
	num_lisns_posted++;

	conn_data->recv_close_conn = FALSE;
	conn_data->send_close_conn = FALSE;
	conn_data->n_read_packets = 0; 
	conn_data->n_write_packets = 0;
	conn_data->send_parms->pcb = conn_data->rcv_parms->pcb 
				= conn_data->lsn_parms->pcb;

	if (num_lisns_posted < n_clients )
	{
		GCC_P_PLIST 	*new_lsn_parms ;
		CONN_DATA	*conn_data;

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

		new_lsn_parms = conn_data->lsn_parms ;
		new_lsn_parms->compl_exit = (VOID(*)(PTR))listen_callback ;
		new_lsn_parms->compl_id = (PTR)conn_data ;
		new_lsn_parms->function_invoked = GCC_LISTEN ;
		(*pce->pce_protocol_rtne)(GCC_LISTEN, new_lsn_parms);
	}
	receive_packet(conn_data); 
	send_packet(conn_data);  
}

static VOID
send_packet(CONN_DATA *conn_data )
{
	GCC_PCE		*pce = conn_data->pce ;
	GCC_P_PLIST 	*parms  = conn_data->send_parms;

	/*
	** send a packet
	*/
	if (verbose_it == TRUE)
		fprintf(stderr,"%d: %s sending packet....\n", pid, whoami);

	parms->buffer_ptr = (conn_data->write_packet + PADDING);
	parms->buffer_lng  =  write_size ;
	parms->compl_exit = (VOID(*)(PTR))send_callback ;
	parms->compl_id = (PTR)conn_data ;
	(*pce->pce_protocol_rtne)(GCC_SEND, parms);
}

static VOID
send_callback(CONN_DATA *conn_data)
{
	GCC_P_PLIST 	*parms  = conn_data->send_parms;

	if(parms->generic_status != OK)
	{
		error("send_callback", conn_data, conn_data->send_parms);
		conn_data->send_close_conn = TRUE;
		close_conn(conn_data);
		return;
	}
	else
	{
		conn_data->n_write_packets++;
		if (verbose_it == TRUE)
		  fprintf(stderr,"%d: %s done sending packet %d\n", pid, whoami,
				conn_data->n_write_packets);
	}

	if (conn_data->n_write_packets == num_packets)
	{
		if (verbose_it == TRUE)
			fprintf(stderr,"%d: %s done sending %d packets\n",
		        pid, whoami, conn_data->n_write_packets);
		conn_data->send_close_conn = TRUE;
		if (conn_data->recv_close_conn ==  TRUE) 
			close_conn(conn_data);
		return;
	}
	else	
	{
		i4 timeout=100;
		if (verbose_it == TRUE)
			fprintf(stderr,"%d: %s still sending packets, waiting a bit.\n",
		        pid, whoami);
		iiCLpoll(&timeout);
		send_packet(conn_data);
	}
}

static VOID
receive_packet(CONN_DATA *conn_data )
{
	GCC_P_PLIST 	*parms  = conn_data->rcv_parms;
	GCC_PCE		*pce = conn_data->pce ;

	/*
	** receive a packet 
	** read_size is always >= write_size 
	*/
	MEfill(read_size, 0, conn_data->read_packet + PADDING);
	conn_data->n_bytes_read = 0;

	parms->buffer_ptr = (conn_data->read_packet+PADDING);
	parms->buffer_lng  =  read_size ;
	parms->compl_exit = (VOID(*)(PTR))receive_callback ;
	parms->compl_id = (PTR)conn_data ;
	parms->function_invoked = GCC_RECEIVE ;
	if(verbose_it == TRUE)
		fprintf(stderr,"%s: Initiating Receive:\n", whoami);
	(*pce->pce_protocol_rtne)(GCC_RECEIVE, parms);
}

static VOID
receive_callback(CONN_DATA *conn_data)
{
	GCC_P_PLIST 	*parms  = conn_data->rcv_parms;

	if(parms->generic_status != OK)
	{
		if( conn_data->n_read_packets < num_packets )
			error("receive_callback", conn_data, conn_data->rcv_parms);
		conn_data->recv_close_conn = TRUE;
		close_conn(conn_data);
		return;
	}

	/* received packet */
	if (parms->buffer_lng )
	{
		if (verbose_it == TRUE)
			fprintf(stderr,"%s: recvd packet size %d\n", whoami, parms->buffer_lng );
		conn_data->n_bytes_read = parms->buffer_lng;
		if ( conn_data->n_bytes_read != write_size)
		{
			fprintf(stderr,"recv_callback: packet has %d bytes\n", 
				conn_data->n_bytes_read);
			error("receive_callback", conn_data, conn_data->rcv_parms);
			conn_data->recv_close_conn = TRUE;
			close_conn(conn_data);
			return;
		}
		conn_data->n_read_packets++;
		if (verify_read_packet(conn_data->read_packet + PADDING, 
			parms->buffer_lng, conn_data) != TRUE)
		{
			fprintf(stderr,"recv_callback: error-verify_packet\n");
			exit(3);
		}

		if(verbose_it == TRUE)
		{
			fprintf(stderr,"%d:%s received msg %d: (%d)\n", pid, 
			   whoami, conn_data->n_read_packets, parms->buffer_lng);
		}
	}
	else
	{
		fprintf(stderr," recv_callback: error - 0 bytes recvd\n");
	}

	if (conn_data->n_read_packets == num_packets) 
	{
		if (verbose_it == TRUE)
			fprintf(stderr,"%d: %s done receiving  %d packets\n", 
				pid, whoami, conn_data->n_read_packets);
		conn_data->recv_close_conn = TRUE ;
		if (conn_data->send_close_conn == TRUE) 
			close_conn(conn_data);
		return;
	}
	receive_packet(conn_data);
}

static VOID
close_conn(CONN_DATA *conn_data)
{
	GCC_P_PLIST 	*parms  = conn_data->send_parms;
	GCC_PCE		*pce = conn_data->pce ;

	if (verbose_it ==  TRUE)
		fprintf(stderr,"%d %s closing  connection\n", pid, whoami);

	parms->compl_exit = (VOID(*)(PTR))close_callback;
	parms->compl_id = (PTR)conn_data;
	parms->function_invoked = GCC_DISCONNECT ;

	(*pce->pce_protocol_rtne)(GCC_DISCONNECT, parms);
}

static VOID
close_callback(CONN_DATA *conn_data)
{
	GCC_P_PLIST 	*parms  = conn_data->send_parms;

	if(parms->generic_status != OK)
	{
		exit_stat = parms->generic_status ; 
		error("close_callback", conn_data, conn_data->send_parms);
	}
	if (verbose_it == TRUE)
		fprintf(stderr,"%d:%s done closing connection:status %d/0x%x\n",
		pid, whoami, exit_stat, exit_stat);

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
}

/*
** Allocate GCC_P_PLIST and tie in pointers
*/
static  VOID
do_ds(CONN_DATA *conn_bk)
{
	GCC_P_PLIST 	*send_parms, *rcv_parms , *lsn_parms;
	int			i = 0;
	GCC_PCT		*pct = (GCC_PCT *)GCC_alloc(sizeof(*pct));
	
	conn_bk->pct = pct ;
	send_parms = (GCC_P_PLIST *)GCC_alloc(sizeof(*send_parms));
	rcv_parms = (GCC_P_PLIST *)GCC_alloc(sizeof(*rcv_parms));
	lsn_parms = (GCC_P_PLIST *)GCC_alloc(sizeof(*lsn_parms));
	conn_bk->send_parms = send_parms ;
	conn_bk->rcv_parms = rcv_parms ;
	conn_bk->lsn_parms = lsn_parms ;
	return;
}

/*
** Return  protocol pce, if not found returns NULL
*/
static  GCC_PCE *
get_pce(CONN_DATA *conn_bk, char *protocol )
{
	GCC_PCE		*pce = 0;
	int		i = 0;
	
	for (i = IIGCc_gcc_pct.pct_n_entries ; i--;  )
	{
		pce = &IIGCc_gcc_pct.pct_pce[i] ;
		if (!STcompare(pce->pce_pid,protocol))
			return pce ;
	}
	return 0;
}

static  VOID
fill_write_packet(PTR write_packet, int wr_size )
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
verify_read_packet(PTR read_packet, int r_size,  CONN_DATA *conn_data)
{
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
