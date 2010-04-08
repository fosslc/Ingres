# include <compat.h>
# include <gl.h>
# include <cs.h>
# include <pc.h>
# include <csinternal.h>
# include <qu.h>
# include <ex.h>
# include <me.h>
# include <tr.h>
# include <gc.h>

# include <bsi.h>
# include "gcarw.h"
# include "gcacli.h"

/*
** Name: GCARW.C - GCsend(), GCreceive(), GCpurge(), GCdisconn()
**
** Description:
**	Sender, Receiver
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
**	12-jan-89 (seiwald)
**		Written.
**	27-jun-89 (seiwald)
**	    Being a little more rigorous about checking the return from
**	    iiCLpoll().
**	25-jul-89 (seiwald)
**	    Be correct in the more rigorous checking of the return from
**	    iiCLpoll() - was wrongly resetting timeout when iiCLpoll
**	    returned OK.
**	17-aug-89 (seiwald)
**	    Leave state machine on read/write error - just call GC_abort() 
**	    and return.  This decreases (but does not eliminate) the chance
**	    of GCrelease() being called when state machines are on the stack.
**	23-aug-89 (seiwald)
**	    Leave sender state machine if only 1 request active.  This 
**	    further decreases (but does not eliminate) the chance of
**	    GCrelease() being called when state machines are on the stack.
**	04-Oct-89 (seiwald)
**	    Propagate CLpoll status upwards through GC_sync_complete for
**	    more revealing error messages.
**	10-Nov-89 (seiwald)
**	    Revamped for async GCdisconn().
**
**	    GCdisconn moved into gcarw.c.  GC_send_sm now handles closing
**	    connection, and calls svc_parms completion routine when done.  
**
**	    Sendchops, flush, and close requests represented by struct 
**	    subchan's, rather than oddball flags.
**
**	    Timeouts reworked: timeouts specified in GCreceive, GCsend,
**	    GCdisconn requests are placed in the receiver or sender 
**	    struct sm, and then copied by the state machines into the
**	    BS request.  BS hands these down to iiCLfdreg, so no timeout
**	    is needed in the call to iiCLpoll.  The state machines know
**	    to specify "no timeout" once an operation is started.
**
**	    Completions reworked: the state machines formerly called
**	    GC_recv_complete and GC_send_complete when the request was
**	    satisfied.  Unfortunately, this caused havoc when the user
**	    completion routines called GCrelease before returning.
**	    This left the state machines running without control
**	    structures.  Now, as requests complete the state machines
**	    mark them as such by changing the "state" flag from ACTIVE
**	    to DONE.  Just before the state machine returns, it calls
**	    GC_drive_complete to drive the completion routines of the
**	    requests marked with DONE.
**
**	    New flag "state" to indicate send/receive sm requests and
**	    control completion.  GCsend/receive/purge/disconn set
**	    state to ACTIVE, GC_send_sm and GC_recv_sm set state to
**	    DONE, and GC_drive_complete set state to QUIET.
**
**	    CCB moved into gcarw.h and allocated as part of the GCB.
**	14-Nov-89 (seiwald)
**	    Touch up signal catching: turn signals back on after
**	    GCdisconn; don't let GCpurge return with signals off.
**	16-Nov-89 (seiwald)
**	    Always drive all outstanding requests upon GCdisonn: they
**	    are aborted.
**	04-Dec-89 (seiwald)
**	    Prevent reentrance into GC_drive_complete by use of a
**	    completing flag in GCB.  Be careful to call the GCdisconn
**	    completion routine as the very last act in GC_drive_complete.
**	    This ensures that no completion routines will be called
**	    after GCdisconn completes.
**	22-Dec-89 (seiwald)
**	    GCA now buffers send/receives.  Allow GCrecieve to return
**	    with short reads.
**	28-Dec-89 (seiwald)
**	    Split GC_abort_all into GC_abort_recvs and GC_abort_sends,
**	    since an error on receive (such as timeout) does not imply
**	    error on send.  Fix spelling of "expedited."
**	30-Dec-89 (seiwald)
**	    Support for async frontends:  Flush after sending expedited
**	    data or chop marks.  Flushes normally occurred only after a
**	    new read was issued, but an async frontend issuing an
**	    attention message can't issue another read until the
**	    previous one completes.  (A sync frontend lets GCsend
**	    cancel the previous read and then issues a new one.) 
**	01-Jan-90 (seiwald)
**	    Removed setaside buffer.  It causes more problems than
**	    it solves, and it may not solve any as well.
**	05-Jan-90 (seiwald)
**	    Always flush after a send, since GCA buffers sends itself.
**	08-Jan-90 (seiwald)
**	    Simplified flushing & closing.  Since flushing now occurs 
**	    after every write, it control is simpler, and GCdisconn
**	    doesn't need to get involved.  GCdisconn runs sync again,
**	    but it retains its async interface.
**	31-Jan-90 (seiwald)
**	    Handle a few oddball conditions: when polling for expedited
**	    data and normal data arrives, timeout the expedited data.
**	    Call expedited completions before normal ones: this seems
**	    obvious, but wasn't being done.  Removed the call to GCpurge
**	    at the end of GC_send_sm: it never made sense and mainline
**	    now works without it.
**	12-Feb-90 (seiwald)
**	    Remove synchronous support.  GCA as of 6.4 handles syncing
**	    async requests.
**	26-Feb-90 (seiwald)
**	    SVC_PARMS sys_err now a pointer (to CL_ERR_DESC structure in
**	    GCA's parmlist).
**	21-Mar-90 (seiwald)
**	    GCpurge sends one chop mark.
**      24-May-90 (seiwald)
**	    Built upon new BS interface defined by bsi.h.
**	15-Jun-90 (seiwald)
**	    GCdisconn aborts outstanding I/O's with GC_ASSN_RLSED now.
**	18-Jun-90 (seiwald)
**	    GCpurge now takes a SVC_PARMS, not GCA_ACB.
**	30-Jun-90 (seiwald)
**	    Changes for buffering: 1) GCsend and GCreceive now
**	    require room for an MTYP preceeding the buffer they are
**	    handed; 2) GC_send_sm now sends buffers in one BS call,
**	    rather than one for the MTYP and one for the data; 3)
**	    GC_recv_sm now manages its own buffer in the GCB. 
**	30-Jun-90 (seiwald)
**	    Private and Lprivate renamed to bcb and lbcb, as private
**	    is a reserved word somewhere.
**	09-Aug-90 (seiwald)
**	    Cleaned up GC_send_sm a bit to keep it from looping on errors.
**	15-Aug-90 (seiwald)
**	    Fix bug found by Jeff: fall through code from cases GC_R_CHECK 
**	    to GC_R_FILL in GC_recv_sm was missetting the state.
**	5-Oct-90 (seiwald)
**	    Once again, send two chop marks on purging: 6.3 BE's seem to 
**	    eat them for lunch.
**	22-Oct-90 (seiwald)
**	    Added missing timeout_status parameter to GC_recv_sm() call.
**	23-Oct-90 (seiwald)
**	    Set gcb->send.bsp.syserr properly (to the send sm syserr).
**	27-Mar-91 (seiwald)
**	    GCpurge is VOID.
**	6-Apr-91 (seiwald)
**	    Be a little more assiduous in clearing svc_parms->status on
**	    function entry.
**	15-May-91 (seiwald)
**	    Removed references to accursed acb->flags.purging.
**	13-Oct-92 (gautam)
**	    Need to fill in bsp.lbcb entry when calling BS modules
**	02-Mar-93 (brucek)
**	    Made GCdisconn async, and added orderly release.
**      16-mar-93 (smc)
**          Fixed forward declarations.
**   	28-jun-93 (edg)
**	    Added forward declaration for GC_disc_sm().  Which looks like it
**	    was removed by last change (or perhaps integration?).  
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      11-aug-93 (ed)
**          added missing includes
**	02-Sep-93 (brucek)
**	    Set bsp->syserr in GC_disc_sm.
**	16-jul-1996 (canor01)
**	    Add completing_mutex to GCA_GCB to protect against simultaneous
**	    access to the completing counter with operating-system threads.
**	22-jul-1996 (canor01)
**	    For operating-system threads, each thread can use blocking I/O
**	    for more efficient processing.  The exception is the expedited
**	    channel, which is multiplexed with the normal channel.  We must
**	    pass the channel to the asynch registration function so it
**	    can deal with the expedited channel differently.
**	 9-Apr-97 (gordy)
**	    In order to support old style peer info exchanges which
**	    combined GCA and CL peer info, GCreceive() and GCsend()
**	    check for flags from GClisten() and handle the peer info
**	    on the first request (which GCA guarantees to be the (now
**	    separate from the CL) peer info exchange.
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**	    GCA callback now takes closure from service parms.
**      07-jul-1997 (canor01)
**          Remove completing_mutex, which was needed only for BIO
**          threads handling completions for other threads as proxy.
**	17-sep-1997 (canor01)
**	    Return GC_NO_PEER when a send fails with EPIPE error.
**	29-Dec-97 (gordy)
**	    Use client buffer for receives if GREATER THAN or equal
**	    to our preferred buffer size.
**	26-oct-1999 (thaju02)
**	    If data is detected on the expedited channel and a read
**	    is pending on the blocking (no timeout) normal channel, 
**	    the expedited channel goes into a suspend state, resulting
**	    in deadlock. Modified GC_recv_sm(). (b76625)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-Jan-04 (gordy)
**	    Replaced GC_NO_PEER (which is reserved for GCA) with
**	    GC_ASSOC_FAIL.
**	23-Jun-2004 (rajus01) startrak prob # INGNET 143; Bug # 112538.
**	    The terminal monitor hangs when it is connected to new Enterprise 
**	    access server (3.0/2.7) that handles DB events by issuing 
**	    asynchronous receives with a poll rate of 1 sec.
**	02-Jul-2004 (rajus01) Bug #112538
**	    Changed the comparison operator from & to == for the above change
**	    as suggested by Gordy.
**	8-Sep-2004 (schka24)
**	    Revert above, breaks COPY FROM: copy-from front-end does a
**	    zero-timeout receive to poll for msgs from the BE.  Above
**	    change causes subchan to stay active and we spin in a zero
**	    timeout poll loop.
**	8-Sep-2004 ( rajus01 )
**	    Put back the fix for Bug #112538. Don't abort the asynchronous
**	    receives when the poll timeout is negative only. 
**	    This fixes the COPY FROM problem above and works for Gateways
**	    as well.
**	14-Jan-2005 (schka24)
**	    Above caused Star segv's on an inactive subchan.  Amazing that
**	    two lines of code could cause so much trouble!
**       2-Feb-2010 (hanal04) Bug 123208
**          In GC_recv_sm() close a security vulnerability by checking
**          the supplied length is greater than zero.
*/

