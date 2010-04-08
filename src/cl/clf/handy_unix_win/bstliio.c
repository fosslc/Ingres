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
# include	<cs.h>

# include	<errno.h>
# include	<bsi.h>

/* we only bother with this if we have TLI in some form or other */

# if	defined(xCL_TLI_TCP_EXISTS) || \
	defined(xCL_TLI_OSLAN_EXISTS) || \
	defined(xCL_TLI_X25_EXISTS) || \
	defined(xCL_TLI_SPX_EXISTS) || \
	defined(xCL_TLI_UNIX_EXISTS)

# include	<systypes.h>
# include	<clsocket.h>

# if  defined(any_aix) || defined(any_hpux)
# include	<xti.h>
# else
# include	<sys/stream.h>
# include	<tiuser.h>
# endif     /* aix, hpux */

# ifdef xCL_006_FCNTL_H_EXISTS
# include	<fcntl.h>
# endif /* xCL_006_FCNTL_H_EXISTS */

/*
** For asynchronous operation define OSI_NONBLOCK: try to use O_NONBLOCK,
** O_NDELAY, or FNDELAY.
*/
# if !defined(OSI_NONBLOCK) && defined(O_NONBLOCK)
# define        OSI_NONBLOCK     O_NONBLOCK
# endif /* !OSI_NONBLOCK && O_NONBLOCK */
# if !defined(OSI_NONBLOCK) && defined(O_NDELAY)
# define        OSI_NONBLOCK     O_NDELAY
# endif /* !OSI_NONBLOCK && O_NDELAY */
# if !defined(OSI_NONBLOCK) && defined(FNDELAY)
# define        OSI_NONBLOCK     FNDELAY
# endif /* !OSI_NONBLOCK && FNDELAY */

# include	"bstliio.h"

/* External variables */


