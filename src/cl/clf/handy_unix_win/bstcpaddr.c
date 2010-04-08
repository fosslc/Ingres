/*
**Copyright (c) 2006 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<cm.h>
# include	<er.h>
# include	<st.h>
# include	<lo.h>
# include	<me.h>
# include	<pm.h>

# include	<systypes.h>
# include	<clsocket.h>

# include	<bsi.h>

# if defined(POSIX_THREADS) && defined(any_hpux)
# include	<netdb.h>
# endif /* POSIX_THREADS && hpux */

# ifndef	GCF63
# include	<cs.h>
# endif

# ifdef xCL_019_TCPSOCKETS_EXIST

#ifndef	MAXHOSTNAME
#define	MAXHOSTNAME	64
#endif

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK          (u_long)0x7F000001
#endif


/*
** Name: BS_tcp_addr - translate symbolic host;;port to sockaddr_in struct
**
** Description:
**	Translates string representation of INET address (host;;port) to
**	a sockaddr_in struct.  
**
**	o	Accepts "dotted quad" notation (123.119.1.103) for host
**	o	Treats missing host;; as local host (INADDR_ANY)
**	o	Treats missing port as port 0.
**
** Inputs:
**	buf		string form of address
**	outbound	set if addr is for outbound connection
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
**	21-Jan-91 (seiwald)
**	    Fail if no port specified on active (outbound) address.
**	    Allow : as well as :: between host and addr.
**	12-Nov-91 (seiwald)
**	    Always include clsocket.h, since it should have the intelligence
**	    to include the right header networking files.
**	1-Nov-92 (seiwald)
**	    Always use INADDR_LOOPBACK for outbound loopback connections - we 
**	    were using INADDR_ANY, but that fails mysteriously on some TCPs.
**	05-Mar-93 (sweeney)
**	    Always define INADDR_LOOPBACK as 0x7f000001.
**	    Move the call to htonl to the point of use.
**	    (usl_us5 defines INADDR_LOOPBACK to be 0x7F000001, and
**	    was missing out on the htonl.)
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-aug-93 (ed)
**	    add missing include
**	21-Sep-93 (brucek)
**	    Cast args to MEcopy to PTRs.
**      12-Apr-95 (georgeb)
**          Added changes from ingres63p library, 27-oct-93 (ajc) :
**              Put in #ifdef xCL_019_TCPSOCKETS_EXIST to encompass the code
**              within this file otherwise platforms without tcp present
**              will fail to compile.
**	03-jun-1996 (canor01)
**	    Replace call to gethostbyname() with MT-safe call to
**	    iiCLgethostbyname().
**	14-may-1997 (muhpa01)
**	    With POSIX_THREADS under hp8_us5, ensure that the buffer passed to
**	    iiCLgethostbyname is large enough to hold hostent_data.  Also,
**	    include netdb.h for hp8_us5
**	25-may-1997 (kosma01)
**	    rs4_us5: Add a hostent_data structure to the hostbuf union.
**          The AIX gethostbyname_r functions uses a hostent_data structure.
**	10-oct-1997/09-apr-1997 (popri01)
**	    Changed h_errno (from previous canor01 change) to local_errno 
**	    due to conflict with Novell's netdb.h
**	30-apr-1998 (bobmart)
**	    Initialized third arg to gethostbyname_r, as stipulated; without,
**	    iigcc will crash on connect if system is not config'ed with DNS.
**	29-apr-1999 (hanch04)
**	    Removed define for inet_addr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**      15-Mar-1999 (hweho01)
**          Added support for AIX 64-bit (ris_u64) platform.
**	29-apr-1999 (hanch04)
**	    Removed define for inet_addr
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	21-May-2003 (bonro01)
**	    Add support for HP Itanium (i64_hpu)
**      05-Aug-2004 (ashco01)
**          Problem INGNET#128, bug#110725.
**          Increased MAXHOSTNAME to 64 characters to better accomodate 
**          longer fully qualified hostnames for target node. This is now
**          inline with GCA max hostname length of GC_L_NODE_ID in gcaint.h. 
**	22-Nov-2004 (wansh01)
**	    Problem INGNET#155, bug#113521	
**	    accept hostname with leading or all digit. 
**	 7-Oct-06 (lunbr01)  Sir 116548
**	    Change separator between host and port from colon (:)
**	    to semicolon (;) since colon is valid in IPv6 addresses.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

STATUS
BS_tcp_addr( buf, outbound, s )
char	*buf;
bool	outbound;
struct sockaddr_in *s;
{
	char	*p;
	char	hostname[ MAXHOSTNAME ];
	struct hostent *hp;
	struct hostent host;
	struct hostent *iiCLgethostbyname();
# if defined(i64_hpu)
	struct hostent hostbuf;
# elif defined(POSIX_THREADS) && defined(any_hpux)
	struct hostent_data hostbuf;
# else
	union
	{
# if defined(any_aix)
	    struct hostent_data h;   /* for alignment purposes only */
	    char   buf[sizeof(struct hostent_data)+512]; /* temporary buffer */
