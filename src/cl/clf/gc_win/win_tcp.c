/*
** Copyright (c) 1987, 2010 Ingres Corporation
*/
/*
**  Name: GCWINTCP.C
**  Description:
**      TCP/IP specific functions for the winsock driver.
**	Contains:
**	    GCwintcp_init()
**	    GCwintcp_addr()
**	    GCwintcp_port()
**  History: 
**	03-dec-93 (edg)
**	   Original.  pulled pieces out of former gcwintcp.c module since
**	   gcwinsck.c now handles most pof the real work.
**      12-Sep-95 (fanra01)
**          Extracted data for DLLs on Windows NT.
**	16-feb-98 (mcgem01)
**	    Read the port identifier from config.dat.  Use II_INSTALLATION
**	    as a last resort, rather than as the default.
**	23-Feb-1998 (thaal01)
**	    Allow space ( 8 bytes ) for string port_id.
**	27-feb-98 (mcgem01)
**	    Replace lines dropped by propagate script.
**	07-Jul-1998 (macbr01)
**	    Bug 91972 - jasgcc not receiving incoming communications. This is 
**	    due to incorrect usage of PMget() in function GCwintcp_init(). 
**	    Changed to test return code of PMget() instead of testing passed 
**	    in parameter for not equal to NULL.
**	15-jul-1998 (canor01)
**	    Move assignment of port_id to port_id_buf to prevent possible
**	    access violation. Clean up comments.
**	15-jul-1998 (canor01)
**	    macbr01's change uses "install" buffer instead of "port_id_buf",
**	    so remove port_id_buf.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	02-may-2000 (somsa01)
**	    Changed subport such that it is now passed in.
**	30-jun-2000 (somsa01)
**	    subport is back here again where it belongs.
**	09-feb-2004 (somsa01)
**	    In GCwintcp_init() and GCwintcp_addr(), if the port ID that is
**	    passed in seems to be a valid port number, use it as is. Also,
**	    renamed global subport variable so as to not clash with the one
**	    used in GCwintcp_port().
**	20-feb-2004 (somsa01)
**	    Updated to account for multiple listen ports per protocol.
**	13-may-2004 (somsa01)
**	    In GCwintcp_init(), updated config.dat string used to retrieve port
**	    information such that we do not rely specifically on the GCC port.
**	    Also, corrected function used to convert subport into a number.
**      26-Jan-06 (loera01) Bug 115671
**          Added GCWINTCP_log_rem_host to allow invocation of gethostbyaddr() 
**          to be disabled in GCwintcp_init().
**      06-Feb-2007 (Ralph Loen) SIR 117590
**          Removed II_TCPIP_LOG_REM_HOST, since gethostbyaddr() is no
**          longer invoked for network listens.
**      03-Jun-08 (gordy and Ralph Loen)  SIR 120457
**          Implemented extended symbolic port range mapping algorithm.
**          Implemented support for explicit port rollup indicator for
**          symbolic and numeric ports.
**	13-Apr-2010 (Bruce Lunsford)  SIR 122679
**	    Set wsd->pce_driver from GCC PCT rather than from ex-global
**	    WS_wintcp.
*/

# include	<compat.h>
# include 	<winsock.h>
# include	<er.h>
# include	<cm.h>
# include	<cs.h>
# include	<cv.h>
# include	<me.h>
# include	<qu.h>
# include	<pc.h>
# include	<lo.h>
# include	<pm.h>
# include	<tr.h>
# include	<nm.h>
# include	<st.h>
# include	<gcccl.h>
# include	<gc.h>
# include	"gclocal.h"
# include	"gcclocal.h"

#define GC_STACK_SIZE   0x8000

/*
* *  Forward and/or External function references.
*/

STATUS 		GCwintcp_init(GCC_PCE * , GCC_WINSOCK_DRIVER *);
STATUS		GCwintcp_addr( char *, char *, struct sockaddr_in *, char * );
STATUS		GCwintcp_port( char *, i4 , char * );

/*
** Statics
*/
static i4 sbprt = 0;

GLOBALREF	THREAD_EVENT_LIST IIGCc_proto_threads;

/*
**  Defines of other constants.
*/
GLOBALREF i4 GCWINTCP_trace;

# define GCTRACE(n) if ( GCWINTCP_trace >= n ) (void)TRdisplay


