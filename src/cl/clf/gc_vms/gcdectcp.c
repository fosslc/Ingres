/*
** Copyright (c) 1988, 2008 Ingres Corporation
*/

# include	<compat.h>             
# include	<gl.h>
# include       <gc.h>
# include	<cm.h>
# include	<ck.h>                                        
# include	<me.h>
# include	<st.h>
# include       <tr.h>
# include	<gcccl.h>
# include	<gcatrace.h>
/*
** Undefine of __CAN_USE_EXTERN_PREFIX is required for a bug in netdb.h in 
** VMS prior to 7.3.2, (rename of getnameinfo() to __bsd44_getnameinfo()),
** but since getnameinfo() is not currently used, this is commented out.
*/
//# undef __CAN_USE_EXTERN_PREFIX
# include       <descrip.h>
# include       <ssdef.h>
# include       <efndef.h>
# include       <iledef.h>
# include       <iodef.h>
# include       <iosbdef.h>
# include	<lib$routines.h>
# include	<starlet.h>
# include	<vmstypes.h>

# include       <in.h>
# include       <inet.h>
# include       <netdb.h>          
# include       <tcpip$inetdef.h>  

# include       <astjacket.h>

# ifndef	E_BS_MASK
# define	E_BS_MASK 0
# endif
# include	<bsi.h>

#ifndef MAXHOSTNAME
#define MAXHOSTNAME     64
#endif

FUNC_EXTERN  STATUS 	GC_tcp_port();
FUNC_EXTERN  STATUS	GC_tcp_addr();
FUNC_EXTERN  STATUS     GC_tcp6_set_socket( char *buf, struct sockaddr_in6 *s);
FUNC_EXTERN  STATUS     GC_tcp6_addr( char *buf, struct addrinfo **addrinfo );

struct	itlst 
{
    int	len;
    struct sockaddr_in *hst;
};

struct itlst_v6
{
    int len;
    struct sockaddr_in6 *hst;
};            

struct sockchar 
{                  /* socket characteristics           */ 
    u_i2 prot;     /* protocol                         */ 
    u_char type;   /* type                             */ 
    u_char af;     /* address format                   */ 
}; 


/*
** Name: gcdectcp.c - BS interface to DEC's TCP/IP (UCX) via VMS QIO's.
**
** Description:
**	This file provides access to TCP IPC via VMS QIO calls, using the 
**	generic BS interface defined in bsi.h.  See that file for a 
**	description of the interface.
**
**	The following functions are exported as part of the BS interface:
**
**		tcp_accept - accept a single incoming connection
**		tcp_close - close a single connection
**		tcp_listen - establish an endpoint for accepting connections
**		tcp_receive - read data from a stream
**		tcp_request - make an outgoing connection
**		tcp_send - send data down the stream
**		tcp_unlisten - stop accepting incoming connections
**		tcp_ok - dummy function returning OK
**
** History:
**	21-Jan-91 (seiwald)
**	    Ported.
**	05-apr-91 (Hasty)
**	    Wollongong tcp driver modified for UCX
**	15-oct-91 (Hasty)
**	    Added keepalive and reuse address feature.
**	    Added tcp_close_0 to support the change of synchronous
**	    close operation to asynchronous close operation.
**	13-Jul-92 (alan) bug #45173
**	    Release resources if tcp_listen fails.
**	04-Mar-93 (edg)
**	    Detect no longer in BS_DRIVER.
**      16-jul-93 (ed)
**	    added gl.h
**	03-Mar-98 (loera01) Bug 89361
**	    In tcp_listen(), removed UCX$M_REUSEADDR flag from setmode
**	    qio.  More recent versions of UCX allow multiple listens on
**	    single ports, and this was disabling the ability of the GCC
**	    to increment listen addresses.
**      23-mar-98 (kinte01)
**          undef'ed u_short. With the addition of this defn to compat.h,
**          it is now defined twice as it is also in twgtypes.h
**      11-dec-98 (kinte01)
**          In tcp_close_0, status was never initialized so the if
**          statement checking the status was bogus. Flagged as a warning
**          with the DEC C 6.0 compiler.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	28-aug-2001 (kinte01)
**	   Remove unneeded twgtypes.h
**	26-oct-2001 (kinte01)
**	   clean up more compiler warnings
**      05-dec-2002 (loera01) SIR 109246
**         For tcp_accept_1, invoke the system services equivalent of 
**         getsockname() and getpeername() to determine the IP addresses
**         of the client and server.  Set the is_remote flag in BS_PARMS
**         accordingly.
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**      07-May-2004 (ashco01) Problem: INGNET99, Bug: 107833
**          Introduced GCC_lsn_chan global ref to store IIGCC listen channel
**          for use during IIGCC process shutdown to ensure that all queued
**          IO is discarded and the listen port is shutdown prior to IIGCC
**          process termination.
**      25-apr-2005 (loera01) Bug 114395
**          In tcp_accept_1(), removed the LIB$GET_EF() call, since event
**          flag 0 may be used for this blocking QIOW call.
**      16-Jan-2007 (Ralph Loen) SIR 116548
**          Added tcp6_listen(), tcp6_request(), tcp6_accept(), and 
**          tcp6_connect(), tcp6_set_trace() to support IPv6.
**      01-Aug-2007 (Ralph Loen) SIR 118898
**          Added GCdns_hostname().
**      09-Aug-2007 (Ralph Loen) SIR 118898
**          Add gc.h to include error numbers for GCdns_hostname().
**      12-Sep-2007 (Ralph Loen) Bug 119117
**          In tcp6_request(), initialize the local addrinfo pointer to
**          NULL to that freaddrinfo() doesn't try to free a garbage
**          pointer.
**      28-Sep-2007 (Ralph Loen) Bug 119215
**          Reverted SOCKADDRIN6 overlay to sockaddr_in6 and used
**          SIN6$C_LENGTH (28) since sizeof(sockaddr_in6) evaluates
**          incorrectly. In tcp6_request(), break from loop when a 
**          connection is successful.  Fixed several erroneous "%p"
**          specifiers in tracing ipv6.
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
**      14-Nov-2009 (Ralph Loen) Bug 122787
**          In tcp_listen() and tcp6_listen(), set the TCPIP$C_REUSEADDR
**          flag when listening, so that the server can listen upon restart 
**          if sockets remain from the previous server instance.  Remove
**          GLOBALREF GCC_lsn_chan.
*/