/*
** Name: bstliio.c - support routines for TLI interface
**
** Description:
**	This file contains routines to support BS to TLI interfaces.
**	These routines are meant to be exported directly or called by
**	the actual module which implements a BS interface to a protocol
**	via TLI.  For example, bstlitcp.c uses these routines to help
**	implement the TLI TCP BS driver.
**
**	The following functions are internal support routines, and
**	are called by the interface routines:
**
** 		tli_error - format CL_ERR_DESC after TLI error
**		tli_addr - strip /dev/name:: name from address
**
**	The following functions nearly conform to the BS interface,
**	but take extra parameters which provide addressing info:
**
**		tli_listen - establish an endpoint for accepting connections
**		tli_request - make an outgoing connection
**
**	The following functions conform to the BS interface, and
**	may be exported as interface routines or called by interface
**	routines:
**
**		tli_unlisten - stop accepting incoming connections
**		tli_accept - accept a single incoming connection
**		tli_connect - complete an outgoing connection
**		tli_send - send data down the stream
**		tli_receive - read data from a stream
**		tli_release - send release indication
**		tli_close - close a single connection
**		tli_abort - abort a single connection
**		tli_regfd - register for blocking operation
**
**	Note that send, receive, and regfd are high traffic routines
**	and should not have further layers of indirection.
**
** History:
**	6-Dec-91 (seiwald)
**	    Extracted from bstlitcp.c.
**	20-Jan-92 (sweeney)
**          Don't look in sys for tiuser.h (it may not be there...)
**	24-Jan-92 (sweeney)
**	    There may be more that one TLI transport provider 
**	    - check for them all.
**	24-Jan-92 (sweeney)
**          use the symbolic names for tli_looks[x].mask from tihdr.h
**	24-Feb-92 (sweeney)
**	    [ swm history comment for sweeney changes ]
**	    Several cleanup changes:
**          o added 'caller' parameter to tli_error() so that caller function
**	      name is logged;
**	    o made tracing of TLI/XTI t_errno, tlook and regfd values symbolic
**	      for readability;
**	    o added II_TLI_TRACE environment variable for conditionally
**	      compiled trace code;
**	    o inserted additional trace statements;
**	    o added maxlen parameter to tli_{listen,request,detect} (for non-
**	      TCP protocols len and maxlen are not necessarily the same);
**	    o get back t_info data from t_getinfo() and for message protocols
**	      return a receive length of 0, only if T_MORE is not set, the
**	      BK driver calculates the no. bytes received using buffer
**	      pointer arithmetic (see bsi.h interface definition); 
**	    o on dra_us5 t_free tried to free memory that was not t_alloc-ed,
**	      set the buf pointers affected to NULL before each t_free to
**	      avoid this behaviour (dra_us5 only);
**	    o anonymous t_bind would not work with ICL OSLAN, so specified
**	      bind addressing explicitly - also XTI does not support
**	      anonymous binds;
**	    o define OSI_NONBLOCK for asynchronous operation using O_NONBLOCK,
**	      O_NDELAY, or FNDELAY [ swm change ].
**	08-Sep-92 (gautam)
**	    More cleanup changes:
**          o extract t_info from open() information into LBCB.
**          o make tli_xxx routines more generic so it will work with OSI/XTI
**            (no more t_free/t_alloc/t_getinfo usage - now only MEreqmem based)
**          o fix tli_receive  bug for tsdu service 
**          o tli_request has more input parameters for non-anonymous bind
**               (local bind address and addrlen)
**          o make sure t_errno is in range of t_errnos array in tli_error
**          o porting changes for RS/6000 OSI support
**	17-Sep-92 (gautam)
**	    o flags field introduced into lbcb for NO_ANON_TBIND handling
**	26-Aug-92 (seiwald)
**	    Added xCL_TLI_SPX_EXISTS to the list of TLI's.
**	    Put "static" in front of definition of regops array, and
**	    reordered entries to be correct.
**	    Removed attempts to trace addrbuf - it is unprintable for 
**	    most protocols.
**	06-Oct-92 (sweeney)
**	    remove some redundant #defines.
**	9-Oct-92 (gautam)
**	    gather t_info information in tli_request.
**	22-Oct-92 (seiwald)
**	    Trace disconnect reason provided by t_rcvdis().
**	04-Jan-93 (edg)
**	    Got rid of NM symbol II_TLI_QLEN and replaced it with PM
**	    symbol ii.host.gca.tli.listen_qlen.
**      4-jan-93 (deastman)
**          This is relevant to xCL_TLI_UNIX_EXISTS as well.
**	11-Jan-93 (edg)
**	    Use gcu_get_tracesym() to get trace level.
**	12-jan-93 (seiwald)
**	    Reordered attempt to push orderly release through: first call
**	    t_sndrel() to send orderly release, then issue blocking t_rcv() 
**	    to pick up orderly release indication from peer.
**	18-Jan-93 (edg)
**	    Replaced gcu_get_tracesym() with inline code.
**	2-feb-93 (peterk)
**	    eliminate the call to t_unbind in tli_close; it sometimes hangs
**	    on closely sequenced disconnects, esp. with TLI_UNIX; the t_unbind
**	    should be unnecessary anyway since the subseqent t_close should
**	    release any transport endpoint associated with the fd. 
**	02-Mar-93 (brucek)
**	    Broke out tli_release from tli_close.
**	04-Mar-93 (edg)
**	    Removed detect logic.
**	10-mar-93 (mikem)
**	    Changed the trace level of a TLI_TRACE() statement in tli_error()
**	    from 0 to 1 so that tracing would be off by default.
**	12-mar-93 (peterk)
**	    put call to t_unbind back into tli_close
**	21-jun-93 (shelby)
**	    Added tdsu checking to tli_send().
**      21-june-93 (mikem)
**          Unregister fds in tli_unlisten(), because if clpoll is caused
**          after unlisten, we can get EBADF and much ensuing badness (AV's).
**	    Following example of change in sock_unlisten.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-Jul-93 (brucek)
**	    Created new function tli_reuse() to set REUSEADDR option;
**	    added call to tli_reuse() out of tli_listen().
**	18-mar-94 (rcs)
**	    Bug #59572
**	    Update tli_listen() to allocate opt and udata buffers for the use of
**	    t_listen, based on the values returned from t_open. If these buffers
**	    are not allocated, t_listen is specified to fail ( and does on
**	    a drs6000). Although many implementations currently do not try to
**	    return data if the buffer size is set to zero, this can
**	    change (and has) so the fix has been made generically.
**	    Updated tli_unlisten() to release new buffers.
**	20-Dec-94 (GordonW)
**		comment after a endif
**      09-feb-95 (chech02)
**          Added rs4_us5 for AIX 4.1.
**      05-jun-95 (canor01)
**          semaphore protect memory allocation functions
**	20-sep-1995 (sweeney)
**	    beefed up comments, tracing. Removed unused variable.
**	20-sep-1995 (sweeney)
**	    cater for t_info values other than -1 (i.e. don't try to allocate
**	    buffers of size -2)
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with su4_us5.
**              Reason, to port Solaris for Intel.
**	08-feb-1996 (sweeney)
**	    Comment TLIDEBUG token after # endif.
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**	15-apr-1999 (popri01)
**	    For Unixware 7 (usl_us5), the C run-time variables
**	    sys_errlist and t_errlist are not available in a dynamic 
**	    load environment. Use the strerror and t_strerror 
**	    functions instead.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	16-Jul-1999 (hweho01)
**	    Added support for AIX 64-bit (ris_u64) platform.
**	30-nov-98 (stephenb)
**	    Use strerror to compare error strings, direct comparison doesn't
**	    work under Solaris 7.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

#ifdef	TLIDEBUG
int	tli_debug;
#endif

#define LISTEN_Q	10


/*
** Support routines - not visible outside this file.
*/

