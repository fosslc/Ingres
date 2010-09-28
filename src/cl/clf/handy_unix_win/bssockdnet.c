/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<cm.h>
# include	<si.h>
# include	<st.h>
# include	<clpoll.h>
# include	<er.h>
# include	<pc.h>
# include	<ctype.h>
# include	<clsigs.h>
# include	<bsi.h>
# include	<cv.h>

# ifdef	xCL_006_FCNTL_H_EXISTS
# include	<fcntl.h>
# endif	/* xCL_006_FCNTL_H_EXISTS */

# include	<errno.h>


/*
** Low level interface for Ultrix/DECnet 
**
**
** Description:
**	This file provides access to ULTRIX/DECnet  via BSD sockets, using the 
**	generic BS interface defined in bsi.h.  See that file for a 
**	description of the interface.
**
**	The following functions are internal to this file:
**
**		dcnet_nonblock - make a fd non-blocking
**
**	The following functions are exported as part of the BS interface:
**
**		dcnet_listen - establish an endpoint for accepting connections
**		dcnet_unlisten - stop accepting incoming connections
**		dcnet_accept - accept a single incoming connection
**		dcnet_request - make an outgoing connection
**		dcnet_connect - complete an outgoing connection
**		dcnet_send - send data down the stream
**		dcnet_receive - read data from a stream
**		dcnet_close - close a single connection
**		dcnet_regfd - register for blocking operation
** History:
**	01-Apr-89 (GordonW)
**		written .
**	17-Aug-90 (gautam)
**		Rewritten to conform to 6.4 GCF
**	6-Sep-90 (seiwald)
**		Set bsp->len to 0 on end of message, to conform with
**		change to BS interface.
**	23-Apr-91 (gautam)
**		Rename protocol driver as BS_sockdnet
**	29-Apr-91 (gautam)
**		Fix for PC group (37258, 37293) - do not process 
**		access control information
**	7-Aug-91 (seiwald)
**		Use new CLPOLL_FD(), replacing CLPOLL_RESVFD().
**	29-aug-91 (bonobo)
**		Added bogus function name to eliminate ranlib warnings.
**	30-apr-92 (jillb--DGC) (kevinm)
**          dg8_us5 returns EWOULDBLOCK instead of EINPROGRESS on a connect
**          to a non-blocking socket. Changed the error return check to
**          EINPROGRESS and EWOULDBLOCK .
**	11-Mar-92 (gautam)
**		Bugfix for iigcc looping when Decnet is not installed (42776)
**	04-Mar-93 (edg)
**		Removed detect logic.
**      16-mar-93 (smc)
**              Cast parameters of ST functions to same type as prototypes.
**	06-Apr-93 (brucek)
**	    Added NULL function pointer to BS_DRIVER for orderly release.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	1-jun-94 (robf)
**  	    added ext_info request for BS driver
**	14-Jan-1998 (merja01)
**		Fix Segmentation violation by changing call to MEcopy to pass
**		addr_dn.sdn_add by reference.  This caused the gcc server to
**		crash when attempting DECnet connections by DECnet address.
**	05-feb-1999 (loera01) Bug 95164
**	    Replaced code for BS_dcnet_port() with the exact code from VMS. 
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      10-Oct-2000 (hanal04) Bug 102878.
**          Remove typedef for _BCB as this is already defined in bsi.h
**      19-nov-2002 (loera01)
**          Set is_remote to TRUE in BS_PARMS.
**	4-Dec-2008 (kschendel)
**	    Declare errno correctly (i.e. via errno.h).
**      17-jun-2010 (shust01)
**          Include cv.h header file to resolve CVupper() being undefined.
**          Bug 123139.
*/

#ifdef	xCL_041_DECNET_EXISTS

# include	<sys/socket.h>
# include	<netdnet/dn.h>
# include	<netdnet/dnetdb.h>

#ifndef	MAXHOSTNAME
#define	MAXHOSTNAME	1064
#endif

#ifndef	MAXPORTNAME
#define	MAXPORTNAME	1064
#endif

/* number of connect indications that can be queued */

#define	LISTEN_Q	5

/*
** External data
*/

/*
** Local definitions 
*/

typedef struct _LBCB
{
	i4	fd;			/* listen fd */
	char	port[ MAXPORTNAME ];	/* port we're listening on */
} LBCB;

# define BS_SKIP_R_SELECT	0x01	/* don't select before next read */
# define BS_SKIP_W_SELECT	0x02	/* don't select before next write */

