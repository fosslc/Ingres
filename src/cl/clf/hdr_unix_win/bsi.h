/*
** Copyright (c) 2006 Ingres Corporation
*/

/**
** Name: BSI.H - interface to byte stream and message IPC routines
**
** Description:
**	This file describes the interface to the BS drivers.  Each
**	driver provides interprocess communication via some OS provided
**	byte stream or message service.  Byte stream services do not
**	preserve message boundaries; examples are TCP, FIFO's, streams,
**	etc.  Message services do preserve message boundaries; examples
**	are DECNET, OSI TP.
**
**	There is one driver for each interface to each IPC service.  
**	For example support for TCP via sockets, TCP via TLI, and UNIX 
**	domain sockets requires three drivers.
**
**	The BS drivers provide support to the UNIX GCA CL and GCC CL.  It
**	looks like this:
**
**	        GCA CL            GCC CL
**	              \          / | \
**	                \      /   |   \	<- bs interface
**	                  \  /     |     \
**			  BS1      BS2    BS3
**
**	The GCA CL calls a single configuration selectable BS driver, while
**	the GCC CL can support multiple BS drivers at once, to provide
**	support for multiple protocols.  GCA only makes intra-host
**	connections, while GCC makes inter-host connections.  BS drivers
**	written solely for GCA need not support functionality used only by
**	GCC, and vice versa.
**
**	Each BS driver can support multiple, simultaneous connections.
**
**	Each BS driver makes external only one symbol, a BS_DRIVER
**	structure which contains pointers to its dozen odd functions and
**	some configuration information.
**
**	BS driver functions are synchronous (i.e. they do not have a
**	callback interface), but each BS driver provides a
**	(*regfd)() function which will call iiCLfdreg to
**	register for a future synchronous driver function call.
**
**	Except where noted, BS driver functions take as their only argument
**	a pointer to a BS_PARMS structure, which contains all input and
**	output parameters.
**
**	Each BS driver that accepts incoming connections has a private data
**	area of size driver->lbcbsize, allocated by the caller and passed
**	in as parms->lbcb for the (*listen)(), (*accept)(), (*regfd),
**	and (*unlisten) function calls.  This area must be allocated before
**	calling (*listen)() and may be freed after calling (*unlisten)().
**	This area will be at most BS_MAX_LBCBSIZE bytes.
**	
**	Each active connection has a private data area of size
**	driver->bcbsize, allocated by the caller and passed in as
**	parms->bcb for the (*accept)(), (*request)(), (*connect)(),
**	(*send)(), (*receive)(), (*regfd)(), (*save)(), and (*restore)()
**	calls.  This area must be allocated before calling (*accept)() or
**	(*request)() and may be freed after calling (*close)().  This area
**	will be at most BS_MAX_BCBSIZE bytes.
**
#ifdef VMS
**	This section describes the way in which this module is used for
**	VMS protocol drivers.
**
**	The VMS BS driver functions (*accept)(), (*request)(),
**	(*send)(), (*receive)(), and (*close)() are asynchronous,
**	driving a callback routine when complete.  The other VMS BS
**	driver functions (*listen)(), (*unlisten)(), and (*portmap)()
**	are synchronous, returning when complete and driving no
**	callback routine.  
**
**	Because the asynchronous routines drive their callbacks, there
**	is no (*regfd)() or (*connect)() function.
**
**	Because this interface is used only for protocol drivers, there
**	is no (*save)(), (*restore)(), or (*detect)() function.
**
**	All error codes returned should be treated as FAIL, because BS
**	error messages are not available on VMS.
# endif
**
** History:
**	16-Apr-90 (seiwald)
**	    Written.
**	30-Jun-90 (seiwald)
**	    Private and Lprivate renamed to bcb and lbcb, as private
**	    is a reserved word somewhere.
**	6-Jul-90 (seiwald)
**	    New generic BS_SYSERR for formatting OS call errors.
**	5-Apr-91 (seiwald)
**	    Added VMS comments.
**	02-Mar-93 (brucek)
**	    Added async orderly release and made close async.
**	04-Mar-92 (edg)
**	    Removed detect from BS_DRIVER.
**	1-jun-94 (robf)
**          Added ext_info to BS_DRIVER, this allows extended information
**	    to be requested about a connection, this is currently used
**	    on secure ports to get info like security label. We add a new
**	    request to keep information about BS private inside BS, rather
**	    than having higher level code poke around inside BS information.
**	27-jul-1996 (canor01)
**	    Added BS_POLL_INVALID to force (*regfd)() to return FALSE,
**	    indicating a need to use blocking IO for use with operating-
**	    system threads.
**      28-jun-2000 (loera01) Bug 101800:
**          Cross-integrated from change 276901 in ingres63p:
**          Moved BCB struct to this file, so that the protocol driver can 
**          see the contents.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 6-Jun-03 (gordy)
**	    Added local/remote connection addresses to extended info.
**	 7-Oct-06 (lunbr01)  Sir 116548
**	    Added addrinfo pointers to BCB for IPv6 support.
**	02-Dec-09 (rajus01) Bug: 120552, SD issue:129268
**	    Added additional parameter to portmapper routine to return the
**	    actual symbolic port.
*/

