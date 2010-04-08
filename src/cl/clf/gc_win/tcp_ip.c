/*
** Copyright (c) 2003, 2007 Ingres Corporation
*/
/*
**  Name: TCP_IP.C
**  Description:
**      TCP/IP specific functions for the Winsock 2.2 driver.
**	Contains:
**	    GCtcpip_init()
**	    GCtcpip_addr()
**	    GCtcpip_port()
**  History: 
**	29-sep-2003 (somsa01)
**	    Created based upon win_tcp.c.
**	09-feb-2004 (somsa01)
**	    In GCtcpip_init() and GCtcpip_addr(), if the port ID that is
**	    passed in seems to be a valid port number, use it as is. Also,
**	    renamed global subport variable so as to not clash with the one
**	    used in GCtcpip_port().
**	20-feb-2004 (somsa01)
**	    Updated to account for multiple listen ports per protocol.
**	13-may-2004 (somsa01)
**	    In GCtcpip_init(), updated config.dat string used to retrieve port
**	    information such that we do not rely specifically on the GCC port.
**	    Also, corrected function used to convert subport into a number.
**      26-Jan-2006 (Ralph Loen) Bug 115671
**          Added GCTCPIP_log_rem_host to optionally disable invocation of 
**          gethostbyaddr() in GCtcpip_init().
**      28-Aug-2006 (lunbr01) Sir 116548
**          Add IPv6 support.
**      06-Feb-2007 (Ralph Loen) SIR 117590
**          Removed II_TCPIP_log_rem_host, since getnameinfo() is no longer 
**          invoked for network listens.
**      03-June-08 (gordy and Ralph Loen)  SIR 120457
**          Implemented extended symbolic port range mapping algorithm.
**          Implemented support for explicit port rollup indicator for
**          symbolic and numeric ports.
*/

#include <winsock2.h>
#include <ws2tcpip.h>
#include <compat.h>
#include <er.h>
#include <cm.h>
#include <cs.h>
#include <cv.h>
#include <me.h>
#include <qu.h>
#include <pc.h>
#include <lo.h>
#include <pm.h>
#include <tr.h>
#include <nm.h>
#include <st.h>
#include <gcccl.h>
#include <gc.h>
#include "gclocal.h"
#include "gcclocal.h"

/*
**  Forward and/or External function references.
*/
STATUS 		GCtcpip_init(GCC_PCE * , GCC_WINSOCK_DRIVER *);
STATUS		GCtcpip_addr( char *, char *, char * );
STATUS		GCtcpip_port( char *, i4 , char * );

GLOBALREF	WS_DRIVER WS_tcpip;

/*
**  Statics
*/
static i4 sbprt = 0;

/*
**  Defines of other constants.
*/
GLOBALREF i4 GCTCPIP_trace;


#define GCTRACE(n) if (GCTCPIP_trace >= n) (void)TRdisplay


