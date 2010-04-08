/*
** Copyright (c) 1988, 2007 Ingres Corporation All Rights Reserved.
*/



# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<clpoll.h>
# include	<st.h>
# include	<me.h>
# include	<pc.h>
# include	<cs.h>
# include	<cv.h>
# include	<er.h>
# include	<lo.h>
# include	<pm.h>

#ifndef MAXHOSTNAME
#define MAXHOSTNAME     64
#endif
 
/*
NO_OPTIM = i64_aix
*/

# ifdef xCL_SUNOS_CMW
# define SunOS_CMW
# undef ulong
# include <cmw/tnet_attrs.h>
# include <cmw/sctnattrs.h>
# include <sys/label.h>
# include <pwd.h>
# endif

# if defined(hp8_bls)
# include <sys/security.h>
# include <mandatory.h>
# include <m6attrs.h>
# include <sys/sctnmasks.h>
# include <sys/sctnerrno.h>
# include <pwd.h>
# endif /* hp8_bls */

# ifdef xCL_066_UNIX_DOMAIN_SOCKETS
# define x_SOCKETS_EXIST
# endif /* xCL_066_UNIX_DOMAIN_SOCKETS */

# ifdef xCL_019_TCPSOCKETS_EXIST
# define x_SOCKETS_EXIST
# endif /* xCL_019_TCPSOCKETS_EXIST */

# ifdef xCL_TCPSOCKETS_IPV6_EXIST
# define x_SOCKETS_EXIST
# endif /* xCL_TCPSOCKETS_IPV6_EXIST */

# ifdef x_SOCKETS_EXIST

# include	<systypes.h>
# include	<clsocket.h>

# if	defined(any_aix)
# define        SOCK_NDELAY     O_NONBLOCK 
# endif

# ifdef xCL_006_FCNTL_H_EXISTS
# include	<fcntl.h>
# ifndef	SOCK_NDELAY
# define        SOCK_NDELAY     O_NDELAY
# endif /* SOCK_NDELAY */
# endif /* xCL_006_FCNTL_H_EXISTS */

# ifdef xCL_007_FILE_H_EXISTS
# include	<sys/file.h>
# ifndef	SOCK_NDELAY
# define        SOCK_NDELAY     FNDELAY
# endif /* SOCK_NDELAY */
# endif /* xCL_007_FILE_H_EXISTS */

# include	<errno.h>

# include	<bsi.h>
# include	"bssockio.h"

#if defined(m88_us5) || defined(pym_us5) || defined(rmx_us5) || defined(rux_us5)
# include       <signal.h>
# endif

static VOID iisock_set_trace();

static i4 iisock_trace=0;
i2 ewbcnt=0;
static i4  sock_trace=0;