typedef long VMS_STATUS;

/*
** Name: LBCB, BCB - listen control block, connection control block
*/

typedef struct _LBCB
{
	II_VMS_EF_NUMBER efn;		/* event flag for open function */
	II_VMS_CHANNEL	chan;		/* channel for open function */
	IOSB	iosb;			/* iosb for open function */
	char	port[128];		/* printable port name */
} LBCB;

typedef struct _BCB
{
	II_VMS_CHANNEL	chan;		/* channel for I/O */
	IOSB	riosb;			/* iosb for receives */
	IOSB	siosb;			/* iosb for sends */
        struct addrinfo *addrinfo;      /* address linked list for ipv6 */
} BCB;

# define LISTEN_QUE_DEPTH 50		/* 50 incoming conns at once */

#define  TCP_TRACE(n)  if(tcp_trace >= n)(void)TRdisplay
static i4 tcp_trace=0;

/*
** Name: tcp_devname - internet device name
*/

static struct dsc$descriptor_s tcp_devname = 
{
	3, 0, 0, "BG:" 
} ;

static VOID tcp_accept_1();
static VOID tcp_request_0();
static VOID tcp_request_1();
static VOID tcp_send_0();
static VOID tcp_receive_0();
static VOID tcp_close_0();
static VOID tcp6_accept_1();
static VOID tcp6_connect();
static VOID tcp6_set_trace();


/* acp implementation of gethostbyname */

static struct hostent *
agethostbyname(hostname)
char *hostname;
{

    static struct hostent hostent;
    static char *h_addr_list;
    static u_long addr;


	int status;
	IOSB	iosb;
	II_VMS_CHANNEL channel;
	short retlen;

/* 
** ACP command to request the binary internet address of a host name
*/
	int comm = INETACP$C_TRANS * 256 + INETACP_FUNC$C_GETHOSTBYNAME;

	int addr_buff[8];

/* descriptor for ACP command */
	struct	dsc$descriptor acp_command = 
	  { 0, DSC$K_CLASS_S, DSC$K_DTYPE_T, 0};

/* descriptor for host name */
	struct	dsc$descriptor hostname_dsc = 
	  { 0, DSC$K_CLASS_S, DSC$K_DTYPE_T, 0};

/* descriptor for returning host address */
	struct	dsc$descriptor host_ad = 
	  { 0, DSC$K_CLASS_S, DSC$K_DTYPE_T, 0};


/*
* INET ACP command 
*/

	acp_command.dsc$a_pointer = (PTR)&comm;
	acp_command.dsc$w_length = sizeof(comm);

/*
** descriptor for ACP output
*/

	host_ad.dsc$a_pointer = (PTR)&addr;
	host_ad.dsc$w_length  = sizeof(addr);

/* Descriptor for host name */

	hostname_dsc.dsc$a_pointer = hostname;
	hostname_dsc.dsc$w_length = STlength(hostname);

/*
** Assign a channel to INET device
*/


	status = sys$assign(&tcp_devname, &channel, 0, 0);

	if ((status & 1) == 0)
	{
	/* failed to assigned INET device */
	   return 0;
	}

/*
**	Get the host address (longword)
*/

	status = sys$qiow(EFN$C_ENF,
			  channel,
			  IO$_ACPCONTROL,
			  &iosb,
			  0,
  			  0,
			  &acp_command,		/* P1 command */
			  &hostname_dsc,           /* P2 host name */
			  &hostent.h_length,    /* P3 adrs to return length */
			  &host_ad,             /* P4 adrs output */
			  0, 0);			

	if ((( status & 1) && (iosb.iosb$w_status & 1)) == 0)
	{
	  sys$dassgn(channel);
	  return 0;
	}


	hostent.h_addr_list = &h_addr_list;
	h_addr_list = (PTR)&addr;

/*
** Return the host internet address
*/

	sys$dassgn(channel);

	return (&hostent);
}