static void	GC_recv_sm( GCA_GCB *gcb, STATUS timeout_status );
static void	GC_send_sm( GCA_GCB *gcb );
static void	GC_disc_sm( GCA_GCB *gcb );
static void	GC_drive_complete( GCA_GCB *gcb );
static VOID	GC_abort_recvs( GCA_GCB *gcb, STATUS status );
static VOID	GC_abort_sends( GCA_GCB *gcb, STATUS status );

GLOBALREF	CS_SYSTEM	Cs_srv_block;

GLOBALREF	bool		iisynclisten;

GLOBALREF	BS_DRIVER	*GCdriver;
extern char     listenbcb[];            /* allocated listen control block */

static		GC_MTYP		GC_chopmarks[2] = { 0, 0, 0, 0 };
static 		char 		*gc_chan[] = { "", " EXPEDITED" };
static 		char 		*gc_chop[] = { "", " NEWCHOP" };


/*
** Name: GCreceive() - post a request to the receive state machine
**
** Description:
**	See comments at head of file.
**
** Called by:
**	GCA mainline
** History:
**	8-jun-93 (ed)
**	    -added VOID to match prototype
**	 9-Apr-97 (gordy)
**	    If there is peer info available from an old-style
**	    peer info exchange, it is used to satisfy the first
**	    receive request.
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
*/