/*
** Name: bssockio.c - support routines for BSD socket interface
**
** Description:
**	This file contains routines to support the interfaces to
**	BSD sockets offered by bssocktcp.c (for access to TCP/IP)
**	and bssockunix.c (for access to UNIX domain sockets).
**
**	The following functions are internal support routines, and
**	are called by the interface routines:
**
**		iisock_error - format syserr
**		iisock_nonblock - make a fd non-blocking
**		iisock_listen - establish an endpoint for accepting connections
**		iisock_listen_compl - intercept multiple listen completions
**		iisock_unlisten - stop accepting incoming connections
**		iisock_accept - accept a single incoming connection
**		iisock_request - make an outgoing connection
**		iisock_connect - complete an outgoing connection
**
**	The following functions conform to the BS interface, and
**	may be exported as interface routines or called by interface
**	routines:
**
**		iisock_send - send data down the stream
**		iisock_receive - read data from a stream
**		iisock_close - close a single connection
**		iisock_regfd - register for blocking operation
**		iisock_ok - dummy function returning OK
**		iisock_ext_info - socket extended info
**
**	Note that send, receive, and regfd are high traffic routines
**	and should not have further layers of indirection.
**
** History:
**	6-Sep-90 (seiwald)
**	    Extracted from bssocktcp.c
**     25-Oct-90 (hhsiung)
**	    Put a missing right comment mark at end of line 29.
**          Defined SOCK_NDELAY as O_NDELAY or FNDELAY depending on
**          the system.  Modfied fcntl call in sock_nonblock to use
**          SOCK_NDELAY. The duplicated call can be avoided if the
**          box has both xCL_006_FCNTL_H_EXISTS and xCL_007_FILE_H_EXISTS
**          defined( like Bull DPX/2).
**	5-Dec-90 (seiwald)
**	    Don't count the null in the size of the error string
**	    and don't bother appending a newline to the string.
**	14-Jan-91 (seiwald)
**	    Removed duplicate definition of SOCK_NDELAY introduced by
**	    hhsiung's change.
**	14-Mar-91 (gautam)
**	    SOCK_NDELAY is a O_NONBLOCK on AIX 3.1 (ris_us5)
**	7-Aug-91 (seiwald)
**	    Use new CLPOLL_FD(), replacing CLPOLL_RESVFD().
**	26-Sep-91 (seiwald) bug #38578
**	    Turn on SO_REUSEADDR, so that we can issue the network listen
**	    even if there are various zombie connections (in FIN_WAIT_1,
**	    etc.) still using the port.
**	21-Nov-91 (seiwald) bug #40990
**	    Added XXX comments near read() and write() to warn of potential
**	    coding errors involving O_NDELAY and skipped selects.
**	13-Feb-92 (gautam) 
**	    For sqs_ptx, bind on an existing address returns
**	    EADDRNOTAVAIL. bind on UNIX domain socket returns EEXIST.
**	27-Feb-92 (gautam) 
**	    Don't issue SO_REUSEADDR on UNIX domain sockets - that is
**	    a protocol error
**	30-apr-92 (jillb--DGC) (kevinm)
**          dg8_us5 returns EWOULDBLOCK instead of EINPROGRESS on a connect
**          to a non-blocking socket. Changed the error return check to
**          EINPROGRESS and EWOULDBLOCK .
**      04-jan-92 (deastman)
**          Block signals around accept call.  If a signal interrupts
**          accept, accept fails w/ EINTR and all subsequent attempts
**          to handle that f/e connent request will fail w/ EINVAL
**          leaving the f/e hung.  This is a pym_us5 specific workaround.
**	04-Jan-93 (edg)
**	    Added support for user-adjustable listen backlog Q's via
**	    new PM variable ii.host.gca.socket.listen_qlen.  Previously
**	    this was just stuffed with a value of 128.  New default is 10.
**	04-Mar-93 (edg)
**	    Removed detect logic.
**	17-May-93 (daveb)
**	    Unregister fds in unlisten, 'cause if clpoll is caused
**	    after unlisten, we can get EBADF and much ensuing badness.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	5-aug-93 (robf)
**	   Integrated change by arc to operate on CMW:
**      10-may-93 (arc)
**          Support for Sun CMW. Enable security attributes on socket
**          operations and pass full CMW label of client process back.
**	12-nov-93 (robf)
**         Get hostname of partner and pass back to caller in sock_accept()
**	   Partially backout out 5-aug-93 change since the label is  
**	   determined  elsewhere.
**	6-jan-94 (robf)
**	   Integrated CMW ES 1.0 change from arc:
**         14-dec-93 (arc)
**              Set security label of data associated with socket to that of 
**		client enabling the client to run unprivileged.
**	21-jan-94 (arc)
**	   Added missing declaration of defaults_mask for su4_cmw.
**      11-feb-94 (ajc)
**          Added hp8_bls specific security handling
**      17-Mar-94 (seiwald)
**          Once an I/O error occurs on a socket, avoid trying further
**          I/O's: they tend to hang on SVR4 machines.
**      23-mar-94 (ajc)
**          Fixed reading of security labels and call to gethostbyaddr() for
**          hp8-bls.
**      28-mar-94 (ajc)
**          Hp-bls - due to OS upgrade, m6attrs.h has moved up from sys.
**	6-apr-94 (robf/ajc)
**          Enable extended attributes when doing a sock_request (needed for
**	    HP) and modularize code into sock_ext_attr()
**	6-apr-94 (arc)
**	    Correct bad #if statement and ensure all #if defined(hp8_bls)
**	    adhere to coding standards (with space char after #).
**      7-apr-94 (ajc)
**          Hp-bls - m6peek will hang because of non block waiting for data
**          to be written. iigcc in loopback mode would hang. Therefore use 
**          m6last_attr instead.
**	1-jun-94 (robf)
**          Added sock_ext_info. This provides the BS extended info service
**	    for UNIX & TCP/IP sockets, currently only used on secure
**	    systems.
**	    Also added tracing in this file to preserve the output previously
**	    sent out via GCTRACE - added new macro SOCK_TRACE() which does
**	    much the same thing but is local to this file.
**	1-jun-94 (arc)
**	    sock_ext_info implemented for Trusted Solaris (Sun CMW).
**	27-jun-94 (ajc)
**	    Default remote hostname to unknown and not use sun specific code.
**      09-feb-95 (chech02)
**          Added rs4_us5 for AIX 4.1.
**      10-nov-1995 (sweeney)
**          Rename *sock_ routines -- sock_connect was clashing with a
**          3rd party comms library.
**      06-dec-1995 (morayf)
**	    Added rmx_us5 for SNI RM400/600 port (ala Pyramid).
**	03-jun-1996 (canor01)
**	    Must pass a pointer to a struct passwd to iiCLgetpwuid().
**	    Introduce IICLgethostbyaddr()--MT-safe version of gethostbyaddr().
**	23-jul-1996 (canor01)
**	    Use local copy of errno in case it's a wrapper for a function.
**	    Error of ECONNREFUSED on connect() means that the server hasn't
**	    recycled fast enough, so sleep and retry.
**	25-mar-1997 (kosma01)
**	    Add rs4_us5 to the list of iiCLgethostbyaddr users.
**	    Add hostent_data buffer to the union hostbuf for rs4_us5.
**	30-jul-1997 (canor01)
**	    Some socket interfaces have trouble with bind() when the
**	    socket file descriptor is greater than 255, so do not try
**	    to relocate the listen socket fd.
**	24-sep-1997 (hanch04)
**	    ipeer->sin_len is for rs4_us5 only, not su4_us5
**	24-Sep-1997 (rajus01)
**	    Added isremote_conn() to find out whether the connection is 
**	    local or remote.
**	10-oct-1997/09-apr-1997 (popri01)
**	    Cross-integrated John Rodger's 1.2 fix (#432793) which we hope 
**	    provides a final solution to the Unixware communication problems.
**	    Changed h_errno (from 23-jul-1996) to local_errno due to
**	    conflict with Novell's netdb.h.
**	    Corrected typo in iisock error call in unlisten (lbcb should be bsp)
**	30-apr-1998 (bobmart)
**	    Initialized third arg to gethostbyname_r, as stipulated; without,
**	    iigcc will crash on connect if system is not config'ed with DNS.
**	03-sep-98 (toumi01)
**	    Conditional const declaration of sys_errlist for Linux
**	    systems using libc6 (else we get a compile error).
**	14-Sep-1998 (rajus01)
**	    Resolved the problem that when II_GC_PROT is set to UNIX,
**	    the recovery server won't start.
**	08-nov-1998 (popri01)
**	    Changed another h_errno (from 14-sep-1998) to local_errno (see
**	    10-oct-1997) due to Unixware 7 collision.
**	15-apr-1999 (popri01)
**	    For Unixware 7 (usl_us5), the C run-time variables
**	    sys_errlist and sys_nerr are not available in a dynamic
**	    load environment. Use the strerror function instead.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	05-may-1999 (matbe01)
**	    Added NO_OPTIM for rs4_us5 to prevent name server from
**	    crashing when recovery server comes up.
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**      15-Oct-1999 (hweho01)
**	    Added support for AIX 64-bit (ris_u64) platform.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	14-jun-2000 (hanje04)
**	    Conditional const declaration of sys_errlist for OS/390 Linux
**	    (ibm_lnx) systems using libc6 (else we get a compile error).
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-nov-98 (stephenb)
**	    Use strerror to compare error strings, direct comparison doesn't
**	    work under Solaris 7.
**	07-Sep-2000 (hanje04)
**	    Added axp_lnx (Alpha Linux) to Linux/libc6 changes
**      15-Sep-2000 (hweho01)
**	    Changed the datatype of buffer in hostbuf structure from   
**          pointer to char type for AIX platforms (rs4_us5 and ris_u64). 
**	14-Mar-2001 (wnash01)
**	    For dgi_us5, the C run-time variables
**	    sys_errlist and sys_nerr are not available in a dynamic
**	    load environment. Use the strerror function instead.
**      03-May-2001 (loera01) bug 104617
**          For isremote_conn(), replaced calls to iiCLgethostbyname() and 
**          iiCLgethostbyaddr() with a simple check of the client's
**          socket address.  It is only necessary to determine whether or not 
**          the client's socket originated locally.
**	23-jul-2001 (stephenb)
**	    Add support fir i64_aix
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	17-Oct-2001 (hanje04)
**	    Removed all references sys_errlist[] as it has been replaced by
**	    strerror().
**      04-DEC-2001 (legru01)
**         Added (su9_us5) to platforms defining have_add
**     19-nov-2002 (loera01)
**         Removed isremote_conn().  Instead, the is_remote flag in 
**         BS_PARMS is set in the callers of iisock_accept. 
**	 6-Jun-03 (gordy)
**	    Added local and remote network addresses to extended info.
**	22-sep-2003 (wanfr01)
**	    Bug 110128, INGNET 121
**	    Sleep for a second in iisock_send in DGUX if you get 500
**	    consecutive EWOULDBLOCK return calls.
**	 6-Aug-04 (gordy)
**	    Initialize listen file descriptor in case timeout occurs prior
**	    to actually calling accept().
**      23-Nov-2004 (hweho01)
**          Remove rs4_us5 from NO_OPTIM list.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mg5_osx)).
**          Based on initial changes by utlia01 and monda07.
**      7-Oct-2006 (lunbr01)  Sir 116548
**          In support of IPv6, add new routine iisock_listen_compl()
**          to intercept and handle listen completions (incoming connection
**          requests) in multiple listen address scenario only.
**          Also, free memory for addrinfo and socket list, if exist,
**          for disconnects and listen failures.
**      31-Oct-2006 (lunbr01)  Sir 116548, bug 117023
**          Incoming connects were crashing GCC in iisock_accept() after
**          IPv6-related getnameinfo() call due to unitialized hostent
**          variable.
**      5-Dec-2006 (lunbr01)  Bug 117251 + Sir 116548
**          Fix compile errors on non-IPv6 platforms by #ifdef'ing around
**          new structures and functions only available on IPv6 OS's.
**      06-Feb-2007 (Ralph Loen) SIR 117590
**          In iisock_accept(), remove invocation of gethostbyaddr() 
**          getnameinfo() for IPv6) for network listens.
**	1-Mar-2007 (lunbr01)  Bug 117783 + Sir 116548
**	    Good sockets may no longer be at start of address list; check
**	    entire list and process each "good" socket. This reduces chance
**	    of Ingres startup failures due to IPv6 @ listen failure on
**	    systems with no or limited IPv6 support (where IPv4 @ works fine).
**	23-Mar-2007 (lunbr01) bug 117995
**	    Add missing 2nd parm (status) in and out to callback function.
**	    Not needed for remote conns (gccbs) but is needed for local.
**	    Caused gateways to fail incoming connections because it was
**	    thought that the listen had timed out (E_GC0020_TIMEOUT).
**	    Application error typically was:
**	      E_GCfe05 -- Unable to make outgoing connection.
**      25-Jul-2007 (rapma01) 
**          Removed routine MEfill for union hostbuf which was solely called
**          for AIX platforms since hostbuf was removed in change 485545.
**	27-Aug-2008 (lunbr01) bug 120802, 120845
**	    Close socket immediately after connect failure detected
**	    in iisock_connect() because (for IPv6) the caller may try
**	    multiple IP addresses.  Intermediate failing IP @s were not
**	    getting closed (only the final one was getting closed at
**	    disconnect time (iisock_close() call).
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/


#define LISTEN_Q	20


/*
** Support routines - not part of BS interface
*/

