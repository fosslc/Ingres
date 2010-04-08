/*
** Copyright (c) 1987, 2009 Ingres Corporation
**
*/

#include    <compat.h>
#include    <cs.h>
#include    <gl.h>
#include    <qu.h>
#include    <ex.h>
#include    <cm.h>
#include    <cv.h>
#include    <id.h>
#include    <si.h>
#include    <bsi.h>
#include    <lo.h>
#include    <nm.h>
#include    <me.h>
#include    <pc.h>
#include    <gc.h>
#include    <sl.h>
#include    <st.h>
#include    <tr.h>
#include    <er.h>
#include    <pm.h>
#include    <pccl.h>
#include    <descrip.h>
#include    <accdef.h>
#include    <dvidef.h>
#include    <iledef.h>
#include    <iodef.h>
#include    <iosbdef.h>
#include    <jpidef.h>
#include    <syidef.h>
#include    <lnmdef.h>
#include    <psldef.h>
#include    <ssdef.h>
#include    <gcatrace.h>
#include    <rmsdef.h>
#include    <starlet.h>
#include    <lib$routines.h>
#include    <efndef.h> 
# include       <ucxinetdef.h>
# include   <vmsclient.h>
# include   <uaidef.h>
#ifdef OS_THREADS_USED
#include    <ssdef.h>
#include    <stsdef.h>
#include    <ints.h>
#include    <gen64def.h>
#include    <builtins.h>
#include    <astjacket.h>
#endif

/*
**
** Name: GCACL.C - GCA private VMS CL routines
**
** Description:
**
**  The following GCA_CL entry routines are contained in this module;
**  the ones with _'s in the names are private to this module:
**
**  GCinitiate		- perform CL level initialization
**  GCterminate		- Perform necessary local termination functions
**  GCregister		- Establish server identity and prepare for "listen"
**  GClisten		- Make server available for connection to peer entity
**  GClisten_complete	- completion of GClisten
**  GCrequest 		- Build connection to peer entity
**  GCrequest_complete	- GCrerequest completion
**  GCsendpeer		- Send peer info
**  GCrecvpeer		- Receive partner's peerinfo
**  GCsend		- Send data over local IPC connection
**  GCsend_complete	- GCsend completion
**  GCreceive		- Receive data over local IPC connection
**  GCreceieve_complete	- GCreceive completion
**  GCpurge		- Cancel outstanding requests and send purge mark
**  GCdisconn		- Force completion of outstanding requests and close
**  GCrelease		- Release connection resources.
**  GCsave		- Save system-dependent data structures
**  GCrestore		- Restore system-dependent data structures
**  GCusrpwd		- Check username/password
**  GCnsid		- Get/set name server listen id
**  GChostname		- return host name for logging purposes
**  GCusername		- return effective user name
**
**  Utility routines:
**
**  GCinitasipc 	- initialize ASSOC_IPC
**  GCgetsym		- get a symbol with installation code in it
**  GCtim_strtoms	- convert a string time value into milliseconds
**  GCtim_mstotim	- convert milliseconds to a bintim
**  GCsignal_startup	- send message to iirundbms, success full startup
**
**  SYS$QIO routines:
**
**  GCsync		- Drive sync GCqio_exec.
**  GCqio_exec		- initiate a sys$qio/sys$timer operation
**  GCqio_timeout	- timer AST completion handler
**  GCqio_complete	- I/O AST completion handler
**  GCqio_status	- get/evaluate GCqio completion status
**  GCqio_pending	- returns the number "pending" I/O
**  GCqio_zombie	- timer AST that checks for zombie associations
**
**  Mailbox routines:
**
**  GCmbx_assoc		- establish the client/server association
**  GCmbx_assign	- assign to a mailbox
**  GCmbx_client	- create client's mailboxes
**  GCmbx_detect	- detect if a datagram "connection" is still up
**  GCmbx_servers	- create a server mailbox
**  GCmbx_userterm	- get user/terminal of a mailbox owner
**
**
** ---- Selections from the GCA CL spec. ----
**
**
** Overview
**
** This section specifies the CL routines which are part of the General
** Communication Facility (GCF).
**
** GCF has three components: GCA is the Application Program Interface
** (API), GCN is the Name Translation Service (Name Server), and GCC is
** the Communication Service (Comm Server).  GCN is fundamental to the
** functioning of GCA, and together they provide support of local
** homogeneous API communications.  The GCC component provides the ability
** to bridge two dissimilar local API communications environments via
** separate heterogeneous communications conducted between them.
**
** The CL routines described in this chapter fall into two separate
** categories: those associated with GCA local API communications, and
** those associated with GCC remote heterogeneous communications.  They
** are known as the GCA CL and the GCC CL, respectively, and collectively
** as the GC CL.  They are the means by which the components of GCF
** interface to a particular execution environment.  The GCA CL and the
** GCC CL are specified separately.
**
** In contrast to other CL modules, the GC CL routines provide two
** different header files: <gc.h> and <gcccl.h>.  These files are constant
** across all implementations of the same release level.
**
** "GCA CL"
**
** The GCA CL contains two types of routines: housekeeping routines and
** the IPC (interprocess communication) support. The housekeeping routines
** are described only in the interface section below.
**
** The IPC support provides an interface between GCA and the preferred IPC
** mechanism supported by the OS.  The interface to the local IPC is some
** dozen routines, most of which take a single parameter list (called
** SVC_PARMS).
**
** "Intended Uses"
**
** The GCA CL routines are intended for use only by the GCA and GCN
** components of GCF.  They provide functional support for the
** various aspects of managing interprocess communication.  They
** are not intended for use outside of GCF.
**
** "Connection oriented"
**
** The GCA CL provides a connection oriented, intra-host, reliable,
** IPC (interprocess communication) mechanism, presumably built upon an
** IPC mechanism provided by the operating system.
**
** The mechanism is said to be connection oriented because it establishes
** a connection, or "association," between two peer processes.  The peer
** processes send and receive data over this association until it is
** dismantled.  Data sent by one process is reliably received by the
** other.  The initiator of the connection is known as the client process
** and the responder is known as the server process (even though a server
** process may be an initiator on one association and a responder on
** another).  The initiator makes a connection request with GCrequest; the
** responder receives a connection indication with GClisten.  These
** connections are called "GCA" connections.
**
** When a process establishes an association, the GCA CL identifies the
** association with a pointer to the GCA CL's local control block (LCB)
** for the association.  GCA passes that LCB pointer to all GCA CL
** routines for the operations it requests on that association.  A process
** may have multiple, concurrent GCA associations, and they should not
** interfere.  The maximum number of concurrent associations is determined
** only by what the GCA CL can (and will) bear.
**
** Since an association is connection oriented, the GCA CL must have
** connection awareness; that is it must be able to determine if its partner has
** exited, abruptly or otherwise, and cause to fail any pending or future
** I/O requests made on that association.
**
** The GCA CL IPC mechanism is said to be intra-host because it is only
** required to build associations between processes on the same host.
**
** The mechanism is said to be reliable because either a requested data
** transfer succeeds or an (unrecoverable) error is reported to both
** parties.  Additionally, data are presented to the receiver in the same
** order they are sent by the sender.
**
** It is recommended that the GCA CL be built upon a connection oriented
** OS IPC mechanism rather than, say, mailboxes.  If the GCA CL uses a
** connection-less IPC mechanism, it will have to provide (usually at no
** small cost) connection control.
**
** "Listen ID"
**
** Server processes are identified through their Listen ID.  A Listen ID
** is generated by the GCA CL when GCA calls GCregister.  GCA then
** normally registers this Listen ID with the Name Server.  The Listen ID
** of the target server process is the addressing information GCA passes
** GCrequest on connection initiation.
**
** It is important that Listen ID's not be reused when one server exits
** and another starts.
**
** "Byte stream"
**
** A GCA CL association is a full duplex, two channel, mostly byte-stream
** oriented connection.
**
** As a GCA CL association is full duplex, it must permit simultaneous
** data transmission in both directions.
**
** A GCA CL association must support two subchannels: NORMAL and
** EXPEDITED.  A send of data on one channel can only be received by the
** peer process issuing a receive on that same channel.  The name
** "EXPEDITED" does not necessarily imply more rapid travel than NORMAL
** data, but only that EXPEDITED data must not lag NORMAL data.  That is,
** EXPEDITED data sent before NORMAL data must be guaranteed to arrive
** before that NORMAL data.  If the OS IPC can not support a separate
** subchannel for EXPEDITED data or the above rules cannot be guaranteed,
** the GCA CL must multiplex NORMAL and EXPEDITED data.  This can cause
** the EXPEDITED data to be flow-controlled with the NORMAL data by the
** underlying IPC, somewhat defeating the purpose of EXPEDITED data, but
** that is deemed to be acceptable.
**
** Each of the two subchannels of a GCA CL association is byte-stream
** oriented, which means that although data must arrive at the receiver in
** the same order as it was sent, the size of the buffer sent need not be
** preserved on receipt.  For example, the GCsend routine may be asked
** to send 100 bytes, after which the peer's GCreceive may return with
** the first 75 bytes on one invocation and then the remaining 25 on the
** next invocation.
**
** For compatibility reasons, the GCA CL must support synchronization
** marks on the NORMAL subchannel.  These are called "chop marks" and are
** generated by the GCpurge routine.  A chop mark flows in-order on the
** NORMAL subchannel, is distinguishable by the GCA CL from other data,
** and causes the partner's NORMAL GCreceive to return with a "new chop"
** indicator (a flag in the parameter list).  The transmission of a chop
** mark implies that NORMAL data sent before the chop mark can be
** discarded (purged), but in practice it is difficult to guarantee the
** ordering of NORMAL and EXPEDITED data if a chop mark purges the NORMAL
** channel.
**
** Both the multiplexing of NORMAL and EXPEDITED data as well as the
** requirement for chop marks will complicate the implementation of the
** GCA CL.  In particular, even though the GCA CL presents a byte stream
** interface and the OS IPC mechanism might be byte stream oriented, the
** GCA CL implementation cannot simply be pass-through.  The GCA CL
** will have to bundle the data into messages with headers indicating the
** subchannel and chop mark presence.
**
** "Asynchronous Interface"
**
** All I/O operations of the GCA CL must function asynchronously; that is,
** return without blocking and signal completion by calling a "callback"
** routine. Under this scheme, GCA makes a request by
** calling a GCA CL routine, passing it a parameter block containing
** (among other things) the parameters of the operation and the address of
** a callback routine.  When the operation completes within the CL, the
** GCA CL calls the callback routine, passing it the parameter block.
**
** An I/O event handling mechanism specific to the CL or OS is responsible
** for carrying the operation between request and callback.
**
** The event mechanism must not interrupt one callback routine to run
** another: once a callback routine is called, the event mechanism must
** wait until the call returns before driving another callback.
** Further, in multithreading servers, the event mechanism must not
** allow a callback to be interrupted to run another thread.
** That is, callback routines run atomically.
**
** To facilitate CL's where flow of control must pass to the event
** handling mechanism for it to drive its events (i.e. the OS does not
** deliver software interrupts when events complete), GCA will call the
** event handling mechanism in a manner dependent (unfortunately) on the
** application:
**
** For synchronous, single threaded applications, GCA will call the routine
** GCsync when it wishes to wait for outstanding operations to complete.
** In this case, GCA will post one or more asynchronous requests, and then
** repeatedly call GCsync until all requested operations complete.
**
** The asynchronous, multithreading GCF applications GCC (Comm Server)
** and GCN (Name Server) will call GCexec after posting the first few
** asynchronous requests.  GCexec is expected to loop driving those
** requests, the callbacks of which will post further asynchronous
** requests.  GCexec continues until GCshut is called.
**
** The INGRES DBMS and STAR servers have their own event handling
** mechanism, the CS CL, which must be arranged to provide the
** functionality of GCexec (or to call GCexec directly).
**
** Implementation of the GCA CL is directed by its asynchronous callback
** interface:
**
** The CL routines must return instead of blocking, leaving information
** with the event mechanism about how to carry on the operation.  Work is
** split between the request side of the operation and the completion
** side of the operation.  The completion side of the operation may
** involve intermediate callbacks from the underlying IPC mechanism,
** terminated finally by driving the GCA CL user's callback.
**
** Because the CL routines return before completing, state information
** cannot be left on the stack in automatic variables.  All information
** associated with the operation must either be placed in the parameter
** list by GCA or in the control block for the association by the GCA CL.
**
** Because the callback routine may post other requests to the CL, even
** a request to terminate the assocation, once the CL completes and calls
** its callback routine it should no longer access the parameter list or
** the control block.
**
** Since status is not returned when an asynchronous request is posted, the
** callback must always be driven, for success or failure.  It is an
** unrecoverable program error to call the GCA CL without providing a
** usable parameter list and callback routine.
**
** Some GCA CL routines retain a synchronous interface.
** The method by which these pass status to their callers varies.
**
** "Memory Allocator"
**
** The GCA CL is obliged to use the GCA allocator provided when GCA calls
** GCinitiate.  This allocator has the following semantics:
**
** It relies on the GCA CL routine GCrelease to free memory which the GCA CL
** allocated for a particular association.  The allocator does not automatically
** free any memory.
**
** It relies on the GCA CL routine GCterminate to free any remaining
** memory allocated by the GCA CL since GCinitiate was called.
**
** It (allocate or free) cannot be called by callback routines.  All
** memory needed for a GCA CL operation must be allocated when the request
** is made.  Further, since the callbacks of some routines may then invoke
** other CL routines, only the routines listed below may allocate or free
** memory.
**
** The following routines may call the allocator, as they are only
** invoked by top-half (non-callback) code:
**
**     GCexec
**     GChostname
**     GCinitiate
**     GClisten
**     GCnsid
**     GCregister
**     GCrelease
**     GCrequest
**     GCrestore
**     GCsave
**     GCshut
**     GCsync
**     GCterminate
**
** The following routines may not call the allocator, as they may be
** invoked as part of the completion of another routine:
**
**     GCdisconn
**     GCpurge
**     GCreceive
**     GCsend
**     GCusrpwd
**
** "Timeouts"
**
** Most connection related routines take a timeout parameter.
** A timeout of -1 means wait forever if necessary; a timeout of 0
** indicates the operation should fail if it can't be completed
** without blocking, and a positive timeout indicates the number
** of milliseconds to allow the operation to block before failing.
**
** If an operation blocks multiple times before completing the timeout
** applies to the cumulative time while blocked and (if possible to compute)
** while running.  The exception is GCsend and GCsendpeer: once they
** send a single byte of data they should no longer be allowed to time out.
**
** The only (non -1) timeouts that the CL must support are a 0 timeout for
** GCreceive (to effect a poll).  The CL may optionally support a positive
** timeout for GCrequest and GCreceive to allow for connection timeout,
** and a 0 or positive timeout for GCdisconn to indicate an abortive
** release.  (Other timeouts are currently unused.)
**
** "Buffer Size"
**
** GCrequest and GClisten, the GCA CL routines which establish
** associations, may indicate to GCA a preferred buffer size for
** use in sending and/or receiving messages.  This buffer size need NOT be
** the same for the two partners in an association.  It also need NOT be
** the same for different associations maintained by the same process.
**
** For environments where data alignment is important, the
** buffer size provided must always be a multiple of
** sizeof(ALIGN_RESTRICT).
**
** "Save/Restore"
**
** The GCA CL must provide a means, through the routines GCsave and
** GCrestore, to pass an existing association from a parent process to a
** child process.
**
** "Security"
**
** The GCA CL has the responsibility for enforcing
** certain aspects of INGRES security.  In particular it
** must:
**
** Supply in GClisten the OS user name of the process which made
** the initiating GCrequest.   This name is trusted by the DBMS;
** local security is only as strong as GClisten's ability to
** authenticate the user.
**
** Supply in GClisten the access point identifier (terminal name) of the
** process which made the initiating GCrequest.  This identifier is
** not validated: it is only displayed by monitor programs.
**
** Validate user/password pairs against the OS password file,
** to validate connections from remote hosts.  If the GCA CL
** treats all user/password pairs as valid, potentially forged
** connection requests from remote hosts will all be trusted.
**
** "Status Mapping"
**
** Currently, GCA requires that the internal GCA CL status codes map
** one-to-one with the GCA client status codes defined in <gca.h>.  CL
** implementors should be aware of these and should exercise caution when
** assigning values to <gc.h> status values.
**
** "Interversion interoperability"
**
** As much as possible, GC routines should be able to interoperate with
** both older and newer versions of the same routines.  GC routines must
** be upwardly compatibly with previous versions wherever possible, to
** minimize the need for customers to have to relink and rebuild their
** applications when moving between INGRES releases.
**
**
** ---- VMS IMPLEMENTATION NOTES ----
**
** Each VMS GCA CL connection uses 4 VMS mailboxes: normal and expedited
** send and receive.  They are created (sy$crembx) by the client (initiator)
** and assigned (sys$assign) by the server (listener).  See the GCmbx_*()
** routines.
**
** Because mailboxes are "connectionless," the CL must periodically count
** the number of processes attached to each active connection.  If the
** number is < 2, the CL causes the connection to fail.  See GCqio_zombie.
** "VMS Mail Box Specific Issues"
**
** The "size_advise" propagated up to GCA main line is computed from LPRSIZE.
** See GCinitiate.  Expedited channels privately use their own size of
** SZ_RIBUFFER.
**
** GCreceive expects to be called with the maximum size buffer possible,
** as neither it nor the mailbox driver hold data sent but not ask for
** on receive.
**
** A server has a listen mailbox associated into which clients write their
** connection requests (in the form of a GC_RQ_ASSOC structure, which includes
** the client -> server peerinfo buffer).  The default BUFQUOTA size of this
** mailbox is large enough to receive 32 peerinfo packets; it can be adjusted
** via "II_SVRxx_LSN_QUE".
**
** A client makes a connection by attaching to the server's listen mailbox
** in GCrequest, loading its 4 mailbox names into a GC_RQ_ASSOC buffer,
** along with mainline peerinfo, and writing the buffer into the server
** mailbox in GCsendpeer.  The server picks up the connection indication
** by reading the mailbox in GClisten and GCreceive and sends back a
** response with its GCsend.  The client reads the response with GCreceive.
**
** This peerinfo exchange is compatible with the original VMS 6.1/01 release.
**
** A timeout of 120 seconds, adjustable with "II_CONNECTxx_TIMEOUT",
** applies to GCsendpeer and GCrecvpeer.  In this way, if the server
** doesn't respond a client doesn't hang forever.
**
** Normally, I/O's are posted with sys$qio.  The completion is indicated
** by the deliver of an AST.
**
** ^C's: for sync front ends, a ^C delivers an AST, which then invokes
** synchronous GCA operations.  Because AST's don't nest, we cannot use
** AST's to indicate synchronous I/O completion.  For sync, GCqio_exec
** puts the IOXB (QIO-level control block) into GCsync_ioxb; GCsync
** waits for the I/O's event flag and then examines GCsync_ioxb to
** drive the completions of sync operations.  GCsync_ioxb is an array 
** of 2: one for the current operation and one for the potentially
** interrupted operation (which must complete as well).
**
** History:
**
**     04-oct-89 (pasker)
**	    Upgraded GCusrpwd to the 6.3 CL spec. This included some
**	    restructuring and bug fixing, like valid username or password that
**	    included speical characters (like $ and _) which would fail.
**
**     04-oct-89 (pasker)
**	    fixed bad bug in GCrecvpeer, where it the GCqio parameters (p1&p2)
**	    specified the entire rq_assoc_buf.  In fact, it only wants the peer
**	    information.  The flow is like:
**
**		requestor		server
**		------------------------------------------
**		rq_assoc_buf    ---->	(listen_mailbox)
**		(chan[RN].mbx) <----	rq_assoc_buf.peer_info
**
**	04-sep-1989 (pasker)
**	    ASK fix #1. restructured GCreceive_complete (renamed from
**	    rcv_complete)
**
**	04-sep-89 (pasker) ASK fix #2.
**	    Changed GCdisconn so that it makes a local copy of the
**	    MBX channels and sets the ASIPC (LCB) channel fields
**	    to GC_CH_ID_NONE before actually deassigning the them. This
**	    prohibits ASTs which may complete as a result of the
**	    deassignment from trying to do I/O to channels which are
**	    no longer are part of this association.
**
**     05-aug-1989 (jennifer, as merged in by pasker)
**         Added getting security label. This is coniditionally compiled
**         based on the xORANGE flag.  Only certain systems will have
**         security releases.
**
**	24-oct-1989 (pasker)
**	    fixed a bug that I introduced in a previous version which
**	    broke reciept of chop-marks by synchronous users.
**
**	24-oct-89 (pasker)
**	    IFDEFed out the rest of the security stuff so that the same copy of
**	    this module is backwards compatible.
**
**     15-Nov-89 (ham)
**         Added io$m_norswait on send peer info and changed return of
**         status from getuai in GCusrpwd.
**
**     16-Nov-89 (ham)
**         Increase 30 second timeout to 60, should be fixed to use
**         connect_timeout,  the user specified timeout.  And  bump
**         the mailbox size on the server listen mailbox.
**
**	20-Nov-89 (ham)
**	    Make GCdisconn asynchronous call and add GCdisconn_complete.
**
**	21-Nov-89 (ham)
**	    Put back the use of the user specified timeout on the connect.
**	    Add in the use of II_SVRxx_LSN_QUE inorder for the user to
**	    calculate the server listen mailbox, remember that the buffio
**	    limit should be increased accordingly.
**
**	22-Nov-89 (jorge)
**	    1) Made the LISTEN mailbox size big enough for queue of 32 .
**	    2) Made the GCsendpeer connect timeout 60 seconds (same as in 6.2),
**	       GCA_SPEER_TIMEOUT, and the GCrecvpeer timmeout 120 seconds,
**	       GCA_RPEER_TIMEOUT.
**	    3) Cleaned up error cheking on requestor vs. listener failure
**	    4) Introduced uniform tests for TIMEOUT after Send/RecvPeer GCqio.
**	    5) Fixed un-initialized "status" in GCrecv_peer_complete, generating
**	       random error numbers.
**	    6) Fixed GCqio for SYNC case, did not propigate TIMEOUT errors.
**	    7) Fixed GCrecvpeer identity authentication to only be done by
**	       LISTENER. B1 label still obtained by both REQUESTOR and
**	       LISTENER.
**	    8) Made connect_timeout and max_mbox static instead of global.
**	28-Nov-1989 (jorge)
**	    Made the II_CONNECTxx_TIMEOUT pertain to both SEND/RECVpeer.
**	    Made RPEER_TIIMEOUT to be a general CONNECT_TIMEOUT.removed
**	    SPEER_TIMEOUT.
**
**	29-Nov-1989 (ham)
**	    Redid timeout again. Fixed an race cond. in GCsend where
**	    the completion routine was called before GCpurge, if
**	    the purging flag was on.
**
**	30-Jan-1990 (ham)
**	    - Changed GC1receive and GCreceive to pass back the amount
**	      we received and not post another receive if the amount is
**	      less than what was asked for.
**
**	    - Made all calls asynchronous.  This is needed for the upper
**	      layers which are now buffering.  GCACALL will call GCsync
**	      when theuser spercified a synchronous call.
**
**	    - Added GCsync.  Gcsync checks the event flag to see if
**	      the I/O has completed.  If it has it just returns if
**	      hasn't it does a sys$waitfr, to wait for the I/O to
**	      complete.
**
**	08-Feb-1090 (jorge)
**	    Fixed bug 9935, GCsendpeer() did not dassign send_peer_chan.
**
**	09-Feb-1990 (ham)
**	    Put in 6.2 ASK fix for specifying the correct buffer_len
**	    in the item list for getuai.
**
**	06-Mar-1990 (seiwald)
**	    Svc_parms->sys_err is now a pointer.
**
**	02-Apr_1990 (ham)
**	    Finish putting changes for user defined timeouts.
**	    And check the proper status.
**
**	27-Apr-90 (jorge)
**	     GCnsid() now uses sys_err instead of svc_parm (pr  Seiwald's
**	     request). Fixed comments on GCqio_exec().
**
**	12-May-1990 (ham)
**	    Fix bad tests for (iosb->ioxb$w_tmo_status "equal to" SS$_TIMEOUT)
**	    Put back the synchronous I/O that was taken out for 6.4
**	    until front-ends get a single ^C exception handler that
**	    will allow ASTs to complete when cleaning up from a ^C.
**
**	30-May-1990 (seiwald)
**	    GCdetect now properly returns a status.
**
**	06-Jun-90 (seiwald)
**	    Made GCalloc() return the pointer to allocated memory.
**
**	13-Jun-90 (jorge)
**	    RE-wrote to conform with correct behavior.
**	    GCdiscon():
**         - Since M_NOW is not used in SYS$QIO, the user's completion
**	      routine could not have been driven if the I/O had not completed.
**	      Thus, if the user issued the GCdiscon() to tear down the
**	      connection, do it NOW, deassign all mboxes.
**	    - There is no need for a disconnect timer since this is an
**	      immediate service in VMS. The outstanding AST's (if any) will
**	      be driven when the mailbox is removed.
**	    - Once the last AST has been driven, the AST handler will invoke
**	      GCdisconn_complete().
**	    - If no outstanding ASTs, call GCdisconn_complete() directly.
**	    GCsend():
**		removed CL level blocking. Kept CL level segmenting.
**	    GCreceive() etal
**		removed Cl level blocking and segmenting.
**	    GCsync() etal
**		added support for deffered synchronous GCdisonn_complet()
**	    GCqio_exec() etal
**		introduced stuctured mailbox datagramm subsystem. Mailbox
**		tear-down still done directly by upper GCA_CL routines.
**	18-Jun-90 (seiwald)
**	    GCsave & GCrestore lng_user_data is a longnat.
**	18-Jun-90 (seiwald)
**	    GCpurge now takes a SVC_PARMS, not GCA_ACB.
**	15-Aug-90 (seiwald)
**	    GCinititate returns STATUS.
**	06-Sep-90 (jorge)
**	    Corrected Zombie timer related active association queue mgt.
**	12-Sep-90 (jorge)
**	    - removed GCreceive_complete call to gca_purge. Simplifies
**	      GCA purging. GCA mainline will now handle purging correctly.
**	    - Introduced mailbox create/assign support routines. Consolidated
**	      code.
**	    - corrected inadequate zombie association detection tests.
**	    - did masive cleanup and code consolidation.
**	30-oct-90 (ralph)
**	    Initialize status in GCqio_exec_tm0 to solve COPY problem
**	06-nov-90 (ralph)
**	    Save process id in GClisten_complete so we can get user name, etc.
**	12-Nov-90 (seiwald)
**	    Deconfused use of status vs. VMS status in GCdetect.
**	    Replaced missing 'if' in GCrestore.
**	    Added a case to GCmbx_assoc to allow clients to attach to
**	    mailboxes.  As coded, only the server could attach.
**	09-Oct-90 (jorge)
**	    NO-OP zombie timer if timer interval is zero (eg II_SVRxx_CONN_CHK
**	    == "0 00:00:00.00")
**	27-Dec-90 (seiwald)
**	    Revamped and made sane.
**
**	    GENERAL CHANGES
**	    -   New intro, including excerpts of the GCA CL spec.
**	    -   Routines re-ordered; follows order in intro.
**	    -   Everything renamed to GCxxxx.  Sys$qio functions are 
**	        GCqio_xxx(); mailbox functions are GCmbx_xxx().
**	    -   All entry points (internal and external) are declared 
**		properly in forwards decl section.
**	    -   Introduced VMS_STATUS type (OK=1), distinguished from
**	        STATUS (OK=0).
**	    -   Parameters and types of many internal functions changed.
**	        Many internal functions now return VMS_STATUS.
**	    -   Removed lots of old unused code: GCsendavail_complete,
**	        GCmbx_owner, obsolete GC exit handler, and many unused 
**	        variables and #defines.
**	    -   Reversed some of Pasker's, uh, unique coding style.
**	    -   Just about every function rewritten.  Replaced old, 
**		wrong documentation.
**	    -   New developer's tracing turned on with II_GC_TRACE.
**	    
**	    MAIN FUNCTION RESTRUCTURING
**	    -   Made GCterminate VOID; it puts its status into svc_parms.
**	    -   Reworked GCregister: removed homespun NMgtAt; removed
**	        loop for listen mailbox creation - it either works or
**	        doesn't; removed extra assignment of listen mailbox;
**	        replaced LSNASSOC_IPC structure with a plain ASSOC_CHAN
**	        - it has all the data GCregister needs.
**	    -   Restructured GCsend, GCreceive: removed blocking code,
**	        since GCA is now kind enough to provide large buffers
**	        and allow shorted reads; removed GCqio_recvlen -
**	        checking for pending I/O is done by GCreceive.
**	    -   Reworked GCdisconn: removed GCdisconn_complete; removed
**	        GCsync_dconn_parms and it's associated bad logic.
**	    
**	    QIO HANDLING
**	    -   GCqio_exec and GCsync now properly handle the accursed
**	        nested GC calls which occur on ^C interrupts in frontends.
**	    -   GCqio_exec drives its completion on sys$qio call failure 
**	        and so no longer returns status.
**	    -   Altered IOXB: renamed w_tmo_status to w_can_status (cancel 
**	        reason code); changed GC_CH_TS_ZOMBIE to IO$_RELEASE;
**	        consolidated GC_CH_ID_EMPTY and GC_CH_ID_RLSED and named it
**	        GC_CH_ID_NONE.
**	    -   Consolidated and renamed event flags.
**	    -   Numbered each of the ASSOC_CHAN's in the ASSOC_IPC and
**	        collected them into an array - this allows for indexed
**	        access.  Changed send_peer_chan into a full ASSOC_CHAN.
**	    
**	    ZOMBIE DETECTION
**	    -   Consolidated GCqio_zombtimer and _complete into GCqio_zombie.
**	    -   Zombie checking now stops if no I/O's are pending on any 
**	        assocation.  It is restarted on the next call to GCqio_exec.
**	    -   GCrequest connections are placed on the zombie checking
**	        queue in GCrecvpeer_complete, not GCsendpeer_complete,
**	        since the server may not have assigned the mailboxes when 
**	        GCsendpeer completes.
**	    
**	    MAILBOX HANDLING
**	    -   Removed duplicate initialization of chan's and status's
**	        in GCmbx_assoc.
**	    -   Made GCmbx_seclabel's interface mimic GCmbx_userterm's.
**	    -   GCmbx_detect now returns a count or -1.
**	    -   Numbered each of the mailbox names in GC_RQ_ASSOC and
**	        collected them into an array - this allows for indexed
**	        access.  They map 1-to-1 into the ASSOC_CHAN's of the
**	        ASSOC_IPC for clients.
**	    
**	    TIMER HANDLING
**	    -   Timers are all now millisecond based (rather than a VMS
**	        date string).
**	    -   New sys$bintim wrapper GCtim_strtoms.
**	    -   Now use quadword arithmetic to compute timers, rather
**	        than (get this) CVna, STpolycat, and sys$bintim.
**	    
**	    NEW UTILITY ROUTINES
**	    -   New routine GCgetsym to get a symbol with installation
**	        code embedded.  The installation code is fetched once in
**	        GCinitiate.
**	    -   New GCinitasipc to allocate and initialize an ASSOC_IPC.
**
**	2-Jan-91 (seiwald)
**	    Moved the bulk of the save/restore operation into GCA mainline.
**	17-Jan-91 (seiwald)
**	    Get the sender_id from the GClsn_assoc IOSB in GClisten_complete;
**	    it was using the (unset) IOSB in asipc->chan[RN].
**	31-Jan-91 (seiwald)
**	    Use a static IOSB in GCpurge, rather than the normal channel's
**	    IOSB.  Using the normal channel's IOSB left the status such 
**	    that GCqio_pending thought an I/O was still waiting to drive 
**	    GCqio_complete.  This caused GCA_CALL to loop on GCsync.
**	    Also reformatted tracing macros.
**	5-Feb-91 (seiwald)
**	    Explicitly zero items in GCinititate, rather than relying
**	    upon static initialization.  This is necessary if the user
**	    invokes GCterminate and GCinitiate between successive sessions,
**	    as LIBQ does.  Failure to zero GCzomb_tmrset was breaking
**	    zombie checking.
**	27-Mar-91 (seiwald)
**	    GCpurge is VOID.
**	6-Apr-91 (seiwald)
**	    Be a little more assiduous in clearing svc_parms->status on
**	    function entry.
**	2-May-91 (seiwald)
**	    Avoid using GC_ASSOC_FAIL except for cases where the partner
**	    has exited.  Use SS$_NOMBX rather than IO$_RELEASE for the
**	    zombie checker's cancel status.  Don't log any system status
**	    for GC_ASSOC_FAIL.
**	10-May-91 (seiwald)
**	    Set the process name to the name of the mailbox in GCregister,
**	    with a special case for the name server, whose process name is
**	    set to II_GCNxx.  Removed setting process name in GCnsid and 
**	    in the GCCCL.C
**	2-Apr-92 (seiwald)
**	    Support for installation passwords: get the user name of the
**	    process in GCregister, and set the svc_parms trusted flag if
**	    this matches the invoking user name during GClisten.
**	3-Dec-92 (seiwald)
**	    Temporarily ifdef'ed for ALPHA: don't do password checking
**	16-Dec-92 (seiwald)
**	    II_TIMEOUT is gone; let someone else support it.
**	18-dec-92 (walt)
**	    Changed the ALPHA ifdef to return OK rather than GC_LOGIN_FAIL.
**	22-Dec-92 (seiwald)
**	    Moved code to call sys$hash_password in from GChpwd.mar.
**	    Dispite what Armancio said, it can be written in C.
**	28-Dec-92 (seiwald)
**	    #ifdeffed for ALPHA to use sys$hash_password directly.
**	04-Mar-93 (edg)
**	    Removed GCdetect().
**	24-Jun-93 (seiwald)
**	    Rather than failing on a GCreceive() larger than size_advise,
**	    just shorten the requested size.  The fastselect code in 
**	    GCA_LISTEN, at least, issues a GCrecieve() of a size it thinks
**	    is appropriate.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	23-May-94 (eml)
**	    Added the routine GCtrace_status( ) to trace a VMS status after
**	    system service failure.
**	    Added routine GCsignal_startup() called from GCregister() to
**	    notify IIRUNDBMS of successful server startup.
**	    Modified the GC_FIND_NSID case of GCnsid use SYS$TRNLNM instead
**	    of NMgtAT to translate logical for the name server id. NTgtAT
**	    translastes all iterations of the logical and we want just one.
**	03-Feb-95 (brogr02)
**	    In GCmbx_userterm(),
**	    Null-terminate username, zapping any trailing blanks.
**	    Needed to make the PM routines find requests correctly.
**	    Also quit tromping on caller's invoker+invoker_size+1'th byte.
**      02-May-95 (abowler)
**          Added code to allow support of test security labels. This was
**          done for Unix in gcaconn.c, but never made it to VMS. New
**          code scans config.dat for default user security labels and
**          caches them away (GClisten). On client connection, the process
**          user name is extracted, and compared against entries in the
**          user/label cache. If none are found, a default is assigned
**          (GCmbx_seclabel).
**	07-jun-95 (albany)
**	    Integrated changes from VAX/VMS CL to AXP/VMS CL.
**	12-jul-95 (duursma)
**	    Translate II_SEC_LEVEL using the new function NMgtIngAt() instead
**	    of NMgtAt() to avoid conflicts between Production and Group level
**	    installations.
**	21-mar-96 (duursma)
**	    Added support for batch mode server and single client connection.
**	    If this mode is turned on (by manually adding batchmode and
**	    gca_single_mode to config.dat), client/server data is exchanged
**	    via shared memory instead of mailbox I/O.   The latter is still
**	    used by either party to post "read" and "write" requests, but
**	    the message lengths are always 0, since data is not sent along
**	    with the mailbox QIO's.
**	03-April-96 (strpa01)
**	    In GCmbx_userterm(),
**	    Correct Null-termination of username. GETJPI always returns 12 as
**	    length of username. Previously invoker[username_length-1]=0 
**	    was truncating last character of a 12 char username. (71018)
**	06-Sep-96 (boama01 and rosga02)
**	    Added a comment for VMS mailbox sizes GCnormal_commsz to be in 
**          sync with GCA_NORM_SIZE
**      27-May-97 (horda03)
**          Increased the size of the VMS mailbox size (GCnormal_commsz) to 4096
**          as the Name server sends FS messages whose max size is defined
**          by GCA_FS_MAX_DATA. Also, extended the comment added above to
**          indicate that the VMS mail box size must also be in sync with
**          GCA_FS_MAX_DATA. This fixes bug 81565.
**      18-Aug-97 (horda03)
**          Back out above change. Increaing VMS mailbox size caused ABF (and other)
**          applications to hang.
**      18-Aug-97 (horda03) X-Integration of change 431293.
**          01-August-97 (horda03)
**               In GCmbx_userterm(),
**               The function was attempting to determine the terminal type of a
**               Mailbox for connections over the network. This resulted in a ""
**               (blank) terminal type being stored. Previously, the terminal type
**               of a connection over the network (determined by dbmsinfo('terminal'))
**               was "unknown". Only determine the terminal type for NON DETACHED
**               processes. (83698)
**      19-aug-97 (rosga02)
**         - Made a correct fix to a bug 74212. Previuos fix was to ignore
**           flags.trusted in GCAASYNC. The real problem was that the flag
**	     was not set correctly due to trailing blank characters in
**	     GC_myname.
**	27-feb-98 (loera01)
**	     Just a little clean-up. Added two missing terminating zero's in
**	     a couple of $JPIDEF's, and changed MEfill on the internal sec-
**	     urity label to fill the entire security label, instead of
**	     the size of the pointer (4 bytes).
**	14-jan-99 (loera01) bug 94610
**	    For GCrestore() and GCsave(), added size_advise field to save 
**	    acb->size_advise--a parameter which GCA sets.  Otherwise, 
**	    GCnormal_commsz is assumed to be size_advise, which corrupts the 
**	    GCA buffers, since it's currently larger (4120) than GCA (4096).
**	27-jan-99 (loera01) bug 94610
**	    For GClisten_complete() and GCrequest(): size_advise needs to be 
**	    set to 4096 (GCA_NORM_SIZE), but the mailbox needs the extra 24 
**	    bytes for the gca_header_length of GCA_CB, for some fastselect 
**	    messages. Created two constants, GC_NORM_SIZE and GC_MAX_MBX_SIZE, 
**	    to hide the definitions.
**      12-jan-99 (kinte01)
**          Modify GCsignal_startup() to be non-static for others to call.
**          Cross-integration of change 439498 from VAX VMS CL
**	14-feb-00 (loera01 bug 100276
**	    For GCusrpwd(), added UAI$_EXPIRATION op to check the 
**	    expiration date of expired passwords, and added a check for 
**	    the UAI$M_DISACNT (disabled account) flag.
**      06-nov-00 (loera01) bug 102701
**          For GCmbx_userterm() and GCmbx_seclabel(), added wait/retry loop
**          SYS$GETJPI if a suspended process is encountered.
**      07-Nov-00 (horda03) Bug 103169.
**          Use GCB in process name for BRIDGE servers.
**      16-feb-01 (loera01)
**          Ingres 2.5 does not support GCsendpeer and GCrecvpeer.  Those
**          routines are now gone.  Instead, GCrequest and GClisten send and
**          receive, respectively, the CL-level peer info.  The GCA peer info 
**          is exchanged via GCreceive and GCsend over normal channels.
**          Backward-compatible support is provided for local clients using
**          old-style GCsendpeer and GCrecvpeer: GClisten detects whether 
**          the requestor has sent an old or new-style request.  If old-style,
**          the first GCreceive of an assocation simply copies the GCA peer 
**          info obtained from GClisten and calls back GCA, without reading 
**          anything else.  Since GCsend sends back GCA peer info only in new 
**          and old implementations, no changes were required for GCsend.
**
**          GC no longer has access to GCA_ACB, and maintains the GC local
**          control block pointer in svc_parms->gc_cb.
**
**          Added a timeout to GClisten, instead of the hard-coded "-1", and
**          altered GCregister so that it looks for a server string of "NMSVR" 
**          instead of "IINMSVR".
**
**          Added new routine GCusername().
**
**          Replaced "nat" references with i4.
**
**          Casts were added to various functions to eliminate warnings from 
**          the more stringent compiler used on Alpha VMS 2.5.
**
**	23-feb-01 (loera01)
**          Actually allocated space for the node name string in GChostname,
**          which previously somehow got away with passing the address of
**          an unallocated string.
**	27-feb-01 (kinte01)
**	    Change GChostname to use $SYIDEF instead of own structure
**	    for sys$getsyi call 
**	26-feb-01 (loera01)
**	    Update for multiple product builds
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**      20-Apr-2004 (ashco01) Problem: INGNET99 Bug: 107833
**          Introduced GCqio_shut() to explicitly shutdown IIGCC listen
**          channel during IIGCC shutdown.
**	06-dec-2004 (abbjo03)
**	    Remove B1 security code.
**	04-oct-2005 (abbjo03)
**	    In GCmbx_userterm(), do not depend on VMS job type for determining
**	    whether a terminal name is present or not:  SSH connections appear
**	    as detached, yet they have a terminal name.
**      10-may-2007 (Ralph Loen) SIR 118032
**          Added support for multi-threaded clients, but not multi-threaded
**          servers.  The "GCregistered" global boolean, set to TRUE when 
**          servers invoke GCregister(), reverts the gcacl routines to their
**          previous paths.  Client applications now wait on the EFN$C_ENF 
**          ("default") event flag instead of GCio_ef, so that each thread 
**          sees event flag events only on the local thread.  For clients, 
**          SYS$SYNCH() is used to wait for I/O completion using an ioxb block 
**          from TLS.  I/O time-outs are handled in client applications via
**          a new AST routine, GCsync_timerset(), since time-outs must also
**          wait on EFN$C_ENF.
**          GCsync() now employs a queue of IOXB blocks instead of a two-
**          element array. This is a requirement of asynchronous API.   
**      17-may-2007 (Ralph Loen) Bug 118350
**          Only utilize GCTRACE at level zero for fatal errors.  Otherwise,
**          utilities that use TR for display purposes, such as iimonitor,
**          display the trace information.
**      11-jun-2007 (Ralph Loen) Bug 118487
**          In GCinitiate(), add capability to recognize gc_trace_level
**          (II_GC_TRACE) in config.dat.
**      13-jun-2007 (Ralph Loen) SIR 118032
**          Created GCsetMode() and GCgetMode() to replace the GLOBALDEF
**          GCregistered.  Replace MEreqmem() calls with the GCalloc
**          specifier passed from GCA.  Use new ME_tls_createkeyFreeFunc()
**          to utilize GCA's GCfree routine for the TLS destructor.
**      09-Jul-2007 (Ralph Loen) Bug 118698
**          Added GC_ASYNC_CLIENT_MODE for API clients.  Async client mode
**          is specific to API applications at present, and avoids the use
**          of SYS$SYNCH() and the EFN$C_ENF event flag for synchronizing 
**          I/O.  This is necessary because the API can queue more than
**          one I/O before GCsync() is invoked, but SYS$SYNCH() must wait for
**          only one I/O at a time.  Obsoleted GCsync_timerset() for
**          synchronous clients, since it doesn't work in the SYS$SYNCH()
**          context.  Instead, GCqio_exec() executes GCqio_timeout() from
**          the timer as for asynchronous servers.
**      29-Aug-2007 Ralph Loen (Bug 119036)
**          Do not fetch the GC_THREAD structure from TLS if the process is in
**          GC_SERVER_MODE or GC_ASYNC_CLIENT_MODE.  Instead, use a structure 
**          allocated from the main thread.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**	    Replace another homegrown list by ILE3.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.  Replace uses of
**	    CL_SYS_ERR by CL_ERR_DESC.
**      26-feb-2009 (Ralph Loen) Bug 121668
**          For servers, add mailboxes created from GCrequest() to the
**          zombie check stack.  The ASSOC_IPC block gets a new field,
**          zombie_check, which is set to TRUE when the partner has
**          connected.  A value of TRUE means that GCzio_zombie() can 
**          legitimately detect dead connections; otherwise the asipc
**          block is ignored.  The new function GCcheck_peer() checks for an
**          actively-connected connection partner.
**      09-Mar-2009 (horda03) Bug 121668
**          Add call to GCgetMode() to initialise the 'mode' local variable.
**      11-mar-2009 (stegr01)
**          use VMS supplied header file uaidef.h and remove homegrown defines
**          for these constants
**      01-May-2009 (horda03) Bug 122016
**          Race condition where an IO Timeout AST is queued before the AST for
**          the IO completion may cause problems. For example, when this happens
**          in the GCN (when the GClisten timesout) corruption of the State machine
**          occurs (the state machine - gcn_server - gets invoked too many times,
**          leading to an ACCVIO and a server shutdown.
**	08-jul-2009 (joea)
**	    Use local variable in GChostname instead of calling MEreqmem.
**	    Add missing #includes for cv.h, id.h, pc.h, tr.h and remove
**	    duplicate for st.h.
**      26-Mar-2009 (Ralph Loen) Bug 121772
**          Extended GCqio_zombie() to non-servers.  Checks made for 
**          GC_SERVER_MODE were removed from GCrequest(), GCrequest_complete(), 
**          GCsend(), GCsend_complete, GCreceive() and GCreceive_complete.
**          Added GCzomb_tmrset and thread_id fields to ASSOC_IPC so that
**          individual threads could track their own zombie events.
**      21-Oct-2009 (Ralph Loen) Bug 122769
**          Cross-integration of bug 121772 corrupted the reqidt argument 
**          in sys$setimr() for GC_SYNC_CLIENT_MODE in GCqio_zombie(): 
**          the address of GCzomb_time was provided instead of 
**          asipc->GCzomb_tmrset, which is what sys$cancel() expects later 
**          when it cancels the timer during normal completion.
**      14-Nov-2009 (Ralph Loen) Bug 122787
**          In GCterminate(), remove code that shuts down the TCP/IP listen 
**          port.  Restore code that deassigns the local listen port.
**          Remove GLOBALDEF GCC_lsn_chan.
**/


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN VOID    GCdisconn();
FUNC_EXTERN VOID    GChostname();
FUNC_EXTERN STATUS  GCinitiate();
FUNC_EXTERN VOID    GClisten();
FUNC_EXTERN STATUS  GCnsid();
FUNC_EXTERN VOID    GCpurge();
FUNC_EXTERN VOID    GCreceive();
FUNC_EXTERN VOID    GCregister();
FUNC_EXTERN VOID    GCrelease();
FUNC_EXTERN VOID    GCrequest();
FUNC_EXTERN VOID    GCrestore();
FUNC_EXTERN VOID    GCsave();
FUNC_EXTERN VOID    GCsend();
FUNC_EXTERN VOID    GCsync();
FUNC_EXTERN VOID    GCterminate();
FUNC_EXTERN STATUS  GCusrpwd();
FUNC_EXTERN VOID    GCusername();