/*
**  Name: GCtcpip_init
**  Description:
**	TCP_IP inititialization function.  This routine is called from
**	GCwinsock2_init() -- the routine GCC calls to initialize protocol 
**	drivers.
**
**	This function does initialization specific to the protocol:
**	Reads any protocol-specific env vars.
**	Sets up the Winsock 2.2 protocol-specific control info.
**
**  History:
**	29-sep-2003 (somsa01)
**	    Created.
**	09-feb-2004 (somsa01)
**	    If the port ID that is passed in seems to be a valid port
**	    number, use it as is.
**	13-may-2004 (somsa01)
**	    Updated config.dat string used to retrieve port information such
**	    that we do not rely specifically on the GCC port. Also, corrected
**	    function used to convert subport into a number.
**      26-Jan-2006 (Ralph Loen) Bug 115671
**          Added GCTCPIP_log_rem_host to disable invocation of gethostbyaddr().
**      28-Aug-2006 (lunbr01) Sir 116548
**	    Add IPv6 support. Check II_TCPIP_VERSION and tcp_ip.version to
**	    see if new protocol restricted to IPv4 or IPv6 only.
**      22-Feb-2008 (rajus01) Bug 119987, SD issue 125582.
**          Bridge server configuration requires listening on a specified
**          three character listen address. During protocol initialization
**          the bridge server fails to start when three character listen
**          address is specified. For example,
**          the following configuration entries in config.dat
**             ii.<host>.gcb.*.tcp_ip.port: <xxn>,
**             ii.<host>.gcb.*.tcp_ip.status:<prot_stat>
**          are for command line configuration. When these entries are
**          present in addition to the CBF VNODE configuration (shown below )
**              ii.rajus01.gcb.*.tcp_ip.port.v1:<xxn>
**              ii.rajus01.gcb.*.tcp_ip.status.v1:<prot_stat>
**          the bridge server fails eventhough the port is available for use.
**          It has been found that the global 'sbprt' variable gets set
**          by the bridge server during protocol initialization to 'n' in the
**          three charater listen address 'xxn'. Later, while resolving the
**          three character portname into port number by GCtcpip_port routine
**          it assumes that this port is already in use even though it is not
**          the case.
**          Added server_type to identify the GCF server type.
**          The error messages from the errlog.log are the following:
**          rajus01 ::[R3\BRIDGE\464     , 1124      , ffffffff]: Tue Feb 19
**          15:16:13 2008 E_GC2808_NTWK_OPEN_FAIL  Network open failed for
**          protocol TCP_IP, port R3; status follows.
**          rajus01 ::[R3\BRIDGE\464     , 1124      , ffffffff]: Tue Feb 19
**          15:16:13 2008 E_CL2787_GC_OPEN_FAIL  An attempted network open
**          failed.
**	    Change description:
**		The code that clears the third character in the listen address
**		specified in the config.dat has been removed. This
**		appears to be a wrong assumption in the protocol driver based
**	        on the documentation in "Appendix A:TCP/IP protocol, Listen
**		Address Format seciton of Connectivity Guide". With these
**		changes the protocol driver will behave the way UNIX does.
**
**		WARNING: This DOES introduce the behavioural changes in the
**		following cases when starting one or more servers by increa-
**		sing the startup count in config.dat.
**		
**		Case 1: 
**		   Both tcp_ip and win_tcp status are set to ON with Listen
**		   Addresses II5 and II5 for example.
**		   New behaviour: The GCF server will come up and listen on 
**	           one protocol using port II5, but will fail on the other 
**		   protocol. 
**		   Original behaviour:
**		   The GCF server will listen on port II5 on the first
**		   protocol and the second one will listen on II6.
**		   This seems to be a bug in the driver as this is not the 
**		   behaviour documented in the connectivity guide. 
**		Case 2:
**		   Both tcp_ip and win_tcp status are set to ON with Listen
**		   Addresses (win_tcp=II, tcp_ip = II1).
**		   New Behaviour:
**		   First GCF server will come up fine. The second GCF server
**		   will fail for tcp_ip protocol, but will come up on win_tcp
**		   protocol. This doesn't seem to be much of an issue because
**		   the second GCF server will still come up using win_tcp.
**		   Original behaviour:
**		   First GCF server will come up OK (II0, II1). The second
**		   GCF server will come up fine too ( II2, II3 ).
*/