i4  iisock_ext_attr( BS_PARMS *bsp, i4  fd);
/*
** Name: iisock_error - format syserr
*/

VOID
iisock_error( bsp, status )
BS_PARMS	*bsp;
STATUS		status;
{
	int local_errno = errno;
	char *s = bsp->syserr->moreinfo[0].data.string;

	bsp->status = status;

	STcopy( strerror(local_errno), s );
	SETCLERR( bsp->syserr, BS_SYSERR, 0  );

	bsp->syserr->moreinfo[0].size = STlength( s );

	iisock_set_trace();
        SOCK_TRACE(1)("iisock_error: %p status=0x%8x errno=%d '%s'\n",
	    bsp->closure, status, local_errno, s);
}

/*
** Name: iisock_nonblock - make a fd non-blocking
*/

VOID
iisock_nonblock( fd )
int     fd;
{
	/* set for non-blocking */

	(void)fcntl( fd, F_SETFL, SOCK_NDELAY );
}

/*
** Name: iisock_listen - establish an endpoint for accepting connections
**
** History:
**      5-Dec-2006 (lunbr01)  Bug 117251 + Sir 116548
**          Fix compile errors on non-IPv6 platforms by #ifdef'ing around
**          new structures and functions only available on IPv6 OS's;
**          specific problems are struct sockaddr_storage and function
**          getnameinfo().  Also skip socket-related function calls used
**          only for traces if tracing not on (to improve performance).
*/

VOID
iisock_listen( bsp, s, size )
BS_PARMS	*bsp;
struct sockaddr	*s;
i4		size;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	int	fd;
	char 	*p;
	int	on = 1;
	int	qlen;
	char	hostaddr[NI_MAXHOST] = {0};
	char	port[NI_MAXSERV] = {0};
	u_i2	port_num = 0;
# ifdef xCL_TCPSOCKETS_IPV6_EXIST
	struct sockaddr_in6	s_work;
# else
	struct sockaddr_in      s_work;
# endif /* xCL_TCPSOCKETS_IPV6_EXIST */
	int	s_work_len = sizeof(s_work);

	iisock_set_trace();

	/* create a socket */

	fd = socket( s->sa_family, SOCK_STREAM, 0 );
	lbcb->fd = fd;
# ifndef xCL_RESERVE_STREAM_FDS
	CLPOLL_FD( fd );
# endif /* xCL_RESERVE_STREAM_FDS */

	if( fd < 0 )
	{
		iisock_error( bsp, BS_SOCK_ERR );
		return;
	}

        /*
        ** Enable the extended network interface on this socket.
        */
	if(iisock_ext_attr(bsp, fd)!=OK)
	{
		iisock_error(bsp, BS_LISTEN_ERR);
		return;
	}

	/*
	** For HP-BLS we need to start as an MLS server
	*/
# if defined(hp8_bls)
        if ( m6mlserver(fd, 1) < 0 )
        {
                iisock_error( bsp, BS_LISTEN_ERR );
                return;
        }