# define VMS_STATUS  long

static bool GCcheckPeer();
static VOID         GCgetsym();
static struct _assoc_ipc *  GCinitasipc();
static VOID         GClisten_complete();
static VMS_STATUS   GCmbx_assign();
static VMS_STATUS   GCmbx_assoc();
static VMS_STATUS   GCmbx_client();
static VMS_STATUS   GCmbx_servers();
static i4           GCmbx_detect();
static VMS_STATUS   GCmbx_userterm();
static VOID         GCqio_complete();
static VOID         GCqio_exec();
static VOID         GCqio_shut();
static i4           GCqio_pending();
static VOID         GCqio_status();
static VOID         GCqio_timeout();
static VOID         GCqio_zombie();
static VOID         GCreceive_complete();
static VOID         GCrequest_complete();
static VOID         GCsend_complete();
FUNC_EXTERN VOID    GCsetMode( i2 mode );
FUNC_EXTERN VOID    GCgetMode();
VMS_STATUS	    GCsignal_startup();
static VOID         GCtim_mstotim();
static long         GCtim_strtoms();
static VOID	    GCtrace_status( char *ident, STATUS status );
static PTR	    shm_addr( SVC_PARMS *svc_parms, i4 op );
static i4	    shm_whoami();

/*
**  Defines of other constants.
*/

# define	    MILLI_TO_BINTIM	10*1000	/* ms -> 100ns */

# define	    GC_ZOMBTIME_DEF     10*1000		/* 10 sec */
# define	    GC_CONNTIME_DEF	120*1000	/* 2 min */

# define	    SZ_RIBUFFER		64	/* expedited chan buf size */

# define 	    GC_NORM_SIZE 	4096	/* must not be > GCA_NORM_SIZE */

# define	    GC_MAX_MBX_SIZE	4120    /* must be at least       */
					        /* GCA_NORM_SIZE +        */
						/* gca_header_length of   */
						/* GCA_CB		  */
# define	    TRY_COUNT 		120     /* Times to retry for     */
						/* suspended process      */
# define GCTRACE(n) if( GCtrace >= n )(void)TRdisplay

# define GCTRACE_STATUS(n) if( GCtrace >= n )(void)GCtrace_status

#define GCACL_TIMER_TRACE(which, reqidt, status)\
    if (GCtrace >= 3)\
	{\
	long tqes = 0;\
	lib$getjpi(\
		&JPI$_TQCNT,	/* itmcod */\
		0,		/* process id */\
		0,		/* process name */\
		&tqes,		/* outvalue */\
		0,		/* outstring */\
		0);		/* outlen */\
	GCX_TRACER_3_MACRO (which, NULL,\
	    "reqid=%d, status = %x, tqcnt = %d",\
	    reqidt,status,tqes);\
	}

# define VMS_OK(x)   (((x) & 1) != 0)
# define VMS_FAIL(x) (((x) & 1) == 0)
# define VMS_SUSPENDED(x) ((x) == SS$_SUSPENDED)

#define IO_DONE 1
#define TM_DONE 2

/* Stuff below is needed to support the "batch mode server" */

static bool batch_mode = FALSE;

#define SHM_SERVER 0
#define SHM_CLIENT 1

#define SHM_ATTACH 0
#define SHM_CREATE 1

typedef struct _GC_BATCH_SHM
{
     i4         bat_inuse;
     PID        process_id;   /* [0] = server ID  [1] = client id */
     i4         bat_signal;   /* [0] = server ID  [1] = client id */
     i4         bat_dirty;    /* [0] = server ID  [1] = client id */
     char       bat_id[ 104 ];
     i4         bat_sendlen;  /* number of bytes sent             */
     i4         bat_pages;
     i4         bat_offset;
} GC_BATCH_SHM;
 
#define MAX_SHM_SIZE 8192

#define ME_SHMEM_DIR "memory"

static GC_BATCH_SHM	*GC_bsm[2] = {NULL,NULL};

/* Support threads */
#ifdef OS_THREADS_USED
GLOBALDEF bool GC_os_threaded   = FALSE;
#endif
static i4 setimr_last_param = 0;


/*}
** Name: GC_RQ_ASSOC - Local IPC new association information
**
** Description:
**      This structure is used only within this file. It stores all
**	the information necessary to build a VMS local IPC association.
**	The information is generated by the client requesting the association
**	and is passed through the server's "listen" mailbox.
**
** History:
**      18-Apr-1987 (berrow)
**          Created
**	16-feb-01 (loera01)
**          Removed GCA peer info.
*/

typedef struct _GC_RQ_ASSOC
{
    char	mbx_name[ 4 ][ 16 ];	/* mailbox names */

	    /* same order as client's ASSOC_IPC.chan[] */

# define CLIENT_SERVER_DATA	0	/* Client --> Server data */
# define SERVER_CLIENT_DATA	1	/* Server --> Client data */
# define CLIENT_SERVER_INTR	2	/* Client --> Server interrupt */
# define SERVER_CLIENT_INTR	3	/* Server --> Client interrupt */

    i4		max_mbx_msg_size;      /* Mailbox size */
}   GC_RQ_ASSOC;

/*}
** Name: GC_RQ_ASSOC1 - Local IPC new association information for users
**     of GCrecvpeer() and GCsendpeer().
**
** Description:
**	This structure is identical to GC_RQ_ASSOC, except that it contains
**      a section for GCA peer info at the end.
**
**      Since GC_RQ_ASSOC and GC_RQ_ASSOC1 are union'ed in ASSOC_IPC, 
**      alterations to this block must be reflected in GC_RQ_ASSOC above.
**      Otherwise, the items in each block must be individually copied when
**      referenced in GCrequest() and GClisten().  Right now, it is assumed
**      that references to non-GCA peer info will do just as well for
**      GC_RQ_ASSOC1 as in GC_RQ_ASSOC.
**
** History:
**      16-feb-01 (loera01)
**          Created.
*/

typedef struct _GC_RQ_ASSOC1
{
    char	mbx_name[ 4 ][ 16 ];	/* mailbox names */
    i4		max_mbx_msg_size;       /* Mailbox size */
    GC_OLD_PEER peer_info;              /* GCA level info. exchange */
}   GC_RQ_ASSOC1;

/*}
** Name: ASSOC_IPC - Local IPC association management structure
**
** Description:
**      This structure is used only within this file. It stores all
**	the information necessary to communicate via the technique
**	used by the routines in this module (for now local IPC).
**	The information is on a per-association basis.
**
**	Local IPC for VMS is via "mailboxes".
**	The structure stores enough state information to provide a "stream"
**	interface to a sequence of incoming mailbox messages.
**	It also allows us to handle requests to send arbitrary length messages.
**
** History:
**      16-Apr-1987 (berrow)
**
*/

