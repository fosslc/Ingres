/*
**    Copyright (c) 1988, 2007 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<me.h>
# include	<lo.h>
# include	<st.h>
# include	<cv.h>

#define IN6_HAS_64BIT_INTTYPE
#define _SOCKADDR_LEN

# include       <in.h>
# include       <inet.h>
# include       <netdb.h>
# include       <tcpip$inetdef.h>  

# ifndef	E_BS_MASK
# define	E_BS_MASK 0
# endif

# include	<bsi.h>


/*
** Name: gctcpaddr.c - tcp address support
**
** Description:
**	This module does the addressing support for TCP protocol drivers
**	on VMS.  It uses the Wollongong TCP structure definitions, so
**	it therefore supports only those TCP implementations which mock
**	the UNIX one.
**
**	Because different implementations of TCP use different a different
**	gethostbyname(), this function is passed into GC_tcp_addr.
**
** History:
**	21-Jan-91 (seiwald)
**	    Ported.
**	23-Apr-91 (seiwald)
**	    Use CVan, not atoi.
**      16-jul-93 (ed)
**	    added gl.h
**      23-mar-98 (kinte01)
**          undef'ed u_short. With the addition of this defn to compat.h,
**          it is now defined twice as it is also in twgtypes.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	28-aug-2001 (kinte01)
**	   Remove unneeded twgtypes.h
**      16-Jan-2007 (Ralph Loen) SIR 116548
**          Added GC_tcp6_addr() and GC_tcp6_set_socket() to support IPv6.    
**      28-Sep-2007 (Ralph Loen) Bug 119215
**          Reverted SOCKADDRIN6 overlay to sockaddr_in6 and used
**          SIN6$C_LENGTH to specify the size.  Removed hard-coded "AI_" 
**          constants.  Included "AI_ALL" in the hints flag mask for
**          getaddrinfo().  Plug in the port value to the sockaddr_in6
**          structure returned from getaddrinfo() rather than relying on
**          getaddrinfo() to resolve the port.
**       02-June-08 (gordy and Ralph Loen)  SIR 120457
**          Implemented extended symbolic port range mapping algorithm.
**          Implemented support for explicit port rollup indicator for
**          symbolic and numeric ports.
**       13-May-2010 (Ralph Loen) Bug 120552
**          Add a new output argument to GC_tcp_addr() which returns the
**          "actual" symbolic port (base port plus subport).
*/

u_long
gc_inet_addr( s )
char	*s;
{
	u_long	addr = 0;
	int	part = 0;

	do switch( *s )
	{
	default: 	part = part * 10 + *s - '0'; break;
	case '.': 	addr = addr * 256 + part; part = 0; break;
	case 0:		addr = addr * 256 + part; break;
	}
	while( *s++ );

	return htonl( addr );
}

#ifndef	MAXHOSTNAME
#define	MAXHOSTNAME	32
#endif

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK         htonl( (u_long)0x7F000001 )
#endif


/*
** Name: GC_tcp_addr - translate symbolic host::port to sockaddr_in struct
**
** Description:
**	Translates string representation of INET address (host::port) to
**	a sockaddr_in struct.  
**
**	o	Accepts "dotted quad" notation (123.119.1.103) for host
**	o	Treats missing host:: as local host (INADDR_ANY)
**	o	Treats missing port as port 0.
**
** Inputs:
**	buf		string form of address
**	isactive 	address must contain connection address (not passive)
**	gethostbyname	function to get hostent struct for hostname
**
** Outputs:
**	s	sockaddr_in form of address
**
** Returns:
**	BS_NOHOST_ERR	host unknown or malformed dotted quad
**	BS_NOPORT_ERR	port malformed
**	OK
**
** History:
**	26-May-90 (seiwald)
**	    Extracted from old BS code.
**	07-Aug-90 (seiwald)
**	    Sequent's connect() doesn't allow INADDR_ANY on active 
**	    (outbound) connections - use INADDR_LOOPBACK instead, defining 
**	    it if necessary.
**	22-Aug-90 (anton)
**	    MCT - protect call to gethostbyname and data copy
**	17-Sep-90 (anton)
**	    Use symbol MT_CS to identify MCT specific code
**	18-Jan-91 (seiwald)
**	    Use CM macro's rather than ctype.
**      27-Mar-98 (kinte01)
**          Rename inet_add to gc_inet_add as it conflicts with a definition
**          already in the DEC C shared library
*/

STATUS
GC_tcp_addr( buf, isactive, gethostbyname, s )
char	*buf;
bool	isactive;
struct hostent	*(*gethostbyname)();
struct sockaddr_in *s;
{
	char	*p;
	char	hostname[ MAXHOSTNAME ];
	struct hostent	*hp;