# endif /* hp8_bls */

	/* Make address reusable */

 	if ( s->sa_family != AF_UNIX )
	{
            SOCK_TRACE(2)("iisock_listen: setsockopt(SO_REUSEADDR)\n");
	    if( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, 
		(char *)&on, sizeof( on ) ) < 0 )
	    {
		iisock_error( bsp, BS_LISTEN_ERR );
		return;
	    }
	}

	/* Trace bind() input info */

	if( ( sock_trace >= 2 ) && ( s->sa_family != AF_UNIX) )
	{
	    port_num = ntohs(((struct sockaddr_in *)(s))->sin_port);
	    SOCK_TRACE(2)("iisock_listen: %p bind fd=%d family=%d port=%d @sockaddr=%p len=%d\n",
		      bsp->closure, fd, s->sa_family, 
		      port_num,
		      s, size);

# ifdef xCL_TCPSOCKETS_IPV6_EXIST
	    getnameinfo(s, size, 
	                hostaddr, sizeof(hostaddr), 
	                port, sizeof(port), 
	                NI_NUMERICHOST | NI_NUMERICSERV);
	    SOCK_TRACE(2)("iisock_listen: %p bind ...host@=%s port=%s\n",
		          bsp->closure, hostaddr, port);
# endif /* xCL_TCPSOCKETS_IPV6_EXIST */
	}

	/* bind it */

	if( bind( fd, s, size ) < 0 )
	{
		iisock_error( bsp, BS_BIND_ERR );
        	SOCK_TRACE(1)("iisock_listen: %p bind failed!\n", bsp->closure);
		return;
	}

	/* Trace the assigned port # if input port was 0 */
	if( ( sock_trace >= 2 ) && ( s->sa_family != AF_UNIX)
	&&  ( port_num == 0 ) )
	{
	    if( getsockname( fd, (struct sockaddr *)&s_work, &s_work_len) == 0)
	    {
# ifdef xCL_TCPSOCKETS_IPV6_EXIST
		getnameinfo((struct sockaddr *)&s_work, s_work_len, 
	                    hostaddr, sizeof(hostaddr), 
	                    port, sizeof(port), 
	                    NI_NUMERICHOST | NI_NUMERICSERV);
		SOCK_TRACE(2)("iisock_listen: %p after bind, new port=%s host@=%s\n",
		              bsp->closure, port, hostaddr);
# else
	        port_num = ntohs(((struct sockaddr_in *)(&s_work))->sin_port);
		SOCK_TRACE(2)("iisock_listen: %p after bind, new port=%d\n",
		              bsp->closure, port_num);
# endif /* xCL_TCPSOCKETS_IPV6_EXIST */
	    }
	}

	/* set listen backlog Q */

	if ( PMget( "$.$.gca.socket.listen_qlen", &p ) != OK ||
	     CVan( p, &qlen ) != OK )
	{
	    qlen = LISTEN_Q;
	}

	/* enable connections */

	if( listen( fd, qlen ) < 0 )
	{
		iisock_error( bsp, BS_LISTEN_ERR );
		return;
	}


	/* return results OK */

	bsp->status = OK;
}

/*
** Name: iisock_listen_compl - intercept multiple listen completions
**
** Description:
**	This routine is only called when multiple network listens have
**	been issued for a single incoming GCC_LISTEN request.  This 
**	occurs, for instance, when supporting both IPv4 and IPv6 addresses.
**	This routine is called directly by CLPOLL due to a posted listen
**	request and basically intercepts what would normally have gone
**	to GCbssm() when state is GC_LSNCMP.  This "interception" is
**	enabled in iisock_regfd() for regop = BS_POLL_ACCEPT.
**
**	The purpose of the routine is to identify which listen fd was
**	posted in CLPOLL. Unfortunately, CLPOLL won't tell us the fd;
**	it only gives us a closure parm, which is normally the GCC_P_PLIST
**	parm in the BS code.  We can't save the fd into the parm because
**	there are multiple fd's involved.  Instead, the closures were set
**	to the addresses of the fdinfo entries.  From the fdinfo address,
**	the appropriate fd is obtained as well as a pointer to the lbcb
**	from which we can set up the needed context to resume the processing
**	through the normal code (func=GCbssm()).
**
**	This routine must also make sure that multiple concurrent network
**	listen completions do not call the completion routine for the same
**	GCC_LISTEN request.  Only "one" can call the completion routine; the
**	other must wait until a new GCC_LISTEN request comes in.
**	
**	NOTE that the input parameter list is different from the standard
**	routines in this module.
**
** History:
**      7-Oct-2006 (lunbr01)  Sir 116548
**          Created to handle multiple address listen completions for 
**          single GCC_LISTEN, in support of IPv6.
**      23-Mar-2007 (lunbr01) bug 117995
**	    Add missing 2nd parm (status) in and out to callback function.
**	    Not needed for remote conns (gccbs) but is needed for local.
*/

VOID
iisock_listen_compl( fd_posted, status )
struct fdinfo	*fd_posted;
STATUS       	status;
{
	LBCB	*lbcb = (LBCB *)fd_posted->lbcb_self;
	PTR	closure = lbcb->closure;

	SOCK_TRACE(2)("iisock_listen_compl: my closure=%p, listen func=%p closure=%p fd=%d status=%d\n",
		      fd_posted, lbcb->func, closure, fd_posted->fd_ai, status);

	if( !closure )  /* wait for next LISTEN request */
	{
	    fd_posted->fd_state = FD_STATE_PENDING_LISTEN_REQ;
	    return;
	}

	lbcb->closure = NULL;	/* make sure no one else gets this LISTEN req*/
	lbcb->fd      = fd_posted->fd_ai;
	fd_posted->fd_state = FD_STATE_INITIAL;
	lbcb->func( closure, status );	/* resume "normal" completion prcsg */
	return;
}

/*
** Name: iisock_unlisten - stop accepting incoming connections
**
** Description:
**	This routine can be called at shutdown to unregister and close
**	all the listen sockets.  It may also be used after a listen 
**	failure during startup to clean up before retrying (another port). 
**
** History:
**	17-May-93 (daveb)
**	    Unregister fds here too, 'cause if clpoll is caused
**	    after unlisten, we can get EBADF and much ensuing badness.
**      7-Oct-2006 (lunbr01)  Sir 116548
**          Free memory for addrinfo list and sockets/freeaddrinfots...
**          in support of IPv6.
**	1-Mar-2007 (lunbr01)  Bug 117783 + Sir 116548
**	    Clear new field num_good_sockets.
*/

VOID
iisock_unlisten( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	i4	fd;
	int	i;

	iisock_set_trace();

	if( lbcb->aiList )
	{
# ifdef xCL_TCPSOCKETS_IPV6_EXIST
	    freeaddrinfo( (struct addrinfo *)lbcb->aiList );
# else
	    MEfree( lbcb->aiList );
# endif /* IPv6 */
	    lbcb->aiList = NULL;
	    if( lbcb->fdList )
	    {
		for( i=0; i < lbcb->num_sockets; i++ )
		{
		    if( lbcb->fdList[i].fd_ai >= 0 )
		    {
		        fd = lbcb->fdList[i].fd_ai;
		        /* unregister file descriptors */
		        (void)iiCLfdreg( fd, FD_READ, (VOID (*))0, (PTR)0, -1 );
		        (void)iiCLfdreg( fd, FD_WRITE, (VOID (*))0, (PTR)0, -1 );
		        if( close( fd ) < 0 )
			    iisock_error( bsp, BS_CLOSE_ERR );
		    }
		}
		lbcb->num_sockets = lbcb->num_good_sockets = 0;
		MEfree( (PTR)lbcb->fdList);
		lbcb->fdList = NULL;
	    }
	}
	else   /* if aiList doesn't exist, then check lbcb->fd */
	if( lbcb->fd >= 0 )
	{
		/* unregister file descriptors */

		(void)iiCLfdreg( lbcb->fd, FD_READ, (VOID (*))0, (PTR)0, -1 );
		(void)iiCLfdreg( lbcb->fd, FD_WRITE, (VOID (*))0, (PTR)0, -1 );
		if( close( lbcb->fd ) < 0 )
			iisock_error( bsp, BS_CLOSE_ERR );
		lbcb->fd = -1;
	}
	bsp->status = OK;
}
	