typedef struct _IOXB
{
    short int ioxb$w_status;
    short int ioxb$w_length;
    long  int ioxb$l_devdepend;
    VOID    (*ioxb$l_astadr)();
    long  int ioxb$l_astprm;
    short int ioxb$w_chan;
    long  int ioxb$l_tmo;
    short int ioxb$w_can_status;
} IOXB;

# define	    GC_CH_ID_NONE	-1	/* for w_chan */
# define	    GC_CH_WS_IDLE	-1	/* for w_status */

typedef struct {
	short	chan;
	char	name[16];
	IOXB	ioxb;
	i4	maxiosize;
} ASSOC_CHAN;

typedef struct _assoc_ipc
{
    QUEUE	    q;	  	/* Active ASIPC que hdr */
    ASSOC_CHAN      chan[5];    /* data channels */

	    /* same order as client's GC_RQ_ASSOC.mbx_name[] */

# define	SN    0		/* Send data channel */
# define	RN    1		/* Receive data channel */
# define	SX    2		/* Send interrupt channel */
# define	RX    3 	/* Receive interrupt channel */
# define	CN    4		/* GCrequest connect channel */

# define	CHANS 4		/* # of channels up to RX */
# define	ALLCHANS 5	/* # of channels including CN */

    /*
    **  Note that ASSOC_IPC may contain either new or old-style peer info.
    */
    union 
    {
        GC_RQ_ASSOC	rq_assoc_buf;	/* what we read from MBX on listen */
        GC_RQ_ASSOC1    rq_assoc_buf1;
    } rq;

    i4	sender_pid;	/* from the IOSB when reading PEER info */
    char	term_name[64];	/* Terminal of requesting client */
    char	invoker[16];	/* Username of requesting client */
    short	am_requestor;	/* identifies client */
    SVC_PARMS	*dcon_svc_parms; /* drive delayed GCdsn_complete (parms)*/
    bool	old_format;
    bool        zombie_check;
    u_i4        thread_id;
    bool        GCzomb_tmrset;
}   ASSOC_IPC;

/*}
** Name: GC_SAVE_DATA - structure for passing GC association between processes
**
** Description:
**	Connection information can be passed between parent/child 
**	processes by GCsave and GCrestore.  This structure describes 
**	the GC level information which must be passed.
**
**	GC_SAVE_LEVEL_MAJOR must be changed if incompatible changes are
**	made to this structure; this will cause GCrestore to reject
**	the connection.
**
**	GC_SAVE_LEVEL_MINOR can be changed when minor, compatible,
**	changes are made to this structure.
**
** History:
**	2-Jan-91 (seiwald)
**	    Created.
*/

# define	GC_SAVE_LEVEL_MAJOR	0x0604
# define	GC_SAVE_LEVEL_MINOR	0x0001

typedef struct _GC_SAVE_DATA
{
    short	save_level_major;
    short	save_level_minor;
    /* pieces of ASSOC_IPC */
    short	am_requestor;
    i4		max_mbx_msg_size;
    char	mbx_name[ 4 ][ 16 ];
} GC_SAVE_DATA;


/*
** Definition of all global variables owned by this file.
*/
/*
**  Definition of static variables and forward static functions.
*/

static  PTR         (*GCalloc)();	/* Memory allocation routine */
static  VOID        (*GCfree)();	/* Memory deallocation routine */

/* Information required to catch association requests */

static	ASSOC_CHAN	GClsn_assoc ZERO_FILL;	/* Only one listen */

i2 GCmode = (i2)GC_SYNC_CLIENT_MODE;

typedef struct
{
    QUEUE q;
    IOXB  *GCsync_ioxb;
} GC_IOXB_QUEUE;

/* Thread local-storage */

typedef struct
{
    i4 counter;
    QUEUE GCsync_ioxb_qhead;	       /* Queued I/Os */
} GC_THREAD;

GC_THREAD main_gc_thread;              /* For single-threaded servers */

/* VMS event flags */

static  i4          GCdv_ef;            /* event flag for sys$getdvi */
static  i4	    GCio_ef;		/* event flag for I/O */
static  i4	    GCtm_ef;		/* event flag for SYNCH timer */

# define GC_EF_MASK ( 1 << GCio_ef % 32 | 1 << GCtm_ef % 32 )

static	QUEUE       GCactive_qhead;	/* Queue of LCB's */

/* "constants" evaluated at GCinitiate time.  */

static  i4	    GCnormal_commsz; 	/* standard normal-flow mbxmsg size,
					** used for size-advise. */
static  i4         GCsvr_mbx_max;	/* Server Listen mbxmsg size */

static  i4          GCconn_time;	/* Default connect timeout */
static  i4          GCzomb_time;	/* Zombie checking interval */
static	volatile i4 GCzomb_tmrset;	/* Zombie checking timer set */

static  char	    GC_instcode[ 3 ];	/* Installation code */

static	i4	    GCtrace = 0;	/* TRdisplay trace level */

static	char	    GC_myname[ 16 ];	/* Name of user doing GCregister */

static  bool        initialized = FALSE; /* Constants evaluated at startup */

static  ME_TLS_KEY  gc_thread_key = 0;   /* Used for TLS storage */

#ifdef OS_THREADS_USED
static GENERIC_64 volatile setimr_time = {0};
static __align(LONGWORD) volatile i4 setimr_astpnd = 0;
static volatile i4 setimr_cnt = 0;
#endif


/*{
**
** Name: GCinitiate - Initialize static elements within GCA CL.
**
** Description:
**
** This routine is called by GCA_INITIATE to pass certain variables to the
** CL.  Currently, these are the addresses of the memory allocation and
** deallocation routines to be used.  Any other system-specific initiation
** functions may be performed here.
**
** See the comments under "Memory Allocator" in the GCA CL introduction
** for restrictions on use the allocator.
** The allocator's calling interface is:
**
** 	PTR	(*alloc)( i4 size )
**
** 	VOID	(*free)( PTR ptr )
**
** GCinitiate is synchronous: it completes before returning.
** It returns no status.
**
** Inputs:
** 	alloc				Pointer to memory allocation routine
** 	free                            Pointer to memory deallocation routine
**
** Outputs:
** 	none
**
** Returns:
** 	OK or FAIL
**
** Exceptions:
** 	none
**
** Side Effects:
** 	none
**
**(	Special VMS note:
**
**	In the VMS environment a special purpose "hack" is added to satisfy
**	a customer requirement for timing out idle FE's.  It is carried to this
**	release intact from previous releases.
**	Calls to INtoignore() are made before and after each qio
**	to stop activity timing while waiting for I/O and to start it again
**	when it is complete.
**)
**
** History:
**     10-Nov-1987 (jbowers)
**         Created.
**      26-Jan-88 (jbowers)
**          Added call to INtoinit to support the INtimeout function
**      29-Nov-89 (ham)
**	    Put the user connect timeout back in and implement user
**	    defined max mailbox size for server listen mailboxes.
**      13-May-98 (kinte01)
**          Remove restriction of 1500 for GCnormal_commsz.  It needs to
**          be GCA_NORM_SIZE + 24 to properly handle fast select messages.
**	    The maximum size of a fast select message is 4096 and then
**	    GCA adds a header of 24 to the message.
**	11-Dec-1998 (kinte01)
**	    Reorganize some of the trace message calls so that the parameters
**	    they want to output have actually been initialized. Found by the
**	    DEC C 6.0 compiler.
**      10-May-2007 (Ralph Loen) SIR 118032
**          GCinitiate() now initializes the GC_THREAD TLS structure.  Added
**          a static boolean to prevent NM and other initialization routines
**          from being called mulitiple times from different threads.
**      11-jun-2007 (Ralph Loen) Bug 118487
**          Add capability to recognize gc_trace_level (II_GC_TRACE) in 
**          config.dat.
**          
*/

STATUS
GCinitiate( alloc, free )
PTR   	    (*alloc)();
VOID  	    (*free)();
{
    VMS_STATUS	status;
    char	symval[64];
    char	*symptr;
    char	*driver = NULL;
    char	*mode   = NULL;
    GC_THREAD   *gc_thread = &main_gc_thread;
    STATUS      pstatus;

    /*  GCX_TRACER_0_MACRO ("GCinitiate>", NULL);  */

    if (!initialized)
    {
        initialized = TRUE;
    
        /*  Get installation code.  */
    
        NMgtAt( "II_INSTALLATION", &symptr );
        STlcopy( symptr ? symptr : "", GC_instcode, 2 );
    
        /* See if we are running the batch mode server */
        batch_mode = (PMget( "!.batchmode", &driver ) == OK &&
    		  STbcompare( driver, 0, "on", 0 , TRUE ) == 0 &&
    		  PMget( "!.gca_single_mode", &mode ) == OK &&
    		  STbcompare( mode, 0, "on", 0, TRUE) == 0);
    
        /*  Set up allocator.  */
    
        GCalloc = alloc;
        GCfree = free;
    
        /*  Get event flags.  */
    
        status = lib$get_ef(&GCtm_ef);
        status = lib$get_ef(&GCio_ef);
    
        /*  Get tracing level.  */
    
        NMgtAt( "II_GC_TRACE", &symptr );
        if( !( symptr && *symptr ) && PMget( "!.gc_trace_level", 
            &symptr ) != OK )
            GCtrace = 0;
        else 
            CVan( symptr, &GCtrace );
    
        /* Prevent user from listening without successful GCregister. */
    
        GClsn_assoc.chan = GC_CH_ID_NONE;
    
        /*
        **  Get the installation id to construct logical name.
        **  The name is of the form II_CONNECTxx_TIMEOUT, where xx
        **  is the installation.  The default is 2 minutes.
        **  Convert the connect timeout to binary time.
        **  Use the default if the conversion fails.
        */
    
        GCgetsym( "II_CONNECT", "_TIMEOUT", symval, 63 );
    
        if( !*symval || ( GCconn_time = GCtim_strtoms( symval ) ) <= 0 )
    	GCconn_time = GC_CONNTIME_DEF;
    
        /*
        **  Resolve the user defined zombie timeout.
        **  If the logical wasn't defined, set connect time out value,
        **  default to 1 minute.
        **  Convert the connect timeout to binary time.
        **  Use the default if the conversion fails.
        */
    
        GCgetsym( "II_SVR", "_CONN_CHK", symval, 63 );
    
        if( !*symval || ( GCzomb_time = GCtim_strtoms( symval ) ) <= 0 )
    	GCzomb_time = GC_ZOMBTIME_DEF;
    
        /*
        **  Resolve the server listen mailbox logical.  It is in the form
        **  II_SVRxx_LSN_QUE,  where xx is the installation if there is one.
        **  Set the max listen mailbox size.  If the user didn't specify the
        **  logical default to 16k.
        */
    
        GCgetsym( "II_SVR", "_LSN_QUE", symval, 63 );
    
        if( !*symval )
    	    GCsvr_mbx_max = 32;
        else
    	    CVan( symval, &GCsvr_mbx_max );
    
        GCsvr_mbx_max *= sizeof( GC_RQ_ASSOC1 );
    
        /*
        ** Compute default mailbox size.
        */
    
        GCnormal_commsz = GC_MAX_MBX_SIZE;
    
        /* make sure this size is not less than GCA_NORM_SIZE in GCAINT.H */
        /* and ensure this size is not less than GCA_FS_MAX_DATA in GCA.H */
    
        /* 
        ** GCnormal_commsz needs to be  GCA_NORM_SIZE + 24 for Fast Select 
        ** Message Handling to work properly
        */

        GCzomb_tmrset = 0;
    
        /* Init the active asipc (LCB) queue.  */
        QUinit( &GCactive_qhead );

        gc_thread->counter = 0;

        QUinit(&gc_thread->GCsync_ioxb_qhead);

#ifdef OS_THREADS_USED
        GC_os_threaded = CS_is_mt();
        if (GC_os_threaded) setimr_last_param = 2;
#endif

    } /* if (!initialized) */

    return OK;
}


/*{
** Name: GCterminate - Perform local system-dependent termination functions
**
** Description:
**
** GCterminate is invoked by mainline code as the last operation done as
** a GCA process is terminating its connection with GCA.  It may be used to
** perform any final system-dependent functions which may be required,
** such as cancelling system functions which are still in process, etc.
**
** GCterminate frees any memory still allocated to the GCA CL.
**
** GCterminate is synchronous: it completes before returning.
** It returns status in the SVC_PARMS.
**
** All parameters (both input and output) are handled via a
** pointer to SVC_PARMS structure containing the following
** elements ...
**
** Inputs:
** 	none
**
** Outputs:
** 	status				OK operation succeeded
** 					or any CL failure code.
**
** 	sys_err				OS specific error data.
**
** Returns:
** 	none
**
** Exceptions:
** 	none
**
** Side Effects:
** 	none
**
** History:
**      19-Jan-88 (jbowers)
**          Initial function implementation
**	23-Sep-90 (jorge)
**	    Corrected bad cancel of timers; added call to INtoterm().
**	06-Apr-92 (alan) bug #42857
**	    Cancel server's read on listen mailbox to prevent clients 
**	    from trying to connect after GCterminate. 
**	11-May-92 (alan)
**	    Mark cancelled listen $qio as SS$_SHUT so it can be swallowed.
**      14-Nov-2009 (Ralph Loen) Bug 122787
**          Remove code that shuts down the TCP/IP listen port.  Restore code
**          that deassigns the local listen port.
*/

VOID
GCterminate( svc_parms )
SVC_PARMS	*svc_parms;
{
    VMS_STATUS	status;

    GCX_TRACER_0_MACRO ("GCterminate>", NULL);
    svc_parms->status = OK;

    /* Cancel the zombie timer. */

    status = sys$cantim( &GCzomb_time, PSL$C_USER );
    GCACL_TIMER_TRACE("cantim>GCterminate#2>",GCzomb_time,status);

    /* If we're a server, cancel read on listen mailbox. */

    if( GClsn_assoc.chan != GC_CH_ID_NONE )
    {
        /* shutdown the listen port and discard all queued IO */

	GClsn_assoc.ioxb.ioxb$w_can_status = SS$_SHUT;
        sys$dassgn( GClsn_assoc.chan );
        GClsn_assoc.chan = GC_CH_ID_NONE;
    }

    /* Free allocated event flags.  */

    lib$free_ef(&GCtm_ef);
    lib$free_ef(&GCio_ef);

    /* Dump tracing */

    if( GCtrace )
	GCX_PRINT_TRACE_MACRO();

} /* end GCterminate */


/*{
** Name: GCregister - Server start-up notification
**
** Description:
**
** This routine is called by GCA_REGISTER on server start-up.  It is
** used to notify the GCA CL that a server has been started.
**
** It is in this routine that the Listen ID, the invoking server's name,
** is assigned.  This is done by a system-specific mechanism that ensures
** the assignment of a unique name.  This is the public external name
** which would be specified to GCrequest to initiate an association with
** the server.  This is returned to the invoker for its information, by
** passing a pointer to a character string.  This string is allocated by
** GCregister and must remain (at least) until GCterminate is called.
**
** If appropriate for the particular system-dependent implementation, any
** set-up operations for subsequently establishing an IPC connection with a
** requesting client may be done at this time.  It may be assumed that the
** routine GClisten will be called subsequently.
**
** GCregister is synchronous: it completes before returning.
** It returns status in the SVC_PARMS.
**
** All parameters (both input and output) are handled via a
** pointer to SVC_PARMS structure containing the following
** elements ...
**
** Inputs:
** 	char *svr_class			Name of type of server registering.
**
**
** Outputs:
** 	status				OK operation succeeded
** 					or any CL failure code.
**
** 	sys_err				OS specific error data.
**
** 	char *listen_id			Assigned listen address.
**
** Returns:
** 	none
**
** Exceptions:
** 	none
**
** Side Effects:
** 	none
**
**  History:    $Log-for RCS$
**      16-Apr-1987 (berrow)
**          Created.
**      02-Feb-88 (jbowers)
**          Added call to INtoignore to prevent server from timing out when
**          INtimeout is activated.
**	01-May-92 (alan)
**	    Change $getjpi return length arg from int to short.
**      16-feb-01 (loera01)
**          Look for "NMSVR" instead of "IINMSVR" string in the server class.
*/

VOID
GCregister( svc_parms )
SVC_PARMS          *svc_parms;
{
    char		*class;
    char		class4[5];
    VMS_STATUS		status = 0;
    char		prn[ 16 ];
    i4 		        pid;
    u_i2		myname_size;
    i4                 i;

    struct dsc$descriptor_s prndesc;
    IOSB		iosb;
    ILE3		jpiget[] = {
	    { sizeof( pid ), JPI$_PID, (PTR)&pid, 0 },
	    { sizeof( GC_myname ) - 1,JPI$_USERNAME,GC_myname,&myname_size},
	    { 0, 0, 0, 0 } };

    GCsetMode((i2)GC_SERVER_MODE);

    svc_parms->status = OK;

    /* Get process id */

    status = sys$getjpiw(EFN$C_ENF, 0, 0, jpiget, &iosb, 0, 0);
    if (status & 1)
	status = iosb.iosb$w_status;
    if (VMS_FAIL(status))
	pid = 0;

    GC_myname[ myname_size ] = 0;
    STtrmwhite(GC_myname);  /* remove trailing blanks */
    CVlower( GC_myname );

    /*
    ** Server name is a logical whose equivalence name is the name of the
    ** mailbox.  The logical is of the form II_XXXX_ii_pid, where XXXX is
    ** the server type (disguised), ii is the installation code, and pid
    ** is the lower 16 bits of the process id.
    */
    /* Disguise the server class to confuse the user. */
    if( !STcompare( svc_parms->svr_class, "INGRES" ) )
	class = "DBMS";
    else if( !STcompare( svc_parms->svr_class, "COMSVR" ) )
	class = "GCC";
    else if( !STcompare( svc_parms->svr_class, "BRIDGE" ) )
        class = "GCB";
    else
	STlcopy( svc_parms->svr_class, class = class4, 4 );

    STprintf( GClsn_assoc.name,
  		    "%s_%s%s%s_%x",
                    SystemCfgPrefix,
		    class,
		    GC_instcode[0] ? "_" : "",
		    GC_instcode,
		    (u_i2)pid );

    CVupper( GClsn_assoc.name );

    /* Create a Server mailbox */

    GClsn_assoc.maxiosize = sizeof( GC_RQ_ASSOC1 );
    status = GCmbx_servers( &GClsn_assoc );

    if (VMS_FAIL(status))
    {
	GCTRACE(1)( "GCregister> (GCmbx_server), Unable to create listen mailbox\n" );
	GCTRACE_STATUS(1)( "GCregister> (GCmbx_server)", status );
	goto error;
    }

    /* If someone else is using our logical, there is not much we can do. */

    if( GCmbx_detect( GClsn_assoc.chan ) != 1 )
    {
	sys$dassgn( GClsn_assoc.chan );
	GClsn_assoc.chan = GC_CH_ID_NONE;
	goto error;
    }

    /* Mark for deletion, so it disappears when we exit. */

    sys$delmbx(GClsn_assoc.chan);

    /* Set process name to name of mailbox. */

    if( !STcompare( svc_parms->svr_class, "NMSVR" ) )
	STprintf( prn, "%s_GCN%s%s", SystemCfgPrefix, GC_instcode[0] ? "_" : 
           "", GC_instcode );
    else
	STcopy( GClsn_assoc.name, prn );
    
    CVupper( prn );

    prndesc.dsc$a_pointer = prn;
    prndesc.dsc$w_length = STlength( prn );
    prndesc.dsc$b_dtype = DSC$K_DTYPE_T;
    prndesc.dsc$b_class = DSC$K_CLASS_S;

    status = sys$setprn( &prndesc );

    if( VMS_FAIL( status ) )
    {
	GCTRACE(1)( "GCregister> (sys$setprn), Unable to set process name\n" );
	GCTRACE_STATUS(1)( "GCregister> (sys$setprn)", status );
	goto error;
    }

    /* Return the generated mailbox name, for use by Name Service */

    svc_parms->listen_id = GClsn_assoc.name;

    /*
    ** Notify IIRUNDBMS of successful server startup.
    */
    status = GCsignal_startup( prn );
    if( VMS_FAIL( status ) )
    {
	GCTRACE(1)( "GCregister> (GCsignal_startup), Unable to notify IIRUNDBMS of startup\n" );
	GCTRACE_STATUS(1)( "GCregister> (GCsignal_startup)", status );
	goto error;
    }

    return;

error:
    svc_parms->status = GC_LSN_FAIL;
    svc_parms->sys_err->error = status;
}


/*{
** Name: GClisten - Establish a server connection with a client
**
** Description:
**
** This routine is, by definition, a server-only function.
**
** This routine establishes a system-specific "read" on the local
** IPC connection previously set up by the GCregister routine.
** This places the server in the position of listening for an
** association request from a client.  The completion of the
** GClisten occurs when a connection via the IPC mechanism is made
** by a client.
**
** GClisten may allocate a CL specific Local Control Block (LCB) and
** place a pointer to the LCB in the ACB (acb->lcb).
**
** GCA_LISTEN guarantees that if GClisten succeeds, it will
** subsequently call GCreceive and GCsend to exchange blocks
** of data of size sizeof( GCA_PEER_INFO ).  Since GCA_LISTEN
** guarantees the size of the data read with GCrecvpeer,
** implementations which receive GCA_PEER_INFO along with their
** own IPC specific connection information may read and buffer
** both, and simply copy the GCA_PEER_INFO when GCA_LISTEN calls
** GCreceive.
**
** GCA_LISTEN guarantees that if GClisten fails, it will subsequently
** call GCdisconn and GCrelease to close the failed connection.
** Since GCA_LISTEN cannot know how far the connection had progressed when
** it failed, GCdisconn and GCrelease should make use of information
** stored in the local control block to choose the proper actions.
**
** The transmission buffer/segment size is set here.  The size_advise
** parameter is set in the svc_parms parm list to specify the optimum
** buffer size to use if so desired.  A value of 0 leaves the client
** to pick a preferred buffer size.  If this value is set, it must be
** a multiple of sizeof(ALIGN_RESTRICT).
**
** GClisten returns the user name, account name, terminal name, and
** security label (all where appropriate) of the user initiating the
** connection with GCrequest.  GClisten does this by returning pointers to
** character strings.  These strings are allocated by GClisten and must
** remain allocated until GCrelease is called.
**
** GClisten is potentially asynchronous: it may return before an
** incoming association is accepted, and should not block waiting.
** It signals completion by setting the status in the SVC_PARMS
** and driving the callback listed in the SVC_PARMS as gca_cl_completion.
** The call is (*(svc_parms->gca_cl_completion))( svc_parms ).
**
** GClisten should honor the SVC_PARMS time_out, which indicates
** how long to allow for the indication of a connection.  If the
** listen does not complete within the alloted time, it completes with a
** status of GC_TIME_OUT.
** A value of -1 instructs GClisten to wait forever; a value of 0
** instructs GClisten to fail if no connection is immediately available; a
** value greater than 0 instruct GClisten to wait that many milliseconds
** for a connection before failing.
** Implementation of timeouts for GClisten is optional.
**
** All parameters (both input and output) are handled via a
** pointer to SVC_PARMS structure containing the following
** elements ...
**
** Inputs:
** 	time_out           		Timeout in ms, 0=poll, -1=no timeout.
**
** 	(*gca_cl_completion)(svc_parms) Routine to call on completion.
**
**
** Outputs:
** 	status				OK operation succeeded
** 					or any CL failure code.
**
** 	sys_err				OS specific error data.
**
** 	acb->lcb			Set to local control block.
**
** 	char *user_name         	Peer entity's user name.
**
** 	char *account_name      	Peer entity's account name.
**
** 	char *access_point_identifier   Peer entity's terminal name.
**
** 	size_advise        		Optimum size for communication.
** 					Must be multiple of
** 					sizeof(ALIGN_RESTRICT).
**
** Returns:
** 	none
**
** Exceptions:
** 	none
**
** History:
**      16-Apr-1987 (berrow)
**          Created.
**      30-Jan-1989 (jbowers)
**          Added user id and password validation through GCusrpwd.
**      29-sep-1989 (jennifer)
**          Added security label return for B1 systems.
**      02-may-1995 (abowler)
**          Added test security label return, removing xORANGE code.
**      16-feb-01 (loera01)
**          Made GClisten recognize timeout from svc_parms, instead of
**          hard-coding "-1".  Added capability to recognize old-style
**          peer info information.
*/

VOID
GClisten(svc_parms)
SVC_PARMS          *svc_parms;
{
    ASSOC_IPC		*asipc;

    static bool init = FALSE; 

    GCX_TRACER_0_MACRO( "GClisten>", 0 );

    svc_parms->status = OK;

    /* Check that GCregister has been called first. */

    if( GClsn_assoc.chan == GC_CH_ID_NONE )
    {
	svc_parms->status = GC_LSN_FAIL;
	goto error;
    }

    /*
    ** Perform all required allocations before issuing the
    ** asynch/synch read to initiate the listen, so no memory
    ** allocation is required in an AST completion exit.
    */

    /*  Allocate local IPC management structure  */

    if( !( asipc = GCinitasipc() ) )
    {
	svc_parms->status = GC_LSN_RESOURCE;
	goto error;
    }

    /* Hang the local control block on the acb */

    svc_parms->gc_cb = (PTR)asipc;

    /*
    ** Now we actually issue a read to the "listen" mailbox
    ** The completion of an asynchronous request causes our exit routine
    ** GClisten_complete to be invoked as an AST.
    */

    GClsn_assoc.ioxb.ioxb$w_status = GC_CH_WS_IDLE;

    GCX_TRACER_1_MACRO( "GClisten>", asipc, "qio ioxb=%x",(i4)&GClsn_assoc.ioxb );

    GCqio_exec(
	GClsn_assoc.chan,	    	/* chan */
	IO$_READVBLK,			/* func */
	&GClsn_assoc.ioxb, 	    	/* iosb */
	GClisten_complete,		/* astadr */
	svc_parms,			/* astprm */
	&asipc->rq.rq_assoc_buf1,	/* p1 = VA */
	sizeof(asipc->rq.rq_assoc_buf1),/* p2 = size */
	svc_parms->time_out,		/* TIMEOUT */
	svc_parms->flags.run_sync);	/* sync indicator */

    if (batch_mode)
    {
	PID	pid;

	/* Create the shared memory segment for the batch mode server */
	if (((GC_BATCH_SHM *) shm_addr(svc_parms, SHM_CREATE)) == NULL)
	    goto error;

	/* Initialize control blocks for readers and writers */

	GC_bsm[SHM_SERVER]->bat_inuse  = GC_bsm[SHM_CLIENT]->bat_inuse = 0;
	GC_bsm[SHM_SERVER]->bat_signal = GC_bsm[SHM_CLIENT]->bat_signal = 0;
	GC_bsm[SHM_SERVER]->bat_dirty  = GC_bsm[SHM_CLIENT]->bat_dirty  = 0;

	/* Get the pid and stick it in the server process control block */

	PCpid(&pid);
	GC_bsm[SHM_SERVER]->process_id = pid;
	GC_bsm[SHM_CLIENT]->process_id = 0;
    }

    return;

error:
    /* Drive completion on error */

    GCTRACE(2)("GClisten error, driving completion\n");

    svc_parms->sys_err->error = 0;

    (*svc_parms->gca_cl_completion)(svc_parms->closure);
}


