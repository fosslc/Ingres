/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<clpoll.h>
# include	<cm.h>
# include	<st.h>
# include	<me.h>
# include	<er.h>

# include	<errno.h>
# include	<bsi.h>

# ifdef xCL_TLI_SPX_EXISTS

# include	<systypes.h>
# include	<clsocket.h>
# include	<sys/stream.h>
# include	<tiuser.h>
# include	<sys/tihdr.h>

# ifdef xCL_006_FCNTL_H_EXISTS
# include	<fcntl.h>
# endif

/* 
** NB: In their infinite portableness, header files for Netware Portable 
** Transports are never in the same place, so bstlispx.h is a conglomeration 
** of the following headers:
**	common.h
**	portable.h
**	ipx_app.h
**	spx_app.h
*/

# ifdef usl_us5
/* The following typdefs are not used - see history */
typedef CL_ERR_DESC queue_t; 
typedef CL_ERR_DESC mblk_t; 
# endif /* usl_us5 */

# include	"bstlispx.h"
# include	"bstliio.h"


/*
** Name: bstlispx.c - BS interface to netware SPX via TLI.
**
** Description:
**	This file provides access to Novell Netware portable transport
**	SPX via TLI sockets, using the generic BS interface defined in bsi.h.  
**	See that file for a description of the interface.
**
**	The following functions are internal to this file:
**
**		spx_addr - turn string version of SPX address into ipxAddr_t
**		spx_cvx() - convert hex into hi-low byte array
**
**	The following functions are exported as part of the BS interface:
**
**		spx_listen - establish an endpoint for accepting connections
**		spx_request - make an outgoing connection
**		spx_port() - turn a installation code into a spx socket number
**
** History:
**	25-Aug-92 (seiwald)
**	    Written.
**	21-Sep-92 (seiwald)
**	    Use CM and ST instead of libc calls.
**	02-Mar-93 (brucek)
**	    Added tli_release to BS_DRIVER.
**	04-Mar-93 (edg)
**	    Remove detect logic.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-Jul-93 (brucek)
**	    Added tcp_transport arg on call to tli_listen.
**	09-Sep-97 (popri01)
**	    In Unixware, the compiler chokes on the connectionEntry typedef 
**	    in bstlispx.h, which references other O/S typedef's. Unixware does
**	    not need the connectionEntry typedef and bstlispx.h contains
**	    proprietary Novell code. So rather than modify bstlispx.h
**	    (as was done in 1.2/01), define the necessary dummy typedef's for 
**	    Unixware in order to satisy the compiler.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Dec-2001 (bonro01)
**	    Move incorrectly propagated change history that was causing
**	    compile errors for usl_us5.
**      19-nov-2002 (loera01)
**          Re-introduced spx_accept() as a wrapper for tli_accept().
*/

#define TLI_SPX "/dev/nspx"

static VOID spx_cvx();

/*
** Name: spx_addr - turn string version of SPX address into ipxAddr_t
**
** Description:
**
**	Spx/ipx addresses, in their numeric form, look like this:
**
**		net.node::sock
**
**	where
**		net 	xxxxxxxx	8 hex digits
**		node	xxxxxxxxxxxx	12 hex digits
**		sock	xxxx		4 hex digits
**
**	Net is some sort of Netware partitioning of the world, because
**	node is (for ethernet interfaces) the actual ethernet address
**	of the card.  Sock is the local address, either in the range
**	0x4000 to 0x4ffff for "dynamic sockets" or in the range 0x8000
**	to 0x8ffff for "static sockets" - those assigned by Novell.
**	Using 0 as a socket number on a listen causes spx to generate
**	a dynamic socket for use.
*/

static STATUS
spx_addr( buf, isactive, s )
char		*buf;
bool		isactive;
ipxAddr_t	*s;
{
	i4 	l;
	char	*p, *q;

	/* set defaults */

	MEfill( sizeof( *s ), '\0', (char *)s );

	/* parse address - look for a ":" */

	for( p = buf; *p; p++ )
	    if( p[0] == ':' )
		break;

	if( *p )
	{
		/* Now pick apart net.node */

		for( q = buf; q < p; q++ )
		    if( q[0] == '.' )
			break;

		/* net */

		if( q < p )
		    spx_cvx( buf, q - buf, s->net, 4 );

		/* node */

		spx_cvx( q + 1, p - q - 1, s->node, 6 );
	}

	/* Get to sock */

	if( !*p )
	    p = buf;
	else if( p[1] == ':' )
	    p = p + 2;
	else
	    p = p + 1;

	if( !*p && isactive )
	    return BS_NOPORT_ERR;

	spx_cvx( p, STlength( p ), s->sock, 2 );

	return OK;
}

/*
** Name: spx_cvx() - convert hex into hi-low byte array
**
** Inputs:
**	s	string of hex characters [0-9,A-F]
**	len	length of s
**	ll	long long for output (u_char array)
**	l_ll	actual length of l_ll
*/

