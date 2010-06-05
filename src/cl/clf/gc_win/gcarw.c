# include <compat.h>
# include <er.h>
# include <gl.h>
# include <cs.h>
# include <ex.h>
# include <pc.h>
# include <csinternal.h>
# include <qu.h>
# include <ex.h>
# include <me.h>
# include <tr.h>
# include <gc.h>

# include <bsi.h>
# include "gclocal.h"

/*
** Name: GCARW.C - GCsend(), GCreceive(), GCpurge(), GCdisconn()
**
** Description:
**	Sender, Receiver
**	
**	NOTE!! Keep IN SYNC with Unix version for generic (non-OS-specific)
**	changes.
**
**	Copied from Unix version of gcarw.c and then modified for
**	Windows.  This file was needed to implement multiplexed
**	sends and receives of the gcacl channels (normal and 
**	expedited) over a single network connection.  The code
**	is not needed or used for the original, native implementations
**	of gcacl over pipes ("named" for Windows NT, "anonymous" for W9x).
**	However, it IS used to implement "tcp_ip" (and theoretically
**	any Windows GCC network protocol driver) as the local IPC
**	protocol, which is enabled by setting II_GC_PROT environment
**	variable.  With the pipes implementation, each gcacl channel
**	was mapped to a separate physical pipe.  With the network 
**	protocol driver implementation, only 1 physical network connection
**	is set up per session; hence, the send and receive requests
**	for the normal and expedited channels must be multiplexed
**	over the same physical network connection.  
**
**	Another reason to use the same code/implementation as Unix is that
**	GCpurge is also implemented in this program and, hence, now
**	done the same way.  For pipes, GCpurge is implemented by sending
**	a message containing the text "IIPRGMSG".  For tcp_ip, 2 chopmarks
**	are sent and received; chopmarks are basically empty messages
**	that contain only the multiplexing prefix.
**
**	Because the multiplexing and GCpurge handling is now the same
**	for "tcp_ip" between Windows and Unix, "direct connect" can 
**	be used between Ingres's on the 2 different platforms as long
**	as the other criteria for "direct connect" are satisfied
**	(eg, same integer formats, float formats, charsets).
**          Add support for net drivers like "tcp_ip" as local IPC.
**
**	The main differences of gcarw.c on Windows versus Unix are:
**	1. There is no BSP network driver interface on Windows.
**	   Though a BSP interface could be created (with a lot of work),
**	   the GCC CL interface is used instead.  Hence, GCC_P_PLIST is
**	   the main parmlist structure for interfacing with the driver(s).
**	2. The gcacl connection control block (svc_parms->gc_cb) is
**	   different; the one being used in Windows is an ASSOC_IPC
**	   structure.  A gcb (GCA_GCB) is allocated when a session is set
**	   up to use a driver and is linked to from the ASSOC_IPC.  The
**	   gcb then is almost the same across Windows and Unix, allowing
**	   the code below to continue to use a gcb as its primary
**	   structure.
**	3. The Unix BSP I/O interface first issues a "register" on an
**	   fd to see if it is has data ready to send or receive and then
**	   issues the send or receive.  On Windows, the first step is to
**	   issue a send or receive and then get posted with a callback
**	   when the I/O is complete.
**	4. The BSP interface updates the buffer pointer to the end of the
**	   data sent or received.  This is used to calculate the length
**	   of data sent or received. With the GCC CL interface, the buffer
**	   pointer is not changed, so the returned length field must be
**	   used instead.
**	5. The externally-visible gcacl GCreceive, GCsend, GCpurge and
**	   GCdisconnect functions were kept in gcacl.c on Windows since
**	   they are also needed if the pipes protocol (default) is used.
**	   The entry function names in this file were changed from
**	   GCreceive to GCDriverReceive, etc.
**	6. Status codes returned from the network driver (gcc cl) are
**	   remapped to valid gc cl status codes, which are later remapped
**	   (in GCA) to GCA status codes.  Not remapping the driver statuses
**	   causes error message file lookup problems because the converted
**	   GCA status won't be valid (see gc.h and gca.h for valid mappings).
**	   Network driver statuses are in the 0x0001278x range such as
**	   GC_CONNECT_FAIL, GC_SEND_FAIL. etc, which have no equivalent
**	   in the GCA statuses which are in the 0x000C0001-58 range (at
**	   this time).  
**	   FYI: Unix BS statuses (0x0001fexx range) don't get converted
**	   by GCA to GCA statuses AND have corresponding error messages,
**	   so don't experience the same problem.  NOTE that the pipes
**	   code on Windows utilizes the BS_READ_ERR code of 0x0001FE07 
**	   but #defines it with the name NP_IO_FAIL.  This is, to some
**	   extent, a hack since the BS errors are really for the Unix CL
**	   BS implementation.
**
**	The remainder of the comments following this point are from
**	the original (Unix) file.
**
**	Both GCsend() and GCreceive() merely post requests to their
**	respective state machines, GC_send_sm() and GC_receive_sm(),
**	and start the state machines if they are not running.
**	GCpurge() is like GCsend(), but posts a special request to the
**	GC_send_sm() to send two 0 length messages.  
**	
**	The sender and receiver state machines list themselves as the
**	completion routines for their ii_BS calls, and store their
**	state in the GCA_GCB allocated by GClisten() or GCrequest().
**	The state machine is "running" if either the state machine code
**	is active or the state machine name (GC_send_sm() GC_recv_sm())
**	is on the callback of an ii_BS routine.  GCsend() and
**	GCreceive() insure the integrity of state machine data by
**	calling the state machine only if it is not already running.
**	Therefore, there is at most one sender and one receiver state
**	machine running for each connection (or GCA_GCB).
**	
**	We handle normal and expedited data (as indicated by SVC_PARMS'
**	"flags.flow_indicator" flag) as two multiplexed subchannels on
**	the same channel.  The sender gives the "expedited" subchannel
**	higher priority; otherwise we treat these subchannels
**	identically.  To note the subchannel in the byte stream, an
**	MTYP, containing the subchannel number (0 or 1) and the block
**	length, precedes each block of data.
**	
**	GC_send_sm() loops, looking for a) a request to send chop
**	marks, b) a request to send data.  To send chop marks, it makes
**	two calls to ii_BSsend() to send two MTYP's with 0 length data
**	blocks, and then ii_BSflush() flush the channel.  To send data,
**	it calls ii_BSsend(), then ii_BSflush() flush the channel, and
**	then drives the client completion routine.  GC_send_sm()
**	returns when there are no requests.
**	
**	GC_recv_sm() performs the following loop: call ii_BSread() to
**	to read the next incoming MTYP;  determine the subchannel of
**	the incoming data; determine the target client buffer; call
**	ii_BSread() to read the data; drive the client completion
**	routine.  GC_recv_sm() returns when there are no requests to
**	read data.
**	
**	When GC_recv_sm() reads a chop mark (an MTYP block with 0
**	length data) it sets the read request's outstanding receive
**	length to 0, so that it is immediately satisfied, and allows
**	GC_recv_complete() to note that the received data length is not
**	what was requested and to set the SVC_PARMS' "flags.new_chop"
**	flag.
**	
** History:
**	 ... copied from Unix version as of following change:
**	 2-Feb-2010 (hanal04) Bug 123208
**	    In GC_recv_sm() close a security vulnerability by checking
**	    the supplied length is greater than zero.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Created.  Copied from Unix version and then modified for
**	    Windows.  Part of project to add support for net drivers
**	    like "tcp_ip" as local IPC on Windows if II_GC_PROT set.
**	    This file was needed to implement multiplexed sends and
**	    receives of the gcacl channels (normal and expedited) over
**	    a single network connection.  NOTE that prior history
**	    comments from the original gcarw.c file were deleted in
**	    in this section; look at the Unix version if interested.
**	    Removed all "#ifdef OS_THREADS_USED" since Ingres on Windows
**	    is always built for OS threads. Removed listenbcb (N/A on
**	    Windows).
**	    Modified GCTRACE levels for non-error conditions to 3.
**	    Add 2-byte prefix to static GC_chopmarks for use by called
**	    network driver for length prefix...otherwise prior field
**	    in static area is overlaid if called protocol driver is
**	    using streams mode (which it doesn't due to Unix/Linux
**	    direct connect incompatibility, but it "could" and was
**	    successfully tested Windows to Windows).
**	    Don't set new_chop flag if receive unsuccessful.
*/