/*{
** Name: GClisten_complete - pick up connection indications
**
** Inputs:
**	svc_parms->acb->asipc	connection control block
**
** Side effects:
**	asipc put onto active connection queue.
**	calls (*svc_parms->gca_cl_completion)().
**
** History:
**	27-Dec-90 (seiwald)
**	    Rewritten and documented.
**	27-jan-99 (loera01) bug 94610
**	    Size_advise needs to be set to 4096 (GCA_NORM_SIZE), but the 
**	    mailbox needs the extra 24 bytes for the gca_header_length of 
**	    GCA_CB, for some fastselect messages.  Created two constants, 
**	    GC_NORM_SIZE and GC_MAX_MBX_SIZE, to hide the definitions.
**     16-feb-01 (loera01)
**          Detect whether the request is of GCsendpeer() format or the
**          new format by the buffer size.  Set a flag if GCsendpeer() format.
*/

static VOID
GClisten_complete(svc_parms)
SVC_PARMS          *svc_parms;
{
    ASSOC_IPC		*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    VMS_STATUS		status = 0;
    i4			i;

    /* Tell the caller if something went wrong with the listen.  */

    GCqio_status( &GClsn_assoc.ioxb, GC_LSN_FAIL, svc_parms );

    if( svc_parms->status != OK )
	goto complete;

    if (GClsn_assoc.ioxb.ioxb$w_length == sizeof(asipc->rq.rq_assoc_buf))
    {
        asipc->old_format = FALSE; /*  We have the expected rq_assoc_buf */
    }
    else if (GClsn_assoc.ioxb.ioxb$w_length == sizeof(asipc->rq.rq_assoc_buf1))
    {
        /* This is the old GCrecvpeer format.  Set flag to indicate that 
        ** the next GCreceive can just fall through with the existing GCA info.
        */
        asipc->old_format = TRUE;  
    }
    else
    {
	svc_parms->status = GC_LSN_FAIL;
	svc_parms->sys_err->error = 0;
        goto complete;
    }

    /* Attach as server */

    status = GCmbx_assoc( 0, &asipc->rq.rq_assoc_buf, asipc );

    if( VMS_FAIL(status))
    {
	GCTRACE_STATUS(1)( "GClisten_complete> (GCmbx_assoc)", status );
	svc_parms->status = GC_LSN_FAIL;
	svc_parms->sys_err->error = status;
	goto complete;
    }

    /* Save process id for getting user name & terminal id */

    asipc->sender_pid = GClsn_assoc.ioxb.ioxb$l_devdepend;

    /* Advise user of optimum I/O size */

    svc_parms->size_advise = GC_NORM_SIZE;

    /* Put on list of active connections for zombie checking.  */

    asipc->zombie_check = TRUE; 
    QUinsert( &asipc->q, &GCactive_qhead );

    if (batch_mode)
    {
	if (GC_bsm[SHM_SERVER]->bat_inuse &&
	    ! PCis_alive(GC_bsm[SHM_CLIENT]->process_id))
	{
	    /* 
	    ** the server is in use but the client is dead (went away);
	    ** we don't allow connections in such a case.
	    */
	    svc_parms->status = GC_ASSOC_FAIL;
	    svc_parms->sys_err->error = 0;
	    goto complete;
	}
	GC_bsm[SHM_SERVER]->bat_inuse = 1;
    }

    GCX_TRACER_1_MACRO( "GClisten_complete>", asipc, "sender %d",
	asipc->sender_pid );

    /* Get authentication info of requestor, using sender id in IOSB. */

    status = GCmbx_userterm( asipc->sender_pid,
        sizeof(asipc->term_name),  asipc->term_name,
	sizeof(asipc->invoker),    asipc->invoker);

    if (VMS_FAIL(status))
    {
        svc_parms->status = GC_LSN_FAIL;
	svc_parms->sys_err->error = status;
	goto complete;
    }

    svc_parms->access_point_identifier = asipc->term_name;
    svc_parms->user_name = asipc->invoker;

    /* Set trusted flag */

    svc_parms->flags.trusted = !STcompare( GC_myname, asipc->invoker );

complete:
    /* Call GCA mainline completion routine */

    (*svc_parms->gca_cl_completion)( svc_parms->closure );
}


/*{
** Name: GCrequest - Build connection to peer entity
**
** Description:
**
** This routine builds a system-dependent IPC connection to a server
** which has issued a GClisten.  GCA invokes GCrequest on behalf of a
** client issuing the GCA_REQUEST service call.
**
** GCrequest may allocate a CL specific Local Control Block (LCB) and
** place a pointer to the LCB in the ACB (acb->lcb).
**
** GCA_REQUEST guarantees that if GCrequest succeeds, it will subsequently
** call GCsend and GCreceive to exchange blocks of data of size
** sizeof( GCA_PEER_INFO ).  GCA_REQUEST does not consider the connection
** made until GCreceive succeeds.  In this way, implementations which send
** GCA_PEER_INFO along with their own IPC specific connection information
** may defer making the connection until GCA_REQUEST calls GCsend or
** GCreceive.
**
** GCA_REQUEST guarantees that if GCrequest fails, it will subsequently
** call GCdisconn and GCrelease to close the failed connection.
** Since GCA_REQUEST cannot know how far the connection had progressed when
** it failed, GCdisconn and GCrelease should make use of information
** stored in the local control block to choose the proper actions.
**
** The transmission buffer/segment size is set here.  The size_advise
** parameter is set in the svc_parms parm list to specify the optimum
** buffer size to use if so desired.  A value of 0 leaves the client
** to pick a preferred buffer size.  If this value is set, it must be
** a multiple of sizeof(ALIGN_RESTRICT).
**
** GCrequest is potentially asynchronous: it may return before the
** outgoing association is complete, and should not block waiting.
** It signals completion by setting the status in the SVC_PARMS and
** driving the callback listed in the SVC_PARMS as gca_cl_completion.
** The call is (*(svc_parms->gca_cl_completion))( svc_parms ).
**
** GCrequest should honor the SVC_PARMS time_out, which indicates
** how long to allow to make the connection.  If the request does not
** complete within the alloted time, it completes with a status of
** GC_TIME_OUT.   A value
** of -1 instructs GCrequest to wait forever; a value of 0 instructs
** GCrequest to fail if no connection is immediately available; a value
** greater than 0 instruct GCrequest to wait that many milliseconds for a
** connection before failing.
** Implementation of timeouts for GCrequest is optional.
**
** All parameters (both input and output) are handled via a
** pointer to SVC_PARMS structure containing the following
** elements ...
**
** Inputs:
** 	time_out           		Timeout in ms, 0=poll, -1=no timeout.
**
** 	char *partner_name      	Target to connect to.
**
** 	(*gca_cl_completion)(svc_parms) Routine to call on completion.
**
**
** Outputs:
** 	status				OK operation succeeded
** 					or any CL failure code.
**
** 	sys_err				OS specific error data.
**
** 	acb->lcb			Set to local control block.
**
** 	size_advise        		Optimum size for communication.
** 					Must be multiple of
** 					sizeof(ALIGN_RESTRICT).
**
** Returns:
** 	none
**
** Exceptions:
** 	none
**
** Side Effects:
** 	none
**
** History:
**      16-Apr-1987 (berrow)
**          Created.
**      05-aug-1989 (jennifer)
**          Added getting security label. This is coniditionally compiled
**          based on the xORANGE flag.  Only certain systems will have
**          security releases.
**	23-Sep-90 (jorge)
**	    Added this comment: Connection related I/O is deffered untill
**	    GCsendpeer is called.
**	27-jan-99 (loera01) bug 94610
**	    Size_advise needs to be set to 4096 (GCA_NORM_SIZE), but the 
**	    mailbox needs the extra 24 bytes for the gca_header_length of 
**	    GCA_CB, for some fastselect messages.  Created two constants, 
**	    GC_NORM_SIZE and GC_MAX_MBX_SIZE, to hide the definitions.
**     16-feb-01 (loera01)
**          GCrequest now sends the CL peer info to the server.
*/

VOID
GCrequest( svc_parms )
SVC_PARMS	*svc_parms;
{
    ASSOC_IPC	*asipc;
    VMS_STATUS	status;

    /* Allocate local IPC management structure */

    if( !( asipc = GCinitasipc() ) )
    {
	GCTRACE(1)( "thread %x. GCrequest> (GCinitasipc), memory allocation failed\n", PCtidx() );
	svc_parms->status = GC_RQ_RESOURCE;
	GCTRACE_STATUS(1)( "GCrequest> (GCinitasipc)", svc_parms->status );
    	svc_parms->sys_err->error = SS$_INSFMEM;
	goto complete;
    }

    /* Attach to ACB and note that this is the initiator */

    svc_parms->gc_cb = (PTR)asipc;
    asipc->am_requestor = 1;

    GCX_TRACER_0_MACRO( "GCrequest>", asipc );

    /* This work area will get reset when the true association is established */

    svc_parms->status = GC_NO_PARTNER;     /* default error */

    /* Assign mailbox - deassigned in GCrequest_complete or GCdisconn */

    STcopy( svc_parms->partner_name, asipc->chan[CN].name );
    status = GCmbx_assign( &asipc->chan[CN] );

    if( VMS_FAIL(status) )
    {
	GCTRACE(1)( "thread %x. GCrequest> (GCmbx_assign), expected partner not available\n", PCtidx() );
	GCTRACE_STATUS(1)( "GCrequest> (GCmbx_assign)", status );
    	svc_parms->sys_err->error = status;
	goto complete;
    }

    /* Check the reference count on the mailbox associated with
    ** this channel. If it's not two, our partner has ungracefully
    ** departed.
    */

    if( GCmbx_detect( asipc->chan[CN].chan ) < 2 )
    {
	GCTRACE(1)( "thread %x. GCrequest> (GCmbx_detect), partner not available\n", PCtidx() );
    	svc_parms->sys_err->error = 0;
	goto complete;
    }

    /* build the client's assocation.  */

    svc_parms->status = GC_RQ_FAIL; 	/* default error */

    status = GCmbx_client( asipc, &asipc->rq.rq_assoc_buf );

    /* if it failed, then release anything we thought we had.  */

    if( VMS_FAIL(status) )
    {
	GCTRACE(1)( "thread %x. GCrequest> (GCmbx_client), Unable to create partner assication\n", PCtidx() );
	GCTRACE_STATUS(1)( "GCrequest> (GCmbx_client)", status );
    	svc_parms->sys_err->error = status;
	goto complete;
    }

    /*
    ** Add the mailboxes to the zombie stack.  Mark them so that GCqio_zombie()
    ** ignores them, since at this point the reference count is only 1.
    */
    asipc->zombie_check = FALSE;

    QUinsert( &asipc->q, &GCactive_qhead );

    /* Advise user of optimum I/O size */

    svc_parms->size_advise = GC_NORM_SIZE;
    svc_parms->status = OK;

    if (batch_mode)
    {
	PID	pid;

	if (((GC_BATCH_SHM *) shm_addr(svc_parms, SHM_ATTACH)) == NULL)
	{
	    svc_parms->status = GC_ASSOC_FAIL;
	    goto complete;
	}
	if (GC_bsm[SHM_SERVER]->bat_inuse &&
	    PCis_alive(GC_bsm[SHM_CLIENT]->process_id))
	{
	    svc_parms->status = GC_ASSOC_FAIL;
	    goto complete;
	}

	PCpid( &pid );
	GC_bsm[SHM_CLIENT]->process_id = pid;
    }	

   /*
    ** Send the GC peer info to partner.
    ** Hijack svc_parms.  
    */
    svc_parms->flags.flow_indicator = 0;
    if( svc_parms->time_out < 0 )
	svc_parms->time_out = GCconn_time;

	GCqio_exec(
	    asipc->chan[CN].chan,		/* chan */
	    IO$_WRITEVBLK|IO$M_NORSWAIT,	/* func */
	    &asipc->chan[CN].ioxb,		/* iosb */
	    GCrequest_complete,			/* astadr */
	    svc_parms,				/* astprm */
	    (PTR)&asipc->rq.rq_assoc_buf, 	/* p1 = VA */
	    sizeof(asipc->rq.rq_assoc_buf),	/* p2 = len */
	    svc_parms->time_out,  		/* p3 = timeout */
	    svc_parms->flags.run_sync);	    	/* sync indicator */
             
            /*
            ** Let GCrequest_complete call back GCA.
            */
            return;

complete:
    /* Call GCA mainline completion routine */

    (*svc_parms->gca_cl_completion)( svc_parms->closure );
}


/*{
** Name: GCrequest_complete - pick up GCrequest status and drive completion
**
** Inputs:
**	svc_parms->acb->asipc	control block
**
** Outputs:
**	svc_parms->status	OK on success or any CL failure
**
** Side effects:
**	Deassigns connection mailbox.
**	Calls (*svc_parms->gca_cl_completion)()
**
** History:
**	16-feb-01 (loera01)
**	    Created from GCsendpeer().
**      26-mar-09 (Ralph Loen) Bug 121772
**          Removed check for server mode from asipc->zombie_check
**          conditional code.
*/
static VOID
GCrequest_complete(svc_parms)
SVC_PARMS          *svc_parms;
{
    ASSOC_IPC		*asipc = (ASSOC_IPC *)svc_parms->gc_cb;

    /*
    ** If the partner has connected, allow GCqio_zombie() to check for
    ** dead connections.
    */
    if ( GCcheckPeer( asipc ) )
        asipc->zombie_check = TRUE;
    else
        asipc->zombie_check = FALSE;

    GCqio_status( &asipc->chan[CN].ioxb, GC_RQ_FAIL, svc_parms );

    /* Deassign the request channel assigned in GCrequest. */

    sys$dassgn( asipc->chan[CN].chan ); 
    asipc->chan[CN].chan = GC_CH_ID_NONE; 

    /* Call GCrequest_complete completion routine */

    (*svc_parms->gca_cl_completion)( svc_parms->closure ); 
}


/*{
** Name: GCsend - Send data on an association
**
** Description:
**
** Send data to the other party in the connection.  This routine simulates
** byte-stream I/O, and utilizes the system-specific IPC mechanism in whatever
** way is necessary to create this illusion.
**
** The user of this routine has requested svc_parms->svc_send_length bytes
** of data to be sent on the outgoing message stream.  This is to be done in a
** single operation from the caller's point of view; i.e., the operation is
** not complete until the caller's entire buffer has been sent.  The exact
** meaning of "sent" is implementation specific, but the data in the caller's
** buffer must be in a safe harbor, and the caller must be able to re-use the
** buffer.  The data sent as the outgoing message stream is read from the
** address starting at svc_parms->svc_buffer.
**
** The flags.flow_indicator element of svc_parms specifies whether the send
** function is to be issued on the normal or expedited flow channel.
** The expedited flow channel supports at least 64 bytes in a single send.
** Sends on the normal channel may be of unlimited size.
**
** GCsend is potentially asynchronous: it may return before the
** data can be copied from the client buffer, and should not block waiting.
** It signals completion by setting the status in the SVC_PARMS and
** driving the callback listed in the SVC_PARMS as gca_cl_completion.
** The call is (*(svc_parms->gca_cl_completion))( svc_parms ).
** The invocation of this exit implies to the mainline that the send
** buffer may now be reused.
**
** GCsend should honor the SVC_PARMS time_out, which indicates
** how long to allow for the first byte of data to be sent.  If the
** send does not start within the alloted time, it completes with a
** status of GC_TIME_OUT.  Once the send starts (i.e. sends a single byte
** of data), it does not time out.
** A value of -1 instructs GCsend to wait forever; a value of 0
** instructs GCsend to fail if data cannot be sent immediately; a
** value greater than 0 instruct GCsend to wait that many milliseconds
** to be able to send data before failing.
** Implementation of timeouts for GCsend is optional.
**
** Possible error conditions are as follows:  If the connection to the
** partner is unexpectedly gone, a status of GC_ASSOC_FAIL is returned.
** If the connection has been terminated locally, by the GCdisconn
** routine, a status of GC_ASSN_RLSED is returned.  The ability to detect
** these two modes of disconnection is system dependent.  If the
** distinction cannot be made in a particular implementation, a status of
** GC_ASSOC_FAIL is the default.  GC_ASSOC_FAIL normally covers network
** errors from which recovery is impossible.  Other CL specific errors
** are possible.
**
** All parameters (both input and output) are handled via a
** pointer to SVC_PARMS structure containing the following
** elements ...
**
** Inputs:
** 	acb->lcb			Local control block
**
** 	time_out           		Timeout in ms, 0=poll, -1=no timeout.
**
** 	char *svc_buffer         	send, receive buffer
**
** 	svc_send_length    		Send buffer size.
**
** 	(*gca_cl_completion)(svc_parms) Routine to call on completion.
**
** 	flags.flow_indicator:1   	0=normal, 1=expedited flow.
**
**
** Outputs:
** 	status				OK operation succeeded
** 					or any CL failure code.
**
** 	sys_err				OS specific error data.
**
** Returns:
** 	none
**
** Exceptions:
** 	none
**
** Side Effects:
** 	none
**
** History:
**      16-Apr-1987 (berrow)
**          Created.
**	21-oct-1988 (rogerk)
**	    Ifdef'ed out the calls to GCmbx_detect - just count on the ast
**	    driven GCqio_zombie routine to catch any
**	    unexpected disconnections
**      26-mar-09 (Ralph Loen) Bug 121772
**          Removed check for server mode from asipc->zombie_check
**          conditional code.
*/

VOID
GCsend(svc_parms)
SVC_PARMS          *svc_parms;
{
    ASSOC_IPC		*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    ASSOC_CHAN		*a_chan;
    i4 		buflen;		/* must be < mbx size */
    i4			idx;
    i2          mode;

    GCX_TRACER_2_MACRO( "GCsend>", asipc, "%s size %d",
	(i4)(svc_parms->flags.flow_indicator ? "EXP" : "NOR"), 
	svc_parms->svc_send_length );

    svc_parms->status = OK;

    /*
    ** If the partner has connected, allow GCqio_zombie() to check for
    ** dead connections.
    */
    if ( !asipc->zombie_check )
    {
        if ( GCcheckPeer( asipc ) )
            asipc->zombie_check = TRUE;
        else
            asipc->zombie_check = FALSE;
    }

    /* Normal or Expedited flow ? */

    if( svc_parms->flags.flow_indicator )
	a_chan = &asipc->chan[SX];
    else
	a_chan = &asipc->chan[SN];

    /* Sanity check: disallow I/O on closed channel.  Should never happen. */

    if( a_chan->chan == GC_CH_ID_NONE )
    {
	svc_parms->status = GC_ASSN_RLSED;
	svc_parms->sys_err->error = 0;
	goto error;
    }

    /* Disallow jumbo writes. */

    buflen = svc_parms->svc_send_length;

    if( buflen > a_chan->maxiosize )
    {
	svc_parms->status = GC_SND1_FAIL;
	goto error;
    }

    /* Do send */

    if (batch_mode)
    {
	char *shmbuf;
	idx	 = shm_whoami();

	/* check server in use and other party alive */
	if (! (GC_bsm[SHM_SERVER]->bat_inuse &&
	       PCis_alive(GC_bsm[!idx]->process_id)))
	{
	    svc_parms->status = GC_SND1_FAIL;
	    goto error;
	}

	/* Spin while the reader's pages are dirty */
	while (GC_bsm[!idx]->bat_dirty)
	    ;
 
	shmbuf = (char *)GC_bsm[!idx] + sizeof(GC_BATCH_SHM) + 8;
	if (svc_parms->svc_buffer < shmbuf ||
	    svc_parms->svc_buffer > shmbuf + MAX_SHM_SIZE)
	    MEcopy(svc_parms->svc_buffer, buflen, shmbuf);
	else
	    GC_bsm[!idx]->bat_offset = svc_parms->svc_buffer - shmbuf;

	svc_parms->svc_buffer    += buflen;
	GC_bsm[!idx]->bat_sendlen = buflen;
	GC_bsm[!idx]->bat_dirty   = 1;

	/*
	** Now set buflen to 0 so we don't actually send 
	** any data through the mailbox
	*/
	buflen = 0;
    }

    GCqio_exec(
	a_chan->chan,			/* chan */
	IO$_WRITEVBLK|IO$M_NORSWAIT,	/* func */
	&a_chan->ioxb,			/* iosb */
	GCsend_complete,		/* astadr */
	svc_parms,			/* astprm */
	svc_parms->svc_buffer,		/* p1 = VA */
	buflen,				/* p2 = len */
	svc_parms->time_out,		/* p3 = TMO */
	svc_parms->flags.run_sync);	/* sync indicator */

    if (batch_mode)
    {
	/* Spin while the reader has dirty pages and we don't */
	while (GC_bsm[!idx]->bat_dirty && !GC_bsm[idx]->bat_dirty)
	    ;
    }

    return;

error:
    /* On error indicate no system error and complete. */

    svc_parms->sys_err->error = 0;

    (*svc_parms->gca_cl_completion)( svc_parms->closure );
}


/*{
** Name: GCsend_complete - AST for GCsend
**
** Description:
** Handles completion of write requests on the local IPC connection.
**
** Inputs:
**	svc_parms->flags.flow_indicator
**	svc_parms->flags.sync_indicator
**
**	svc_parms->acb->asipc->chan[SN]\
**					 >   depending on flow_indicator
**	svc_parms->acb->asipc->chan[SX]/
**
** Outputs:
**
**	svc_parms->status		if we fail at all
**		GC_ASSOC_FAIL		    iosb was bad.
**	sys_err				o/s error if svc_parms->status is bad.
**
**
** Returns:
**	void function.
** Exceptions:
**     none
**
** Side Effects:
**    svc_parms->svc_send_length -= a_chan->ioxb.ioxb$w_length;
**    svc_parms->svc_buffer      += a_chan->ioxb.ioxb$w_length;
**
** History:
**	28-sep-89 (pasker)
**
**	ASK fix #1b
**
**	1) Remove test where the IOSB is SS$_ABORT or the association is
**	released.   We really dont care.  It's up to mainlnie to deal properly
**	with this condition.
**      26-mar-09 (Ralph Loen) Bug 121772
**          Removed check for server mode from asipc->zombie_check
**          conditional code.
*/

static VOID
GCsend_complete(svc_parms)
SVC_PARMS          *svc_parms;
{
    ASSOC_IPC		*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    ASSOC_CHAN		*a_chan;

    /*
    ** If the partner has connected, allow GCqio_zombie() to check for
    ** dead connections.
    */
    if ( !asipc->zombie_check )
    {
        if ( GCcheckPeer( asipc ) )
            asipc->zombie_check = TRUE;
        else
            asipc->zombie_check = FALSE;
    }

    /* Normal or Expedited flow ? */

    if( svc_parms->flags.flow_indicator )
	a_chan = &asipc->chan[SX];
    else
	a_chan = &asipc->chan[SN];

    /* Pick up status and note data sent */

    GCqio_status( &a_chan->ioxb, GC_SND2_FAIL, svc_parms );

    if( svc_parms->status == OK
     && a_chan->ioxb.ioxb$w_length != svc_parms->svc_send_length )
    {
	svc_parms->status = GC_SND2_FAIL;
	svc_parms->sys_err->error = 0;
    }

    /*
    ** If GCdisconn has been called and this is the last pending I/O,
    ** drive GCdisconn's completion after this one.
    */

    if( asipc->dcon_svc_parms && !GCqio_pending( asipc ) )
    {
	SVC_PARMS *dconn_parms = asipc->dcon_svc_parms;

	asipc->dcon_svc_parms = NULL;

	/* Drive normal completion */

	(*svc_parms->gca_cl_completion)( svc_parms->closure );

	/* Drive disconn completion */

	(*dconn_parms->gca_cl_completion)( dconn_parms->closure );
    }
    else
    {
	/* Drive normal completion */

	(*svc_parms->gca_cl_completion)( svc_parms->closure );
    }
}


/*{
** Name: GCreceive - Receive data on an association
**
** Description:
**
** The user of this routine has requested at most svc_parms->reqd_amount
** bytes of data from the incoming message stream.
** The data read from the incoming messages is placed at the
** address starting at svc_parms->svc_buffer.
**
** GCreceive completes when any data are received.  GCreceive may
** successfully complete with less data that requested.  It places the
** amount of data actually received into svc_parms->rcv_data_length.
**
** If GCreceive receives a "chop mark" generated by the peer calling
** GCpurge, it sets the svc_parms->flags.new_chop flag before completing.
** If this flag is set, the svc_parms->rcv_data_length is not important.
**
** The flags.flow_indicator element of svc_parms specifies whether the receive
** function is to be issued on the normal or expedited flow channel:
** 0 is NORMAL flow, 1 is EXPEDITED flow.
**
** GCreceive is potentially asynchronous: it may return before the
** buffer is filled, and should not block waiting.
** It signals completion by setting the status in the SVC_PARMS and
** driving the callback listed in the SVC_PARMS as gca_cl_completion.
** The call is (*(svc_parms->gca_cl_completion))( svc_parms ).
**
** GCreceive should honor the SVC_PARMS time_out, which indicates
** how long to allow for data to arrive.  If any data arrives before the
** alloted time, GCreceive completes with that data as normal.  If not, it
** completes with a status of GC_TIME_OUT.  A value of -1 instructs
** GCreceive to wait forever; a value of 0 instructs GCreceive to fail if
** no data is immediately available; a value greater than 0 instruct
** GCreceive to wait that many milliseconds for data before failing.
** All implementations of GCreceive must support a timeout of -1 and 0.
** GCreceive may optionally treat a positive timeout as -1.
**
** Possible error conditions are as follows:  If the connection to the
** partner is unexpectedly gone, a status of GC_ASSOC_FAIL is returned.
** If the connection has been terminated locally, by the GCdisconn
** routine, a status of GC_ASSN_RLSED is returned.  The ability to detect
** these two modes of disconnection is system dependent.  If the
** distinction cannot be made in a particular implementation, a status of
** GC_ASSOC_FAIL is the default.  GC_ASSOC_FAIL normally covers network
** errors from which recovery is impossible.  Other CL specific errors
** are possible.
**
** All parameters (both input and output) are handled via a
** pointer to SVC_PARMS structure containing the following
** elements ...
**
** Inputs:
** 	acb->lcb			Local control block
**
** 	time_out           		Timeout in ms, 0=poll, -1=no timeout.
**
** 	char *svc_buffer         	send, receive buffer
**
** 	reqd_amount    			Receive buffer size.
**
** 	(*gca_cl_completion)(svc_parms) Routine to call on completion.
**
** 	flags.flow_indicator:1   	0=normal, 1=expedited flow.
**
**
** Outputs:
** 	status				OK operation succeeded
** 					or any CL failure code.
**
** 	sys_err				OS specific error data.
**
** 	rcv_data_length        		Amount of data received.
**
** 	flags.new_chop:1         	0 = no chop mark received
** 					1 = chop mark received
**
** Returns:
** 	none
**
** Exceptions:
** 	none
**
** Side Effects:
** 	none
**
** History:
**      16-Apr-1987 (berrow)
**          Created.
**	21-oct-1988 (rogerk)
**	    Ifdef'ed out the calls to GCmbx_detect - just count on the ast
**	    driven GCqio_zombie routine to catch any unexpected
**	    disconnections.
**	06-Mar-1990 (ham)
**	    Removed allocation of asipc buffers.  The svc_parms->sva_buffer
**	    is used to read in the data.  Now we only adust the pointers and the
**	    lengths.
**      16-feb-01 (loera01)
**          Detect whether this GCreceive follows an old-style GCsendpeer()
**          message.  If it does, just copy the GCA peer info and call back
**          GCA without doing any reads.
**      26-mar-09 (Ralph Loen) Bug 121772
**          Removed check for server mode from asipc->zombie_check
**          conditional code.
*/

