/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<clpoll.h>
# include	<st.h>
# include	<me.h>
# include	<er.h>

# include	<errno.h>
# include	<bsi.h>

# ifdef xCL_TLI_TCP_EXISTS

# include	<systypes.h>
# include	<clsocket.h>
# include	<sys/stream.h>
# include	<tiuser.h>
# include	<sys/tihdr.h>
# if  defined(sqs_ptx)
# include       <netinet/netinet.h>
# endif /* sqs_ptx */
# ifdef xCL_006_FCNTL_H_EXISTS
# include	<fcntl.h>
# endif

# include	"bstliio.h"


/*
** Name: bstlitcp.c - BS interface to TCP via TLI.
**
** Description:
**	This file provides access to TCP IPC via TLI sockets, using the 
**	generic BS interface defined in bsi.h.  See that file for a 
**	description of the interface.
**
**	The following functions are internal to this file:
**
**		tcp_options - turn on TCP_NODELAY
**
**	The following functions are exported as part of the BS interface:
**
**		tcp_listen - establish an endpoint for accepting connections
**		tcp_request - make an outgoing connection
**		tcp_accept - accept a single incoming connection
**
** History:
**	26-May-90 (seiwald)
**	    Written.
**	30-Jun-90 (seiwald)
**	    Private and Lprivate renamed to bcb and lbcb, as private
**	    is a reserved word somewhere.
**	1-Jul-90 (seiwald)
**	    Fixes for Gautam.
**	    - Call t_free for all items t_alloc'ed.
**	    - Consolidate t_bind0 and t_bind1 into t_bind0.
**	    - Allocated tli_accept's t_call in tli_listen; free it 
**	      in tli_unlisten.
**	    - Allow addresses to be prefixed with "/dev/name::" -
**	      tli_listen returns a listen address so formatted if 
**	      the device name is different from the default TLI_TCP.
**	    - Use t_optmgmt instead of ioctl for setting TCP_NODELAY.
**	      The call to tli_nodelay must be from the T_IDLE state.
**	    - Send orderly release on close.  We can assume this is
**	      TCP and therefore COTS_ORD.
**	5-Jul-90 (seiwald)
**	    Use CLPOLL_RESVFD to save fds for SYSV stdio.
**	6-Jul-90 (seiwald)
**	    Straighten out tli_error and make it use the new BS_ERR
**	    for getting useful system info out.
**	15-Aug-90 (gautam/gordonw)
**	    Changed the tli_accept() to handle multiple connect requests
**	    and corresponding Q handling routines. Also added tli_release,
**	    tli_abort, functions to support orderly release 
**	31-Aug-90 (seiwald)
**	    o Tuned some of the tracing levels.
**	    o Made tli_error output useful data for TLOOK erorrs.
**	    o Stopped calling tli_error for non-tli errors.
**	    o Removed wrongful setting of errno and t_errno.
**	    o Removed duplicating formatting of port name in tli_open.
**	    o Reinstated t_rcvconnect on async connect completion.
**	    o Removed retry logic from tli_send, tli_receive.
**	    o Moved inline the handling of the listen queue.
**	    o Reworked tli_accept logic, so that it doesn't attempt to
**	      call t_snddis on the fd from the failed t_accept.
**	    o Stripped out release, abort code: that can't be handled at
**	      this level.
**	07-Nov-90 (anton)
**	    Fault in read buffer for MCT
**	5-Dec-90 (seiwald)
**	    Don't count the null in the size of the error string
**	    and don't bother appending a newline to the string.
**	30-Jan-91 (anton)
**	    Don't read or write if connection is closed
**	    MCT needs to lock out CLpoll during close.
**	7-Aug-91 (seiwald)
**	    Use new CLPOLL_FD(), replacing CLPOLL_RESVFD().
**	26-Sep-91 (seiwald) bug #39654
**	    Turn on keepalives.
**	25-Nov-91 (seiwald)
**	    Use t_sync() after CLPOLL_FD() so that the new descriptor
**	    is known to TLI; include <clsocket.h> rather than the
**	    sundry other files; define TLI_TCP if it's not already
**	    there.
**	6-Dec-91 (seiwald)
**	    Common routines moved out to bstliio.c.
**	20-Jan-92 (sweeney)
**	    tihdr.h should always be in /usr/include.
**	24-Feb-92 (sweeney)
**	    [ swm history comment for sweeney changes ]
**	    Pass size twice to tli_{listen,request,detect} - actual address
**	    size and maximum address size can differ for OSI.
**	29-aug-91 (bonobo)
**	    Added bogus function to eliminate ranlib warnings.
**	9-Sep-92 (gautam)
**	    Cleanup & porting changes : 
**          o Removed tli functions externals. These are now in bstliio.h.
**          o tli_request takes in 2 more arguments
**          o tcp_detect made more general
**	28-oct-92 (peterk)
**	    in tcp_options() the #ifdef sun should be #ifndef sun, better
**	    should be a capability keyed to SPX.
**      26-feb-93 (mikem) integrated following from 6.4: 28-oct-92 (peterk)
**          in tcp_options() the #ifdef sun should be #ifndef sun, better
**          should be a capability keyed to SPX.
**	04-Mar-93 (edg)
**	    Removed detect logic.
**      15-Mar-93 (rkumar)
**          include netinet/netinet.h for TP_LINGER, TP_KEEPALIVE etc.,
**          defines  for sqs_ptx
**	16-Mar-93 (brucek)
**	    Added tli_release to BS_DRIVER.
**	17-Mar-93 (brucek)
**	    Added NCR flavor of tcp_options.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-Jul-93 (brucek)
**	    Added Solaris flavor of tcp_options; 
**	    made tcp_options and tli_accept static VOIDs;
**	    added tcp_transport arg on call to tli_listen.
**	25-Apr-1995 (murf)
**		Added dr6_us5 flavour of tcp_options as in
**		the above change.
**      15-may-95 (thoro01)
**	    Added an ifdef in the #else for nc4_us5 code to check that the
**	    symbols T_NEGOTIATE, TP_NODELAY, TP_KEEPALIVE and TP_LINGER are
**	    defined.  This is to avoid compiler errors for HP/UX systems.
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with su4_us5.
**              Reason, to port Solaris for Intel.
**      19-nov-2002 (loera01)
**          Set is_remote to TRUE in BS_PARMS.  This change may not work if
**          an installation uses TLI TCP for both local and remote connections.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

extern	STATUS	BS_tcp_port();

#ifdef t15_us5
#define TLI_TCP "/dev/inet/tcp"
#endif

#ifndef TLI_TCP
#define TLI_TCP "/dev/tcp"
#endif

/*
** Name: tcp_options - turn on TCP_NODELAY
*/

