/*
**    Copyright (c) 1988, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<ck.h>
# include	<me.h>
# include	<lo.h>
# include	<si.h>
# include	<st.h>
# include	<gcatrace.h>

# include       <descrip.h>
# include       <ssdef.h>
# include       <iodef.h>
# include       <efndef.h>
# include       <iosbdef.h>
# include	<lib$routines.h>
# include	<starlet.h>

# include	<twgsocket.h>		/* nee sys/socket.h */
# include	<twgin.h>		/* nee netinet/in.h */
# include	<twgiodef.h>		/* nee vms/inetiodef.h */
# include	<twgnetdb.h>		/* nee netdb.h */

# include       <astjacket.h>

# ifndef	E_BS_MASK
# define	E_BS_MASK 0
# endif

# include	<bsi.h>


/*
** Name: gcwoltcp.c - BS interface to Wollongong TCP via VMS QIO's.
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
**	10-Sep-91 (seiwald)
**	    Don't free the BCB - we never allocated it.
**      26-Sep-91 (seiwald) bug #39654
**          Turn on keepalives.
**      26-Sep-91 (seiwald) bug #38578
**          Turn on SO_REUSEADDR, so that we can issue the network listen
**          even if there are various zombie connections (in FIN_WAIT_1,
**          etc.) still using the port.
**	6-Nov-91 (seiwald)
**	    Compute lwords properly before handing to STgetwords in case
**	    a line in etc:hosts. contains more than 16 words.
**	15-Oct-91 (hasty)
**	    Added tcp_close_0 to support asynchronous operation of
**	    tcp_close which now operates only asynchronously.
**	13-Jul-92 (alan)
**	    Release resources if tcp_listen fails.
**	04-Mar-93 (edg)
**	    Detect no longer in BS_DRIVER.
**      16-jul-93 (ed)
**	    added gl.h
**	23-mar-98 (kinte01)
**	    undef'ed u_short. With the addition of this defn to compat.h,
**	    it is now defined twice as it is also in twgtypes.h
**      27-Mar-98 (kinte01)
**          Rename inet_add to gc_inet_add as it conflicts with a definition
**          already in the DEC C shared library
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	04-jun-01 (kinte01)
**	    Replace STindex with STchr
**	28-aug-2001 (kinte01)
**	   Remove unneeded twgtypes.h
**      05-dec-2002 (loera01) SIR 109246
**         Set bsp->is_remote to TRUE.
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
*/

typedef long VMS_STATUS;

/*
** Name: LBCB, BCB - listen control block, connection control block
*/

typedef struct _LBCB
{
	i4	efn;			/* event flag for open function */
	short	chan;			/* channel for open function */
	IOSB	iosb;			/* iosb for open function */
	char	port[128];		/* printable port name */
} LBCB;

typedef struct _BCB
{
	short	chan;			/* channel for I/O */
	IOSB	riosb;			/* iosb for receives */
	IOSB	siosb;			/* iosb for sends */
} BCB;

# define LISTEN_QUE_DEPTH 10		/* 10 incoming conns at once */

/*
** Name: tcp_devname - internet device name
*/

static struct dsc$descriptor_s tcp_devname = 
{
	sizeof( "_inet0:" ), 0, 0, "_inet0:" 
} ;

/* From gctcpaddr.c */

FUNC_EXTERN STATUS GC_tcp_addr();
FUNC_EXTERN STATUS GC_tcp_port();
FUNC_EXTERN u_long gc_inet_addr();

static VOID tcp_accept_0();
static VOID tcp_accept_1();
static VOID tcp_request_0();
static VOID tcp_request_1();
static VOID tcp_send_0();
static VOID tcp_receive_0();
static VOID tcp_close_0();

GLOBALDEF int h_errno;


/*
** Brutal implementations of BSD library routines.
** Can't use entries from twg$tcp:[netdist.lib]libnet.olb - they link in the
** eunice world.
*/