/*
** Name: tli_error - format CL_ERR_DESC after TLI error
*/

/*
** Some human-readable symbols
*/

/*
** Returned from t_look()
*/

static struct {
	i4	mask;
	char	*name;
	char	*desc;
} tli_looks[] = {
	T_LISTEN,	"T_LISTEN",	"connection indication received",
	T_CONNECT,	"T_CONNECT",	"connect confirmation received",
	T_DATA,		"T_DATA",	"normal data received",
	T_EXDATA,	"T_EXDATA",	"expedited data received",
	T_DISCONNECT,	"T_DISCONNECT",	"disconnect received",
#if defined(T_ERROR)
	T_ERROR,	"T_ERROR",	"fatal error occurred",
#endif
	T_UDERR,	"T_UDERR",	"data gram error indication",
	T_ORDREL,	"T_ORDREL",	"orderly release indication",
	0x0000, "T_LOOK",	"event requires attention",	/* last! */
} ;

/*
** info.servtype should be 1, 2 or 3
*/
static char * t_servtype[] = 
	{
	"",
	"T_COTS",
	"T_COTS_ORD",
	"T_CLTS"
	};

/*
** Global TLI error number (t_errno)
*/
static char * t_errnos[] = 
	{
	"T_NULL",		/* 0 */
	"TBADADDR",	/* incorrect addr format     */
	"TBADOPT",	/* incorrect option format   */
	"TACCES",	/* incorrect permissions     */
	"TBADF",	/* illegal transport fd      */
	"TNOADDR",	/* couldn't allocate addr    */
	"TOUTSTATE",	/* out of state              */
	"TBADSEQ",	/* bad call sequence number  */
	"TSYSERR",	/* system error              */
	"TLOOK",	/* event requires attention  */
	"TBADDATA",	/* illegal amount of data    */
	"TBUFOVFLW",	/* buffer not large enough   */
	"TFLOW",	/* flow control              */
	"TNODATA",	/* no data                   */
	"TNODIS",	/* discon_ind not found on q */
	"TNOUDERR",	/* unitdata error not found  */
	"TBADFLAG",	/* bad flags                 */
	"TNOREL",	/* no ord rel found on q     */
	"TNOTSUPPORT",	/* primitive not supported   */
	"TSTATECHNG",	/* state is in process of changing */
	"TNOSTRUCTYPE",	/* unsupported struct-type requested */
	"TBADNAME",	/* invalid transport provider name */
	"TBADQLEN",	/* qlen is zero */
	"TADDRBUSY",     /* address in use */
	"UNKNOWN"       /* unknown error */
	};

# if defined(any_aix)

/*
** The following should be defined by the system. IBM doesn't bother.
*/
char * t_errlist[] = 
	{
	" 0",
	"incorrect addr format",
	"incorrect option format",
	"incorrect permissions",
	"illegal transport fd",
	"couldn't allocate addr",
	"out of state",
	"bad call sequence number",
	"system error",
	"event requires attention",
	"illegal amount of data",
	"buffer not large enough",
	"flow control",
	"no data",
	"discon_ind not found on q",
	"unitdata error not found",
	"bad flags",
	"no ord rel found on q",
	"primitive not supported",
	"state is in process of changing",
	"unsupported struct-type requested",
	"invalid transport provider name",
	"qlen is zero",
	"address in use",
	"unknown tli error"
	};
# endif /* my system doesn't define t_errlist */

VOID
tli_error( bsp, status, fd, er_name, caller )
BS_PARMS	*bsp;
STATUS		status;
i4		fd;
i4		er_name;
char		*caller;
{
	char	*s = bsp->syserr->moreinfo[0].data.string;
	int	tarray_size = sizeof(t_errnos)/sizeof(char *)  ;
	int	my_terrno = (t_errno < tarray_size ? t_errno : tarray_size - 1);

	bsp->status = status;

	TLITRACE(1)( "%s: tli_error %d on fd %d\n", caller, t_errno, fd );
	
	if( t_errno == TLOOK && fd >= 0 )
	{
		i4	i;
		i4	look = t_look( fd );

#ifdef TLIDEBUG
	TLITRACE(1)( "%s: tli_error look %x on fd %d\n", caller, look, fd );
#endif /* TLIDEBUG */

		if( look < 0 )
			look = 0;

		for( i = 0; tli_looks[ i ].mask; i++ )
		    if( look & tli_looks[ i ].mask )
			break;

		STprintf( s, "%s (%s)", 
			tli_looks[ i ].name,
			tli_looks[ i ].desc );

		SETCLERR( bsp->syserr, BS_SYSERR, 0 ); 
	}
	else if( t_errno == TSYSERR && fd >= 0)
        {
		STcopy( strerror(errno), s );
                SETCLERR( bsp->syserr, BS_SYSERR, er_name  );
        }
	else if( t_errno && fd >= 0)
	{
#ifdef usl_us5
		STprintf( s, "%s (%s)", t_errnos[my_terrno], t_strerror( my_terrno ) );
#else
		STprintf( s, "%s (%s)", t_errnos[my_terrno], t_errlist[ my_terrno ] );
#endif /* usl_us5 */
		SETCLERR( bsp->syserr, BS_SYSERR, 0 );
	}
	else
	{
		*s = '\0';
		SETCLERR( bsp->syserr, 0, 0 );
	}

	bsp->syserr->moreinfo[0].size = STlength( s );

#ifdef TLIDEBUG
	TLITRACE(1)( "%s: tli_error status %x errno %d t_errno %d msg '%s'\n", 
			caller, status, errno, t_errno, s );
#endif /* TLIDEBUG */
}