VOID
GCreceive(svc_parms)
SVC_PARMS          *svc_parms;
{
    ASSOC_IPC		*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    ASSOC_CHAN		*a_chan;
    VMS_STATUS		qiostat;
    i4 		buflen;
    IOXB        ioxb;

    GCX_TRACER_2_MACRO( "GCreceive>", asipc, "%s size %d",
	(i4)(svc_parms->flags.flow_indicator ? "EXP" : "NOR"), 
	svc_parms->reqd_amount );

    svc_parms->status = OK;

    /*
    ** If the partner has connected, allow GCqio_zombie() to check for
    ** dead connections.
    */
    if ( !asipc->zombie_check )
    {
        if ( GCcheckPeer( asipc ) )
            asipc->zombie_check = TRUE;
        else
            asipc->zombie_check = FALSE;
    }

    /*
    **  Is this GCreceive being done just after a GClisten, and a 
    **  old-style GCrecvpeer packet was received?  Just copy the GCA
    **  peer info and return.
    */
    if (asipc->old_format)
    {
        MEcopy( (char *)&asipc->rq.rq_assoc_buf1.peer_info,	
		sizeof(asipc->rq.rq_assoc_buf1.peer_info), 
		svc_parms->svc_buffer );
        svc_parms->rcv_data_length = sizeof(asipc->rq.rq_assoc_buf1.peer_info);	
        asipc->old_format = FALSE;
	(*svc_parms->gca_cl_completion)( svc_parms->closure );
        return;
    }

    /*
    ** Perform "first time through" initialization.
    */

    svc_parms->rcv_data_length = 0;	/* Start running count of recv'd data */
    svc_parms->flags.new_chop = FALSE;	/* Ready for possible chop mark */

    /* Normal or Expedited flow ? */

    if( svc_parms->flags.flow_indicator )
	a_chan = &asipc->chan[RX];
    else
	a_chan = &asipc->chan[RN];

    /* Sanity check: disallow I/O on closed channel.  Should never happen. */

    if( a_chan->chan == GC_CH_ID_NONE )
    {
	svc_parms->status = GC_ASSN_RLSED;
	svc_parms->sys_err->error = 0;
	goto error;
    }

    /* Shorten jumbo reads. */

    buflen = svc_parms->reqd_amount;

    if( buflen > a_chan->maxiosize )
	buflen = a_chan->maxiosize;

    /*
    **  The case of a ZERO timeout READ is special and we must simulate
    **  a timeout sequence of calls if there is no data ready for reading.
    */

    if( !svc_parms->time_out )
    {
	/*
	**  Check that there is a message waiting already
	**  ie. we would NOT block.
	*/

	i4 	msg_count = 0;
	ILE3		dvi_list[] = {
			    {4, DVI$_DEVDEPEND, (PTR)&msg_count, 0},
			    {0, 0, 0, 0} };
	IOSB	iosb;

	qiostat = sys$getdviw(EFN$C_ENF, a_chan->chan, 0, dvi_list, &iosb, 0, 0, 0);
	if (qiostat & 1)
	    qiostat = iosb.iosb$w_status;
	if( VMS_FAIL(qiostat) )
	{
	    svc_parms->status = GC_RCV2_FAIL;
	    svc_parms->sys_err->error = qiostat;
	    goto error;
	}

	/* similuate Zero length timeout */

	msg_count &= 0xffff;

	GCTRACE(2)( "thread %x. GCreceive asipc=%x msg_count=%d\n", PCtidx(),
            asipc, msg_count);
	GCX_TRACER_1_MACRO( "GCreceive>", asipc, "msg_count=%d", msg_count );

	if( !msg_count )
	{
	    svc_parms->status = GC_TIME_OUT;
	    svc_parms->sys_err->error = SS$_NORMAL;
	    goto error;
	}
    }

    if (batch_mode)
	/* Not reading data, just being notified that a write has occurred */
	buflen = 0;

    /* Do receive */

    GCqio_exec(
	    a_chan->chan,		/* chan */
	    IO$_READVBLK,		/* func */
	    &a_chan->ioxb,		/* iosb */
	    GCreceive_complete,		/* astadr */
	    svc_parms,			/* astprm */
	    svc_parms->svc_buffer, 	/* p1 = VA */
	    buflen,		    	/* p2 = len */
	    svc_parms->time_out,	/* p3 = TMO */
	    svc_parms->flags.run_sync);	/* sync indicator */

    return;

error:
    /* On error complete. */

    (*svc_parms->gca_cl_completion)( svc_parms->closure );
}


/*{
** Name: GCreceive_complete - completion routine for GCrecieve.
**
** Description:
**
**	When a read completes this routine is executed to check the status of
**	the read and either finish the I/O or issue another read.
**
** Inputs:
**	svc_parms->flags.flow_indicator	    Which ASSOC_CHAN to use:
**	svc_parms->acb->asipc->chan[RN]    ASSOC_CHAN for normal
**	svc_parms->acb->asipc->chan[RX]    ASSOC_CHAN for interrupt
**
** Outputs:
**	none
**
** Returns:
**	VOID function.
**
** Exceptions:
**	none
**
** Side Effects:
**	svc_parms->flags.new_chop	    set to TRUE if ENDOFFILE rec'd
**	acb->async_count		    decremented.
**
** History:
**	12-Aug-87 (jbowers)
**	    Initial function implementation
**
**	28-sep-89 (pasker)
**
**	    ASK fix 1a. General Restructuring.
**
**	    1) Fixed this up so that if there's ANY error, it calls the
**	    mainline completion routine.  It's up to GCA mainline to decide
**	    what to do about this.
**
**	    2) Fix forcing of normal completion when expedited completes so
**	    that
**		(a) it has the proper test between GCmbx_detect and qiostat.
**		(b) it does not use an IOSB, so it doesnt clobber the READ's
**
**	    3) removed some intermediary variables, changd IOSB_ADDR to IOSB.
**
**	    4) restructured the IF(iosb is not ok) test to be more readable.
**
**	    5) the asynch count was being decremented regardless of whether or
**	    not it was asynchronous.
**
**	06-Mar-90 (ham)
**	    Take out the copy from a_chan-bfr to svc_parms->svc_buffer.
**      26-mar-09 (Ralph Loen) Bug 121772
**          Removed check for server mode from asipc->zombie_check
**          conditional code.
**
*/

static VOID
GCreceive_complete(svc_parms)
SVC_PARMS          *svc_parms;
{
    ASSOC_IPC		*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    ASSOC_CHAN		*a_chan;

    /*
    ** If the partner has connected, allow GCqio_zombie() to check for
    ** dead connections.
    */
    if ( !asipc->zombie_check )
    {
        if ( GCcheckPeer( asipc ) )
            asipc->zombie_check = TRUE;
        else
            asipc->zombie_check = FALSE;
    }

    /* Normal or Expedited flow ? */

    if( svc_parms->flags.flow_indicator )
	a_chan = &asipc->chan[RX];
    else
	a_chan = &asipc->chan[RN];

    /* Pick up status and note data received */

    GCqio_status( &a_chan->ioxb, GC_RCV3_FAIL, svc_parms );

    if( !a_chan->ioxb.ioxb$w_length )
	svc_parms->flags.new_chop = TRUE;
    else
	svc_parms->rcv_data_length = a_chan->ioxb.ioxb$w_length;

    GCX_TRACER_1_MACRO( "GCreceive_complete>", asipc, "rcv len = %d",
	a_chan->ioxb.ioxb$w_length );

    if (batch_mode)
    {
	i4     undirty = 0;
	i4	idx     = shm_whoami();
	char    *shmbuf;

	if (GC_bsm[idx]->bat_sendlen == 0 ||
	    /* nothing to read */

	    !PCis_alive(GC_bsm[!idx]->process_id))
	    /* partner is dead */
	{
	    svc_parms->status = GC_RCV1_FAIL;
	    return;
	}
	
	shmbuf = (char *)GC_bsm[idx] + sizeof(GC_BATCH_SHM) + 8;
	if (svc_parms->svc_buffer < shmbuf ||
	    svc_parms->svc_buffer > shmbuf + MAX_SHM_SIZE)
	{
	    /* buffer lies outside shared memory area: copy it */
	    MEcopy(shmbuf + GC_bsm[idx]->bat_offset, 
		   GC_bsm[idx]->bat_sendlen, svc_parms->svc_buffer);
	    undirty = 1 ;
	}
	GC_bsm[idx]->bat_offset = 0;

	svc_parms->svc_buffer      += GC_bsm[idx]->bat_sendlen;
	svc_parms->rcv_data_length -= GC_bsm[idx]->bat_sendlen;  
	/* zero unless read was short */
	GC_bsm[idx]->bat_sendlen = 0;

	if (undirty)
	    /* done reading, this allows writer to proceed */
	    GC_bsm[idx]->bat_dirty = 0;
    }

    /*
    ** If GCdisconn has been called and this is the last pending I/O,
    ** drive GCdisconn's completion after this one.
    */

    if( asipc->dcon_svc_parms && !GCqio_pending( asipc ) )
    {
	SVC_PARMS *dconn_parms = asipc->dcon_svc_parms;

	asipc->dcon_svc_parms = NULL;

	/* Drive normal completion */

	(*svc_parms->gca_cl_completion)( svc_parms->closure );

	/* Drive disconn completion */

	(*dconn_parms->gca_cl_completion)( dconn_parms->closure );
    }
    else
    {
	/* Drive normal completion */

	(*svc_parms->gca_cl_completion)( svc_parms->closure );
    }
}


/*{
** Name: GCpurge - Purge normal flow send channel.
**
** Description:
**
** GCpurge places a synchronization mark ("chop mark") into the normal
** flow channel.  The chop mark flows with the normal data, and causes the
** peer's GCreceive to return with the svc_parms->flags.new_chop indicator
** on receipt.
**
** GCpurge may optionally discard normal data sent with GCsend before
** sending the chop mark, but guarantees a) no data from previous invocations
** of GCsend will follow the chop mark and b) the chop mark will not
** arrive earlier than previously sent expedited data.  Note that the
** order of data flow depends on when GCsend and GCpurge are invoked -
** not when they complete.
**
** GCpurge is synchronous: it completes before returning.
** Although GCpurge has a synchronous interface, it does not block.
** Somehow the GCA CL provides for this.
**
** All parameters (both input and output) are handled via a
** pointer to SVC_PARMS structure containing the following
** elements ...
**
** Inputs:
** 	acb->lcb			Local control block
**
** Outputs:
** 	status				OK operation succeeded
** 					or any CL failure code.
**
** 	sys_err				OS specific error data.
**
** Returns:
** 	none
**
** Exceptions:
** 	none
**
** Side Effects:
** 	none
**
** History:
**      19-Oct-87 (jbowers)
**          Initial function implementation
*/

VOID
GCpurge( svc_parms )
SVC_PARMS	*svc_parms;
{
    ASSOC_IPC		*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    i4			i;

    GCX_TRACER_0_MACRO( "GCpurge>", asipc );

    svc_parms->status = OK;

    /*
    ** In the VMS environment, cancellation of outstanding operations
    ** isn't done, as it causes EOF's to randomly disappear, for obscure
    ** reasons.
    */

    /* Send 2 EOF's (async) */

    for (i = 0; i < 2; i++)
    {
	static IOXB ioxb; /* ignorance is bliss */

	(VOID)sys$qio(
	    EFN$C_ENF, 		        /* Event flag number. */
	    asipc->chan[SN].chan, 	/* I/O channel */
	    IO$_WRITEOF|IO$M_NOW, 	/* Function code and modifier */
	    &ioxb,  			/* I/O status block  */
	    0, 				/* Address of AST routine to call */
	    0, 				/* AST routine parameter */
	    0, 				/* Pointer to message area */
	    0, 				/* Amount to write */
	    0, 0, 0, 0); 		/* Not used */
    }

    /* too bad GCpurge isn't async... */

}


/*{
** Name: GCdisconn - Complete outstanding requests and close assocation.
**
** Description:
**
** GCdisconn performs any I/O necessary to close the association.
** If the implementation of the GCA CL buffers, GCdisconn flushes those
** buffers to the OS.
**
** GCdisconn forces completion of any outstanding GCsend or GCreceive
** requests on the specified association.  It does so by setting the
** status in each request's SVC_PARMS to GC_ASSN_RLSED, and then calling
** (*(svc_parms->gca_cl_completion))( svc_parms ).  These are "orphan"
** requests resulting from an unexpected failure of the association, or
** from normal release of an association which still has requests
** outstanding.  If there are outstanding GCsend requests which GCdisconn
** must abort, GCdisconn need not attempt to flush buffered send data:
** calling GCdisconn while send requests are outstanding implies that
** GCA wishes to abort the association.
**
** GCdisconn is potentially asynchronous: it may return before the
** association is closed, and should not block waiting.
** It signals completion by setting the status in the SVC_PARMS and
** driving the callback listed in the SVC_PARMS as gca_cl_completion.
** The call is (*(svc_parms->gca_cl_completion))( svc_parms ).
**
** GCdisconn honors the SVC_PARMS time_out parameter:
** the timeout specifies how long in
** milliseconds GCdisconn is to attempt to flush buffered data before
** closing the association.  A timeout of -1 indicates GCdisconn should
** not time out, and not complete until the buffers are flushed.  A
** timeout of 0 indicates an abortive release: GCdisconn should close the
** connection immediately, losing any buffered data.  If the
** implementation of the GCA CL does not buffer, the timeout may be
** ignored.
**
** There is no deallocation of association resources (freeing of memory).
** That is the job of GCrelease.
**
** All parameters (both input and output) are handled via a
** pointer to SVC_PARMS structure containing the following
** elements ...
**
** Inputs:
** 	acb->lcb			Local control block
**
** 	time_out           		Timeout in ms, 0=abort, -1=no timeout.
**
** 	(*gca_cl_completion)(svc_parms) Routine to call on completion.
**
**
** Outputs:
** 	status				OK operation succeeded
** 					or any CL failure code.
**
** 	sys_err				OS specific error data.
**
** Returns:
** 	none
**
** Exceptions:
** 	none
**
** Side Effects:
** 	Association flushed and closed.
**
** History:
**      17-Aug-87 (jbowers)
**          Initial function creation
**	04-oct-89 (pasker)
**	    ASK fix #2.
**      11-Nov-89 (ham)
**	    Implement the user timeout and make GCdisconn call the
**	    callers completion routine.
**	13-Jun-90 (jorge)
**	    RE-wrote to conform with correct behavior.
**          - Since M_NOW is not used in SYS$QIO, the user's completion
**	      routine could not have been driven if the I/O had not completed.
**	      Thus, if the user issued the GCdiscon() to tear down the
**	      connection, do it NOW, deassign all mboxes.
**	    - There is no need for a disconnect timer since this is an
**	      immediate service in VMS. The outstanding AST's (if any) will
**	      be driven when the mailbox is removed.
**	    - Once the last AST has been driven, the AST handler will invoke
**	      GCdisconn_complete().
**	    - If no outstanding ASTs, call GCdisconn_complete() directly.
*/

VOID
GCdisconn(svc_parms)
SVC_PARMS          *svc_parms;
{
    ASSOC_IPC		*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    VMS_STATUS		chkstat;
    i4			io_count;
    long 		old_ast;
    short		tmpchan;
    i4			i;

    GCX_TRACER_0_MACRO( "GCdisconn>", asipc );

    svc_parms->status = OK;

    /* In case a GClisten or GCrequest fails before allocating asipc */

    if( !asipc )
    {
	(*svc_parms->gca_cl_completion)(svc_parms->closure);
	return;
    }

    /*
    ** Deassign channels.
    ** This include the GCrequest connect channel chan[CN], which may be
    ** left open by a GCrequest which never proceeds to GCsendpeer.
    ** This drives outstanding I/O's with SS$_CANCEL or SS$_ABORT,
    ** which GCqio_status will translate into GC_ASSN_RLSED.
    ** Set channel to CH_CH_ID_NONE so GCqio_zombie knows to stay away.
    */

    for( i = 0; i < ALLCHANS; i++ )
	if( asipc->chan[ i ].chan != GC_CH_ID_NONE )
    {
	tmpchan = asipc->chan[ i ].chan;
	asipc->chan[ i ].chan = GC_CH_ID_NONE;

	if( VMS_FAIL( chkstat = sys$dassgn( tmpchan ) ) )
	{
	    svc_parms->status = GC_ASSOC_FAIL;
	    svc_parms->sys_err->error = chkstat;
	}
    }

    /* Zombie checking queue fiddling & I/O counting musn't be interrupted. */

    old_ast = sys$setast((char)FALSE);

    /* Remove from zombie checking queue. */

    if( asipc->q.q_next )
	    QUremove( &asipc->q );

    /* If any outstanding I/O's, make the last drive disconn completion. */

    io_count = GCqio_pending( asipc );

    if( io_count )
	asipc->dcon_svc_parms = svc_parms;

    /* Enable ASTs again */

    if(old_ast == SS$_WASSET)
	    sys$setast((char)TRUE);

    /* If there were no outstanding I/O', indicate disconn completion. */

    if( !io_count )
	(*svc_parms->gca_cl_completion)(svc_parms->closure);

    if (batch_mode)
    {
	PID		pid;

	PCpid(&pid);

	if (GC_bsm[SHM_SERVER] != NULL)
	{
	    i4 idx = shm_whoami();

	    if (GC_bsm[idx]->process_id == pid)
	    {
		if ( idx == SHM_SERVER )
		    GC_bsm[idx]->process_id = 0;
		GC_bsm[idx]->bat_dirty = 0;
		GC_bsm[idx]->bat_offset = 0;
		GC_bsm[SHM_SERVER]->bat_inuse = 0;
	    }
	}
    }
}


/*{
** Name: GCrelease - Release buffers allocated during connection.
**
** Description:
**
** Release system specific resources assigned to the association, including
** the LCB.
**
** GCrelease is synchronous: it completes before returning.
** It returns status in the SVC_PARMS.
**
** All parameters (both input and output) are handled via a pointer to
** SVC_PARMS structure containing the following elements ...
**
** Inputs:
** 	acb->lcb                CL communication block
**
** Outputs:
** 	status				OK operation succeeded
** 					or any CL failure code.
**
** 	sys_err				OS specific error data.
**
** Returns:
** 	none
**
** Exceptions:
** 	none
**
** Side Effects:
** 	Deallocator called.
**
** History:
**      16-Apr-1987 (berrow)
**          Created.
**      11-Nov-89 (ham)
**          Removed PCsleep.  This was done for asynchronous disassociate.
*/

VOID
GCrelease(svc_parms)
SVC_PARMS          *svc_parms;
{
    ASSOC_IPC		*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    CL_ERR_DESC err_code;
    i2 mode;
    VMS_STATUS status;

    GCgetMode( &mode );

    svc_parms->status = OK;

    /* Get local control block for this association and release connections. */

    GCX_TRACER_0_MACRO( "GCrelease>", asipc );

    if ( mode == GC_SYNC_CLIENT_MODE )
        status = sys$cantim( &asipc->GCzomb_tmrset, PSL$C_USER );

    /* De-Allocate the Local Control Block */

    if (asipc)
	(*GCfree)(asipc);

    svc_parms->gc_cb = 0;

    if (batch_mode)
    {
	(VOID) MEfree_pages((PTR) GC_bsm[SHM_SERVER], 
			    (GC_bsm[SHM_SERVER]->bat_pages/ME_MPAGESIZE)*2,
			    &err_code);

	/* blast away the connection control blocks */
	GC_bsm[SHM_SERVER] = GC_bsm[SHM_CLIENT] = NULL;
    }

} /* end GCrelease */


/*{
** Name: GCsave - Save data for process switch
**
** Description:
**
**
** GCsave fills a buffer with enough CL private infomation so that the
** association can be recovered by a child process calling GCrestore.
** GCA mainline transports the buffer of data from the GCsave
** call in the parent process to the GCrestore call in the child process.
** 
** It is never the case that both parent and child use the association
** simultaneously.
** 
** There is currently no mechanism for handing an association back to the
** parent process.  It is assumed that once the child process exits, the
** parent process may then continue using the association.
** 
** GCsave may use the allocator provided to GCinitiate.
** 
** GCsave is synchronous: it completes before returning.
** 
** All parameters (both input and output) are handled via a
** pointer to SVC_PARMS structure containing the following
** elements ...
** 
** Inputs:
**	acb->lcb			Local control block
** 
**	char *svc_buffer		buffer to fill
** 
**	reqd_amount			max size of svc_buffer
** 
** Outputs:
**	status				OK operation succeeded
**					or any CL failure code.
** 
**	sys_err				OS specific error data.
** 
**	rcv_data_length			Amount of svc_buffer used.
** 
** Exceptions:
** 	none
**
** Side Effects:
** 	none
**
** History:
**      17-Aug-87 (jbowers)
**          Initial function creation
**      30-Sept-87 (jbowers)
**          Changed to move actual save operations into CL
**	2-Jan-90 (seiwald)
**	    GCA mainline now does the bulk of the save/restore operation.
**	14-jan-99 (loera01) bug 94610
**	    Added size_advise field to save acb->size_advise--a parameter
**	    which GCA sets.  Otherwise, GCnormal_commsz is assumed to be
**	    size_advise, which corrupts the GCA buffers, since it's currently
**	    larger (4120) than GCA (4096).
*/

VOID
GCsave( svc_parms )
SVC_PARMS	*svc_parms;
{
    ASSOC_IPC		*asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    GC_SAVE_DATA	*save_data = (GC_SAVE_DATA *)svc_parms->svc_buffer;

    svc_parms->status = OK;

    /* Get size of buffer */

    if( svc_parms->reqd_amount < sizeof( *save_data ) )
    {
	GCTRACE(0)( "GCsave: buffer too small\n" );
	GCX_TRACER_1_MACRO( "GCsave>", asipc, "buffer too small", 0 );
	svc_parms->status = GC_SAVE_FAIL;
	return;
    }

    svc_parms->rcv_data_length = sizeof( *save_data );

    /* Plop user data into save struct */

    save_data->save_level_major = GC_SAVE_LEVEL_MAJOR;
    save_data->save_level_minor = GC_SAVE_LEVEL_MINOR;
    save_data->am_requestor = asipc->am_requestor;
    save_data->max_mbx_msg_size = asipc->rq.rq_assoc_buf.max_mbx_msg_size;
    MEcopy( (PTR)asipc->rq.rq_assoc_buf.mbx_name, 
	    sizeof( save_data->mbx_name ), (PTR)save_data->mbx_name );
}


/*{
** Name: GCrestore - Restore data following process switch
**
** Description:
** 
** GCrestore recovers an association in a child process, whose data
** was saved by GCsave in the parent process.  GCrestore (re)creates
** the Local Control Block LCB and places the LCB pointer, which
** identifies the connection, into the ACB (acb->lcb).
** 
** GCrestore may use the allocator provided to GCinitiate.
** 
** GCrestore is synchronous: it completes before returning.
** 
** All parameters (both input and output) are handled via a
** pointer to SVC_PARMS structure containing the following
** elements ...
** 
** Inputs:
**      char *svc_buffer                buffer from GCsave
** 
**      svc_send_length                 size of saved buffer
** 
** Outputs:
**      status                          OK operation succeeded
**                                      or any CL failure code.
** 
**      sys_err                         OS specific error data.
** 
**      gcb                        Set to local control block.
**
** Returns:
**	none
**
** Exceptions:
** 	none
**
** Side Effects:
** 	none
**
** History:
**      17-Aug-87 (jbowers)
**          Initial function creation
**      30-Sept-87 (jbowers)
**          Changed to perform all restore operations in CL
**	2-Jan-90 (seiwald)
**	    GCA mainline now does the bulk of the save/restore operation.
**	14-jan-99 (loera01) bug 94610
**	    Added size_advise field to save acb->size_advise--a parameter
**	    which GCA sets.  Otherwise, GCnormal_commsz is assumed to be
**	    size_advise, which corrupts the GCA buffers, since it's currently 
**	    larger (4120) than GCA (4096).
*/

VOID
GCrestore( svc_parms )
SVC_PARMS	*svc_parms;
{
    GC_SAVE_DATA	*save_data = (GC_SAVE_DATA *)svc_parms->svc_buffer;
    ASSOC_IPC		*asipc;
    VMS_STATUS		status;

    svc_parms->status = OK;

    /* Check for proper save block. */

    if( save_data->save_level_major != GC_SAVE_LEVEL_MAJOR )
    {
	svc_parms->status = GC_BAD_SAVE_NAME;
	return;
    }

    /* Allocate an ASIPC */

    if( !( asipc = GCinitasipc() ) )
    {
	svc_parms->status = GC_RESTORE_FAIL;
    	svc_parms->sys_err->error = 0;
	return;
    }

    svc_parms->gc_cb = (PTR)asipc;

    /* Recover parts of asipc. */

    asipc->am_requestor = save_data->am_requestor;
    asipc->rq.rq_assoc_buf.max_mbx_msg_size = save_data->max_mbx_msg_size;

    MEcopy( (PTR)save_data->mbx_name, sizeof( save_data->mbx_name ), 
	    (PTR)asipc->rq.rq_assoc_buf.mbx_name );
    /* Set I/O channel size */

    asipc->chan[RN].maxiosize =
    asipc->chan[SN].maxiosize = asipc->rq.rq_assoc_buf.max_mbx_msg_size;

    /* Assign to the parent process mailboxes */

    status = GCmbx_assoc( asipc->am_requestor, &asipc->rq.rq_assoc_buf, asipc );
    
    if( VMS_FAIL( status ) )
    {
	svc_parms->status = GC_RESTORE_FAIL;
    	svc_parms->sys_err->error = status;
	return;
    }

    /* Put association on zombie checking queue. */

    asipc->zombie_check = TRUE;
    QUinsert( &asipc->q, &GCactive_qhead );
}