	/* set defaults */

	s->sin_family = AF_INET;
	s->sin_addr.s_addr = INADDR_ANY;
	s->sin_port = 0;

	/* parse address - look for a "::" */

	for( p = buf; *p; p++ )
	    if( p[0] == ':' && p[1] == ':' )
		break;

	/* get hostname, if present */

	if( *p )
	{
		/* host::port */

		STlcopy( buf, hostname, p - buf );

		if( CMdigit( hostname ) )
		{
			/* host id, e.g. 128.0.2.1 */

			if( !( s->sin_addr.s_addr = gc_inet_addr( hostname ) ) )
				return BS_NOHOST_ERR;
		}
		else
		{
			/* host name, e.g. llama */

			if( !( hp = (*gethostbyname)( hostname ) ) )
			{
				return BS_NOHOST_ERR;
			}
			MEcopy((PTR)hp->h_addr, hp->h_length, (char *)&s->sin_addr);
		}
	} 

	/* now get port, if present */

	p = *p ? p + 2 : buf;

	if( CMdigit( p ) )
	{
		/* 1234 port number */

		i4 port;

		CVan( p, &port );

		s->sin_port = htons( (u_short)port );
	}
	else if( *p || isactive )
	{
		return BS_NOPORT_ERR;
	}

	/* no obvious errors */

	return OK;
}


/*
** Name: GC_tcp_port - turn a tcp port name into a tcp port number
**
** Description:
**	This routines provides mapping a symbolic port ID, derived
**	from II_INSTALLATION, into a unique tcp port number for the 
**	installation.  This scheme was originally developed by the 
**	NCC (an RTI committee) and subsequently enhanced with an
**	extended port range and explicit port rollup indicator.
**
**	If pin is of the form:
**		XY{n}{+}
**	where
**		X is [a-zA-Z]
**		Y is [a-zA-Z0-9]
**		n is [0-9] | [0][0-9] | [1][0-5]
**		+ indicates port rollup permitted.
**
**	then pout is the string representation of the decimal number:
**
**        15  14  13  12  11  10  09  08  07  06  05  04  03  02  01  00
**       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
**       !   S   !  low 5 bits of X  !   low 6 bits of Y     !     #     !
**       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
**
**	# is the low 3 bits of the subport and S is 0x01 if the subport
**	is in the range [0,7] and 0x10 if in range [8,15]. 
**
**	The subport is a combination of the optional base subport {n} and
**	the rollup subport parameter.  The default base subport is 0.  A
**	rollup subport is permitted only if a '+' is present or if the
**	symbolic ID has form XY.
**
**	Port rollup is also permitted with numeric ports when the input
**	is in the form n+.  The numeric port value is combined with the
**	rollup subport parameter which must be in the range [0,15].
**
**	If pin is not a recognized form; copy pin to pout.
**
** Inputs:
**	portin - source port name
**	subport - added as offset to port number
**	portout - resulting port name 
**
** History
**	23-jun-89 (seiwald)
**	    Written.
**	28-jun-89 (seiwald)
**	    Use wild CM macros instead of the sane but defunct CH ones.
**	2-feb-90 (seiwald)
**	    New subport parameter, so we can programmatically generate
**	    successive listen ports to use on listen failure.
**	24-May-90 (seiwald)
**	    Renamed from GC_tcp_port.
**	 2-Apr-08 (gordy)
**	    Implemented extended symbolic port range mapping algorithm.
**	    Implemented support for explicit port rollup indicator for
**	    symbolic and numeric ports.
*/
STATUS
GC_tcp_port( pin, subport, pout, pout_symbolic )
char	*pin;
i4	subport;
char	*pout;
char    *pout_symbolic;
{
    u_i2 portid, offset;
    
    /*
    ** Check for symbolic port ID format: aa or an
    **
    ** Input which fails to correctly match the symbolic 
    ** port ID syntax is ignored (by breaking out of the 
    ** for-loop) rather than being treated as an error.
    */
    for( ; CMalpha( &pin[0] ) && (CMalpha( &pin[1] ) | CMdigit( &pin[1] )); )
    {
	bool	rollup = FALSE;
	u_i2	baseport = 0;
	char	p[ 2 ];

	/*
	** A two-character symbolic port permits rollup.
	*/
	offset = 2;
	if ( ! pin[offset] )  rollup = TRUE;

	/*
	** One or two digit base subport may be specified
	*/ 
	if ( CMdigit( &pin[offset] ) )
	{
	    baseport = pin[offset++] - '0';

	    if ( CMdigit( &pin[offset] ) )
	    {
		baseport = (baseport * 10) + (pin[offset++] - '0');

		/*
		** Explicit base subport must be in range [0,15]
		*/
		if ( baseport > 15 )  break;
	    }
	}

	/*
	** An explict '+' indicates rollup.
	*/
	if ( pin[ offset ] == '+' )
	{
	    rollup = TRUE;
	    offset++;
	}

	/*
	** There should be no additional characters.
	** If so, input is ignored.  Otherwise, this
	** is assumed to be a symbolic port and any
	** additional errors are not ignored.
	*/
	if ( pin[ offset ] )  break;

	/*
	** Prepare symbolic port components.
	** Alphabetic components are forced to upper case.
	** Rollup subport increases base subport.
	*/
	CMtoupper( &pin[0], &p[0] );
	CMtoupper( &pin[1], &p[1] );
	baseport += subport;

	/*
	** A rollup subport is only permitted when
	** rollup is specified.  The combined base
	** and rollup subports must be in the range 
	** [0,15].
	*/
	if ( subport  &&  ! rollup )  return( FAIL );
	if( baseport > 15 )  return( FAIL );

	/*
	** Map symbolic port ID to numeric port.
	*/
	portid = (((baseport > 0x07) ? 0x02 : 0x01) << 14)
		    | ((p[0] & 0x1f) << 9)
		    | ((p[1] & 0x3f) << 3)
		    | (baseport & 0x07);

	CVla( (u_i4)portid, pout );
	/* Suppress 0 when displaying the actual symbolic port. 
	** Windows don't display subport value of 0... 
	*/
	if( baseport == 0 )
	    STprintf(pout_symbolic, "%c%c", pin[0], pin[1]);
	else
	    STprintf(pout_symbolic, "%c%c%d", pin[0], pin[1], baseport);
	return( OK );
    } 

    /*
    ** Check for a numeric port and optional port rollup.
    **
    ** If input is not strictly numeric or if port rollup
    ** is not explicitly requested, input is ignored.
    **
    ** Extract the numeric port and check for expected
    ** syntax: n{n}+
    */
    for( offset = portid = 0; CMdigit( &pin[offset] ); offset++ )
	portid = (portid * 10) + (pin[offset] - '0');

    if ( pin[offset] == '+'  &&  ! pin[offset+1] )
    {
	/*
	** Port rollup is restricted to range [0,15].
	*/
	if ( subport > 15 )  return( FAIL );
	CVla( portid + subport, pout );
	STcopy(pout, pout_symbolic);
	return( OK );
    }

    /*
    ** Whatever we have at this point, port rollup is not
    ** allowed.  Input is simply passed through unchanged.
    */
    if( subport )  return( FAIL );
    STcopy( pin, pout );
    STcopy(pin, pout_symbolic);

    return( OK );
}