GLOBALREF i4  GC_trace;
# define GCTRACE( n )  if ( GC_trace >= n )(void)TRdisplay

static void	GC_recv_sm( GCA_GCB *gcb );
static void	GC_send_sm( GCA_GCB *gcb );
static void	GC_disc_sm( GCA_GCB *gcb );
static void	GC_drive_complete( GCA_GCB *gcb );
static VOID	GC_abort_recvs( GCA_GCB *gcb, STATUS status );
static VOID	GC_abort_sends( GCA_GCB *gcb, STATUS status );

static		struct
{
	char		GC_chopmarks_len[2];  /* used by network driver*/
	GC_MTYP		GC_chopmarks[2];
} GC_chopmarks_buf = {0x0000, 0, 0, 0, 0 };
static 		char 		*gc_chan[] = { "", " EXPEDITED" };
static 		char 		*gc_chop[] = { "", " NEWCHOP" };


/*
** Name: GCDriverReceive() - post a request to the receive state machine
**
** Description:
**	See comments at head of file.
**
** Called by:
**	GCA CL (GCreceive)
**
** History:
**	8-jun-93 (ed)
**	    -added VOID to match prototype
**	 9-Apr-97 (gordy)
**	    If there is peer info available from an old-style
**	    peer info exchange, it is used to satisfy the first
**	    receive request.
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Modified for Windows. Changed function name from GCreceive to
**	    GCDriverReceive.
**	    Get access to GCA_GCB via ASSOC_IPC->gca_gcb rather than
**	    directly from gc_cb in SVC_PARMS, which points to ASSOC_IPC
**	    on Windows. Not supporting pre-Ingres r3/2006 protocol for
**	    exchanging peerinfo message so remove related code.
*/

VOID
GCDriverReceive( SVC_PARMS *svc_parms )
{
	ASSOC_IPC	*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
	GCA_GCB		*gcb = (GCA_GCB *)asipc->gca_gcb;
	register struct subchan	*subchan = &gcb->recv_chan 
				[ svc_parms->flags.flow_indicator ];
	 
	GCTRACE(3)( "%sGCDriverReceive %d: want %d%s\n",
		GC_trace > 1 ? "===\n" : "",
		gcb->id, svc_parms->reqd_amount,
		gc_chan[ svc_parms->flags.flow_indicator ] );

	svc_parms->status = OK;

	/* Sanity check for duplicate request. */

	if( subchan->state != GC_CHAN_QUIET )
	{
		GCTRACE(1)( "GCDriverReceive %d: duplicate request\n", gcb->id );
		GC_abort_recvs( gcb, GC_ASSOC_FAIL );
		GC_drive_complete( gcb );
		return;
	}

	if( !svc_parms->svc_buffer )
	{
		GCTRACE(1)( "GCDriverReceive %d: null request\n", gcb->id );
		GC_abort_recvs( gcb, GC_ASSOC_FAIL );
		GC_drive_complete( gcb );
		return;
	}

	/* Note request on subchannel.  */

	subchan->svc_parms = svc_parms;
	subchan->buf = svc_parms->svc_buffer;
	subchan->len = svc_parms->reqd_amount;
	subchan->state = GC_CHAN_ACTIVE;
	gcb->recv.timeout = svc_parms->time_out;

	/* If nothing (more) to read, complete.  */

	if( !subchan->len )
	{
		subchan->state = GC_CHAN_DONE;
		GC_drive_complete( gcb );
	}

	/* Start the reader state machine, if not already running. */

	if( !gcb->recv.running )
	{
		GCTRACE(3)( "GCDriverReceive %d: recv_sm not running so start it\n", gcb->id);
		gcb->recv.running = TRUE;
		GC_recv_sm( gcb );
	}

}