/*
** local data
*/
static	i4	listen_num = 128;	/* starting listening number */


/*
** Support routines - not visible outside this file.
*/

/*
** Name: sock_nonblock - make a fd non-blocking
*/

static int
dcnet_nonblock( fd )
int     fd;
{
	int	status;

	/* set for non-blocking */

# ifdef xCL_006_FCNTL_H_EXISTS
	status = fcntl( fd, F_SETFL, O_NDELAY );
# endif /* xCL_006_FCNTL_H_EXISTS */

# ifdef xCL_007_FILE_H_EXISTS
	status = fcntl( fd, F_SETFL, FNDELAY );
# endif /* xCL_007_FILE_H_EXISTS */

	return status;
}

/*
** Name: sock_nodelay - turn on DECNET_NODELAY
*/

static VOID
dcnet_nodelay( fd )
int     fd;
{
}


/*
** Name: dcnet_listen - establish an endpoint for accepting connections
*/

static	VOID
dcnet_listen(bsp)
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	int	len, sock, pid;
	struct	sockaddr_dn	addr_dn;
	char	type, object[MAXHOSTNAME];

	/* clear fd for sanity */
	lbcb->fd = -1;

	/*
	** setup objectname
	*/
	if(STlength(bsp->buf) == 0)
	{
		PCpid(&pid);
		(VOID)STprintf(object, "II_IPC%d_%d", pid, listen_num);
		listen_num++;
	}
	else
	{
		(VOID)STcopy(bsp->buf, object);
	}

	/*
	** create a socket 
	*/

	sock = socket(AF_DECnet, SOCK_SEQPACKET, 0);
	CLPOLL_FD( sock );

	if (sock < 0)
	{
		bsp->status = BS_SOCK_ERR;
		SETCLERR(bsp->syserr, 0, 0);
		return;
	}
	
	/*
	** setup sockaddr_dn
	*/
	len = sizeof(addr_dn);
	(VOID)MEfill(len, NULL, &addr_dn);
	addr_dn.sdn_family = AF_DECnet;
	if( object[0] == '#' || isalpha(object[0]) )
	{
		(VOID)STcopy(object, addr_dn.sdn_objname);
		addr_dn.sdn_objnamel = STlength(addr_dn.sdn_objname);
	}
	else
	{
		addr_dn.sdn_objnum = atoi(object);
	}

	/*
	** bind it
	*/
	len = sizeof(addr_dn);
	if(bind(sock, &addr_dn, len) < 0)
	{
		(VOID)close(sock);
		bsp->status = BS_BIND_ERR;
		SETCLERR(bsp->syserr, 0, 0);
		return;
	}

	/*
	** now let it listen
	*/
	if(listen(sock, LISTEN_Q) < 0)
	{
		(VOID)close(sock);
		bsp->status = BS_LISTEN_ERR;
		SETCLERR(bsp->syserr, 0, 0);
		return;
	}

	/*
	** setup defered mode
	*/
	type = (char)ACC_DEFER;
	len = sizeof(type);
	if(setsockopt(sock, DNPROTO_NSP, DSO_ACCEPTMODE, &type, len) < 0)
	{
		(VOID)close(sock);
		bsp->status = BS_LISTEN_ERR;
		SETCLERR(bsp->syserr, 0, 0);
		return;
	}

	/*
	** A OK
	*/
	lbcb->fd = sock;
	STcopy(object, bsp->buf);
	bsp->status = OK;
	return;
} 


/*
** Name: dcnet_unlisten: stop accepting incoming connections
*/

static	VOID
dcnet_unlisten(bsp)
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;

	if (lbcb->fd >= 0)
	{
		shutdown(lbcb->fd, 2);
		close(lbcb->fd);
	}
	bsp->status = OK;
	return;
}