# else
	    struct hostent h;   /* for alignment purposes only */
	    char   buf[sizeof(struct hostent)+512]; /* temporary buffer */
# endif
	} hostbuf;
# endif
	int local_errno;

# if defined(POSIX_THREADS) && defined(any_hpux) && !defined(i64_hpu)
	hostbuf.current = NULL;
# endif /* POSIX_THREADS && hp8_us5 */

# if defined(any_aix)
	MEfill( sizeof(hostbuf), 0, &hostbuf );
# endif

	/* set defaults */

	s->sin_family = AF_INET;
	s->sin_addr.s_addr = outbound ? htonl(INADDR_LOOPBACK) : INADDR_ANY;
	s->sin_port = 0;

	/* parse address - look for a ";" */

	for( p = buf; *p; p++ )
	    if( p[0] == ';' )
		break;

	/* get hostname, if present */

	if( *p )
	{
		/* host;;port */

		STncpy( hostname, buf, p - buf );
		hostname[ p - buf ] = EOS;

		if( ( hp = iiCLgethostbyname( hostname,
					       &host,
					       &hostbuf,
					       sizeof(hostbuf),
					       &local_errno ) ) )
		    MEcopy(hp->h_addr, hp->h_length, (PTR)&s->sin_addr);
		else 
		    if( CMdigit( hostname ) )
		    {
			/* host id, e.g. 128.0.2.1 */

			if( !( s->sin_addr.s_addr = inet_addr( hostname ) ) )
				return BS_NOHOST_ERR;
		    }
		    else 
		    {
			return BS_NOHOST_ERR;
		    }
	}

	/* now get port, if present - allow an extra ; */

	if( p[0] == '\0' )
	    p = buf;
	else if( p[1] == ';' )
	    p = p + 2;
	else
	    p = p + 1;

	if( CMdigit( p ) )
	{
		/* 1234 port number */
		s->sin_port = htons( (u_short)atoi( p ) );
	}
	else if( *p || outbound )
	{
		return BS_NOPORT_ERR;
	}

	/* no obvious errors */

	return OK;
}
# endif /* xCL_019_TCPSOCKETS_EXIST */

# ifdef xCL_TCPSOCKETS_IPV6_EXIST
/*
** Name: BS_tcp_addrinfo - Get list of IP addrs for symbolic host;;port
**
** Description:
**	Parses string representation of INET address (host;;port) to
**	call getaddrinfo() function, which will return a list of 
**	matching IP addresses (IPv6 &/or IPv4) in addrinfo chained list
**	format.
**
**	o	Accepts any format for host.
**	o	Treats missing host;; as local host (INADDR_ANY)
**	o	Assumes port already converted to numeric string format.
**	o	Treats missing port as port 0.
**
** Inputs:
**	buf		string form of address
**	outbound	set if addr is for outbound connection
**	ip_family	IP address family (eg, AF_UNSPEC, AF_INET, AF_INET6)
**
** Outputs:
**	ai_list		pointer to addrinfo form of addresses in a chained list
**
** Returns:
**	BS_NOHOST_ERR	host unknown or malformed dotted quad
**	BS_NOPORT_ERR	port malformed
**	OK
**
** History:
**	 7-Oct-06 (lunbr01)  Sir 116548
**	    New function created and modeled after BS_tcp_addr(),
**	    which only supports IPv4 (AF_INET); new one supports both 
**	    IPv4 AND IPv6.  New function BS_tcp_addrinfo() populates 
**	    addrinfo rather than sockaddr_in structure.
**	 4-Jan-07 (lunbr01)  Bug 117251 + Sir 116548
**	    When port and/or hostname are pointers to empty string,
**	    replace with NULL pointer for call to getaddrinfo()...
**	    needed for base HP-UX 11iv1, possibly other OS's as well.  
**	    Also, for performance, only check II_TCPIP_VERSION 1st time in.
** 	 5-Mar-2007 (lunbr01)  Bug 117783 + Sir 116548
**	    Move logic to process II_TCPIP_VERSION variable to bssocktcp6.c.
**	    As a result, add ip_family as an input parm.
**	 6-Mar-2009 (hweho01) Bug 121737 and Bug 121686 
**          For Tru64 platform, SegV or malloc error occurred in function 
**          getaddrinfo if ai_family is AF_UNSPEC on 5.1B with patch kit 6.  
**          To work around the problem, make two calls to get the address 
**          infomations by specifying AF_INET6 first then AF_INET. After  
**          IPv6 and IPv4 addrinfo lists are retrieved, merge into one list. 
**          Also removed the change that set the second argument to NULL in  
**          getaddrinfo, it resulted in error return code EAI_NONAME (8). 
**	   
*/