/*
** Name: GC_recv_sm() - receiver state machine
**
** Description:
**	See comments at head of file.
**
** Called by:
**	GCDriverReceive()
**	GC_recv_sm() via callback
**
** History:
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    All 'bsp' refs were changed to 'gccclp' (plus corresponding
**	    field names within the structure) since BS_PARMS interface
**	    is replaced with GCC_P_PLIST interface on Windows.
**	    Change register (GCdriver->regfd) to a GCC_RECEIVE call for
**	    the GCC CL protocol driver.  Eliminate the BSP receive and
**	    just check the return status and lengths at that point (since
**	    receive was already done).  Emulate BSP updating of buffer_ptr
**	    to end of data received.  CL_ERR_DESC in gccclp is a full
**	    CL_ERR_DESC whereas it was a pointer in BSP, so swap roles
**	    of gcb->recv.syserr from a CL_ERR_DESC struct to a pointer.
**	    Ie, gcb->recv.syserr is now a pointer to gccclp.system_status
**	    instead of the reverse.  Move GCTRACE of state machine info
**	    into while loop to get more detailed tracing.
**	    If receive was immediate (versus pending), handle at that
**	    point rather than waiting for completion exit to be called.
*/

# define GC_R_IDLE 0
# define GC_R_FILL 1
# define GC_R_CHECK 2
# define GC_R_LOOK 3

static char *gc_trr[] = { "IDLE", "FILL", "CHECK", "LOOK" };

