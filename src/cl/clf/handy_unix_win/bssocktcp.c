/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include       <gc.h>
# include       <gcccl.h>
# include	<clconfig.h>
# include	<st.h>
# include	<me.h>
# include	<cv.h>

# include       <diracc.h>
# include       <handy.h> 
# include	<systypes.h>
# include	<clsocket.h>
# include	<errno.h>

# ifdef xCL_019_TCPSOCKETS_EXIST

# include	<bsi.h>
# include	"bssockio.h"


/*
** Name: bssocktcp.c - BS interface to BSD TCP via sockets.
**
** Description:
**	This file provides access to TCP IPC via BSD sockets, using the 
**	generic BS interface defined in bsi.h.  See that file for a 
**	description of the interface.
**
**	The following functions are internal to this file:
**
**		tcp_options - turn on TCP_NODELAY and keepalives
**
**	The following functions are exported as part of the BS interface:
**
**		tcp_listen - establish an endpoint for accepting connections
**		tcp_accept - accept a single incoming connection
**		tcp_request - make an outgoing connection
**		ii_tcp_connect - complete an outgoing connection
**
**	This module also calls and exports the routines found in bssockio.c.
**
** History:
**	24-May-90 (seiwald)
**	    Written.
**	26-May-90 (seiwald)
**	    Renamed to BS_socktcp for consistency.
**	    Removed sock_addr(); we use BS_tcp_addr() now.
**	30-Jun-90 (seiwald)
**	    Private and Lprivate renamed to bcb and lbcb, as private
**	    is a reserved word somewhere.
**	5-Jul-90 (seiwald)
**	    Use CLPOLL_RESVFD to save fds for SYSV stdio.
**	7-Aug-90 (seiwald)
**	    Pass BS_tcp_addr() a flag indicating whether the connection
**	    is passive (false) or active (true).
**	14-Aug-90 (seiwald)
**	    Allow for EINTR from read and write calls.
**	21-Aug-90 (anton)
**	    Semaphore the CLPOLL_RES[VS]FD calls
**	    reorder & polish connect error recovery to preserve errno.
**	23-Aug-90 (seiwald)
**	    Ifdef'ed MCT_TAS a few things.
**	    New sock_error to format system errors.
**	31-Aug-90 (seiwald)
**	    Bump moreinfo.size CL_ERR_DESC->moreinfo[0].size by 1
**	    in sock_error: it seems to appease ERlookup.
**	6-Sep-90 (seiwald)
**	    Moved generic routines to bssockio.c.
**	26-Sep-91 (seiwald) bug #39654
**	    Turn on keepalives.
**	5-Mar-92 (gautam) 
**	    NCR has no TCP_NODELAY defined. Make the setsockopt()
**	    call only if TCP_NODELAY is defined.
** 	18-sep-92 (pghosh)
**	    tcp_connect() is renamed to ii_tcp_connect. NCR has their
** 	    own fn called tcp_connect, and for nc5_us5 platform, it was 
**  	    giving compiler problem.
**	30-dec-92 (mikem)
**	    su4_us5 port.  Added cast in tcp_options().
**	02-Mar-93 (brucek) 
**	    Added NULL function ptr to BS_DRIVER for orderly release;
**	    use TO_NODELAY on setsockopt() if TCP_NODELAY not defined.
**	04-Mar-93 (edg)
**	    Remove detect logic.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	1-jun-94 (robf)
**  	    added ext_info request for BS driver
**	19-apr-95 (canor01)
**	    added <errno.h>
**      10-nov-1995 (sweeney)
**          Rename *sock_ routines -- sock_connect was clashing with a
**          3rd party comms library.
**	03-jun-1996 (canor01)
**	    Do not use NODELAY with operating-system threads, block instead.
**	18-apr-1998 (canor01)
**	    Pass expected parameter to tcp_options.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      03-Aug-2007 (Ralph Loen) SIR 118898
**          Add GCdns_hostname().
**      09-Aug-2007 (Ralph Loen) SIR 118898
**          Add gc.h to include error numbers for GCdns_hostname().
**      04-Sep-2007 (Ralph Loen) SIR 118898
**          In GCdns_hostname(), added arguments required for 
**          iiCLgethostbyaddr().
**      04-Sep-2007 (Ralph Loen) SIR 118898
**          In GCdns_hostname(), removed reference to non-existent variable. 
**      17-Oct-2007 (rapma01) SIR 118898
**          missing headers handy.h and diracc.h included 
**          which contain the prototype of iiCLgethostbyname()
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/


/*
** Support routines - not visible outside this file.
*/

