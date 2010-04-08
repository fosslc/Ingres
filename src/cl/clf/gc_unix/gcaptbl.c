/*
**Copyright (c) 2006 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <qu.h>
#include    <gc.h>
#include    <meprivate.h>

#include    <bsi.h>

#include    "gcacli.h"
#include    "gcarw.h"

/**
**
** Name: GCAPTBL.C - GCA CL protocol drivers
**
** Description:
**	This lists the protocol drivers (and their names) available
**	for local IPC.  Case is not important in the name.
**
**  History: 
**	4-Jul-90 (seiwald)
**	    Written.
**	30-Jan-92 (sweeney)
**	    Renamed xCL to xCL_TLI_TCP. 
**	27-Mar-92 (GordonW)
**	    Setup table for defaulting to correct driver. The default
**	    driver is the first entry. 
**		sockets/TCP only = default sockets/TCP
**		sockets/TCP. and TLI/TCP, TLI/TCP.
**	    For now add BS_tliunix entry which doesn't as yet
**	    exists but will.
**	28-Sep-92 (gautam)
**	    Correct xCL_066_UNIX_DOMAIN_EXIST to xCL_066_UNIX_DOMAIN_SOCKETS
**	14-Oct-92 (seiwald)
**	    Simplify selection of interface of TCP/UNIX driver by
**	    having multiple entries for each driver: first by its protocol
**	    name (e.g. "TCP_IP", "UNIX") then by a combo of its protocol
**	    and interface (e.g. "TLI_TCP_IP").
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      05-apr-95 (canor01)
**          add batchmode shared memory driver and reorder defaults
**          to put fastest drivers first
**      19-may-95 (chech02)
**          changed the default driver back to tcp-ip.
**      25-apr-95 (canor01)
**          add <meprivate.h> for definition of SYS_V_SHMEM
**      07-oct-2006 (lunbr01)  Sir 116548
**          Add new sockets TCP/IP protocol driver that supports both
**	    IPv4 and IPv6 and make it the default (protocol = "tcp_ip"),
**	    if it exists on the platform.  Provide "tcp_ipv4" as 
**	    backup for old sockets TCP IPv4-only protocol driver.
*/

/*
** Externals
*/

# ifdef xCL_019_TCPSOCKETS_EXIST
GLOBALREF 	    BS_DRIVER 	BS_socktcp;
# endif

# ifdef xCL_TCPSOCKETS_IPV6_EXIST
GLOBALREF 	    BS_DRIVER 	BS_socktcp6;
# endif

# ifdef xCL_TLI_TCP_EXISTS
GLOBALREF 	    BS_DRIVER 	BS_tlitcp;
# endif

# ifdef xCL_066_UNIX_DOMAIN_SOCKETS
GLOBALREF 	    BS_DRIVER 	BS_sockunix;
# endif

# ifdef xCL_TLI_UNIX_EXISTS
GLOBALREF 	    BS_DRIVER 	BS_tliunix;
# endif

# if defined(SYS_V_SHMEM) || defined(xCL_077_BSD_MMAP)
GLOBALREF           BS_DRIVER   BS_shm;
# endif

/*
** defined driver table
*/

GLOBALDEF GC_PCT GCdrivers[] = 
{

# ifdef xCL_TCPSOCKETS_IPV6_EXIST
	"TCP_IP", &BS_socktcp6,
	"SOCK_TCP_IP", &BS_socktcp6,
# endif

# ifdef xCL_019_TCPSOCKETS_EXIST
	"TCP_IP", &BS_socktcp,
	"SOCK_TCP_IP", &BS_socktcp,
	"TCP_IPV4", &BS_socktcp,
# endif

# ifdef xCL_TLI_TCP_EXISTS
	"TCP_IP", &BS_tlitcp,
	"TLI_TCP_IP", &BS_tlitcp,
# endif 

# ifdef xCL_066_UNIX_DOMAIN_SOCKETS
	"UNIX", &BS_sockunix,
	"SOCK_UNIX", &BS_sockunix,
# endif 

# ifdef xCL_TLI_UNIX_EXISTS
	"UNIX", &BS_tliunix,
	"TLI_UNIX", &BS_tliunix,
# endif

# if defined(SYS_V_SHMEM) || defined(xCL_077_BSD_MMAP)
	"SHM_BATCHMODE", &BS_shm,
	"BATCHMODE", &BS_shm,
# endif
/*
** Terminator...
*/

	0, 0
} ;