/*
** Name: GC_tcp6_addr - translate symbolic host::port to addrinfo linked list
**
** Description:
**	Translates string representation of INET address (host::port) to
**	an addrinfo linked list.  
**
**	o	Accepts "dotted quad" notation (123.119.1.103) for host
**	o	Accepts IPv6 notation (xxxx::xxxx:xxxx:xxxx:xxxx) for host
**
** Inputs:
**	buf		string form of address or hostname
**
** Outputs:
**      addrinfo    	linked list of IPv6 and IPv4 addresses
**                      Note: memory allocated by the call to getaddrinfo()
**                      needs to be freed later by freeaddrinfo().
**
** Returns:
**	BS_NOHOST_ERR	host unknown or malformed dotted quad
**	BS_NOPORT_ERR	port malformed
**	BS_INTERNAL_ERR	bad "tcpip_version" specified
**	OK
**
** History:
**  16-Jan-2007 (Ralph Loen) SIR 116548
**      Created.    
*/

STATUS
GC_tcp6_set_socket( char *buf, struct sockaddr_in6 *s)
{
    i4 port;

    /*
    ** Initialize socket structure.  A side-effect is that the wildcard 
    ** listen address is set to IN6ADDR_ANY_INIT.
    */
    MEfill ( SIN6$C_LENGTH, 0, s );

    /* set socket family */
    s->sin6_family = AF_INET6;

    if (CMdigit(buf))
    {
        CVan( buf, &port );
        s->sin6_port = htons( (u_short)port );
    }
    else
        return (BS_NOPORT_ERR);

    /* no obvious errors */
    return OK;
}