/*{
** Name: GCusrpwd - Validate a username/password
**
** Description:
**
** GCusrpwd takes the given user name and password and validates them
** against the system password file.  If both the user name is valid and
** the password is correct for the user, GCusrpwd returns OK; otherwise,
** GCusrpwd returns a descriptive error number.
**
** The user name is not necessarily the same as the one returned by GClisten.
** GClisten returns the local peer's user name; GCusrpwd validates the
** user name for a remote connection.
**
** GCusrpwd is synchronous: it completes before returning.
** It returns its status.
**
** Inputs:
** 	username			user name to validate
**
** 	password			password to validate
**
** 	sys_err				for CL failures
**
** Returns:
** 	STATUS
** 	    OK				user and password valid
** 	    GC_LOGIN_FAIL		user or password not valid
** 	    GC_LSN_FAIL			validation failed
**
**	History:
**	    30-Jan-1989	(Schimmelman)
**		Initial function implementation.
**	    29-sep-1989	(pasker)
**		make it conform to the new CL spec.
**	    06-Dec-1989 (ham)
**		Blank terminate the username and password. Specify correct
**		length in the item_list.
**	    18-Jul-91 (alan) Bug #32306
**		If account has "nopassword" (in AUTHORIZE), accept it.
**          22-Dec-92 (seiwald)
**		Moved code to call sys$hash_password in from GChpwd.mar.
**	    14-feb-00 (loera01 bug 100276
**	    	Added UAI$_EXPIRATION op to check the expiration date of 
**		expired passwords, and added a check for the UAI$M_DISACNT 
**		(disabled account) flag.
*/

STATUS
GCusrpwd(username,password,sys_err)
char	*username;
char	*password;
CL_ERR_DESC *sys_err;
{
    char	encrypt;
    u_i2   	encrypt_len;
    i4     	real_passwd[2];
    u_i2   	real_passwd_len;
    i2     	salt;
    u_i2   	salt_len;
    i4	        flags;
    u_i2	flags_len;
    i4          expire[2];
    u_i2        expire_len;

    ILE3        item_list[6] =
    {
	sizeof(encrypt),     UAI$_ENCRYPT,    &encrypt,    &encrypt_len,
	sizeof(real_passwd), UAI$_PWD,	 (PTR)real_passwd, &real_passwd_len,
	sizeof(salt),	     UAI$_SALT,	 (PTR)&salt,	   &salt_len,
	sizeof(flags),       UAI$_FLAGS, (PTR)&flags,      &flags_len,
	sizeof(expire),      UAI$_EXPIRATION, (PTR)expire, &expire_len,
	0,	    0,		    0,		 0
    };

    i4	ret_passwd[2];
    char        lcl_passwd[80 + 1]; /* leave room for null byte */
    char	lcl_uname[12 + 1]; /* leave  room for null byte */

    struct dsc$descriptor_s
        ret_passwd_dsc = {sizeof(ret_passwd), 0, 0, (PTR)ret_passwd},
	lcl_passwd_dsc = {0,		      0, 0, lcl_passwd},
	lcl_uname_dsc  = {0,                  0, 0, lcl_uname };

    i4     status;
    i4	i;

    char	vers[ 32 ];
    u_i2 	vers_len;

    ILE3		syi_list[2] = {  
			{ sizeof( vers ), SYI$_VERSION, vers, &vers_len },
		        { 0, 0, 0, 0 }  
		};
    i4 tm_now, tm_exp;
    char string[24];
    i4 timeadr[2];
    struct
    {
        i4     length;
        char        *buffer;
    } desc = { 24, string };
    IOSB iosb;

    GCX_TRACER_0_MACRO ("GCusrpwd>", NULL);
    CL_CLEAR_ERR(sys_err);
    /*  Validate the pointers  */

    if (username == NULL || password == NULL)
        return(GC_LOGIN_FAIL);

    /*
    ** fill output string, copy user's data, convert it to uppercase
    */

    MEfill( sizeof(lcl_uname), (u_char)' ', lcl_uname );
    lcl_uname_dsc.dsc$w_length = STlcopy(username, lcl_uname,
					sizeof(lcl_uname) - 1);
    lcl_uname[lcl_uname_dsc.dsc$w_length] = ' ';
    CVupper(lcl_uname);

    /*
    ** same for password.
    */
    MEfill(sizeof(lcl_passwd), (u_char)' ', lcl_passwd );
    lcl_passwd_dsc.dsc$w_length = STlcopy(password,lcl_passwd,
					sizeof(lcl_passwd) - 1);
    lcl_passwd[lcl_passwd_dsc.dsc$w_length] = ' ';
    CVupper(lcl_passwd);

    /*
    ** Get the password, algorithm, and salt values
    */
    sys_err->error = sys$getuai(0, 0, &lcl_uname_dsc, item_list, 0, 0, 0);

    if (VMS_FAIL(sys_err->error))
    {
	/*
	** Failed to read the UAF.  If its cause the username wasnt found
	** then return login fail (to inhibit 'poking' for valid user names)
	** otherwise return an error that we had some other problem.
	*/
	if (sys_err->error == RMS$_BUSY)
	    return(GC_USRPWD_FAIL);
	else if (sys_err->error == RMS$_RNF)
	    return(GC_LOGIN_FAIL);
	else
	    return(GC_USRPWD_FAIL);
    }

    /*  Check to see if the password is expired or the account is disabled  */

    if ((flags & UAI$M_PWD_EXPIRED) || (flags & UAI$M_DISACNT))
	return(GC_LOGIN_FAIL);

    string[23] = 0;
    sys$gettim(timeadr);
    tm_now = TMconv(timeadr);
    tm_exp = TMconv(expire);
    if (expire[0] && expire[1] && (tm_exp < tm_now))
	return(GC_LOGIN_FAIL);

    /*
    **  If hashed password in the SYSUAF is zero, then account has nopassword 
    **  (at least that's the way it works on encryption algorithms 2 (VMS 5.3 
    **  and earlier) and 3 (VMS 5.4)).  We accept it.
    */
    if((real_passwd[0] == 0) && (real_passwd[1] == 0))
            return(OK);

# if (defined(ALPHA) || defined(IA64))
    sys_err->error = sys$hash_password(
				    &lcl_passwd_dsc,
				    encrypt,
				    salt,
				    &lcl_uname_dsc,
				    ret_passwd);
# else /* ALPHA | IA64 -- don't really need this section anymore */

    /*
    ** In VMS 5.4 or higher, we use sys$hashpasswd
    ** else, we use GChpwd - our version filched from fiche
    */

    sys_err->error = sys$getsyiw(EFN$C_ENF, 0, 0, syi_list, &iosb, 0, 0);
    if (sys_err->error & 1)
	sys_err->error = iosb.iosb$w_status;

    if (VMS_FAIL(sys_err->error))
	    return(GC_USRPWD_FAIL);

    if( vers[1] > '5' || vers[1] == '5' && vers[3] >= '4' )
    {
	/*
	** To link on VMS 5.3 and earlier, we can't call sys$hash_password
	** directly.  Instead we use the VMS transfer vector, guaranteed
	** not to change.
	*/

	i4 (*sys$hash_password)() = (i4 (*)())0x7ffee538;

	sys_err->error = (*sys$hash_password)(
				    &lcl_passwd_dsc,
				    encrypt,
				    salt,
				    &lcl_uname_dsc,
				    ret_passwd);
    }
    else
    {
	sys_err->error = GChpwd(
				    &ret_passwd_dsc,
				    &lcl_passwd_dsc,
				    encrypt,
				    salt,
				    &lcl_uname_dsc);
    }
# endif /* ALPHA */

    /*  Encrypt user name & passwd given on command line */

    if (VMS_FAIL(sys_err->error))
	return(GC_LOGIN_FAIL);

    /*  Compare the two password quadwords  */

    if( ret_passwd[0] != real_passwd[0] || ret_passwd[1] != real_passwd[1] )
	return(GC_LOGIN_FAIL);

    return(OK);
}


/*{
** Name: GCnsid - Establish or obtain the identification of the local Name Server
**
** Description:
**
** GCnsid is invoked to establish and to obtain the identity of the Name
** Server in a system-dependent way.  This routine provides the
** "bootstrapping" capability required to allow the GCA_REQUEST
** service to establish an association with the Name Server.
**
** GCnsid provides four operations:  RESV, SET, CLEAR, and FIND.  The Name
** Server uses RESV, SET, and CLEAR to make its listen address public on
** startup and to remove its listen address on shutdown.  FIND is used to
** retrieve this listen address by GCA clients wishing to connect to the
** Name Server.
**
** RESV and CLEAR form a primitive mechanism to prevent two Name Servers
** from running simultaneouly.  They may always return OK.
**
** The exact syntax of the Listen ID may be system dependent: it
** is understood only by the GCA CL. The identifiier consists of a
** null-terminated character string.
**
** Inputs:
** 	flag				Operations code
**
** 	    GC_RESV_NSID		Called before GC_SET_NSID.
**
** 	    GC_SET_NSID			Establish a globally accessible
** 					identifier for the Name Server.
**
** 	    GC_CLEAR_NSID		Clear the Name Server identifier,
** 					so any subsequent FIND_NSID will
** 					fail.
**
** 	    GC_FIND_NSID		Retrieve the identifier of the Name
** 					Server in the local node.
**
**
** 	listen_id			Source buffer for SET operation.
**
**     	length				The maximum length of listen_id for
** 					the SET or FIND operations.
**
** Outputs:
**     	listen_id                  	Target buffer for FIND operation.
** 					GCA_GET_NSID only
**
**     	sys_err				CL specific error info.
**
**
** Returns:
** 	STATUS
** 	    OK				Operation successful
** 	    GC_NM_SRVR_ID_ERR		Unable to get/set ND id.
**
** Exceptions:
** 	none
**
** Side Effects:
** 	none
**
** History:
**      24-Sept-1987 (lin,jbowers)
**          Created.
**	23-jun-1988 (lin)
**	    add to set process name.
**      05-Nov-88 (jbowers)
**	    Corrected the translation of II_GCNii to follow the normal search
**	    order of logical name tables.
**	05-Nov-88 (jorge)
**	    Added the GC_RESV_LISTEN state to address race conditions with
**	    possible simultaneous GCN initialization. Also added svc_parms
**	    to conform with calling interface.
**	17-may-90 (ham)
**	    Add GC_CLEAR_NSID.  On termination of the name server this
**	    routine is called to deassign the name server id logical.
*/

STATUS
GCnsid(flag, gca_listen_id, length, sys_err)
i4		flag;
char		*gca_listen_id;
i4		length;
CL_ERR_DESC	*sys_err;
{

    $DESCRIPTOR(table_dsc, "LNM$SYSTEM");
    ILE3			lnm_item_list[2];
    struct dsc$descriptor_s	logdesc;
    VMS_STATUS		status;
    u_i2		ret_leng;
    char		tmp_lsn_id[256];
    char		symval[256];

    /*
    **   On VMS, the mechanism used to semaphore is the Name Server
    **   process name itself. Since VMS process names are unique,
    **   only one Name Server at a time can even start initialitization.
    **   On VMS, the Name Server process name is of the form
    **   "II_GCNii" where 'ii' is the II_INSTALLATION 2-letter
    **   code. The mechanism used to identify what GCN listen address
    **   to use for a given installation is to store the GCF address
    **   in an installation specific VMS logical. The logical that is
    **   used is of the same name as the Name Server process name.
    */

    STpolycat(2,"II_GCN", GC_instcode, symval );

    logdesc.dsc$a_pointer = symval;
    logdesc.dsc$w_length = STlength(logdesc.dsc$a_pointer);
    logdesc.dsc$b_dtype = DSC$K_DTYPE_T;
    logdesc.dsc$b_class = DSC$K_CLASS_S;

    switch(flag)
    {
      case GC_RESV_NSID:
      {
	break;
      }/* end case GC_RESV_NSID */

      case GC_SET_NSID:
      {
      	lnm_item_list[0].ile3$w_code = LNM$_STRING;
	lnm_item_list[0].ile3$w_length = (short) length ;
	lnm_item_list[0].ile3$ps_retlen_addr = &ret_leng;
	lnm_item_list[0].ile3$ps_bufaddr = gca_listen_id;
	lnm_item_list[1].ile3$w_code = 0;
	lnm_item_list[1].ile3$w_length = 0;

	status = sys$crelnm(0, &table_dsc, &logdesc, 0,	lnm_item_list);
	if((status != SS$_NORMAL) && (status != SS$_SUPERSEDE))
	{
		sys_err->error = status;
		return(GC_NM_SRVR_ID_ERR);
	}
	break;
      }/* end case GC_SET_NSID */

      case GC_FIND_NSID:
      {
	/*
	** initialize item list and translate SYS$DISK
	*/
	ILE3 itmlst[ ] = { 
		{ sizeof( tmp_lsn_id ), LNM$_STRING, tmp_lsn_id, &ret_leng },
		{ 0, 0, NULL, NULL } };

	status = sys$trnlnm( NULL, &table_dsc, &logdesc, NULL, itmlst );
	if ( !(status & 1) )
	{
	    GCTRACE_STATUS(1)( "GCnsid> ", status );
	    sys_err->error = status;
	    return(GC_NM_SRVR_ID_ERR);
	}

	tmp_lsn_id[ret_leng] = '\0';
	STlcopy(tmp_lsn_id, gca_listen_id, length);
	break;
      }/* end case GC_FIND_NSID */

      case GC_CLEAR_NSID:
      {
	/*
	**  Deassign the logcal the name server is coming down.
	*/
	status = sys$dellnm(&table_dsc, &logdesc, 0);

	if ( status != SS$_NORMAL )
	{
		sys_err->error = status;
		return(GC_NM_SRVR_ID_ERR);
	}
	break;
      }

      default:		/* Bad parm */
		sys_err->error = 0;
	    	return(GC_NM_SRVR_ID_ERR);
    }
    sys_err->error = 0;
    return(OK);
} /* end GCnsid */


/*{
** Name: GChostname - Request the local host name.
**
** Description:
**
** Attempts to find the local host name as appropriate for a given
** implementation.  The only requirement for the local host name
** is that it be unique among hosts using shared disks to access
** a common INGRES installation.
**
** Inputs:
** 	hostlen			 maximum length of ourhost string
**
** Outputs:
** 	ourhost			 the local hostname buffer string
**
** Returns:
** 	none
**
** Side Effects:
** 	none
**
**
** History:
**      8-Jun-88 lin
**          Modified from 5.0 NTHNAME.
**	21-Sept-88 Jorge
**	    Simplified to handle only local hostname lookup and make robust.
**	26-Aug-98 (kinte01) Bug 72325
**	    Replaced translation of SYS$NODE with sys$getsyi() to get the host
**	    name independent of whether or not DECNet is installed.
**      07-Nov-2000 (horda03) Bug 103137
**          Obtain USERNAME of a session from the connect Process ID (not the
**          MASTER PID. This gets round a problem with QASETUSER on VMS 7.2.
**	23-feb-01 (loera01)
**          Actually allocated space for the node name string which previously 
**          somehow got away with passing the address of an unallocated string.
**	27-feb-01 (kinte01)
**	    Change GChostname to use $SYIDEF instead of own structure
**	    for sys$getsyi call
**	26-oct-01 (kinte01)
**	    clean up compiler warnings
**/

VOID
GChostname(char *ourhost, i4  hostlen)
{

	char nodename[8];

    ILE3		itmlst[]
        =
        {
                {sizeof(nodename), SYI$_NODENAME, nodename, 0},
                {0, 0, 0, 0}
        };
    IOSB		iosb;
    VMS_STATUS		qiostat;

    GCX_TRACER_0_MACRO ("GChostname>", NULL);

    qiostat = sys$getsyiw(EFN$C_ENF, 0, 0, itmlst, &iosb, 0, 0);
    if (qiostat & 1)
	qiostat = iosb.iosb$w_status;

    if( VMS_FAIL(qiostat) )
    {
	*ourhost = NULL;
    }
    else
    {
        STlcopy(nodename, ourhost, hostlen -1);
    }
} /* end GChostname */

/*
** Name: GCshm_addr - turn a string into shared memory id
**
** Description:
**	This routine takes a listen address and turns it into a VMS mailbox name
**
*/
static PTR
shm_addr( SVC_PARMS *svc_parms, i4 op )
{
    PID		pid;
    i4		len;
    PTR		memory;
    i4		alloc_pages;
    i4		bat_pages;
    i4		size;
    i4		flags;
    i4		status;
    CL_ERR_DESC err_code;

    if ( svc_parms != NULL )
    {
	/* some algorithm to conjure up a name for the resource */

	/* results in bsm.bat_id being written to */
	/* may use PCpid( &pid ) */
	
	/* compute the size */
	size = MAX_SHM_SIZE + sizeof(GC_BATCH_SHM);
	bat_pages = size / ME_MPAGESIZE;
	
	/* if there is a leftover, allocate an extra page for it */
	if ( size % ME_MPAGESIZE > 0 )
	    bat_pages++;
    }

    if ( GC_bsm[SHM_SERVER] == NULL )
    {
	flags = ME_SSHARED_MASK;
	if ( op == SHM_CREATE )
	    flags |= (ME_CREATE_MASK | ME_MZERO_MASK);
	
	/* Get the memory */

   	status = MEget_pages(flags,
			     bat_pages*2,
			     svc_parms->listen_id,
			     (PTR*)&GC_bsm[SHM_SERVER],
			     &alloc_pages,
			     &err_code);

	/* if already exists, call it again with new flags */

	if ( op == SHM_CREATE && status == ME_ALREADY_EXISTS )
	    status = MEget_pages(ME_SSHARED_MASK,
				 bat_pages*2,
				 svc_parms->listen_id,
				 (PTR*)&GC_bsm[SHM_SERVER],
				 &alloc_pages,
				 &err_code);

  	if ( status != OK )
	    /* it died */
	    return( NULL );

	/* put the client memory right after the server memory */

	GC_bsm[SHM_CLIENT] = (GC_BATCH_SHM*)((char*)GC_bsm[SHM_SERVER]+
					     bat_pages*ME_MPAGESIZE);

	if ( op == SHM_CREATE )
	{
	    /* copy the name */

	    STcopy( (char *) svc_parms->listen_id, (char *) GC_bsm[SHM_SERVER]->bat_id );

	    /* set number of pages for both server & client */

	    GC_bsm[SHM_SERVER]->bat_pages = 
	    GC_bsm[SHM_CLIENT]->bat_pages = bat_pages*ME_MPAGESIZE;
	}
    }

    if ( svc_parms == NULL && op == SHM_ATTACH )
    {
	/*
	** We're just attaching to memory that already exists,
	** return the pointer to the other guy's memory
	*/
	return (PTR) (GC_bsm[!shm_whoami()]);
    }

    return (PTR) GC_bsm[SHM_SERVER];
}

static i4
shm_whoami()
{
    static PID pid = 0;

    if (pid == 0)
	PCpid( &pid );

    if ( GC_bsm[SHM_SERVER]->process_id == pid )  /* are we Server? */
	return SHM_SERVER;

    return SHM_CLIENT;
}

/*{
** Name: GCinitasipc - allocate and initialize ASSOC_IPC
**
** Returns:
**	0		allocation failure
**	ptr		ASSOC_IPC ptr
**
** History:
**	27-Dec-90 (seiwald)
**	    Written.
**      29-Sep-1998 (horda03)
**          Initialise w_can_status to 0. Any other value will stop messages
**          being posted to the channel. Bug 90634.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

static ASSOC_IPC *
GCinitasipc()
{
    ASSOC_IPC	*asipc;
    GC_THREAD *gc_thread = &main_gc_thread;
    STATUS    status;
    i2        mode;

    GCgetMode(&mode);

    if (mode == GC_SYNC_CLIENT_MODE)
    {
        if (!gc_thread_key)
            ME_tls_createkeyFreeFunc(&gc_thread_key, GCfree, &status);
    
        ME_tls_get(gc_thread_key, (PTR *)&gc_thread, &status);
    
        if (!gc_thread)
        {
            if( !( gc_thread = (GC_THREAD *)(*GCalloc)( sizeof(GC_THREAD))) )
            {
       	        GCX_TRACER_0_MACRO( "GCinitasipc>fail>GCalloc fail", NULL );
    	        return 0;
            }
    
            gc_thread->counter = 0;
    
            QUinit(&gc_thread->GCsync_ioxb_qhead);

            ME_tls_set(gc_thread_key, (PTR)gc_thread, &status);
        }  /* if (!gc_thread) */
    }  /* if (mode != GC_SERVER_MODE) */


    /*
    ** Allocate local IPC management structure
    */

    if( !( asipc = (ASSOC_IPC *)(*GCalloc)( sizeof(ASSOC_IPC))) )
    {
	GCX_TRACER_0_MACRO( "GCinitasipc>fail>GCalloc fail", NULL );
	return 0;
    }

    /* ... and clear it */

    MEfill( sizeof(ASSOC_IPC), 0, (PTR)asipc );

    /* Initialize ALL channel status to IDLE */

    asipc->chan[SN].ioxb.ioxb$w_status =
    asipc->chan[RN].ioxb.ioxb$w_status =
    asipc->chan[SX].ioxb.ioxb$w_status =
    asipc->chan[RX].ioxb.ioxb$w_status =
    asipc->chan[CN].ioxb.ioxb$w_status = GC_CH_WS_IDLE;

    /* Initialise w_can_status to 0. Any other value will stop messages
    ** being posted to the channel.
    */
    asipc->chan[RN].ioxb.ioxb$w_can_status =
    asipc->chan[SX].ioxb.ioxb$w_can_status =
    asipc->chan[RX].ioxb.ioxb$w_can_status =
    asipc->chan[CN].ioxb.ioxb$w_can_status = 0;

    /* Initialize association ALL  chanel IDs to "empty" */

    asipc->chan[RN].chan =
    asipc->chan[SN].chan =
    asipc->chan[RX].chan =
    asipc->chan[SX].chan =
    asipc->chan[CN].chan = GC_CH_ID_NONE;

    /* Set I/O channel size */

    asipc->chan[RN].maxiosize =
    asipc->chan[SN].maxiosize = GCnormal_commsz;

    asipc->chan[RX].maxiosize =
    asipc->chan[SX].maxiosize = SZ_RIBUFFER;

    asipc->chan[CN].maxiosize = sizeof( GC_RQ_ASSOC1 );

    asipc->dcon_svc_parms = NULL;
    asipc->am_requestor = 0;
    
    asipc->GCzomb_tmrset = FALSE;
    asipc->thread_id = PCtidx();

    /* return it */

    GCTRACE(1)( "thread %x. GCinitasipc asipc=%x\n", PCtidx(), asipc );
    GCX_TRACER_0_MACRO( "GCinitasipc>", asipc );

    return asipc;
}


/*{
** Name: GCgetsym - get a symbol with installation code in it
**
** Inputs:
**	sym1	part of symbol name before the installation code
**	sym2	part of symbol name after the installation code
**	len	max length of result
**
** Outputs:
**	result	filled in result
**
** History:
**	13-Dec-90 (seiwald)
**	    Written.
*/

static VOID
GCgetsym( sym1, sym2, result, len )
char	*sym1;
char	*sym2;
char	*result;
i4	len;
{
    char	*val;
    char	symbol[ 128 ];

    STpolycat( 3, sym1, GC_instcode, sym2, symbol );

    NMgtAt( symbol, &val );

    STlcopy( val && *val ? val : "", result, len );

    GCTRACE(2)( "thread %x. GCgetsym %s -> %s\n", PCtidx(), symbol, result );
}


/*{
** Name: GCtim_strtoms - convert a string time value into milliseconds
**
** Inputs:
**	convtim	time value to convert
**
** Returns:
**	-1 on error
**	milliseconds
**
** History:
**	13-Dec-90 (seiwald)
**	    Written.
*/

static long
GCtim_strtoms( convtime )
char	*convtime;
{
    struct      dsc$descriptor_s timdsc;
    long	minus10k = -MILLI_TO_BINTIM;
    long	bintime[2];
    long	millis = -1;
    long	junk;

    /* Point string descriptor at convtime */

    timdsc.dsc$b_class = DSC$K_CLASS_S;
    timdsc.dsc$b_dtype = DSC$K_DTYPE_T;
    timdsc.dsc$a_pointer = convtime;
    timdsc.dsc$w_length = STlength( convtime );

    /* Convert convtime & scale to miliseconds.  */

    if( VMS_OK( sys$bintim( &timdsc, bintime ) ) )
	lib$ediv( &minus10k, bintime, &millis, &junk );

    GCTRACE(2)( "thread %x. GCtim_strtoms %s -> %d ms\n", PCtidx(),
        convtime, millis );

    return millis;
}


/*{
** Name: GCtim_mstotim - convert milliseconds to a bintim
**
** Inputs:
**	millis	milliseconds of timer
**
** Outputs:
**	bintim	filled in result
**
** History:
**	13-Dec-90 (seiwald)
**	    Written.
*/

static VOID
GCtim_mstotim( millis, bintime )
long	millis;
long	bintime[2];
{
    long zero = 0;
    long minus10k = -MILLI_TO_BINTIM;

    /*
    ** The VMS representation of time interval is -(# of 100ns intervals).
    ** Multiply milliseconds by -10000 to get this.
    */

    lib$emul( &millis, &minus10k, &zero, bintime );
}


/*{
** Name: GCsync - Drive Asynchronous Events and wait for Interrupt
**
** Description:
**
** GCsync drives the event handling mechanism in synchronous GCA
** applications.  For each requested GCA operation, GCA posts one or
** more asynchronous GCA CL requests and then passes control to GCsync.
**
** GCsync drives asynchronous event completion as GCexec does, but must
** return if either a) all outstanding requests have completed or b) a
** keyboard signal (^C) is delivered to the process.  GCsync may return
** prematurely; GCA will simply reinvoke it.
**
** GCsync is called after inhibiting deliver of keyboard signals with
** EXinterrupt( EX_OFF ).  GCsync must return if a keyboard signal is
** waiting to be delivered with EXinterrupt( EX_ON ).
**
** Inputs:
**     	none
**
** Outputs:
** 	none
**
** Returns:
** 	none
**
** Exceptions:
** 	none
**
** Side Effects:
** 	Events driven.
**
** History:
**      30-Jan-1990 (ham)
**          Created.
**	27-Dec-90 (seiwald)
**	    Rewritten.
*/