static void
GC_recv_sm( GCA_GCB *gcb )
{
	i4		len;
	i4		n;
	struct subchan *subchan;
	STATUS		status;
	GCC_PCE		*pce    = (GCC_PCE *)gcb->GCdriver_pce;

	GCTRACE(3)( "GC_recv_sm %d: entered\n", gcb->id );

	/* Loop while requests outstanding. */

	while( gcb->recv_chan[ n = 0 ].state == GC_CHAN_ACTIVE ||
	       ( gcb->recv_chan[ n = 1 ].state == GC_CHAN_ACTIVE ) )
	{
	    GCTRACE(3)( "GC_recv_sm %d: state %s\n", 
		    gcb->id, gc_trr[ gcb->recv.state ] );

	    if( gcb->buffer_guard != GCB_BUFFER_GUARD )
	    {
		DebugBreak();
	    }

	    switch( gcb->recv.state )
	{
	case GC_R_IDLE:
	    /* 
	    ** Setup to fill buffer.  To avoid copying, read directly 
	    ** into the NORMAL flow buffer, if there is a NORMAL request
	    ** posted.
	    */

	    subchan = &gcb->recv_chan[ 0 ];

	    if( subchan->state == GC_CHAN_ACTIVE &&
		subchan->len >= sizeof( gcb->buffer ) - sizeof( GC_MTYP ) )
	    {
		/* Use NORMAL flow receive buffer */
		gcb->recv.gccclp.buffer_ptr = subchan->buf - sizeof( GC_MTYP );
		gcb->recv.gccclp.buffer_lng = sizeof( gcb->buffer );
		/*
		** If protocol driver were to be configured in (gcacl) PCT
		** to process in stream mode rather than block mode, then the
		** length of the caller's (longer) buffer should be used
		** instead of our (shorter) private buffer size to
		** avoid getting a protocol driver receive failure
		** when the incoming message is larger than our private
		** buffer size.  In stream mode, the driver will always
		** get the "whole" message (and only 1 message),
		** whereas in block mode he returns whatever he gets
		** with a receive and it is up to this program to call
		** back if more data is wanted.  Also, in stream mode,
		** our private buffer would never be used as there would
		** never be any overflow/excess except to manage GC_MTYP
		** for amount of jumbo message remaining to be read.
		** I'm leaving this commented out code here "just in case"
		** someone wants to try stream mode in the future, as
		** it DOES basically work.  However, current driver stream
		** code attaches a 2 byte length prefix (rather than using
		** embedded MTYP length), which prevents direct connect
		** to Unix/Linux systems from working.
		*/
		//gcb->recv.gccclp.buffer_lng = subchan->len + sizeof( GC_MTYP );
	    }
	    else
	    {
		/* Use our private buffer */
		gcb->recv.gccclp.buffer_ptr = gcb->buffer;
		gcb->recv.gccclp.buffer_lng = sizeof( gcb->buffer );
	    }
	    gcb->recv.save_buffer_lng = gcb->recv.gccclp.buffer_lng;

	    gcb->mtyp = (GC_MTYP *)gcb->recv.gccclp.buffer_ptr;

	    /* Go to register for read */

	    gcb->recv.state = GC_R_CHECK;
	    continue;

	case GC_R_FILL:
	    /* Check results from read */

	    if( gcb->recv.gccclp.generic_status != OK )
	    {
		GCTRACE(1)( "GC_recv_sm %d: GCC_RECEIVE failed, status=%x system_status=%d\n",
			gcb->id,
		 	gcb->recv.gccclp.generic_status,
			gcb->recv.gccclp.system_status.callid );
		goto abort;
	    }
	
	    /*
	    ** Emulate BSP updating of buffer_ptr to end of data read
	    ** and decrementing buffer_lng to amount left to read.
	    */
	    gcb->recv.gccclp.buffer_ptr += gcb->recv.gccclp.buffer_lng;
	    gcb->recv.save_buffer_lng   -= gcb->recv.gccclp.buffer_lng;
	    gcb->recv.gccclp.buffer_lng  = gcb->recv.save_buffer_lng;

	    gcb->recv.state = GC_R_CHECK;

	    /* fall through */
		
	case GC_R_CHECK:
	    /* Get at least the mtyp and its data, or fill the buffer. */

	    len = gcb->recv.gccclp.buffer_ptr - (char *)gcb->mtyp;

	    if( len < sizeof( GC_MTYP ) ||
		len < (i4)sizeof( GC_MTYP ) + gcb->mtyp->len &&
		gcb->recv.gccclp.buffer_lng )
	    {
		/* Use timeout for first read */

		gcb->recv.gccclp.timeout = len ? -1 : gcb->recv.timeout;
		gcb->recv.state = GC_R_FILL;

		/* Issue a read op */

		GCTRACE(3)( "GC_recv_sm %d: issuing read for %d bytes\n", 
			gcb->id, gcb->recv.gccclp.buffer_lng );

		/* Copy some parameters to call the network driver */

		gcb->recv.gccclp.function_invoked = GCC_RECEIVE;
		gcb->recv.gccclp.pce        = pce;
		gcb->recv.gccclp.pcb        = gcb->GCdriver_pcb;
		gcb->recv.gccclp.compl_exit = (void (*)(PTR))GC_recv_sm;
		gcb->recv.gccclp.compl_id   = (PTR)gcb;
		gcb->recv.gccclp.generic_status = OK;
		gcb->recv.syserr	    = &gcb->recv.gccclp.system_status;

		status = (pce->pce_protocol_rtne)(
		          gcb->recv.gccclp.function_invoked,
		          &gcb->recv.gccclp);
		if( gcb->buffer_guard != GCB_BUFFER_GUARD )
		{
		    DebugBreak();
		}
		if( status != OK )
		{
		    /*
		    ** If the status is NOT OK, then the receive is done:
		    ** either there was an error or the receive was
		    ** completed immediately.  In either case, the completion
		    ** exit will not be called back by the lower layer.
		    */
		    GCTRACE(3)( "GC_recv_sm %d: issued read -- completed with status=%x\n", 
			gcb->id, gcb->recv.gccclp.generic_status );
		    continue;
		}
		GCTRACE(3)( "GC_recv_sm %d: issued read -- pending\n", 
			gcb->id );
		goto complete;
	    }

	    /* Got MTYP.  Check subchannel. */

	    if( gcb->mtyp->chan < 0 || gcb->mtyp->chan >= GC_SUB )
	    {
		GCTRACE(1)( "GC_recv_sm %d: bad MTYP %d/%d\n", 
			gcb->id, 
			gcb->mtyp->chan, 
			gcb->mtyp->len );
		gcb->recv.gccclp.generic_status = GC_ASSOC_FAIL;
		goto abort;
	    }

	    GCTRACE(3)( "GC_recv_sm %d: recv MTYP %s len %d (%d read)\n", 
		    gcb->id, 
		    gc_chan[ gcb->mtyp->chan ],
		    gcb->mtyp->len,
		    len );

	    gcb->recv.state = GC_R_LOOK;

	    /* fall through */

	case GC_R_LOOK:
	    /* A request waiting on this subchannel? */

	    subchan = &gcb->recv_chan[ gcb->mtyp->chan ];

	    if( subchan->state != GC_CHAN_ACTIVE )
	    {
		/* 
		** Data on wrong channel. 
		** If polling on other channel, time it out.
		*/

		subchan = &gcb->recv_chan[ gcb->mtyp->chan ? 0 : 1];

		if( subchan->state == GC_CHAN_ACTIVE && 
		    subchan->svc_parms->time_out != -1 )
		{
		    subchan->svc_parms->status = GC_TIME_OUT;
		    subchan->svc_parms->sys_err = gcb->recv.syserr; 
		    subchan->state = GC_CHAN_DONE;
		}

		if ( gcb->mtyp->chan == GC_EXPEDITED &&
		     subchan->state == GC_CHAN_ACTIVE &&
		     subchan->svc_parms->time_out == -1 )
		{
		    /* 
		    ** data detected on expedited channel and
		    ** read is blocking on the normal channel
		    */
		    subchan->svc_parms->status = GC_NOTIMEOUT_DLOCK;
		    subchan->svc_parms->sys_err = gcb->recv.syserr;
		    subchan->state = GC_CHAN_DONE;
		}

		goto suspend;
	    }
	    else
	    {
		/*
		** Satisfy request by copying data.
		*/

		char	*src = (char *)gcb->mtyp + sizeof(GC_MTYP);

		/* 
		** Compute the minimum of what's in the buffer,
		** what's in the mtyp, and what the user asked for.
		*/

		len = gcb->recv.gccclp.buffer_ptr - src;

		if( gcb->mtyp->len < len )
		    len = gcb->mtyp->len;

		if( subchan->len < len )
		    len = subchan->len;

		/* Copy buffer and update request counters */

		GCTRACE(3)( "GC_recv_sm %d: using %d bytes\n", gcb->id, len );

		if( src != subchan->buf )
                {
                    /* Close security vulnerabilty hole */
                    if( len < 0 )
                    {
                        subchan->svc_parms->status = GC_INVALID_ARGS;
                        goto abort;
                    }
		    MEcopy( src, len, subchan->buf );
                }

		subchan->buf += len;
		subchan->len -= len;
		subchan->state = GC_CHAN_DONE;

		/*
		** Adjust mtyp for the used data.
		** Set src to point to the unused data, and
		** set len to amount of unused data.
		*/

		gcb->mtyp->len -= len;
		src += len;
		len = gcb->recv.gccclp.buffer_ptr - src;

		/*
		** If both the mtyp and buffer were depleted, go to idle.
		*/

		if( !gcb->mtyp->len && !len )
		{
		    gcb->recv.state = GC_R_IDLE;
		    continue;
		}
	
		/*
		** Normally, GCreceive requests are large enough to
		** use all the data in the MTYP and buffer.  In two
		** cases, they are not: if an older program sends us
		** a jumbo MTYP too big for our buffer; or if a short
		** MTYP (such as expedited data) is immediately 
		** followed by another MTYP.
		*/

		/*
		** Copy pieces back to the beginning of the buffer.
		*/

		GCTRACE(3)( "GC_recv_sm %d: mtyp %d excess %d buffer\n", 
			gcb->id, gcb->mtyp->len, len );

		gcb->recv.gccclp.buffer_ptr = gcb->buffer;
		gcb->recv.gccclp.buffer_lng = sizeof( gcb->buffer );

		/* If any data left in mtyp, copy mtyp over */

		if( gcb->mtyp->len )
		{
		    if( (PTR)gcb->mtyp != gcb->recv.gccclp.buffer_ptr )
		    {
			MEcopy( (PTR)gcb->mtyp, sizeof( GC_MTYP ), 
			        gcb->recv.gccclp.buffer_ptr );
		    }
		    gcb->recv.gccclp.buffer_ptr += sizeof( GC_MTYP );
		    gcb->recv.gccclp.buffer_lng -= sizeof( GC_MTYP );
		}
		if( gcb->buffer_guard != GCB_BUFFER_GUARD )
		{
		    DebugBreak();
		}

		/* Copy remaining user data */

		if( len )
		{
		    MEcopy( (PTR)src, len, gcb->recv.gccclp.buffer_ptr );
		    gcb->recv.gccclp.buffer_ptr += len;
		    gcb->recv.gccclp.buffer_lng -= len;
		}
		if( gcb->buffer_guard != GCB_BUFFER_GUARD )
		{
		    DebugBreak();
		}

		gcb->recv.save_buffer_lng = gcb->recv.gccclp.buffer_lng;

		/* Point base back to our buffer. */

		gcb->mtyp = (GC_MTYP *)gcb->buffer;
		gcb->recv.state = GC_R_CHECK;
		continue;
	    } /* end if/else state != GC_CHAN_ACTIVE */

    abort:
	    GCTRACE(1)( "GC_recv_sm %d: GCC_RECEIVE failed %x system_status=%d\n", 
		    gcb->id,
		    gcb->recv.gccclp.generic_status,
		    gcb->recv.gccclp.system_status.callid );

	    GC_abort_recvs( gcb, gcb->recv.gccclp.generic_status );
	    gcb->recv.state = GC_R_IDLE;
	} /* end switch( gcb->recv.state )  **********/
	} /* end while requests outstanding **********/

suspend:
	/* No outstanding requests - suspend state machine */

	GCTRACE(3)( "GC_recv_sm %d: suspend\n", gcb->id );

	gcb->recv.running = FALSE;

complete:
	GC_drive_complete( gcb );
}