/*
** Name: GCwintcp_init
** Description:
**	WINTCP inititialization function.  This routine is called from
**	GCwinsock_init() -- the routine GCC calls to initialize protocol 
**	drivers.
**
**	This function does initialization specific to the protocol:
**	   Reads any protocol-specific env vars.
**	   Sets up the winsock protocol-specific control info.
** History:
**	05-Nov-93 (edg)
**	    created.
**	23-Feb-1998 (thaal01)
**	    Allow space for port_id, stops GCC crashing on startup sometimes.
**	07-Jul-1998 (macbr01)
**	    Bug 91972 - jasgcc not receiving incoming communications. This is 
**	    due to incorrect usage of PMget() in function GCwintcp_init(). 
**	    Changed to test return code of PMget() instead of testing passed 
**	    in parameter for not equal to NULL.
**	15-jul-1998 (canor01)
**	    Move assignment of port_id to port_id_buf to prevent possible
**	    access violation. Clean up comments.
**	09-feb-2004 (somsa01)
**	    When working with instance identifiers as port IDs, make sure
**	    we initialize sbprt with the trailing number, if set.
**	13-may-2004 (somsa01)
**	    Updated config.dat string used to retrieve port information such
**	    that we do not rely specifically on the GCC port. Also, corrected
**	    function used to convert subport into a number.
**      26-Jan-2006 (loera01) Bug 115671
**          Added GCWINTCP_log_rem_host to allow of gethostbyaddr() to be
**          disabled.
**      06-Feb-2007 (Ralph Loen) SIR 117590
**          Removed GCWINTCP_log_rem_host, since gethostbyaddr() is no
**          longer invoked for network listens.
**      22-Feb-2008 (rajus01) Bug 119987, SD issue 125582
**          Bridge server configuration requires listening on a specified
**          three character listen address. During protocol initialization
**          the bridge server fails to start when three character listen
**          address is specified. For example,
**          the following configuration entries in config.dat
**               ii.<host>.gcb.*.wintcp.port: <xxn>,
**               ii.<host>.gcb.*.wintcp.status:<prot_stat>
**          are for command line configuration. When these entries are
**          present in addition to the CBF VNODE configuration (shown below )
**                ii.rajus01.gcb.*.wintcp.port.v1:<xxn>
**                ii.rajus01.gcb.*.wintcp.status.v1:<prot_stat>
**          the bridge server fails even though the port is available for use.
**          It has been found that the global 'sbprt' variable gets set
**          by the bridge server during protocol initialization to 'n' in the
**          three charater listen address 'xxn'. Later, while resolving the
**          three character portname into port number by GCwintcp_port routine
**          it assumes that this port is already in use even though it is not
**          the case.
**	    Added server_type to determine the GCF server type.
**          The error messages from errlog.log are the following:
**          rajus01 ::[R3\BRIDGE\12c4    , 4804      , ffffffff]: Tue Feb 19
**          19:49:27 2008 E_GC2808_NTWK_OPEN_FAIL  Network open failed for
**          protocol TCP_IP, port R3; status follows.
**          rajus01 ::[R3\BRIDGE\12c4    , 4804      , ffffffff]: Tue Feb 19
**          19:49:27 2008 E_CL2787_GC_OPEN_FAIL    An attempted network open
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
**		   Original behaviour:
**		   First GCF server will come up OK (II0, II1). The second
**		   GCF server will come up fine too ( II2, II3 ).
**		   New Behaviour:
**		   First GCF server will come up fine. The second GCF server
**		   will fail for tcp_ip protocol, but will come up on win_tcp
**		   protocol. This doesn't seem to be much of an issue because
**		   the second GCF server will still come up using win_tcp. 
**	13-Apr-2010 (Bruce Lunsford)  SIR 122679
**	    Set wsd->pce_driver from GCC PCT rather than from ex-global
**	    WS_wintcp.
*/
STATUS
GCwintcp_init(GCC_PCE * pptr, GCC_WINSOCK_DRIVER *wsd)
{

	char           *ptr = NULL;
	char            real_name_size[] = "100";
	char           *host, *server_id, *port_id;
	char            config_string[256];
	char            install[32]; //II_INSTALLATION code

        /*
        ** Get set up for the PMget call.
        */
        PMinit();
        if( PMload( NULL, (PM_ERR_FUNC *)NULL ) != OK )
                PCexit( FAIL );

        /*
        ** Construct the network port identifier.
        */

        host = PMhost();
        server_id = PMgetDefault(3);
        if (!server_id)
                server_id = "*" ;
        STprintf( config_string, ERx("!.wintcp.port"),
                  SystemCfgPrefix, host, server_id);

        /*
        ** Search config.dat for a match on the string we just built.
        ** If we don't find it, then use the value for II_INSTALLATION
        ** failing that, default to II.
        */
        if ((PMget( config_string, &port_id ) != OK) ||
            (port_id == NULL ))
        {
                NMgtAt("II_INSTALLATION", &ptr);
		port_id = install; 
                if (ptr != NULL && *ptr != '\0')
                {
                        STcopy(ptr, port_id);
                }
                else
                {
                        STcopy(SystemVarPrefix, port_id);
                }
        }

	STcopy(port_id, pptr->pce_port);
	GCTRACE(1)("GCwintcp_init: port = %s\n", pptr->pce_port );


	/*
	** Fill in protocol-specific info
	*/
	wsd->addr_len = sizeof( struct sockaddr_in );
	wsd->sock_fam = AF_INET;
	wsd->sock_type = SOCK_STREAM;
	wsd->sock_proto = 0;
	wsd->block_mode = FALSE;
	wsd->pce_driver = pptr->pce_driver;

	/*
	** Get trace variable
	*/
        ptr = NULL;
	NMgtAt( "II_WINTCP_TRACE", &ptr );
	if ( !(ptr && *ptr) && PMget("!.wintcp_trace_level", &ptr) != OK )
	{
	    GCWINTCP_trace = 0;
	}
	else
	{
	    GCWINTCP_trace = atoi( ptr );
	}

	return OK;
}