VOID
GCsync(i4 timeout)
{
    long 	old_ast;
    IOXB   	*ioxb;
    bool 	io_done = FALSE, tm_done = FALSE;
    GC_THREAD   *gc_thread = &main_gc_thread;
    STATUS      status;
    QUEUE       *q,*p;
    i4          counter = 0;
    STATUS      istatus;
    i4          i = 0;
    QUEUE       ioxb_q;
    i2          mode;

    GCgetMode(&mode);

    QUinit(&ioxb_q);

    if (mode == GC_SYNC_CLIENT_MODE)
        ME_tls_get(gc_thread_key, (PTR *)&gc_thread, &istatus);

    /* wait for I/O or timer event flag */

    GCTRACE(2)("thread %x. GCsync entry counter %d mode %d\n", PCtidx(),
        gc_thread->counter, mode);

    if (mode != GC_SYNC_CLIENT_MODE)
    {
        /* Enable AST's - they should be off. */
        old_ast = sys$setast((char)TRUE); 

        if( VMS_FAIL( sys$wflor( GCio_ef, GC_EF_MASK ) ) )
        {
            GCTRACE(0)( "thread %x. GCsync: sys$wflor failed\n", PCtidx() );
            GCX_TRACER_1_MACRO( "GCsync>", NULL, "sys$wflor failed", 0 );
            lib$signal(SS$_DEBUG);
        }

        /* Redisable AST's, they should had been turned off. */
        if( old_ast == SS$_WASCLR )
       	     sys$setast( (char)FALSE );

        /* 
        ** Check and clear event flags. 
        ** GCtm_ef is set by sys$setimr completion (from GCqio_exec).
        */
        
        tm_done = sys$clref( GCtm_ef ) == SS$_WASSET;
        io_done = sys$clref( GCio_ef ) == SS$_WASSET;

        GCTRACE(2)( "thread %x. GCsync wflor io=%d timer=%d\n", PCtidx(),
        io_done, tm_done );
        GCX_TRACER_2_MACRO( "GCsync>", 0, "wflor io=%d timer=%d", 
            io_done, tm_done);
    }
  
    /*
    ** Build a queue of completed IOXB's.  Drive time-outs on any timed-out
    ** I/O.  Delete completed I/O queue from the TLS queue.
    */  
    for (q  = gc_thread->GCsync_ioxb_qhead.q_prev;
         q != &gc_thread->GCsync_ioxb_qhead;
         q  = p)
    {
        ioxb = ((GC_IOXB_QUEUE *)q)->GCsync_ioxb;
        p = q->q_prev;

        GCTRACE(5)("GCsync checking ioxb %p\n",ioxb);
        if (mode == GC_SYNC_CLIENT_MODE)
        {
            GCTRACE(5)("GCsync waiting for ioxb %p\n",ioxb);
            /* Enable AST's - they should be off. */
            old_ast = sys$setast((char)TRUE); 

            if (VMS_FAIL(sys$synch(EFN$C_ENF, ioxb)))
            {
                GCTRACE(0)( "thread %x. GCsync: sys$sync failed\n",PCtidx() );
                GCX_TRACER_1_MACRO( "GCsync>", NULL, "sys$sync failed", 0 );
                lib$signal(SS$_DEBUG);
            }
            /* Redisable AST's, they should had been turned off. */
            if( old_ast == SS$_WASCLR )
       	        sys$setast( (char)FALSE );
        }

        /*
        ** Synchronous clients invoke GCqio_timeout from SYS$SETIMR()
        ** (as set in GCqio_exec()) because SYS$SYNC() requires the status 
        ** field of IOXB block to be non-zero in order to return.  Asynchronous
        ** servers do the same thing, but in that case GCsync() is never 
        ** invoked, and this bit of code is never touched.  Servers with sync 
        ** I/O and asynchronous clients invoke GCqio_timeout() from GCsync(). 
        ** This is the preferred method.
        */
        if (mode == GC_SYNC_CLIENT_MODE)
        {
  	    if( ioxb->ioxb$w_status != SS$_CANCEL
	        && ioxb->ioxb$w_status != SS$_ABORT )
            tm_done = TRUE;
        }
        else if( ioxb->ioxb$l_tmo > 0 && tm_done ) 
        {                                          
            GCqio_timeout( ioxb );
        }

      	if( ioxb->ioxb$w_status != 0 )
        {

            if (mode == GC_SYNC_CLIENT_MODE)
                io_done = TRUE;

            QUremove(q);

            QUinsert(q, &ioxb_q);
            gc_thread->counter--;
        }

        if (mode == GC_SYNC_CLIENT_MODE)
        {
            GCTRACE(2)( "thread %x. GCsync sync io=%d timer=%d\n", PCtidx(),
                io_done, tm_done );
        }
    }  
    
    /* 
    ** Drive all completed I/O's to completion.
    */
    for (q = ioxb_q.q_prev; q != &ioxb_q; q = p)
    {
        ioxb = ((GC_IOXB_QUEUE *)q)->GCsync_ioxb;

        GCTRACE(2)( "thread %x. GCsync ioxb %x\n", PCtidx(), ioxb);
        GCX_TRACER_1_MACRO( "GCsync>", ioxb, "queue ptr %x", q);

        p = q->q_prev;

  	GCqio_complete( ioxb );

        QUremove(q);
 
        (*GCfree)((PTR)q);
    }

    GCTRACE(2)("thread %x. GCsync exit\n",PCtidx());
    GCX_TRACER_0_MACRO ("GCsync exit>", NULL);
}


/*{
** Name: GCqio_exec - sys$qio with all the trimmings
**
** Description:
**	Posts a sys$qio: operation is function func on channel chan.
**	Buffer is va and its size is len.
**
**	Uses an IOXB (extended I/O status block) for storing control
**	information.
**
**	Calls (*astadr)(astprm) on success or failure.  I/O status
**	must be propagated to svc_parms by calling GCqio_status.
**
**	Takes a positive timeout tmo in milliseconds.  On timeout,
**	GCqio_status will inidicate a GC_TIME_OUT status.
**
**	Takes a sync flag indicating that the user will wait for
**	I/O completion by calling GCsync().
**
**	All I/O's pass through these routines:
**		GCqio_exec	- post the I/O
**		GCqio_timeout	- called if I/O times out
**		GCqio_complete	- called on I/O completion
**		(*astadr)()	- called by GCqio_complete
**		GCqio_status	- called by (*astadr)()
**
**	If the operation is async, GCqio_timeout will be called as the
**	AST of a timer set with sys$setimr and GCqio_complete will be
**	called as the AST of the sys$qio.  If the operation is sync,
**	these GCqio_timeout and GCqio_complete will be called by GCsync
**	as it examines the event flags handed to sys$setimr and sys$qio.
**
**	The operation may be cancelled by GCqio_timeout, by GCqio_zombie
**	(the "parnterless connection" checking routine), or by GCdisconn.
**	The first two issue sys$cancel's on the I/O's, while GCdisconn
**	calls sys$dassgn on the channel.  These all cause the AST to
**	delivered for async calls and the event flag to be set for sync
**	calls.
**
**	For some customer somewhere, there is a front end "idle process"
**	checking routine which must be disabled for sync I/O's.  Hence
**	the various calls to INGRES 5.0's INtoxxxx().
**
** Inputs:
**	chan	channel mailbox is assigned to
**	func	function code for sys$qio
**	ioxb	status control block
**	astadr	routine to call on completion
**	astprm	parameter to hand to (*astadr)().
**	va	buffer pointer
**	len	buffer len
**	tmo	timeout, in millseconds; <= 0 means no timeout.
**	sync	user will call GCsync to wait for I/O.
**
** Side effects:
**	sys$setimr called to set timer.
**	sys$qio called to post I/O.
**	zombie checking may be invoked.
**
** History:
**	27-Dec-90 (seiwald)
**	    Rewritten and ocumented.
**      29-Sep-1998 (horda03)
**          Abort the operation if a process has disassociated from the
**          mailbox. Bug 90634.
*/

static VOID
GCqio_exec( chan, func, ioxb, astadr, astprm, va, len, tmo, sync )
i4	chan;
i4	func;
IOXB	*ioxb;
VOID	(*astadr)();
long 	astprm;
PTR	va;
i4	len;
i4	tmo;
i4	sync;
{
    VMS_STATUS 	status;
    GC_THREAD   *gc_thread = &main_gc_thread;
    GC_IOXB_QUEUE *gc_ioxb_queue;
    i2 mode;
    SVC_PARMS *svc_parms = (SVC_PARMS *)astprm;
    ASSOC_IPC *asipc = (ASSOC_IPC *)svc_parms->gc_cb;
    PTR reqidt = 0;

    /*
    ** If GCexec() is invoked, gcacl is invoked in a purely asynchronous 
    ** context, with the possible exception of GCdisconn().  Gcacl is tipped 
    ** off that GCexec() is about to be called by a prior call to GCregister().
    **
    ** Only servers call GCregister().  This is the origin of the 
    ** GC_SERVER_MODE.
    **
    ** Only the Ingres API invokes gcacl asynchronously without calling
    ** GCregister().  This is the origin of the GC_ASYNC_CLIENT_MODE.
    **
    ** Only client processes based on LIBQ invoke gcacl exclusively in
    ** sync mode.  This is the origin of the GC_SYNC_CLIENT_MODE.  Only
    ** synchronous client processes are allowed to use pthread routines
    ** used by MEtls functions.  Otherwise, the process is assumed to be
    ** single-threaded.
    ** 
    ** If the mode is GC_ASYNC_CLIENT_MODE, GCsync() must be eventually
    ** invoked via IIapi_wait().  Override the default "sync" flag 
    ** passed from GCA and process all I/O's as if they were called 
    ** synchronously.
    */
  
    GCgetMode(&mode);
 
    if (mode == GC_SYNC_CLIENT_MODE && !sync)
    {
        mode = GC_ASYNC_CLIENT_MODE;
        GCsetMode(mode);
        sync = TRUE;
    }

    if (mode == GC_SYNC_CLIENT_MODE)
        ME_tls_get(gc_thread_key, (PTR *)&gc_thread, &status);

    GCTRACE(1)("thread %x. GCqio_exec ioxb=%x func=%x tmo=%d sync=%d mode=%d\n",
	PCtidx(), ioxb, func, tmo, sync, mode );
    GCX_TRACER_4_MACRO( "GCqio_exec>", ioxb, "func=%x tmo=%d sync=%d mode=%d",
	    func, tmo, sync, mode );

    /* Check that the required processes are still connected */

    if (ioxb->ioxb$w_can_status)
    {
       /* There is only one process associated with the mailbox, so
       ** abort the message operation.
       */
       status = SS$_ABORT;

       goto error;
    }

    /* Set up IOXB */

    ioxb->ioxb$l_astadr	    = astadr;
    ioxb->ioxb$l_astprm	    = astprm;
    ioxb->ioxb$w_chan	    = chan;
    ioxb->ioxb$l_tmo	    = tmo;

    /* Set up timeout if requested */

    if( tmo > 0 )
    {
	long 	bintime[2];

	/* Convert ms timeout to VMS bintime. */

	GCtim_mstotim( tmo, bintime );

        reqidt = ioxb;

	/*
	** Set ioxb timer.
	** GCtm_ef is tested and cleared in GCsync.
	** Timer may be cancelled by GCqio_complete.
	*/

        status = sys$setimr(
            (mode == GC_SYNC_CLIENT_MODE) ? EFN$C_ENF : GCtm_ef, /* efn */
            bintime,                                             /* daytim */
            (mode == GC_SYNC_CLIENT_MODE) ? GCqio_timeout :      /* astadr */
                (mode == GC_SERVER_MODE && ! sync) ? GCqio_timeout :
                     NULL,
            ioxb, 0);                                            /* reqidt */

	GCACL_TIMER_TRACE("setimr>GCqio>", (i4)ioxb, status);

	if (VMS_FAIL(status))
	    goto error;
    }

    /* Start QIO */

    GCTRACE(2)("thread %x. GCqio_exec> Starting qio with sync %d\n",
        PCtidx(), sync);

    status = sys$qio(
    	  (mode == GC_SYNC_CLIENT_MODE) ? 
              EFN$C_ENF : GCio_ef,              /* efn */ 
          chan,				    	/* chan */
    	  func,				    	/* func */
    	  ioxb,				    	/* ioxb */
          sync ? NULL : (mode == GC_ASYNC_CLIENT_MODE) ? 
              NULL : GCqio_complete,            /* astadr */
          sync ? NULL : (mode == GC_ASYNC_CLIENT_MODE) ?
              NULL : ioxb,                      /* astprm */
    	  va,				    	/* p1 = VA */
    	  len,			    	      	/* p2 = size */
    	  0, 0, 0, reqidt); 			/* p3..p6 not used */

    if( VMS_FAIL( status ) )
	goto error;

    GCTRACE(2)("thread %x. GCqio_exec> Qio queued with status %d\n",
        PCtidx(), status);

#ifdef OS_THREADS_USED

#ifdef ALPHA
    if (GC_os_threaded)
#endif
    if (GCzomb_tmrset && setimr_astpnd && GCzomb_time && (mode == GC_SERVER_MODE))
    {
        GENERIC_64 curr_time;
        GENERIC_64 ela_time;
        i4         ela_msecs;
        i4         remainder;
        i4         ast_leeway;

        if (setimr_time.gen64$q_quadword == 0) sys$gettim(&setimr_time);
        sys$gettim(&curr_time);
        status = lib$subx ((uint *)&curr_time, (uint *)&setimr_time, (uint *)&ela_time);
        if (!(status & STS$M_SUCCESS)) lib$signal(status);
        status = lib$ediv(&10000, (int64 *)&ela_time, &ela_msecs, &remainder);
        if (!(status & STS$M_SUCCESS)) lib$signal(status);
        ast_leeway = GCzomb_time + 2000; /* allow 2 secs leeway for ast deliver */
        if (ela_msecs > ast_leeway)
        {
            GCzomb_tmrset = 0;
            __UNLOCK_LONG(&setimr_astpnd);
            setimr_cnt = 0;
        }
    }
#endif

    /*
    ** If the user wants zombie checking and the timer's not set to do it,
    ** do the zombie check now.
    */
    if ( mode == GC_SYNC_CLIENT_MODE )
    {
        if( GCzomb_time && !asipc->GCzomb_tmrset )
            GCqio_zombie();
    }
    else 
    {
        if( GCzomb_time && !GCzomb_tmrset )  
            GCqio_zombie();
    }

    /* 
    ** For sychronous I/O or asynchronous clients, stash ioxb for GCsync 
    ** to find. 
    */

    if (sync || (mode == GC_ASYNC_CLIENT_MODE))
    {
        if( !( gc_ioxb_queue = 
             (GC_IOXB_QUEUE *)(*GCalloc)( sizeof(GC_IOXB_QUEUE))) )
        {
   	    GCX_TRACER_0_MACRO( "GCqio_exec>fail>GCalloc fail", NULL );
	    status = SS$_INSFMEM;
            goto error;
        }

        gc_ioxb_queue->GCsync_ioxb = ioxb;

        QUinsert(&gc_ioxb_queue->q, &gc_thread->GCsync_ioxb_qhead);

        gc_thread->counter++;
    }

    return;

error:
    /* On error, set failure status and drive completion. */

    GCTRACE(1)( "thread %x. GCqio_exec ioxb=%x fail VMS status %x\n", 
        PCtidx(), ioxb, status );
    GCX_TRACER_1_MACRO( "GCqio_exec>", ioxb, "fail VMS status %x", status );

    ioxb->ioxb$w_status = status;

    (*ioxb->ioxb$l_astadr)( ioxb->ioxb$l_astprm );
}

/*{
** Name: GCqio_shut - sys$qio to shutdown listen port.
**
** Description:
**      Posts a IO$M_SHUDOWN sys$qio: operation on channel chan.
**
**      Uses an IOXB (extended I/O status block) for storing control
**      information.
**
** Inputs:
**      evf     event flag
**      chan    channel mailbox is assigned to
**      ioxb    status control block
**
** Side effects:
**      sys$qio called to post I/O.
**
** History:
**      20-Apr-2004 (ashco01) Problem: INGNET99 Bug: 107833
**          Introduced to enable full listen port shutdown and discard of 
**          queued I/O when shutting down IIGCC to prevent time_wait2
**          state connections holding listen port open.
*/

static VOID
GCqio_shut( evf, chan, ioxb)
i4     evf;
i4     chan;   /* GCC Listen Cannel */
IOXB    *ioxb;
{
    VMS_STATUS  status;

    GCTRACE(1)( "GCqio_shut evf=%x chan=%x ioxb=%x\n", evf, chan, ioxb );

    /* Set up IOXB */
    ioxb->ioxb$w_chan       = chan;

    /* Shutdown the port and discard all queued IO */
    status = sys$qiow(
          evf,                                  /* event */
          chan,                                 /* chan */
          IO$_DEACCESS|IO$M_SHUTDOWN,           /* shutdown port */
          ioxb,                                 /* ioxb */
          0,                                    /* astadr */
          0,                                    /* astprm */
          0,                                    /* p1 = VA */
          0,                                    /* p2 = size */
          0,                                    /* p3 */
          UCX$C_DSC_ALL,                        /* p4 IO function */
          0, 0);                                /* p5..p6 not used */

    if( VMS_FAIL( status ) )
    {
          GCTRACE(1)( "GCqio_shut ioxb=%x shutdown fail VMS status %x\n", 
                      ioxb, status);
          GCX_TRACER_1_MACRO( "GCqio_shut>", 
                      ioxb, "shutdown fail VMS status %x", status );
    }

    /* Shutdown with outstanding IO will generate VMS_STATUS warnings
    ** so fake status and return cleanly. */

    status = 0;
    ioxb->ioxb$w_status = status;

    return;
}


/*{
** Name: GCqio_timeout - cancel I/O and set timeout status
**
** Description:
**	On timeout, an I/O operation calls GCqio_timeout to cancel the
**	I/O (if not already complete) and set ioxb$w_can_status so
**	GCqio_status can determine the reason for failure.
**
** Inputs:
**	ioxb		extended IO status block for QIO
**
** Side effects:
**	I/O may be cancelled.
**
** History:
**	27-Dec-90 (seiwald)
**	    Documented.
*/

static VOID
GCqio_timeout( ioxb )
IOXB	*ioxb;
{
    GCTRACE(2)( "thread %x. GCqio_timeout ioxb=%x %s cancel QIO\n", PCtidx(),
		ioxb, ioxb->ioxb$w_status ? "won't" : "will" );
    GCX_TRACER_1_MACRO( "GCqio_timeout", (i4)ioxb, "%s cancel QIO",
		(i4)(ioxb->ioxb$w_status ? "won't" : "will" ));

    /* Set the cancel status for GCqio_complete to find. */

    ioxb->ioxb$w_can_status = SS$_TIMEOUT;

    /* If I/O not complete, force completion by cancelling. */

    if( ioxb->ioxb$w_status == 0 )
	sys$cancel( ioxb->ioxb$w_chan );
}


/*{
** Name: GCqio_complete - cancel timer and drive completion for QIO
**
** Description:
**	All QIO's, sucess or failure, drive GCqio_complete to cancel
**	the timer (if present) and drive the higher level completion
**	routine.
**
** Inputs:
**	ioxb		extended IO status block for QIO
**
** Side effects:
**	Timer may be cancelled.
**	Drives completion routine.
**
** History:
**	27-Dec-90 (seiwald)
**	    Documented.
**	11-May-92 (alan) bug #44006
**	    Swallow GCterminate-driven listen mailbox completions.
*/


static VOID
GCqio_complete( ioxb )
IOXB	*ioxb;
{
    GCTRACE(2)( "thread %x. GCqio_complete ioxb=%x w_status %x w_can_status %x\n",
		PCtidx(), ioxb, ioxb->ioxb$w_status, ioxb->ioxb$w_can_status );
    GCX_TRACER_2_MACRO( "GCqio_complete>", ioxb, "w_status %x w_can_status %x",
		ioxb->ioxb$w_status, ioxb->ioxb$w_can_status );

    /*
    ** Cancel ioxb timer if set but not triggered.
    ** Cancelled timers don't drive their AST's.
    */

#if defined(ALPHA) || !defined(OS_THREADS_USED)
#ifdef OS_THREADS_USED
    if (GC_os_threaded)
#endif
    if( ioxb->ioxb$l_tmo > 0 && ioxb->ioxb$w_can_status != SS$_TIMEOUT )
	sys$cantim( ioxb, 0 );
#endif

    /*
    ** Call the user's I/O completion routine
    ** (unless a listen mailbox read driven by GCterminate).
    */

    if( ioxb->ioxb$w_can_status != SS$_SHUT )
        (*ioxb->ioxb$l_astadr)( ioxb->ioxb$l_astprm );
}


/*{
** Name: GCqio_status - translate QIO status into svc_parms->status
**
** 	This routine must be called after a GCqio_exec completes, so
**	that it can clear ioxb$w_status for GCqio_pending.
**
** Inputs:
**	ioxb->w_status		status of QIO
**	ioxb->w_can_status	reason for cancelled QIO
**	def_err			error number to use for generic failures
**
** Outputs:
**	svc_parms->status	GC reason for failure
**	svc_parms->sys_err->error	VMS reason for failure
**
** Side effects:
**	Clears ioxb->w_status.
**
** History:
**	27-Dec-90 (seiwald)
**	    Rewritten and documented.
**      29-Sep-1998 (horda03)
**          After a I/O operation timeout, allow future I/O
**          operations. Don't reset the w_can_status! Bug 90634.
**      01-May-2009 (horda03) Bug 122016
**          Clear timeout if IO request succeeded.
*/


static VOID
GCqio_status( ioxb, def_err, svc_parms )
IOXB		*ioxb;
STATUS		def_err;
SVC_PARMS       *svc_parms;
{
    /*
    ** On failure, determine source of failure.
    ** If the I/O was cancelled, look to w_can_status for reason.
    */

    if( VMS_FAIL( ioxb->ioxb$w_status ) )
    {
	if( ioxb->ioxb$w_status == SS$_ENDOFFILE )
	{
	    /* EOF (chop mark on read): just set length to 0. */
	    ioxb->ioxb$w_length = 0;
	}
	else if( ioxb->ioxb$w_status != SS$_CANCEL
	      && ioxb->ioxb$w_status != SS$_ABORT )
	{
	    /* Unknown failure - just mark status */
	    svc_parms->status = def_err;
	    svc_parms->sys_err->error = ioxb->ioxb$w_status;
	}
	else if( ioxb->ioxb$w_can_status == SS$_TIMEOUT )
	{
	    /* Cancelled by GCqio_timeout: indicate timeout status. */
	    svc_parms->status = GC_TIME_OUT;
	    svc_parms->sys_err->error = ioxb->ioxb$w_can_status;

            /* Allow future I/O operation to this channel */
            ioxb->ioxb$w_can_status = 0;
	}
	else if( ioxb->ioxb$w_can_status == SS$_NOMBX )
	{
	    /* Cancelled by GCqio_zombie: indicate release. */
	    svc_parms->status = GC_ASSOC_FAIL;
	    svc_parms->sys_err->error = 0;
	}
	else
	{
	    /* Cancelled (probably) by GCdisconn. */
	    svc_parms->status = GC_ASSN_RLSED;
	    svc_parms->sys_err->error = ioxb->ioxb$w_status;
	}
    }
    else if ( ioxb->ioxb$w_can_status == SS$_TIMEOUT )
    {
       /* The QIO request timed out and the AST for the timer has
       ** tried to cancel the QIO. However, before it called sys$cancel()
       ** (see GCqio_timeout(), the QIO request completed..e. the AST
       ** for the QIO was queued to the process. As the request completed i
       ** before the Timeout completed the cancel, let the request through.
       ** But need to cancel the TIMEOUT status otherwise the next
       ** QIO request will fail. In the GCN this will cause the
       ** server to crash (Bug 122016)
       */
       ioxb->ioxb$w_can_status = 0;
    }

    /* Reset to idle.  GCqio_pending counts non-idle I/O's */

    ioxb->ioxb$w_status = GC_CH_WS_IDLE;

    GCTRACE(2)( "thread %x. GCqio_status ioxb=%x status %x VMS status %x\n",
	PCtidx(), ioxb, svc_parms->status, svc_parms->sys_err->error );
    GCX_TRACER_2_MACRO( "GCqio_status>", ioxb, "status %x VMS status %x",
	svc_parms->status, svc_parms->sys_err->error );
}


/*{
** Name: GCqio_pending - count undelivered AST's on a connection
**
** Description:
**	Counts undelivered AST's of I/O's on a connection.
**
**	Must be called with AST's disabled or at AST time.
**
** Inputs:
**	asipc		connection were interested in
**
** Returns:
**	count of undelivered AST's
**
** History:
**	27-Dec-90 (seiwald)
**	    Rewritten and documented.
*/

static i4
GCqio_pending(asipc)
ASSOC_IPC *asipc;
{
    i4 io_count = 0;
    i4 i;

    /*
    ** for OS_THREADS, the completion AST (generally GCqio_complete)
    ** has not yet been delivered - so the ioxb$w_status has not yet
    ** been set to GC_CH_WS_IDLE - if a read was outstanding and the 
    ** channel has been deassigned - it should be SS$_ABORT
    ** So treat this as an IDLE
    */

    for( i = 0; i < CHANS; i++ )
        if (
            ( asipc->chan[i].ioxb.ioxb$w_status != GC_CH_WS_IDLE )
#ifdef OS_THREADS_USED
        &&  (asipc->chan[i].chan != GC_CH_ID_NONE)
#endif
           )
           io_count++;

    GCTRACE(2)( "thread %x. GCqio_pending asipc=%x pending %d\n", PCtidx(),
        asipc, io_count );
    GCX_TRACER_1_MACRO( "GCqio_pending>", asipc, "pending %d", io_count );

    return io_count;
}



/*{
** Name: GCqio_zombie - check for missing partner on all connections
**
** Description:
**	Checks the reference count on the mailboxes for each connection
**	with pending I/O's, and cancels I/O's on connections without partners.
**	This causes send/receive failures and leads the user to abort
**	the connection.
**
**	If I/O's were pending on any association, this routine will set
**	a timer to reactivate itself in a few minutes.  Otherwise, it
**	will wait for GCqio_exec to reinvoke it.
**
** Side effects:
**	I/O's cancelled.
**	Timer set.
**
** History:
**	27-Dec-90 (seiwald)
**	    Rewritten and documented.
**      26-mar-09 (Ralph Loen) Bug 121772
**          Cancel I/O only on a thread matching the calling thread.
**          Ingres is currently linked with upcalls disabled, which
**          means GCqio_zombie(), when executed from a timer AST, may
**          exexcute in a different thread than the thread from which it 
**          was originally queued.
**      21-Oct-2009 (Ralph Loen) Bug 122769
**          Cross-integration of bug 121772 corrupted the reqidt argument 
**          in sys$setimr() for GC_SYNC_CLIENT_MODE: the address of 
**          GCzomb_time was provided instead of asipc->GCzomb_tmrset, 
**          which is what sys$cancel() expects later when it cancels 
**          the timer during normal completion.
*/