static VOID
tcp_options( fd )
int     fd;
{
	/*
	** Must be called in T_IDLE (just after calling t_bind).
	*/

	struct t_optmgmt t_opt;
	int	status;

# if defined (sparc_sol) || defined (a64_sol) || defined(sui_us5)
 
        /*
        ** Set no_delay, keepalives, and linger on.
        */
 
        struct {
                struct opthdr   opthdr;
                union {
                        int             value;
                        struct linger   linger;
                } optval;
        } tcp_options;
 
        t_opt.flags = T_NEGOTIATE;
        t_opt.opt.len = sizeof(tcp_options.opthdr) + sizeof(int);
        t_opt.opt.maxlen = sizeof( tcp_options );
        t_opt.opt.buf = (char *)&tcp_options;
 
        tcp_options.opthdr.level = IPPROTO_TCP;
        tcp_options.opthdr.name = TCP_NODELAY;
        tcp_options.opthdr.len = sizeof(tcp_options.optval.value);
        tcp_options.optval.value = 1;

        (void)t_optmgmt( fd, &t_opt, &t_opt );
 
        t_opt.flags = T_NEGOTIATE;
        t_opt.opt.len = sizeof(tcp_options.opthdr) + sizeof(int);
        t_opt.opt.maxlen = sizeof( tcp_options );
        t_opt.opt.buf = (char *)&tcp_options;
 
        tcp_options.opthdr.level = SOL_SOCKET;
        tcp_options.opthdr.name = SO_KEEPALIVE;
        tcp_options.opthdr.len = sizeof(tcp_options.optval.value);
        tcp_options.optval.value = 1;
 
        (void)t_optmgmt( fd, &t_opt, &t_opt );
 
        t_opt.flags = T_NEGOTIATE;
        t_opt.opt.len = sizeof(tcp_options.opthdr) + sizeof(int);
        t_opt.opt.maxlen = sizeof( tcp_options );
        t_opt.opt.buf = (char *)&tcp_options;
 
        tcp_options.opthdr.level = SOL_SOCKET;
        tcp_options.opthdr.name = SO_LINGER;
        tcp_options.opthdr.len = sizeof(tcp_options.optval.linger);
        tcp_options.optval.linger.l_onoff = 1;
        tcp_options.optval.linger.l_linger = 10;
 
        (void)t_optmgmt( fd, &t_opt, &t_opt );
 
# else  /* solaris */
# ifdef nc4_us5

	/*
	** Set no_delay, keepalives, and linger on.
	*/

	struct {
		struct opthdr	opthdr;
		union {
			long		value;
			struct linger	linger;
		} optval;
	} tcp_options;

	t_opt.flags = T_NEGOTIATE;
	t_opt.opt.len = sizeof( tcp_options );
	t_opt.opt.maxlen = sizeof( tcp_options );
	t_opt.opt.buf = (char *)&tcp_options;

	tcp_options.opthdr.level = IPPROTO_TCP;
	tcp_options.opthdr.name = TO_NODELAY;
	tcp_options.opthdr.len = OPTLEN(sizeof(tcp_options.optval.value));
	tcp_options.optval.value = 1;

	(void)t_optmgmt( fd, &t_opt, &t_opt );

	tcp_options.opthdr.level = IPPROTO_TCP;
	tcp_options.opthdr.name = TO_KEEPALIVE;
	tcp_options.opthdr.len = OPTLEN(sizeof(tcp_options.optval.value));
	tcp_options.optval.value = 1;

	(void)t_optmgmt( fd, &t_opt, &t_opt );

	tcp_options.opthdr.level = IPPROTO_TCP;
	tcp_options.opthdr.name = TO_LINGER;
	tcp_options.opthdr.len = OPTLEN(sizeof(tcp_options.optval.linger));
	tcp_options.optval.linger.l_onoff = 1;
	tcp_options.optval.linger.l_linger = 10;

	(void)t_optmgmt( fd, &t_opt, &t_opt );

# else  /* nc4_us5 */

# if defined(T_NEGOTIATE) && defined(TP_NODELAY) &&\
     defined(TP_KEEPALIVE) && defined(TP_LINGER)
	/*
	** Set no_delay, keepalives, and linger on (to the default),
	** and use default buffers.
	*/

	struct tcp_options	tcp_options;

	tcp_options.pr_options = TP_NODELAY | TP_KEEPALIVE | TP_LINGER;
	tcp_options.ltime = TP_LINGDEF;
	tcp_options.snd_buf = 0;
	tcp_options.rcv_buf = 0;

	t_opt.flags = T_NEGOTIATE;
	t_opt.opt.len = sizeof( tcp_options );
	t_opt.opt.maxlen = sizeof( tcp_options ); 
	t_opt.opt.buf = (char *)&tcp_options;

	(void)t_optmgmt( fd, &t_opt, &t_opt );

# ifdef TLIDEBUG
	TLITRACE(2)( "t_optmgmt options %x ltime %d snd %d rcv %d\n",
			tcp_options.pr_options,
			tcp_options.ltime,
			tcp_options.snd_buf,
			tcp_options.rcv_buf );
# endif /* TLIDEBUG */
# endif /* defined(T_NEGOTIATE) && defined(TP_NODELAY) && */
        /* defined(TP_KEEPALIVE) && defined(TP_LINGER) */ 
	  
# endif /* nc4_us5 */
# endif /* solaris */
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
	struct	sockaddr_in s;
	char	*addr;

	/* clear fd for sanity */

	lbcb->fd = -1;

	/* get device */

	tli_addr( TLI_TCP, bsp->buf, lbcb->device, &addr );

	/* get listen port, if specified */

	if( ( bsp->status = BS_tcp_addr( addr, FALSE, &s ) ) != OK )
	{
		SETCLERR( bsp->syserr, 0, 0 );
		return;
	}

	/* do tli listen */

	tli_listen( bsp, (char *)&s, sizeof( s ), sizeof( s ), TRUE );

	if( bsp->status != OK )
	    return;

	/* get name for output */

	if( STcompare( lbcb->device, TLI_TCP ) )
	    STprintf( lbcb->port, "%s::%d", lbcb->device, ntohs( s.sin_port ) );
	else
	    STprintf( lbcb->port, "%d", ntohs( s.sin_port ) );

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

	/* 
	** Do tli accept.  NOTE: since TLI TCP may be used for both
	** local and remote connections, the assumption about a remote 
	** connection below may require revision.
	*/

	bsp->is_remote = TRUE;
	tli_accept( bsp );

	if( bsp->status != OK )
		return;

	/* Turn on TCP_NODELAY */

	tcp_options( bcb->fd );
}