/*
** Name: GC_tcp6_addr - translate symbolic host::port to addrinfo linked list
**
** Description:
**	Translates string representation of INET address (host::port) to
**	an addrinfo linked list.  
**
**	o	Accepts "dotted quad" notation (123.119.1.103) for host
**	o	Accepts IPv6 notation (xxxx::xxxx:xxxx:xxxx:xxxx) for host
**
** Inputs:
**	buf		string form of address or hostname
**
** Outputs:
**      addrinfo    	linked list of IPv6 and IPv4 addresses
**                      Note: memory allocated by the call to getaddrinfo()
**                      needs to be freed later by freeaddrinfo().
**
** Returns:
**	BS_NOHOST_ERR	host unknown or malformed dotted quad
**	BS_NOPORT_ERR	port malformed
**	BS_INTERNAL_ERR	bad "tcpip_version" specified
**	OK
**
** History:
**  16-Jan-2007 (Ralph Loen) SIR 116548
**      Created.    
*/

STATUS
GC_tcp6_addr( char *buf, struct addrinfo **res )
{
    char	*p, *p1;
    char	hostname[ MAXHOSTNAME ];
    STATUS status;         
    struct addrinfo hints; /* input values to direct operation */
    i4 port;
    int i;
    static i4    tcpip_version = -1;
    char *ptr;
    struct sockaddr_in6 *s_in6;
    struct addrinfo *resAddr=NULL;

    /*
    **  Check to see if driver is being restricted to IPv6 only.  If the
    **  version is 4, we don't even get here because only IPv4 routines 
    **  are used.  See GCpinit().
    */
    if (tcpip_version == -1)
    {
        NMgtAt( "II_TCPIP_VERSION", &ptr );   
        if ( !(ptr && *ptr) && PMget("!.tcp_ip.version", &ptr) != OK )
            tcpip_version = 0;  /* Default is both ipv6 and ipv4 ("all") */
        else if (STbcompare(ptr, 0, "ALL", 0, TRUE) == 0)
            tcpip_version = 0;  
        else if (CMdigit(ptr))
        {
            CVal(ptr, &tcpip_version);     /* 4=IPv4 or 6=IPv6 */
            if (tcpip_version != 4 && tcpip_version != 6)
                 return BS_INTERNAL_ERR;
        }
        else
            return BS_INTERNAL_ERR;
    }
    else if (tcpip_version && tcpip_version != 6)
        return BS_INTERNAL_ERR;

    /* set defaults */

    /* parse address - look for a "::" */
    p = p1 = STstrindex(buf,"::", 0, FALSE);

    /* Get hostname, if present */
    if ( p )
    {
        /* 
        ** Ipv6 addresses may also have "::", so look for the last one. 
        */
        while(p1)
        {
            CMnext(p1);
            CMnext(p1);
            p1 = STstrindex(p1,"::", 0, FALSE);
            if (p1)
                p = p1;
        }
        	    
        /* host::port */
        STlcopy( buf, hostname, p - buf );
        CMnext(p);
        CMnext(p);
        if (CMdigit(p))
        {
            hints.ai_family         = AF_INET6;
            if (tcpip_version == 6)
                hints.ai_flags      = AI_ADDRCONFIG;
            else
                hints.ai_flags      = AI_ADDRCONFIG | AI_V4MAPPED | AI_ALL;
            hints.ai_socktype       = SOCK_STREAM;
            hints.ai_protocol       = IPPROTO_TCP;
            hints.ai_addrlen        = 0;
            hints.ai_addr           = NULL;
            hints.ai_canonname      = NULL;
            hints.ai_next           = NULL;

            status = getaddrinfo( hostname, p, &hints, res );
            if ( status )
            {
                TRdisplay("GC_tcp6_addr> getaddrinfo error: %s\n",
                    gai_strerror(status));
                return BS_NOHOST_ERR;
            }
        }
        else
            return BS_NOPORT_ERR;
    } 

  
    /*
    ** Plug in the byte-ordered port number from the supplied
    ** port string for each returned sockaddr_in6 structure.  
    ** Getaddrinfo() is not guaranteed to resolve the port
    ** argument when a numeric host argument is supplied.
    */
    if( CMdigit( p ) )
        CVan( p, &port );  /* Get port number if present */
    else 
        return BS_NOPORT_ERR;  /* No supplied port is an error */

    resAddr = *res;
    while(resAddr)
    {
        if (resAddr->ai_family == AF_INET6)
        {
            s_in6 = (struct sockaddr_in6 *) resAddr->ai_addr;
            s_in6->sin6_port = htons( (u_short)port );
        }
        resAddr = resAddr->ai_next;
    }

    /* no obvious errors */
    return OK;
}