VOID
GCreceive( SVC_PARMS *svc_parms )
{
	GCA_GCB		*gcb = (GCA_GCB *)svc_parms->gc_cb;
	register struct subchan	*subchan = &gcb->recv_chan 
				[ svc_parms->flags.flow_indicator ];
	 
	GCTRACE(1)( "%sGCreceive %d: want %d%s\n",
		gc_trace > 1 ? "===\n" : "",
		gcb->id, svc_parms->reqd_amount,
		gc_chan[ svc_parms->flags.flow_indicator ] );

	svc_parms->status = OK;

	/* Sanity check for duplicate request. */

	if( subchan->state != GC_CHAN_QUIET )
	{
		GCTRACE(1)( "GCreceive %d: duplicate request\n", gcb->id );
		GC_abort_recvs( gcb, GC_ASSOC_FAIL );
		GC_drive_complete( gcb );
		return;
	}

	if( !svc_parms->svc_buffer )
	{
		GCTRACE(1)( "GCreceive %d: null request\n", gcb->id );
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

	if ( gcb->ccb.flags & GC_PEER_RECV )
	{
	    /*
	    ** We have already received an old style peer info
	    ** packet.  We return the GCA portion of the packet
	    ** on the first receive request.  GCA must provide 
	    ** enough room to retrieve all the info.
	    */
	    u_i2 len = min( sizeof( gcb->ccb.assoc_info.orig_info.gca_info ),
			    subchan->len );

	    MEcopy( (PTR)&gcb->ccb.assoc_info.orig_info.gca_info,
		    len, (PTR)subchan->buf );

	    subchan->buf += len;
	    subchan->state = GC_CHAN_DONE;
	    gcb->ccb.flags &= ~GC_PEER_RECV;

	    GCTRACE(1)( "GCreceive %d: returning peer info\n", gcb->id );
	    GC_drive_complete( gcb );
	    return;
	}

	/* If nothing (more) to read, complete.  */

	if( !subchan->len )
	{
		subchan->state = GC_CHAN_DONE;
		GC_drive_complete( gcb );
	}

	/* Start the reader state machine, if not already running. */

	if( !gcb->recv.running )
	{
		gcb->recv.running = TRUE;
		GC_recv_sm( gcb, OK );
	}
# ifdef OS_THREADS_USED
	if ( iisynclisten && svc_parms->flags.flow_indicator == 0 )
		GC_recv_sm( gcb, OK );
# endif /* OS_THREADS_USED */

}


/*
** Name: GC_recv_sm() - receiver state machine
**
** Description:
**	See comments at head of file.
**
** Called by:
**	GCreceive()
**	GC_recv_sm() via callback
*/

# define GC_R_IDLE 0
# define GC_R_FILL 1
# define GC_R_CHECK 2
# define GC_R_LOOK 3

static char *gc_trr[] = { "IDLE", "FILL", "CHECK", "LOOK" };

static void
GC_recv_sm( GCA_GCB *gcb, STATUS timeout_status )
{
	i4		len;
	int		n;
	struct subchan *subchan;

	/* Copy some parameters */

	gcb->recv.bsp.func = GC_recv_sm;
	gcb->recv.bsp.closure = (PTR)gcb;
	gcb->recv.bsp.syserr = &gcb->recv.syserr;
	gcb->recv.bsp.bcb = gcb->bcb;
	gcb->recv.bsp.lbcb = listenbcb;
# ifdef OS_THREADS_USED
	if ( iisynclisten )
	    gcb->recv.bsp.regop = BS_POLL_INVALID; /* force blocking IO */
	else
	    gcb->recv.bsp.regop = BS_POLL_RECEIVE;
# else /* OS_THREADS_USED */
	gcb->recv.bsp.regop = BS_POLL_RECEIVE;
# endif /* OS_THREADS_USED */
	gcb->recv.bsp.status = OK;

	GCTRACE(3)( "GC_recv_sm %d: state %s\n", 
		gcb->id, gc_trr[ gcb->recv.state ] );

	/* Loop while requests outstanding. */

	while( gcb->recv_chan[ n = 0 ].state == GC_CHAN_ACTIVE ||
	       ( !iisynclisten && 
                 gcb->recv_chan[ n = 1 ].state == GC_CHAN_ACTIVE ) )
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
		gcb->recv.bsp.buf = subchan->buf - sizeof( GC_MTYP );
		gcb->recv.bsp.len = sizeof( gcb->buffer );
	    }
	    else
	    {
		/* Use our private buffer */
		gcb->recv.bsp.buf = gcb->buffer;
		gcb->recv.bsp.len = sizeof( gcb->buffer );
	    }

	    gcb->mtyp = (GC_MTYP *)gcb->recv.bsp.buf;

	    /* Go to register for read */

	    gcb->recv.state = GC_R_CHECK;
	    continue;

	case GC_R_FILL:
	    /* Check for timeout */

	    if( timeout_status != OK )
	    {
		gcb->recv.state = GC_R_IDLE;
		gcb->recv.bsp.status = GC_TIME_OUT;
		goto abort;
	    }

	    /* Issue BS read. */

	    (*GCdriver->receive)( &gcb->recv.bsp );

	    if( gcb->recv.bsp.status != OK )
		goto abort;

	    gcb->recv.state = GC_R_CHECK;

	    /* fall through */
		
	case GC_R_CHECK:
	    /* Get at least the mtyp and its data, or fill the buffer. */

	    len = gcb->recv.bsp.buf - (char *)gcb->mtyp;

	    if( len < sizeof( GC_MTYP ) ||
		len < sizeof( GC_MTYP ) + gcb->mtyp->len &&
		gcb->recv.bsp.len )
	    {
		/* Use timeout for first read */

		gcb->recv.bsp.timeout = len ? -1 : gcb->recv.timeout;
		gcb->recv.state = GC_R_FILL;

		/* Register for a read op */

		GCTRACE(3)( "GC_recv_sm %d: polling to read %d bytes\n", 
			gcb->id, gcb->recv.bsp.len );

		if( (*GCdriver->regfd)( &gcb->recv.bsp ) )
		    goto complete;

		timeout_status = OK;
		continue;
	    }

	    /* Got MTYP.  Check subchannel. */

	    if( gcb->mtyp->chan < 0 || gcb->mtyp->chan >= GC_SUB )
	    {
		GCTRACE(1)( "GCreceive %d: bad MTYP %d/%d\n", 
			gcb->id, 
			gcb->mtyp->chan, 
			gcb->mtyp->len );
		gcb->recv.bsp.status = GC_ASSOC_FAIL;
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
		    *(subchan->svc_parms->sys_err) = gcb->recv.syserr; 
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
		    *(subchan->svc_parms->sys_err) = gcb->recv.syserr;
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

		len = gcb->recv.bsp.buf - src;

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
		len = gcb->recv.bsp.buf - src;

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

		GCTRACE(3)( "GC_recv_sm %d: excess %d mtyp %d buffer\n", 
			gcb->id, gcb->mtyp->len, len );

		gcb->recv.bsp.buf = gcb->buffer;
		gcb->recv.bsp.len = sizeof( gcb->buffer );

		/* If any data left in mtyp, copy mtyp over */

		if( gcb->mtyp->len )
		{
		    MEcopy( (PTR)gcb->mtyp, sizeof( GC_MTYP ), 
			gcb->recv.bsp.buf );
		    gcb->recv.bsp.buf += sizeof( GC_MTYP );
		    gcb->recv.bsp.len -= sizeof( GC_MTYP );
		}

		/* Copy remaining user data data */

		if( len )
		{
		    MEcopy( (PTR)src, len, gcb->recv.bsp.buf );
		    gcb->recv.bsp.buf += len;
		    gcb->recv.bsp.len -= len;
		}

		/* Point base back to our buffer. */

		gcb->mtyp = (GC_MTYP *)gcb->buffer;
		gcb->recv.state = GC_R_CHECK;
		continue;
	    }

    abort:
	    GCTRACE(1)( "GC_recv_sm %d: BSread failed %x\n", 
		    gcb->id, gcb->recv.bsp.status );

	    GC_abort_recvs( gcb, gcb->recv.bsp.status );
	}