static VOID
spx_cvx( s, len, ll, l_ll )
char	*s;
i4	len;
u_char	*ll;
i4	l_ll;
{
	char *hex = "0123456789ABCDEF";
	i4 j = l_ll * 2 - len;
	i4 i;

	for( i = 0; i < l_ll; i++ )
	    ll[ i ] = 0;

	if( j < 0 )
	    return; /* input too long ! */

	for( ; len--; j++, s++ )
	{
	    char up, *val;

	    CMtoupper( s, &up );

	    val = STchr( hex, up );

	    ll[ j / 2 ] |= ( val ? val - hex : 0 ) * ( j % 2 ? 0x01 : 0x10 );
	}
}
	    

/*
** BS entry points
*/

/*
** Name: spx_listen - establish an endpoint for accepting connections
*/

static VOID
spx_listen( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	char	*addr;
	u_short	portnum;
	ipxAddr_t s;

	/* clear fd for sanity */

	lbcb->fd = -1;

	/* get device */

	tli_addr( TLI_SPX, bsp->buf, lbcb->device, &addr );

	/* get listen port, if specified */

	if( ( bsp->status = spx_addr( addr, FALSE, &s ) ) != OK )
	{
		SETCLERR( bsp->syserr, 0, 0 );
		return;
	}

	/* do tli listen */

	tli_listen( bsp, (char *)&s, sizeof( s ), sizeof( s ), FALSE );

	if( bsp->status != OK )
	    return;

	/* get name for output */

	portnum = ( s.sock[0] << 8 ) + s.sock[1];

	if( STcompare( lbcb->device, TLI_SPX ) )
	    STprintf( lbcb->port, "%s::%x", lbcb->device, portnum );
	else
	    STprintf( lbcb->port, "%x", portnum );

	bsp->buf = lbcb->port;
}

/*
** Name: spx_request - make an outgoing connection
*/

static VOID
spx_request( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	char	device[ MAXDEVNAME ];
	char	*addr;
	ipxAddr_t s;

	/* clear fd for sanity */

	bcb->fd = -1;

	/* get device */

	tli_addr( TLI_SPX, bsp->buf, device, &addr );

	/* translate address */

	if( ( bsp->status = spx_addr( addr, TRUE, &s ) ) != OK )
    	{
		bsp->status = BS_INTERNAL_ERR;
		return;
	}

	/* do tli connect request */

	tli_request( bsp, device, (char *)&s, sizeof( s ), 
				(char *)0, 0, sizeof( s ) );
}


/*
** Name: spx_port() - turn a installation code into a spx socket number
**
** Description:
**	This routines provides mapping from II_INSTALLATION into a
**	unique spx sockeT number for the installation.  
**
**	If pin is of the form:
**		XY
**	or	XY#
**	where
**		X is [a-zA-Z]
**		Y is [a-zA-Z0-9]
**	and	# is [0-1]
**
**	then pout is the string representation of the hex number:
**
**        15  14  13  12  11  10  09  08  07  06  05  04  03  02  01  00
**       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
**       ! 0 ! 1 ! 0 ! 0 !  low 5 bits of X  !   low 6 bits of Y     ! # !
**       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
**
**	This will be in the range 0x4000 - 0x4ffff.
**
**	If # is not provided, then the subport parameter is used.  This
**	allows programmatic permutations of the XY form of the port name.
**	If subport > 0 and port is not the XY form, fail.
**
**	If pin is not of the form XY or XY#; copy pin to pout.
**
** Inputs:
**	portin - source port name
**	subport - added as offset to port number
**	portout - resulting port name 
**
** History
**	16-sep-92 (seiwald)
**	    Modified from BS_tcp_port.
*/

static STATUS
spx_port( pin, subport, pout )
char	*pin;
i4	subport;
char	*pout;
{
	if( CMalpha( &pin[0] ) && ( CMalpha( &pin[1] ) | CMdigit( &pin[1] ) ) )
	{
		long	portid;
		char	p[ 3 ];

		if( subport > 1 || subport && pin[2] )
			return FAIL;

		CMtoupper( &pin[0], &p[0] );
		CMtoupper( &pin[1], &p[1] );
		p[2] = CMdigit( &pin[2] ) ? pin[2] : subport;

		portid = 1 << 14 
			| ( p[0] & 0x1f ) << 7
			| ( p[1] & 0x3f ) << 1
			| ( p[2] & 0x01 );

		CVlx( portid, pout );

		return OK;
	} else {
		if( subport )
			return FAIL;

		STcopy( pin, pout );

		return OK;
	}
}

/*
** Name: spx_accept - accept a single connect request.
**
** Inputs:
**      bsp             parameters for BS interface. 
**
** Outputs:
**	bsp->is_remote gets set to TRUE.
**
** Returns:
**	void.
**
** Side effects:
**	bsp gets modified by tli_accept().
**
** History:
**      19-nov-2002 (loera01)
**          Created.
*/
static VOID
spx_accept( bsp )
BS_PARMS	*bsp;
{
    bsp->is_remote = TRUE;
    tli_accept(bsp);
}

/*
** Exported driver specification.
*/

BS_DRIVER BS_tlispx = {
	sizeof( BCB ),
	sizeof( LBCB ),
	spx_listen,
	tli_unlisten,
	spx_accept,
	spx_request,
	tli_connect,
	tli_send,
	tli_receive,
	tli_release,
	tli_close,
	tli_regfd,
	tli_sync,	/* save */
	tli_sync,	/* restore */
	spx_port,	
} ;

# endif /* xCL_TLI_SPX_EXISTS */