/*
** Name: iisock_accept - accept a single incoming connection
**
** History:
**      31-Oct-2006 (lunbr01)  Sir 116548, bug 117023
**          Incoming connects were crashing GCC in iisock_accept() after
**          IPv6-related getnameinfo() call due to unitialized hostent
**          variable.  Modified getnameinfo() call to put hostname 
**          directly into bsp->buf and initialized hostent to NULL.
**      06-Feb-2007 (Ralph Loen) SIR 117590
**          Remove invocation of gethostbyaddr() (getnameinfo() for IPv6)
**          for network listens.
*/

VOID
iisock_accept( bsp, s, size )
BS_PARMS	*bsp;
struct sockaddr	*s;
i4		size;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;
	int	fd;
	int	local_errno;
# ifdef xCL_SUNOS_CMW
        int       status;
        tnet_attr_t attrp;
        caddr_t   bufp;
        int     attr_mask;
        int     defaults_mask;
# endif
# if defined(hp8_bls)
        int             status;
        m6attr_t        attrp;
        int             attr_mask, def_attr_mask;
        extern int 	errno;
# endif /* hp8_bls */
# if defined(pym_us5) || defined(rmx_us5) || defined(rux_us5) 
	sigset_t set, oset;
# endif

	SOCK_TRACE(2)("iisock_accept: listen fd=%d\n", lbcb->fd );

# if defined(pym_us5) || defined(rmx_us5) || defined(rux_us5)
	(void) sigfillset(&set);
	(void) sigprocmask(SIG_BLOCK, &set, &oset);
# endif
	/* try to do an accept */

	fd = accept( lbcb->fd, s, &size );
# if defined(pym_us5) || defined(rmx_us5) || defined(rux_us5)
	(void) sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);
# endif

	SOCK_TRACE(2)("iisock_accept: client fd=%d\n", fd );

	CLPOLL_FD( fd );

	SOCK_TRACE(2)("iisock_accept: returned from CLPOLL\n");

	if( fd < 0 )
	{
		iisock_error( bsp, BS_ACCEPT_ERR );
		return;
	}

# ifdef xCL_SUNOS_CMW
        /*
        ** Allocate attributes array and initialize two elements to hold
        ** the SL/IL pair of the client.
        */
        attr_mask = TNET_SW_SEN_LABEL | TNET_SW_INFO_LABEL;
        status = tnet_create_attr_buf(&attrp,
                                      attr_mask,
                                  (caddr_t *) NULL);
        if (status != 0) 
        {
            iisock_error( bsp, BS_ACCEPT_ERR );
	    return;
        }

        /*
        ** Find out the SL/IL pair of the client.
        */
        if (tnet_last_attrs(fd, attrp, &attr_mask) != 0 ) 
        {
            free(attrp);
            iisock_error( bsp, BS_ACCEPT_ERR );
	    return;
        }

        /*
        ** Set the default sensitivity label for data written to this socket to
        ** the label of the connecting client so the client can read without
        ** requiring privilege.
        */
        defaults_mask = 0;
        status = tnet_def_attrs(fd, attr_mask, defaults_mask, attrp);
        if (status != 0) 
        {
            iisock_error( bsp, BS_ACCEPT_ERR );
	    return;
        }

        free(attrp);
# endif
# if defined(hp8_bls)
            
        /*
        ** Allocate attributes array and initialize two elements to hold
        ** the SL of the client.
        */

	attr_mask = M6M_SEN_LABEL;
        status = m6create_attr_buf(&attrp, attr_mask, 0);

        if (status != 0)
        {
		free((char *) attrp);
		iisock_error( bsp, BS_ACCEPT_ERR );
		return;
        }

        /*
        ** Find out the SL of the client.
        */

        if (m6last_attr(fd, attrp, &attr_mask) != 0 )
        {
		free((char *) attrp);
		iisock_error( bsp, BS_ACCEPT_ERR );
		return;
        }

        /*
        ** Set the security label of socket to that of the incoming
        ** connection so that we can talk down to it.
        */

        def_attr_mask = 0;
        status = m6def_attr(fd, attr_mask, def_attr_mask, attrp);
        if (status != 0) 
        {
		free((char *) attrp);
		iisock_error( bsp, BS_ACCEPT_ERR );
		return;
        }

	free((char *) attrp);
# endif /* hp8_bls */

        /*
        ** Bsp->buf corresponds to the listen.node_id field in the GCC
        ** function_parms union.  This field should always be set to NULL.
        ** See gl/hdr/hdr/gcccl.h.
        */
        bsp->buf = NULL;
        bsp->len = 0;

	/* set options on socket */

	if ( bsp->regop != BS_POLL_INVALID )
	    iisock_nonblock( fd );

	/* return results OK */

	bcb->fd = fd;
	bsp->status = OK;

	SOCK_TRACE(2)("iisock_accept: exiting  client fd=%d\n", bcb->fd);
}

/*
** Name: iisock_request - make an outgoing connection
*/

VOID
iisock_request( bsp, s, size )
BS_PARMS	*bsp;
struct sockaddr	*s;
i4		size;
{
    BCB	*bcb = (BCB *)bsp->bcb;
    int	fd;
    int	errnum;
    int retry_count;

    for ( retry_count = 0; retry_count < 9; retry_count++ )
    {
	/* create a socket */

	fd = socket( s->sa_family, SOCK_STREAM, 0 );
	CLPOLL_FD( fd );

	if( fd < 0 )
	{
	    iisock_error( bsp, BS_SOCK_ERR );
	    return;
	}

	/* set for non-blocking */

	if ( bsp->regop != BS_POLL_INVALID )
	    iisock_nonblock( fd );

	/* enable any extended attributes  */
	if( iisock_ext_attr(bsp, fd)!= OK)
	{
	    iisock_error( bsp, BS_SOCK_ERR );
	    return;
	}

        /*
        ** Try to connect.
        ** We are checking both EINPROGRESS and EWOULDBLOCK below because
        ** dg8_us5 returns EWOULDBLOCK instead of EINPROGRESS on a connect
        ** to a non-blocking socket.
        */

	if( connect( fd, s, size ) < 0 )
	{
	    errnum = errno;
	    /* 
	    ** "If a connection request arrives with the queue full,
	    ** the client will receive an error with an indication of
	    ** ECONNREFUSED for AF_UNIX sockets."
	    ** "For AF_INET sockets, the tcp will retry the connection."
	    */
	    if ( errnum == ECONNREFUSED ) /* try again */
	    {
		iisock_error( bsp, BS_CONNECT_ERR );
		_VOID_ close( fd );
		sleep(1);
		continue;
	    }
	    if ( errnum != EINPROGRESS && errnum != EWOULDBLOCK )
	    {
		iisock_error( bsp, BS_CONNECT_ERR );
		_VOID_ close( fd );
		return;
	    }
	}

	/* return OK */

	bcb->fd = fd;
	bsp->status = OK;
	break;
    }
}