static VOID
GCqio_zombie()
{
    ASSOC_IPC	*asipc;
    QUEUE	*asipc_qhdr;
    long 	old_ast;
    i4		pending_io = 0;
    i4 	i;
    i2          mode;
    bool        set_timer = FALSE;
    i4		count;
    long bintime[2];
    i4   ss_sts;

    GCgetMode( &mode );
#ifdef OS_THREADS_USED

#ifdef ALPHA
    if (GC_os_threaded)
#endif
       __UNLOCK_LONG(&setimr_astpnd);
#endif

    if ( mode == GC_SYNC_CLIENT_MODE )
    {
        for (asipc_qhdr  =  GCactive_qhead.q_next;
       	    asipc_qhdr != &GCactive_qhead;
    	    asipc_qhdr  =  asipc_qhdr->q_next )
        {
            asipc = (ASSOC_IPC *)asipc_qhdr;
            if ( asipc->thread_id == PCtidx() )
            {
                GCTRACE(2)( "thread %x. GCqio_zombie started by %s\n", PCtidx(),
	            asipc->GCzomb_tmrset ? "timer" : "qio" );
                GCX_TRACER_1_MACRO( "GCqio_zombie", 0, "started by %s",
   	            (i4)(asipc->GCzomb_tmrset ? "timer" : "qio" ));
                break;
            }
        }
    }
    else
    {
        GCTRACE(2)( "thread %x. GCqio_zombie started by %s\n", PCtidx(),
    	    GCzomb_tmrset ? "timer" : "qio" );
        GCX_TRACER_1_MACRO( "GCqio_zombie", 0, "started by %s",
	    (i4)(GCzomb_tmrset ? "timer" : "qio" ));
    }
    
    /* Possibly called from non-AST code - prevent interruptions. */

    old_ast = sys$setast((char)FALSE);

    /*
    ** Scan list of active associations.  This list is only manipulated
    ** at AST time.
    */

    for (asipc_qhdr  =  GCactive_qhead.q_next;
    	 asipc_qhdr != &GCactive_qhead;
    	 asipc_qhdr  =  asipc_qhdr->q_next )
    {
	IOXB		*ioxbs[ CHANS ];

	asipc = (ASSOC_IPC *)asipc_qhdr;

        if ( asipc->thread_id != PCtidx() )
            continue;

        /*
        ** If the partner has not yet established a connection,
        ** don't check, as this would yield a false positive.
        */
        if ( !asipc->zombie_check )
            continue;

	/* Sanity check: don't check a connection in GCdisconn.  */
	/* Connections that failed to start won't be on the queue. */

	if( asipc->chan[RN].chan == GC_CH_ID_NONE )
	    continue;

	/* Note pending I/O's. */

	for( i = count = 0; i < CHANS; i++ )
	    if( asipc->chan[ i ].ioxb.ioxb$w_status == 0 )
		ioxbs[ count++ ] = &asipc->chan[ i ].ioxb;

	/* Don't bother checking an idle association. */

	if( !count )
	    continue;

	/*
	** Is partner still there?  Guess by checking reference count
	** on the normal receive mailbox.  It should be at least 2:
	** this process and the partner.  This logic fails when the
	** association is passed to a child process with GCsave.
	*/

	if( GCmbx_detect( asipc->chan[RN].chan ) < 2 )
	{
	    /* Partner gone - cancel pending I/O's. */

	    GCTRACE(1)( "thread %x. GCqio_zombie asipc=%x dead\n", PCtidx(),
                asipc );
	    GCX_TRACER_1_MACRO( "GCqio_zombie>", asipc, "dead, pending=%d", 
			count );

	    /* Call sys$cancel to drive QIO completion on any I/O's. */

	    for( i = 0; i < count; i++ )
	    {
		ioxbs[ i ]->ioxb$w_can_status = SS$_NOMBX;
		sys$cancel( ioxbs[ i ]->ioxb$w_chan );
	    }
	}
	else
	{
	    /* Partner there - tally pending I/O's. */

            GCTRACE(1)( "thread %x. GCqio_zombie pending chan %x refcnt >= 2, RN=%x RX=%x SN=%x SX=%x CN=%x\n",
                       PCtidx(), asipc->chan[RN].chan, 
                       asipc->chan[RN].chan, asipc->chan[RX].chan,
                       asipc->chan[SN].chan, asipc->chan[SX].chan,
                       asipc->chan[CN].chan);

	    pending_io += count;
	}
        if ( mode == GC_SYNC_CLIENT_MODE )
            break;
    }

    /*
    ** Set zombie checking to occur by timer if there are pending I/O's
    ** on any association, or if this current check was not done by timer.
    ** Otherwise, the next call to GCqio_exec will restart it.
    */

    GCTRACE(2)( "thread %x. GCqio_zombie done I/O's pending = %d\n", 
        PCtidx(), pending_io );
    GCX_TRACER_1_MACRO( "GCqio_zombie>", 0, "I/O's pending = %d", pending_io );

    /* convert timer */
    GCtim_mstotim( GCzomb_time, bintime );

    /* 
    ** Set the zombie timer if interval not zero. 
    */
    if ( mode == GC_SYNC_CLIENT_MODE ) 
    {
        if ( pending_io || !asipc->GCzomb_tmrset )
        {
	    GCX_TRACER_0_MACRO ("GCqio_zombie>", NULL);


      	    ss_sts = sys$setimr(
			EFN$C_ENF, 	        /* efn */
			bintime,		/* daytime (relative) */
 		        GCqio_zombie,		 /* astadr */
			&asipc->GCzomb_tmrset,  /* reqidt */
                        setimr_last_param);	/* flags */

	    /* Note that zombie checking is active (on a timer). */

	    asipc->GCzomb_tmrset = TRUE;
        }
        else
        {
	    asipc->GCzomb_tmrset = FALSE;
        }
    }
    else
    {
        if ( pending_io || !GCzomb_tmrset )
        {
	    GCX_TRACER_0_MACRO ("GCqio_zombie>", NULL);

    	    ss_sts = sys$setimr(
			EFN$C_ENF, 	        /* efn */
			bintime,		/* daytime (relative) */
 		        GCqio_zombie,		 /* astadr */
			&GCzomb_time,           /* reqidt */
                        setimr_last_param);	/* flags */
#ifdef OS_THREADS_USED
#ifdef ALPHA
         if (GC_os_threaded)
#endif
            if (ss_sts & STS$M_SUCCESS)
            {
               sys$gettim(&setimr_time);
               __LOCK_LONG(&setimr_astpnd);
               setimr_cnt++;
            }
#endif /* OS_THREADS_USED */

	    /* Note that zombie checking is active (on a timer). */

            GCzomb_tmrset = TRUE;
        }
        else
        {
	    GCzomb_tmrset = FALSE;
        }
    }

    /* Enable ASTs again */

    if(old_ast == SS$_WASSET)
	    sys$setast((char)TRUE);
}


/*{
** Name: GCmbx_attach - attach to a client's 4 mailboxes
**
** Description:
**	Attaches to the 4 mailboxes created by a client with GCmbx_client.
**	A server does this to establish a connection with a client.  A
**	client does this after a GCrestore.
**
** Inputs:
**	requestor			0 - client, 1 - server
**	rq_assoc_buf->mbx_name[]	names of mailboxes
**
** Outputs:
**	asipc->chan[].name		names of mailboxes
**	asipc->chan[].chan		channel numbers of mailboxes
**	asipc->chan[].maxiosize		size advise for mailboxes
**
** Returns:
**	VMS status	SS$_NORMAL or some failure
**
** History:
**	27-Dec-90 (seiwald)
**	    Rewritten and documented.
*/

static VMS_STATUS
GCmbx_assoc( requestor, rq_assoc_buf, asipc )
i4		    requestor;
GC_RQ_ASSOC 	   *rq_assoc_buf;
ASSOC_IPC	   *asipc;
{
    VMS_STATUS		status;
    i4			i;

    /*
    ** Attach to IPC mailboxes for this association.
    ** There is a 1-to-1 mapping between mailbox names rq_assoc_buf->mbx_name[]
    ** and channels asipc->chan[] for clients.  For listeners, sends ands
    ** receives are swapped.
    */

    for( i = 0; i < CHANS; i++ )
    {
	i4 who = requestor ? i : ( i ^ 1 ); /* swap for listener */

	STcopy( rq_assoc_buf->mbx_name[ who ], asipc->chan[ i ].name );

	if( VMS_FAIL( status = GCmbx_assign( &asipc->chan[ i ] ) ) )
	    return status;

	GCTRACE(2)( "thread %x. GCmbx_assoc: asipc=%x chan %d: %s\n",
	    PCtidx(), asipc, i, asipc->chan[ i ].name );
	GCX_TRACER_2_MACRO( "GCmbx_assoc>", asipc, "chan %d: %s",
	    i, (i4)(asipc->chan[ i ].name ));
    }

    return SS$_NORMAL;
}


/*{
** Name: GCmbx_assign - attach to an existing mailbox
**
** Inputs:
**	a_chan->name	name of mailbox to attach to
**
** Outputs:
**	a_chan->chan	channel number of attached mailbox
**
** Returns:
**	VMS status	SS$_NORMAL or some failure
**
** History:
**	27-Dec-90 (seiwald)
**	    Rewritten and documented.
*/

static VMS_STATUS
GCmbx_assign( a_chan )
ASSOC_CHAN *a_chan;
{
    struct dsc$descriptor_s     name_desc;
    VMS_STATUS			status;

    name_desc.dsc$w_length = STlength(a_chan->name);
    name_desc.dsc$a_pointer = a_chan->name;
    name_desc.dsc$b_dtype = DSC$K_DTYPE_T;
    name_desc.dsc$b_class = DSC$K_CLASS_S;

    status = sys$assign(&name_desc, &a_chan->chan, 0, 0);
    return status;
}


/*{
** Name: GCmbx_server - create listen mailbox for a server
**
** Description:
**	Creates a mailbox on which the serve can listen for incoming
**	connection requests.  Creates a logical named a_chan->name
**	whose equivalence is the mailbox name.
**
** Inputs:
**	a_chan->maxiosize	maxmsg size of mailboxes
**	a_chan->name		logical for name of the mailbox
**
** Outputs:
**	a_chan->chan		channel number of mailbox
**
** Returns:
**	VMS status	SS$_NORMAL or some failure
**
** History:
**	27-Dec-90 (seiwald)
**	    Written.
*/

static VMS_STATUS
GCmbx_servers( a_chan )
ASSOC_CHAN *a_chan;
{
    struct      dsc$descriptor_s  name_desc;
    VMS_STATUS 	status;

    /* Associate a logical name with this Server mailbox */
    name_desc.dsc$a_pointer = a_chan->name;
    name_desc.dsc$w_length = STlength(a_chan->name);
    name_desc.dsc$b_dtype = DSC$K_DTYPE_T;
    name_desc.dsc$b_class = DSC$K_CLASS_S;

    /* Servers get GCsvr_mbx_max bufquota/ */

    status = sys$crembx(
	    1,              	/* Mailbox is "permanent" */
	    &a_chan->chan,	/* chan */
	    a_chan->maxiosize,	/* max msg size */
	    GCsvr_mbx_max,      /* Buf quota */
	    0,                  /* prot mask = all priv */
	    PSL$C_USER,		/* acmode */
	    &name_desc );	/* lognam */

    return status;
}


/*{
** Name: GCmbx_client - create mailboxes for a client's connection
**
** Description:
**	Creates 4 mailboxes, normal and expedited send and receive,
**	placing their names and channel numbers in the chan's of the
**	asipc, and copying the names to the mbx_name's in the rq_assoc_buf.
**
** Inputs:
**	asipc->chan[].maxiosize		maxmsg size of mailboxes
**
** Outputs:
**	asipc->chan[].chan		channel numbers of mailboxes
**	asipc->chan[].name		names of mailboxes
**	rq_assoc_buf.mbx_name[]		names of mailboxes
**
** Returns:
**	VMS status	SS$_NORMAL or some failure
**
** History:
**	27-Dec-90 (seiwald)
**	    Written.
*/

static VMS_STATUS
GCmbx_client( asipc, rq_assoc_buf )
ASSOC_IPC	   *asipc;
GC_RQ_ASSOC 	   *rq_assoc_buf;
{
    VMS_STATUS	status;
    i4		i;
    IOXB        ioxb;

    /*
    ** Create mailboxes for the client.
    ** There is a 1-to-1 mapping between mailbox names rq_assoc_buf->mbx_name[]
    ** and channels asipc->chan[] for clients.
    */

    for( i = 0; i < CHANS; i++ )
    {
	ASSOC_CHAN	*a_chan = &asipc->chan[ i ];
	u_i2  		mbx_namelen = 0;
	ILE3	 dvi_list[] =
	     { /* We want the name and it's length */
		{16, DVI$_DEVNAM, (PTR)&a_chan->name, &mbx_namelen},
		{0, 0, 0, 0} };
	IOSB		iosb;

	/* Create mailbox.  Clients get 2*mbx_size bufquota. */

	status = sys$crembx(
	    0,			/* prmflg */
	    &a_chan->chan,	/* chan	  */
	    a_chan->maxiosize,	/* maxmsg */
	    2*a_chan->maxiosize,/* bufquo */
	    0xff00,		/* promsg */
	    0,			/* acmode */
	    0);			/* lognam */

	if (VMS_FAIL(status))
	   return status;

	/* Get name of mailbox. */

	status = sys$getdviw(
	    EFN$C_ENF,	        /* efn */
	    a_chan->chan,	/* chan */
	    0,			/* devnam */
	    &dvi_list,		/* itmlst */
	    &iosb,		/* iosb */
	    0,			/* astadr */
	    0,			/* astprm */
	    0);			/* nullarg */
	if (status & 1)
	    status = iosb.iosb$w_status;
	if (VMS_FAIL(status))
	    return status;

	a_chan->name[ mbx_namelen ] = 0;  /* null terminate */

	/* Put mailbox name into assoc buf.  */

	STcopy( a_chan->name, rq_assoc_buf->mbx_name[ i ] );
    }

    /* Propagate max I/O size. */

    rq_assoc_buf->max_mbx_msg_size = asipc->chan[ RN ].maxiosize;

    return SS$_NORMAL;
}


/*{
** Name: GCmbx_detect - count processes attached to mailbox
**
** Description:
**	Counts the numbers of processes attached to the mailbox assigned
**	on the I/O channel channel.   If < 2, this mailbox has no partner.
**
** Inputs:
**	channel		Channel number of assigned mailbox.
**
** Returns:
**	-1		something went wrong
**	>=0		number of attached processes
**
** History:
**	27-Dec-90 (seiwald)
**	    Renamed and documented.
*/

static i4
GCmbx_detect( channel )
i4		channel;
{
    i4		ref_count = 0;
    ILE3		dvi_list[] = {
		    {4, DVI$_REFCNT, (PTR)&ref_count, 0},
		    {0, 0, 0, 0} };
    IOSB iosb;
    int status;

    status = sys$getdviw(EFN$C_ENF, channel, 0, dvi_list, &iosb, 0, 0, 0);
    if (status & 1)
	status = iosb.iosb$w_status;
    if (VMS_FAIL(status))
	return -1;

    return ref_count;
}


/*{
** Name: GCmbx_userterm - get username and terminal of peer process
**
** Inputs:
**	sender_pid	process were interested in
**	term_name_size	max len for term_name
**	invoker_size	make len for invoker
**
** Outputs:
**	term_name	terminal name of peer
**	invoker		user name of peer
**
** Returns:
**	VMS status	SS$_NORMAL or some failure
**
** History:
**	27-Dec-90 (seiwald)
**	    Renamed and documented.
**	23-feb-95 (wolf, from brogr02)
**	    Null-terminate username, zapping any trailing blanks.
**	    Needed to make the PM routines find requests correctly.
**	    Also quit tromping on caller's invoker+invoker_size+1'th byte.
**	03-April-96 (strpa01)
**	    In GCmbx_userterm(),
**	    Correct Null-termination of username. GETJPI always returns 12 as
**	    length of username. Previously invoker[username_length-1]=0 
**	    was truncating last character of a 12 char username. (71018)
**     01-August-97 (horda03)
**          In GCmbx_userterm(),
**          The function was attempting to determine the terminal type of a
**          Mailbox for connections over the network. This resulted in a ""
**          (blank) terminal type being stored. Previously, the terminal type
**          of a connection over the network (determined by dbmsinfo('terminal'))
**          was "unknown". Only determine the terminal type for NON DETACHED
**          processes. (83698)
**    06-November-00 (loera01) Bug 102701
**          Added wait/retry loop for SYS$GETJPI if a suspended partner 
**          is encountered.
**    07-Nov-2000 (horda03) Bug 103137
**          Get the Username at the same time that we get the MASTER PID of the
**          connected session.
**	05-oct-2005 (abbjo03)
**	    Don't depend on VMS job type to determine whether a terminal name
**	    is present or not.  SSH connections appears as detached jobs but
**	    have a terminal.
*/

static VMS_STATUS
GCmbx_userterm( sender_pid, term_name_size, term_name, invoker_size, invoker )
int	sender_pid;
i4	term_name_size;
char	*term_name;
i4	invoker_size;
char	*invoker;
{
    VMS_STATUS		    status;

    u_i2 		    ret_vterm_size;
    u_i2 		    ret_term_size;
    u_i2 		    ret_inv_size;

    char		    vterm_name[16];
    i4			    vterm_size = sizeof(vterm_name);

    int			    parent_id;
    struct dsc$descriptor_s term_desc;
    i4                      i;
    IOSB		iosb;
    ILE3		jpiget[] = {
	{ sizeof(parent_id),JPI$_MASTER_PID,    (PTR)&parent_id, 0},
	{ invoker_size-1, JPI$_USERNAME,	invoker,    &ret_inv_size},
	{ 0, 0, 0, 0}	};

    ILE3	 	dviget2[] = {
	{ term_name_size-1, DVI$_TT_PHYDEVNAM,  term_name,  &ret_term_size },
	{ 0, 0, 0, 0} };

    ILE3		jpiget2[] = {
	{ vterm_size-1,   JPI$_TERMINAL,	vterm_name, &ret_vterm_size},
	{ 0, 0, 0, 0} };

    GCX_TRACER_0_MACRO ("userterm>", NULL);

    /*
    ** Get parent process id
    */

    for (i = 0; i < TRY_COUNT; i++)
    {
        status = sys$getjpiw(EFN$C_ENF, &sender_pid, 0, jpiget, &iosb, 0, 0);
	if (status & 1)
	    status = iosb.iosb$w_status;
        if (VMS_SUSPENDED(status))
        {
	   PCsleep(1000);
	   continue;
        }
        else if (VMS_FAIL(status))
        {
	    return status;
	}
	break;
    }

    /*
    ** Get the physical device name of the terminal that the parent
    ** process owns. Make sure it's in lower case.
    */

    for (i = 0; i < TRY_COUNT; i++)
    {
        status = sys$getjpiw(EFN$C_ENF, &parent_id, 0, jpiget2, &iosb, 0, 0);
	if (status & 1)
	    status = iosb.iosb$w_status;
        if (VMS_SUSPENDED(status))
        {
	   PCsleep(1000);
	   continue;
        }
        else if (VMS_FAIL(status))
        {
	    return status;
	}
	break;
    }

    /*
    ** ret_inv_size always 12 for username. Null terminate invoker. (71018)
    */

    invoker[ret_inv_size] = 0;
    STtrmwhite(invoker);  /* remove trailing blanks */
    CVlower(invoker);

    /*
    ** if he really has a terminal, then find out the username of the
    ** person who owns it and the virtual terminal name.
    */

    if (ret_vterm_size && !CMspace(vterm_name))
    {
	term_desc.dsc$w_length = ret_vterm_size;
	term_desc.dsc$a_pointer = vterm_name;

	status = sys$getdviw(EFN$C_ENF, 0, &term_desc, dviget2,	&iosb, 0, 0, 0);
	if (status & 1)
	    status = iosb.iosb$w_status;
	if (VMS_FAIL(status))
	    return status;

	/*
	** strip off the trailing colon, if there is one.
	*/
	if (term_name[ret_term_size - 1] == ':')
	    ret_term_size--;

	/*
	** null terminate it so that we can lower case it.
	*/
	term_name[ret_term_size] = 0;
	CVlower(term_name);

	/*
	** if the name begins with an underscore, then remove it.
	*/
	if (term_name[0] == '_')	/* Shift it over */
	    MEcopy( term_name + 1, ret_term_size, term_name );
    }
    else
    {
	MEcopy("unknown", sizeof("unknown"), term_name);
    }

    return SS$_NORMAL;
}



/*
** Name: GCtrace_status
**
** Inputs:
**	ident -- character string put on the from of the stat output.
**	OpenVMS status
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
**	02-May-94 (eml)
**	    Created
*/
static VOID
GCtrace_status( char *ident,
		STATUS status )
{
    STATUS stat;
    short msglen = 0;
    char buf[256];
    struct dsc$descriptor_s bufadr;

    msglen = 0;
    bufadr.dsc$w_length = sizeof( buf ) - 1;
    bufadr.dsc$b_dtype = DSC$K_DTYPE_T;
    bufadr.dsc$b_class = DSC$K_CLASS_S;
    bufadr.dsc$a_pointer = buf;

    stat = SYS$GETMSG( status, &msglen, &bufadr, 15, 0 );
    if ( !(stat & 1) )
	return ;

    buf[msglen] = '\0';
    GCTRACE(2)( "thread %x. %s: %s\n", PCtidx(), ident, buf );

    return;
}


/*
** Name: GCsignal_startup - notify iirundbms startup process, startup complete
**
** Description:
**      This routine sends a hacked-up (non-standard) OpenVMS termination
**	mailbox message to the iirundbms startup process, notifying it of
**	successful server startup. The hacked-up termination mailbox structure
**	is identified by a zero status and the process name of the server.
**
**	If the termination mailbox does not exist, assume this is an
**      interactive process and ignore the status.
**
** Inputs:
**      cs_name	-- pointer to the process name.
**
** Outputs:
**      OpenVMS Status
**
** Side Effects:
**	IIRUNDBMS startup process may take to action based on the mailbox
**	message sent.
**
*/
VMS_STATUS
GCsignal_startup( const char *const cs_name )
{
    IOSB	iosb;
    VMS_STATUS	status;
    int		unit_num = 0;
    short	term_chan;
    char	term_name[16], buf[16];

    struct dsc$descriptor_s term_dsc;

    ILE3		itmlst[] = {
			    { sizeof(unit_num), JPI$_TMBU, (PTR)&unit_num, 0},
			    { 0, 0, 0, 0}
			};

    /*
    ** Signal startup program - RUNDBMS - that we have started up
    ** and pass the server name back.
    */

    status = sys$getjpiw( EFN$C_ENF, NULL, NULL, itmlst, &iosb, NULL, NULL );
    if ( status & 1 ) status = iosb.iosb$w_status;
    if ( !(status & 1) )
	return ( status );

	/*
	** If unit number is zero, assume this is an interactive process
	*/
    if ( unit_num == 0 )
	return ( SS$_NORMAL );

    CVla( unit_num, buf );
    (*((long *)term_name)) = 'MBA\0';
    STcopy(buf, &term_name[3]);
    term_dsc.dsc$w_length = STlength(term_name) + 1;
    term_name[term_dsc.dsc$w_length - 1] = ':';
    term_dsc.dsc$a_pointer = term_name;
    term_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
    term_dsc.dsc$b_class = DSC$K_CLASS_S;

    status = sys$assign( &term_dsc, &term_chan, NULL, NULL );
    if ( status & 1 )
    {
	ACCDEF2	accmsg;

	((ACCDEF *)&accmsg)->acc$l_finalsts = 0;
	STcopy( (PTR)cs_name, (PTR)&accmsg.acc$t_user_data );
	status = sys$qiow(EFN$C_ENF, term_chan, IO$_WRITEVBLK | IO$M_NOW, &iosb,
			   NULL, NULL, &accmsg, sizeof(accmsg), 0, 0, 0, 0 );

	(VOID)sys$dassgn( term_chan );

	if ( status & 1 ) status = iosb.iosb$w_status;
    }

    if ( status == SS$_NOSUCHDEV )
	return ( SS$_NORMAL );
    else
	return ( status );
}

/*
** Name: GCusername
**
** Description:
**      Set the passed in argument 'name' to contain the name of the user
**      who is executing this process.  Name is a pointer to an array
**      large enough to hold the user name.  The user name is returned
**      NULL-terminated in lowercase with any trailing whitespace deleted.
**
**      On systems where relevant, this should return the effective rather
**      than the real user ID.
**
** Input:
**      size            Length of buffer for user name.
**
** Output:
**      name            The effective user name.
**
** Returns:
**      VOID
**
** History:
**      16-feb-01 (loera01)  Created.
**          The VMS implementation is simply a wrapper for IDname().
*/
VOID
GCusername( char *name, i4  size )
{
    IDname(&name);
}

/*
** Name: GCgetMode
**
** Description:
**      Fetch the gcacl run-time mode.
**
** Input:
**      None.
**
** Output:
**      mode           GCACL mode: GC_SYNC_CLIENT_MODE, GC_ASYNC_CLIENT_MODE
**                     or GC_SERVER_MODE.
**
** Returns:
**      VOID
**
** History:
**      12-June-2007 (Ralph Loen) SIR 118032
**          Created.
*/

VOID GCgetMode( i2 *mode )
{
    *mode = GCmode;
}

/*
** Name: GCsetMode
**
** Description:
**      Set the gcacl run-time mode.
**
** Input:
**      mode           GCACL mode: GC_SYNC_CLIENT_MODE, GC_ASYNC_CLIENT_MODE,
**                     or GC_SERVER_MODE.
**
** Output:
**      None.
**
** Returns:
**      VOID
**
** History:
**      12-June-2007 (Ralph Loen) SIR 118032
**          Created.
*/

VOID GCsetMode( i2 mode )
{
    GCmode = mode;
}

/*
** Name: GCcheckPeer
**
** Description:
**      Check the expedited and normal channels for receive and send.
**      If any of these has a reference count of at least two, the
**      connection partner must be present.
**
** Input:
**      asipc       local association control block.
**
** Output:
**      None.
**
** Returns:
**      TRUE if partner is present, FALSE otherwise.
**
** History:
**      12-June-2007 (Ralph Loen) SIR 118032
**          Created.
*/
static bool GCcheckPeer( ASSOC_IPC *asipc )
{
   bool partner_present = FALSE;

   while ( TRUE )
   {
       if ( GCmbx_detect(asipc->chan[RN]) >= 2 )
       {
           partner_present = TRUE;
           break;
       }
       else if ( GCmbx_detect(asipc->chan[RX]) >= 2 )
       {
           partner_present = TRUE;
           break;
       }
       else if ( GCmbx_detect(asipc->chan[SN]) >= 2 )
       {
           partner_present = TRUE;
           break;
       }
       else if ( GCmbx_detect(asipc->chan[SX]) >= 2 )
       {
           partner_present = TRUE;
           break;
       }
       else 
           break;
   }

   return partner_present;
}
