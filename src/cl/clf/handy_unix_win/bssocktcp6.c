/*
**Copyright (c) 2006, 2007 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<st.h>
# include	<me.h>

# include	<systypes.h>
# include	<clsocket.h>
# include	<errno.h>

# ifdef xCL_TCPSOCKETS_IPV6_EXIST

# include	<bsi.h>
# include	"bssockio.h"
# include	"handylocal.h"

/*
**  Forward and/or External function references.
*/
VOID iisock_unlisten( BS_PARMS *bsp );
static VOID tcp6_switch_to_old_driver(void);
GLOBALREF     BS_DRIVER BS_socktcp;

static	i4	tcpip_version = -1;
static	i4	ip_family = AF_UNSPEC;  
static i4 tcp_trace=0;
#define  TCP_TRACE(n)  if(tcp_trace >= n)(void)TRdisplay

/*
** Flag indicating at least one successful connect or listen to an IPv6
** address has been done.  Used to determine if backoff to old IPv4-only
** driver is feasible.  It is as long as only IPv4 connects and listens
** have been done because the required fields (like the fd) are
** maintained in higher level (BS) structures; also, the new fields in
** the structures for IPv6 were added at the end of the structures.
*/
static	bool	IPv6_listen_or_connect_done = FALSE;


/*
** Name: bssocktcp6.c - BS interface to BSD TCP IPv6 (and IPv4) via sockets.
**
** Description:
**	This file provides access to TCP IPC via BSD sockets, using the 
**	generic BS interface defined in bsi.h.  See that file for a 
**	description of the interface.  Both IPv6 and IPv4 are supported.
**	This file was modeled after bssocktcp.c, which only supports IPv4,
**	but was kept to provide "backup/backoff" capability.
**
**	The main difference between this program and its predecessor
**	is that multiple addresses per listen and per connect must
**	be supported.  Typically, there will be one IPv6 and one IPv4
**	address, but there may indeed be more than 1 of each (if multiple
**	NICs are on the machine).  To make the transition from IPv4 to IPv6
**	easy for the user, we listen on each available IP address.  The
**	tricky part is that if more than one of them completes at a time,
**	we can only call the completion routine for the pending GCC_LISTEN 
**	request with ONE of the incoming connections and save the other
**	for the next GCC_LISTEN request when it comes in.
**
**	Outgoing connections (GCC_CONNECT) are similarly more complicated
**	(but less so) because the target may have both IPv6 and IPv4
**	addresses associated with it (returned by OS function getaddrinfo()). 
**	Rather than quit if the connection to the first address fails, 
**	the code will now continue and try each address in the list.
**	If all fail, then the GCC_CONNECT will be posted with a failure.
**
**	The following functions are internal to this file:
**
**		tcp6_set_trace - sets trace level for TCP
**		tcp6_set_version - sets version for TCP
**		tcp6_dump_addrinfo - dump/display addrinfo entry
**		tcp6_options - turn on TCP_NODELAY and keepalives
**		tcp6_switch_to_old_driver - back off to old IPv4-only driver
**
**	The following functions are exported as part of the BS interface:
**
**		tcp6_listen - establish an endpoint for accepting connections
**		tcp6_accept - accept a single incoming connection
**		tcp6_request - make an outgoing connection
**		tcp6_connect - complete an outgoing connection
**
**	This module also calls and exports the routines found in bssockio.c.
**
** History:
**	6-Oct-2006 (lunbr01)  Sir 116548
**	    Add IPv6 support.
**	    Copied from bssocktcp.c as of 31-aug-2000 version.
**	12-Dec-2006 (lunbr01)  Bug 117251 + Sir 116548
**	    Change struct sockaddr_storage refs to sockaddr_in6 since
**	    sockaddr_storage is not defined on all Unixes (such as
**	    Unixware and HP-UX). 
**	1-Mar-2007 (lunbr01)  Bug 117783 + Sir 116548
**	    Only require one, rather than all, addresses from getaddrinfo()
**	    to be successfully listened on.  This reduces chance of Ingres
**	    startup failures due to IPv6 @ listen failure on systems with 
**	    no or limited IPv6 support (where IPv4 addresses work fine). 
**	    Also, allow for back off to old driver, either manually by
**	    setting II_TCPIP_VERSION=4 or automatically under specific
**	    error conditions that indicate IPv6 is not configured on the
**	    machine. For performance/consistency, don't reissue call to
**	    get new addrinfo list in connect processing if resuming in
**	    the middle of a list.
**	20-Mar-2007 (lunbr01)  Bug 117783 continue/fix
**	    Original fix for bug 117783 caused DBMS server on HP-UX to
**	    register with name server on port 0, which is invalid.
**	    Change type on size parm to getsockname() and getpeername()
**	    calls from size_t to u_i4 (originally was int). 
**	23-Mar-2007 (lunbr01) bug 117995
**	    Free aiList as soon as finished with it rather than waiting
**	    till disconnect because it is an address and hence can't be
**	    passed to another process via GCsave.  Would have caused
**	    gateways to crash at disconnect in spawned GW process.
**	16-Jan-2008 (lunbr01) bug 119770
**	    Slow (~25 sec) connects via Ingres/Net if OS is IPv6-enabled
**	    but doesn't have IPv6 kernel loaded.
**	27-Aug-2008 (lunbr01) bug 120802, 120845
**	    When connect request fails in 2nd phase (completion...ie,
**	    when tcp6_connect is called), and there are more getaddrinfo()
**	    addresses to try, aiCurr was not getting bumped to the next
**	    address entry; this resulted in a loop continually retrying the
**	    first address.
**	4-Sep-2008 (lunbr01) bug 120855
**	    Call iisock_unlisten() at time of error detection rather than
**	    next time through tcp6_listen() to avoid erroneously closing
**	    a previously good open socket whose fd happens to be zero.
**	    This was causing IIGCC network port to get closed by registry
**	    (Ingres discovery) listen port.
**	29-Nov-2010 (frima01) SIR 124685
**	    Added include of handylocal.h
*/