/*
** Name: iisock_connect - complete an outgoing connection
*/

VOID
iisock_connect( bsp, s, size )
BS_PARMS	*bsp;
struct sockaddr	*s;
i4	     	size;
{
	BCB	*bcb = (BCB *)bsp->bcb;

	/*
	** Check if connect succeeded.
	** Since connect() above may return EINPROGRESS, we must 
	** determine the state of the connect() after the select()
	** says we can write on it.  We try to get the peer name,
	** and it will fail (ENOTCONN) if we are not connected.
	** XXX Most (all?) systems don't support getpeername() on a unix
	** domain socket.  Fortunately, they return 0 if it's connected.
	*/
/*
** History:
**	11-nov-97 (popri01)
**	    Cross-integrated John Rodger's 1.2 fix (#432793) which we 
**	    hope provides a final solution to the Unixware 2.x 
**	    communication problems - use getsockname instead of getpeername.
*/

#ifdef usl_us5
	if( getsockname( bcb->fd, s, &size ) < 0 )
# else
	if( getpeername( bcb->fd, s, &size ) < 0 )
# endif
	{
		iisock_error( bsp, BS_CONNECT_ERR );
		_VOID_ close( bcb->fd );
		bcb->fd = -1;
		return;
	}

	/* return OK */

	bsp->status = OK;
}


/*
** BS entry points
*/
	
/*
** Name: iisock_send - send data down the stream
*/

VOID
iisock_send( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	int	n;

	/* short sends are OK */

	/* XXX write() may return 0 if O_NDELAY is used and */
	/* BS_SKIP_W_SELECT is set.  A return of 0 must not */
	/* be an error! */

        if( bcb->optim & BS_SOCKET_DEAD )
        {
            bsp->status = BS_WRITE_ERR;
            return;
        }

	if( ( n = write( bcb->fd, bsp->buf, bsp->len ) ) < 0 )
	{
		if( errno != EWOULDBLOCK ) 
		{
			iisock_error( bsp, BS_WRITE_ERR );
                	bcb->optim |= BS_SOCKET_DEAD;
			return;
		}
		else
		{
		   /* Under extreme socket contention in dgux, a write
		      may return EWOULDBLOCK.  If you get 500 consecutive
		      EWOULDBLOCKs, sleep for a second so the socket has
 		      a chance to catch up */    
# if defined(dgi_us5)
		   ewbcnt++;
		   if (ewbcnt>=500) 
		   {
			sleep(1);
			ewbcnt = 0;
		   }
# endif
		} 
		n = 0;
	}
 	else
	{
# if defined(dgi_us5)
	   if (ewbcnt)
		ewbcnt = 0;
# endif
	}	
	if( n == bsp->len )
	    bcb->optim |= BS_SKIP_W_SELECT;

	bsp->len -= n;
	bsp->buf += n;
	bsp->status = OK;
}

/*
** Name: iisock_receive - read data from a stream
*/

VOID
iisock_receive( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	int	n;
	int	errnum;

	/* short reads are OK - EOF is not */

	/* XXX read() may return 0 if O_NDELAY is used and */
	/* BS_SKIP_R_SELECT is set.  Since we use 0 to mean */
	/* EOF, BS_SKIP_R_SELECT must never be used! */

        if( bcb->optim & BS_SOCKET_DEAD )
        {
            bsp->status = BS_READ_ERR;
            return;
        }

	if( ( n = read( bcb->fd, bsp->buf, bsp->len ) ) <= 0 )
	{
		errnum = errno;
		if( !n )
		{
			bsp->status = BS_READ_ERR;
                        bcb->optim |= BS_SOCKET_DEAD;
			SETCLERR( bsp->syserr, 0, 0 );
			return;
		} 
		else if( errnum != EWOULDBLOCK && errnum != EINTR )
		{
                        bcb->optim |= BS_SOCKET_DEAD;
			iisock_error( bsp, BS_READ_ERR );
			return;
		}
		n = 0;
	}

	bcb->optim |= BS_SKIP_W_SELECT;

	bsp->len -= n;
	bsp->buf += n;
	bsp->status = OK;
}

/*
** Name: iisock_close - close a single connection
*/

VOID
iisock_close( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;

	if( bcb->aiList )
	{
# ifdef xCL_TCPSOCKETS_IPV6_EXIST
	    freeaddrinfo( (struct addrinfo *)bcb->aiList );
# else
	    MEfree( bcb->aiList );
# endif /* IPv6 */
	    bcb->aiList = bcb->aiCurr = NULL;
	}
		
	if( bcb->fd < 0 )
		goto ok;

	/* unregister file descriptors */

	(void)iiCLfdreg( bcb->fd, FD_READ, (VOID (*))0, (PTR)0, -1 );
	(void)iiCLfdreg( bcb->fd, FD_WRITE, (VOID (*))0, (PTR)0, -1 );

	if( close( bcb->fd ) < 0 )
	{
		iisock_error( bsp, BS_CLOSE_ERR );
		return;
	}

ok:
	bsp->status = OK;
}

/*
** Name: iisock_regfd - register for blocking operation
**
** History:
**	 6-Aug-04 (gordy)
**	    Initialize file descriptor in case timeout occurs prior
**	    to actually calling accept().
**      7-Oct-2006 (lunbr01)  Sir 116548
**          In support of IPv6, modify BS_POLL_ACCEPT to be able to handle
**          multiple sockets (one per address).
**	1-Mar-2007 (lunbr01)  Bug 117783 + Sir 116548
**	    Use num_good_sockets rather than num_sockets (total) as
**	    criteria for going into "multiple sockets" logic plus skip
**	    any bad sockets (from initial listen).
*/

