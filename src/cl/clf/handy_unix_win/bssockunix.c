/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<st.h>
# include	<me.h>
# include	<pc.h>

# include	<systypes.h>
# include	<clsocket.h>

# if defined(xCL_066_UNIX_DOMAIN_SOCKETS)

# include	<bsi.h>
# include	"bssockio.h"

# include	<errno.h>


/*
** Name: bssockunix.c - BS interface to BSD unix domain sockets.
**
** Description:
**	This file provides access to BSD unix domain sockets, using the 
**	generic BS interface defined in bsi.h.  See that file for a 
**	description of the interface.
**
**	The following functions are internal to this file:
**
**		unix_addr - turn a string into unix domain socket path
**
**	The following functions are exported as part of the BS interface:
**
**		unix_listen - establish an endpoint for accepting connections
**		unix_unlisten - stop accepting incoming connections
**		unix_accept - accept a single incoming connection
**		unix_request - make an outgoing connection
**		unix_connect - complete an outgoing connection
**
**	This module also calls and exports the routines found in bssockio.c.
**
** History:
**	4-Jul-90 (seiwald)
**	    Made from bssockunix.c
**	5-Jul-90 (seiwald)
**	    Use CLPOLL_RESVFD to save fds for SYSV stdio.
**	29-Aug-90 (gautam)
**	    Change socket permissions to 666 after  creation
**	31-Aug-90 (seiwald)
**	    Changed to use sock_error().
**	6-Sep-90 (seiwald)
**	    Moved generic routines to bssockio.c.
**	12-Apr-91 (seiwald)
**	    Modified unix_detect to attempt to succeed at connecting rather
**	    than to fail at binding.  If you bind to an unused UNIX socket,
**	    you create the socket, leaving the husk lying around.
**	    Make the default name /tmp/ii.pid, not /tmp/UNIX.pid.
**	13-Mar-92 (gautam)
**	    1. Changed unix_detect() to first try and kill() server_pid
**	       to avoid GCA/session_startup  overhead, assuming address is of
**		   the form "some_directory/ii.pid" - bug # 38056
**          2. Do not chmod on sqs_ptx, since that changes the file mode from
**             socket to regular-file.
**	08-apr-92 (sbull/pholman)
**	    The dr6_uv1 (V1+ OS) and dr6_us5 have the same problem as sqs_ptx
**	   - don't chmod socket.
**	29-apr-92 (pearl)
**	    Don't chmod socket for nc5_us5.
**	9-nov-92 (gautam)
**	    Fix from Kola for checking length of buffer in unix_detect().
**	11-jan-93 (deastman)
**	     Go with the prevailing wind regarding chmod. 
**      27-jan-93 (pghosh)
**          Modified nc5_us5 to nc4_us5. Till date nc5_us5 & nc4_us5 were
**          being used for the same port. This is to clean up the mess so
**          that later on no confusion is created.
**      02-Mar-93 (brucek)
**          Added NULL function pointer to BS_DRIVER for orderly release.
**	04-Mar-93 (edg)
**	     Remove detect logic.
**      16-mar-93 (smc)
**           Commented out text after #endif.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	1-jun-94 (robf)
**  	    added ext_info request for BS driver
**	13-mar-95 (smiba01)
**	    Added dr6_ues options in line with dr6_us5.
**      10-nov-1995 (sweeney)
**          Rename *sock_ routines -- sock_connect was clashing with a
**          3rd party comms library.
**      08-dec-1995 (morayf)
**	    Added SNI support for RMx00 (rmx_us5) Pyramid clone.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_ues, dr6_uv1.
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      19-nov-2002 (loera01)
**          Set is_remote to FALSE in BS_PARMS.
**	15-Jun-2004 (schka24)
**	    Be safe handling environment variables.
**	4-Dec-2008 (kschendel)
**	    Declare errno correctly (i.e. via errno.h).
*/

extern  VOID	iisock_send();
extern  VOID	iisock_receive();
extern  VOID	iisock_close();
extern  bool	iisock_regfd();
extern	VOID	iisock_ok();
extern  STATUS  iisock_ext_info();


/*
** Support routines - not visible outside this file.
*/

/*
** Name: unix_addr - turn a string into unix domain socket path
**
** Description:
**	This routine takes a listen address and turns it into a unix
**	domain socket pathname.  The inputs are these:
**
**		INPUT			OUTPUT
**		-------			------
**		/path/file		/path/file
**		/path/			/path/ii.pid
**		(null)			/tmp/ii.pid
**		ii.pid			II_GC_PORT/ii.pid
*/