/*
** Name: BS_DRIVER - BS driver entry points
**
** Description:
**	Each BS driver makes external only one symbol, a BS_DRIVER
**	structure which contains pointers to its dozen odd functions and
**	some configuration information.
**
**	Except where noted, all functions are VOID.
**
**	Except where noted, BS driver functions take as their only argument
**	a pointer to a BS_PARMS structure, which contains all input and
**	output parameters.
**
**	The functions which a BS driver must provide are given below.  Only
**	the per function parameters are described here: see BS_PARMS for
**	the generic ones.
**
**	    (*driver->listen)( &parms )
**		Open the driver and allow it to accept incoming
**		connections.  The requested listen port is represented
**		by the null terminated string parms->buf, which is then
**		pointed at a buffer containing the actual listen port.
**		This function initializes the contents of parms->lbcb 
**		and does not reference parms->bcb.
**
**	    (*driver->unlisten)( &parms )
**		Close the driver.
**	
**	    (*driver->accept)( &parms )
**		Accept an incoming connection.  The caller must first call
**		(*driver->regfd)() with the BS_POLL_ACCEPT flag to register
**		for this operation via iiCLfdreg().  This function
**		initializes the contents of parms->bcb.  If this 
**		function fails, client will call (*driver->close)().
**
**	    (*driver->request)( &parms )
**		Make outgoing connection request.  The target listen
**		address is represented by the null terminated string
**		parms->buf, whose syntax is either "port" for local
**		addresses or "host::port" for remote, off-host addresses.
**		This function initializes the contents of parms->bcb.
**		If this function fails, client will call (*driver->close)().
**
**	    (*driver->connect)( &parms )
**		Complete outgoing connection request.  The caller must
**		first call (*driver->regfd)() with the BS_POLL_CONNECT flag
**		to register for this operation via iiCLfdreg().  If this 
**		function fails, client will call (*driver->close)().
**
**	    (*driver->send)( &parms )
**		Send parms->len bytes starting at parms->buf.  The caller
**		must first call (*driver->regfd)() with the BS_POLL_SEND
**		flag to register for this operation via iiCLfdreg().  Short
**		sends are permitted:  parms->len and parms->buf will be
**		decremented and incremented, respectively, by the number of
**		bytes actually sent.
**
**	    (*driver->receive)( &parms )
**		Receive parms->len bytes starting at parms->buf.  The
**		caller must first call (*driver->regfd)() with the
**		BS_POLL_RECEIVE flag to register for this operation via
**		iiCLfdreg().  Short receives are permitted:  parms->len and
**		parms->buf will be decremented and incremented,
**		respectively, by the number of bytes actually received.
**
**		A driver for a message service will set parms->len to 0 
**		to indicate that the last or only segment of a message 
**		has been received.
**
**	    (*driver->release)( &parms )
**		Send orderly release.  The caller must first call 
**		(*driver->regfd)() with the BS_POLL_SNDREL flag to 
**		register for this operation via iiCLfdreg().  
**		
**	    (*driver->close)( &parms )
**		Close a connection.  The caller must first call 
**		(*driver->regfd) with the BS_POLL_RCVREL flag to register 
**		for this operation via iiCLfdreg().  parms->bcb may be 
**		subsequently freed by the caller.
**
**	    BOOL 
**	    (*driver->regfd)( &parms )
**		Register for an accept, connect, send, or receive by
**		calling iiCLfdreg() with the appropriate file descriptor,
**		mode, and the parameters parms->func, parms->closure, and
**		parms->timeout.  This function returns TRUE if the
**		operation was registered and the caller should wait for
**		callback by iiCLpoll() before continuing, or FALSE if the
**		caller may continue immediately.  Be aware that flow of
**		control passes to the iiCLpoll() event mechanism when this
**		function returns TRUE: the caller should not subsequently
**		access parms.
**
**	    (*driver->save)( &parms ) (GCA only)
**		Prepare the parms->bcb area for a GCA_SAVE operation.
**		The process will fork and the child will invoke 
**		(*driver->restore)(). 
**
**	    (*driver->restore)( &parms ) (GCA only)
**		Recover the parms->bcb area for a GCA_RESTORE operation.
**
**	    STATUS
**	    (*driver->portmap)( char *port_in, i4  subport, char *port_out,
**				char *port_out_symbolic )
**		(GCC only)
**		Map symbolic port address to an address suitable for
**		handing to (*driver->listen)() or (*driver->request)().
**		Port_in is the symbolic GCC listen port address (such as
**		that entered by the user with NETU), and port_out is the
**		result buffer.  Subport is a number (>=0) representing a
**		variation of port_in, and is used for programmatic
**		preturburation of port names.  Returns FAIL when subport is
**		out of range, OK otherwise.
**
**		For example, for TCP if port_in=XY, then
**		port_out=28872..28879 for subport 0..7.  Returns FAIL
**		on subport >7.  If port_in=20000, then port_out=20000
**		for subport 0.  Returns fail on subport >0.
**
**	    STATUS
**	    (*driver->ext_info)( &parms )
**	        Get extended information about a connection.
**	        buf should point to a structure of type BS_EXT_INFO.
**		
**
** History:
**	16-Apr-90 (seiwald)
**	    Written.
**	04-Mar-93 (edg)
**	    Removed detect.
**	1-jun-94 (robf)
**          Added ext_info.
*/