suspend:
	/* No outstanding requests - suspend state machine */

	GCTRACE(3)( "GC_recv_sm %d: suspend\n", gcb->id );

	gcb->recv.running = FALSE;

complete:
	GC_drive_complete( gcb );
}


/*
** Name: GCsend() - post request to sender state machine
**
** Description:
**	See comments at head of file.
**
** Called by:
**	GCA mainline
** History:
**	8-jun-93 (ed)
**	    -added VOID to match prototype
**	 9-Apr-97 (gordy)
**	    If our partner is expecting an old style peer info packet,
**	    combine CL peer info with contents of first send request.
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
*/

VOID
GCsend( SVC_PARMS *svc_parms )
{
	GCA_GCB		*gcb = (GCA_GCB *)svc_parms->gc_cb;
	struct subchan	*subchan = &gcb->send_chan
				[ svc_parms->flags.flow_indicator ];

	GCTRACE(1)( "%sGCsend %d: send %d%s\n",
		gc_trace > 2 ? "===\n" : "",
		gcb->id, svc_parms->svc_send_length,
		gc_chan[ svc_parms->flags.flow_indicator ] );

	svc_parms->status = OK;

	/* Sanity check for duplicate request. */

	if( subchan->state != GC_CHAN_QUIET )
	{
		GCTRACE(1)( "GCsend: duplicate request\n" );
		GC_abort_sends( gcb, GC_ASSOC_FAIL );
		GC_drive_complete( gcb );
		return;
	}

	/* Note request on subchannel.  */

	subchan->svc_parms = svc_parms;
	subchan->buf = svc_parms->svc_buffer;
	subchan->len = svc_parms->svc_send_length;
	subchan->state = GC_CHAN_ACTIVE;

	if ( gcb->ccb.flags & GC_PEER_SEND )
	{
	    /*
	    ** Our partner sent an old style peer info packet to us
	    ** and is expecting one in return.  We must combine the 
	    ** GCA info with ours.
	    */
	    MEcopy( (PTR)svc_parms->svc_buffer,
		    min( svc_parms->svc_send_length, 
			 sizeof( gcb->ccb.assoc_info.orig_info.gca_info ) ),
		    (PTR)&gcb->ccb.assoc_info.orig_info.gca_info );
	    subchan->len = sizeof( gcb->ccb.assoc_info.orig_info );
	    subchan->buf = (char *)&gcb->ccb.assoc_info.orig_info;
	    gcb->ccb.flags &= ~GC_PEER_SEND;
	    GCTRACE(1)( "GCsend %d: sending GCA and CL peer info\n", gcb->id );
	}

	/* Start the writer state machine, if not already running.  */

	if( !gcb->send.running )
	{
		gcb->send.running = TRUE;
		GC_send_sm( gcb );
	}
# ifdef OS_THREADS_USED
	if ( iisynclisten && svc_parms->flags.flow_indicator == 0 )
		GC_send_sm( gcb );
# endif /* OS_THREADS_USED */
}


