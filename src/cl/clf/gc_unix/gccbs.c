/*
**Copyright (c) 2006 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<er.h>
#include	<nm.h>
#include	<gcatrace.h>
#include	<gcccl.h>
#include	<bsi.h>
#include	<st.h>
#include	<me.h>
#include	<lo.h>
#include	<pm.h>
#include	<errno.h>


/*{
** Name: GCpdrvr    - Protocol driver routine
**
** Description:
**
**	[These few paragraphs brought to you by the original authors.]
**
**	GCpdrvr is the model for the protocol driver routines which interface
**	to the local system facilities through which access to specific
**	protocols is offered.  In a given environment there may be multiple
**	protocol drivers, depending on the number of separate protocols which
**	are to be supported and the specific implementation of protocol drivers
**	in the environment.  This is an implementation issue, and is dealt with
**	by appropriate construction of the Protocol Control Table (PCT) by the
**	initialization routine GCpinit().
**
**	Protocol driver routines are not called directly by the CL client, but
**	rather are invoked by pointer reference through the Protocol Control
**	Table.  This is therefore a model specification for all protocol
**	drivers, whose names are not actually known outside the CL.  They are
**	known externally only by an identifier in the PCT.
**	
**	All protocol drivers have an indentical interface, through which a set
**	of service functions is offered to clients.  Two parameters are passed
**	by the caller: a function identifier and a parameter list pointer.  The
**	function ID specifies which of the functions in the repertoire is being
**	requested.  The parm list supplies the parameters required for
**	execution of the function.  It contains both input and output
**	parameters.  It has a constant section, and a section containing
**	parameters specific to the requested function.  The interface for all
**	routines is specified here instead of with each routine, as is
**	customary, since it is identical for all.
**
**	It is the responsibility of a single protocol driver to map the service
**	interface for a particular protocol, as provided by a specific
**	environment, to the model interface specified here.  In particular, this
**	includes the management of asynchronous events in the specified way.
**	The protocol driver interface is asynchronous in the sense that when a
**	function is invoked by the client, control is returned prior to
**	completion of the function.  Notification of completion is provided
**	to the client by means of a callback routine provided by the client as a
**	service request parameter.  This routine is to be invoked when the
**	requested service is complete.
**
**	It is to be noted that execution control is guaranteed to pass to the
**	CL routine GCc_exec, and remain there for the duration of server
**	execution.  It is possible for protocol driver service requests to be
**	invoked prior to the time GCc_exec receives control.  It is not
**	required that these service requests complete until control passes to
**	GCc_exec.  This enables the detection and allowance of external
**	asynchronous events to reside in a place to which control is guaranteed
**	to pass subsequent to a service request, if this is appropriate in a
**	particular environment.  This is not required, and if the environment
**	permits, the entire mechanism of dealing with external asynchronous
**	events may be implemented in the protocol driver.  It is also to be
**	noted that when a CL client's callback routine is invoked, it is
**	guaranteed that control will be returned from that invocation when the
**	client's processing is complete, and that the client will not wait or
**	block on some external event.
**
**
** Inputs:
**      Function_code                   Specifies the requested function
**          GCC_OPEN        ||
**	    GCC_CONNECT	    ||
**	    GCC_DISCONNECT  ||
**	    GCC_SEND	    ||
**	    GCC_RECEIVE	    ||
**	    GCC_LISTEN
**      parameter_list                  A pointer to a set of parameters
**					qualifying the requested function.
**					See gcccl.h for description.
**
** Outputs:
**      Contained in the parameter list, described above.
**
**	Returns:
**	    STATUS:
**		OK
**		FAIL
**
** History:
**    05-Jan-90 (seiwald)
**	Revamped and made sane.  Split into GCbs, the entry point (from
**	the TL) which takes two parameters, and GCbssm, the entry point
**	(from BS) which takes one parameter.  Initial register occurs at
**	GCC_OPEN time.  Buffering of reads is turned on.
**    02-Feb-90 (seiwald)
**	Support for listen port hunting: GC_REGISTER iterative bumps
**	the "subport" counter and calls BSregister until BSregister
**	succeeds or GC_tcp_lport indicates the listen ports have been
**	exhausted.   The maximum subport for for unqualified symbolic
**	listen ports (i.e. XY) is 7, otherwise 0 (no hunting).
**    08-Feb-90 (seiwald)
**	Assume that GCC_DISCONNECT will follow a failing GCC_LISTEN
**	or GCC_CONNECT, so don't free resources in GC_CONNCMP.
**	Pass listen port back up on GCC_OPEN completion, so that
**	the TL can log it.
**    21-Feb-90 (cmorris)
**	Added real-time tracing.
**    01-Mar-90 (seiwald)
**	Built in read buffering.  This removes reliance upon BS's buffering,
**	which will probably not be available in non-UNIX environs.
**    25-May-90 (seiwald)
**	Taken from gcctcp.c and made into the generic BS protocol driver
**	on the new interface.  Removed history prior to 05-Jan-90.
**    19-Jun-90 (seiwald)
**	Made output of the listen port fancier.
**    20-Jun-90 (seiwald)
**	Put per-driver data into the new pce->pce_dcb.  It was formerly
**	static, but that does work too well with multiple drivers.
**    30-Jun-90 (seiwald)
**	Private and Lprivate renamed to bcb and lbcb, as private
**	is a reserved word somewhere.
**    28-Aug-90 (seiwald)
**	Added (as yet untested) support for message protocols with
**	the GCbk() protocol driver.
**    6-Sep-90 (seiwald)
**	Use bsp.len == 0 to check for end of message when receiving
**	over message (GCbk()) services.
**    18-Jan-91 (seiwald)
**	A little recreational reformatting.
**    31-Jan-91 (cmorris)
**  	Moved in tracing initialization.
**    13-Feb-91 (cmorris)
**  	Added support for "NO_PORT" to register function.
**    14-Feb-91 (cmorris)
**  	Made gc_names static to avoid clashes with other drivers.
**    23-Apr-91 (seiwald)
**      Replaced enums with #defines, 'cause they're bad.
**    12-Feb-92 (sweeney)
**	Reset default listen port to the one we actually try since
**	we're going to log it if we fail.
**    17-Dec-92 (gautam)
**	Use PM calls to get the listen port.
**    11-Jan-93 (edg)
**	Use gcu_get_tracesym() to get trace level.
**    18-Jan-93 (edg)
**	Replace gcu_get_tracesym() with inline code.
**    28-Jan-93 (edg)
**	Fixed GCbslport -- should try to use installation code as default
**	port if no PM symbol is set before returning NULL.
**    18-Feb-93 (brucek)
**	Added async orderly release and made close async.
**    15-jul-93 (ed)
**	adding <gl.h> after <compat.h>
**    11-aug-93 (ed)
**      add missing include, remove conflicting externs
**    07-oct-1993 (tad)
**	Bug #56449
**	Changed %x to %p for opinter values in GCbs(), GCbk(),
**	GCbssm().
**    12-nov-93 (robf)
**      In GC_LSNCMP initialize bsp.buf so the accept driver routine
**	can fill  in the remote node name, if available.
**	19-apr-95 (canor01)
**	    added <errno.h>
**	06-jun-95 (canor01)
**	    semaphore protect memory allocation routines
**	01-nov-1995 (canor01)
**	    Increase size of GC_PCB buffer to maximum size as
**	    allowed in gcc.h
**	12-Feb-1996 (rajus01)
**	    Added support for Protocol Bridge. Bridge doest not
**	    require "II" value for listen port.
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**	 2-Oct-98 (gordy)
**	    Cleaned up the handling of listen ports.  The symbolic port
**	    ID is placed back into the PCE while the actual port number
**	    is returned in the listen parms.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**    23-Jun-00 (loera01) Bug 101800 
**	    Cross-integrated from 276902 in ingres63p:
**          Protect against dead sockets, just as the sockets for local IPC do.
**    23-Mar-01 (loera01) Bug 104337
**          The fix for bug 101800 should have included setting an error status
**          and calling back GCA, instead of simply returning.  Otherwise 
**          GCA will not clean up the session. 
**	16-mar-2001 (somsa01)
**	    Modified GCbssm() to properly set function_parms.listen.node_id .
**     18-Mar-2002 (hweho01)
**          Turned off compiler optimization for rs4_us5, avoid the network
**          connection error. Symptom: after iigcc has been started for
**          15-20 minutes, segmentation violation occurred when there is
**          connect attempt from remote server. (B107517)
**	20-Nov-02 (wansh01)
**	    In GCbssm(), set parms->options to indicate remote/local
**	    connection. 
**	 6-Jun-03 (gordy)
**	    Removed PM check for listen port.  This is callers responsibility.
**	    Remainder of GCbslport() moved inline.  Return local/remote
**	    addresses for LISTEN/CONNECT requests.
**      24-Jul-2003 (hweho01)
**          Added r64_us5 to NO_OPTIM list, avoid network connection error.
**	22-Dec-03 (gordy)
**	    The register loop which searches for available sub-ports also
**	    results in subsequent port addresses to be ignored.  Separated
**	    GC_REG into an initialization state (GC_REG) and the sub-port
**	    loop (GC_REG1).
**	15-Jun-2004 (schka24)
**	    Safe handling of env vars.
**	23-Nov-2004 (hweho01)
**	    Removed rs4_us5 and r64_us5 from NO_OPTM list.
**	6-Oct-2005 (wansh01) 
**	    INGNET 184 bug 115355 gcd server memory leak 	
**	    during GC_DISCONN action added GC_abort() to look and fail  
**	    outstanding send/recv request.   
**   	    Added active, parms fields in GC_PCB 
**	 7-Oct-06 (lunbr01)  Sir 116548
**	    Change separator between host and port from colon (:)
**	    to semicolon (;) since colon is valid in IPv6 addresses.
**	    Also provide for resuming connect processing with next
**	    address after a connect fails on one address.
**	05-Oct-07 (gordy & rajus01) Bug # 119256; SD issues: 121963, 119093 
**	    Related old Startrak issue: 13872724.
**	    The QA net tests specifically gaa04 tests crashed Bridge Server.
**	    Inspection of the corrupted memory address revealed that the 
**	    memory corruption occured while in GC_RCV4 state doing MEcopy()
**	    to copy excess data to pcb->buffer. The length of the excess data 
**	    to be copied exceeded size of pcb->buffer (~8K) defined by the 
**	    protocol driver causing the buffer overrun. 
**	    This behaviour in GCB is identified to be the side effect of the 
**	    change 466477 (Bug# 111652) to allow Ingres/Net to support 32K 
**	    message buffers. The problem is that the Bridge Server sits in 
**	    between GCCs and simply reads blocks of data from one port and 
**	    write it to another port without looking at the contents of the 
**	    data blocks and there is no packet size negotiation happening 
**	    during connection initiation. This is set to 4K in protocol driver
**	    table and as the GCCs do the packet size negotiation it was not a
**	    problem for GCCs whereas in the case of GCB the pcb->rcv.len 
**	    always resulted in 32K and the excess data to be copied into 
**	    the overflow buffer exceeded 8K in situations where more
**	    data than current packet was received thus resulting 
**	    in buffer overrun, corrupting the memory control blocks, lead
**	    to crashing in GCB. 
**	    Code changes:
**	    A new variable called buf_len is added to protocol driver control 
**	    block ( GC_PCB). This is actual length of the overflow buffer. 
**	    The buf_len is calculated for each new incoming/outgoing 
**	    connections. While reading network data, the length of data 
**	    (pcb->recv.len) that can be read is now restricted to the minimum 
**	    of size of the buffer provided by the GC caller and the 
**	    buf_len. i.e the reads are restricted to the size of the overflow 
**	    buffer supported by the protocol driver to avoid buffer overruns.
**	    This means multiple reads will be required to receive requests 
**	    larger than the declared limit.
**	06-Oct-09 (rajus01) Bug: 120552, SD issue:129268 
**	    When starting more than one GCC server the symbolic ports are not
**	    incremented (Example: II1, II2) in the startup messages, while the 
**	    actual portnumber is increasing and printed correctly.
**	15-nov-2010 (stephenb)
**	    Correctly proto all functions.
**
*/