bool
iisock_regfd( bsp )
BS_PARMS	*bsp;
{
	int	fd;
	int	op;
	BCB	*bcb = (BCB *)bsp->bcb;
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	int	i;
	bool	found_completed_listen = FALSE;

	SOCK_TRACE(4)("iisock_regfd: regop=%d func=%p closure=%p timeout=%d\n",
		      bsp->regop, bsp->func, bsp->closure, bsp->timeout);

	switch( bsp->regop )
	{
	case BS_POLL_ACCEPT:
		SOCK_TRACE(4)("iisock_regfd: BS_POLL_ACCEPT  #listen sockets=%d good out of %d total\n",
                      lbcb->num_good_sockets, lbcb->num_sockets);
		op = FD_READ;
		/*
		** Make sure bcb file descriptor is invalid
		** so that if a timeout occurs the bogus fd
		** value will be detected.
		*/
		bcb->fd = -1;

		if( lbcb->num_good_sockets > 1 )
		{
		    /* hijack normal callback & redirect to iisock_listen_compl() */
		    lbcb->func    = bsp->func;
		    lbcb->closure = bsp->closure;
		    for( i=0; i < lbcb->num_sockets; i++)
		    {
		        SOCK_TRACE(4)("iisock_regfd: BS_POLL_ACCEPT  [%d] state=%d fd=%d lbcb=%p\n",
                                      i, 
		                      lbcb->fdList[i].fd_state, 
		                      lbcb->fdList[i].fd_ai, 
		                      lbcb->fdList[i].lbcb_self);
			if( lbcb->fdList[i].fd_ai < 0 )  /* skip bad sockets */
			    continue;
		        switch( lbcb->fdList[i].fd_state)
		        {
		        case FD_STATE_INITIAL:   /* need to register the fd */
		            fd = lbcb->fdList[i].fd_ai;
		            if( fd >= 0)
		            {
		              /* Pass control to CLpoll */
		              iiCLfdreg( fd, op, iisock_listen_compl,
		                         (PTR)&lbcb->fdList[i], bsp->timeout );
		              lbcb->fdList[i].fd_state = FD_STATE_REGISTERED;
		            }
		            break;
		        case FD_STATE_REGISTERED:
		            break;
		        case FD_STATE_PENDING_LISTEN_REQ:
		            if( found_completed_listen )  /* only take one */
		                continue;
		            else
		            {
		                lbcb->fd = lbcb->fdList[i].fd_ai;
		                lbcb->fdList[i].fd_state = FD_STATE_INITIAL;
		                found_completed_listen = TRUE;
		            }
		            break;
		        } /* end switch on state */
		    } /* end for loop thru the socket fd's */
		    if( found_completed_listen )
		        return FAIL;
		    else
		        return TRUE;
		} /* endif more than 1 listen socket fd */
		else
		    fd = lbcb->fd; 
		break;

	case BS_POLL_SEND:
		if( bcb->optim & BS_SKIP_W_SELECT )
		{
			bcb->optim &= ~BS_SKIP_W_SELECT;
			return FALSE;
		}
		/* fall through */

	case BS_POLL_CONNECT:
		fd = bcb->fd; op = FD_WRITE; break;

	case BS_POLL_RECEIVE:
		if( bcb->optim & BS_SKIP_R_SELECT )
		{
			bcb->optim &= ~BS_SKIP_R_SELECT;
			return FALSE;
		}

		fd = bcb->fd; op = FD_READ; break;

	default:
		return FALSE;
	}

	if( fd < 0 )
		return FALSE;

	/* Pass control to CLpoll */

	iiCLfdreg( fd, op, bsp->func, bsp->closure, bsp->timeout );

	return TRUE;
}

/*
** Name: iisock_ok - dummy function returning OK
*/

VOID
iisock_ok( bsp )
BS_PARMS	*bsp;
{
	bsp->status = OK;
}

/*
** Name: iisock_ext_attr - enable extended attributes on a socket
*/

i4
iisock_ext_attr( BS_PARMS *bsp, i4  fd)
{

# if defined(hp8_bls)
	static bool mand_call=FALSE;
# endif

# if defined(hp8_bls)
        /* m6ext_err() produces undefined symbols
        m6errno = m6ext_err();
        */

        /* 
        ** Intialise system wide security variables with one
        ** call to mand_init(), this will set up the globals
        ** mand_syshi, mand_syslo, mand_max_class and mand_max_cat
        ** such that calls to mand_bytes(), m6peek() will return correct 
        ** values.
        */
        if(mand_call == FALSE) 
        {
            if(mand_init() != 0)
            {
                return !OK;
            } 
            else 
            {
                mand_call = TRUE;
            }
        }
        if ( m6ext_attr(fd, M6_EXT_ATTRS_ON) < 0 )
        {
            return !OK;
        }
# endif

# ifdef xCL_SUNOS_CMW
        /*
        ** Enable the extended network interface on this socket.
        */
        if ( tnet_ext_attrs(fd, 1) < 0 ) 
        {
	    return !OK;
        }
# endif
	return OK;
}