/*
** Name: GCDriverSend() - post request to sender state machine
**
** Description:
**	See comments at head of file.
**
** Called by:
**	GCA CL (GCsend)
**
** History:
**	8-jun-93 (ed)
**	    -added VOID to match prototype
**	 9-Apr-97 (gordy)
**	    If our partner is expecting an old style peer info packet,
**	    combine CL peer info with contents of first send request.
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Modified for Windows. Changed function name from GCsend to
**	    GCDriverSend.
**	    Get access to GCA_GCB via ASSOC_IPC->gca_gcb rather than
**	    directly from gc_cb in SVC_PARMS, which points to ASSOC_IPC
**	    on Windows.  Removed code related to old-style peer info
**	    message exchange since it is not supported.
*/

VOID
GCDriverSend( SVC_PARMS *svc_parms )
{
	ASSOC_IPC	*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
	GCA_GCB		*gcb = (GCA_GCB *)asipc->gca_gcb;
	struct subchan	*subchan = &gcb->send_chan
				[ svc_parms->flags.flow_indicator ];

	GCTRACE(3)( "%sGCDriverSend %d: send %d%s\n",
		GC_trace > 2 ? "===\n" : "",
		gcb->id, svc_parms->svc_send_length,
		gc_chan[ svc_parms->flags.flow_indicator ] );

	svc_parms->status = OK;

	/* Sanity check for duplicate request. */

	if( subchan->state != GC_CHAN_QUIET )
	{
		GCTRACE(1)( "GCDriverSend: duplicate request\n" );
		GC_abort_sends( gcb, GC_ASSOC_FAIL );
		GC_drive_complete( gcb );
		return;
	}

	/* Note request on subchannel.  */

	subchan->svc_parms = svc_parms;
	subchan->buf = svc_parms->svc_buffer;
	subchan->len = svc_parms->svc_send_length;
	subchan->state = GC_CHAN_ACTIVE;

	/* Start the writer state machine, if not already running.  */

	if( !gcb->send.running )
	{
		gcb->send.running = TRUE;
		GC_send_sm( gcb );
	}
}


/*
** GC_send_sm() - sender state machine
**
** Description:
**	See comments at head of file.
**
** Called by:
**	GCDriverSend()
**	GCDriverPurge()
**	GC_send_sm() via callback
**
** History:
**	16-Jan-04 (gordy)
**	    Replaced GC_NO_PEER (which is reserved for GCA) with
**	    GC_ASSOC_FAIL.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    All 'bsp' refs were changed to 'gccclp' (plus corresponding
**	    field names within the structure) since BS_PARMS interface
**	    is replaced with GCC_P_PLIST interface on Windows.
**	    Change register (GCdriver->regfd) to a GCC_SEND call for
**	    the GCC CL protocol driver.  Eliminate the BSP send and
**	    just check the return status and lengths at that point (since
**	    send was already done).  Emulate BSP updating of buffer_ptr
**	    to end of data sent and decrementing of the length left to send.
**	    CL_ERR_DESC in gccclp is a full CL_ERR_DESC whereas it was
**	    a pointer in BSP, so swap roles of gcb->send.syserr from a
**	    CL_ERR_DESC struct to a pointer.  Ie, gcb->send.syserr is now
**	    a pointer to gccclp.system_status instead of the reverse.
**	    If send was immediate (versus pending), handle at that
**	    point rather than waiting for completion exit to be called.
*/

# define GC_W_LOOK 0
# define GC_W_CHOP 1
# define GC_W_DATA 2

static char *gc_trs[] = { "LOOK", "CHOP", "DATA" };