/*
** Name: tcp_request - make an outgoing connection
*/

static VOID
tcp_request( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	char	device[ MAXDEVNAME ];
	struct	sockaddr_in s;
	char	*addr;

	/* clear fd for sanity */

	bcb->fd = -1;

	/* get device */

	tli_addr( TLI_TCP, bsp->buf, device, &addr );

	/* translate address */

	if( ( bsp->status = BS_tcp_addr( addr, TRUE, &s ) ) != OK )
    	{
		bsp->status = BS_INTERNAL_ERR;
		return;
	}

	/* do tli connect request */

	tli_request( bsp, device, (char *)&s, sizeof( s ), NULL, 0,sizeof( s ));

	if( bsp->status != OK )
		return;

	/* Turn on TCP_NODELAY */

	tcp_options( bcb->fd );
}


/*
** Exported driver specification.
*/

BS_DRIVER BS_tlitcp = {
	sizeof( BCB ),
	sizeof( LBCB ),
	tcp_listen,
	tli_unlisten,
	tcp_accept,
	tcp_request,
	tli_connect,
	tli_send,
	tli_receive,
	tli_release,
	tli_close,
	tli_regfd,
	tli_sync,	/* save */
	tli_sync,	/* restore */
	BS_tcp_port,	
} ;

# else
VOID xCL_042_TLI_DOES_NOT_EVEN_EXIST(){};
# endif /* xCL_TLI_TCP_EXISTS */