/*
** GC_send_sm() - sender state machine
**
** Description:
**	See comments at head of file.
**
** Called by:
**	GCsend()
**	GCreceive()
**	GCdisconn()
**	GC_send_sm() via callback
**
** History:
**	16-Jan-04 (gordy)
**	    Replaced GC_NO_PEER (which is reserved for GCA) with
**	    GC_ASSOC_FAIL.
*/

# define GC_W_LOOK 0
# define GC_W_CHOP 1
# define GC_W_DATA 2

static char *gc_trs[] = { "LOOK", "CHOP", "DATA" };

static void
GC_send_sm( GCA_GCB *gcb )
{
	register int 	n;

	/* Copy some parameters */

	gcb->send.bsp.func = GC_send_sm;
	gcb->send.bsp.closure = (PTR)gcb;
	gcb->send.bsp.syserr = &gcb->send.syserr;
	gcb->send.bsp.bcb = gcb->bcb;
	gcb->send.bsp.lbcb = listenbcb;
	gcb->send.bsp.timeout = -1;
# ifdef OS_THREADS_USED
	if ( iisynclisten )
	    gcb->send.bsp.regop = BS_POLL_INVALID; /* force blocking IO */
	else
	    gcb->send.bsp.regop = BS_POLL_SEND;
# else /* OS_THREADS_USED */
	gcb->send.bsp.regop = BS_POLL_SEND;
# endif /* OS_THREADS_USED */
	gcb->send.bsp.status = OK;

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

		    gcb->send.bsp.buf = buf;
		    gcb->send.bsp.len = len + sizeof( GC_MTYP );
		    gcb->sending = &gcb->send_chan[ n ];

		    gcb->send.state = GC_W_DATA;
		    goto writereg;
	    }
	    else if( gcb->sendchops.state == GC_CHAN_ACTIVE )
	    {
		    /* Sending 2 chop marks.  Set up first.  */

		    gcb->send.bsp.buf = (char *)GC_chopmarks;
		    gcb->send.bsp.len = sizeof( GC_chopmarks );
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

	    /* Issue BS write to write chop mark or data. */

	    (*GCdriver->send)( &gcb->send.bsp );

            if( gcb->send.bsp.status != OK )
	    {
		GCTRACE(1)( "GC_send_sm %d: BSsend failed %x\n", 
			    gcb->id, gcb->send.bsp.status );

		/*
		** An error of EPIPE is a special case, usually
		** meaning that the connection is not valid.
		** The connection was probably not valid to begin with.
		*/
		if ( gcb->send.bsp.syserr->errnum == EPIPE )
		    gcb->send.bsp.status = GC_ASSOC_FAIL;

		GC_abort_sends( gcb, gcb->send.bsp.status );
		gcb->send.running = FALSE;
		break;
	    }

            /* More to send? */

            if( gcb->send.bsp.len )
                goto writereg;

	    /* Write done. */

	    gcb->sending->state = GC_CHAN_DONE;
	    gcb->send.state = GC_W_LOOK;
	    goto top;

	writereg:
	    /* Register for a send op */

	    GCTRACE(3)( "GC_send_sm %d: polling to send %d bytes\n", 
		    gcb->id, gcb->send.bsp.len );

	    if( gcb->send.bsp.len 
		&& (*GCdriver->regfd)( &gcb->send.bsp, n ) )
		break;

	    goto top;
        }

	GC_drive_complete( gcb );
}