/*
** Name:  dcnet_accept - accept a single incoming connection
*/
static	VOID
dcnet_accept(bsp)
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	BCB	*bcb =  (BCB *) bsp->bcb;
	int	len, sock;
	struct sockaddr_dn	addr_dn;

	/*
	** clear fd for sanity
	*/
	bcb->fd = -1;
	bsp->is_remote = TRUE;

	/*
	** try an do an accept
	*/
	len = sizeof(addr_dn);

	sock = accept(lbcb->fd, &addr_dn, &len);
	CLPOLL_FD( sock );

	if (sock < 0)
	{
		bsp->status = BS_ACCEPT_ERR;
		SETCLERR(bsp->syserr, 0 , 0);
		return;
	}

	/*
	** allow access
	*/
	if(setsockopt(sock, DNPROTO_NSP, DSO_CONACCEPT, (char *)0, 0) < 0)
	{
		close(sock);
		bsp->status = BS_ACCEPT_ERR;
		SETCLERR(bsp->syserr, 0 , 0);
		return;
	}

	/*
	** set to NON-blocking
	*/
	if(dcnet_nonblock(sock) < 0)
	{
		(VOID)close(sock);
		bsp->status = BS_ACCEPT_ERR;
		SETCLERR(bsp->syserr, 0 , 0);
		return;
	}

	/*
	** A OK
	*/
	bcb->fd = sock;
	bsp->status = OK;
	return;
} 


/*
** Name: dcnet_request: make an outgoing connection
**
**	14-Jan-1998 (merja01)
**		Fix Segmentation violation by changing call to MEcopy to pass
**		addr_dn.sdn_add by reference.  This caused the gcc server to
**		crash when attempting DECnet connections by DECnet address.
*/

static  VOID
dcnet_request(bsp)
BS_PARMS	*bsp;
{
	BCB	*bcb =  (BCB *) bsp->bcb;
	int	len, sock, l;
	struct sockaddr_dn	addr_dn;
	struct accessdata_dn	login;
	struct dn_naddr	*naddr, *dnet_addr();
	struct nodeent	*np, *getnodebyname();
	char	*p, hostname[MAXHOSTNAME], acc[1];
	char	user[MAXHOSTNAME], pass[MAXHOSTNAME], object[MAXHOSTNAME];
	char	*allow_proxy, proxy, old_proxy;
	char	*buf;

	buf = bsp->buf;

	/*
	** look for a "::"
	*/
	for(p=buf, l=0; *p; p++, l++)
	{
		if(*p == ':' && *(p+1) == ':')
			break;
	}

	/*
	** what did we find ? 
	*/
	if(*p)
	{
		/*
		** host::object
		*/
		STncpy( hostname, buf, l);
		hostname[ l ] = EOS;
		(VOID)STcopy( (p+2), object);
	}
	else
	{
		/*
		** No host
		*/
		l = sizeof(hostname);
		(VOID)GChostname(hostname, l);
		(VOID)STcopy(buf, object);
	}

	/*
	** parse the login info
	*/
	acc[0] = '\0';
	user[0] = '\0';
	pass[0] = '\0';

	/*
	** check for proxy login
	*/
	proxy = 0;
	old_proxy = proxy_requested;
	(VOID)NMgtAt("II_DECNET_PROXY", &allow_proxy);
	if( allow_proxy && *allow_proxy )
	{
		if(STcompare(allow_proxy, "no") == 0)
			proxy = old_proxy;
		else if(STcompare(allow_proxy, "off") == 0)
			proxy = 0;
	}
	proxy_requested = proxy;

	/*
	** create a socket
	*/
	sock = socket(AF_DECnet, SOCK_SEQPACKET, 0);
	CLPOLL_FD( sock );

	if (sock < 0)
	{
		bsp->status = BS_SOCK_ERR;
		SETCLERR(bsp->syserr, 0, 0);
		return;
	}

	/*
	** set for non-blocking
	*/
	if (dcnet_nonblock(sock) < 0)
	{
		(VOID)close(sock);
		bsp->status = BS_SOCK_ERR;
		SETCLERR(bsp->syserr, 0, 0);
		return;
	}

	/*
	** setup access
	*/
	len = sizeof(login);
	(VOID)MEfill(len, NULL, &login);
	(VOID)STcopy(acc, (char *)login.acc_acc);
	login.acc_accl = STlength((char *)login.acc_acc);
	(VOID)STcopy(pass, (char *)login.acc_pass);
	login.acc_passl = STlength((char *)login.acc_pass);
	(VOID)STcopy(user, (char *)login.acc_user);
	login.acc_userl = STlength((char *)login.acc_user);
	if( setsockopt(sock, DNPROTO_NSP, DSO_CONACCESS, &login, len) < 0)
	{
		(VOID)close(sock);
		bsp->status = BS_SOCK_ERR;
		SETCLERR(bsp->syserr, 0, 0);
		return;
	}

	/*
	** now fill in the sockaddr_dn
	*/
	len = sizeof(addr_dn);
	(VOID)MEfill(len, NULL, &addr_dn);
	addr_dn.sdn_family = AF_DECnet;
	if( object[0] == '#' || isalpha(object[0]) )
	{
		(VOID)STcopy(object, addr_dn.sdn_objname);
		addr_dn.sdn_objnamel = STlength(addr_dn.sdn_objname);
	}
	else
	{
		addr_dn.sdn_objnum = atoi(object);
	}
	addr_dn.sdn_flags |= (proxy ? SDF_PROXY : 0);

	/*
	** set the node addr
	*/
	if( (naddr = dnet_addr(hostname)) == NULL)
	{
		if( (np = getnodebyname(hostname)) == NULL)
		{
			(VOID)close(sock);
			bsp->status = BS_SOCK_ERR;
			SETCLERR(bsp->syserr, 0, 0);
			return;
		}

		(VOID)MEcopy(np->n_addr, np->n_length, addr_dn.sdn_nodeaddr);
		addr_dn.sdn_nodeaddrl = np->n_length;
	}
	else
	{
		(VOID)MEcopy(naddr, sizeof(*naddr), &addr_dn.sdn_add);
	}

	/*
	** try to connect
	*/
	len = sizeof(addr_dn);

	/* 
	** We are checking both EINPROGRESS and EWOULDBLOCK below because
	** dg8_us5 returns EWOULDBLOCK instead of EINPROGRESS on a connect
	** to a non-blocking socket.
	*/

	if( (connect(sock, &addr_dn, len) < 0) && (errno != EINPROGRESS) 
		&& (errno != EWOULDBLOCK) )
	{
		bsp->status = BS_CONNECT_ERR;
		(VOID)close(sock);
		SETCLERR(bsp->syserr, 0, 0);
		return;
	}

	/*
	** A OK
	*/
	bcb->fd = sock;
	return;
} 