/*
** Name: tli_addr - strip /dev/name:: name from address
*/

VOID
tli_addr( defdev, buf, device, addr )
char	*defdev;
char	*buf;
char	*device;
char	**addr;
{
	char	*p;

	if( *buf != '/' )
	{
		STcopy( defdev, device );
		*addr = buf;
	}
	else if( ( p = STchr( buf, ':' ) ) && p[1] == ':' )
	{
		STncpy( device, buf, p-buf );
		device[ p-buf ] = EOS;
		*addr = p + 2;
	}
	else
	{	
		STcopy( buf, device );
		*addr = "";
	}
# ifdef TLIDEBUG
	TLITRACE(2)( "tli_addr '%s' is dev '%s' addr '%s'\n",
			buf, device, *addr );	
# endif /* TLIDEBUG */
}


/*
** Name: tli_reuse - set REUSEADDR option
*/
 
static VOID
tli_reuse( fd )
int   fd;
{
 
	struct t_optmgmt t_opt;
        int     status;
 
# if defined(sparc_sol) || defined(sui_us5)
 
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
 
        tcp_options.opthdr.level = SOL_SOCKET;
        tcp_options.opthdr.name = SO_REUSEADDR;
        tcp_options.opthdr.len = sizeof(tcp_options.optval.value);
        tcp_options.optval.value = 1;
 
        (void)t_optmgmt( fd, &t_opt, &t_opt );

# endif /* su4_us5 sui_us5 */

}

/*
** BS entry points
*/

/*
** Name: tli_listen - establish an endpoint for accepting connections
*/

VOID
tli_listen( bsp, addrbuf, addrlen, maxaddrlen, tcp_transport )
BS_PARMS	*bsp;
char		*addrbuf;
int		addrlen;
int		maxaddrlen;
bool		tcp_transport;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	struct t_bind *t_bind0 = &lbcb->bind;
	struct  t_info info;
	char    *p;
	int	fd;
	int	i;
	char	*fn="tli_listen";


# ifdef TLIDEBUG
	{
	char	*trace;
	NMgtAt( "II_TLI_TRACE",  &trace );
	if ( !( trace && *trace ) && PMget("!.tli_trace_level", &trace) != OK )
	    tli_debug = 0;
	else
	    tli_debug =  atoi(trace);
	}
# endif /* TLIDEBUG */

	if (!lbcb->q_max)   /* if not already set by caller */
	{
	   	if ( PMget("$.$.gca.tli.listen_qlen", &p) != OK ||
		     CVan( p, &lbcb->q_max ) != OK )
		{
	   	  	lbcb->q_max =  LISTEN_Q;
		}
	}

	/* create a listen endpoint */

	fd = t_open( lbcb->device, O_RDWR | OSI_NONBLOCK, &info );
	CLPOLL_FD( fd );
	(void)t_sync( fd );

	if( fd < 0 )
	{
		tli_error( bsp, BS_SOCK_ERR, fd, ER_open, fn );
		return;
	}
	/* bail out if we don't have user access to transport addresses */

	if( info.addr == -2)
	{
		tli_error(bsp, BS_BADADDR_ERR, fd, 0, fn);
		(void)t_close( fd );
		return;
	}

	/* extract information into lbcb */ 
	lbcb->tsdu = info.tsdu;
       	lbcb->servtype = info.servtype;
       	lbcb->maxaddr = info.addr == -1 ? maxaddrlen : info.addr;
       	lbcb->maxconnect = info.connect == -1 ? MAX_CONNECT_LEN : info.connect;
       	lbcb->maxopt = info.options == -1 ? MAX_OPT_LEN : info.options;

#ifdef TLIDEBUG
	TLITRACE(2)("%s: lbcb->servtype = %s\n",
				fn, t_servtype[lbcb->servtype]);
	TLITRACE(3)("info.addr = %d, using %d\n", info.addr, maxaddrlen );
	TLITRACE(3)("info.connect = %d, using %d\n", info.connect, MAX_CONNECT_LEN );
	TLITRACE(3)("info.options = %d, using %d\n", info.options, MAX_OPT_LEN );