STATUS
GCtcpip_init(GCC_PCE * pptr, GCC_WINSOCK_DRIVER *wsd)
{
    char	*ptr;
    char	real_name_size[] = "100";
    char	*host, *server_id, *port_id;
    char	config_string[256];
    char	install_id[3];
    int		sock_fam, tcpip_version;

    /*
    **  Get trace variable
    */
    NMgtAt("II_TCP_TRACE", &ptr);
    if (!(ptr && *ptr) && PMget("!.tcp_trace_level", &ptr) != OK)
	GCTCPIP_trace = 0;
    else
	GCTCPIP_trace = atoi(ptr);

    /*
    **  Get set up for the PMget call.
    */
    PMinit();
    if (PMload(NULL, (PM_ERR_FUNC *)NULL) != OK)
	PCexit(FAIL);

    /*
    **  Construct the network port identifier.
    */
    host = PMhost();
    server_id = PMgetDefault(3);
    if (!server_id)
	server_id = "*" ;
    STprintf(config_string, ERx("!.tcp_ip.port"), SystemCfgPrefix,
	     host, server_id);

    /*
    **  Search config.dat for a match on the string we just built.
    **  If we don't find it, then use the value for II_INSTALLATION
    **  failing that, default to II.
    */
    if ((PMget(config_string, &port_id) != OK) || (port_id == NULL))
    {
	NMgtAt("II_INSTALLATION", &ptr);
	port_id = install_id; 
	if (ptr != NULL && *ptr != '\0')
	    STcopy(ptr, port_id);
	else
	    STcopy(SystemVarPrefix, port_id);
    }

    STcopy(port_id, pptr->pce_port);
    GCTRACE(1)("GCtcpip_init: port = %s\n", pptr->pce_port);

    /*
    **  Check to see if driver being restricted to IPv4 or IPv6 only.
    */
    NMgtAt( "II_TCPIP_VERSION", &ptr );
    if ( !(ptr && *ptr) && PMget("!.tcp_ip.version", &ptr) != OK )
	sock_fam = AF_UNSPEC;
    else if (STbcompare(ptr, 0, "ALL", 0, TRUE) == 0)
	sock_fam = AF_UNSPEC;
    else
    {
	tcpip_version = atoi(ptr);     /* 4=IPv4 or 6=IPv6 */
	if (tcpip_version == 4)
	    sock_fam = AF_INET;
	else if (tcpip_version == 6)
	    sock_fam = AF_INET6;
	else
	{
	    GCTRACE(1)("GCtcpip_init: Invalid tcp_ip.version = %s\n", ptr);
	    return(FAIL);
	}
    }

    /*
    **  Fill in protocol-specific info
    */
    wsd->addr_len = sizeof(struct sockaddr_in);
    wsd->sock_fam = sock_fam;     /* IPv4 or IPv6 or both*/
    wsd->sock_type = SOCK_STREAM;
    wsd->sock_proto = IPPROTO_TCP;
    wsd->block_mode = FALSE;
    wsd->pce_driver = (PTR)&WS_tcpip;

    return(OK);
}

/*
**  Name: GCtcpip_addr
**  Description:
**	Takes a node and port name and translates the port into
**	a value suitable for a getaddrinfo() call.
**	Returns OK or FAIL.  Puts a character string representation
**	of the numeric port into port_name.
**
**  History:
**	29-sep-2003 (somsa01)
**	    Created.
**	09-feb-2004 (somsa01)
**	    If the port ID that is passed in seems to be a valid port
**	    number, use it as is.
**	28-Aug-2006 (lunbr01) Sir 116548
**	    For IPv6 support, use addrinfo input parm instead of 
**	    sockaddr_in.  Remove unneeded code now done by MS
**	    Winsock2 routine getaddrinfo().
*/

STATUS
GCtcpip_addr(char *node, char *port, char *port_name)
{
    char *p = STindex(port, "+", 0);

    if (node == NULL)
    {
	/*
	** Local listen address, use subport if applicable.
	*/
	if (CMdigit(port) && p == NULL)
	    STcopy(port, port_name);
	else
	{
	    if (GCtcpip_port(port, sbprt, port_name) != OK)
		return(FAIL);

	    if (sbprt && p == NULL)
		STprintf(port, "%s%d", port, sbprt);
	    sbprt++;
	}
    }
    else
    {
        if (GCtcpip_port(port, 0, port_name) != OK)
	    return FAIL;
    }

    return(OK);
}


/*
** Name: GCtcpip_port - turn a tcp port name into a tcp port number
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
**  History
**      29-sep-2003 (somsa01)
**          Created.
**	 2-Apr-08 (gordy)
**	    Implemented extended symbolic port range mapping algorithm.
**	    Implemented support for explicit port rollup indicator for
**	    symbolic and numeric ports.
*/

STATUS
GCtcpip_port( pin, subport, pout )
char	*pin;
i4	subport;
char	*pout;
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
	return( OK );
    }

    /*
    ** Whatever we have at this point, port rollup is not
    ** allowed.  Input is simply passed through unchanged.
    */
    if( subport )  return( FAIL );
    STcopy( pin, pout );
    return( OK );
}