/*
** Forward functions
*/
static	VOID	GCbssm(GCC_P_PLIST *);

/*
** local variables
*/

/* Per driver control block */

typedef struct _GC_DCB
{
	i4		subport;	/* for listen attempts */
	STATUS		last_status;
	char		lsn_port[32];	/* untranslated listen port */
	char		lsn_bcb[ BS_MAX_LBCBSIZE ];
    	char		portbuf[ GCC_L_NODE + GCC_L_PORT + 5 ];
	/*
	** The "actual_lsn_port" field is added to return the 
	** actual symbolic port equivalent to "lsn_port" field. 
	*/ 
    	char		actual_lsn_port[ GCC_L_PORT + 1 ];  /* for logging */

} GC_DCB;

/* Per connection control block */

typedef struct _GC_PCB
{
	struct	{
		char	*buf;
		i4	len;
		bool    active; 
		GCC_P_PLIST *parms; 
	} send, recv;

	char		*b1;		/* start of read buffer */
	char		*b2;		/* end of read buffer */
	char		bcb[ BS_MAX_BCBSIZE ];

	/*
	** Determine I/O buffer size.  For GCbs() there must be room
	** for the max block size plus two bytes for the block length.
	** For GCbk() there only needs to be room for to hold output
	** parameters for the various protocol driver requests.  Note
	** that overhead bytes are provided in the buffer declaration
	** while the base buffer sized is contained in the macros.
	*/
# define	BS_PCB_BUFF_SZ	8192
# define	BK_PCB_BUFF_SZ	(GCC_L_NODE + sizeof(GCC_ADDR) * 2)

	u_i2		buf_len;	/* Length of overflow buffer */
	char		buffer[ 2 ];	/* Variable length (extra 2 bytes) */

} GC_PCB;