/*
** Name: iisock_ext_info, get socket extended info
**
** Description:
**	This routine tries to provide "extended information" about a
**	socket, such as security label and real user name. Getting this
**	kind of information can be expensive so we only do it on
**	request. Its also potentially quite large so we don't try & store
**	it in a LBCB or similar.
**
**	The caller sets flags in info_request to specify which pieces of
**	information are needed. The values actually returned are set in
**	in info_value - that is its legal for only some of the information
**	to be available. The caller should figure out what to do in this
**	case. 
**
**	Memory for the return values is supplied by the caller.
**
** History:
**	1-jun-94 (robf)
**          Created.
**	1-jun-94 (arc)
**          Minor fixes to CMW implementation of sock_ext_info.
**          Return user name from socket extended attributes.
**	2-jun-94 (arc)
**          Must be able to return username and/or security label
**          independently.
**      7-jun-94 (ajc)
**          Added hp8_bls specific entries for uid.
**	 6-Jun-03 (gordy)
**	    Added local and remote network addresses.
*/
STATUS
iisock_ext_info(BS_PARMS *bsp, BS_EXT_INFO *info)
{
    char 		textlabel[128];
    CL_ERR_DESC 	sys_err;
    BCB        		*bcb = (BCB *)bsp->bcb;
    STATUS     		ret_status=OK;

    iisock_set_trace();

    info->info_value=0; /* Nothing returned */
    /*
    ** Check if security label is wanted
    */
    for(;;)
    {
        if(info->info_request & BS_EI_USER_ID)
	{
	    /*
	    ** Get user id associated with socket.
	    ** info->user_id should hold output value.
	    ** max size is set by info->len_user_id
	    ** on output, info->len_user_id should be set to 
	    ** the actual length of the user name, and not be null
	    ** terminated.
	    */
	    if(info->len_user_id<1 || 
	       info->user_id==NULL)
	    {
	        /*
	        ** Invalid parameter
	        */
	        ret_status=BS_INTERNAL_ERR;
	        break;
	    }
# ifdef xCL_B1_SECURE
	    info->info_value|=BS_EI_USER_ID;
# ifdef xCL_SUNOS_CMW
            {
                int       status;
                u_long    attr_mask;
                tnet_attr_t attrp;
                uid_t uid;
                struct passwd *pwentry, *getpwuid();
                i4  uid_len;

                attr_mask = TNET_SW_UID;

                status = tnet_create_attr_buf(&attrp, attr_mask, (caddr_t*)0);
                if (status != 0) 
                {
                    SOCK_TRACE(1)("sock_ext_info: tnet_create_attr_buf() failed, status %d errno %d\n",
                                          status, errno);
                    ret_status = BS_INTERNAL_ERR;
                    break;
                }

                /*
                ** Get the UID of the client.
                */
                status = tnet_last_attrs(bcb->fd, attrp, &attr_mask);
                if(status != 0)
                {
                    SOCK_TRACE(2)("sock_ext_info: tnet_last_attrs() failed, status %d errno %d\n",
                                  status, errno);
                    ret_status = BS_INTERNAL_ERR;
                    break;
                }

                uid = tn_uid(attrp);
                if ((pwentry = getpwuid (uid)) == (struct passwd *)NULL)
                {
                    SOCK_TRACE(2)("sock_ext_info:getpwuid(%d) failed (errno %d)\n",
                              uid, errno);
                    ret_status = BS_INTERNAL_ERR;
                    free(attrp);
                    break;
                }
                if ((uid_len = STlength(pwentry->pw_name)) > info->len_user_id)
                {
                    SOCK_TRACE(2)("sock_ext_info:user buffer too small (%d) user id = %d chars\n",
                              info->len_user_id, uid_len);
                    ret_status = BS_INTERNAL_ERR;
                    free(attrp);
                    break;
                }

                /*
                ** Return the user name and length (without null terminator)
                */
                STncpy( info->user_id, pwentry->pw_name, uid_len);
                info->user_id[ uid_len ] = EOS;
                info->len_user_id = uid_len;

                free(attrp);
            }
# endif

#if defined(hp8_bls)
            {
                int             status;
                m6attr_t        attrp;
                int		attr_mask;
                uid_t uid;
                struct passwd	*pwentry, *getpwuid();
		struct passwd	pwd;
		char            pwuid_buf[512];
                i4  uid_len;
                extern int	errno, m6errno;


                /*
                ** Allocate attributes array and initialize elements to hold
                ** the uid of the client.
                */

                attr_mask = M6M_UID;
                
                status = m6create_attr_buf(&attrp, attr_mask, 0);
                if (status != 0)
                {
                    SOCK_TRACE(1)("sock_ext_info: m6create_attr_buf() for \
                        uid failed: %d\n", errno);
                    free(attrp);
                    ret_status=BS_INTERNAL_ERR;
                    break;
                }

                /*
                ** Find out the uid of the client from the socket.
                */
                m6errno = 0;
                if (m6last_attr( bcb->fd, attrp,&attr_mask) != 0 )
                {
                    SOCK_TRACE(2)("sock_ext_info: m6last_attr() failed:%d\n", 
                        errno);
                    free(attrp);
                    ret_status=BS_INTERNAL_ERR;
                    break;
                }
                
                uid = m6_uid(attrp);
                if ((pwentry = iiCLgetpwuid (uid, &pwd, pwuid_buf, sizeof(pwuid_buf))) == (struct passwd *)NULL)
                {
                    SOCK_TRACE(2)("sock_ext_info:getpwuid(%d) \
                        failed (errno %d)\n", uid, errno);
                    ret_status = BS_INTERNAL_ERR;
                    free(attrp);
                    break;
                }
                if ((uid_len = STlength(pwentry->pw_name)) > info->len_user_id)
                {
                    SOCK_TRACE(2)("sock_ext_info:user buffer too small (%d) \
                        user id = %d chars\n", info->len_user_id, uid_len);
                    ret_status = BS_INTERNAL_ERR;
                    free(attrp);
                    break;
                }

                /*
                ** Return the user name and length (without null terminator)
                */
                STncpy( info->user_id, pwentry->pw_name, uid_len);
		info->user_id[ uid_len ] = EOS;
                info->len_user_id = uid_len;               

                free((char *) attrp);
            }
# endif /* hp8_bls */
# endif
	} /* End user id */

	/* Stop on error */
	if ( ret_status != OK )  break;

	if ( info->info_request & BS_EI_LCL_ADDR )
	{
	    struct sockaddr_in	s;
	    int		size = sizeof( s );
	    char	*node;
	    char	port[16];

	    if ( info->len_lcl_node < 1  ||  info->lcl_node == NULL  ||
		 info->len_lcl_port < 1  ||  info->lcl_port == NULL )
	    {
	        /* Invalid parameter */
	        ret_status = BS_INTERNAL_ERR;
	        break;
	    }

	    if ( getsockname( bcb->fd, (struct sockaddr *)&s, &size ) < 0  ||
		 s.sin_family != AF_INET )
	    {
		ret_status = BS_INTERNAL_ERR;
		break;
	    }
	    else
	    {
		info->info_value |= BS_EI_LCL_ADDR;

		/* Return local node */
		node = inet_ntoa( s.sin_addr );

		if ( STlength( node ) < info->len_lcl_node )
		    STcopy( node, info->lcl_node );
		else
		{
		    STncpy( info->lcl_node, node, info->len_lcl_node );
		    info->lcl_node[ info->len_lcl_node - 1 ] = EOS;
		}

		/* Return local port */
		CVla( (i4)ntohs( s.sin_port ), port );

		if ( STlength( port ) < info->len_lcl_port )
		    STcopy( port, info->lcl_port );
		else
		{
		    STncpy( info->lcl_port, port, info->len_lcl_port );
		    info->lcl_port[ info->len_lcl_port - 1 ] = EOS;
		}
	    }
	}

	/* Stop on error */
	if ( ret_status != OK )  break;

	if ( info->info_request & BS_EI_RMT_ADDR )
	{
	    struct sockaddr_in	s;
	    int		size = sizeof( s );
	    char	*node;
	    char	port[16];

	    if ( info->len_rmt_node < 1  ||  info->rmt_node == NULL  ||
		 info->len_rmt_port < 1  ||  info->rmt_port == NULL )
	    {
	        /* Invalid parameter */
	        ret_status = BS_INTERNAL_ERR;
	        break;
	    }

	    if ( getpeername( bcb->fd, (struct sockaddr *)&s, &size ) < 0  ||
		 s.sin_family != AF_INET )
	    {
		ret_status = BS_INTERNAL_ERR;
		break;
	    }
	    else
	    {
		info->info_value |= BS_EI_RMT_ADDR;

		/* Return local node */
		node = inet_ntoa( s.sin_addr );

		if ( STlength( node ) < info->len_rmt_node )
		    STcopy( node, info->rmt_node );
		else
		{
		    STncpy( info->rmt_node, node, info->len_rmt_node );
		    info->rmt_node[ info->len_rmt_node - 1 ] = EOS;
		}

		/* Return local port */
		CVla( (i4)ntohs( s.sin_port ), port );

		if ( STlength( port ) < info->len_rmt_port )
		    STcopy( port, info->rmt_port );
		else
		{
		    STncpy( info->rmt_port, port, info->len_rmt_port );
		    info->rmt_port[ info->len_rmt_port - 1 ] = EOS;
		}
	    }
	}

	/*
	** Add other new options here
	*/

	/* Done all options */
	break;
    } /* FOR loop to break out of */

    return( ret_status );
}


/*
** iisock_set_trace - sets trace level for sockio. 
*/
static VOID
iisock_set_trace()
{
	static bool init=FALSE;
	char	*trace;

	if(init)
	    return;

	init=TRUE;

	NMgtAt( "II_SOCK_TRACE",  &trace );

	if ( !( trace && *trace ) && PMget("!.sock_trace_level", &trace) != OK )
	    sock_trace = 0;
	else
	    sock_trace =  atoi(trace);
}

# endif /* x_SOCKETS_EXIST */