typedef struct _BS_DRIVER
{
	i4		bcbsize;	/* size of per connection data area */
	i4		lbcbsize;	/* size of per driver data area */
	VOID		(*listen)();	/* allow incoming connections */
	VOID		(*unlisten)();	/* disallow incoming connections */
	VOID		(*accept)();	/* accept an incoming connection */
	VOID		(*request)();	/* make outgoing connection request */
	VOID		(*connect)();	/* complete outgoing connection */
	VOID		(*send)();	/* send data */
	VOID		(*receive)();	/* receive data */
	VOID		(*release)();	/* send orderly release */
	VOID		(*close)();	/* close connection */
	bool		(*regfd)();	/* register for one of above funcs */
	VOID		(*save)();	/* prepare for GCA_SAVE */
	VOID		(*restore)();	/* reinit after GCA_RESTORE */
	STATUS		(*portmap)();	/* generate GCC port */
	STATUS		(*ext_info)();  /* get extended information */
} BS_DRIVER;

#define BS_MAX_BCBSIZE		32
#define BS_MAX_LBCBSIZE		256

/*
** Name: BS_PARMS - parameter structure for BS calls
**
** Description:
**	Most BS routines take a single argument, a pointer to a
**	BS_PARMS parameter block.  
**
**	The elements are:
**
**	    bcb		A pointer to a per-connection data area, of size 
**			driver->bcbsize, allocated by the caller.  This is
**			initialized by the (*accept)() or (*request)()
**			functions, and may be freed after calling
**			(*close)().  Referenced by the (*accept)(),
**			(*request)(), (*connect)(), (*send)(),
**			(*receive)(), (*regfd)(), (*save)(), and
**			(*restore)() calls.
**
**	    lbcb	A pointer to a per-driver data area, of size
**			driver->lbcbsize, allocated by the caller.  This
**			is initialized by the (*listen)() function, and
**			may be freed after calling (*unlisten)().
**			Referenced by the (*listen)(), (*accept)(),
**			(*regfd), and (*unlisten) function calls.
**
**	    buf, len	A buffer pointer and length: the use depends on 
**			the function.
**
**	    regop, func, closure, timeout
**			Parameters for the (*regfd)() function.
**
**	    status	The return status of the function.  Set to
**			OK on success.
**
**	    sys_err	Pointer to a CL_ERR_DESC which may be scribbled
**			upon if status != OK.
**
** History:
**	16-Apr-90 (seiwald)
**	    Written.
**	09-Sep-97 (rajus01)
**	    Added "is_remote" flag in BS_PARMS.
**	    Added IPADDR_COUNT, BYTES_IN_IPADDR, LOOPBACK_ADDRESS defines.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Prototype the callback.
*/

# include <pc.h>

#define IPADDR_COUNT            256
#define BYTES_IN_IPADDR         16
#define LOOPBACK_ADDRESS        127