/*
** Forward references
*/
static i4  GCbs_trace = 0;
static void GC_abort( GC_PCB *);
STATUS	GCbk( i4, GCC_P_PLIST * );
STATUS  GCbs( i4, GCC_P_PLIST * );

# define GCTRACE(n) if( GCbs_trace >= n )(void)TRdisplay

/* Actions in GCbssm. */

# define GC_REG		0	/* initialize register request */
# define GC_REG1	1	/* open listen port */
# define GC_LSN		2	/* wait for inbound connection */
# define GC_LSNBK	3	/* wait for inbound connection (on block conn)*/
# define GC_LSNCMP	4	/* LISTEN completes */
# define GC_CONN	5	/* make outbound connection */
# define GC_CONNBK	6	/* make outbound connection (on block conn) */
# define GC_CONNCMP	7	/* CONNECT completes */
# define GC_SND		8	/* send TPDU */
# define GC_SNDBK	9	/* send TPDU (on block conn) */
# define GC_SNDCMP	10	/* SEND completes */
# define GC_RCV		11	/* receive TPDU */
# define GC_RCV1	12	/* received NDU header */
# define GC_RCV2	13	/* received NDU header */
# define GC_RCV3	14	/* RECEIVE completes */
# define GC_RCV4	15	/* RECEIVE completes */
# define GC_RCVBK	16	/* receive TPDU over (on block conn) */
# define GC_RCV1BK	17	/* RECEIVE completes */
# define GC_DISCONN	18	/* close a connection */
# define GC_DISCONN1	19	/* send orderly release */
# define GC_DISCONN2	20	/* poll for close */
# define GC_DISCONN3	21	/* close complete */

/* Action names for tracing. */

static char *gc_names[] = { 
	"REG", "REG1", "LSN", "LSNBK", "LSNCMP", "CONN", "CONNBK", "CONNCMP",
	"SND", "SNDBK", "SNDCMP", "RCV", "RCV1", "RCV2", "RCV3", "RCV4", 
	"RCVBK"," RCV1BK", "DISCONN", "DISCONN1", "DISCONN2", "DISCONN3"
} ;