#endif

	/* make address reuseable (only if transport is TCP) */

	if( tcp_transport )
	    tli_reuse( fd );

	/* bind it */
	t_bind0->qlen = lbcb->q_max;
	t_bind0->addr.buf = addrbuf;
	t_bind0->addr.len = addrlen;
	t_bind0->addr.maxlen = lbcb->maxaddr ;

	if( t_bind( fd, t_bind0, t_bind0 ) < 0 )
	{
		tli_error( bsp, BS_BIND_ERR, fd, ER_bind, fn );
		(void)t_close( fd );
		lbcb->fd = -1 ;
		return;
	}
#ifdef TLIDEBUG
	TLITRACE(2)("%s: bind on fd %d dev %s succeeded\n", 
			fn, fd, lbcb->device);
#endif

	/* Save bound address into lbcb->addrbuf */
	lbcb->addrlen =  t_bind0->addr.len;
	lbcb->addrbuf = (char *)MEreqmem( 0, lbcb->addrlen,
			 TRUE, (STATUS *) 0 );
	if( !lbcb->addrbuf ) 
	{
		tli_error( bsp, BS_INTERNAL_ERR, -1, ER_t_alloc, fn );
		(void)t_close( fd );
		return;
	}
	MEcopy(t_bind0->addr.buf, lbcb->addrlen, lbcb->addrbuf);

	/* Initialize the Q */

	lbcb->q_count = 0;
	lbcb->q_array = (struct t_call  **)MEreqmem(0, 
            lbcb->q_max * sizeof(struct t_call *),TRUE,(STATUS *)NULL);

	if( !lbcb->q_array ) 
	{
		tli_error( bsp, BS_INTERNAL_ERR, -1, ER_t_alloc, fn );
		(void)t_close( fd );
		return;
	}
#ifdef TLIDEBUG
	TLITRACE(2)("%s: allocated Q array at %x\n", 
			fn, lbcb->q_array);
#endif

	/* loop thru allocating call structures */

	for( i = 0; i < lbcb->q_max; i++ )
	{
		struct t_call *calls;

		calls = (struct t_call *)MEreqmem( 0, sizeof(struct t_call),
		    TRUE, (STATUS *)NULL );

		if( !calls )
		{
			tli_error( bsp, BS_INTERNAL_ERR, fd, ER_t_alloc, fn );
			(void)t_close( fd );
			lbcb->fd = -1 ;
			return;
		}
		calls->addr.maxlen = lbcb->maxaddr ;
		calls->addr.buf = (char *)MEreqmem( 0, calls->addr.maxlen, 
		    TRUE, (STATUS *) 0 );

		/* We must allocate buffers for opt and connect too even
		   though we do not use the data, because some t_listen
		   implementations fail otherwise */

		/* XXX sweeney -- do not allocate if options/connect 0 or -ve */

		if ((calls->opt.maxlen = lbcb->maxopt) > 0 )
		{
			calls->opt.buf = (char *)MEreqmem( 0, 
				calls->opt.maxlen, TRUE, (STATUS *) 0 );
		}

		if ((calls->udata.maxlen = lbcb->maxconnect) > 0 )
		{
			calls->udata.buf = (char *)MEreqmem( 0,
				calls->udata.maxlen, TRUE, (STATUS *) 0 );
		}

		if ( !calls->addr.buf )
		{
			tli_error( bsp, BS_INTERNAL_ERR, fd, ER_t_alloc, fn );
			(void)t_close( fd );
			return;
		}

		lbcb->q_array[ i ] = calls;
#ifdef TLIDEBUG
	TLITRACE(2)("%s: allocated Q array[%d]\n", 
			fn, i);
#endif
	}

	/* return results OK */

	lbcb->fd = fd;

	bsp->status = OK;
}

/*
** Name: tli_unlisten - stop accepting incoming connections
**
** History:
**      21-june-93 (mikem)
**          Unregister fds here too, 'cause if clpoll is caused
**          after unlisten, we can get EBADF and much ensuing badness.
**	    Following example of change in sock_unlisten.
*/

VOID
tli_unlisten( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	i4	i;

	/* reject any queued requests; free the call structures */
		
	for( i = 0; i < lbcb->q_max; i++ )
	{
		struct	t_call *calls = lbcb->q_array[ i ]; 

		if( i < lbcb->q_count )
		    (VOID)t_snddis( lbcb->fd,  calls ); 

                MEfree( (PTR)calls->addr.buf );

                if ( calls->opt.buf )
			MEfree( (PTR)calls->opt.buf );

                if ( calls->udata.buf )
			MEfree( (PTR)calls->udata.buf );

                MEfree( (PTR)calls );
	}

	MEfree( (PTR)lbcb->q_array );

	lbcb->q_count = 0;
	lbcb->q_array = 0;

	/* Close the listen fd */

	if( lbcb->fd >= 0 )
	{
	    /* unregister file descriptors */
	    (void)iiCLfdreg( lbcb->fd, FD_READ, (VOID (*))0, (PTR)0, -1 );
	    (void)iiCLfdreg( lbcb->fd, FD_WRITE, (VOID (*))0, (PTR)0, -1 );

	    (VOID)t_unbind( lbcb->fd );
	    (VOID)t_close( lbcb->fd );
	}

	bsp->status = OK;
}
	