static struct hostent *
gethostbyname( name )
char *name;
{
    LOCATION	loc[1];
    char	loc_buf[ MAX_LOC ];
    char 	line[ 132 ];
    FILE	*file;

    /* Returns pointer to this data */

    static struct hostent hostent;
    static char *h_addr_list;
    static u_long addr;

    /* Open the hosts file. */

    STcopy( "TWG$ETC:[000000]HOSTS.", loc_buf );
    LOfroms( PATH & FILENAME, loc_buf, loc );

    if( LOexist( loc ) != OK || SIopen( loc, "r", &file ) != OK )
	    return 0;

    /* Look for the given hostname on one of the lines. */

    while( SIgetrec( line, sizeof( line ), file ) != ENDFILE )
    {
	char 	*p;
	int 	lwords;
	char	*words[ 16 ];
	int	i;

	/* Trim # comments */

	if( p = STchr( line, '#' ) )
		*p = 0;

	/* Split line apart at whitespace */

	lwords = sizeof( words ) / sizeof( *words );
	STgetwords( line, &lwords, words );

	/* Look for hostname */

	for( i = 1; i < lwords; i++ )
	    if( !STbcompare( name, 0, words[ i ], 0, TRUE ) )
	{
	    /* Found hostname - return pointer to hostent with  */
	    /* proper address */

	    hostent.h_length = sizeof( addr );
	    hostent.h_addr_list = &h_addr_list;
	    h_addr_list = (char *)&addr;
	    addr = gc_inet_addr( words[ 0 ] );
	    SIclose( file );
	    return &hostent;
	}
    }

    /* Couldn't find name - indicate failure. */

    SIclose( file );

    return 0;
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
	int	on = 1;

	VMS_STATUS              status;
	struct	sockaddr_in	s[1];

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

	/*  Assign channel to tcp device _inet0  */

	status = sys$assign( &tcp_devname, &lbcb->chan, 0, 0 );
	
	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Create a socket  */

	status = sys$qiow( 
			lbcb->efn, 
			lbcb->chan, 
			IO$_SOCKET,
			&lbcb->iosb, 
			0, 
			0, 
			AF_INET, 
			SOCK_STREAM, 
			0, 0, 0, 0 );

	if( status != SS$_NORMAL || 
	    ( status = lbcb->iosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/* get listen port, if specified */

	bsp->status = GC_tcp_addr( bsp->buf, FALSE, gethostbyname, s );

	if( bsp->status != OK )
	{
	    bsp->syserr->error = 0;
	    goto complete;
	}

	/* Make address reusable */

	status = sys$qiow(
			lbcb->efn,
			lbcb->chan,
			IO$_SETSOCKOPT,
			&lbcb->iosb,
			0,
			0,
			SOL_SOCKET,	/* level */
			SO_REUSEADDR,	/* optname */
			&on,		/* optval */
			sizeof( on ),	/* optlen */
			0, 0);

	if( status != SS$_NORMAL ||
	    ( status = lbcb->iosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Bind the socket to the internet address  */

	status = sys$qiow( 
			lbcb->efn,
			lbcb->chan,
			IO$_BIND,
			&lbcb->iosb,
			0,
			0,
			s, 
			sizeof( *s ), 
			0 , 0, 0, 0);

	if( status != SS$_NORMAL || 
	    ( status = lbcb->iosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = BS_BIND_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Set the listen que depth  */

	status = sys$qiow(
			lbcb->efn,
			lbcb->chan,
			IO$_LISTEN,
			&lbcb->iosb,
			0,
			0,
			LISTEN_QUE_DEPTH,
			0, 0, 0, 0, 0);

	if( status != SS$_NORMAL ||
	    ( status = lbcb->iosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = BS_LISTEN_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/* get name for output */

	STcopy( bsp->buf, lbcb->port );
	bsp->buf = lbcb->port;

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

        bsp->is_remote = TRUE;

	/* clear chan for sanity */

	bcb->chan = -1;

	/* Open Inet device */

	status = sys$assign( &tcp_devname, &bcb->chan, 0, 0);

	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Post an outstanding listen  */

	status = sys$qio(
		lbcb->efn, 
		lbcb->chan, 
		IO$_ACCEPT_WAIT,
		&bcb->riosb, 
		tcp_accept_0, 
		bsp, 
		0, 0, 0, 0, 0, 0);

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
tcp_accept_0( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb = (BCB *)bsp->bcb;

	VMS_STATUS              status;

	/*  Check completion status of listen  */

	if( ( status = bcb->riosb.iosb$w_status ) != SS$_NORMAL)
	{
	    bsp->status = BS_ACCEPT_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Accept connect request for io channel  */

	status = sys$qio(
		    0, 
		    bcb->chan,
		    IO$_ACCEPT,
		    &bcb->riosb,
		    tcp_accept_1,
		    bsp,
		    0,
		    0,
		    lbcb->chan, 
		    0, 0, 0);

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
	BCB	*bcb = (BCB *)bsp->bcb;
	int	on = 1;

	VMS_STATUS              status;

	/*  Check completion status of accept  */

	if( ( status = bcb->riosb.iosb$w_status ) != SS$_NORMAL)
	{
	    bsp->status = BS_ACCEPT_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Turn on Keepalives */

	status = sys$qiow(EFN$C_ENF,
			bcb->chan,
			IO$_SETSOCKOPT,
			&bcb->riosb,
			0,
			0,
			SOL_SOCKET,	/* level */
			SO_KEEPALIVE,	/* optname */
			&on,		/* optval */
			sizeof( on ),	/* optlen */
			0, 0);

	if( status != SS$_NORMAL ||
	    ( status = bcb->riosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = BS_ACCEPT_ERR;
	    bsp->syserr->error = status;
	}

complete:
	/*  Drive the completion routine  */

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

	/* clear chan for sanity */

	bcb->chan = -1;

	/*  Assign channel to tcp device _inet0  */

	status = sys$assign( &tcp_devname, &bcb->chan, 0, 0 );
	
	if( status != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Create a socket  */

	status = sys$qio(
			0, 
			bcb->chan,
			IO$_SOCKET,
			&bcb->siosb,
			tcp_request_0,
			bsp,
			AF_INET,
			SOCK_STREAM,
			0, 0, 0, 0 );

	/*  Create a socket for communication  */

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

	/* Check status from socket creation */

	if( ( status = bcb->siosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = BS_SOCK_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/* translate address */

	bsp->status = GC_tcp_addr( bsp->buf, TRUE, gethostbyname, s );

	if( bsp->status != OK )
	{
	    bsp->syserr->error = 0;
	    goto complete;
	}

	/*  Now do the actual connect  */

	status = sys$qio(
			0,
			bcb->chan,
			IO$_CONNECT,
			&bcb->siosb,
			tcp_request_1,
			bsp,
			s,
			sizeof( *s ),
			0, 0, 0, 0 );

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
	int	on = 1;

	VMS_STATUS              status;

	/*  Check completion status of accept  */

	if( ( status = bcb->siosb.iosb$w_status ) != SS$_NORMAL)
	{
	    bsp->status = BS_CONNECT_ERR;
	    bsp->syserr->error = status;
	    goto complete;
	}

	/*  Turn on Keepalives */

	status = sys$qiow(EFN$C_ENF,
			bcb->chan,
			IO$_SETSOCKOPT,
			&bcb->siosb,
			0,
			0,
			SOL_SOCKET,	/* level */
			SO_KEEPALIVE,	/* optname */
			&on,		/* optval */
			sizeof( on ),	/* optlen */
			0, 0);

	if( status != SS$_NORMAL ||
	    ( status = bcb->siosb.iosb$w_status ) != SS$_NORMAL )
	{
	    bsp->status = BS_CONNECT_ERR;
	    bsp->syserr->error = status;
	}

complete:
	/*  Drive the completion routine  */

	(*(bsp->func))( bsp->closure );
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
	    bsp->status = BS_WRITE_ERR;
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
	    bsp->status = BS_READ_ERR;
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
	    (VOID)sys$qio(
			lbcb->efn,
			bcb->chan,
			IO$_SHUTDOWN,
			&bcb->siosb,
			tcp_close_0,
			bsp,
			2,		/* shutdown both sides */
			0, 0, 0, 0, 0);

	}
}

static VOID
tcp_close_0( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	(VOID)sys$dassgn( bcb->chan );
	(*(bsp->func))( bsp->closure );

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

GLOBALDEF BS_DRIVER GC_woltcp = {
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