STATUS
GCbs( func_code, parms )
i4		    func_code;
GCC_P_PLIST	    *parms;
{
    /*
    ** Compute initial state based on function code.
    */

    switch( func_code )
    {
    case GCC_OPEN: 	parms->state = GC_REG; break;
    case GCC_LISTEN: 	parms->state = GC_LSN; break;
    case GCC_CONNECT: 	parms->state = GC_CONN; break;
    case GCC_SEND: 	parms->state = GC_SND; break;
    case GCC_RECEIVE: 	parms->state = GC_RCV; break;
    case GCC_DISCONNECT:parms->state = GC_DISCONN; break;
    }

    GCX_TRACER_1_MACRO("GCbs>", parms,
		"function code = %d", func_code);

    GCTRACE(1)("GC%s: %p operation %s\n", 
	parms->pce->pce_pid, parms, gc_names[ parms->state ] );

    parms->generic_status = OK;
    CL_CLEAR_ERR( &parms->system_status );

    /* Start sm. */

    GCbssm( parms );
    return OK;
}

STATUS
GCbk( func_code, parms )
i4		    func_code;
GCC_P_PLIST	    *parms;
{
    /*
    ** Compute initial state based on function code.
    */

    switch( func_code )
    {
    case GCC_OPEN: 	parms->state = GC_REG; break;
    case GCC_LISTEN: 	parms->state = GC_LSNBK; break;
    case GCC_CONNECT: 	parms->state = GC_CONNBK; break;
    case GCC_SEND: 	parms->state = GC_SNDBK; break;
    case GCC_RECEIVE: 	parms->state = GC_RCVBK; break;
    case GCC_DISCONNECT:parms->state = GC_DISCONN; break;
    }

    GCX_TRACER_1_MACRO("GCbk>", parms,
		"function code = %d", func_code);

    GCTRACE(1)("GC%s: %p operation %s\n", 
	parms->pce->pce_pid, parms, gc_names[ parms->state ] );

    parms->generic_status = OK;
    CL_CLEAR_ERR( &parms->system_status );

    /* Start sm. */

    GCbssm( parms );
    return OK;
}

