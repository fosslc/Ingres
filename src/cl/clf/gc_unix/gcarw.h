/*
**Copyright (c) 2004, 2009 Ingres Corporation
*/
/*
** Name: gcarw.h - UNIX GCA CL read/write state machine definition
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
**	15-Jul-09 (gordy)
**	    Enhanced GC_RQ_ASSOC to handle longer variable length names.
**	17-Dec-09 (Bruce Lunsford)  Sir 122536
**	    Changed max connect peer info message size from 512 to 1024
**	    to handle worst-case scenario of maximum lengths; also now
**	    agrees with comments and with Windows implementation.
**	15-Feb-2010 (kschendel) SIR 123275
**	    Crank buffer size up to 12K.
**	    12K shows a 10-15% improvement over 4K on large load times.
*/

# include <sl.h>
# include <cs.h>

/*
** Name: GC_PCT - protocol driver table 
*/

typedef struct _GC_PCT
{
    char	*prot;
    BS_DRIVER	*driver;
} GC_PCT;

/*
** Name: GC_RQ_ASSOC
**
** Description:
**	Information exchanged during connection establishment.
**
** History:
**	30-Jan-98 (gordy)
**	    Added version 1 and GC_RQ_ASSOC1 with flags for handling
**	    direct remote connections.
**	15-Jul-09 (gordy)
**	    Added version 2 and GC_RQ_ASSOC2 to allow longer variable
**	    length names.  GC_RQ_ASSOC is now union of GC_RQ_ASSOC*
**	    and 1K buffer representing current maximum length;
*/

typedef struct _GC_RQ_ASSOC0
{
    i4		length;
    i4		id;
    i4		version;
    char	host_name[ 32 ];
    char	user_name[ 32 ];
    char	term_name[ 32 ];

} GC_RQ_ASSOC0;

typedef struct _GC_RQ_ASSOC1
{
    GC_RQ_ASSOC0	rq_assoc0;
    i4			flags;

} GC_RQ_ASSOC1;

typedef struct _GC_RQ_ASSOC2
{
    i4		length;
    i4		id;
    i4		version;
    u_i4	flags;
    i4		host_name_len;		/* These lengths include EOS */
    i4		user_name_len;
    i4		term_name_len;

    /*
    ** This is a variable length structure.  Host name, user name,
    ** and terminal name immediately follow in order indicated.
    */

} GC_RQ_ASSOC2;

typedef union _GC_RQ_ASSOC
{
    GC_RQ_ASSOC0	v0;
    GC_RQ_ASSOC1	v1;
    GC_RQ_ASSOC2	v2;

    /*
    ** The following buffer provides storage space for variable length
    ** data associated with GC_RQ_ASSOC2.
    */
    u_i1		buffer[ 1024 ];

} GC_RQ_ASSOC;


#define GC_RQ_ASSOC_ID	0x47435251	/* 'GCRQ' (ascii big-endian) */

#define	GC_ASSOC_V0	0
#define GC_ASSOC_V1	1		/* Added flags */
#define	GC_ASSOC_V2	2		/* Names variable length */

#define	GC_RQ_REMOTE	0x01		/* Flags: Direct remote connection */


/*
** Name: GC_OLD_ASSOC
**
** Description:
**	Original information exchanged upon connection.
*/

typedef struct _GC_OLD_ASSOC
{
    char        user_name[32];
    char        account_name[32];
    char        access_point_id[32];
}   GC_OLD_ASSOC;

/*
** Name: GC_PEER_ORIG
**
** Description:
**	Peer info from the olden days when GC{send,recv}peer() 
**	existed and GCA and CL peer info were combined.
*/

typedef struct _GC_PEER_ORIG
{
    GC_OLD_PEER		gca_info;
    GC_OLD_ASSOC	gc_info;
}   GC_PEER_ORIG;

/*
** Name: GC_MTYP - CL level message header
*/

typedef struct _GC_MTYP 
{
	int chan;
	int len;
} GC_MTYP;

/*
** Name: GCA_CONNECT - info for peer info exchange state machine
**
** Description:
**	What would normally be automatic variables have to be stuffed
**	in an allocted structure for use with callbacks.  This structure
**	holds the data to get us through the peer info exchange.
*/

typedef struct _GCA_CONNECT
{
    u_i4		flags;

#define	GC_PEER_RECV	0x01		/* Peer info received and available */
#define GC_PEER_SEND	0x02		/* Need to send peer info */
#define GC_IS_REMOTE	0x04		/* Is remote access allowed?? */
	    
    VOID		(*save_completion)();	/* For hijacked svc_parms */
    PTR			save_closure;

    GC_MTYP		mtyp_room;	/* pad for GCsend */

    union 
    {
	GC_RQ_ASSOC	rq_assoc;	/* Association info */
	GC_PEER_ORIG	orig_info;	/* Original peer info */
    } assoc_info;

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
**
** History:
**	13-Jan-1989 (seiwald)
**	    Written.
*/

# define GC_SUB 	2	/* normal + exp data */
# define NORMAL		0	/* normal channel */
# define EXPEDITED	1	/* expedited channel */
# define GC_SIZE_ADVISE	12288	/* bigger is better, up to a point and then
				** you start wasting memory (especially in the
				** DBMS server).  12K seems to be close to
				** the diminishing-returns point as of 2010.
				*/

typedef struct _GCA_GCB 
{

	/* 
	** id - connection counter.
	** islisten - connection generated by GClisten().
	** completing - mutex for calling completion routines
	*/
	unsigned char id;
	unsigned char islisten;
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

		BS_PARMS	bsp;
		CL_ERR_DESC	syserr;
	} recv, send, close;

	/* mtyp - pointer into incoming buffer */

	GC_MTYP	*mtyp;

	/* ccb - connection requests control block */

	GCA_CONNECT ccb;

	/* bcb - byte stream I/O control structure.  */

	char bcb[ BS_MAX_BCBSIZE ];	

	/* buffer - receive buffer */

	char buffer[ GC_SIZE_ADVISE ];
	/*
	** Storage for session security label.
	*/
	SL_LABEL     sec_label;
} GCA_GCB ;