/*
** Name: tcp_options - turn on NODELAY and KEEPALIVE
**
** History:
**	30-dec-92 (mikem)
**	    su4_us5 port.  Added cast of &val argument to get rid of acc compile
**	    warning: argument is incompatible with prototype: #4.
*/

#ifdef TCP_NODELAY
#define BS_NODELAY TCP_NODELAY
#else
#define BS_NODELAY TO_NODELAY
#endif /* TCP_NODELAY */

static VOID
tcp_options( fd, which )
int     fd;
int	which;
{
    int val = 1;

    /* turn on NODELAY and KEEPALIVE */

    if ( which != 0 )
	(void)setsockopt( fd, IPPROTO_TCP, BS_NODELAY, &val, sizeof( val ) );

    (void)setsockopt( fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof( val ) );
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
	struct	sockaddr_in	s[1];
	int	size = sizeof( *s );

	/* clear fd for sanity */

	lbcb->fd = -1;

	/* get listen port, if specified */

	if( ( bsp->status = BS_tcp_addr( bsp->buf, FALSE, s ) ) != OK )
	{
		SETCLERR( bsp->syserr, 0, 0 );
		return;
	}

	/* do socket listen */

	iisock_listen( bsp, s, size );

	if( bsp->status != OK )
		return;

	/* get name for output */

	if( getsockname( lbcb->fd, (struct sockaddr *)s, &size ) < 0 )
	{
		iisock_error( bsp, BS_LISTEN_ERR );
		return;
	}

	STprintf( lbcb->port, "%d", ntohs( s->sin_port ) );
	bsp->buf = lbcb->port;
}
	
/*
** Name: tcp_accept - accept a single incoming connection
*/

static VOID
tcp_accept( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	struct sockaddr_in	s[1];
	int	size = sizeof( *s );
	u_i4 socket_inaddr,peer_inaddr;

	/* clear fd for sanity */

	bcb->fd = -1;
	bsp->is_remote = TRUE;

	/* do socket accept */

	iisock_accept( bsp, s, size );

	if( bsp->status != OK )
		return;

	/*
	** Compare the IP addresses of the server and client.  If they
	** match, the connection is local.
	*/
 	getpeername( ((BCB *)bsp->bcb)->fd, (struct sockaddr *)s, (i4 *)&size );
	MEcopy(&s->sin_addr, sizeof(i4), &peer_inaddr);
 	getsockname( ((BCB *)bsp->bcb)->fd, (struct sockaddr *)s, (i4 *)&size );
	MEcopy(&s->sin_addr, sizeof(i4), &socket_inaddr);
        if (socket_inaddr == peer_inaddr)
	    bsp->is_remote = FALSE;

	/* Turn on TCP_NODELAY */

	if ( bsp->regop == BS_POLL_INVALID )
	    tcp_options( bcb->fd, 0 );
	else
	    tcp_options( bcb->fd, 1 );
}

/*
** Name: tcp_request - make an outgoing connection
*/

static VOID
tcp_request( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	struct	sockaddr_in	s[1];
	i4	size = sizeof( *s );

	/* clear fd for sanity */

	bcb->fd = -1;

	/* translate address */

	if( ( bsp->status = BS_tcp_addr( bsp->buf, TRUE, s ) ) != OK )
    	{
		SETCLERR( bsp->syserr, 0, 0 );
		return;
	}

	if( !s->sin_port )
	{
		bsp->status = BS_NOPORT_ERR;
		SETCLERR( bsp->syserr, 0, 0 );
		return;
	}

	/* do socket connect */

	iisock_request( bsp, s, size );
}