static STATUS
unix_addr( buf, s, pathlen )
char	*buf;
struct sockaddr_un *s;
i4	pathlen;
{
	PID		pid;
	i4		len;
	char		*path;

	/* Keep it in the family */

	s->sun_family = AF_UNIX;

	if( !( len = STlength( buf ) ) || buf[ len - 1 ] == '/' )
	{
	    /* if given no hard path, use /tmp/ii.pid */
	    /* if given /dir../, use /dir../ii.pid */

	    PCpid( &pid );

	    STprintf( s->sun_path, "%sii.%d", len ? buf : "/tmp/", pid );
	}
	else if(!STncasecmp(buf, "ii.", 3))
	{
		/*
		** Relative file name, so make full path
		*/
		NMgtAt("II_GC_PORT", &path);
		TRdisplay("PATH is '%s'\n",path);
		if (path == NULL || *path == EOS)
		    path = "/tmp";
		STlpolycat(3, pathlen, path, "/", buf, s->sun_path);
	}
	else
	{
	    /* just use the path given */

	    STcopy( buf, s->sun_path );
	}

# ifdef UNIXDEBUG
	TRdisplay( "unix_addr '%s' is '%s'\n", buf, s->sun_path );
# endif /* UNIXDEBUG */
	return OK;
}


/*
** BS entry points
*/

/*
** Name: unix_listen - establish an endpoint for accepting connections
*/

static VOID
unix_listen( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	struct	sockaddr_un	s[1];
	int	size = sizeof( *s );
	int	fd;

	/* clear fd for sanity */

	lbcb->fd = -1;

	/* get listen port, if specified */

	if( ( bsp->status = unix_addr( bsp->buf, s, sizeof(s->sun_path)-1 ) ) != OK )
	{
		SETCLERR( bsp->syserr, 0, 0 );
		return;
	}

	/* Remove old entry */

	(void)unlink( s->sun_path );

	/* do socket listen */

	iisock_listen( bsp, s, size );

	if( bsp->status != OK )
		return;

	/* format name for output */

	STprintf( lbcb->port, "%s", s->sun_path );
	bsp->buf = lbcb->port;

  	/* change permissions on it */

#if !(defined(sqs_ptx) || defined(dr6_us5) || \
      defined(nc4_us5) || defined(pym_us5) || defined(rmx_us5) || \
      defined(rux_us5))
  	(VOID)chmod(s->sun_path, 0666);
#endif
}

/*
** Name: unix_unlisten - stop accepting incoming connections
*/

static VOID
unix_unlisten( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;

	iisock_unlisten( bsp );

	if( bsp->status != OK )
		return;

	(void)unlink( lbcb->port );
}
	
/*
** Name: unix_accept - accept a single incoming connection
*/

static VOID
unix_accept( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	struct sockaddr_un	s[1];
	int	size = sizeof( *s );

	/* clear fd for sanity */

	bcb->fd = -1;
        bsp->is_remote = FALSE;

	/* do socket accept */

	iisock_accept( bsp, s, size );
}

/*
** Name: unix_request - make an outgoing connection
*/
static VOID
unix_request( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	struct	sockaddr_un	s[1];
	i4	size = sizeof( *s );

	/* clear fd for sanity */

	bcb->fd = -1;

	/* translate address */

	if( ( bsp->status = unix_addr( bsp->buf, s, sizeof(s->sun_path)-1) ) != OK )
    	{
		SETCLERR( bsp->syserr, 0, 0 );
		return;
	}

	/* do socket request */

	iisock_request( bsp, s, size );
}

/*
** Name: unix_connect - complete an outgoing connection
*/

static VOID
unix_connect( bsp )
BS_PARMS	*bsp;
{
	struct 	sockaddr_un 	s[1];
	int 	size = sizeof( *s );

	/* do socket connect */

	iisock_connect( bsp, s, size );
}


/*
** Exported driver specification.
*/

BS_DRIVER BS_sockunix = {
	sizeof( BCB ),
	sizeof( LBCB ),
	unix_listen,
	unix_unlisten,
	unix_accept,
	unix_request,
	unix_connect,
	iisock_send,
	iisock_receive,
        0,              /* no orderly release */
	iisock_close,
	iisock_regfd,
	iisock_ok,	/* save not needed */
	iisock_ok,	/* restore not needed */
	0,		/* remote port translation needed */
	iisock_ext_info   /* extended information */
} ;

# endif /* xCL_066_UNIX_DOMAIN_SOCKETS */