/*
** Name: tli_accept - accept a single incoming connection
*/

VOID
tli_accept( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;
	struct	t_bind *t_req = 0 ;
	struct  t_info info;
	int	fd;
	char	*fn="tli_accept";

	/* clear fd for sanity */

	bcb->fd = -1;
	bsp->status = OK;

	/* create an endpoint */

	fd = t_open( lbcb->device, O_RDWR | OSI_NONBLOCK, &info );
	CLPOLL_FD( fd );
	(void)t_sync( fd );

	if( fd < 0)
	{
		tli_error( bsp, BS_SOCK_ERR, fd, ER_open, fn );
		return;
	}

	if (lbcb->flags & NO_ANON_TBIND)
	{
	    /* cannot do anonyous t_bind */
	    t_req = &lbcb->bind ;
	    t_req->addr.buf = lbcb->addrbuf;
	    t_req->addr.len = lbcb->addrlen;
	    t_req->addr.maxlen = lbcb->maxaddr;
	    t_req->qlen  = 0;
	}

	if( t_bind( fd, t_req, t_req ) < 0 )
	{
		tli_error( bsp, BS_BIND_ERR, fd, ER_bind, fn );
		(void)t_close(fd);
		return;
	}

#ifdef TLIDEBUG
	TLITRACE(2)("%s: bind to %s on fd %d dev %s succeeded\n", 
			fn, "anon", fd, lbcb->device);
#endif

	/* 
	** Listen and accept the connection.
	** This logic follows the TLI spec: all connection indications
	** must be picked up with t_listen before accepting or rejecting
	** any of the connections.  We rely on t_accept() returning TLOOK
	** if another connection indication is outstanding.
	*/ 

	for(;;)
	{
	    struct t_call **calls = lbcb->q_array + lbcb->q_count;

# ifdef TLIDEBUG
	    TLITRACE(2)( "tli_accept count %d\n", lbcb->q_count );
# endif /* TLIDEBUG */

	    if( lbcb->q_count )
	    {
		if( t_accept( lbcb->fd, fd, calls[ -1 ] ) >= 0 )
		{
		    /* return OK */

		    bcb->fd = fd;
		    bsp->status = OK;
		    lbcb->q_count--;
		    return;
		}

		switch( t_errno == TLOOK ? t_look( lbcb->fd ) : 0 )
		{
		default:
		    tli_error(bsp, BS_ACCEPT_ERR, lbcb->fd, ER_t_accept, fn);
		    (void)t_close( fd );
		    return;

		case T_DISCONNECT:
		    (void)t_rcvdis( lbcb->fd, (struct t_discon *)0 );
		    continue; /* for */

		case T_LISTEN:
		    break; /* switch */
		}
	    }

	    if( lbcb->q_count == lbcb->q_max )
	    {
		/* 
		** XXX no fair: more T_LISTEN's than q_max 
		** Return error and waste them all.
		*/

		lbcb->q_count = 0;
		bsp->status = BS_LISTEN_ERR;
		SETCLERR( bsp->syserr, 0, 0 );
		(void)t_close(fd);
		return;
	    }

	    if( t_listen( lbcb->fd, calls[ 0 ] ) < 0 && t_errno !=  TBUFOVFLW)
	    {
		tli_error( bsp, BS_LISTEN_ERR, lbcb->fd, ER_listen,fn);
		(void)t_close(fd);
		return;
	    }

	    lbcb->q_count++;
	}
}

/*
** Name: tli_request - make an outgoing connection
*/

VOID
tli_request( bsp, device, addrbuf, addrlen, laddrbuf, laddrlen, maxaddrlen)
BS_PARMS	*bsp;
char		*device;
char		*addrbuf;        /* remote address */
int		addrlen;         /* remote addrlen */
char            *laddrbuf;       /* local bind address */
int             laddrlen;        /* local bind addrlen */
int		maxaddrlen;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;
	struct	t_call *t_call;
	struct	t_bind *t_req = 0 ;
	struct  t_info info;
	int	fd;
	char	*fn="tli_request";


	/* create an endpoint */

	fd = t_open( device, O_RDWR | OSI_NONBLOCK, &info );
	CLPOLL_FD( fd );
	(void)t_sync( fd );

	if( fd < 0 )
	{
		tli_error( bsp, BS_SOCK_ERR, fd, ER_open, fn );
		return;
	}

	if (!lbcb->servtype)
	{
	    /* extract information into lbcb - we're a front-end  */ 
	    lbcb->tsdu = info.tsdu;
	    lbcb->servtype = info.servtype;
	    lbcb->maxaddr = info.addr == -1 ? maxaddrlen : info.addr;
	}

	if (laddrlen)
	{
	    /* 
	    ** If local address is requested explicitly, bind to it
	    */
	    t_req = &lbcb->bind ;
	    t_req->addr.buf = laddrbuf;
	    t_req->addr.len = laddrlen;
	    t_req->addr.maxlen = maxaddrlen;
	    t_req->qlen  = 0;
	}
					  
	/* bind */

	if( t_bind( fd, t_req, t_req ) < 0 )
	{
		tli_error( bsp, BS_BIND_ERR, fd, ER_bind, fn );
		(void)t_close(fd);
		return;
	}
