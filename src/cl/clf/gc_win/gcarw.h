/*
**Copyright (c) 2010 Ingres Corporation
*/
/*
** Name: gcarw.h - WINDOWS GCA CL read/write state machine definition
**		   (for network protocol drivers, NOT for pipes)
**		   ** Copied from Unix version Oct 2009 **
**
** History:
**	10-Nov-89 (seiwald)
**	    CCB moved in wholesale.
**	    Flush flag reworked.
**	    Prevailing timeout moved into sm's.
**	    Send chops, flush, and close requests now have a struct subchan.
**	    New state flag in struct subchan, to control completion.
**	04-Dec-89 (seiwald)
**	    Added completing flag to GCA_GCB, to provide a simple mutex
**	    on calling completion routines.
**	01-Jan-90 (seiwald)
**	    Removed setaside buffer.
**	08-Jan-90 (seiwald)
**	    No more mustflush, sendflush.  Flushing occurs after every
**	    write.
**	19-Jan-90 (seiwald)
**	    Removed mainline's peer info from GC_RQ_ASSOC, so it can be
**	    used for other things.
**	12-Feb-90 (seiwald)
**	    Rearranged and compacted GCA_GCB.
**      24-May-90 (seiwald)
**	    Built upon new BS interface defined by bsi.h.
**	30-Jun-90 (seiwald)
**	    Changes for buffering: 1) GCB now has a read buffer of
**	    GC_SIZE_ADVISE bytes; 2) MTYP is now a typedef for ease of
**	    use; 3) put room for an MTYP before the sendpeer/recvpeer
**	    buffer, since GCsend and GCreceive now require it.
**	30-Jun-90 (seiwald)
**	    Private and Lprivate renamed to bcb and lbcb, as private
**	    is a reserved word somewhere.
**	1-Jul-90 (seiwald)
**	    Added GC_PCT.
**	24-Oct-90 (pholman)
**	    Add security label 'sec_label' to GCA_GCB (plus inclusion of sl.h)
**	04-Mar-93 (brucek)
**	    Added close sm struct to GCA_GCB.
**	30-mar-93 (robf)
**	    Remove xORANGE, make security labels generally available
**	16-jul-1996 (canor01)
**	    Add completing_mutex to GCA_GCB to protect against simultaneous
**	    access to the completing counter with operating-system threads.
**	 9-Apr-97 (gordy)
**	    GCA peer info is no longer referenced by the CL.  CL peer
**	    info now exchanged during GCrequest()/GClisten().  For 
**	    backward compatibility, added GC_PEER_ORIG representing 
**	    the original peer info exchange.  Reworked GCA_CONNECT 
**	    to support the original and new peer info protocols.
**	10-Apr-97 (gordy)
**	    Save closure in CONNECT control block when svc_parms hijacked.
**	15-Apr-97 (gordy)
**	    Implemented new request association data structure.
**      07-jul-1997 (canor01)
**          Remove completing_mutex, which was needed only for BIO
**          threads handling completions for other threads as proxy.
**	09-Sep-97 (rajus01)
**	    Added GC_IS_REMOTE flag in GCA_CONNECT.
**	30-Jan-98 (gordy)
**	    Added GC_RQ_ASSOC1 with flags for direct remote connections.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Copied from latest Unix version to be in sync with current
**	    gcarw.c, also copied from Unix, and used to implement
**	    "tcp_ip" as a local IPC protocol. Removed GC_PCT as it is
**	    not used on Windows. Also removed items pertaining to the
**	    peer info message which are instead contained in gclocal.h
**	    on Windows (in the ASSOC_IPC structure).
*/

# include <sl.h>
# include <cs.h>

/*
** Name: GC_MTYP - CL level message header
*/

typedef struct _GC_MTYP 
{
	i4 chan;
	i4 len;
} GC_MTYP;

/*
** Name: GCA_CONNECT - info for peer info exchange state machine
**
** Description:
**	What would normally be automatic variables have to be stuffed
**	in an allocted structure for use with callbacks.  This structure
**	holds the data to get us through the peer info exchange.
**
** History:
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Removed flags/GC_IS_REMOTE since this was moved to asipc
**	    struct as GC_IPC_IS_REMOTE so that it can be used by driver
**	    and by pipes.
**	    Remove GC_PEER_RECV/SEND flags since no support is provided
**	    for handling the pre-Ingres r3/2006 peer info protocol;
**	    this means, eg, that Ingres 2.6 Linux/Unix on Intel clients
**	    can't direct connect to Windows (which is reasonable since
**	    Ingres 2.6 no longer supported anyway).
*/

typedef struct _GCA_CONNECT
{
    VOID		(*save_completion)(PTR closure); /* For hijacked svc_parms */
    PTR			save_closure;

} GCA_CONNECT;