static VOID
GCbssm( parms )
GCC_P_PLIST	*parms;
{
    char			*portname, *hostname;
    GC_PCB			*pcb = (GC_PCB *)parms->pcb;
    GC_DCB			*dcb = (GC_DCB *)parms->pce->pce_dcb;
    BS_DRIVER			*bsd = (BS_DRIVER *)parms->pce->pce_driver;
    BS_PARMS			bsp;
    i4				len;
    char    	    	 	*trace;

    /* Allocate per driver control block */

    if( !dcb )
    {
	dcb = (GC_DCB *)MEreqmem( 0, sizeof( *dcb ), TRUE, (STATUS *)0 );
	parms->pce->pce_dcb = (PTR)dcb;
    }

    if( !dcb )
    {
	bsp.status = GC_OPEN_FAIL;
	goto complete;
    }

    /* Copy some parameters */

    bsp.func = GCbssm;
    bsp.closure = (PTR)parms;
    bsp.timeout = -1;
    bsp.syserr = &parms->system_status;
    bsp.bcb = pcb ? pcb->bcb : NULL;
    bsp.lbcb = dcb->lsn_bcb;
    bsp.status = OK;

    /* main loop of execution */

top:
    GCTRACE(2)("GC%s: %p state %s\n", 
	parms->pce->pce_pid, parms, gc_names[ parms->state ] );

    switch( parms->state )
    {
    case GC_REG:
	/*
	** Register before the first listen.
	*/

	NMgtAt( "II_GCCL_TRACE", &trace );
	if ( ! (trace && *trace)  &&
	     PMget( "!.gccl_trace_level", &trace ) != OK )
    	    GCbs_trace  = 0;
	else
    	    GCbs_trace  = atoi( trace );

	parms->function_parms.open.lsn_port = NULL;

    	/* See whether this is a "no listen" protocol */
    	if ( parms->pce->pce_flags & PCT_NO_PORT )  goto complete;

	/*
	** Use requested listen port unless it is the generic
	** default "II" in which case try II_INSTALLATION and
	** protocol default first.
	*/
	if ( STcompare( parms->function_parms.open.port_id, "II" ) )
	    STcopy( parms->function_parms.open.port_id, dcb->lsn_port );
	else
	{
	    char *value ;

	    NMgtAt( "II_INSTALLATION", &value );

	    if ( value && *value )
		STlcopy( value, dcb->lsn_port, sizeof(dcb->lsn_port)-1 );
	    else  if ( *parms->pce->pce_port != EOS )
		STcopy( parms->pce->pce_port, dcb->lsn_port );
	    else
		STcopy(parms->function_parms.open.port_id, dcb->lsn_port);
	}

	dcb->subport = 0;
	parms->state = GC_REG1;
	break;

    case GC_REG1:
	/*
	** Translate listen port name into tcp port number. 
	** Fail with status from last listen if we have exhausted
	** usable listen subports.
	*/
	if ( (*bsd->portmap)( dcb->lsn_port, dcb->subport, dcb->portbuf, 
						dcb->actual_lsn_port) != OK )
	{
	    bsp.status = dcb->last_status;
	    goto complete;
	}

	GCTRACE(1)("GC%s: %p listening at %s\n", 
	    parms->pce->pce_pid, parms, dcb->portbuf );

	/* Extend listen with BS listen. */

	bsp.buf = dcb->portbuf;

	(*bsd->listen)( &bsp );

	if( bsp.status != OK )
	{
	    dcb->subport++;
	    dcb->last_status = bsp.status;
	    break;
	}

	/* Note protocol is open and print message to that effect. */

	GCTRACE(1)( "GC%s: Listen port %s subport %d (%s)\n",
		    parms->pce->pce_pid, dcb->lsn_port, dcb->subport, bsp.buf );

	parms->function_parms.open.lsn_port = dcb->portbuf;
        /* set the value of  parms->function_parms.open.actual_port_id */
        parms->function_parms.open.actual_port_id = dcb->actual_lsn_port;
        goto complete;

    case GC_LSN:
    case GC_LSNBK:
	/*
	** Extend a listen to accept a connection.  
	*/

	parms->function_parms.listen.node_id = NULL;
	parms->function_parms.listen.lcl_addr = NULL;
	parms->function_parms.listen.rmt_addr = NULL;

	/* Allocate pcb */

	switch( parms->state )
	{
	case GC_LSN: 	len = BS_PCB_BUFF_SZ;	break;
	case GC_LSNBK:	len = BK_PCB_BUFF_SZ;	break;
	}
	
	pcb = (GC_PCB *)MEreqmem( 0, sizeof(*pcb) + len, TRUE, (STATUS *)0 );
	pcb->buf_len = sizeof( pcb->buffer ) + len;
	parms->pcb = (PTR)pcb;

	if( !pcb )
	{
	    bsp.status = GC_LISTEN_FAIL;
	    goto complete;
	}

	/* Poll for incoming connection. */

	bsp.bcb = pcb->bcb;
	bsp.regop = BS_POLL_ACCEPT; 
	parms->state = GC_LSNCMP;

        if (((BCB *)bsp.bcb)->optim & BS_SOCKET_DEAD)
        {
	    bsp.status = GC_LISTEN_FAIL;
	    goto complete;
        }
	if( (*bsd->regfd)( &bsp ) )
	    return;
	break;

    case GC_LSNCMP:
	/*
	** Listen ready.  
	*/
	
	/* Initialize buffer to get host node name
	** This is only valid while calling the completion routine !
	** Initialize options to get local/remote indicator    
	*/
	parms->options = 0;
	pcb->buffer[ 0 ] = 0;
	bsp.buf = pcb->buffer;
	bsp.len = BK_PCB_BUFF_SZ;

	/* Pick up connection with BS accept. */
	(*bsd->accept)( &bsp );
	if ( bsp.status != OK )  goto complete;

	/* Set up LISTEN output */
	if ( pcb->buffer[0] == 0 )  STcopy("<unknown>", pcb->buffer );
	parms->function_parms.listen.node_id = pcb->buffer;
	if ( ! bsp.is_remote )  parms->options |= GCC_LOCAL_CONNECT;

	/* Set up read buffer pointers. */
	pcb->b1 = pcb->b2 = pcb->buffer;

	if ( bsd->ext_info )
	{
	    GCC_ADDR	*lcl_addr = (GCC_ADDR *)&pcb->buffer[GCC_L_NODE];
	    GCC_ADDR	*rmt_addr = &lcl_addr[1];
	    BS_EXT_INFO	info;

	    parms->function_parms.listen.lcl_addr = lcl_addr;
	    parms->function_parms.listen.rmt_addr = rmt_addr;

	    lcl_addr->node_id[0] = lcl_addr->port_id[0] = EOS;
	    rmt_addr->node_id[0] = rmt_addr->port_id[0] = EOS;

	    info.info_request = BS_EI_LCL_ADDR | BS_EI_RMT_ADDR;
	    info.len_lcl_node = sizeof( lcl_addr->node_id );
	    info.lcl_node = lcl_addr->node_id;
	    info.len_lcl_port = sizeof( lcl_addr->port_id );
	    info.lcl_port = lcl_addr->port_id;
	    info.len_rmt_node = sizeof( rmt_addr->node_id );
	    info.rmt_node = rmt_addr->node_id;
	    info.len_rmt_port = sizeof( rmt_addr->port_id );
	    info.rmt_port = rmt_addr->port_id;

	    (*bsd->ext_info)( &bsp, &info );
	}

	GCTRACE(1)( "GCbssm : Connection with %s is %s\n", 
			parms->function_parms.listen.node_id,
			bsp.is_remote ? "remote" : "local" );

	goto complete;

    case GC_CONN:
    case GC_CONNBK:
	/*
	** Make outgoing connection with ii_BSconnect 
	*/

	parms->function_parms.connect.lcl_addr = NULL;
	parms->function_parms.connect.rmt_addr = NULL;

	/* Allocate pcb */

	switch( parms->state )
	{
	case GC_CONN:	len = BS_PCB_BUFF_SZ;	break;
	case GC_CONNBK:	len = BK_PCB_BUFF_SZ;	break;
	}
	
	if( parms->pcb)
	    pcb = (GC_PCB *) parms->pcb;
	else
	{
	    pcb = (GC_PCB *)MEreqmem(0, sizeof(*pcb) + len, TRUE, (STATUS *)0);
	    pcb->buf_len = sizeof( pcb->buffer ) + len;
	    parms->pcb = (PTR)pcb;
	}

	if( !pcb )
	{
	    bsp.status = GC_CONNECT_FAIL;
	    goto complete;
	}

	/* get the hostname/portname */

	hostname = parms->function_parms.connect.node_id; 
	portname = parms->function_parms.connect.port_id; 

	/* setup the connection name */

	dcb->portbuf[0] = '\0';
	if( hostname && *hostname )
	    STprintf( dcb->portbuf, "%s;;", hostname );

	if( portname && *portname )
	{
	    dcb->actual_lsn_port[0] = '\0';
	    (void)(*bsd->portmap)( portname, 0,
	    dcb->portbuf + STlength( dcb->portbuf ), dcb->actual_lsn_port );
	}

	GCTRACE(1)("GC%s: %p connecting to %s\n",
	    parms->pce->pce_pid, parms, dcb->portbuf );

	/* Make outgoing request call with BS request. */

	bsp.bcb = pcb->bcb;
	bsp.buf = dcb->portbuf;

	(*bsd->request)( &bsp );

	if( bsp.status != OK )
	    goto complete;

	/* Poll for connect completion. */

	parms->state = GC_CONNCMP;
	bsp.regop = BS_POLL_CONNECT; 

        if (((BCB *)bsp.bcb)->optim & BS_SOCKET_DEAD)
        {
	    bsp.status = GC_CONNECT_FAIL;
	    goto complete;
        }
	if( (*bsd->regfd)( &bsp ) )
	    return;
	break;

    case GC_CONNCMP:
	/*
	** Connect completes.  
	*/

	/* Pick up connection with BS connect. */
	(*bsd->connect)( &bsp );
	if( bsp.status == BS_INCOMPLETE )
	{
	    parms->state = GC_CONN;
	    break;
	}
	if ( bsp.status != OK )  goto complete;

	/* Set up read buffer pointers. */
	pcb->b1 = pcb->b2 = pcb->buffer;

	if ( bsd->ext_info )
	{
	    GCC_ADDR	*lcl_addr = (GCC_ADDR *)pcb->buffer;
	    GCC_ADDR	*rmt_addr = &lcl_addr[1];
	    BS_EXT_INFO	info;

	    parms->function_parms.connect.lcl_addr = lcl_addr;
	    parms->function_parms.connect.rmt_addr = rmt_addr;

	    lcl_addr->node_id[0] = lcl_addr->port_id[0] = EOS;
	    rmt_addr->node_id[0] = rmt_addr->port_id[0] = EOS;

	    info.info_request = BS_EI_LCL_ADDR | BS_EI_RMT_ADDR;
	    info.len_lcl_node = sizeof( lcl_addr->node_id );
	    info.lcl_node = lcl_addr->node_id;
	    info.len_lcl_port = sizeof( lcl_addr->port_id );
	    info.lcl_port = lcl_addr->port_id;
	    info.len_rmt_node = sizeof( rmt_addr->node_id );
	    info.rmt_node = rmt_addr->node_id;
	    info.len_rmt_port = sizeof( rmt_addr->port_id );
	    info.rmt_port = rmt_addr->port_id;

	    (*bsd->ext_info)( &bsp, &info );
	}
	goto complete;

    case GC_SND:
	/*
	** Send NPDU with ii_BSwrite
	*/

	/* add NDU header (TPC specific length) */

	pcb->send.buf = parms->buffer_ptr - 2;
	pcb->send.len = parms->buffer_lng + 2;
	pcb->send.active = TRUE;  
	pcb->send.parms = parms; 

	pcb->send.buf[0] = (u_char)(pcb->send.len % 256);
	pcb->send.buf[1] = (u_char)(pcb->send.len / 256);

	GCTRACE(2)("GC%s: %p send size %d\n",
	    parms->pce->pce_pid, parms, parms->buffer_lng );

	/* Poll for send. */

	parms->state = GC_SNDCMP;
	bsp.regop = BS_POLL_SEND; 
        if (((BCB *)bsp.bcb)->optim & BS_SOCKET_DEAD)
        {
	    bsp.status = GC_SEND_FAIL;
	    pcb->send.active = FALSE;  
	    goto complete;
        }
	if( (*bsd->regfd)( &bsp ) )
	    return;
	break;

    case GC_SNDBK:
	/*
	** Send NPDU with ii_BSwrite
	*/

	pcb->send.buf = parms->buffer_ptr;
	pcb->send.len = parms->buffer_lng;

	GCTRACE(2)("GC%s: %p send size %d\n",
	    parms->pce->pce_pid, parms, parms->buffer_lng );

	/* Poll for send. */

	parms->state = GC_SNDCMP;
	bsp.regop = BS_POLL_SEND; 
        if (((BCB *)bsp.bcb)->optim & BS_SOCKET_DEAD)
        {
	    bsp.status = GC_SEND_FAIL;
	    pcb->send.active = FALSE;  
	    goto complete;
        }
	if( (*bsd->regfd)( &bsp ) )
	    return;
	break;

    case GC_SNDCMP:
	/*
	** Send ready.
	*/

	/* Send data with BS send. */

	bsp.buf = pcb->send.buf;
	bsp.len = pcb->send.len;

	(*bsd->send)( &bsp );

	if( bsp.status != OK )
	{
	    pcb->send.active = FALSE;  
	    goto complete;
	}

	pcb->send.buf = bsp.buf;
	pcb->send.len = bsp.len;

	/* More to send? */

	if( bsp.len )
	{
		bsp.regop = BS_POLL_SEND; 
                if (((BCB *)bsp.bcb)->optim & BS_SOCKET_DEAD)
                {
	            bsp.status = GC_SEND_FAIL;
		    pcb->send.active = FALSE;  
	            goto complete;
                }
		if( (*bsd->regfd)( &bsp ) )
		    return;
		break;
	}

	pcb->send.active = FALSE;  
	goto complete;

    case GC_RCV:
	/*
	** Setup bounds of read area.
	**
	** Caller should restrict requests to the limit defined
	** in the protocol table.  We allow longer requests, but
	** are limited by the size of the overflow buffer.  By
	** restricting reads to the size of the overflow buffer,
	** we ensure that a buffer overrun won't occur.  This
	** means multiple reads will be required to receive
	** requests larger than the declared limit.
	*/
	pcb->recv.buf = parms->buffer_ptr - 2;
	pcb->recv.len = min( parms->buffer_lng + 2, pcb->buf_len );
	pcb->recv.active = TRUE;  
	pcb->recv.parms = parms; 

	/* Get what we can from the buffer */

	if( len = ( pcb->b2 - pcb->b1 ) )
	{
	    /* If we have the NDU header, use the length */

	    if( len >= 2 )
	    {
		i4 len2 = ((u_char *)pcb->b1)[0] +
			   ((u_char *)pcb->b1)[1] * 256;
		len = min( len, len2 );
	    }

	    GCTRACE(3)("GC%s: %p using %d\n",
		parms->pce->pce_pid, parms, len );

	    MEcopy( pcb->b1, len, pcb->recv.buf );
	    pcb->recv.buf += len;
	    pcb->recv.len -= len;
	    pcb->b1 += len;
	}

	parms->state = GC_RCV2;
	break;

    case GC_RCV1:
	/*
	** receive ready.
	*/

	/* Read data with BS receive. */

	bsp.buf = pcb->recv.buf;
	bsp.len = pcb->recv.len;

	(*bsd->receive)( &bsp );

	if( bsp.status != OK )
	{
	    pcb->recv.active = FALSE;  
	    goto complete;
	}

	pcb->recv.buf = bsp.buf;
	pcb->recv.len = bsp.len;

	parms->state = GC_RCV2;
	break;

    case GC_RCV2:
	/*
	** Data read.
	*/

	/* Reinvoke for short read */

	if( pcb->recv.buf < parms->buffer_ptr )
	{
	    parms->state = GC_RCV1;
	    bsp.regop = BS_POLL_RECEIVE; 
            if (((BCB *)bsp.bcb)->optim & BS_SOCKET_DEAD)
            {
	        bsp.status = GC_RECEIVE_FAIL;
		pcb->recv.active = FALSE;  
	        goto complete;
            }
	    if( (*bsd->regfd)( &bsp ) )
		return;
	    break;
	}

	/* strip NDU header */

	len = ((u_char *)parms->buffer_ptr)[-2] +
	      ((u_char *)parms->buffer_ptr)[-1] * 256 - 2;

	/*
	** Packets cannot exceed the size of the buffer
	** provided by the caller.  They may exceed our
	** declared limit, but that will require multiple
	** reads to receive the entire packet.
	*/
	if( len < 0 || len > parms->buffer_lng )
	{
	    bsp.status = GC_RECEIVE_FAIL;
	    pcb->recv.active = FALSE;  
	    goto complete;
	}

	parms->buffer_lng = len;

	/* Issue read for TPDU */

	GCTRACE(2)("GC%s: %p read size %d\n",
	    parms->pce->pce_pid, parms, parms->buffer_lng );

	parms->state = GC_RCV4;
	break;

    case GC_RCV3:
	/*
	** ii_BSread completes for TPDU.
	*/

	/* Read data with BS receive. */

	bsp.buf = pcb->recv.buf;
	bsp.len = pcb->recv.len;

	(*bsd->receive)( &bsp );

	if( bsp.status != OK )
	{
	    pcb->recv.active = FALSE;  
	    goto complete;
	}

	pcb->recv.buf = bsp.buf;
	pcb->recv.len = bsp.len;
	parms->state = GC_RCV4;
	break;

    case GC_RCV4:
	/*
	** Data read.
	*/

	len = pcb->recv.buf - ( parms->buffer_ptr + parms->buffer_lng );

	/*
	** A negative length indicates the number of bytes
	** remaining in the current packet to be read.
	**
	** Reinvoke for short read.
	*/
	if( len < 0 )
	{
	    /*
	    ** The read must be restricted to the size of the 
	    ** overflow buffer to prevent an overrun in the 
	    ** buffer if more data than the current packet is 
	    ** received.
	    */
	    pcb->recv.len = min( -len, pcb->buf_len );
	    parms->state = GC_RCV3;
	    bsp.regop = BS_POLL_RECEIVE; 
            if (((BCB *)bsp.bcb)->optim & BS_SOCKET_DEAD)
            {
	        bsp.status = GC_RECEIVE_FAIL;
		pcb->recv.active = FALSE;  
	        goto complete;
            }
	    if( (*bsd->regfd)( &bsp ) )
		return;
	    break;
	}

	/* 
	** A positive length indicates excess data received
	** beyond the current packet.
	**
	** Copy any excess 
	*/
	if( len > 0 )
	{
	    GCTRACE(3)("GC%s: %p saving %d\n",
		parms->pce->pce_pid, parms, len );
	    MEcopy( pcb->recv.buf - len, len, pcb->buffer );
	    pcb->b1 = pcb->buffer;
	    pcb->b2 = pcb->buffer + len;
	}

	pcb->recv.active = FALSE;  
	goto complete;

    case GC_RCVBK:
	/*
	** Setup bounds of read area.
	*/

	pcb->recv.buf = parms->buffer_ptr;
	pcb->recv.len = parms->buffer_lng;
	pcb->recv.active = TRUE;  

	parms->state = GC_RCV1BK;
	bsp.regop = BS_POLL_RECEIVE; 
        if (((BCB *)bsp.bcb)->optim & BS_SOCKET_DEAD)
        {
	    bsp.status = GC_RECEIVE_FAIL;
	    pcb->recv.active = FALSE;  
	    goto complete;
        }
	if( (*bsd->regfd)( &bsp ) )
	    return;
	break;

    case GC_RCV1BK:
	/*
	** receive ready.
	*/

	/* Read data with BS receive. */

	bsp.buf = pcb->recv.buf;
	bsp.len = pcb->recv.len;

	(*bsd->receive)( &bsp );

	if( bsp.status != OK )
	{
	    pcb->recv.active = FALSE;  
	    goto complete;
	}

	pcb->recv.buf = bsp.buf;
	pcb->recv.len = bsp.len;

	/* Reinvoke if not end of message */

	if( pcb->recv.len )
	{
	    bsp.regop = BS_POLL_RECEIVE; 
            if (((BCB *)bsp.bcb)->optim & BS_SOCKET_DEAD)
            {
	        bsp.status = GC_RECEIVE_FAIL;
		pcb->recv.active = FALSE;  
	        goto complete;
            }
	    if( (*bsd->regfd)( &bsp ) )
		return;
	    break;
	}

	parms->buffer_lng = pcb->recv.buf - parms->buffer_ptr;

	/* Got TPDU */

	GCTRACE(2)("GC%s: %p read size %d\n",
	    parms->pce->pce_pid, parms, parms->buffer_lng );

	pcb->recv.active = FALSE;  
	goto complete;

    case GC_DISCONN:
	/*
	** Poll for release.
	*/

	if( !pcb )
		goto complete;

	/*
	** Fail any outstanding send/recv requests
	*/
	GC_abort( pcb );

	/* If there's a release function, poll for it. */

	if( bsd->release )
	{
	    bsp.regop = BS_POLL_SNDREL;
	    parms->state = GC_DISCONN1;
            if (((BCB *)bsp.bcb)->optim & BS_SOCKET_DEAD)
            {
	        bsp.status = GC_DISCONNECT_FAIL;
	        goto complete;
            }
	    if( (*bsd->regfd)( &bsp ) )
		return;
	}
	else
	{
	    parms->state = GC_DISCONN3;
	}
	break;
    
    case GC_DISCONN1:
	/*
	** Make orderly release.
	*/

	(*bsd->release)( &bsp );

	/* Don't check status - it's not worth trying to recover */

	parms->state = GC_DISCONN2;

	break;

    case GC_DISCONN2:
	/*
	** Poll for rcvrel.
	*/

	bsp.regop = BS_POLL_RCVREL;
	parms->state = GC_DISCONN3;
        if (((BCB *)bsp.bcb)->optim & BS_SOCKET_DEAD)
        {
	    bsp.status = GC_DISCONNECT_FAIL;
	    goto complete;
        }
	if( (*bsd->regfd)( &bsp ) )
	    return;
	break;

    case GC_DISCONN3:
	/*
	** Final close.
	*/

	(*bsd->close)( &bsp );

	if( bsp.status == BS_INCOMPLETE )
	{
	    parms->state = GC_DISCONN2;
	    break;
	}

	MEfree( (PTR)pcb );
	goto complete;

    }
    goto top;

complete:
    /*
    ** Drive completion routine.
    */

    parms->generic_status = bsp.status;

    GCTRACE(1)("GC%s: %p complete %s status %x\n",
	parms->pce->pce_pid, parms, gc_names[ parms->state ],
	parms->generic_status );

    (*parms->compl_exit)(parms->compl_id);
}

static VOID
GC_abort( GC_PCB *pcb )
{
 GCC_P_PLIST *parms;

	/*
	** Look for active SEND/RECV requests.
	** copy the status around and set the request up
	** for callback.
	*/

        if( pcb->send.active )
	{
	    pcb->send.active = FALSE; 
	    parms = pcb->send.parms;
	    parms->generic_status = GC_SEND_FAIL;
	    GCTRACE(1)("GC%s: %p abort send %s status %x\n",
	    parms->pce->pce_pid, parms, gc_names[ parms->state ],
	    parms->generic_status );

	    (*parms->compl_exit)(parms->compl_id);
	}
        if( pcb->recv.active )
	{
	    pcb->recv.active = FALSE; 
	    parms = pcb->recv.parms;
	    parms->generic_status = GC_RECEIVE_FAIL;
	    GCTRACE(1)("GC%s: %p abort recv %s status %x\n",
	    parms->pce->pce_pid, parms, gc_names[ parms->state ],
	    parms->generic_status );

	    (*parms->compl_exit)(parms->compl_id);

	}
	return;
}