#ifdef TLIDEBUG
	TLITRACE(2)("%s: bind on fd %d dev %s succeeded\n", fn, fd, device);
#endif

	t_call = (struct t_call *)MEreqmem( 0, sizeof(struct t_call),
		    TRUE, (STATUS *)NULL );

	if( !t_call )
	{
		tli_error( bsp, BS_INTERNAL_ERR, fd, ER_t_alloc, fn );
		(void)t_close( fd );
		return;
	}

	/* try to connect */

	t_call->addr.buf = addrbuf;
	t_call->addr.len = addrlen;
	t_call->addr.maxlen = maxaddrlen;

# ifdef TLIDEBUG
	TLITRACE(1)("tli_request: connecting\n" );
# endif 

	if( t_connect( fd, t_call, t_call ) < 0 && t_errno != TNODATA )
	{
		tli_error( bsp, BS_CONNECT_ERR, fd, ER_t_connect, fn );
		(void)t_close(fd);
		return;
	}
 
# ifdef TLIDEBUG
	TLITRACE(1)("tli_request on fd%d complete, returning to caller\n" ,fd);
# endif

	MEfree((PTR)t_call); 

	/* return OK */

	bcb->fd = fd;
	bsp->status = OK;
}


/*
** Name: tli_connect - complete an outgoing connection
*/

VOID
tli_connect( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	char	*fn="tli_connect";

        /*
        ** Check if connect succeeded.
        */

        if( t_rcvconnect( bcb->fd, NULL ) < 0 )
        {

# ifdef TLIDEBUG
	TLITRACE(1)("tli_connect fail: t_errno is %s (%s)\n"
		,t_errnos[t_errno]
# ifdef usl_us5
		,t_strerror(t_errno));
# else
		,t_errlist[t_errno]);
# endif /* usl_us5 */
# endif

                tli_error( bsp, BS_CONNECT_ERR, bcb->fd, 0, fn );
                return;
        }

        /* return OK */

        bsp->status = OK;
}

/*
** Name: tli_send - send data down the stream
*/

VOID
tli_send( bsp )
BS_PARMS	*bsp;
{
	LBCB    *lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;
	int     len = bsp->len;
	int	n;
	char	*fn="tli_send";

	if (bcb->fd < 0)
	    goto done;

	/*
	** check if length is greater than TSDU
	*/
	if ((lbcb->tsdu > 0) && (len > lbcb->tsdu))
		len = lbcb->tsdu;

	/* short sends are OK */

	if( ( n = t_snd( bcb->fd, bsp->buf, len, NULL ) ) < 0 )
	{
		if( t_errno != TFLOW )
		{
			tli_error( bsp, BS_WRITE_ERR, bcb->fd, ER_write, fn );
			return;
		}
		n = 0;
	}
	else if( n == bsp->len )
	{
		bcb->optim |= BS_SKIP_W_SELECT;
	}

	bsp->len -= n;
	bsp->buf += n;
done:
	bsp->status = OK;
}

/*
** Name: tli_receive - read data from a stream
*/

VOID
tli_receive( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;
	int	n;
	int	flags;
	char	*fn="tli_receive";

# ifdef TLIDEBUG
	TLITRACE(3)("%s on fd%d - calling t_rcv for %d bytes\n", 
			fn, bcb->fd, bsp->len );
# endif

	if (bcb->fd < 0)
	    goto done;

	/* short reads are OK - EOF is not */

	/* Touch buffer to fault it in for system call - MCT requirement */
	bsp->buf[bsp->len-1] = '\0';

	if( ( n = t_rcv( bcb->fd, bsp->buf, bsp->len, &flags ) ) <= 0 )
	{
		if( !n || t_errno != TNODATA )
		{
			tli_error( bsp, BS_READ_ERR, bcb->fd, ER_read, fn );
			return;
		}
		n = 0;
	}
# ifdef TLIDEBUG
	TLITRACE(3)("%s on fd%d - received %d bytes, T_MORE is %s\n", 
			fn, bcb->fd, n, flags & T_MORE ? "set" : "clear" );
# endif

	if (lbcb->tsdu) /* look for T_MORE, else complete if something read */
	{
        	if( flags & T_MORE )
                	bcb->optim |= BS_SKIP_R_SELECT;
		else
			if (n)
			    bsp->len = n ;
	}

        bcb->optim |= BS_SKIP_W_SELECT;

	bsp->len -= n;
	bsp->buf += n;
# ifdef TLIDEBUG
	TLITRACE(3)("%s on fd%d - %d bytes remaining\n", fn, bcb->fd, bsp->len);
# endif

done:
	bsp->status = OK;
}