/*
** Name: dcnet_connect: complete outgoing connection
*/

static VOID
dcnet_connect( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	int	len;
	struct  sockaddr_dn	addr_dn;

	len = sizeof(addr_dn);
	if((getpeername(bcb->fd, &addr_dn, &len) < 0) ||   (len == 0) )
	{
		bsp->status = BS_CONNECT_ERR;
		SETCLERR( bsp->syserr, 0, 0 );
		return;
	}
	/* return OK */

	bsp->status = OK;
	return;
}


/*
** Name: dcnet_receive - read data from DECNET connection
*/
static VOID
dcnet_receive( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	int	n;

	/* short reads are OK - EOF is not */
	/* set bsp->len to 0 on end of message */

	if( ( n = read( bcb->fd, bsp->buf, bsp->len ) ) <= 0 )
	{
		if( !n || ( errno != EWOULDBLOCK && errno != EINTR ) )
		{
			bsp->status = BS_READ_ERR;
			SETCLERR( bsp->syserr, 0, 0 );
			return;
		}
		n = 0;
	} else {
		bsp->len = 0;
	}

	bsp->buf += n;
	bsp->status = OK;
}

/*
** Stub for completeness
*/
static VOID
dcnet_ok( bsp )
BS_PARMS	*bsp;
{
	bsp->status = OK;
}
/*
** Name: dcnet_send - send data down thru  DECNET 
*/
static VOID
dcnet_send( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	int	n;

	/* short sends are OK */

	if( ( n = write( bcb->fd, bsp->buf, bsp->len ) ) < 0 )
	{
		if( errno != EWOULDBLOCK)
		{
			bsp->status = BS_WRITE_ERR;
			SETCLERR( bsp->syserr, 0, 0 );
			return;
		}
		n = 0;
		bcb->optim &= ~BS_SKIP_W_SELECT;
	}
	else
	{
		bcb->optim |= BS_SKIP_W_SELECT;
	}

	bsp->len -= n;
	bsp->buf += n;
	bsp->status = OK;
}

/*
** Name: dcnet_close - close a single connection
*/
static VOID
dcnet_close( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
		
	if( bcb->fd < 0 )
		goto ok;

	/* unregister file descriptors */

	(void)iiCLfdreg( bcb->fd, FD_READ, (VOID (*))0, (PTR)0, -1 );
	(void)iiCLfdreg( bcb->fd, FD_WRITE, (VOID (*))0, (PTR)0, -1 );

	(VOID)shutdown(bcb->fd, 2);
	(VOID)close(bcb->fd);
ok:
	bsp->status = OK;
}


/*
** Name: dcnet_regfd - register for blocking operation
*/