static void
GC_send_sm( GCA_GCB *gcb )
{
	register int 	n;
	STATUS		status;
	GCC_PCE		*pce    = (GCC_PCE *)gcb->GCdriver_pce;

top:
	GCTRACE(3)( "GC_send_sm %d: state %s\n", 
		gcb->id, gc_trs[ gcb->send.state ] );

	switch( gcb->send.state )
	{
	case GC_W_LOOK:
	    /* Look for something to send. */

	    if( gcb->send_chan[ n = 1 ].state == GC_CHAN_ACTIVE || 
		gcb->send_chan[ n = 0 ].state == GC_CHAN_ACTIVE )
	    {
		    /* Sending data.  Setup MTYP. */

		    char	*buf = gcb->send_chan[ n ].buf;
		    i4		len = gcb->send_chan[ n ].len;

		    buf -= sizeof( GC_MTYP );

		    ((GC_MTYP *)buf)->len = len;
		    ((GC_MTYP *)buf)->chan = n;

		    gcb->send.gccclp.buffer_ptr = buf;
		    gcb->send.gccclp.buffer_lng = len + sizeof( GC_MTYP );
		    gcb->sending = &gcb->send_chan[ n ];

		    gcb->send.state = GC_W_DATA;
		    goto writereg;
	    }
	    else if( gcb->sendchops.state == GC_CHAN_ACTIVE )
	    {
		    /* Sending 2 chop marks.  Set up first.  */

		    gcb->send.gccclp.buffer_ptr = (char *)GC_chopmarks_buf.GC_chopmarks;
		    gcb->send.gccclp.buffer_lng = sizeof( GC_chopmarks_buf.GC_chopmarks );
		    gcb->sending = &gcb->sendchops;

		    gcb->send.state = GC_W_CHOP;
		    goto writereg;
	    } 
	    else
	    {
		    /* No outstanding requests - suspend state machine */

		    GCTRACE(3)( "GC_send_sm %d: suspend\n", gcb->id );
		    gcb->send.running = FALSE;
		    break;
	    }

	case GC_W_CHOP:
	case GC_W_DATA:

	    /* Check results from write chop mark or data. */

            if( gcb->send.gccclp.generic_status != OK )
	    {
		GCTRACE(1)( "GC_send_sm %d: GCC_SEND failed %x system_status=%d\n", 
			    gcb->id,
			    gcb->send.gccclp.generic_status,
			    gcb->send.gccclp.system_status.callid );

		GC_abort_sends( gcb, gcb->send.gccclp.generic_status );
		gcb->send.running = FALSE;
		gcb->send.state = GC_W_LOOK;
		break;
	    }

	    /*
	    ** Emulate BSP updating of buffer_ptr to end of data sent
	    ** and decrementing buffer_lng to amount left to send.
	    */
	    gcb->send.gccclp.buffer_ptr += gcb->send.gccclp.buffer_lng;
	    gcb->send.save_buffer_lng   -= gcb->send.gccclp.buffer_lng;
	    gcb->send.gccclp.buffer_lng  = gcb->send.save_buffer_lng;

            /* More to send? */

            if( gcb->send.gccclp.buffer_lng )
                goto writereg;

	    /* Write done. */

	    gcb->sending->state = GC_CHAN_DONE;
	    gcb->send.state = GC_W_LOOK;
	    goto top;

	writereg:
	    /* Issue a send op */

	    GCTRACE(3)( "GC_send_sm %d: issuing send for %d bytes\n", 
		    gcb->id, gcb->send.gccclp.buffer_lng );

	    gcb->send.save_buffer_lng = gcb->send.gccclp.buffer_lng;

	    /* Copy some parameters for network driver */

	    gcb->send.gccclp.function_invoked = GCC_SEND;
	    gcb->send.gccclp.pce        = pce;
	    gcb->send.gccclp.pcb        = gcb->GCdriver_pcb;
	    gcb->send.gccclp.compl_exit = (void (*)(PTR))GC_send_sm;
	    gcb->send.gccclp.compl_id   = (PTR)gcb;
	    gcb->send.gccclp.generic_status = OK;
	    gcb->send.syserr	    = &gcb->send.gccclp.system_status;

	    if( gcb->send.gccclp.buffer_lng )
	    {
		status = (pce->pce_protocol_rtne)(
		          gcb->send.gccclp.function_invoked,
		          &gcb->send.gccclp);
		if( status != OK )
		{
		    /*
		    ** If the status is NOT OK, then the send is done:
		    ** either there was an error or the send was
		    ** completed immediately.  In either case, the completion
		    ** exit will not be called back by the lower layer.
		    */
		    GCTRACE(3)( "GC_send_sm %d: issued send -- completed with status=%x\n", 
				gcb->id, gcb->send.gccclp.generic_status );
		    goto top;
		}
		GCTRACE(3)( "GC_send_sm %d: issued send -- pending\n", 
			    gcb->id );
		break;
	    }

	    goto top;
        }

	GC_drive_complete( gcb );
}


/*
** Name:  GCDriverPurge() - send two chop marks down the normal subchannel
**
** Description:
**	See comments at head of file.
**
** Called by:
**	GCA CL (GCpurge)
**
** History:
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Modified for Windows. Changed function name from GCpurge to
**	    GCDriverPurge.
**	    Get access to GCA_GCB via ASSOC_IPC->gca_gcb rather than
**	    directly from gc_cb in SVC_PARMS, which points to ASSOC_IPC
**	    on Windows.
*/

VOID
GCDriverPurge( SVC_PARMS *svc_parms )
{
	ASSOC_IPC	*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
	GCA_GCB		*gcb = (GCA_GCB *)asipc->gca_gcb;

	GCTRACE(3)( "%sGCDriverPurge %d\n", GC_trace > 2 ? "===\n" : "", gcb->id );

	svc_parms->status = OK;

	/* No queueing purges. */

	if( gcb->sendchops.state == GC_CHAN_ACTIVE )
		return;

	/* Note request. */

	gcb->sendchops.state = GC_CHAN_ACTIVE;

	/* Start the sender state machine, if not already running.  */

	if( !gcb->send.running )
	{
		gcb->send.running = TRUE;
		GC_send_sm( gcb );
	}
}


/*{
** Name: GCDriverDisconn() - Deassign all connections for an association
**
** Description:
**	Closes the connection.
**
** Called by:
**	GCA CL (GCdisconn)
**
** History:
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**	    GCA callback now takes closure from service parms.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Modified for Windows. Changed function name from GCdisconn to
**	    GCDriverDisconn.
**	    Get access to GCA_GCB via ASSOC_IPC->gca_gcb rather than
**	    directly from gc_cb in SVC_PARMS, which points to ASSOC_IPC
**	    on Windows.
*/

VOID
GCDriverDisconn( SVC_PARMS *svc_parms )
{
    ASSOC_IPC	*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    GCA_GCB	*gcb = (GCA_GCB *)asipc->gca_gcb;

    svc_parms->status = OK;

    /* 
    ** Not connected? 
    */
    if ( ! gcb )
    {
	(*svc_parms->gca_cl_completion)( svc_parms->closure );
	return;
    }

    GCTRACE(3)( "%sGCDriverDisconn %d\n", GC_trace > 2 ? "===\n" : "", gcb->id );

    /*
    ** Fail any outstanding requests: calling GCdisconn with 
    ** incomplete requests aborts those reqests.
    */
    GC_abort_recvs( gcb, GC_ASSN_RLSED );
    GC_abort_sends( gcb, GC_ASSN_RLSED );

    /*
    ** Start up disconnect state machine.
    */
    gcb->sendclose.svc_parms = svc_parms;
    gcb->sendclose.state = GC_CHAN_ACTIVE;
    GC_disc_sm( gcb );

    return;
}


/*{
** Name: GC_disc_sm() - disconnect state machine
**
** Description:
**	See comments at head of file.
**
** Called by:
**	GCDriverDisconn()
**	GC_disc_sm() via callback
**
** History:
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    All 'bsp' refs were changed to 'gccclp' (plus corresponding
**	    field names within the structure) since BS_PARMS interface
**	    is replaced with GCC_P_PLIST interface on Windows.
**	    Change register (GCdriver->regfd) to a GCC_DISCONNECT call for
**	    the GCC CL protocol driver.  Eliminate the BSP close and
**	    just check the return status at that point.  If disconnect
**	    was immediate (versus pending), handle at that point rather
**	    than waiting for completion exit to be called.
*/