STATUS
BS_tcp_addrinfo( buf, outbound, ip_family, aiList )
char	*buf;
bool	outbound;
int	ip_family;
struct addrinfo **aiList;
{
	char	*p;
	char	port_zero[] = "0";
	char	hostname_buf[ MAXHOSTNAME ] = {0};
	char	*hostname = hostname_buf;
	int	status;
	struct addrinfo hints;
#if defined(axp_osf)
        struct addrinfo *aiList4 = NULL;
        struct addrinfo *last_ipv6_record, *ptr;
        bool   no_ipv6_result = FALSE ;
        bool   no_ipv4_result = FALSE ;
#endif
char gai_buf[1024]; 
	/* set defaults */
	MEfill(sizeof(struct addrinfo), 0, &hints);
	hints.ai_family = ip_family;
	hints.ai_socktype = SOCK_STREAM;  
	hints.ai_protocol = IPPROTO_TCP;  
	hints.ai_flags    = outbound ? 0 : AI_PASSIVE;

	/* parse address - look for a ";" */

	for( p = buf; *p; p++ )
	    if( p[0] == ';' )
		break;

	/* get hostname, if present */

	if( *p )
	{
	    if( (p - buf) == 0 )
		hostname = NULL;
	    else
	    {
		/* host;;port */
		STncpy( hostname, buf, p - buf );
		hostname[ p - buf ] = EOS;
	    }
	}
	else
		hostname = NULL;

	/* now get port, if present - allow an extra ; */

	if( p[0] == '\0' )
	    p = buf;
	else if( p[1] == ';' )
	    p = p + 2;
	else
	    p = p + 1;

	if( CMdigit( p ) )
	{
		/* 1234 port number */
		if( !htons( (u_short)atoi( p ) ) && outbound)
		    return BS_NOPORT_ERR;
	}
	else if( *p || outbound )
	{
		return BS_NOPORT_ERR;
	}

	if( STlength( p ) == 0 )   /* Replace empty string for port with "0". */
	    p = port_zero;

	/* no obvious errors */

#if defined(axp_osf)
        if( outbound && (hostname != NULL) ) 
         { 
	   status = getaddrinfo( hostname, p, &hints, aiList);
	   if( status || !(*aiList))
	      return BS_NOHOST_ERR;
         } 
        else 
         { 
           /*
           ** SegV or malloc error occurred on 5.1B with patch kit 6 
           ** if ai_family is AF_UNSPEC in getaddrinfo call.  
           ** So make two calls (first IPv6,then IPv4), after the lists     
           ** are retrieved, merge them into one list. 
           */ 
	   hints.ai_family = AF_INET6;
	   status = getaddrinfo( NULL, p, &hints, aiList );
	   if( status || !( *aiList ))
	      no_ipv6_result = TRUE;    
	   hints.ai_family = AF_INET;
	   status = getaddrinfo( NULL, p, &hints, &aiList4);
	   if( status || !( aiList4 ))
	      no_ipv4_result = TRUE;    
	   if( no_ipv6_result && no_ipv4_result )
	      return BS_NOHOST_ERR;
           if( aiList4 )
            {
             if ( *aiList )   /* merge aiList and aiList4 if not NULL */  
               {
                 for(ptr=*aiList; ptr; ptr=ptr->ai_next)  
                   last_ipv6_record = ptr; 
                 last_ipv6_record->ai_next = aiList4; 
                }
             else  
               *aiList = aiList4; 
            }
         } 

#else  /* if defined(axp_osf) */

	status = getaddrinfo(outbound ? hostname : NULL, p, &hints, aiList);
	if( status || !(*aiList))
	    return BS_NOHOST_ERR;

#endif /* if defined(axp_osf) */ 

	return OK;
}
# endif /* xCL_TCPSOCKETS_IPV6_EXIST */