/*
** Name: tli_release - send an orderly release indication
*/

VOID
tli_release( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;
	char	*fn="tli_release";

	if( bcb->fd < 0 )
	{
	    bcb->fd = -1;
	    goto done;
	}

	/*
	** Send orderly release indication if supported.
	*/

	if( lbcb->servtype & T_COTS_ORD )
	{    
	    int status = t_sndrel( bcb->fd );

#ifdef TLIDEBUG
	    if( status )
		TLITRACE(2)("%s: sndrel on fd %d failed t_errno %d\n", 
		    fn, bcb->fd, t_errno);
#endif
	}

done:
	bsp->status  = OK;
}


/*
** Name: tli_close - close a single connection
*/

VOID
tli_close( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;
	int	flags = 0;
	char	*fn="tli_close";
	struct t_discon discon[1];

	if( bcb->fd < 0 )
	    goto done;

	/*
	** Receive orderly release indication if available.
	*/

	if( lbcb->servtype & T_COTS_ORD )
	{    
	    char	buf[32];

	    t_rcv( bcb->fd, buf, sizeof(buf), &flags );

	    if( t_errno == TNODATA )
	    {
		bsp->status = BS_INCOMPLETE;
		return;
	    }

	    t_rcvrel( bcb->fd );
	}

	/*
	** Set fd to blocking
	*/

	flags = fcntl( bcb->fd, F_GETFL, (OSI_NONBLOCK|O_SYNC) );
	if( flags > 0 )
	{
	    flags &= ~OSI_NONBLOCK;
	    (void)fcntl( bcb->fd, F_SETFL, flags );
	}
	
	(void)t_unbind( bcb->fd );

	(void)iiCLfdreg( bcb->fd, FD_READ, (VOID (*))0, (PTR)0, -1 );
	(void)iiCLfdreg( bcb->fd, FD_WRITE, (VOID (*))0, (PTR)0, -1 );

        if( t_close( bcb->fd ) < 0 )
        {
	    tli_error( bsp, BS_CLOSE_ERR, bcb->fd, 0, fn );
        }

done:
	bcb->fd = -1;
	bsp->status  = OK;
}

/*
** Name: tli_regfd - register for blocking operation
*/

bool
tli_regfd( bsp )
BS_PARMS	*bsp;
{
	int	fd;
	int	op;
	BCB	*bcb = (BCB *)bsp->bcb;
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	static char *regops[]={
			    "BS_POLL_ACCEPT" , 
			    "BS_POLL_CONNECT", 
			    "BS_POLL_SEND", 
			    "BS_POLL_RECEIVE",
			    "BS_POLL_SNDREL", 
			    "BS_POLL_RCVREL" 
			  };

	switch( bsp->regop )
	{
	case BS_POLL_ACCEPT:
		if( lbcb->q_count )
			return FALSE;
		fd = lbcb->fd; op = FD_READ; break;

	case BS_POLL_SEND:
		if( bcb->optim & BS_SKIP_W_SELECT )
		{
			bcb->optim &= ~BS_SKIP_W_SELECT;
			return FALSE;
		}
		fd = bcb->fd; op = FD_WRITE; break;

	case BS_POLL_CONNECT:
		fd = bcb->fd; op = FD_READ; break;

	case BS_POLL_RECEIVE:
		fd = bcb->fd; op = FD_READ; break;

	case BS_POLL_SNDREL:
		fd = bcb->fd; op = FD_WRITE; break;

	case BS_POLL_RCVREL:
		fd = bcb->fd; op = FD_READ; break;

	default:
		return FALSE;
	}

# ifdef TLIDEBUG
	TLITRACE(2)("tli_regfd: fd=%d, regop=%s\n", fd, regops[bsp->regop] );
# endif

	if( fd < 0 )
		return FALSE;

	/* Pass control to CLpoll */

	iiCLfdreg( fd, op, bsp->func, bsp->closure, bsp->timeout );

	return TRUE;
}

VOID
tli_sync( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	char	*fn="tli_sync";

	bsp->status = OK;

	if( t_sync( bcb->fd ) < 0 )
		tli_error( bsp, BS_INTERNAL_ERR, bcb->fd, 0, fn );
}

# endif /* xCL_TLI_TCP_EXISTS */