# define GC_D_REL_WAIT		0
# define GC_D_RELEASE		1
# define GC_D_CLOSE_WAIT	2
# define GC_D_CLOSE		3

static char *gc_trd[] = { "REL_WAIT", "RELEASE", "CLOSE_WAIT", "CLOSE" };

static void
GC_disc_sm( GCA_GCB *gcb )
{
	GCC_P_PLIST	*gccclp = &gcb->close.gccclp;
	GCC_PCE		*pce    = (GCC_PCE *)gcb->GCdriver_pce;
	STATUS		status;


	GCTRACE(3)( "GC_disc_sm %d: <<<entered>>>\n", gcb->id );

top:

	GCTRACE(3)( "GC_disc_sm %d: state %s\n", 
		gcb->id, gc_trd[ gcb->close.state ] );

	switch( gcb->close.state )
	{
	case GC_D_REL_WAIT:
	    /*
	    ** If orderly release is supported, poll to send the indication.
	    ** Otherwise, just go right to the close ==> which is the
	    ** case on Windows calling GCC CL protocol drivers, so FORCE
	    ** state to GC_D_CLOSE_WAIT.
	    */

	    gcb->close.state = GC_D_CLOSE_WAIT;
	    goto top;
	
	    gcb->close.state = GC_D_RELEASE;

	    gcb->close.state = GC_D_CLOSE;
	    goto top;

	case GC_D_RELEASE:
	    /* Send orderly release indication */

	    gcb->close.state = GC_D_CLOSE_WAIT;
	    goto top;

	case GC_D_CLOSE_WAIT:

	    gcb->close.state = GC_D_CLOSE;

	    /* Copy some parameters for network driver */

	    gccclp->function_invoked = GCC_DISCONNECT;
	    gccclp->pce        = pce;
	    gccclp->pcb        = gcb->GCdriver_pcb;
	    gccclp->compl_exit = (void (*)(PTR))GC_disc_sm;
	    gccclp->compl_id   = (PTR)gcb;
	    gccclp->generic_status = OK;
	    gcb->close.syserr = &gcb->close.gccclp.system_status;

	    /* Close connection with GCC_DISCONNECT */

	    status = (pce->pce_protocol_rtne)(
		          gccclp->function_invoked,
		          gccclp);
	    if( status != OK )
	    {
		/*
		** If the status is NOT OK, then the disconnect is done:
		** either there was an error or the disconnect was
		** completed immediately.  In either case, the completion
		** exit will not be called back by the lower layer.
		*/
		goto top;
	    }

	    return;

	case GC_D_CLOSE:
	    /* Check status of disconnect/close */
	    if(gccclp->generic_status != OK)
	    {
		GCTRACE(1)( "GC_disc_sm %d: GCC_DISCONNECT failed %x system_status=%d\n", 
			        gcb->id,
			        gccclp->generic_status,
			        gccclp->system_status.callid);
	    }

	    gcb->sendclose.state = GC_CHAN_DONE;
	    break;
        }

	/* 
	**  Drive completion events.
	*/

	GC_drive_complete( gcb );
}


/*
** Name:  GC_drive_complete() - complete async portion of request
**
** Description:
**	Scans requests finishes up those marked as DONE:  touches
**	up return values in the SVC_PARMS and drives async completion
**	(sync completion done by GC_sync_complete).
**
** Inputs:
**	gcb	- control block
**
** Called by:
**	GCDriverReceive()
**	GC_recv_sm()
**	GCDriverSend()
**	GC_send_sm()
**	GCDriverDisconn()
**
** History:
**	10-Apr-97 (gordy)
**	    GCA callback now takes closure from service parms.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Drive successes before failures.
**	    Only set the new_chop flag if receive was successful.
*/

static void
GC_drive_complete( GCA_GCB *gcb )
{
	register struct subchan	*chan;
	register SVC_PARMS	*svc_parms;
	i4 			i;

	GCTRACE(3)( "GC_drive_complete %d: entered with completing=%d\n",
		    gcb->id, gcb->completing);
	/*
	** The "completing" field indicates the level of nesting within
	** completion routine callbacks that we are currently at.
	** A value of 0 indicates we are not nested and it is OK to
	** execute the completion routines. Any value > 0 indicates
	** a called completion routine invoked further GCA processing
	** which is now completing, in which case we want to return up
	** the stack and avoid calling more completion routines and 
	** getting nested even deeper in the stack.
	*/
	if( gcb->completing++ )
		return;

	/*
	** Look for active RECV, SEND, requests.
	**
	** Drive successful completions before unsuccessful ones.
	** This enables "cleaner" server-side disconnects by processing
	** a successful receive of a GCA_RELEASE message on the normal
	** channel before processing a failed expedited receive,
	** which can occur when the pending expedited receive request
	** is reissued but the other side of the connection has
	** already closed the socket.  Since the code below is
	** executed twice anyway due to "completing" field, process
	** successes in 1st pass, failures in 2nd.  Since failures
	** are now processed in 2nd pass and "completing" flag will
	** be zero at that time, bump it up if calling the completion
	** routine in case failure drives a disconnect back down into
	** this code; that will cause it to return at top and finish
	** below before completing the disconnect and freeing resources.
	** Ideally, it would probably be better to process the
	** completions in the order received; however, no situation
	** was encountered that required the extra complexity.
	*/

    again:
	for( i = GC_SUB; i--; )
	{
	    chan = &gcb->recv_chan[i];

	    if( ( chan->state == GC_CHAN_DONE ) &&
		( chan->svc_parms->status == 0 || gcb->completing == 0 ) )
	    {
		svc_parms = chan->svc_parms;

		svc_parms->rcv_data_length = 
		    chan->buf - svc_parms->svc_buffer;

		/*
		** Set the new chop flag only if receive was successful.
		** Eg, if connection dropped, expedited receive will fail
		** and length will be 0, but don't want to flag new_chop.
		*/
		if( svc_parms->status == OK)
		    svc_parms->flags.new_chop = !svc_parms->rcv_data_length;

		chan->state = GC_CHAN_QUIET;

		if( gcb->completing == 0 )
		    gcb->completing++;

		GCTRACE(2)( "GC_recv_comp %d: recv %d%s stat=%x%s svc_parms %p\n",
		    gcb->id, svc_parms->rcv_data_length,
		    gc_chan[ svc_parms->flags.flow_indicator ],
		    svc_parms->status, 
		    gc_chop[ svc_parms->flags.new_chop ],
		    svc_parms );

		(*( svc_parms->gca_cl_completion ) )( svc_parms->closure );

	    }

	    chan = &gcb->send_chan[i];

	    if( ( chan->state == GC_CHAN_DONE ) &&
		( chan->svc_parms->status == 0 || gcb->completing== 0 ) )
	    {
		svc_parms = chan->svc_parms;

		chan->state = GC_CHAN_QUIET;

		if( gcb->completing == 0 )
		    gcb->completing++;

		GCTRACE(2)( "GC_send_comp %d: sent %d%s status %x svc_parms %p\n",
		    gcb->id, svc_parms->svc_send_length, 
		    gc_chan[ svc_parms->flags.flow_indicator ],
		    svc_parms->status,
		    svc_parms );

		(*( svc_parms->gca_cl_completion ) )( svc_parms->closure );

	    }
	}

	if ( gcb->completing > 0 )
	{
		--gcb->completing;
		goto again;
	}

	/*
	** Check close request.
	*/

	chan = &gcb->sendclose;

	if( chan->state == GC_CHAN_DONE )
	{
		svc_parms = chan->svc_parms;

		chan->state = GC_CHAN_QUIET;

		GCTRACE(2)( "GC_close_comp %d: status %x svc_parms %p\n",
		    gcb->id, svc_parms->status, svc_parms );

		(*( svc_parms->gca_cl_completion ) )( svc_parms->closure );
	}
}