/*
** Name:  GCpurge() - send two chop marks down the normal subchannel
**
** Description:
**	See comments at head of file.
**
** Called by:
**	GCA mainline.
**
** History:
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
*/

VOID
GCpurge( SVC_PARMS *svc_parms )
{
	GCA_GCB 	*gcb = (GCA_GCB *)svc_parms->gc_cb;

	GCTRACE(1)( "%sGCpurge %d\n", gc_trace > 2 ? "===\n" : "", gcb->id );

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
# ifdef OS_THREADS_USED
	if ( iisynclisten && svc_parms->flags.flow_indicator == 0 )
		GC_send_sm( gcb );
# endif /* OS_THREADS_USED */
}


/*{
** Name: GCdisconn() - Deassign all connections for an association
**
** Description:
**	Closes the connection.
**
** Called by:
**	GCA mainline.
**
** History:
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**	    GCA callback now takes closure from service parms.
*/

VOID
GCdisconn( SVC_PARMS *svc_parms )
{
    GCA_GCB	*gcb = (GCA_GCB *)svc_parms->gc_cb;
    BS_PARMS	bsp;

    svc_parms->status = OK;

    /* 
    ** Not connected? 
    */
    if ( ! gcb )
    {
	(*svc_parms->gca_cl_completion)( svc_parms->closure );
	return;
    }

    GCTRACE(1)( "%sGCdisconn %d\n", gc_trace > 2 ? "===\n" : "", gcb->id );

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
**	GCdisconn()
**	GC_disc_sm() via callback
*/

# define GC_D_REL_WAIT		0
# define GC_D_RELEASE		1
# define GC_D_CLOSE_WAIT	2
# define GC_D_CLOSE		3

static char *gc_trd[] = { "REL_WAIT", "RELEASE", "CLOSE_WAIT", "CLOSE" };

static void
GC_disc_sm( GCA_GCB *gcb )
{
	BS_PARMS	*bsp = &gcb->close.bsp;

	bsp->func = GC_disc_sm;
	bsp->closure = (PTR)gcb;
	bsp->syserr = &gcb->close.syserr;
	bsp->bcb = gcb->bcb;
	bsp->lbcb = listenbcb;
	bsp->status = OK;

	GCTRACE(3)( "GC_disc_sm %d: <<<entered>>>\n", gcb->id );

top:

	GCTRACE(3)( "GC_disc_sm %d: state %s\n", 
		gcb->id, gc_trd[ gcb->close.state ] );

	switch( gcb->close.state )
	{
	case GC_D_REL_WAIT:
	    /*
	    ** If orderly release is supported, poll to send the indication.
	    ** Otherwise, just go right to the close.
	    */

	    if ( !GCdriver->release )
	    {
	        gcb->close.state = GC_D_CLOSE;
		goto top;
	    }
	
	    gcb->close.state = GC_D_RELEASE;
# ifdef OS_THREADS_USED
	    if ( iisynclisten )
	        bsp->regop = BS_POLL_INVALID; /* force blocking IO */
	    else
	        bsp->regop = BS_POLL_SNDREL;
# else /* OS_THREADS_USED */
	    bsp->regop = BS_POLL_SNDREL;
# endif /* OS_THREADS_USED */
	    if( (*GCdriver->regfd)( bsp ) )
		return;

	    gcb->close.state = GC_D_CLOSE;
	    goto top;

	case GC_D_RELEASE:
	    /* Send orderly release indication */

	    (*GCdriver->release)( bsp );
	
	    gcb->close.state = GC_D_CLOSE_WAIT;
	    goto top;

	case GC_D_CLOSE_WAIT:
	    /* Poll for the release confirmation */

	    gcb->close.state = GC_D_CLOSE;
# ifdef OS_THREADS_USED
	    if ( iisynclisten )
	        bsp->regop = BS_POLL_INVALID; /* force blocking IO */
	    else
	        bsp->regop = BS_POLL_RCVREL;
# else /* OS_THREADS_USED */
	    bsp->regop = BS_POLL_RCVREL;
# endif /* OS_THREADS_USED */
	    if( (*GCdriver->regfd)( bsp ) )
		return;

	    goto top;

	case GC_D_CLOSE:
	    /* Close connection with BS close */

	    (*GCdriver->close)( bsp );

	    if( bsp->status == BS_INCOMPLETE )
	    {
	        gcb->close.state = GC_D_CLOSE_WAIT;
	        goto top;
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
**	GCreceive()
**	GC_recv_sm()
**	GCsend()
**	GC_send_sm()
**	GCdisconn()
**
** History:
**	10-Apr-97 (gordy)
**	    GCA callback now takes closure from service parms.
*/

static void
GC_drive_complete( GCA_GCB *gcb )
{
	register struct subchan	*chan;
	register SVC_PARMS	*svc_parms;
	i4 			i;

	if( gcb->completing++ )
		return;

	/*
	** Look for active RECV, SEND, requests.
	*/

    again:
	for( i = GC_SUB; i--; )
	{
	    chan = &gcb->recv_chan[i];

	    if( chan->state == GC_CHAN_DONE )
	    {
		svc_parms = chan->svc_parms;

		svc_parms->rcv_data_length = 
		    chan->buf - svc_parms->svc_buffer;
		svc_parms->flags.new_chop = !svc_parms->rcv_data_length;

		chan->state = GC_CHAN_QUIET;

		GCTRACE(2)( "GC_recv_comp %d: recv %d%s stat=%x%s\n",
		    gcb->id, svc_parms->rcv_data_length,
		    gc_chan[ svc_parms->flags.flow_indicator ],
		    svc_parms->status, 
		    gc_chop[ svc_parms->flags.new_chop ] );

		(*( svc_parms->gca_cl_completion ) )( svc_parms->closure );

	    }

	    chan = &gcb->send_chan[i];

	    if( chan->state == GC_CHAN_DONE )
	    {
		svc_parms = chan->svc_parms;

		chan->state = GC_CHAN_QUIET;

		GCTRACE(2)( "GC_send_comp %d: sent %d%s status %x\n",
		    gcb->id, svc_parms->svc_send_length, 
		    gc_chan[ svc_parms->flags.flow_indicator ],
		    svc_parms->status );

		(*( svc_parms->gca_cl_completion ) )( svc_parms->closure );

	    }
	}

# ifdef OS_THREADS_USED
	if ( gcb->completing > 0 )
	{
		--gcb->completing;
		goto again;
	}
# else /* OS_THREADS_USED */
	if( --gcb->completing )
		goto again;
# endif /* OS_THREADS_USED */

	/*
	** Check close request.
	*/

	chan = &gcb->sendclose;

	if( chan->state == GC_CHAN_DONE )
	{
		svc_parms = chan->svc_parms;

		chan->state = GC_CHAN_QUIET;

		GCTRACE(2)( "GC_close_comp %d: status %x\n",
		    gcb->id, svc_parms->status );

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
		GCTRACE(2)( "GC_abort_recvs %d:%d status %x\n", gcb->id,
	           gc_chan[ chan->svc_parms->flags.flow_indicator ], status );
		continue;
	    }

	    if( chan->state == GC_CHAN_ACTIVE )
	    {
		chan->svc_parms->status = status;
		*(chan->svc_parms->sys_err) = gcb->recv.syserr; 
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
		chan->svc_parms->status = status;
		*(chan->svc_parms->sys_err) = gcb->send.syserr; 
		chan->state = GC_CHAN_DONE;
	    }
	}
}