/*
** Name: GCwintcp_addr
** Description:
**	Takes a node and port name and translates them this into
**	a sockaddr structure suitable for appropriate winsock call.
**	returns OK or FAIL.  puts a character string representation
**	into port_name.
** History:
**	03-dec-93 (edg)
**	    Original.
**	30-jun-2000 (somsa01)
**	    Update subport number after success.
**	30-aug-2001 (rodjo04) Bug 105650
**	    The machine name may have leading digits. We must see if this is
**	    a valid name before we can assume that this is an IP address. 
**	09-feb-2004 (somsa01)
**	    If the port ID that is passed in seems to be a valid port
**	    number, use it as is.
*/
STATUS
GCwintcp_addr( char *node, char *port, struct sockaddr_in *ws_addr, char *port_name )
{
    struct hostent *host_ent;
    char *p = STindex(port, "+", 0);
   
    if ( node == NULL )
    {
        /*
	** local listen address, use INADDR_ANY and subport if applicable.
	*/
	if (CMdigit(port) && p == NULL)
	    STcopy(port, port_name);
	else
	{
	    if ( GCwintcp_port(port, sbprt, port_name) != OK )
		return FAIL;

	    if (sbprt && p == NULL)
		STprintf(port, "%s%d", port, sbprt);
	    sbprt++;
	}

	ws_addr->sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
       
	/* check for host id, e.g. 128.0.2.1 */
	if (CMdigit(node)) 
	{
		/*  Host name might have leading digits or could be the
		**  IP address. Call gethostbyname() anyway. If it is a name,
		**  we will get a pointer to the structure. If its an IP (in 
		**  IP format) then a structure will be returned even if the 
		**  IP is not 'alive'. If the IP is not alive, it will fail 
		**  later. 
		*/

		if (host_ent = gethostbyname(node))
			ws_addr->sin_addr.s_addr = *(u_long *) host_ent->h_addr;
		else
		{
			if (!(ws_addr->sin_addr.s_addr = inet_addr(node))) 
			 {
			    return FAIL;
			 }
		}
	} 
	else 
	{
	    if (!(host_ent = gethostbyname(node))) 
	    {
	        return FAIL;
	    }
	    ws_addr->sin_addr.s_addr = *(u_long *) host_ent->h_addr;
	}

        if ( GCwintcp_port(port, 0, port_name) != OK )
	    return FAIL;
    }

    ws_addr->sin_port = (u_short)htons( (u_short)atoi(port_name));
    ws_addr->sin_family = AF_INET;

    return OK;
}


/*
** Name: GCwintcp_port - turn a tcp port name into a tcp port number
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
** History
**      23-jun-89 (seiwald)
**          Written.
**      28-jun-89 (seiwald)
**          Use wild CM macros instead of the sane but defunct CH ones.
**      2-feb-90 (seiwald)
**          New subport parameter, so we can programmatically generate
**          successive listen ports to use on listen failure.
**      24-May-90 (seiwald)
**          Renamed from GC_tcp_port.
**      03-dec-93 (edg)
**          This is pretty much a straight copy of BS_tcp_port.
**       2-Apr-08 (gordy)
**          Implemented extended symbolic port range mapping algorithm.
**          Implemented support for explicit port rollup indicator for
**          symbolic and numeric ports.
**          
*/

STATUS
GCwintcp_port( pin, subport, pout )
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
