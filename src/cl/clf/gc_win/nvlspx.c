/*
** Copyright (c) 1995, 2004 Ingres Corporation
**
** History:
**      12-Sep-95 (fanra01)
**          Extracted data for DLLs on Windows NT.
**	16-feb-98 (mcgem01)
**	    Read the port identifier from config.dat.  Use II_INSTALLATION
**	    only as a default.
**	23-Feb-1998 (thaal01)
**	    Allow space for string port_id
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	20-feb-2004 (somsa01)
**	    Updated to account for multiple listen ports per protocol.
**	13-may-2004 (somsa01)
**	    In GCnvlspx_init(), updated config.dat string used to retrieve
**	    port information such that we do not rely specifically on the GCC
**	    port.
*/

/*
**  Name: GCNVLSPX.C
**
**  Description:
**
**      Novell SPX specific functions for the winsock driver.
**	Contains:
**	    GCnvlspx_init()
**	    GCnvlspx_addr()
**	    GCnvlspx_port()
**  History: 
*/

# include	<compat.h>
# include 	<winsock.h>
# include 	<wsipx.h>
# include	<er.h>
# include	<cm.h>
# include	<cs.h>
# include	<me.h>
# include	<qu.h>
# include	<lo.h>
# include	<pm.h>
# include	<tr.h>
# include	<pc.h>
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

STATUS 		GCnvlspx_init(GCC_PCE * , GCC_WINSOCK_DRIVER *);
STATUS		GCnvlspx_addr( char *, char *, SOCKADDR_IPX *, char * );
STATUS		GCnvlspx_xlate_addr( char *, char *, SOCKADDR_IPX * );
bool		atoh( char *, WORD *);

/*
** Statics
*/
static          subport = 0;	/* number of failed listen attempts */

GLOBALREF	THREAD_EVENT_LIST IIGCc_proto_threads;

GLOBALREF	WS_DRIVER WS_nvlspx;
GLOBALREF i4 GCNVLSPX_trace;

/*
** Debug macro.
*/

# define GCTRACE(n) if ( GCNVLSPX_trace >= n ) (void)TRdisplay