typedef struct _BS_PARMS
{
	PTR		bcb;		/* per connection data area */
	PTR		lbcb;		/* per driver data area */
	char		*buf;		/* buffer for certain functions */
	i4		len;		/* length of buffer */
	char		regop;		/* for regfd */
	VOID		(*func)(void *, i4); /* for regfd */
	void		*closure;	/* for regfd */
	i4		timeout;	/* for regfd */
	STATUS		status;		/* return status */
	CL_ERR_DESC	*syserr;	/* modified if status != OK */
	PID		pid;		/* pid */
	bool		is_remote;	/* flag to identify remote or local */
} BS_PARMS;

/*
** Name: BS_EXT_INFO - Extended information request block
**
** Description:
**	This is a structure used by the ext_info call to return information
**	about a connection.
**
** History:
**	1-jun-94 (robf)
**	    Written.
**	 6-Jun-03 (gordy)
**	    Added local/remote connection addresses.
*/

typedef struct _BS_EXT_INFO
{
# define BS_EI_SEC_LABEL	0x0001	/* Security label */
# define BS_EI_USER_ID		0x0002	/* User id */
# define BS_EI_LCL_ADDR		0x0004	/* Local Address */
# define BS_EI_RMT_ADDR		0x0008	/* Remote Address */

   i4  info_request;	/* Requested information */

   i4  info_value;      /* Information provided */

   i4	len_sec_label;
   PTR	sec_label;	/* Pointer to connection security label */

   i4   len_user_id;
   char *user_id;

   i4	len_lcl_node;	/* Local network address */
   char	*lcl_node;
   i4	len_lcl_port;
   char	*lcl_port;

   i4	len_rmt_node;	/* Remote network address */
   char	*rmt_node;
   i4	len_rmt_port;
   char	*rmt_port;

} BS_EXT_INFO;

/*
** Name: BCB - connection control block
*/

typedef struct _BCB
{
	i4	fd;		/* I/O fd */
	int	optim;		/* for skipping select()s */
	PTR	aiList;		/* addrinfo list from getaddrinfo() */
	PTR	aiCurr;		/* current addrinfo entry in aiList */
} BCB;

# define BS_SKIP_R_SELECT	0x01	/* don't select before next read */
# define BS_SKIP_W_SELECT	0x02	/* don't select before next write */
# define BS_SOCKET_DEAD         0x04    /* I/O failed - don't try again */


/* values for regop */

# define BS_POLL_ACCEPT		0
# define BS_POLL_CONNECT 	1
# define BS_POLL_SEND		2
# define BS_POLL_RECEIVE	3
# define BS_POLL_SNDREL		4
# define BS_POLL_RCVREL		5
# define BS_POLL_INVALID	127

/* values for status */

#define	BS_SOCK_ERR	( E_CL_MASK | E_BS_MASK | 0x01 ) /* create failure */
#define	BS_BIND_ERR	( E_CL_MASK | E_BS_MASK | 0x02 ) /* bind failure */
#define	BS_LISTEN_ERR	( E_CL_MASK | E_BS_MASK | 0x03 ) /* listen failure */
#define	BS_ACCEPT_ERR	( E_CL_MASK | E_BS_MASK | 0x04 ) /* accept failure */
#define	BS_CONNECT_ERR	( E_CL_MASK | E_BS_MASK | 0x05 ) /* connect failure */
#define	BS_WRITE_ERR	( E_CL_MASK | E_BS_MASK | 0x06 ) /* write failure */
#define	BS_READ_ERR	( E_CL_MASK | E_BS_MASK | 0x07 ) /* read failure */
#define	BS_CLOSE_ERR	( E_CL_MASK | E_BS_MASK | 0x08 ) /* close failure */
#define	BS_REGISTER_ERR	( E_CL_MASK | E_BS_MASK | 0x09 ) /* register failure */
#define	BS_BADADDR_ERR	( E_CL_MASK | E_BS_MASK | 0x0A ) /* malformed address */
#define	BS_NOHOST_ERR	( E_CL_MASK | E_BS_MASK | 0x0B ) /* host unknown */
#define	BS_NOPORT_ERR	( E_CL_MASK | E_BS_MASK | 0x0C ) /* bad port spec */
#define	BS_INTERNAL_ERR	( E_CL_MASK | E_BS_MASK | 0x0D ) /* consistency check */
#define	BS_INCOMPLETE	( E_CL_MASK | E_BS_MASK | 0x0E ) /* must call again */
#define BS_SYSERR	( E_CL_MASK | E_BS_MASK | 0x0F ) /* %0c. */