/*
**	All requests marked with ACTIVE
**	except the sendclose request are completed with the failing status.  
**	This aborts requests when an I/O fails.  The sendclose requests 
**	is not affected by I/O failure.
*/

static VOID
GC_abort_recvs( GCA_GCB *gcb, STATUS status )
{
	i4 i;

	/*
	** Look for active RECV requests.
	** For each, copy the status around and set the request up
	** for callback.
	*/

	for( i = GC_SUB; i--; )
	{
	    struct subchan *chan = &gcb->recv_chan[i];

  	    if( status == GC_TIME_OUT &&
		chan->svc_parms != NULL &&
	        chan->svc_parms->time_out == -1 )
	    {
		GCTRACE(1)( "GC_abort_recvs %d:%d status %x\n", gcb->id,
	           gc_chan[ chan->svc_parms->flags.flow_indicator ], status );
		continue;
	    }

	    if( chan->state == GC_CHAN_ACTIVE )
	    {
		/*
		** Remap Driver GCC_RECEIVE GCC CL status GC_RECEIVE_FAIL
		** to GC CL status of either GC_RCV1_FAIL or GC_ASSOC_FAIL;
		** latter case is when ER_close is set, which means the
		** socket is closed.
		** If incoming status is other than GC_RECEIVE_FAIL, then
		** caller already set it appropriately, so leave it "as is".
		*/
		if( status == GC_RECEIVE_FAIL )
		    if( gcb->recv.syserr->callid == ER_close )
		    {
			chan->svc_parms->status = NP_IO_FAIL;
			/*
			** SPECIAL CASE for DBMS! Defer calling completion
			** with failure of expedited channel when the
			** connection is being closed by the client.
			** The DBMS likely has just received a GCA_RELEASE
			** from the client, BUT he likely hasn't
			** had a chance to resume processing in the mainline,
			** which will issue GCA_DISASSOC.  Telling the
			** DBMS about the failed expedited receive before
			** GCA_DISASSOC starts causes very messy disconnects;
			** the DBMS sees it as an interrupt and reissues
			** an expedited GCA_RECEIVE plus other things ending
			** in an internal processing error being attempted
			** to be sent to the client (who is now long gone).
			** Another symptom is that the DBMS usually won't
			** shutdown (ingstop says it did, but dmfacp, dmfrcp,
			** and iidbms are still running).
			** Some attempts were made to fix the DBMS, but were
			** only partially helpful.  GC* servers do not have
			** the same problem.  This problem is unique to
			** Windows and is due to the fact that Windows
			** posts all available async IO completions before
			** returning to exit from WFMOex() {in CSsuspend
			** in particular). So, the completion for both the
			** GCA_RELEASE on normal channel and the failed
			** expedited receive completion (because client
			** closed the socket immediately after sending
			** GCA_RELEASE) occur before DBMS mainline gets
			** control back.  NOTE that deferring the failed
			** completion until later DEPENDS ON a GCdisconnect
			** being issued subsequent to this by the caller.
			** If that doesn't happen, it is conceivable that a
			** hang could occur, since the completion for the
			** failed expedited receive might never be called.
			*/
			if (i == GC_EXPEDITED)
			    break;
		    }
		    else
			chan->svc_parms->status = GC_RCV1_FAIL;
		else
			chan->svc_parms->status = status;
		chan->svc_parms->sys_err = gcb->recv.syserr; 
		chan->state = GC_CHAN_DONE;
	    }
	}
}

static VOID
GC_abort_sends( GCA_GCB *gcb, STATUS status )
{
	i4 i;

	/*
	** Look for active SEND requests.
	** For each, copy the status around and set the request up
	** for callback.
	*/

	for( i = GC_SUB; i--; )
	{
	    struct subchan *chan = &gcb->send_chan[i];

	    if( chan->state == GC_CHAN_ACTIVE )
	    {
		/*
		** Remap Driver GCC_SEND GCC CL status GC_SEND_FAIL
		** to GC CL status of either GC_SND1_FAIL or GC_ASSOC_FAIL;
		** latter case is when ER_close is set, which means the
		** socket is closed.
		** If incoming status is other than GC_SEND_FAIL, then
		** caller already set it appropriately, so leave it "as is".
		*/
		if( status == GC_SEND_FAIL )
		    if( gcb->send.syserr->callid == ER_close )
			chan->svc_parms->status = GC_ASSOC_FAIL;
		    else
			chan->svc_parms->status = GC_SND1_FAIL;
		else
			chan->svc_parms->status = status;
		chan->svc_parms->sys_err = gcb->send.syserr; 
		chan->state = GC_CHAN_DONE;
	    }
	}
}