/*
** Name: ii_tcp_connect - complete an outgoing connection
*/

static VOID
ii_tcp_connect( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	struct 	sockaddr_in 	s[1];
	int 	size = sizeof( *s );

	/* do socket connect */

	iisock_connect( bsp, s, size );

	if( bsp->status != OK )
		return;

	/* Turn on TCP_NODELAY */

	tcp_options( bcb->fd, 1 );
}


/*
** Exported driver specification.
*/

extern	STATUS	BS_tcp_port();
extern	VOID	iisock_unlisten();
extern  VOID	iisock_send();
extern  VOID	iisock_receive();
extern  VOID	iisock_close();
extern  bool	iisock_regfd();
extern	VOID	iisock_ok();
extern  STATUS  iisock_ext_info();

BS_DRIVER BS_socktcp = {
	sizeof( BCB ),
	sizeof( LBCB ),
	tcp_listen,
	iisock_unlisten,
	tcp_accept,
	tcp_request,
	ii_tcp_connect,
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
**      hostlen         Byte length of returned host name buffer
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
**    04-Sep-2007 (Ralph Loen) SIR 118898
**      Added arguments required for iiCLgethostbyaddr().
**    04-Sep-2007 (Ralph Loen) SIR 118898
**      Removed reference to non-existent variable. 
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

STATUS GCdns_hostname( char *ourhost, i4 hostlen )
{
    struct hostent *hp, *hp1;
    struct hostent lhostent;
    struct hostent *iiCLgethostbyaddr();
    union
    {
# if defined(any_aix)
        struct hostent_data h;   /* for alignment purposes only */
        char  *buffer[sizeof(struct hostent_data)+512];
# else
        struct hostent h;   /* for alignment purposes only */
	char  *buffer[sizeof(struct hostent)+512];
# endif
    } hostbuf;
# ifdef xCL_TCPSOCKETS_IPV6_EXIST 
    struct addrinfo *result=NULL;
# endif /* xCL_TCPSOCKETS_IPV6_EXIST */
    int err;
    size_t size;
    struct sockaddr *s;
    char hostname[GCC_L_NODE+1];
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

# if defined(any_aix)
    MEfill( (u_i2)sizeof(hostbuf), 0, &hostbuf );
# endif

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

# ifdef xCL_TCPSOCKETS_IPV6_EXIST 
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
    }
# else
    goto deprecated_funcs;
# endif /* xCL_TCPSOCKETS_IPV6_EXIST */

deprecated_funcs:
    /*
    ** If the preferred approach, using getnameinfo() and getaddrinfo(),
    ** doesn't produce a fully qualified host name, fall back on
    ** the deprecated gethostname() and gethostbyname() functions.
    */        
    hp = iiCLgethostbyname( hostname, &lhostent, (char *) &hostbuf,
                                sizeof(hostbuf), &h_errno );

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
                int local_errno;

                addr = inet_addr(ipaddr);

# if defined(sparc_sol) || defined(any_aix)
                hp1 = iiCLgethostbyaddr((const char *)&addr,
                                         sizeof(struct in_addr),
                                         AF_INET,
                                         &lhostent,
                                         &hostbuf,
                                         sizeof(hostbuf),
                                         &local_errno );
# else
                hp1 = gethostbyaddr((const char *)&addr, sizeof(struct in_addr),
                    AF_INET);
# endif

                if (hp1)
                {
                    if (STindex(hp1->h_name, ".", 0))
                    {
                        len = STlength(hp1->h_name);
                        if ((len > hostlen-1) || (len > GCC_L_NODE))
                        {
                            STlcopy(hp1->h_name, ourhost, hostlen);
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
# ifdef xCL_TCPSOCKETS_IPV6_EXIST 
    if (result)
        freeaddrinfo(result);
# endif /* xCL_TCPSOCKETS_IPV6_EXIST */
    if (!STlength(ourhost))
        status = GC_NO_HOST;
    else
        CVlower(ourhost);
    return status;
}
# endif /* xCL_019_TCPSOCKETS_EXIST */