/*
** Name: GCA_GCB - GCA CL per association data structure
**
** Description:
**	This structure describes a connection for the GCA CL layer.
**
**	A connection to the GCA CL:
**		- is potentially full duplex
**		- multiplexes two subchannels (normal, expidited)
**		- is controlled by sender and receiver state machines.
**	This structure reflects that.
**
** History:
**	13-Jan-1989 (seiwald)
**	    Written.
**      13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add pointer (GCdriver_pcb) to driver-specific connection
**	    control block--required for each call into driver after
**	    connection setup.  Removed BSP-related field bcb as it is
**	    functionally replaced by the pcb.
**	    Add pointer (GCdriver_pce) to driver's entry in the PCT
**	    (Protocol Control Table).  Could use global GCdriver_pce
**	    instead but want to keep this per session and not use global.
**	    Removed sec_label because it is already defined in Windows
**	    gcacl connection block structure ASSOC_IPC.  This GCA_GCB
**	    structure is pointed to by ASSOC_IPC and is only allocated
**	    and used when network driver is being used for the local IPC
**	    protocol (versus pipes). Make syserr in sm struct a pointer
**	    rather than an actual structure, since actual struct of
**	    CL_ERR_DESC is now kept in the GCC_P_PLIST struct.
**	    Added some filler space for protocol driver to use for length
**	    prefix in front of the buffer area. Add buffer_guard at end
**	    for diagnosing buffer overruns.
**	    Remove unused islisten flag.
**	    Change default buffer size from 4096 to 12288 to match Unix
**	    and other related buffer/packet size changes in Windows GC CL.
*/


# define GC_SUB 	2	/* normal + exp data */
# define NORMAL		0	/* normal channel */
# define EXPEDITED	1	/* expedited channel */
# define GC_SIZE_ADVISE	12288	/* bigger is "usually" better */

typedef struct _GCA_GCB 
{
	PTR	GCdriver_pcb; /* ptr to driver-specific connection ctl block */
	PTR	GCdriver_pce; /* ptr to driver PCT entry */

	/* 
	** id - connection counter.
	** completing - mutex for calling completion routines
	*/
	unsigned char id;
	char completing;

	/*
	** user request data for each subchannel
	**	recv_chan[ 2 ] - normal + exp receive
	**	send_chan[ 2 ] - normal + exp send
	**	sendchops - send chop marks on normal subchannel (gag)
	**
	** 	svc_parms - used on completion
	**	state - state of request
	** 	buf, len - set on request
	**	      modified by GC_{recv,send}_sm
	**	      cleared on completion
	*/
	struct subchan 
	{
		SVC_PARMS	*svc_parms; 	
		char		*buf;
		char		state;
		i4		len;
	} recv_chan[ GC_SUB ], send_chan[ GC_SUB ], 
		sendchops, sendclose, *sending;

# define GC_CHAN_QUIET		0	/* no request */
# define GC_CHAN_ACTIVE		1	/* waiting for GC_{recv,send}_sm */
# define GC_CHAN_DONE		2	/* waiting for GC_drive_complete */

	/* 
	** read/write info for the connection
	**
	** state machine data
	**	state - state of recv/send state machine
	**	running - state machine alive on this channel
	**	timeout - prevailing timeout 
	**	bsp - service parms for BS operations
	**	syserr - for i/o errors
	*/

	struct sm
	{
		char		state;
		bool		running; 
		i4		timeout;
		i4		save_buffer_lng;
		GCC_P_PLIST	gccclp;
		CL_ERR_DESC	*syserr;
	} recv, send, close;

	/* mtyp - pointer into incoming buffer */

	GC_MTYP	*mtyp;

	/* ccb - connection requests control block */

	GCA_CONNECT ccb;

	/*
	** buffer - receive buffer
	**
	** This is a local buffer that may be used in lieu of the
	** buffer passed in by GCA.  GCA provides GCA_CL_HDR_SZ bytes
	** in front of the actual GCA message for use by GC CL; this
	** receive buffer area must do the same.  The GC CL multiplexed
	** code prefixes the GCA message with a GC_MTYP structure;
	** this is already accounted for within the "buffer" field
	** (same as Unix GCreceive code).  The protocol driver called
	** by the multiplexed code may prefix additional information;
	** currently, for streamed I/O (such as TCP/IP) a 2 byte prefix
	** is added.  For generality, a full GCA_CL_HDR_SZ space (minus
	** GC_MTYP which is already included within "buffer") is provided.
	*/

	char gcccl_prefix[ GCA_CL_HDR_SZ - sizeof(GC_MTYP)];
	char buffer[ GC_SIZE_ADVISE ];
	i4   buffer_guard;	/* Useful for detecting buffer overrun */
#define GCB_BUFFER_GUARD -1
	/*
	** Storage for session security label.
	*/
	SL_LABEL     sec_label;
} GCA_GCB ;