static bool
dcnet_regfd( bsp )
BS_PARMS	*bsp;
{
	int	fd;
	int	op;
	BCB	*bcb = (BCB *)bsp->bcb;
	LBCB	*lbcb = (LBCB *)bsp->lbcb;

	switch( bsp->regop )
	{
	case BS_POLL_ACCEPT:
		fd = lbcb->fd; op = FD_READ; break;

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
** Name: BS_dcnet_port: map a port name into a port number
**
** Description:
**	Decnet addresses for INGRES/NET, as known to decnet, are of the form:
**
**		II_GCCxx_N
**
**	Users may use any of the following shorthand address formats
**	when refering to decnet addresses:
**
**		N
**		xx
**		xxN
**		II_GCCxx
**		II_GCCxx_N
**
**	This routine maps the above into II_GCCxx_N.  If xx is not given
**	as part of the address, it is ommitted in the output address.
**	When N is not given as part of the address, the subport parameter
**	will be used.
**
** Inputs:
**	pin	input address
**	subport	value to use for N
**
** Outputs:
**	pout	output address
**
** Returns:
**	FAIL	if N given and subport != 0 or subport > 9
**	OK	otherwise
**
** History:
**	21-Sep-92 (seiwald) bug #45772
**	    Fixed listen logical in production installations.
**      19-Jan-1999 (hanal04/horda03) Bug 94918
**          The character pointer p was not advanced sufficiently
**          when the pin parameter was of the form II_GCCxxxxx.
**          Thus the function failed to determine the real decnet
**          address. Also, if an installation id is specified,
**          this is limited to 2 characters (the char pointer inst
**          is set to the start of the installation id from the PIN
**          parameter; but this parameter could contain the port
**          number too - e.g II_GCCAB_1).
**          There was also a problem where a listen address of II_GCC
**          was deemed invalid (so II_GCC would be returned). II_GCC
**          is a valid listen address and should have the port id
**          appended (e.g II_GCC ==> II_GCC_x).
**	09-feb-1999 (loera01) Bug 95164
**	    Cross-integrated from VMS.  This routine supercedes the previous
**	    Unix code.
*/

STATUS
BS_dcnet_port( pin, subport, pout )
char	*pin;
i4 	subport;
char	*pout;
{
	char	*inst = 0;
	char	*n = 0;
	bool	iigcc = FALSE;
	char	*p = pin; 
	char	portbuf[ 5 ];

	/* Pull pin apart */

	if( !STncasecmp( p, "II_GCC", 6 ) )
	{
	    iigcc = TRUE;
	    p += STlength("II_GCC");   /* Bug 94918 */
	}

	if( CMalpha( &p[0] ) && ( CMalpha( &p[1] ) | CMdigit( &p[1] ) ) )
	{
	    inst = p;
	    p += 2;
	}

	if( iigcc && p[0] == '_' )
        {
	    p++;
	}

	if( CMdigit( &p[0] ) )
	{
	    n = p;
	    p++;
	}

	/* Is port an unrecognised form? */

	if( p[0] || !inst && !n && !iigcc)
	{
	    if( subport )
		return FAIL;

	    STcopy( pin, pout );

	    return OK;
	}

	/* Put pout together. */

	if( n && subport || subport > 9 )
	    return FAIL;

	if( !n )
	    CVna( subport, n = portbuf );

	if( !iigcc && !inst )
	    NMgtAt( "II_INSTALLATION", &inst );

	if( !inst )
	    inst = "";

        /* Bug 94918. Only use a max. of 2 characters from INST.
        ** If INST = "" then pout will be of the form II_GCC_x.
        **
        ** If INST = "AB" or "AB_1" then the pout will be
        ** of the form II_GCCAB_x.
        */
	STprintf( pout, "II_GCC%0.2s_%s", inst, n );
        CVupper(pout);
	return OK;
}

/*
** Exported driver specification.
*/

BS_DRIVER BS_sockdnet = {
	sizeof( BCB ),
	sizeof( LBCB ),
	dcnet_listen,
	dcnet_unlisten,
	dcnet_accept,
	dcnet_request,
	dcnet_connect,
	dcnet_send,
	dcnet_receive,
	0,		/* no orderly release */
	dcnet_close,
	dcnet_regfd,
	dcnet_ok,	/* save */
	dcnet_ok,	/* restore */
	BS_dcnet_port,
	0,		/* no extended info currently */
} ;
# else
VOID xCL_041_DECNET_DOES_NOT_EVEN_EXIST(){};
# endif