/*
** Name: GCnvlspx_init
**
** Description:
**   NVLSPX inititialization function.  This routine is called from
**   GCwinsock_init() -- the routine GCC calls to initialize protocol 
**   drivers.
**
**   This function does initialization specific to the protocol:
**   Reads any protocol-specific env vars.
**   Sets up the winsock protocol-specific control info.
**
** History:
**	23-Feb-1998 ( thaal01)
**	    Allow space for port_id string, stops random crashing on startup.
**	13-may-2004 (somsa01)
**	    Updated config.dat string used to retrieve port information such
**	    that we do not rely specifically on the GCC port.
*/
STATUS
GCnvlspx_init(GCC_PCE * pptr, GCC_WINSOCK_DRIVER *wsd)
{
	char  	*ptr, *host, *server_id, *port_id;
	char	port_id_buf[8];
	char	config_string[256] ;

	port_id = port_id_buf ;

	/*
	** Get trace variable
	*/

	NMgtAt( "II_NVLSPX_TRACE", &ptr );
	if ( !(ptr && *ptr) && PMget("!.nvlspx_trace_level", &ptr) != OK )
	{
	    GCNVLSPX_trace = 0;
	}
	else
	{
	    GCNVLSPX_trace = atoi( ptr );
	}

	GCTRACE(3)( "GCnvlspx_init: Entry point.\n" );

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
        STprintf( config_string, ERx("!.nvlspx.port"),
                  SystemCfgPrefix, host, server_id);

        /*
        ** Search config.dat for a match on the string we just built.
        ** If we don't find it, then use the value for II_INSTALLATION
        ** failing that, default to II.
        */
        PMget( config_string, &port_id );
        if (port_id == NULL )
        {
                NMgtAt("II_INSTALLATION", &ptr);
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

	GCTRACE(1)("GCnvlspx_init: port = %s\n", pptr->pce_port );

	/*
	** Set protocol-specific info 
	*/

	wsd->addr_len = sizeof( SOCKADDR_IPX );
	wsd->sock_fam = AF_IPX;
	wsd->sock_type = SOCK_SEQPACKET;
	wsd->sock_proto = NSPROTO_SPX;
	wsd->block_mode = TRUE;
	wsd->pce_driver = (PTR)&WS_nvlspx;

	return OK;
}



/*
** Name: GCnvlspx_addr
**
** Description:
**   Takes a node and port name and translates them this into
**   a sockaddr structure suitable for appropriate winsock call.
**   returns OK or FAIL.  puts a character string representation
**   into port_name.
**
** History:
**	18-dec-1995 (canor01)
**	    Always use a well-known port (0x8265) on a listen, since
**	    that's what the client will use on a connect.
*/
STATUS
GCnvlspx_addr( char *node, char *port, SOCKADDR_IPX *ws_addr, char *port_name )
{

    if ( node == NULL )
    {
	int i;
        /*
	** local listen address.  This might need revisiting -- use the
	** subport stuff as in wintcp (?)
	*/

	/*
	** Don't try to translate the user-supplied port, use
	** the well-known one instead, since that's what the client does
	**
	** ws_addr->sa_socket = (unsigned short)htons( (unsigned short)atoi(port));
	**
	*/

	/*
	** put assigned socket address into addr.
	*/
	i = 0x8265;  /* This is a botch!!
		     ** due to net byte ordering this gets turned into
		     ** 6582.
		     */

	MEcopy (&i, 2, &ws_addr->sa_socket);

	STcopy( port, port_name );
    }
    else
    {
	if ( GCnvlspx_xlate_addr(node, port, ws_addr) != OK ) 
	    return FAIL;
    }
    ws_addr->sa_family = AF_IPX;
    return OK;
}

/*
** Name: GCnvlspx_xlate_addr
** Description:
**	Node is equivalent to SPX 12 byte node address, port is equivalent
**	to SPX 8 byte network no.  Munge these into the sockaddr
**	structure.
**	
**	As in PC implementation socket number is hardwired into what turns
**	out to be 6582.  blecchh.
** History:
**	01-dec-93 (edg)
**	    Original -- cribbed from dos/windows spx driver.
*/
STATUS    
GCnvlspx_xlate_addr (str1, str2, ipxaddr)
char *str1, *str2;
SOCKADDR_IPX *ipxaddr;
{
	char *p;
	int i, zeros_needed;
	WORD n;
	char tmp_str[13];

	/*
	** string1 is IPX node address 12 bytes.
	** Prepend 0's to the string if less than 12 bytes -- leading
	** 0's are not significant.
	*/

	zeros_needed = 12 - strlen( str1 );
	if ( zeros_needed < 0 )
	    return FAIL;   /* string too big */

	for ( i = 0, p = tmp_str; i < zeros_needed; i++ )
	    *p++ = '0';
        *p = (char)NULL;
	strcat( tmp_str, str1 );

	for (i=0, p=tmp_str; i<6; i++, p+=2) {
		if (!atoh (p, &n))
			return FAIL;
		ipxaddr->sa_nodenum[i] = (BYTE) (n);
	}

	/*
	** string2 is IPX network address 8 bytes
	*/

	zeros_needed = 8 - strlen( str2 );
	if ( zeros_needed < 0 )
	    return FAIL;   /* string too big */

	for ( i = 0, p = tmp_str; i < zeros_needed; i++ )
	    *p++ = '0';
        *p = (char)NULL;
	strcat( tmp_str, str2 );

	for (i=0, p=tmp_str; i<4; i++, p+=2) {
		if (!atoh (p, &n))
			return FAIL;
		ipxaddr->sa_netnum[i] = (BYTE) (n);
	}
	if (*p != '\0')
		return FAIL;
	/*
	** put assigned socket address into addr.
	*/
	i = 0x8265;  /* This is a botch!!
		     ** due to net byte ordering this gets turned into
		     ** 6582.
		     */

	MEcopy (&i, 2, &ipxaddr->sa_socket);
	
	return OK;
}

/* Name: atoh
**
** Description:
**	further munges spx addr for representation in SOCKADDR_IPX
** 
** History:
*/
bool
atoh (str, phex)
char *str;
WORD *phex;
{
	WORD n;
	char c;

	c = *str++;
	if (c >= '0' && c <= '9')
		n = (c-'0')<<4;
	else if (c >= 'A' && c <= 'F')
		n = (10 + c - 'A')<<4;
	else if (c >= 'a' && c <= 'f')
		n = (10 + c - 'a')<<4;
	else
		return (FALSE);
	c = *str;
	if (c >= '0' && c <= '9')
		n |= (c-'0');
	else if (c >= 'A' && c <= 'F')
		n |= (10 + c - 'A');
	else if (c >= 'a' && c <= 'f')
		n |= (10 + c - 'a');
	else
		return (FALSE);
	*phex = n;
	return (TRUE);
}