/*
** BS entry points
*/

/*
** Name: tcp_listen - establish an endpoint for accepting connections
*/

static VOID
tcp_listen( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;

	VMS_STATUS              status;
	struct	sockaddr_in	s[1];
        struct sockchar         sck_parm;
	struct itlst		lhst_adrs;
 

	/* clear chan for sanity */

	lbcb->chan = -1;
	lbcb->efn = -1;

	/* Get an event flag to use */

	if( ( status = lib$get_ef( &lbcb->efn ) ) != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}


	/*  Assign channel to tcp device bg0  */

	status = sys$assign( &tcp_devname, &lbcb->chan, 0, 0 );
	
	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Create and bind a socket  */
	lhst_adrs.len = sizeof(s);
	lhst_adrs.hst = &s;

   	/* get listen port, if specified */

	if( ( bsp->status = GC_tcp_addr( bsp->buf, FALSE,  &agethostbyname, s ))
             != OK )
	{
	    bsp->syserr->error = 0;
	    goto complete;
	}
  
	sck_parm.prot = INET$C_TCP;		/* TC/IP protocol */
	sck_parm.type = INET_PROTYP$C_STREAM;
        sck_parm.af   = INET$C_AF_INET;
  
	status = sys$qiow( 
			lbcb->efn, 
			lbcb->chan, 
			IO$_SETMODE,
			&lbcb->iosb, 
			0, 
			0, 
			&sck_parm, 
			0x01000000 | TCPIP$M_KEEPALIVE | TCPIP$C_REUSEADDR,
			&lhst_adrs, 
			LISTEN_QUE_DEPTH,			
			0, 0 );      

	/* get name for output */

	STcopy( bsp->buf, lbcb->port );
	bsp->buf = lbcb->port;

	if(( status != SS$_NORMAL ) ||
	   ( status = lbcb->iosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = BS_LISTEN_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	return;

    complete:
	/* On failure, release resources */

	if( lbcb->chan != -1 )
	    sys$dassgn( lbcb->chan );

	if( lbcb->efn != -1 )
	    lib$free_ef( &lbcb->efn );

	lbcb->chan = -1;

        return;
}


/*
** Name: tcp6_listen - establish an endpoint for accepting connections
*/

static VOID
tcp6_listen( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;

	VMS_STATUS              status;
	struct sockaddr_in6	s6, *s = &s6;
        struct sockchar         sck_parm;
	struct itlst_v6		lhst_adrs;
 
        tcp6_set_trace();

        TCP_TRACE(2)("tcp6_listen: %p entered  port='%s'\n", bsp->closure,
            bsp->buf);

	/* clear chan for sanity */

	lbcb->chan = -1;
	lbcb->efn = -1;

        /* Use the default "user" event flag */

        lbcb->efn = 0;

	/*  Assign channel to tcp device bg0  */

	status = sys$assign( &tcp_devname, &lbcb->chan, 0, 0 );
	
	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Create and bind a socket  */
	lhst_adrs.len = SIN6$C_LENGTH;
	lhst_adrs.hst = s;

   	/* get listen port, if specified */

	if( ( bsp->status = GC_tcp6_set_socket( bsp->buf, s)) != OK )
	{
	    bsp->syserr->error = 0;
	    goto complete;
	}
  
        sck_parm.prot = INET$C_TCP;		/* TC/IP protocol */
        sck_parm.type = INET_PROTYP$C_STREAM;
        sck_parm.af   = INET$C_AF_INET6;        /* IPv6 socket family */
  
        status = sys$qiow( 
			lbcb->efn, 
			lbcb->chan, 
			IO$_SETMODE,
			&lbcb->iosb, 
			0, 
			0, 
			&sck_parm, 
			0x01000000 | TCPIP$M_KEEPALIVE | TCPIP$C_REUSEADDR,
			&lhst_adrs, 
			LISTEN_QUE_DEPTH,			
			0, 0 );      

	/* get name for output */

	STcopy( bsp->buf, lbcb->port );
	bsp->buf = lbcb->port;

	if(( status != SS$_NORMAL ) ||
	   ( status = lbcb->iosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = BS_LISTEN_ERR;
	    bsp->syserr->error = status;
            if( bsp->status != OK )
            {
                TCP_TRACE(1)
                    ("tcp6_listen: failure from sys$qio(), status=%x\n",status);
	        goto complete;
            }
	}

	return;

    complete:
	/* On failure, release resources */

	if( lbcb->chan != -1 )
	    sys$dassgn( lbcb->chan );

	if( lbcb->efn != -1 )
	    lib$free_ef( &lbcb->efn );

	lbcb->chan = -1;

        return;
}

/*
** Name: tcp_unlisten - stop accepting incoming connections
**                      (not currently driven by VMS BS layer)
*/
	
static VOID
tcp_unlisten( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;


	if( lbcb->chan != -1 )
	    sys$dassgn( lbcb->chan );

	if( lbcb->efn != -1 )
	    lib$free_ef( &lbcb->efn );

	lbcb->chan = -1;
}

/*
** Name: tcp_accept - accept a single incoming connection
*/

static VOID
tcp_accept( bsp )
BS_PARMS	*bsp;     
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;
	VMS_STATUS              status;

	/* clear chan for sanity */

 	bcb->chan = -1;

 	bcb->chan = -1;

        /* Open Inet device  */

	status = sys$assign( &tcp_devname, &bcb->chan, 0, 0);

	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_ACCEPT_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

 	/*  Post an outstanding listen  */

	status = sys$qio(
		lbcb->efn, 
		lbcb->chan, 
		IO$_ACCESS | IO$M_ACCEPT,
		&bcb->riosb, 
		tcp_accept_1, 
		bsp, 
		0, 0, 0,
		&bcb->chan,
		0, 0);

	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_ACCEPT_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	return;

    complete:
	/* On failure drive completion */

	(*(bsp->func))( bsp->closure );
}

static VOID
tcp_accept_1( bsp )
BS_PARMS	*bsp;
{
    BCB	                    *bcb = (BCB *)bsp->bcb;
    VMS_STATUS              status;
    u_i4 peer_inaddr, socket_inaddr;
    u_i4 peer_addrlen;               /* returned length of client socket */ 
    struct sockaddr_in peer_addr;    /* client socket address structure  */ 
    ILE3	       peer_itemlst;     /* client socket address item-list  */ 

    u_i4 socket_addrlen;             /* returned length of server socket */ 
    struct sockaddr_in socket_addr;  /* server socket address structure  */ 
    ILE3	       socket_itemlst;   /* server socket address item-list  */ 

    /*  Check completion status of accept  */

    if( ( status = bcb->riosb.iosb$w_status ) != SS$_NORMAL)
    {
        bsp->status = BS_ACCEPT_ERR;
        bsp->syserr->error = status;
        goto complete;
    }

    bsp->is_remote = TRUE;
 
    /* 
    **  Initialize item-list descriptors.
    */ 
    MEfill( sizeof(peer_addr), 0, &peer_addr );
    MEfill( sizeof(socket_addr), 0, &socket_addr);

    peer_itemlst.ile3$w_length = sizeof( peer_addr ); 
    peer_itemlst.ile3$w_code = INET$C_SOCK_NAME;
    peer_itemlst.ile3$ps_bufaddr = &peer_addr; 
    peer_itemlst.ile3$ps_retlen_addr = &peer_addrlen; 

    socket_itemlst.ile3$w_length = sizeof( socket_addr ); 
    socket_itemlst.ile3$w_code = INET$C_SOCK_NAME; 
    socket_itemlst.ile3$ps_bufaddr = &socket_addr; 
    socket_itemlst.ile3$ps_retlen_addr = &socket_addrlen; 
  
    status = sys$qiow(EFN$C_ENF,                 /* event flag */ 
		         bcb->chan,           /* i/o channel */ 
                         IO$_SENSEMODE,       /* i/o function code */ 
                         &bcb->riosb,         /* i/o status block */ 
                         0,                   /* ast service routine */ 
                         0,                   /* ast parameter */ 
                         0,                   /* p1 */ 
                         0,                   /* p2 */ 
                         &socket_itemlst,     /* p3 */ 
                         &peer_itemlst,        /* p4 - peer socket name */ 
                         0,                   /* p5 */ 
                         0                    /* p6 */ 
                     ); 
    if (status & 1)
	status = bcb->riosb.iosb$w_status;
    if( status != SS$_NORMAL )
    { 
        bsp->status = BS_SOCK_ERR;
	bsp->syserr->error = status;
	goto complete;
    } 

    MEcopy(&peer_addr.sin_addr, sizeof(i4), &peer_inaddr);
    MEcopy(&socket_addr.sin_addr, sizeof(i4), &socket_inaddr);
    if ( socket_inaddr == peer_inaddr )
        bsp->is_remote = FALSE;

    /* Turn on Keepalives */

    status = sys$qio(
                        0, 
                        bcb->chan,
                        IO$_SETMODE,
                        &bcb->siosb,
                        0,0,0,
			0x01000000 | TCPIP$M_KEEPALIVE,
                        0, 0, 0, 0 );

    if( status != SS$_NORMAL ||
            ( status = bcb->riosb.iosb$w_status ) != SS$_NORMAL )
    {
            bsp->status = BS_ACCEPT_ERR;
            bsp->syserr->error = status;
    }

    (*(bsp->func))( bsp->closure );
    return;

complete:
    /* 
    ** On failure, release resources and drive the completion routine.
    */
    bcb->chan = -1;

    (*(bsp->func))( bsp->closure );
}

/*
** Name: tcp_request - make an outgoing connection
*/

static VOID
tcp_request( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;

	VMS_STATUS              status;
        short			sck_parm[2];
	/* clear chan for sanity */

	bcb->chan = -1;

	/*  Assign channel to tcp device _bg0  */

	status = sys$assign( &tcp_devname, &bcb->chan, 0, 0 );
	
	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Create a socket  */

	sck_parm[0] = IPPROTO_TCP;		/* TC/IP protocol */
	sck_parm[1] = SOCK_STREAM;
  
 
	status = sys$qio(
			0, 
			bcb->chan,
			IO$_SETMODE,
			&bcb->siosb,
			tcp_request_0,
			bsp,
			&sck_parm,
			0x01000000 |  SO_KEEPALIVE,
			 0, 0, 0, 0 );


	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	return;

    complete:
	/* On failure drive completion */

	(*(bsp->func))( bsp->closure );
}

static VOID
tcp_request_0( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;

	VMS_STATUS              status;
	struct	sockaddr_in	s[1];
	struct itlst		lhst_adrs;
 
	/* Check status from socket creation */

	if( ( status = bcb->siosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	lhst_adrs.len = sizeof(s);
	lhst_adrs.hst = s;


	/* translate address */
	if( GC_tcp_addr( bsp->buf, TRUE,  &agethostbyname, s ) != OK )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Now do the actual connect  */
	status = sys$qio(
			0,
			bcb->chan,
			IO$_ACCESS,
			&bcb->siosb,
			tcp_request_1,
			bsp,
			0,
			0,
			&lhst_adrs, 0, 0, 0 );

	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_CONNECT_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}
	
	return;

    complete:
	/* On failure drive completion */

	(*(bsp->func))( bsp->closure );
}

static VOID
tcp_request_1( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;

	VMS_STATUS              status;

	/*  Check completion status of accept  */

	if( ( status = bcb->siosb.iosb$w_status ) != SS$_NORMAL)
	{
	    bsp->status = BS_CONNECT_ERR;
	    bsp->syserr->error = status;
	}

	/*  Drive the completion routine  */

	(*(bsp->func))( bsp->closure );
}

/*
** Name: tcp6_accept - accept a single incoming connection
*/

static VOID
tcp6_accept( bsp )
BS_PARMS	*bsp;     
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;
	VMS_STATUS              status;

	/* clear chan for sanity */

 	bcb->chan = -1;

        TCP_TRACE(2)("tcp6_accept: %p entered\n", bsp->closure);

        /* Open Inet device  */

	status = sys$assign( &tcp_devname, &bcb->chan, 0, 0);

	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_ACCEPT_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

 	/*  Post an outstanding listen  */

	status = sys$qio(
		lbcb->efn, 
		lbcb->chan, 
		IO$_ACCESS | IO$M_ACCEPT,
		&bcb->riosb, 
		tcp6_accept_1, 
		bsp, 
		0, 0, 0,
		&bcb->chan,
		0, 0);

	if( status != SS$_NORMAL )
	{
            TCP_TRACE(1)("tcp6_accept: failure from sys$qio(), status=%x\n",
                status);
	    bsp->status = BS_ACCEPT_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	return;

    complete:
	/* On failure drive completion */

	(*(bsp->func))( bsp->closure );
}

static VOID
tcp6_accept_1( bsp )
BS_PARMS	*bsp;
{
    BCB	                    *bcb = (BCB *)bsp->bcb;
    VMS_STATUS              status;
    u_i4 peer_addrlen;               /* returned length of client socket */ 
    ILE3	peer_itemlst;     /* client socket address item-list  */ 
    struct sockaddr_in6 peer_addr, socket_addr; 
    u_i4 socket_addrlen;             /* returned length of server socket */ 
    ILE3	socket_itemlst;   /* server socket address item-list  */ 

    /*  Check completion status of accept  */
    if( ( status = bcb->riosb.iosb$w_status ) != SS$_NORMAL)
    {
        bsp->status = BS_ACCEPT_ERR;
        bsp->syserr->error = status;
        goto complete;
    }

    bsp->is_remote = TRUE;
 
    /* 
    **  Initialize item-list descriptors.
    */ 
    MEfill( SIN6$C_LENGTH, 0, &peer_addr );
    MEfill( SIN6$C_LENGTH, 0, &socket_addr);

    peer_itemlst.ile3$w_length = sizeof( peer_addr ); 
    peer_itemlst.ile3$w_code = INET$C_SOCK_NAME;
    peer_itemlst.ile3$ps_bufaddr = &peer_addr; 
    peer_itemlst.ile3$ps_retlen_addr = &peer_addrlen; 

    socket_itemlst.ile3$w_length = sizeof( socket_addr ); 
    socket_itemlst.ile3$w_code = INET$C_SOCK_NAME; 
    socket_itemlst.ile3$ps_bufaddr = &socket_addr; 
    socket_itemlst.ile3$ps_retlen_addr = &socket_addrlen; 
  
    status = sys$qiow(EFN$C_ENF,                 /* event flag */ 
		         bcb->chan,           /* i/o channel */ 
                         IO$_SENSEMODE,       /* i/o function code */ 
                         &bcb->riosb,         /* i/o status block */ 
                         0,                   /* ast service routine */ 
                         0,                   /* ast parameter */ 
                         0,                   /* p1 */ 
                         0,                   /* p2 */ 
                         &socket_itemlst,     /* p3 */ 
                         &peer_itemlst,        /* p4 - peer socket name */ 
                         0,                   /* p5 */ 
                         0                    /* p6 */ 
                     ); 
    if (status & 1)
	status = bcb->riosb.iosb$w_status;
    if( status != SS$_NORMAL )
    { 
        bsp->status = BS_SOCK_ERR;
	bsp->syserr->error = status;
	goto complete;
    }

    /*
    ** Commenting getnameinfo() for now, since it's only required for
    ** filling out the node_id field in the listen parameters.
    ** This is not currently supported on VMS.
    */
    /*
    char node[MAXHOSTNAME];
    status = getnameinfo( (struct sockaddr *)&peer_addr, 
        peer_addrlen, node, sizeof(node), NULL, 0, 
        NI_NAMEREQD | NI_NOFQDN );

    if ( status )
    {
       bsp->status = BS_SOCK_ERR;
       bsp->syserr->error = status;
       goto complete;
    }
    */
    if (IN6_ARE_ADDR_EQUAL(
        (struct in6_addr *)&peer_addr.sin6_addr, 
        (struct in6_addr *)&socket_addr.sin6_addr))
        bsp->is_remote = FALSE;

    /* Turn on Keepalives */

    status = sys$qio(
                        0, 
                        bcb->chan,
                        IO$_SETMODE,
                        &bcb->siosb,
                        0,0,0,
			0x01000000 | TCPIP$M_KEEPALIVE,
                        0, 0, 0, 0 );

    if( status != SS$_NORMAL ||
            ( status = bcb->siosb.iosb$w_status ) != SS$_NORMAL )
    {
            bsp->status = BS_ACCEPT_ERR;
            bsp->syserr->error = status;
    }

    (*(bsp->func))( bsp->closure );
    return;

complete:
    /* 
    ** On failure, release resources and drive the completion routine.
    */
    bcb->chan = -1;

    (*(bsp->func))( bsp->closure );
}

/*
** Name: tcp6_request - make an outgoing connection
*/

static VOID
tcp6_request( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
        struct  addrinfo *addrinfo = NULL;
	VMS_STATUS              status;

        tcp6_set_trace();

        TCP_TRACE(2)("tcp6_request: %p entered  target='%s'\n",
             bsp->closure, bsp->buf);

	/* translate address */
        bcb->addrinfo = NULL;
	if( bsp->status = GC_tcp6_addr( bsp->buf, &bcb->addrinfo ) != OK )
	{
	    bsp->syserr->error = 0;
	    goto complete;
	}
        addrinfo = bcb->addrinfo;

        for( ; bcb->addrinfo; bcb->addrinfo = bcb->addrinfo->ai_next )
        {
            /*
            ** Attempt to connect on the next ipv6 or ipv4/ipv6-mapped 
            ** address presented on the address list.
            */
            if (bcb->addrinfo->ai_family != AF_INET6)
                continue;

            tcp6_connect(bsp);
            if (bsp->status == OK)
                break;
        }

    complete:

        if (addrinfo)
            freeaddrinfo(addrinfo);

	/* Drive completion */
	(*(bsp->func))( bsp->closure );
}

/*
** Name: tcp6_connect - make an outgoing connection
*/

static VOID
tcp6_connect( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;

	VMS_STATUS              status;
        struct sockchar  conn_sockchar;
	struct sockaddr_in6	*s;
	struct itlst_v6	lhst_adrs;
        struct addrinfo *addrinfo;
        i4 i;
        struct sockaddr_in6 *s_in6;
        char dst[128]="\0";

	/* clear chan for sanity */

	bcb->chan = -1;

	/*  Assign channel to tcp device _bg0  */

	status = sys$assign( &tcp_devname, &bcb->chan, 0, 0 );
	
	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    return;
	}

	/*  Create a socket  */
        conn_sockchar.prot = TCPIP$C_TCP;
        conn_sockchar.type = TCPIP$C_STREAM;
        conn_sockchar.af   = TCPIP$C_AF_INET6;
 
        status = sys$qiow(EFN$C_ENF, 
			bcb->chan,
			IO$_SETMODE,
			&bcb->siosb,
			0,
			0,
			&conn_sockchar, 
			0x01000000 |  SO_KEEPALIVE, 
			0, 0, 0, 0 );
        if (status & 1)
	    status = bcb->siosb.iosb$w_status;
	if( status != SS$_NORMAL )
	{
            TCP_TRACE(3)("tcp6_connect: failure from sys$qiow(), status=%x\n",
                status);
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    return;
	}

	/* Check status from socket creation */
	if( ( status = bcb->siosb.iosb$w_status ) != SS$_NORMAL )
	{
            TCP_TRACE(2)
                ("tcp6_connect: socket creation failure, IOSB status=%x\n",
                status);

	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    return;
	}

        TCP_TRACE(3)
("tcp6_connect: flags 0x%x, protocol %d, family %d, next addr %p, addr %p\n",
            bcb->addrinfo->ai_flags, bcb->addrinfo->ai_protocol,
            bcb->addrinfo->ai_family, bcb->addrinfo->ai_next, bcb->addrinfo);

        s_in6 = (struct sockaddr_in6 *) bcb->addrinfo->ai_addr;
        inet_ntop(AF_INET6, (void *)&s_in6->sin6_addr, dst, sizeof(dst));

        TCP_TRACE(5) ("tcp6_connect: Getaddrinfo TCP6 addr is %s\n", dst);

        TCP_TRACE(2)
            ("tcp6_connect: Initiated connection to host: %s, port: %d\n",
                bsp->buf, htons(
                   ((struct sockaddr_in6 *)bcb->addrinfo->ai_addr)->sin6_port));

        s = (struct sockaddr_in6 *)bcb->addrinfo->ai_addr;
        MEfill( sizeof(lhst_adrs), 0, &lhst_adrs);
        lhst_adrs.len = SIN6$C_LENGTH;
        lhst_adrs.hst = s;

        if( !s->sin6_port )
        {
            TCP_TRACE(2)("tcp6_connect error: no port returned\n"); 

            bsp->status = BS_SOCK_ERR;
            bsp->syserr->error = status;
            return;
        }

        /*  Now do the actual connect  */
        status = sys$qiow(EFN$C_ENF,
			bcb->chan,
			IO$_ACCESS,
			&bcb->siosb,
			0,
			0,
			0,
			0,
			&lhst_adrs, 0, 0, 0 );
        if (status & 1)
	    status = bcb->siosb.iosb$w_status;
        if( status != SS$_NORMAL )
        {
            TCP_TRACE(3)("tcp6_connect: connect failure, status=%x\n", status);
            bsp->status = BS_CONNECT_ERR;
            bsp->syserr->error = status;
            return;
        }
	
        /*  Check completion status of accept  */
        if( ( status = bcb->siosb.iosb$w_status ) != SS$_NORMAL)
        {
            TCP_TRACE(2)
              ("tcp6_connect: connect completion failure IOSB status=%x\n", 
                 status);

            bsp->status = BS_CONNECT_ERR;
            bsp->syserr->error = status;
        }

        return;
}

/*
** Name: tcp_send - send data down the stream
*/

static VOID
tcp_send( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;

	VMS_STATUS              status;

	/* short sends are OK */

	status = sys$qio(
			0,
			bcb->chan,
			IO$_WRITEVBLK,
			&bcb->siosb, 
			tcp_send_0,
			bsp,
			bsp->buf,
			bsp->len,
			0, 0, 0, 0);

	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_WRITE_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	return;

    complete:
	/* On failure drive completion */

	(*(bsp->func))( bsp->closure );
}

static VOID
tcp_send_0( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;

	VMS_STATUS              status;

	if( ( status = bcb->siosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = GC_SEND_FAIL;
	    bsp->syserr->error = status;
	}
	else
	{
	    bsp->buf += bcb->siosb.iosb$w_bcnt;
	    bsp->len -= bcb->siosb.iosb$w_bcnt;
	}

	(*(bsp->func))( bsp->closure );
}

/*
** Name: tcp_receive - read data from a stream
*/

static VOID
tcp_receive( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;

	VMS_STATUS              status;

	/* short receives are OK */

	status = sys$qio(
			0,
			bcb->chan,
			IO$_READVBLK,
			&bcb->riosb,
			tcp_receive_0,
			bsp,
			bsp->buf,
			bsp->len, 
			0, 0, 0, 0 );

	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_READ_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	return;

    complete:
	/* On failure drive completion */

	(*(bsp->func))( bsp->closure );
}

static VOID
tcp_receive_0( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;

	VMS_STATUS              status;

	/* Check status or for 0 length read (EOF) */

	status = bcb->riosb.iosb$w_status;

	if( status != SS$_NORMAL || bcb->riosb.iosb$w_bcnt == 0 )
	{
	    bsp->status = GC_RECEIVE_FAIL;
	    bsp->syserr->error = status;
	}
	else
	{
	    bsp->buf += bcb->riosb.iosb$w_bcnt;
	    bsp->len -= bcb->riosb.iosb$w_bcnt;
	}

	(*(bsp->func))( bsp->closure );
}

/*
** Name: tcp_close - close a single connection
*/

static VOID
tcp_close( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;

	VMS_STATUS              status;

	if( !bcb )
	    return;

	/* Ignorance is bliss */

	if( bcb->chan >= 0 )
	{
	    status = sys$qio(
			lbcb->efn,
			bcb->chan,
			IO$_DEACCESS,
			&bcb->siosb,
			tcp_close_0, bsp, 0, 0, 0, 0, 0, 0);

	     if( status != SS$_NORMAL)
	     {
	       bsp->status = GC_DISCONNECT_FAIL;
	       bsp->syserr->error = status;
	       (VOID)sys$dassgn( bcb->chan );
	       (*(bsp->func))( bsp->closure );
	     }

	} else 	(*(bsp->func))( bsp->closure );


}


static VOID
tcp_close_0( bsp )
BS_PARMS	*bsp;
{

	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;

	VMS_STATUS              status;

        status = bcb->siosb.iosb$w_status;

	if( status != SS$_NORMAL)
	{
	    bsp->status = GC_DISCONNECT_FAIL;
	    bsp->syserr->error = status;
	}
	(VOID)sys$dassgn( bcb->chan );
	(*(bsp->func))( bsp->closure );
	return;
}

/*       
** Name: tcp_ok - dummy function returning OK
*/

static VOID
tcp_ok( bsp )
BS_PARMS	*bsp;
{
	bsp->status = OK;
}


/*
** Exported driver specification.
*/

GLOBALDEF BS_DRIVER GC_dectcp = {
	sizeof( BCB ),
	sizeof( LBCB ),
	tcp_listen,
	tcp_unlisten,
	tcp_accept,
	tcp_request,
	tcp_ok,		/* connect not needed */
	tcp_send,
	tcp_receive,
	tcp_close,
	tcp_ok,		/* regfd not needed */
	tcp_ok,		/* save not needed */
	tcp_ok,		/* restore not needed */
	GC_tcp_port,	
} ;

GLOBALDEF BS_DRIVER GC_dectcp_v6 = {
	sizeof( BCB ),
	sizeof( LBCB ),
	tcp6_listen,
	tcp_unlisten,
	tcp6_accept,
	tcp6_request,
	tcp_ok,		/* connect not needed */
	tcp_send,
	tcp_receive,
	tcp_close,
	tcp_ok,		/* regfd not needed */
	tcp_ok,		/* save not needed */
	tcp_ok,		/* restore not needed */
	GC_tcp_port,	
} ;


/*
** tcp6_set_trace - sets trace level for TCP.
*/
static VOID
tcp6_set_trace()
{
    static bool init_trace=FALSE;
    char    *trace;

    if(init_trace) 
        return;

    init_trace=TRUE;

    NMgtAt( "II_TCP_TRACE",  &trace );

    if ( !( trace && *trace ) && PMget("!.tcp_trace_level", &trace) != OK )
        tcp_trace = 0;
    else
        tcp_trace =  atoi(trace);

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
**      ourhost         Allocated buffer at least "buflen" in size.
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
*/

STATUS GCdns_hostname( char *ourhost, i4 hostlen )
{
    struct hostent *hp, *hp1;
    struct addrinfo *result=NULL;
    int err;
    size_t size;
    struct sockaddr *s;
    char hostname[GCC_L_NODE+1]="\0";
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

    /*
    ** gethostname() is the most straightforward means of getting
    ** the host name.
    */
    
    hostname[0] = '\0';
    if (gethostname(hostname, GCC_L_NODE))
        goto deprecated_funcs; 

    /*
    ** Sometimes gethostname() returns the fully qualified host name, so
    ** there is nothing more required.
    */
    if (STindex(hostname, ".", 0))
    {
        len = STlength(hostname);
        if ((len > hostlen-1) || (len > GCC_L_NODE))
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
                if ((len > hostlen-1) || (len > GCC_L_NODE))
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
                if ((len > hostlen-1) || (len > GCC_L_NODE))
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
            if ((len > hostlen-1) || (len > GCC_L_NODE))
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
                        if ((len > hostlen-1) || (len > GCC_L_NODE) )
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