/*
** Support routines - not visible outside this file.
*/


/*
** tcp6_set_trace - sets trace level for TCP.    
*/
static VOID
tcp6_set_trace()
{
	static bool init_trace=FALSE;
	char	*trace;

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
** tcp6_set_version - sets version for TCP.    
**
** Check to see if driver being restricted to IPv4 or IPv6 only.
** Default is both IPv4(AF_INET) + IPv6(AF_INET6)
**
** VERSION -> meaning . . .
**  ALL or unspecified -> (default) Use both IPv4 and IPv6 addresses
**                        with new (IPv6-enabled) driver
**    4    -> Use old IPv4-only driver (ie, back off to old driver)
**   46    -> Use only IPv4 addresses with new (IPv6-enabled) driver
**    6    -> Use only IPv6 addresses with new (IPv6-enabled) driver
**
*/
static int
tcp6_set_version()
{
	char	*p;
	if( tcpip_version == -1 )  /* Only do 1st time in for performance */
	{
	    TCP_TRACE(2)("tcp6_set_version: entered first time\n");
	    tcpip_version = 0;
	    NMgtAt( "II_TCPIP_VERSION", &p );
	    if ( !(p && *p) && PMget("!.tcp_ip.version", &p) != OK )
                ip_family = AF_UNSPEC;
	    else if (STbcompare(p, 0, "ALL", 0, TRUE) == 0)
                ip_family = AF_UNSPEC;
	    else
	    {
	        tcpip_version = atoi(p);     /* 4*=IPv4 or 6=IPv6 */
	        if ((tcpip_version == 4) || (tcpip_version == 46))
	            ip_family = AF_INET;
	        else if (tcpip_version == 6)
	            ip_family = AF_INET6;
	        else
		{
		    TCP_TRACE(2)("tcp6_set_version: failed! II_TCPIP_VERSION=%s\n", p);
	            return(FAIL);
		}
	    }
	    TCP_TRACE(2)("tcp6_set_version: exiting version=%d, family=%d\n",
		tcpip_version, ip_family);
	} /* end if first time */
	return(OK);
}


/*
** tcp6_dump_addrinfo - dump/display addrinfo entry
*/
static VOID
tcp6_dump_addrinfo(ai)
struct addrinfo *ai;
{
    struct sockaddr *s = ai->ai_addr;
    size_t  size = ai->ai_addrlen;
    char    hostaddr[NI_MAXHOST] = {0};
    char    port[NI_MAXSERV] = {0};

    TRdisplay("tcp6_dump_addrinfo(%p) flags=%d family=%d socktype=%d protocol=%d @next=%p\n",
	      ai, ai->ai_flags, 
	      ai->ai_family, ai->ai_socktype, ai->ai_protocol, 
	      ai->ai_next);

    getnameinfo(s, size, hostaddr, sizeof(hostaddr), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);

    TRdisplay("tcp6_dump_addrinfo(%p) -> sockaddr info: len=%d @=%p hostIP@=%s port=%s\n",
	      ai, ai->ai_addrlen, ai->ai_addr, hostaddr, port);
}

/*
** Name: tcp6_options - turn on NODELAY and KEEPALIVE
*/

#ifdef TCP_NODELAY
#define BS_NODELAY TCP_NODELAY
#else
#define BS_NODELAY TO_NODELAY
#endif /* TCP_NODELAY */

static VOID
tcp6_options( fd, which )
int     fd;
int	which;
{
    int val = 1;

    TCP_TRACE(3)("tcp6_options: fd=%d  set KEEPALIVE %s\n", fd, (which != 0) ? "and NODELAY" : "");

    /* turn on NODELAY and KEEPALIVE */

    if ( which != 0 )
	(void)setsockopt( fd, IPPROTO_TCP, BS_NODELAY, &val, sizeof( val ) );

    (void)setsockopt( fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof( val ) );
}


/*
** BS entry points
*/

/*
** Name: tcp6_listen - establish an endpoint for accepting connections
*/

static VOID
tcp6_listen( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	struct sockaddr_in6 s[1];
	u_i4	size = sizeof( *s );
	struct addrinfo *ai = NULL;
	u_i2	port_num, sin_port_assigned=0;
	int	i;
	STATUS	save_bsp_status;

	tcp6_set_trace();

	TCP_TRACE(2)("tcp6_listen: %p entered  port='%s' lbcb=%p\n",
	    bsp->closure, bsp->buf, lbcb);

	if( tcpip_version == -1 )  /* Only do 1st time in for performance */
	{
	    if( tcp6_set_version() != 0)
	    {
	        iisock_error( bsp, BS_LISTEN_ERR );
	        return;
	    }
	    if( tcpip_version == 4 )  /* Back off to old driver? */
	    {
	        tcp6_switch_to_old_driver();
	        (BS_socktcp.listen)( bsp );  /* resume by calling old drvr listen*/
	        return;
	    }
	}

	/* clear fd for sanity */

	lbcb->fd = -1;

	/* get listen port, if specified, and addresses to listen on */

	if( ( bsp->status = BS_tcp_addrinfo( bsp->buf, FALSE, ip_family, &ai ) ) != OK )
	{
		SETCLERR( bsp->syserr, 0, 0 );
		TCP_TRACE(1)("tcp6_listen: %p failure from BS_tcp_addrinfo() status=%8x\n", 
		             bsp->closure, bsp->status);
		/*
		** If getaddrinfo() fails, this system probably doesn't
		** support or hasn't properly been configured for IPv6;
		** one example was AIX 5.1.  Unless IPv6 was specifically
		** requested (II_TCPIP_VERSION=6), fall back to prior driver.
		** Don't do if sessions already established however.
		*/
		if(( !IPv6_listen_or_connect_done ) &&
		   ( bsp->status == BS_NOHOST_ERR ) && 
		   ( tcpip_version != 6 ))
		{
		    TCP_TRACE(1)("tcp6_listen: %p AUTOMATIC driver switch due to getaddrinfo() NOHOST error\n", 
			     bsp->closure);
		    tcp6_switch_to_old_driver();
		    (BS_socktcp.listen)( bsp ); /* resume by calling old drvr listen*/
		}
		return;
	}
	lbcb->aiList = (PTR)ai;   /* save pointer to addrinfo output list */
	/*
	** get numeric port - fortunately, it's in same location and format
	** for both IPv4 & IPv6 in the sockaddr structure.
	*/
	port_num = ntohs( ((struct sockaddr_in *)(ai->ai_addr))->sin_port );

        /* count number of addresses returned. */

	for( i=0; ai; ai = ai->ai_next, i++ )
        {
	    TCP_TRACE(2)("tcp6_listen: ai[%d]:\n", i), tcp6_dump_addrinfo(ai);
        }
        lbcb->num_sockets = i;

	TCP_TRACE(2)("tcp6_listen: %p addrinfo for '%s' - port=%d, version/family=%d/%d, #addrs=%d\n",
	     bsp->closure, bsp->buf, port_num, tcpip_version, ip_family, lbcb->num_sockets);

	/* allocate space for the listen sockets */

	lbcb->fdList = (struct fdinfo *)MEreqmem( 0, sizeof(struct fdinfo) * lbcb->num_sockets, TRUE, NULL );
	if( !lbcb->fdList )
	{
	    (VOID)iisock_unlisten( bsp );  /* Clean up listen info */
	    iisock_error( bsp, BS_LISTEN_ERR );
	    return;
	}
	/*  Initialize fds to -1 since zero is a valid fd value. */
	for( ai = (struct addrinfo *)lbcb->aiList, i=0; ai; ai = ai->ai_next, i++ )
	{
	    lbcb->fdList[i].fd_ai = -1;
	}
	
	/* do socket listens -- one for each address */

        lbcb->num_good_sockets = 0;
	for( ai = (struct addrinfo *)lbcb->aiList, i=0; ai; ai = ai->ai_next, i++ )
	{
	    /* if port was zero, use port assigned to 1st good @ for other @s */
	    if( port_num == 0 )
		((struct sockaddr_in *)(ai->ai_addr))->sin_port = sin_port_assigned;
	    iisock_listen( bsp, ai->ai_addr, ai->ai_addrlen );
	    lbcb->fdList[i].fd_ai = lbcb->fd;
	    lbcb->fdList[i].fd_state = FD_STATE_INITIAL;
	    lbcb->fdList[i].lbcb_self = (PTR)lbcb;
	    if( bsp->status == OK )
	    {
        	lbcb->num_good_sockets++;
		if( ((struct sockaddr *)(ai->ai_addr))->sa_family == AF_INET6 )
		    IPv6_listen_or_connect_done = TRUE;
	    }
	    else
	    {
		TCP_TRACE(1)("tcp6_listen: %p failure from iisock_listen() status=%8x - addr#=%d fd=%d\n", 
		             bsp->closure, bsp->status, i+1, lbcb->fd);
		if( lbcb->fd >= 0 )
		    close( lbcb->fd );
		lbcb->fdList[i].fd_ai = -1;
		/*
		** Special error handling cases
		*/
		/*
		** Check to see if port already in use by another process.
		** On some OS's, the IPv6 socket can handle both IPv6 and
		** IPv4 traffic.  If so, the attempt to bind() on the IPv4
		** address (2nd in list or later) with the same port as 
		** the IPv6 address will fail with EADDRINUSE; this is OK...
		** just use the IPv6 address socket.
		*/
		if( ( bsp->status == BS_BIND_ERR) && 
		    ( bsp->syserr->errnum == EADDRINUSE ) )
		{
		    if( lbcb->num_good_sockets == 0 )  /* Is it us? */
		        break;   /* No, port already in use by other process */
		    else
		    {
		        TCP_TRACE(1)("tcp6_listen: %p EADDRINUSE ignored since not first listen socket\n", 
		                 bsp->closure);
		        continue;
		    }
		}
		/*
		** Check to see if IPv6 address listen failed.
		** On some OS's, if IPv6 is enabled but not loaded in the
		** kernel, outbound connects using a hostname are very
		** slow (~27 seconds). In that case, turn off IPv6 address
		** resolution.
		*/
		if( ( bsp->status == BS_BIND_ERR ) && 
		    ( bsp->syserr->errnum == ENETUNREACH ) &&
		    ( ((struct sockaddr *)(ai->ai_addr))->sa_family == AF_INET6 ))
		{
		    TCP_TRACE(1)("tcp6_listen: %p ENETUNREACH error on IPv6 listen, so restrict to IPv4 addresses\n", 
		                 bsp->closure);
		    ip_family = AF_INET;
		    continue;
		}
		continue;  /* Listen failed, but try next listen address */
	    }  /* end if status == OK */
	    /*
	    ** If input port was zero, the bind() will have given us a new
	    ** port assigned by the system.  Save the port assigned to
	    ** the first address to use for subsequent addresses in list.
	    */
	    if( ( port_num == 0 ) && ( sin_port_assigned == 0 ) )
	    {
		if( getsockname( lbcb->fdList[i].fd_ai, (struct sockaddr *)s, &size) < 0 )
		{
		    (VOID)iisock_unlisten( bsp );  /* Clean up listen info */
		    iisock_error( bsp, BS_LISTEN_ERR );
		    return;
		}
		else
		    sin_port_assigned = ((struct sockaddr_in *)(s))->sin_port;
	    }  /* end if input port number was zero */
	}  /* end for loop listening on each address in list */

	/* Fail if we didn't get at least 1 good socket out of the list. */
	if( lbcb->num_good_sockets == 0 )
	{
	    save_bsp_status = bsp->status; /* Save because unlisten() sets it */
	    (VOID)iisock_unlisten( bsp );  /* Clean up listen info */
	    bsp->status = save_bsp_status; /* Restore original bsp->status */
	    return;
	}
	else  /* Got at least one good socket, so continue. */
	{
	    bsp->status = OK;
	    /* Init lbcb->fd to first good socket */
	    for( i=0; i < lbcb->num_sockets; i++)
	    {
		if( lbcb->fdList[i].fd_ai >= 0 )
		{
		    lbcb->fd = lbcb->fdList[i].fd_ai;
		    break;
		}
	    } /* End foreach socket in lbcb->fdList */
	} /* End else we have some good sockets */

	/* get name for output */

	if( port_num == 0)    /* ...bind() will have given us a new port */
	    port_num = ntohs( sin_port_assigned );
	STprintf( lbcb->port, "%d", port_num );
	bsp->buf = lbcb->port;

	TCP_TRACE(2)("tcp6_listen: %p exiting status=%8x - port=%s\n", 
		     bsp->closure, bsp->status, bsp->buf);
}
	
/*
** Name: tcp6_accept - accept a single incoming connection
*/

static VOID
tcp6_accept( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	struct sockaddr_in6	s[1];
	u_i4	size = sizeof( *s );
	struct sockaddr_in6	peer[1];
	u_i4	peer_size = sizeof( *peer );

	TCP_TRACE(2)("tcp6_accept: %p entered\n", bsp->closure);

	/* clear fd for sanity */

	bcb->fd = -1;
	bsp->is_remote = TRUE;

	/* do socket accept */

	iisock_accept( bsp, s, size );

	if( bsp->status != OK )
	{
		TCP_TRACE(1)("tcp6_accept: %p failure from iisock_accept() status=%8x\n", 
		             bsp->closure, bsp->status);
		return;
	}

	/*
	** Compare the IP addresses of the server and client.  If they
	** match, the connection is local.
	*/
 	getpeername( ((BCB *)bsp->bcb)->fd, (struct sockaddr *)peer, &peer_size );
 	getsockname( ((BCB *)bsp->bcb)->fd, (struct sockaddr *)s, &size );
	if( size == peer_size )
	    if( ((struct sockaddr_in *)(s))->sin_family == AF_INET6 )
	    {
		if( MEcmp( &((struct sockaddr_in6 *)(s))->sin6_addr, 
		           &((struct sockaddr_in6 *)(peer))->sin6_addr, 
		           sizeof(((struct sockaddr_in6 *)(s))->sin6_addr) 
		         ) == 0 )
	            bsp->is_remote = FALSE;
	    }
	    else  /* should be AF_INET (IPv4) */
	    {
		if( MEcmp( &((struct sockaddr_in *)(s))->sin_addr, 
		           &((struct sockaddr_in *)(peer))->sin_addr, 
		           sizeof(((struct sockaddr_in *)(s))->sin_addr) 
		         ) == 0 )
	            bsp->is_remote = FALSE;
	    }
		
	/* Turn on TCP_NODELAY */

	if ( bsp->regop == BS_POLL_INVALID )
	    tcp6_options( bcb->fd, 0 );
	else
	    tcp6_options( bcb->fd, 1 );

	TCP_TRACE(2)("tcp6_accept: %p exiting status=%8x - %s client\n", 
		     bsp->closure, bsp->status, bsp->is_remote ? "REMOTE" : "LOCAL");
}

/*
** Name: tcp6_request - make an outgoing connection
*/

static VOID
tcp6_request( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	struct	sockaddr	*s;
	struct	addrinfo	*ai = NULL;
	i4	i;
	size_t	size;

	tcp6_set_trace();

	TCP_TRACE(2)("tcp6_request: %p entered  target='%s'\n",
		     bsp->closure, bsp->buf);

	if( tcpip_version == -1 )  /* Only do 1st time in for performance */
	{
	    if( tcp6_set_version() != 0)
	    {
	        iisock_error( bsp, BS_CONNECT_ERR );
	        return;
	    }
	    if( tcpip_version == 4 )  /* Back off to old driver? */
	    {
	        tcp6_switch_to_old_driver();
	        (BS_socktcp.request)( bsp );  /* resume by calling old drvr request*/
	        return;
	    }
	}

	/* clear fd for sanity */

	bcb->fd = -1;

	/* get list of addresses for target host/port */

	if( !bcb->aiCurr )  /* If not already working on an ai list */
	{
	    if( ( bsp->status = BS_tcp_addrinfo( bsp->buf, TRUE, ip_family, &ai ) ) != OK )
    	    {
		SETCLERR( bsp->syserr, 0, 0 );
		TCP_TRACE(1)("tcp6_request: %p failure from BS_tcp_addrinfo() status=%8x, family=%d\n", 
		             bsp->closure, bsp->status, ip_family);
		return;
	    }
	    bcb->aiList = (PTR)ai;
	}

	/* do socket connect for each address in list til OK or end of list */

	TCP_TRACE(2)("tcp6_request: %p bcb->aiCurr=%p, bcb->aiList=%p\n",
		bsp->closure, bcb->aiCurr, bcb->aiList);
	for( ai = (struct addrinfo *)(bcb->aiCurr ? bcb->aiCurr : bcb->aiList), i=0;
	     ai;
	     ai = ai->ai_next, i++ )
	{
	    TCP_TRACE(2)("tcp6_request: %p  ai[%d]=%p\n",
		    bsp->closure, i, ai), tcp6_dump_addrinfo(ai);
	    s    = ai->ai_addr;
	    size = ai->ai_addrlen;
	    iisock_request( bsp, s, size );
	    if( bsp->status == OK )
	    {
		if( ((struct sockaddr *)(ai->ai_addr))->sa_family == AF_INET6 )
		    IPv6_listen_or_connect_done = TRUE;
		break;
	    }
	}
	bcb->aiCurr = (PTR)ai;

	TCP_TRACE(2)("tcp6_request: %p exiting status=%8x  addr#=%d%s\n", 
		     bsp->closure, bsp->status, i+1, ai ? "" : "(END)");

	/*
	**  If all the connection address(es) were tried and failed,
	**  and a socket couldn't even be created, then system is likely
	**  not configured for IPv6, so back off to old driver...
	**  unless they specifically requested IPv6 (II_TCPIP_VERSION=6).
	**  One example was HP-UX B.11.11.
	**  NOTE that "back off" is not done for normal connect error.
	**  Also, don't do if sessions already established.
	*/
	if(( !IPv6_listen_or_connect_done ) &&
	   ( bsp->status == BS_SOCK_ERR ) && 
	   ( tcpip_version != 6 ))
	{
	    TCP_TRACE(1)("tcp6_request: %p AUTOMATIC driver switch due to socket error\n", 
		     bsp->closure);
	    tcp6_switch_to_old_driver();
	    (BS_socktcp.request)( bsp ); /* resume by calling old drvr request*/
	}
}

/*
** Name: tcp6_connect - complete an outgoing connection
*/

static VOID
tcp6_connect( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	struct addrinfo *ai = (struct addrinfo *)bcb->aiCurr;
	struct 	sockaddr_in6 s[1];
	int 	size = sizeof( *s );

	TCP_TRACE(2)("tcp6_connect: %p entered\n", bsp->closure);

	/* do socket connect */

	iisock_connect( bsp, s, size );

	if( bsp->status != OK )
	{
	    TCP_TRACE(2)("tcp6_connect: %p failure from iisock_connect() status=%8x\n", 
		bsp->closure, bsp->status);
	    if( ai->ai_next )  /* Try next address (if any) */
	    {
		bcb->aiCurr = (PTR)ai->ai_next;
		bsp->status = BS_INCOMPLETE;
		TCP_TRACE(2)("tcp6_connect: %p Try next address entry (ai=%p) in getaddrinfo() list\n", 
		    bsp->closure, bcb->aiCurr);
	    }
	    return;
	}

        /*
        ** Free aiList now instead of at disconnect as it is no longer
        ** needed.  Also, waiting till disconnect/close causes a crash
        ** if the connection is passed to another process (via GCsave/
        ** GCrestore)-- eg, gateways.  The aiList is no longer needed
        ** at this point because we have either successfully connected
        ** and have a socket, or have failed on all the IP addresses in
        ** the list.
        */
        if( bcb->aiList )
        {
            freeaddrinfo( (struct addrinfo *)bcb->aiList );
            bcb->aiList = bcb->aiCurr = NULL;
        }

	/* Turn on TCP_NODELAY */

	tcp6_options( bcb->fd, 1 );

	TCP_TRACE(2)("tcp6_connect: %p exiting status=%8x\n", 
		     bsp->closure, bsp->status);
}


/*
** Exported driver specification.
*/

extern  VOID	iisock_unlisten(BS_PARMS *bsp);
extern  VOID	iisock_send();
extern  VOID	iisock_receive();
extern  VOID	iisock_close();
extern  bool	iisock_regfd();
extern	VOID	iisock_ok();
extern  STATUS  iisock_ext_info();

BS_DRIVER BS_socktcp6 = {
	sizeof( BCB ),
	sizeof( LBCB ),
	tcp6_listen,
	iisock_unlisten,
	tcp6_accept,
	tcp6_request,
	tcp6_connect,
	iisock_send,
	iisock_receive,
	0,		/* no orderly release */
	iisock_close,
	iisock_regfd,
	iisock_ok,	/* save not needed */
	iisock_ok,	/* restore not needed */
	BS_tcp_port,	
	iisock_ext_info   /* extended information */
} ;


/*
** tcp6_switch_to_old_driver - back off to old IPv4-only driver
**
**  To back off to the old driver, this routine overlays this driver's
**  function pointers with those of the prior driver.  The caller has
**  a pointer to the array of pointers so will be unaware that the
**  switch has been made.
*/
static VOID
tcp6_switch_to_old_driver()
{
	TCP_TRACE(1)("tcp6_switch_to_old_driver: BACKING OFF TO OLD DRIVER!\n");
	MEcopy( &BS_socktcp, sizeof(BS_DRIVER), &BS_socktcp6 );
	return;
}
# endif /* xCL_TCPSOCKETS_IPV6_EXIST */
