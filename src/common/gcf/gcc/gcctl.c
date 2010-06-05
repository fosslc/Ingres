/*
** Copyright (c) 1987, 2010 Ingres Corporation All Rights Reserved.
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>

#include    <cv.h>
#include    <cm.h>
#include    <si.h>
#include    <gc.h>
#include    <gcccl.h>
#include    <me.h>
#include    <mh.h>
#include    <nm.h>		 
#include    <er.h>
#include    <lo.h>
#include    <pm.h>		 
#include    <qu.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <sl.h>
#include    <sp.h>
#include    <mo.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcs.h>
#include    <gcc.h>
#include    <gccpci.h>
#include    <gccer.h>
#include    <gccgref.h>
#include    <gcctl.h>
#include    <gcm.h>
#include    <gcatrace.h>
#include    <gcxdebug.h>

/*
**
**  Name: GCCTL.C - GCF communication server Transport Layer module
**
**  Description:
**      GCC TL.C contains all routines which constitute the Transport Layer 
**      of the GCF communication server. 
**
**	gcc_tl			Main entry point for the Transport Layer.
**	gcc_tl_event		FSM processor.
**	gcc_tlinit		TL initialization routine.
**	gcc_tlterm		TL termination routine.
**	gcc_tl_exit		Exit routine for protocol request completions.
**	gcc_tl_evinp		FSM input event evaluator.
**	gcc_tlout_exec		FSM output event execution processor.
**	gcc_tlactn_exec		FSM action execution processor.
**	gcc_tl_send		Perform GCC_SEND operation.
**	gcc_tpdu_size		Negotiate TPDU size.
**	gcc_tl_abort		TL internal abort routine.
**	gcc_gl_shut		Shut down connection.
**	gcc_tl_release_ccb	Release CCB resources.
**
**  History:
**      29-Feb-88 (jbowers)
**          Initial module creation
**      07-Dec-88 (jbowers)
**          Corrected FSM error logout in gcc_tlfsm(); added correct upward
**	    propogation of network errors in gcc_prot_exit() and
**	    gcc_tlout_exec().
**      22-Dec-88 (jbowers)
**	    Fixed bug wherein TL issues a LISTEN request even when a network
**	    OPEN has failed.
**      03-Jan-89 (jbowers)
**          Added better error handling for the case where evaluation of the
**	    input event fails (i.e., gcc_tl_evinp returns a failure).
**      10-Jan-89 (jbowers)
**          Fixed bugs involving the coordination of the release of CCB's and
**	    MDE's.  In the CCB, the ccb_hdr.async_cnt represents a use count to
**	    allow coordinated release of the CCB when all users are finished
**	    with it.  In the MDE, a flag MDE_SAVE is set to prevent another
**	    layer from deleting it.
**      16-Jan-89 (jbowers)
**          Documentation change to FSM to document fix for an invalid
**	    intersection. 
**      15-Feb-89 (jbowers)
**          Add local flow control: expand fsm to send and receive XON and XOFF
**	    primitives for bidirectional control of MDE flow.
**      13-Mar-89 (jbowers)
**          Fix for Bug 4985, failure to clean up a session when remote comm
**	    server crashes.
**      16-Mar-89 (jbowers)
**          Fix in gcc_tlout_exec to allow an MDE to be immediately deleted by
**	    an upper layer if required.
**      23-Mar-89 (jbowers)
**          Fix in gcc_prot_exit to allow for a specified number of retries of a
**	    failed GCC_LISTEN.
**      31-Mar-89 (jbowers)
**          Miscellaneous code cleanup: typos, additional error condition
**	    handling, etc.  Changes in all routines.
**      18-APR-89 (jbowers)
**          Added handling of (16, 5) intersection to FSM (documentation only:
**	    actual code fix is in GCCTL.H).
**	26-May-89 (jbowers/heaherm/jorge)
**	    Added the comments for the previous uodates to support the
**	    GCA 2.2 API error message Spec.
**	21-jun-1989 (pasker)
**	    Added traceing to the FSM and added a check to the FSM which
**	    will determine if the TL state has changed while calling action
**	    routines or output events.
**	21-jun-1989 (pasker)
**	    change GCC_TLFSM() so that it keeps a private copy of the
**	    CCB associated with the incoming MDE.  Then, all further reference
**	    to the CCB will be direct, instead of going through the MDE.  This
**	    avoids the bug whereby the MDE is deleted as a result of an output
**	    event and then the CCB is not availible to the action routines. We
**	    now pass the CCB to the action routines as a parameter so it can
**	    reference it directly without relying on the MDE.
**	22-jun-89 (seiwald)
**	    Straightened out gcc[apst]l.c tracing.
**	03-jul-89 (seiwald)
**	    Tracing code to dump network level messages.
**	25-jul-89 (ham)
**	    Fixed bug for the TL_PROTOCOL CLASS.  This was never found
**	    before because we only set it we don't check it. 
**      06-Sep-89 (cmorris)
**          Spruced up tracing.
**	21-sep-89 (ham)
**	    Byte alignment.
**      04-Oct-89 (cmorris)
**          Added missing argument to GCshut call.
**	07-Dec-89 (seiwald)
**	     GCC_MDE reorganised: mde_hdr no longer a substructure;
**	     some unused pieces removed; pci_offset, l_user_data,
**	     l_ad_buffer, and other variables replaced with p1, p2, 
**	     papp, and pend: pointers to the beginning and end of PDU 
**	     data, beginning of application data in the PDU, and end
**	     of PDU buffer.
**	09-Dec-89 (seiwald)
**	     Allow for variously sized MDE's: specify size when allocating
**	     one; negotiate tpdu_size properly using the new gcc_tpdu_size
**	     function.  Remove unworkable T_CONTINUE code.
**      13-Dec-89 (cmorris)
**           Fix memory leak. (Documentation only; fix is in fsm in
**           gcctl.h).
**      08-Jan-89 (cmorris)
**           Tidied up GCA real-time tracing.
**	09-Jan-90 (seiwald)
**	     Remove MEfill of system_status.  It is costly and should always
**	     be set (when necessary) by CL code.
**	12-Jan-90 (seiwald)
**	     Ironed out ownership of MDE's: an MDE now belongs to exactly 
**	     one layer at a time.  See change comments in gccal.c.
**      23-Jan-90 (cmorris)
**           More tracing stuff.
**      06-Feb-90 (cmorris)
**           Ceased to use NL_PARMS.
**      08-Feb-90 (cmorris)
**	     Fix FSM holes surrounding network connection indications.
**	08-Feb-90 (seiwald)
**	     Log successful (as well as unsuccessful) network opens.
**	08-Feb-90 (cmorris)
**	     Do a disconnect after failed connect.
**	08-Feb-90 (seiwald)
**	     When logging successful network opens, log the actual listen
**	     port returned by the protocol driver.
**	11-Feb-90 (seiwald)
**	     Turned prot_indx, an index into the protocol driver table,
**	     into prot_pce, a pointer to the table entry.
**	12-Feb-90 (cmorris)
**	     Restructured gcc_prot_exit to switch on requested function
**	     as opposed to service primitive stored in mde.
**	14-Feb-90 (cmorris)
**	     Added N_ABORT event to handle protocol exit errors correctly.
**	22-Feb-90 (seiwald)
**	    GCcexec and GCcshut removed.  IIGCC now shares GCexec and 
**	    GCshut with IIGCN.
**	06-Mar-90 (cmorris)
**	    Reworked internal aborts.
**	10-Apr-90 (seiwald)
**	    Hand pce to protocol driver, so it can access its own config
**	    info.
**	30-Apr-90 (cmorris)
**	    Removed comments on output events OTNxxx; they don't exist!
**	20-Jun-90 (seiwald)
**	    Disallow outbound connections on protocols where the network
**	    listen had failed.
**	8-Aug-90 (seiwald)
**	    Normalized handling of inbound connection counts: they are
**	    incremented upon network listen completion and decremented upon
**	    network disconn completion (both in gcc_prot_exit).  
**  	17-Dec-90 (cmorris)
**  	    Fixed up handling of release of underlying network connections
**  	    when we were the receiver of the TL DC TPDU. (Documentation
**  	    only; fixes in FSM in gcctl.h)
**  	17-Dec-90 (cmorris)
**  	    Now that STCLG is not being abused, rename it to STWDC:- waiting
**  	    disconnect confirm, and clean up a handful of crappy state
**  	    transitions.
**  	10-Jan-90 (cmorris)
**  	    Use the right MDE when aborting.
**	27-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	10-Apr-91 (seiwald)
**	    Introduced support for II_GCCxx_PROTOCOLS, which lists which
**	    specific protocols to start, rather than starting all.
**	    Tightened up the handling of PCT_OPN_FAIL, so that failing
**	    protocol drivers are properly accounted for.
**	16-Apr-91 (seiwald)
**	    Added a warning message E_GC2818_PROT_ID in case II_GCC_PROTOCOLS
**	    lists an unknown protocol.   This message logs the known valid
**	    protocols as well.
**	16-Apr-91 (seiwald)
**	    New E_GC2819_NTWK_CONNECTION_FAIL to be returned to user
**	    for connection initiation failure rather than the generic
**	    E_GC280D_NTWK_ERROR.
**	2-May-91 (seiwald)
**	    Replace erroneous references to pci_l, which was unset, with
**	    the constants to which pci_l should have been set.  The li
**	    field in many TL PCI's was bogus.  Curiously, the field is
**	    never examined.
**      28-May-91 (cmorris)
**          Purge send queue on network disconnections and internal aborts.
**	31-May-91 (cmorris)
**	    Got rid of remaining vestiges of attempts to multiplex.
**	31-May-91 (cmorris)
**	    Got rid of remaining vestiges of attempts to implement peer
**	    Transport layer flow control.
**      09-Jul-91 (cmorris)
**	    Split STCLD state into STCLD, STWCR abd STCLG. Fixes a number
**          of disconnect problems.
**	29-Jul-91 (cmorris)
**	    In state STCLG, we still have to handle and chuck away a number
**	    of send/receive related input events until such time as all
**	    protocol drivers are updated.
**	30-Jul-91 (cmorris)
**	    Got rid of CL_SYS_ERR in mde (assigned to but never used).
**	31-Jul-91 (cmorris)
**	    Check status of calls to gcc_add_ccb.
**	14-Aug-91 (seiwald)
**	    GCaddn2() takes a i4, so use 0 instead of NULL.  NULL is
**	    declared as a pointer on OS/2.
**	14-Aug-91 (seiwald)
**	    Redeclared pci as a char *, not PTR, so pointer arithmetic
**	    can properly be used.
**  	15-Aug-91 (cmorris)
**  	    Don't send XOFF/XON up after a disconnect indication/request.
**	17-aug-91 (leighb) DeskTop Porting Change:
**	    Added include of nm.h;
**	    Corrected func decl and def of gcc_tpdu_size();
**	    Don't log network startup messages for MS-DOS front ends;
**	    Don't search the Protocol PCT for MS-DOS - it has only one
**	    entry - load that entry.
**  	06-Sep-91 (cmorris)
**  	    Don't free CCB on abort when we're already in STCLD; the CCB has
**  	    already been freed to get to this state.
**	18-oct-91 (seiwald) bug #40468
**	    Don't return failure after reintroducing an event unless
**	    an abort is generated.  This allows important subsequent 
**	    actions (like repostings listens and receives) to take place
**	    when certain actions fail.
**  	17-Dec-91 (cmorris)
**  	    Renamed gcc_prot_exit to gcc_tl_exit as both the TL and
**  	    PB have protocol exits.
**	19-feb-92 (leighb) DeskTop Porting Change:
**	    Added type VOID to gcc_tdump().
**	03-mar-92 (leighb) DeskTop Porting Change:
**	    Added type static to gcc_tdump().
**      02-apr-92 (fraser)
**          Desktop cleanup.  Removed ifdef PMFE from gcc_tlactn_exec.
**	28-Sep-92 (brucek)
**	    ANSI C fixes.
**  	28-Dec-92 (cmorris)
**  	    If we don't check result of gcc_tl call, cast result to VOID.
**  	28-Dec-92 (cmorris)
**  	    Move logging of network disconnection out of input
**  	    evaluator and into the protocol exit.
**  	28-Dec-92 (cmorris)
**  	    Tidied up logging of synchronous open failures.
**    	04-Jan-93 (cmorris)
**  	    Don't keep doing multiple uppercasings of protocol ids
**  	    on every connect:- uppercase entries in the protocol
**  	    table when the protocol is opened, and uppercase the
**  	    protocol id specified on the connect outside the loop
**  	    through the protocol table!!
**  	08-Jan-93 (cmorris)
**  	    Removed unused input events.
**	21-Jan-93 (edg)
**	    II_GCCxx_PROTOCOLS replaced by PM parmater(s).
**	    ii.hostname.gcc.instance.protocol-id.status	{on/off}
**  	05-Feb-93 (cmorris)
**  	    Removed expedited data support - not used.
**	10-Feb-93 (seiwald)
**	    Removed generic_status and system_status from gcc_xl(),
**	    gcc_xlfsm(), and gcc_xlxxx_exec() calls.
**	11-Feb-93 (seiwald)
**	    Merge the gcc_xlfsm()'s into the gcc_xl()'s.
**	11-Feb-93 (seiwald)
**	    Use QU routines rather than the roll-your-own gca_qxxx ones.
**	16-Feb-93 (seiwald)
**	    GCA service parameters have been moved from hanging off the
**	    ccb to overlaying the protocol driver's p_plist in the MDE.
**	17-Feb-93 (seiwald)
**	    When validating the partner's claimed dst_ref, just check it
**	    against the lcl_cei, rather than trying to look up the CCB
**	    with it.
**  	11-Mar-93 (cmorris)
**  	    Correct 11_Feb change:- arguments to QUinsert call were reversed!!
**	22-Mar-93 (seiwald)
**	    Fix to change of 11-feb-93: call QUinsert with a pointer to the
**	    last element on the queue, so that the MDE is inserted at the end.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-Sep-93 (brucek)
**	    Removed include of dbdbms.h.
**  	28-Sep-93 (cmorris)
**  	    Added support to pass 'flush data' flag to protocol drivers
**  	    on GCC_SEND requests.
**  	27-Oct-93 (cmorris)
**  	    Don't hand in mde to error logging if error is not related
**  	    to a particular connection.
**	19-jan-94 (edg)
**	    Put include of si.h before gcccl.h.  This change is for NT.
**	    Becuase the Microsoft header, io.h (eventually included by
**	    si.h), redefines open to _open, we need to make sure the
**	    redfinition occurs globally as both gcccl.h and this module
**	    refer to a structure member called open.
**	17-Mar-94 (seiwald)
**	    Gcc_tdump() relocated and renamed to gcx_tdump().
**      13-Apr-1994 (daveb) 62240
**          Keep some stats when SEND and RECEIVE are sucessful.
**	25-Oct-95 (sweeney)
**	    Reset failure count on valid inbound connection.
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	21-Nov-95 (gordy)
**	    For embedded Comm Server: don't post network LISTEN, don't
**	    log successful OPEN.
**	24-par-96 (chech02, lawst01)
**	    cast %d arguments to i4  in TRdisplay for windows 3.1 port.
**	19-Jun-96 (gordy)
**	    Recursive requests are now queued and processed by the first
**	    instance of gcc_tl().  This ensures that all outputs of an 
**	    input event are executed prior to processing subsequent requests.
**	27-May-97 (thoda04)
**	    Included pm.h, er.h headers for function prototypes.
**	13-jun-97 (hayke02)
**	    Added include of lo.h (which includes locl.h), so that pm.h
**	    will have the definition of LOCATION.
**	30-Jun-97 (rajus01)
**	    Made major changes to add encryption/decryption interface
**	    in TL. See individual routines for the changes. Added a new
**	    routine gcc_e_tsnd() to encrypt the data before we send.
**	    Added new action ATSDP.
**	20-Aug-97 (gordy)
**	    Combined gcc_tsnd(), gcc_e_tsnd() into single routine, 
**	    gcc_tl_send().  Added gcc_tl_shut() to facilitate shutting
**	    down a connection due to encryption initialization failure.
**	    Moved CCB cleanup to own routine, gcc_tl_release_cb().
**	    Added OTTDR (superset of OTTDI) to process DR TPDU prior 
**	    to sending disconnect to SL.  Moved GCS init/term to GCC
**	    general processing.  Reworked MDE PCI and user data 
**	    processing to be more robust and permit future extensions 
**	    with minimal impact on backward compatibility.  Added 
**	    support for encryption modes.
**	10-Oct-97 (gordy)
**	    Increased TL protocol level for SL password encryption.
**      22-Oct-97 (rajus01)
**          Replaced gcc_get_obj(), gcc_del_obj() with
**          gcc_add_mde(), gcc_del_mde() calls respectively.
**	    Encode, Decode routines are provided with extra
**	    buffer space to handle encryption and decryption.
**	    In GCC_RECEIVE action, extra buffer space is
**	    provided to receive encrypted data.
**	31-Mar-98 (gordy)
**	    Added default encryption mechanism.
**	 8-Sep-98 (gordy)
**	    Save installation registry protocols.  Added gcc_tl_registry()
**	    to initialize installation registry.
**	 2-Oct-98 (gordy)
**	    Make protocol info visible through MO/GCM for successful opens.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	09-jun-1999 (somsa01)
**	    In gcc_tl_exit(), on successful startup, write
**	    E_GC2A10_LOAD_CONFIG to the errlog.
**	15-jun-1999 (somsa01)
**	    In gcc_tl_exit(), moved printout of E_GC2A10_LOAD_CONFIG to
**	    iigcc.c .
**	05-may-2000 (somsa01 & rajus01)
**	    In gcc_tlinit(), changed the logic around such that we now have
**	    two loops. The first loop will gather information about the
**	    protocols from config.dat . The second loop will attempt to
**	    initialize each protocol. In this way, when winding back from a
**	    possible error from starting a protocol, we do not hit a
**	    situation where we would be analyzing an uninitialized pct_pce
**	    for a particular protocol.
**	28-jun-2000 (somsa01)
**	    In gcc_tlinit(), the previous change did not take into account
**	    that a particular protocol description may not even exist in
**	    config.dat . This case is now handled.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	28-jul-2000 (somsa01)
**	    We now write out the successful port ids to standard output.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-sep-2000 (somsa01)
**	    In gcc_tlinit(), make sure we do a PMinit() and PMload() before
**	    calling PMget().
**	24-may-2001 (somsa01)
**	    In gcc_tl_exit(), since node_id will always be a string, use
**	    STcopy() to transfer the data from one node_id to another.
**      25-Jun-01 (wansh01) 
**          replace gcxlevel with IIGCc_global->gcc_trace_level
**	 6-Jun-03 (gordy)
**	    Protocol information now saved in PIB.  Remote target address
**	    info moved to trg_addr.  Attach CCB as MIB object after LISTEN.
**	    Load actual local/remote address after LISTEN/CONNECT.
**	16-Jan-04 (gordy)
**	    MDE's may not be upto 32K.
**	21-Jan-04 (gordy)
**	    Registry PCT is now copy of original PCT.  Added registry flags.
**	19-feb-2004 (somsa01)
**	    Added stdout_closed static, set to TRUE when we are done writing
**	    out to standard output. Do not run SIstd_write() when this is set.
**	    Otherwise, on Windows, SIstd_write() will run FlushFileBuffers()
**	    which will hang because no one will be reading data from standard
**	    output.
**	16-Jun-04 (gordy)
**	    Added session mask to CR and CC.
**	 5-Aug-04 (gordy)
**	    First active protocol is now considered the default.
**	31-Mar-06 (gordy)
**	    Sanity check lengths read from IO buffers.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**	22-Apr-10 (rajus01) SIR 123621
**	    After a successful network open request update PIB with 
**	    the actual_port_id. 
*/



static char	*Yes = "Y";
static bool	stdout_closed = FALSE;

/*
**  Forward and/or External function references.
*/

static VOID	gcc_tl_exit( PTR ptr );
static STATUS	gcc_tl_event( GCC_MDE *mde );
static u_i4	gcc_tl_evinp( GCC_MDE *mde, STATUS *generic_status );
static STATUS	gcc_tlout_exec( i4  output_id, GCC_MDE *mde, GCC_CCB *ccb );
static STATUS	gcc_tlactn_exec(u_i4 action_code, GCC_MDE *mde, GCC_CCB *ccb);
static STATUS	gcc_tl_send( bool encrypt, GCC_MDE *mde );
static VOID	gcc_tpdu_size( GCC_CCB *ccb, u_char max_tpdu_size );
static VOID	gcc_tl_shut( GCC_MDE *, GCC_CCB *, 
			     STATUS *, CL_ERR_DESC *, GCC_ERLIST * );
static VOID	gcc_tl_release_ccb( GCC_CCB *ccb );


/*
** Name: gcc_tl	- Entry point for the Transport Layer
**
** Description:
**	gcc_tl is the entry point for the Transport Layer of GCC.  It invokes
**	the services of subsidiary routines to perform its requisite functions.
**	It is invoked with a Message Data Element (MDE) as its input parameter.
**	The MDE may come from SL and is outbound for the network, or from NL
**	and is inbound for a local client or server, sent from an external
**	node.  For an architectural overview of the GCF communication server,
**	see the header of IIGCC.C.
**
**	The TL interfaces directly to the protocol handler modules of the CL.
**	These are invoked by generic service requests to perform environment
**	and protocol-specific functions.  This interface is described in detail
**	in GCCCL.C.  The protocol handlers are TL's window on the world, the 
**	connection to the local communication services.
** 
**      There are three phases of TL operation: 
** 
**      - Connection initiation; 
**      - Data transfer;
**      - Connection termination. 
**
**	Each of these is discussed in detail below.  Then the operation of the
**	TL is defined by a Finite State Machine (FSM) specification.  This is
**	the definitive description of TL behavior.  A discussion of this
**	representation is given in detail in GCCAL.C.
**
**	States, input events, output events and actions are symbolized as
**	STxxx, ITxxx, OTxxx and ATxxx respectively.
**
**	Output events and actions are marked with a '*' if they pass 
**	the incoming MDE to another layer, or if they delete the MDE.
**	There should be exactly one such output event or action per
**	state cell.

**
**      Connection initiation is indicated in one of two ways:
**
**	by recipt of a T_CONNECT request sent by SL;
**
**	or by completion of a LISTEN function invoked through a protocol
**      handler.
**
**      In the first case, we are the Initiating Transport Layer (ITL),
**	driven by a client.  A CONNECT service request is invoked from the
**	appropriate protocol handler to establish contact with the remote 
**	communication server.  If the CONNECT request completes without error,
**	a SEND request is  issued to send a CR (Connection Request) TPDU to
**	the peer TL.  A RECEIVE is then invoked to receive either a CC
**	(Connection Confirm) or a DR (Disconnect Request) from the peer.
**	If the connection request succeeds, a T_CONNECT confirm is generated
**	and sent to SL.  If not, a T_DISCONNECT indication is sent.  In the
**	former case, the Data Transfer phase is entered. In the latter case, 
**	the connection is abandoned.
** 
**      In the second case, we are the Responding Transport Layer (RTL),
**	in a remote server node.  A RECEIVE request is invoked from the protocol
**	handler to receive a CR TPDU from the peer (ITL).  When it is received,
**	a T_CONNECT indication is constructed and sent to SL.  A T_CONNECT
**      response is awaited before proceeding.  When received, a CC (Connection
**	Confirm) TPDU is sent to the peer TL by a SEND protocol request, and
**      the data transfer phase is entered.  If a T_DISCONNECT request is
**	received instead, a DR TPDU is sent to the peer TL and LISTEN is invoked
**	to re-initiate the Connection Initiation phase.
** 
**
**      The Data Transfer phase is initiated by both ITL and RTL by invoking 
**      the RECEIVE service on the newly established connection.  Each 
**      received message is packaged into a T_DATA indication and sent to SL.
**      Upon receipt of a T_DATA request, a SEND is invoked to pass the
**      message to the recipient.  During this phase, there is no distinction 
**      between ITL and RTL, and hence between the client end and the server 
**      end of the connection.  
** 
**
**      The Connection Termination phase is entered as a result of several 
**	possible events.   In the DT phase, a T_DISCONNECT request from
**	SL causes TL to send a DR (Disconnect Request) TPDU, and await a
**	DC (Disconnect Confirm) TPDU.  The receipt of a DR TPDU from the peer
**	TL causes a T_DISCONNECT indication to be passed to SL and a DC TPDU
**	to be sent to the peer TL.  A locally detected failure of the network
**	connection by a protocol handler results in a T_DISCONNECT indication
**	being sent to SL.
**

** 
**	Following is the list of states, events and actions defined
**	for the Transport Layer, with the state characterizing an individual
**	connection.
**	
**	There are some input events which are not actually defined in the
**	OSI machine, such as N_DATA confirm.  This is an internally-generated
**	event whose function is to drive some internal action.  This is simply a
**	convenient way of driving actions which are specific to this
**	implementation, such as discarding MDE's and CCB's.
**
**	States:
**
**	    STCLD   - Idle (closed), transport connection does not exist.
**	    STWNC   - ITL: Waiting for network connection (N_CONNECT confirm)
**	    STWCC   - ITL: Waiting for CC (Connection Confirm TPDU)
**	    STWCR   - RTL: Waiting for CR (Connect Request TPDU)
**	    STWRS   - RTL: Waiting for T_CONNECT RSP
**	    STOPN   - Open (Data Transfer)
**	    STWDC   - Waiting for DC (Disconnect Confirm TPDU)
**	    STSDP   - Open, completion of previous send pending
**	    STOPX   - Open (Data Transfer), XOFF received, no rcv outstanding
**	    STSDX   - Open, completion of previous sendpending, XOFF received,
**		      no rcv outstanding
**  	    STWND   - Waiting for N_DISCONNECT indication
**	    STCLG   - Closing, waiting for N_DISCONNECT confirm
**
**	Input Events:
**
**	    ITOPN   -  TL initial startup
**	    ITNCI   -  N_CONNECT indication (Local LISTEN has completed)
**	    ITNCC   -  N_CONNECT confirm (Local CONNECT has completed)
**	    ITTCQ   -  T_CONNECT request (From SL: we are ITL)
**	    ITCR    -  CR (Connection Request) TPDU
**	    ITCC    -  CC (Connection Confirm) TPDU
**	    ITTCS   -  T_CONNECT response (From SL: we are RTL)
**	    ITTDR   -  T_DISCONNECT request (From SL: terminate connection)
**	    ITNDI   -  N_DISCONNECT indication (Connection has failed)
**	    ITNDC   -  N_DISCONNECT confirm (DISCONNECT is complete)
**	    ITTDA   -  T_DATA request (From SL: sendnormal data)
**	    ITDR    -  DR (Disconnect Request) TPDU (in N_DATA ind)
**	    ITDC    -  DC (Disconnect Confirm) TPDU (in N_DATA ind)
**	    ITDT    -  DT (Data) TPDU (in N_DATA ind)
**	    ITNC    -  N_DATA confirm (SEND is complete) & Q is empty
**	    ITNCQ   -  N_DATA confirm (SEND is complete) & Q is not empty
**	    ITNOC   -  N_OPEN confirm (network OPEN is complete)
**	    ITABT   -  T_P_ABORT: TL internal execution error
**	    ITNRS   -  N_RESET indication: network OPEN failed.
**	    ITXOF   -  T_XOFF request (From SL: shut down data flow)
**	    ITXON   -  T_XON request (From SL: resume data flow)
**	    ITDTX   -  DT (Data) TPDU && "XOFF rcvd" is set
**	    ITTDAQ  -  T_DATA request && sendqueue is full && NOT XOF sent
**	    ITNCX   -  N_DATA confirm && Q is empty && XOF sent
**	    ITNLSF  -  N_LSNCLUP request (network listen failure)
**
**	Output Events:
**
**	    OTTCI*  -  T_CONNECT indication (Process tpdu; To SL: we are RTL)
**	    OTTCC*  -  T_CONNECT confirm (Process tpdu; To SL: we are ITL)
**	    OTTDR   -  T_DISCONNECT indication (Process tpdu; To SL: terminated)
**	    OTTDI   -  T_DISCONNECT indication (To SL: connection terminated)
**	    OTTDA*  -  T_DATA indication (Process tpdu; To SL: data received)
**	    OTCR    -  Construct CR (Connection Request) TPDU
**	    OTCC    -  Construct CC (Connection Confirm) TPDU
**	    OTDR    -  Construct DR (Disconnect Request) TPDU
**	    OTDC    -  Construct DC (Disconnect Confirm) TPDU
**	    OTDT    -  Construct DT (Data) TPDU
**	    OTXOF   -  Send T_XOFF indication to SL: shut down data flow
**	    OTXON   -  Send T_XON indication to SL: resume data flow
**
**	Actions:
**
**	    ATCNC*  -  Invoke CONNECT function from protocol handler
**	    ATDCN*  -  Invoke DISCONNECT function from protocol handler
**	    ATMDE*  -  Discard the input MDE
**	    ATCCB   -  Discard the CCB associated with the input MDE
**	    ATLSN   -  Invoke LISTEN function from the protocol handler
**	    ATRVN   -  Invoke RECEIVE for normal flow data
**	    ATSDN*  -  Invoke SEND for normal flow data
**	    ATSDP*  -  Invoke SEND for encrypted data
**	    ATOPN*  -  Invoke OPEN function from the protocol handler
**	    ATSNQ*  -  Enq an MDE to the SEND queue (a sendis pending)
**	    ATSDQ   -  Dequeue an MDE and send it
**	    ATSXF   -  Set the "XOFF-received" indicator in the CCB
**	    ATRXF   -  Reset the "XOFF-received" indicator in the CCB
**	    ATPGQ   -  Purge the sendqueue
**
**	In the FSM definition matrix which follows, the parenthesised integers
**	are the designators for the valid entries in the matrix, andare used in
**	the representation of the FSM in GCCTL.H as the entries in TLFSM_TLB
**	which are the indices into the entry array TLFSM_ENTRY.  See GCCTL.H
**	for further details.

**	State ||STCLD|STWNC|STWCC|STWCR|STWRS|STOPN|STWDC|STSDP|STOPX|STSDX|STWND|STCLG|
**	      ||  00 |  01 |  02 |  03 |  04 |  05 |  06 |  07 |  08 |  09 |  10 |  11 |
**	================================================================================
**	Event ||
**	================================================================================
**	ITOPN ||STCLD|     |     |     |     |     |     |     |     |     |     |     |
**	(00)  ||     |     |     |     |     |     |     |     |     |     |     |     |
**	      ||ATOPN*     |     |     |     |     |     |     |     |     |     |     |
**	      || (1) |     |     |     |     |     |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITNCI ||STWCR|     |     |     |     |     |     |     |     |     |     |     |
**	(01)  ||     |     |     |     |     |     |     |     |     |     |     |     |
**	      ||ATRVN|     |     |     |     |     |     |     |     |     |     |     |
**	      ||ATLSN|     |     |     |     |     |     |     |     |     |     |     |
**            ||ATMDE*     |     |     |     |     |     |     |     |     |     |     |
**	      || (2) |     |     |     |     |     |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITNCC ||     |STWCC|     |     |     |     |     |     |     |     |     |     |
**	(02)  ||     |OTCR |     |     |     |     |     |     |     |     |     |     |
**	      ||     |ATSDP*     |     |     |     |     |     |     |     |     |     |
**	      ||     | (3) |     |     |     |     |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITTCQ ||STWNC|     |     |     |     |     |     |     |     |     |     |     |
**	(03)  ||ATCNC*     |     |     |     |     |     |     |     |     |     |     |
**	      || (4) |     |     |     |     |     |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITCR  ||     |     |     |STWRS|     |     |     |     |     |     |     |     |
**	(04)  ||     |     |     |OTTCI*     |     |     |     |     |     |     |     |
**	      ||     |     |     | (7) |     |     |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITCC  ||     |     |STOPN|     |     |     |STWDC|     |     |     |     |     |
**	(05)  ||     |     |OTTCC*     |     |     |ATRVN|     |     |     |     |     |
**	      ||     |     |ATRVN|     |     |     |ATMDE*     |     |     |     |     |
**	      ||     |     | (8) |     |     |     | (63)|     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITTCS ||     |     |     |     |STSDP|     |     |     |     |     |     |     |
**	(06)  ||     |     |     |     |OTCC |     |     |     |     |     |     |     |
**	      ||     |     |     |     |ATRVN|     |     |     |     |     |     |     |
**	      ||     |     |     |     |ATSDP*     |     |     |     |     |     |     |
**	      ||     |     |     |     | (9) |     |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITTDR ||     |STCLG|STWDC|     |STWDC|STWDC|     |STWDC|STWDC|STWDC|     |     |
**	(07)  ||     |     |OTDR |     |OTDR |OTDR |     |OTDR |OTDR |OTDR |     |     |
**	      ||     |ATDCN*ATSDN*     |ATSDN*ATSDN*     |ATSNQ*ATSDN*ATSNQ*     |     |
**	      ||     |     |     |     |     |     |     |     |ATRVN|ATRVN|     |     |
**	      ||     | (10)| (13)|     | (13)| (13)|     | (11)| (5) | (14)|     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITNDI ||     |STCLG|STCLG|STCLG|STCLG|STCLG|STCLG|STCLG|STCLG|STCLG|STCLG|STCLG|
**	(08)  ||     |OTTDI|OTTDI|     |OTTDI|OTTDI|     |OTTDI|OTTDI|OTTDI|     |     |
**	      ||     |ATDCN*ATDCN*ATDCN*ATDCN*ATDCN*ATDCN*ATPGQ|ATDCN*ATPGQ|ATDCN*ATMDE*
**            ||     |     |     |     |     |     |     |ATDCN*     |ATDCN*     |     |
**	      ||     | (17)| (17)| (10)| (17)| (17)| (10)| (62)| (17)| (62)| (10)| (36)|
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+

**	State ||STCLD|STWNC|STWCC|STWCR|STWRS|STOPN|STWDC|STSDP|STOPX|STSDX|STWND|STCLG|
**	      ||  00 |  01 |  02 |  03 |  04 |  05 |  06 |  07 |  08 |  09 |  10 |  11 |
**	================================================================================
**	Event ||
**	================================================================================
**	ITNDC ||     |     |     |     |     |     |     |     |     |     |     |STCLD|
**	(09)  ||     |     |     |     |     |     |     |     |     |     |     |     |
**	      ||     |     |     |     |     |     |     |     |     |     |     |ATCCB|
**	      ||     |     |     |     |     |     |     |     |     |     |     |ATMDE*
**	      ||     |     |     |     |     |     |     |     |     |     |     | (19)|
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITTDA ||     |     |     |     |     |STSDP|     |STSDP|STSDX|STSDX|     |     |
**	(10)  ||     |     |     |     |     |OTDT |     |OTDT |OTDT |OTDT |     |     |
**	      ||     |     |     |     |     |ATSDN*     |ATSNQ*ATSDN*ATSNQ*     |     |
**	      ||     |     |     |     |     | (20)|     | (24)| (40)| (41)|     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITDR  ||     |     |STCLG|     |STWND|STWND|STCLG|STWND|     |     |     |STCLG|
**	(11)  ||     |     |OTTDR|     |OTTDR|OTTDR|     |OTTDR|     |     |     |     |
**	      ||     |     |OTDC |     |OTDC |OTDC |     |OTDC |     |     |     |     |
**	      ||     |     |ATSDN*     |ATSDN*ATSDN*ATDCN*ATPGQ|     |     |     |ATMDE*
**	      ||     |     |     |     |     |     |     |ATSNQ*     |     |     |     |
**	      ||     |     | (23)|     | (23)| (23)| (10)| (44)|     |     |     | (36)|
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITDC  ||     |     |     |     |     |     |STCLG|     |     |     |     |STCLG|
**	(12)  ||     |     |     |     |     |     |     |     |     |     |     |     |
**	      ||     |     |     |     |     |     |ATDCN*     |     |     |     |ATMDE*
**	      ||     |     |     |     |     |     | (10)|     |     |     |     | (36)|
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITDT  ||     |     |     |     |     |STOPN|STWDC|STSDP|     |     |     |STCLG|
**	(13)  ||     |     |     |     |     |OTTDA*     |OTTDA*     |     |     |     |
**	      ||     |     |     |     |     |ATRVN|ATRVN|ATRVN|     |     |     |     |
**	      ||     |     |     |     |     |     |ATMDE*     |     |     |     |ATMDE*
**	      ||     |     |     |     |     | (27)| (63)| (18)|     |     |     | (36)|
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITNC  ||     |     |STWCC|     |     |     |STWDC|STOPN|     |STOPX|STWND|STCLG|
**	(14)  ||     |     |     |     |     |     |     |     |     |     |     |     |
**	      ||     |     |ATRVN|     |     |     |     |     |     |     |ATRVN|ATMDE*
**	      ||     |     |ATMDE*     |     |     |ATMDE*ATMDE*     |ATMDE*ATMDE*     |
**	      ||     |     | (34)|     |     |     | (35)| (30)|     | (45)| (25)| (36)|
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITNCQ ||     |     |     |     |     |     |STWDC|STSDP|     |STSDX|STWND|STCLG|
**	(15)  ||     |     |     |     |     |     |     |     |     |     |     |     |
**	      ||     |     |     |     |     |     |ATSDQ|ATSDQ|     |ATSDQ|ATSDQ|ATMDE*
**	      ||     |     |     |     |     |     |ATMDE*ATMDE*     |ATMDE*ATMDE*     |
**	      ||     |     |     |     |     |     | (37)| (15)|     | (46)| (39)| (36)|
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+

**	State ||STCLD|STWNC|STWCC|STWCR|STWRS|STOPN|STWDC|STSDP|STOPX|STSDX|STWND|STCLG|
**	      ||  00 |  01 |  02 |  03 |  04 |  05 |  06 |  07 |  08 |  09 |  10 |  11 |
**	================================================================================
**	Event ||
**	================================================================================
**	ITNOC ||STCLD|     |     |     |     |     |     |     |     |     |     |     |
**	(16)  ||     |     |     |     |     |     |     |     |     |     |     |     |
**	      ||ATLSN|     |     |     |     |     |     |     |     |     |     |     |
**	      ||ATCCB|     |     |     |     |     |     |     |     |     |     |     |
**	      ||ATMDE*     |     |     |     |     |     |     |     |     |     |     |
**	      || (16)|     |     |     |     |     |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITABT ||STCLD|STCLG|STCLG|STCLG|STCLG|STCLG|STCLG|STCLG|STCLG|STCLG|STCLG|STCLD|
**	(17)  ||     |OTTDI|OTTDI|     |OTTDI|OTTDI|     |OTTDI|OTTDI|OTTDI|     |     |
**	      ||     |ATDCN*ATDCN*ATDCN*ATDCN*ATDCN*ATDCN*ATPGQ|ATDCN*ATPGQ|ATDCN*ATCCB|
**            ||ATMDE*     |     |     |     |     |     |ATDCN*     |ATDCN*     |ATMDE*
**	      || (6) | (17)| (17)| (10)| (17)| (17)| (10)| (62)| (17)| (62)| (10)| (19)|
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITNRS ||STCLD|STCLD|     |     |     |     |     |     |     |     |     |     |
**	(18)  ||     |OTTDI|     |     |     |     |     |     |     |     |     |     |
**	      ||ATCCB|ATCCB|     |     |     |     |     |     |     |     |     |     |
**	      ||ATMDE*ATMDE*     |     |     |     |     |     |     |     |     |     |
**	      || (19)| (33)|     |     |     |     |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITXOF ||     |     |     |     |     |STOPN|     |STSDP|STOPX|STSDX|     |     |
**	(19)  ||     |     |     |     |     |     |     |     |     |     |     |     |
**	      ||     |     |     |     |     |ATSXF|     |ATSXF|ATSXF|ATSXF|     |     |
**	      ||     |     |     |     |     |ATMDE*     |ATMDE*ATMDE*ATMDE*     |     |
**	      ||     |     |     |     |     | (47)|     | (48)| (49)| (50)|     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITXON ||     |     |     |     |     |STOPN|     |STSDP|STOPN|STSDP|     |     |
**	(20)  ||     |     |     |     |     |     |     |     |     |     |     |     |
**	      ||     |     |     |     |     |ATRXF|     |ATRXF|ATRXF|ATRXF|     |     |
**	      ||     |     |     |     |     |     |     |     |ATRVN|ATRVN|     |     |
**	      ||     |     |     |     |     |ATMDE*     |ATMDE*ATMDE*ATMDE*     |     |
**	      ||     |     |     |     |     | (51)|     | (52)| (53)| (54)|     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITDTX ||     |     |     |     |     |STOPX|     |STSDX|     |     |     |STCLG|
**	(21)  ||     |     |     |     |     |OTTDA*     |OTTDA*     |     |     |     |
**	      ||     |     |     |     |     |     |     |     |     |     |     |ATMDE*
**	      ||     |     |     |     |     | (55)|     | (56)|     |     |     | (36)|
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITTDAQ||     |     |     |     |     |     |     |STSDP|     |STSDX|     |     |
**	(22)  ||     |     |     |     |     |     |     |OTXOF|     |OTXOF|     |     |
**	      ||     |     |     |     |     |     |     |OTDT |     |OTDT |     |     |
**	      ||     |     |     |     |     |     |     |ATSNQ*     |ATSNQ*     |     |
**	      ||     |     |     |     |     |     |     | (60)|     | (61)|     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITNCX ||     |     |     |     |     |     |STWDC|STOPN|     |STOPX|STWND|STCLG|
**	(23)  ||     |     |     |     |     |     |     |OTXON|     |OTXON|     |     |
**	      ||     |     |     |     |     |     |ATMDE*ATMDE*     |ATMDE*ATRVN|ATMDE*
**            ||     |     |     |     |     |     |     |     |     |     |ATMDE*     |
**	      ||     |     |     |     |     |     | (35)| (58)|     | (59)| (25)| (36)|
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
**	ITNLSF||STCLG|     |     |     |     |     |     |     |     |     |     |     |
**	(24)  ||     |     |     |     |     |     |     |     |     |     |     |     |
**	      ||ATLSN|     |     |     |     |     |     |     |     |     |     |     |
**	      ||ATDCN*     |     |     |     |     |     |     |     |     |     |     |
**	      || (12)|     |     |     |     |     |     |     |     |     |     |     |
**	------++-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+

**
** The following is a list of fixes or functional enhancements which are known
** to be desirable or required for fully functional TL operation:
**
** 1.  Addition of a CLOSE function to the GCCCL protocol driver interface.
**
**
** Inputs:
**      mde			Pointer to Message Data Element
**
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-Jun-96 (gordy)
**	    Renamed gcc_tl() to gcc_tl_event() and created gcc_tl()
**	    to disallow nested processing of events.  The first
**	    call to this routine sets a flag which causes recursive
**	    calls to queue the MDE for processing sequentially.
**	30-Jun-1997 (rajus01)
**	    Terminate the security mechanisms and free the GCS control block
**	    queue. Reset the encrypt flag.
**	20-Aug-97 (gordy)
**	    Moved CCB cleanup to own routine, gcc_tl_release_cb().
*/

STATUS
gcc_tl( GCC_MDE *mde )
{
    GCC_CCB	*ccb = mde->ccb;
    STATUS	status = OK;
    
    /*
    ** If the event queue active flag is set, this
    ** is a recursive call and TL events are being
    ** processed by a previous instance of this 
    ** routine.  Queue the current event, it will
    ** be processed once we return to the active
    ** processing level.
    */
    if ( ccb->ccb_tce.tl_evq.active )
    {
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 2 )
	{
            TRdisplay( "gcc_tl: queueing event %s %s\n",
                   gcx_look( gcx_mde_service, mde->service_primitive ),
                   gcx_look( gcx_mde_primitive, mde->primitive_type ));
	}
# endif /* GCXDEBUG */

	QUinsert( (QUEUE *)mde, ccb->ccb_tce.tl_evq.evq.q_prev );
	return( OK );
    }

    /*
    ** Flag active processing so any subsequent 
    ** recursive calls will be queued.  We will 
    ** process all queued events once the stack
    ** unwinds to us.
    */
    ccb->ccb_tce.tl_evq.active = TRUE;

    /*
    ** Process the current input event.  The queue
    ** should be empty since no other instance of
    ** this routine is active, and the initial call
    ** always clears the queue before exiting.
    */
    status = gcc_tl_event( mde );

    /*
    ** Process any queued input events.  These
    ** events are the result of recursive calls
    ** to this routine while we are processing
    ** events.  Note that this will also process
    ** events queued during the processing of
    ** this loop.
    */
    while( ccb->ccb_tce.tl_evq.evq.q_next != &ccb->ccb_tce.tl_evq.evq )
    {
	mde = (GCC_MDE *)QUremove( ccb->ccb_tce.tl_evq.evq.q_next );

# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 2 )
	{
            TRdisplay( "gcc_tl: processing queued event %s %s\n",
                   gcx_look( gcx_mde_service, mde->service_primitive ),
                   gcx_look( gcx_mde_primitive, mde->primitive_type ));
	}
# endif /* GCXDEBUG */

	status = gcc_tl_event( mde );
    }

    ccb->ccb_tce.tl_evq.active = FALSE;		/* Processing done */

    /*
    ** If a processed event resulted in the
    ** CCB being released, a flag was set so
    ** that we would perform the actual release
    ** once we were done processing the queue.
    */
    if ( ccb->ccb_tce.t_flags & GCCTL_CCB_RELEASE )
	gcc_tl_release_ccb( ccb );

    return( status );
} /* gcc_tl */


/*
** Name: gcc_tl_event
**
** Description:
**	Transport layer event processing.  An input MDE is evaluated
**	to a TL event.  The input event causes a state transition and
**	the associated outputs and actions are executed.
**	
** Input:
**	mde		Current MDE event.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**      18-Jan-88 (jbowers)
**          Initial routine creation
**      03-Jan-89 (jbowers)
**          Added better error handling for the case where evaluation of the
**	    input event fails (i.e., gcc_tl_evinp returns a failure).
**      13-Dec-89 (cmorris)
**          Don't log errors returned by FSM; the FSM's already logged them!
**          Removed code that checked for orphan MDEs. AL Disassociate
**          changes did away with orphans.
**	19-Jun-96 (gordy)
**	    Renamed from gcc_tl() to handle recursive requests.
**	    This routine will no longer be called recursively.
**	30-Jun-97 (rajus01)
**	    Removed gcx_tdump() trace to avoid dumping the buffer
**	    in unwanted places in the error log.
*/

static STATUS
gcc_tl_event( GCC_MDE *mde )
{
    u_i4	input_event;
    STATUS	status;
    
    /*
    ** Invoke  the FSM executor, passing it the current state and the 
    ** input event, to execute the output event(s), the action(s)
    ** and return the new state.  The input event is determined by the
    ** output of gcc_tl_evinp();
    */

    status = OK;
    input_event = gcc_tl_evinp(mde, &status);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "gcc_tl: primitive %s %s event %s size %d\n",
		gcx_look( gcx_mde_service, mde->service_primitive ),
		gcx_look(gcx_mde_primitive, mde->primitive_type ),
		gcx_look( gcx_gcc_itl, input_event ),
		(i4)(mde->p2 - mde->p1) );
# endif /* GCXDEBUG */

    if (status == OK)
    {
	i4		output_event;
	u_i4           action_code;
	i4		i;
	i4		j;
	GCC_CCB		*ccb = mde->ccb;
	u_i4		old_state;

	/*
	** Determine the state transition for this state and input event.
	*/

	old_state = ccb->ccb_tce.t_state;
	j = tlfsm_tbl[input_event][old_state];

	/* Make sure we have a legal transition */
	if (j != 0)
	{
	    ccb->ccb_tce.t_state = tlfsm_entry[j].state;

	    /* do tracing... */
	    GCX_TRACER_3_MACRO("gcc_tlfsm>", mde, "event = %d, state = %d->%d",
			     input_event, old_state, (u_i4)ccb->ccb_tce.t_state);

# ifdef GCXDEBUG
	    if ( IIGCc_global->gcc_trace_level >=1 )
		TRdisplay( "gcc_tlfsm: new state=%s->%s\n", 
			   gcx_look(gcx_gcc_stl, old_state), 
			   gcx_look(gcx_gcc_stl, (u_i4)ccb->ccb_tce.t_state) );
# endif /* GCXDEBUG */

	    /*
	    ** For each output event in the list for this state and input event,
	    ** call the output executor.
	    */

	    for( 
		 i = 0;
		 i < TL_OUT_MAX  && 
		 (output_event = tlfsm_entry[j].output[i]) > 0;
		 i++
	       )
	    {
		if ((status = gcc_tlout_exec(output_event, mde, ccb)) != OK)
		{
		    /*
		    ** Execution of the output event has failed. Aborting the 
		    ** connection will already have been handled so we just get
		    ** out of here.
		    */
		    return (status);
		} /* end if */
	    } /* end for */

	    /*
	    ** For each action in the list for this state 
	    ** and input event, call the action executor.
	    */

	    for(
		 i = 0;
		 i < TL_ACTN_MAX &&
		 (action_code = tlfsm_entry[j].action[i]) != 0;
		 i++
	       )
	    {
		if ((status = gcc_tlactn_exec(action_code, mde, ccb)) != OK)
		{
		    /*
		    ** Execution of the action has failed. Aborting the 
		    ** connection will already have been handled so we just get
		    ** out of here.
		    */
		    return (status);
		} /* end if */
	    } /* end for */
	} /* end if */
	else
	{
	    GCC_ERLIST		erlist;

	    /*
	    ** This is an illegal transition.  This is a program bug or a
	    ** protocol violation.  Log the error and abort.  The message 
	    ** contains two parms: the input event and the current state.
	    */
	    erlist.gcc_parm_cnt = 2;
	    erlist.gcc_erparm[0].size = sizeof(input_event);
	    erlist.gcc_erparm[0].value = (PTR)&input_event;
	    erlist.gcc_erparm[1].size = sizeof(old_state);
	    erlist.gcc_erparm[1].value = (PTR)&old_state;
	    status = E_GC280C_TL_FSM_STATE;

	    gcc_tl_abort(mde, ccb, &status, (CL_ERR_DESC *) NULL, &erlist);
	    status = FAIL;
	}
    }
    else
    {
	/*
	** Evaluation of the input event has failed.  Log the status
	** returned by gcc_tl_evinp, and abort FSM execution. 
	*/
	gcc_tl_abort(mde, mde->ccb, &status, (CL_ERR_DESC *) NULL,
			(GCC_ERLIST *) NULL);
	status = FAIL;
    }

    return(status);
} /* gcc_tl_event */


/*
** Name: gcc_tlinit	- Transport Layer initialization routine
**
** Description:
**      gcc_tlinit is invoked by ILC on comm server startup.  It initializes
**      the TL and returns.  The following functions are performed: 
**
**	- Invoke GCpinit() to initialize the protocol CL and construct the PCT;
**	
**      - Establish an outstanding GCC_LISTEN on each local network port.
**
** Inputs:
**      None
**
** Outputs:
**      generic_status	    Mainline status code for initialization error.
**	system_status	    System_specific status code.
**
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-Jan-88 (jbowers)
**          Initial routine creation
**      03-Nov-88 (jbowers)
**          If *ALL* protocol initialization attempts fail, return failed
**	    status. 
**  	14-Feb-90 (cmorris)
**  	    Introduced support for "no listen" drivers:- attempt
**  	    open even if NO_PORT flag is set.
**  	12-Dec-91 (cmorris)
**  	    If a protocol is flagged as both NO_PORT and NO_OUT,
**  	    disable attempts to open it.
**  	20-Dec-91 (cmorris)
**  	    Don't log errors that our caller is going to log!
**	21-Nov-95 (gordy)
**	    Don't do network LISTENs if we are embedded.
**	 8-Sep-98 (gordy)
**	    Save installation registry info.
**	05-may-2000 (somsa01 & rajus01)
**	    Changed the logic around such that we now have two loops. The
**	    first loop will gather information about the protocols from
**	    config.dat . The second loop will attempt to initialize each
**	    protocol. In this way, when winding back from a possible
**	    error from starting a protocol, we do not hit a situation where
**	    we would be analyzing an uninitialized pct_pce for a particular
**	    protocol.
**	28-jun-2000 (somsa01)
**	    In gcc_tlinit(), the previous change did not take into account
**	    that a particular protocol description may not even exist in
**	    config.dat . This case is now handled.
**	28-jul-2000 (somsa01)
**	    After a successful startup, write "PASS\n" to standard output.
**	    This is to signal the parent that started us that we have no
**	    more information to write out.
**	11-sep-2000 (somsa01)
**	    Make sure we do a PMinit() and PMload() before calling PMget().
**	 6-Jun-03 (gordy)
**	    No longer update PCT, protocol information now saved in PIB.
**	21-Jan-04 (gordy)
**	    Copy PCT for registry.
*/

STATUS
gcc_tlinit( STATUS *generic_status, CL_ERR_DESC *system_status )
{
    GCC_PCT		*pct;
    STATUS		status;
    i4			not_failed = 0;
    u_i4		i;

    *generic_status = OK;

    /*
    ** Invoke GCpinit() to construct the PCT, then save the PCT.
    ** Also save a copy of the PCT for the registry.
    */
    if ( (status = GCpinit( &pct, generic_status, system_status )) != OK )
	return( FAIL );

    IIGCc_global->gcc_pct = pct;
    MEcopy( (PTR)pct, sizeof( *pct ), (PTR)&IIGCc_global->gcc_reg_pct );

    /*
    ** The embedded Comm Server does not establish 
    ** a listen on network protocols.
    */
    if ( ! (IIGCc_global->gcc_flags & GCC_STANDALONE) )  return( OK );

    /*
    ** Loop through protocol table looking for protocols
    ** enabled for listening.
    */
    for( i = 0; i < pct->pct_n_entries; i++ )
    {
	GCC_PCE	*pce = &pct->pct_pce[i];
	GCC_PIB	*pib;
	GCC_CCB	*ccb;
	GCC_MDE	*mde;
	char	pmsym[128];
	char	*pm_val;

	CVupper( pce->pce_pid );

	/*
	** Check if protocol permits listening.
	*/
	if ( pce->pce_flags & PCT_NO_PORT )  continue;

	/*
	** Check protocol status.  Skip if not enabled.
	*/
	STprintf( pmsym, "!.%s.status", pce->pce_pid );

	if ( PMget( pmsym, &pm_val ) != OK  ||  STcasecmp( pm_val, "ON" ) )
	    continue;

	/*
	** Allocate and initialize resources.
	*/
	if ( 
	     ! (pib = (GCC_PIB *)gcc_alloc( sizeof(GCC_PIB) ))  ||
	     ! (mde = gcc_add_mde( NULL, 0 ))  ||  
	     ! (ccb = gcc_add_ccb()) 
	   )
	{
	    *generic_status = E_GC2004_ALLOCN_FAIL;
	    gcc_er_log( generic_status, NULL, NULL, NULL );
	    return( FAIL );
	}

	QUinit( &pib->q );
	STcopy( pce->pce_pid, pib->pid );
	STcopy( IIGCc_global->gcc_host, pib->host );

	STprintf( pmsym, "!.%s.port", pce->pce_pid );

	if ( PMget( pmsym, &pm_val ) == OK )
	    STcopy( pm_val, pib->addr );
	else  if ( *pce->pce_port != EOS )
	    STcopy( pce->pce_port, pib->addr );
	else
	    STcopy( "II", pib->addr );

	QUinsert( &pib->q, IIGCc_global->gcc_pib_q.q_prev );

	/*
	** Place an OPEN on each protocol routine.  Each instance 
	** represents a nascent protocol.  Invoke the TL FSM to do 
	** it.  If all attempts fail, return a failed status to 
	** indicate initialization failure.  If any is successful, 
	** return OK.
	**
	** The MDE is set up as a T_OPEN request, a non-OSI 
	** specified SDU.
	*/
	mde->service_primitive = T_OPEN;
	mde->primitive_type = RQS;
	mde->ccb = ccb;
	ccb->ccb_tce.pib = pib;
	ccb->ccb_tce.prot_pce = pce;
	ccb->ccb_tce.t_state = STCLD;

	if ( gcc_tl(mde) != OK )
	    pib->flags |= PCT_OPN_FAIL;
	else
	    not_failed++;
    }

    /*
    ** Initialization is complete.  Return to caller.
    */
    if ( not_failed )
    {
	if (!stdout_closed)
	{
	    SIstd_write(SI_STD_OUT, "PASS\n");
	    stdout_closed = TRUE;
	}

	status = OK;
    }
    else
    {
	if (!stdout_closed)
	{
	    SIstd_write(SI_STD_OUT, "FAIL\n");
	    stdout_closed = TRUE;
	}
	*generic_status = E_GC280A_NTWK_INIT_FAIL;
	status = FAIL;
    }

    return( status );
}  /* gcc_tlinit */


/*
** Name: gcc_tlterm	- Transport Layer termination routine
**
** Description:
**      gcc_tlterm is invoked by ILC on comm server shutdown.  It currently
**	performs only the function of calling GCpterm().
**
** Inputs:
**      None
**
** Outputs:
**      generic_status	    Mainline status code for initialization error.
**	system_status	    System_specific status code.
**
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-Jan-88 (jbowers)
**          Initial routine creation
**      04-Oct-89 (cmorris)
**          Make function of type STATUS.
**  	20-Dec-91 (cmorris)
**  	    Don't log errors that our caller is going to log!
*/

STATUS
gcc_tlterm( STATUS *generic_status, CL_ERR_DESC *system_status )
{
    return( GCpterm( IIGCc_global->gcc_pct, generic_status, system_status ) );
} /* gcc_tlterm */


/*
** Name: gcc_tl_registry
**
** Description:
**	Open inactive protocols used for installation registry.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK.
**
** History:
**	 8-Sep-98 (gordy)
**	    Created.
**	 6-Jun-03 (gordy)
**	    Protocol information now saved in PIB.
**	21-Jan-04 (gordy)
**	    Registry PCT is now simply a copy of original PCT
**	    and is initialized in gcc_tlinit().  Simply scan
**	    registry PCT and use directly rather than scanning
**	    original PCT, building registry PCT and then re-
**	    scanning registy PCT.  Added registry flags.  No
**	    longer need gcc_call() signature as now provided
**	    by external cover routine.
**	 6-Jul-06 (usha)
**	    Added support for "Show server" command o/p to display
**	    registry address.
**	18-Aug-06 (usha)
**	    Added support to display stats info for GCC sessions.
**	    
*/

STATUS
gcc_tl_registry()
{
    GCC_PCT		*pct;
    STATUS		status;
    u_i4		i;

    /*
    ** The embedded Comm Server does not establish 
    ** a listen on network protocols.  Also check
    ** for prior initialization.
    */
    if ( ! (IIGCc_global->gcc_flags & GCC_STANDALONE)  ||
         IIGCc_global->gcc_reg_flags & GCC_REG_ACTIVE )  
	return( OK );

    IIGCc_global->gcc_reg_flags |= GCC_REG_ACTIVE;

    /*
    ** Loop through protocol table looking for protocols
    ** enabled for registry listening.
    **
    ** Place an OPEN on each inactive registy protocol routine.  
    ** Invoke the TL FSM to do it.  Allocate a CCB and an MDE 
    ** for each protocol listen.  The MDE is set up as a T_OPEN 
    ** request, a non-OSI specified SDU.
    */
    pct = &IIGCc_global->gcc_reg_pct;
    for( i = 0; i < pct->pct_n_entries; i++ )
    {
	GCC_PCE	*pce = &pct->pct_pce[i];
	GCC_PIB	*pib;
	GCC_CCB	*ccb;
	GCC_MDE	*mde;
	char	pmsym[128];
	char	*pm_val;

	/*
	** Check if protocol permits listening.
	*/
	if ( pce->pce_flags & PCT_NO_PORT )  continue;

	/*
	** Check registry status.  Skip if not enabled.
	*/
	STprintf( pmsym, "$.$.registry.%s.status", pce->pce_pid );

	if ( PMget( pmsym, &pm_val ) != OK  ||  STcasecmp( pm_val, "ON" ) )
	    continue;

	/*
	** A unique registry protocol must also be configured.
	*/
	STprintf( pmsym, "$.$.registry.%s.port", pce->pce_pid );
	if ( PMget( pmsym, &pm_val ) != OK )  continue;

	/*
	** Allocate and initialize the protocol information block.
	*/
	if ( 
	     ! (pib = (GCC_PIB *)gcc_alloc( sizeof(GCC_PIB) ))  ||
	     ! (mde = gcc_add_mde(NULL, 0))  ||  
	     ! (ccb = gcc_add_ccb()) 
	   )
	{
	    STATUS status = E_GC2004_ALLOCN_FAIL;
	    gcc_er_log( &status, NULL, NULL, NULL );
	    return( status );
	}

	QUinit( &pib->q );
	STcopy( pce->pce_pid, pib->pid );
	STcopy( IIGCc_global->gcc_host, pib->host );
	STcopy( pm_val, pib->addr );
        pib->flags |= GCC_REG_MASTER;
	QUinsert( &pib->q, &IIGCc_global->gcc_pib_q );

	/*
	** Issue OPEN request.
	*/
	mde->service_primitive = T_OPEN;
	mde->primitive_type = RQS;
	mde->ccb = ccb;
	ccb->ccb_tce.pib = pib;
	ccb->ccb_tce.prot_pce = pce;
	ccb->ccb_tce.t_state = STCLD;

	status = gcc_tl( mde );
	if ( status != OK )  pib->flags |= PCT_OPN_FAIL;
    }

    /*
    ** Failure in registry protocols is not fatal.
    */
    return( OK );
}  /* gcc_tl_registry */


/*
** Name: gcc_tl_exit	- Asynchronous protocol service exit
**
** Description:
**	gcc_tl_exit is the exit handler invoked by all protocol handler
**	routines, upon completion of asynchronous protocol service requests.
**	It was specified as the completion exit by the original invoker of the
**	protocol handler service request.  It is passed a pointer to the MDE
**	associated with the service request (For details of the protocol
**	handler interface, see the CL specification for GCC).  It invokes
**	gcc_tl, the main transport layer entry point, with the MDE.
**	
**	For certain protocol handler service function completions, some special
**	processing is required.
**
** Inputs:
**      mde				The identifier specified at the time
**                                      of the service request.  It is a
**                                      pointer to the MDE for the service 
**					request.
**
** Outputs:
**      None
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	None
**
** History:
**      20-Jan-88 (jbowers)
**          Initial routine creation
**      22-Dec-88 (jbowers)
**	    When a network OPEN has failed, set primitive to N_RESET indication.
**      13-Mar-89 (jbowers)
**          Fix for Bug 4985, failure to clean up a session when remote comm
**	    server crashes.
**      23-Mar-89 (jbowers)
**	    Fix to allow for a specified number of retries of a failed
**	    GCC_LISTEN.  The ccb_tce.lsn_retry_cnt is incrmented on failure.
**	    If < max, set N_DISCONNECT indication.  Else, set N_RESET
**	    indication.
**      25-Aug-89 (scmorris)
**          Output the cei on trace.
**      12-Dec-89 (cmorris)
**          Inhibit logging listen failures unless listen retry count
**          is exceeded.
**      13-Dec-89 (cmorris)
**          Don't log error returned by gcc_tl; it's already been logged!
**  	18-Dec-90 (cmorris)
**  	    Incorporation of Eurotech fix: an immediate shutdown was
**  	    effected if no GCC_OPEN had succeeded so far and one had failed.
**  	    Changed logic so that a shutdown will occur only if a GCC_OPEN
**  	    request has been made for all network protocols for which an
**  	    open will be performed. This is necessary because a protocol
**  	    driver may drive its completion routine before all opens have
**  	    been attempted (ie the completion routine may be driven
**  	    synchronously).
**  	14-Feb-91 (cmorris)
**  	    Changed "all_failed" logic of open completion to match
**  	    support for "no listen" drivers.
**      13-Aug-91 (cmorris)
**  	    Timestamp ccb at connection startup.
**	12-nov-93 (robf)
**          On LISTEN copy remote node into ccb_hdr if CL tells us what it
**	    is.
**      13-Apr-1994 (daveb) 62240
**          Keep some stats when SEND and RECEIVE are sucessful.
**	25-Oct-95 (sweeney)
**	    Reset failure count on valid inbound connection.
**	21-Nov-95 (gordy)
**	    Don't log successfull OPEN if embedded.
**	30-Jun-97 (rajus01)
**	    Changed GCC_RECEIVE to receive encrypted data and to
**	    perform decryption.
**	22-Oct-97 (rajus01)
**	    Extra buffer space is provided to perform decryption. 
**	    ( mde->pxtend - mde->p1 ).
**	 2-Oct-98 (gordy)
**	    Make protocol info visible through MO/GCM for successful opens.
**	09-jun-1999 (somsa01)
**	    On successful startup, write E_GC2A10_LOAD_CONFIG to the errlog.
**	15-jun-1999 (somsa01)
**	    Moved printout of E_GC2A10_LOAD_CONFIG to iigcc.c .
**	28-jul-2000 (somsa01)
**	    Write out the successful port ids to standard output.
**	24-may-2001 (somsa01)
**	    Since node_id will always be a string, use STcopy() to transfer
**	    the data from one node_id to another.
**	 6-Jun-03 (gordy)
**	    Attach CCB as MIB object after GCC_LISTEN successfully completes.
**	    Save local/remote addresses returned by LISTEN/CONNECT.  Protocol
**	    information now stored in PIB.
**	21-Jul-09 (gordy)
**	    Remove security label stuff.
**      06-Oct-09 (rajus01) Bug: 120552, SD issue:129268
**          When starting more than one GCC server the symbolic ports are not
**          incremented (Example: II1, II2) in the startup messages, while the
**          actual portnumber is increasing and printed correctly.
**	    Log the actual_port_id for a successful network open request.
**	22-Apr-10 (rajus01) SIR 123621
**	    After a successful network open request update PIB with 
**	    the actual_port_id. 
*/

static VOID
gcc_tl_exit( PTR ptr )
{
    GCC_MDE		*mde = (GCC_MDE *)ptr;
    GCC_P_PLIST		*p_list;
    STATUS		status;
    STATUS		generic_status;
    CL_ERR_DESC		system_status;
    GCC_CCB		*ccb;

    generic_status = OK;
    p_list = &mde->mde_act_parms.p_plist;
    ccb = mde->ccb;
    
    /*
    ** Perform the processing appropriate to the completed service request.
    **
    ** It is here that errors resulting from CL level operations are discovered
    ** and logged.  Status from the operation is checked in the parm list for
    ** successful completion.  If OK, deal with the specific situation if any
    ** special processing is required.  If not OK, the error is logged out.
    ** Usually there is mainline level information to be logged out, to provide
    ** context for the CL level information.  Then modify the primitive
    ** indicator in the MDE to indicate an N_DISCONNECT indication (unless this
    ** is already an N_DISCONNECT) or an N_RESET indication, depending on the
    ** particular circumstances.
    **
    ** If this is a failed GCC_OPEN, N_RESET indication is set.  In the case of
    ** a failed GCC_LISTEN, a sequence of retries will be made until it is
    ** successful or a retry maximum has been exceeded.  The retry counter in
    ** the CCB is incremented at the completion of a failed GCC_LISTEN.  If the
    ** maximum is not exceeded, N_DISCONNECT indication is set.  If it is
    ** exceeded, N_RESET indication is set.
    **
    ** Note: This is the point at which network status and management
    ** indications might be returned.  This is a future enhancement to deal with
    ** these.
    */

    /* do tracing... */
    GCX_TRACER_2_MACRO("gcc_tl_exit>", mde,
		       "primitive = %d, cei = %d",
		       p_list->function_invoked,
		       ccb->ccb_hdr.lcl_cei);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
    {
	TRdisplay( "----- UP\n" );
	TRdisplay( "gcc_tl_exit: primitive %s cei %d\n",
		gcx_look( gcx_gcc_rq, p_list->function_invoked),
                ccb->ccb_hdr.lcl_cei ); 
    }
# endif /* GCXDEBUG */

    switch (p_list->function_invoked)
    {
    case GCC_LISTEN:
    {
	if (p_list->generic_status != OK)
	{
	    GCC_ERLIST		erlist;
	    GCC_PCE		*pce;

   	    /* 
	    ** A GCC_LISTEN has failed.
	    ** Log the error.  The message contains 2 parms: the protocol
	    ** id and the listen port.
	    */

	    ccb->ccb_tce.lsn_retry_cnt += 1;
	    if (ccb->ccb_tce.lsn_retry_cnt < GCCTL_LSNMAX)
	    {
	        mde->service_primitive = N_LSNCLUP;
	        mde->primitive_type = RQS;
	    }
	    else
	    {
	        mde->service_primitive = N_RESET;
  	        pce = ccb->ccb_tce.prot_pce;
	        erlist.gcc_parm_cnt = 2;
	        erlist.gcc_erparm[0].value = pce->pce_pid;
	        erlist.gcc_erparm[0].size = STlength(pce->pce_pid);
	        erlist.gcc_erparm[1].value = ccb->ccb_tce.pib->addr;
	        erlist.gcc_erparm[1].size = STlength(ccb->ccb_tce.pib->addr);
	        generic_status = E_GC2813_NTWK_LSN_FAIL;
	        gcc_er_log(&generic_status, (CL_ERR_DESC *)NULL, mde, 
                    	   &erlist);
		gcc_er_log(&p_list->generic_status,
			   &p_list->system_status,
		           mde,
		           (GCC_ERLIST *)NULL);
	    } /* end if */
	} 
	else		/* Successful Listen */
	{
	    char	buff[16];

	    if ( p_list->function_parms.listen.lcl_addr )
	    {
		STcopy( ccb->ccb_tce.prot_pce->pce_pid, 
			ccb->ccb_hdr.lcl_addr.n_sel);
		STcopy( p_list->function_parms.listen.lcl_addr->node_id,
			ccb->ccb_hdr.lcl_addr.node_id );
		STcopy( p_list->function_parms.listen.lcl_addr->port_id,
			ccb->ccb_hdr.lcl_addr.port_id );
	    }

	    if ( p_list->function_parms.listen.rmt_addr )
	    {
		STcopy( ccb->ccb_tce.prot_pce->pce_pid, 
			ccb->ccb_hdr.rmt_addr.n_sel);
		STcopy( p_list->function_parms.listen.rmt_addr->node_id,
			ccb->ccb_hdr.rmt_addr.node_id );
		STcopy( p_list->function_parms.listen.rmt_addr->port_id,
			ccb->ccb_hdr.rmt_addr.port_id );
	    }

	    /*
	    ** Create MIB entry for connection
	    */
	    MOulongout(0, (u_i8)ccb->ccb_hdr.lcl_cei, sizeof(buff), buff);

	    if ( MOattach( MO_INSTANCE_VAR, 
			   GCC_MIB_CONNECTION, buff, (PTR)ccb ) == OK )
		ccb->ccb_hdr.flags |= CCB_MIB_ATTACH;

	    /* Increment connection counters */
	    IIGCc_global->gcc_ib_conn_ct++;
	    IIGCc_global->gcc_conn_ct++;
	    ccb->ccb_hdr.flags |= CCB_IB_CONN;
	    ccb->ccb_hdr.inbound = Yes;

	    /* reset the "listen retry count" */
	    ccb->ccb_tce.lsn_retry_cnt = 0;

	    /* timestamp the ccb */
	    TMnow(&ccb->ccb_hdr.timestamp);
	} 

	ccb->ccb_tce.pcb = p_list->pcb;
	break;
    } /* end case GCC_LISTEN */
    case GCC_CONNECT:
    {
	ccb->ccb_tce.pcb = p_list->pcb;

	if (p_list->generic_status != OK)
	{
	    GCC_ERLIST		erlist;
	    STATUS		temp_status;

  	    /*
	    ** This is the failure of a GCC_CONNECT.
	    ** Log the error.  The message contains 3 parms: the protocol
	    ** id, the node and the listen port.
	    */

	    erlist.gcc_parm_cnt = 3;
	    erlist.gcc_erparm[0].value = ccb->ccb_hdr.trg_addr.n_sel;
	    erlist.gcc_erparm[1].value = ccb->ccb_hdr.trg_addr.node_id;
	    erlist.gcc_erparm[2].value = ccb->ccb_hdr.trg_addr.port_id;
	    erlist.gcc_erparm[0].size = 
	        STlength(erlist.gcc_erparm[0].value);
	    erlist.gcc_erparm[1].size = 
	        STlength(erlist.gcc_erparm[1].value);
	    erlist.gcc_erparm[2].size = 
	        STlength(erlist.gcc_erparm[2].value);
	    temp_status = E_GC2809_NTWK_CONNECTION_FAIL;
	    gcc_er_log( &temp_status, NULL, mde, &erlist );
	    gcc_er_log( &p_list->generic_status, 
			&p_list->system_status, mde, NULL );
	    mde->service_primitive = N_DISCONNECT;
	    mde->primitive_type = IND;
	    mde->mde_generic_status[0] = E_GC2819_NTWK_CONNECTION_FAIL;
	    mde->mde_generic_status[1] = p_list->generic_status;
	}
	else		/* Successful Connect */
	{
	    if ( p_list->function_parms.connect.lcl_addr )
	    {
		STcopy( ccb->ccb_tce.prot_pce->pce_pid, 
			ccb->ccb_hdr.lcl_addr.n_sel );
		STcopy( p_list->function_parms.connect.lcl_addr->node_id,
			ccb->ccb_hdr.lcl_addr.node_id );
		STcopy( p_list->function_parms.connect.lcl_addr->port_id,
			ccb->ccb_hdr.lcl_addr.port_id );
	    }

	    if ( p_list->function_parms.connect.rmt_addr )
	    {
		STcopy( ccb->ccb_tce.prot_pce->pce_pid, 
			ccb->ccb_hdr.rmt_addr.n_sel );
		STcopy( p_list->function_parms.connect.rmt_addr->node_id,
			ccb->ccb_hdr.rmt_addr.node_id );
		STcopy( p_list->function_parms.connect.rmt_addr->port_id,
			ccb->ccb_hdr.rmt_addr.port_id );
	    }
	}
        break;
    } /* end case GCC_CONNECT */
    case GCC_DISCONNECT:
    {
	STATUS		temp_status;

	/* Log end of connection */

	temp_status = E_GC2802_NTWK_CONNCTN_END;
	gcc_er_log(&temp_status,
		   (CL_ERR_DESC *)NULL,
		   mde,
		   (GCC_ERLIST *)NULL);

	/* Decrement connection counter. */
	if (ccb->ccb_hdr.flags & CCB_IB_CONN)
	{
	    IIGCc_global->gcc_ib_conn_ct--;
	    IIGCc_global->gcc_conn_ct--;
	}
        break;
    } /* end case GCC_DISCONNECT */
    case GCC_RECEIVE:
    {
	if (p_list->generic_status != OK) 
	{
	    /* log error */
    	    if (ccb->ccb_tce.t_state != STWND)
    	    	gcc_er_log(&p_list->generic_status,
		       	   &p_list->system_status,
		       	   mde,
		       	   (GCC_ERLIST *)NULL);
	    mde->service_primitive = N_DISCONNECT;
	    mde->primitive_type = IND;
	    mde->mde_generic_status[0] = E_GC280D_NTWK_ERROR;
	    mde->mde_generic_status[1] = p_list->generic_status;
	}
	else
	{
	    static  gcc_data_in_bits = 0;

	    /* 
	    ** A RECEIVE for normal flow data has completed.  Save the input
	    ** buffer length.
	    */
	    IIGCc_global->gcc_msgs_in++;
	    IIGCc_global->gcc_data_in += 
				(gcc_data_in_bits + p_list->buffer_lng) / 1024;
	    gcc_data_in_bits =  (gcc_data_in_bits + p_list->buffer_lng) % 1024;

            ccb->ccb_tce.msgs_in = IIGCc_global->gcc_msgs_in;
            ccb->ccb_tce.data_in = IIGCc_global->gcc_data_in;

	    if ( ! (ccb->ccb_tce.t_flags & GCCTL_ENCRYPT) )
		mde->p2 = mde->p1 + p_list->buffer_lng;
	    else
	    {
	    	GCS_EDEC_PARM	dec_parm;
		STATUS		gcs_status;

# ifdef GCXDEBUG
    		if ( IIGCc_global->gcc_trace_level >= 4 )
		{
		    TRdisplay( "gcc_tl_exit: received %d encrypted bytes\n",
			       p_list->buffer_lng );
		    gcx_tdump( mde->p1, p_list->buffer_lng );
		}
# endif /* GCXDEBUG */

		dec_parm.escb = ccb->ccb_tce.gcs_cb;
		dec_parm.size = (i4)(mde->pxtend - mde->p1);
		dec_parm.length = p_list->buffer_lng;
		dec_parm.buffer	= mde->p1;

		gcs_status = IIgcs_call( GCS_OP_E_DECODE, 
					 ccb->ccb_tce.mech_id, (PTR)&dec_parm );

		if ( gcs_status == OK )
		    mde->p2 = mde->p1 + dec_parm.length;
		else
		{
		    /* This will be considered internal abort */
	    	    status = E_GC281D_TL_ENCRYPTION_FAIL;
	    	    gcc_tl_abort( mde, ccb, &status, NULL, NULL );
		    mde->p2 = mde->p1;
		}
	    }

# ifdef GCXDEBUG
	    if ( IIGCc_global->gcc_trace_level >= 3 )
	    {
		TRdisplay( "gcc_tl_exit: received %d bytes\n",
			   (i4)(mde->p2 - mde->p1) );
		gcx_tdump( mde->p1, (i4)(mde->p2 - mde->p1) );
	    }
# endif /* GCXDEBUG */
	} /* end else */
        break;
    } /* end case GCC_RECEIVE */
    case GCC_SEND:
    {
	if (p_list->generic_status == OK) /* keep the stats */
	{
	    static  gcc_data_out_bits = 0;

	    IIGCc_global->gcc_msgs_out++;
	    IIGCc_global->gcc_data_out += 
				(gcc_data_out_bits + p_list->buffer_lng) / 1024;
	    gcc_data_out_bits = (gcc_data_out_bits + p_list->buffer_lng) % 1024;
            ccb->ccb_tce.msgs_out = IIGCc_global->gcc_msgs_out;
            ccb->ccb_tce.data_out = IIGCc_global->gcc_data_out;
	}
	else			/* Log the error. */
	{
	    gcc_er_log( &p_list->generic_status, 
			&p_list->system_status, mde, NULL );
	    mde->service_primitive = N_DISCONNECT;
	    mde->primitive_type = IND;
	    mde->mde_generic_status[0] = E_GC280D_NTWK_ERROR;
	    mde->mde_generic_status[1] = p_list->generic_status;
	} /* end if */
	
        break;
    } /* end case GCC_SEND */
    case GCC_OPEN:
    {
	GCC_PCE		*pce = ccb->ccb_tce.prot_pce;
	GCC_PIB		*pib = ccb->ccb_tce.pib;
	GCC_ERLIST	erlist;

	if ( p_list->generic_status != OK )
	{
	    i4	not_failed = 0;
	    
	    pib->flags |= PCT_OPN_FAIL;

	    /*
	    ** Log the error.  The message contains 2 parms: the protocol
	    ** id and the listen port.
	    */
	    erlist.gcc_parm_cnt = 2;
	    erlist.gcc_erparm[0].value = pce->pce_pid;
	    /* Use the actual symbolic listen port in the error messages.*/
	    if( p_list->function_parms.open.actual_port_id != NULL )
	        erlist.gcc_erparm[1].value = 
			p_list->function_parms.open.actual_port_id;
	    else
	        erlist.gcc_erparm[1].value = pib->addr;
	    erlist.gcc_erparm[0].size = STlength(erlist.gcc_erparm[0].value);
	    erlist.gcc_erparm[1].size = STlength(erlist.gcc_erparm[1].value);
	    generic_status = E_GC2808_NTWK_OPEN_FAIL;
	    gcc_er_log( &generic_status, NULL, NULL, &erlist );
	    gcc_er_log( &p_list->generic_status,
			&p_list->system_status, NULL, NULL );
	    mde->service_primitive = N_RESET;
	    mde->primitive_type = IND;

	    /*
	    ** See if all attempted protocol opens have failed.  If a 
	    ** protocol open has failed, PCT_OPN_FAIL will be set.  If 
	    ** the open is not yet complete or did succeed, we count 
	    ** it with "not_failed." Since the completion of protocol 
	    ** opens is asynchronous, we check at each open completion 
	    ** since we don't know which will be the last one.
	    */
	    
	    for( 
		 pib = (GCC_PIB *)IIGCc_global->gcc_pib_q.q_next;
		 (QUEUE *)pib != &IIGCc_global->gcc_pib_q;
		 pib = (GCC_PIB *)pib->q.q_next
	       )
		if ( ! (pib->flags & PCT_OPN_FAIL) )  not_failed++;

	    if ( ! not_failed )
	    {
		generic_status = E_GC280A_NTWK_INIT_FAIL;
		gcc_er_log( &generic_status, NULL, NULL, NULL );
		GCshut();
	    }
	}
	else		/* Successful OPEN */
	{
	    STATUS	temp_status;
	    char	port[ GCC_L_PORT * 2 + 5 ];
	    char	buffer[ GCC_L_PROTOCOL + GCC_L_PORT + 16 ];

	    pib->flags |= PCT_OPEN;

	    if ( p_list->function_parms.open.lsn_port )
		STcopy( p_list->function_parms.open.lsn_port, pib->port );
	    else
		STcopy( pib->addr, pib->port );

	    /*
	    ** Add MO instance for network protocols
	    */
	    if ( 
		 pce >=  &IIGCc_global->gcc_pct->pct_pce[0]  &&
		 pce <=  &IIGCc_global->gcc_pct->pct_pce[ 
					IIGCc_global->gcc_pct->pct_n_entries ] 
	       )
		MOattach( 0, GCC_MIB_PROTOCOL, pib->pid, (PTR)pib );
	    else  if (
		       pce >=  &IIGCc_global->gcc_reg_pct.pct_pce[0]  &&
		       pce <=  &IIGCc_global->gcc_reg_pct.pct_pce[ 
				    IIGCc_global->gcc_reg_pct.pct_n_entries ] 
	       )
		MOattach( 0, GCC_MIB_REGISTRY, pib->pid, (PTR)pib );

	    /*
	    ** The attempted network open succeeded.  Log it. 
	    */
	    if ( ! STcompare( pib->addr, pib->port ) )
		STcopy( pib->addr, port );
	    else if( p_list->function_parms.open.actual_port_id != NULL )
	    {
		/* Use the actual symbolic listen port in the error messages.*/
                   STprintf( port, "%s (%s)",
                       p_list->function_parms.open.actual_port_id, pib->port );
		   STcopy(p_list->function_parms.open.actual_port_id, pib->addr );
	    }
	    else
                   STprintf( port, "%s (%s)",
                       p_list->function_parms.open.port_id, pib->port );

	    erlist.gcc_parm_cnt = 2;
	    erlist.gcc_erparm[0].value = pce->pce_pid;
	    erlist.gcc_erparm[1].value = port;
	    erlist.gcc_erparm[0].size = STlength(erlist.gcc_erparm[0].value);
	    erlist.gcc_erparm[1].size = STlength(erlist.gcc_erparm[1].value);

	    temp_status = E_GC2815_NTWK_OPEN;
	    gcc_er_log( &temp_status, NULL, NULL, &erlist );

	    if (!stdout_closed)
	    {
		STprintf(buffer, "    %s port = %s\n", pce->pce_pid, port);
		SIstd_write(SI_STD_OUT, buffer);
	    }
	} /* end if */
	break;
    } /* end case GCC_OPEN */
    default:
	{
	    /*
	    ** Unrecognized service primitive.  This is a program bug.  Log
	    ** the problem and abort the connection.
	    */

	    mde->service_primitive = N_ABORT;
	    mde->primitive_type = RQS;
	    mde->mde_generic_status[0] = E_GC2805_TL_INTERNAL_ERROR;
	    gcc_er_log(&mde->mde_generic_status[0],
		       (CL_ERR_DESC *) NULL,
		       mde,
		       (GCC_ERLIST *) NULL);
	    break;
	} /* end default */
    }/* end switch */

    (VOID) gcc_tl(mde);
# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
    {
	TRdisplay( "----- UP-END\n" );
    }
# endif /* GCXDEBUG */
    return;
} /* gcc_tl_exit */


/*
** Name: gcc_tl_evinp	- TL FSM input event id evaluation routine
**
** Description:
**      gcc_tl_evinp accepts as input an MDE received by the  TL.  It  
**      evaluates the input event based on the MDE type and possibly on  
**      some other status information.  It returns as its function value the  
**      ID of the input event to be used by the FSM engine.
**
** Inputs:
**      mde			Pointer to dequeued Message Data Element
**
** Outputs:
**      generic_status		Mainline status code.
**	system_status		System_specific status code.
**
**	Returns:
**	    u_i4	input_event_id
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      25-Jan-88 (jbowers)
**          Initial function implementation
**      22-Dec-88 (jbowers)
**	    Added ITNRS input event recognition.
**      13-Dec-89 (cmorris)
**          If we fail to evaluate the input event, return an illegal
**          event rather than 0 (which IS legal!).
**      14-Dec-89 (cmorris)
**          Fix error logging when illegal cei is found in tpdu.
**      07-Feb-90 (cmorris)
**	    Process N_LSNCLUP service primitive.
**	14-Feb-90 (cmorris)
**	    Process N_ABORT event.
**      07-Mar-90 (cmorris)
**          Process T_P_ABORT event.
**      19-Jan-93 (cmorris)
**  	    Got rid of system_status argument:- never used.
**	31-Mar-06 (gordy)
**	    Sanity check lengths read from IO buffers.
*/

static u_i4
gcc_tl_evinp( GCC_MDE *mde, STATUS *generic_status )
{
    TL_FP_PCI		fp_pci;
    u_i4               input_event = ITMAX;
    i4			i;
    char		*pci;

    /*
    ** The input event is determined from the contents of the MDE.  It may be an
    ** SDU from the transport user above, or an SDU from Network Layer
    ** possibly containing a TPDU.  In some cases there may be some incidental
    ** processing beyond the evaluation of the input, since this may be the only
    ** place that  a certain condition may be recognized.  An example is the
    ** allocation of a CCB when the input is a Connection Request (CR) TPDU.
    */

    GCX_TRACER_3_MACRO("gcctl_evinp>", mde,
	"primitive = %d/%d, size = %d",
	    mde->service_primitive,
	    mde->primitive_type,
	    (i4)(mde->p2 - mde->p1));
	    
    switch (mde->service_primitive)
    {
    case N_CONNECT:
    {
	/*
	** Both IND and CNF represent nascent connections.  The former is a
	** completed GCC_LISTEN and the latter is a completed GCC_CONNECT.
	*/
	
	switch (mde->primitive_type)
	{
	case IND:
	{
	    input_event = ITNCI;
	    break;
	}
	case CNF:
	{
	    input_event = ITNCC;
	    break;
	}
	default:
	    {
		*generic_status = E_GC2806_TL_FSM_INP;
		break;
	    }
	} /* end switch */
	break;
    } /* end case N_CONNECT */
    case N_DISCONNECT:
    {
	switch (mde->primitive_type)
	{
	case IND:
	{
	    input_event = ITNDI;
	    break;
	}
	case CNF:
	{
	    input_event = ITNDC;
	    break;
	}
	default:
	    {
		*generic_status = E_GC2806_TL_FSM_INP;
		break;
	    }
	} /* end switch */
	break;
    } /* end case N_DISCONNECT */
    case N_DATA:
    {
	switch (mde->primitive_type)
	{
	case IND:
	{
	    /*
	    ** This is a completed GCC_RECEIVE.
	    ** The N_DATA indication carries a TPDU in the PCI area.  The TPDU
	    ** code in the PCI header indicates the type.
	    */
	    fp_pci.li = 0;
	    fp_pci.tpdu = 0;
	    fp_pci.dst_ref = 0xFFFF;

	    pci = mde->p1;
	    if ( pci < mde->p2 )  pci += GCgeti1(pci, &fp_pci.li);
	    if ( pci < mde->p2 )  pci += GCgeti1(pci, &fp_pci.tpdu);

	    switch (fp_pci.tpdu & TPDU_CODE_MASK)
	    {
	    case TCR_TPDU:
	    {
		input_event = ITCR;
		break;
	    }
	    case TCC_TPDU:
	    {
		input_event = ITCC;
		break;
	    }
	    case TDR_TPDU:
	    {
		input_event = ITDR;
		break;
	    }
	    case TDC_TPDU:
	    {
		input_event = ITDC;
		break;
	    }
	    case TDT_TPDU:
	    {
		input_event = ITDT;
		break;
	    }
	    default:
		{
		    GCC_ERLIST		erlist;
		    STATUS		tmp_gnrc_status;
		    u_char		tpdu;

		    /*
		    ** This is an unrecognized TPDU type.  Log the error.  This
		    ** is a serious error.  Either the partner TL has sent
		    ** something bad, or the message has gotten hit.  In either
		    ** case, we cannot proceed.  We have to abort the
		    ** connection; unfortunately, we can't depend on the
		    ** dest_ref to identfy the connection, since something is
		    ** badly amiss.
		    */

		    tmp_gnrc_status = E_GC2811_TL_INVALID_TPDU;
		    tpdu = fp_pci.tpdu & TPDU_CODE_MASK;
		    erlist.gcc_parm_cnt = 1;
		    erlist.gcc_erparm[0].size = sizeof(tpdu);
		    erlist.gcc_erparm[0].value = (PTR)&tpdu;
		    gcc_er_log( &tmp_gnrc_status, NULL, mde, &erlist );
		    *generic_status = E_GC2806_TL_FSM_INP;
		    break;
		}
	    } /* end switch */

	    /*
	    ** The dest-ref element of the TPDU header is our connection ID for
	    ** all but new connections.  Get the CCB for the connection except
    	    ** in the case where the TPDU is a CR, in which case no connection 
    	    ** previously existed.
	    */
	    if (input_event != ITCR)
	    {
		if ( pci < mde->p2 )  pci += GCgeti2(pci, &fp_pci.dst_ref);

		if (mde->ccb->ccb_hdr.lcl_cei != fp_pci.dst_ref)
		{
		    GCC_ERLIST		erlist;

		    /*
		    ** Unable to find the CCB.  This is an error condition, but
		    ** not one that is easily dealt with.  We have an MDE
		    ** representing an event on an association that is gone.
		    ** All we can do is protect ourselves from consequent
		    ** damage.  
		    */

		    *generic_status = E_GC2812_TL_INVALID_CONNCTN;
		    erlist.gcc_parm_cnt = 1;
		    erlist.gcc_erparm[0].size = sizeof(fp_pci.dst_ref);
		    erlist.gcc_erparm[0].value = (PTR)&fp_pci.dst_ref;
		    gcc_er_log(generic_status, NULL, mde, &erlist);
		    *generic_status = E_GC2806_TL_FSM_INP;
		    input_event = ITMAX;
		    break;
		} /* end if */
		else
		{
		    /* Check for a previously received XOFF */
		    if ((input_event == ITDT) &&
			(mde->ccb->ccb_tce.t_flags & GCCTL_RXOF))
		    {
			input_event = ITDTX;
		    }
		} /* end else */
	    } /* end if */
	    break;
	} /* end case IND */
	case CNF:
	{
	    /* Is the send queue empty? */

	    if(mde->ccb->ccb_tce.t_snd_q.q_next == &mde->ccb->ccb_tce.t_snd_q)
	    {
		/* Have we sent an XOFF? */

		if (mde->ccb->ccb_tce.t_flags & GCCTL_SXOF)
		{
		    input_event = ITNCX;
		}
		else
		{
		    input_event = ITNC;
		}
	    }
	    else
	    {
		input_event = ITNCQ;
	    }
	    break;
	}
	default:
	    {
		*generic_status = E_GC2806_TL_FSM_INP;
		break;
	    }
	} /* end switch */
	break;
    } /* end case N_DATA */
    case N_OPEN:
    {
	input_event = ITNOC;
	break;
    } /* end case N_OPEN */
    case N_RESET:
    {
	input_event = ITNRS;
	break;
    } /* end case N_RESET */
    case N_LSNCLUP:
    {
	/* 
	** The network listen has failed. Increment connection counters
	** to allow for their being decremented when the CCB is released
	 */
	input_event = ITNLSF;
	break;
    } /* end case N_LSNCLUP */
    case N_ABORT:
    {
	/* treat this just like a TL abort */
	input_event = ITABT;
	break;
    } /* end case N_ABORT */
    case T_CONNECT:
    {
	switch (mde->primitive_type)
	{
	case RQS:
	{
	    mde->ccb->ccb_hdr.async_cnt += 1;
	    input_event = ITTCQ;
	    break;
	} /* end case RQS */
	case RSP:
	{
	    input_event = ITTCS;
	    break;
	}
	default:
	    {
		*generic_status = E_GC2806_TL_FSM_INP;
		break;
	    }
	} /* end switch */
	break;
    } /* end case T_CONNECT */
    case T_DISCONNECT:
    {
	/* T_DISCONNECT has only a request form */
	
	input_event = ITTDR;
	break;
    } /* end case T_DISCONNECT */
    case T_DATA:
    {
	/* T_DATA has only a request form */

	/* See if the send queue is full and we haven't sent XOFF */
	
	if ((mde->ccb->ccb_tce.t_sq_cnt >= GCCTL_SQMAX) &&
	    !(mde->ccb->ccb_tce.t_flags & GCCTL_SXOF))
	{
	    input_event = ITTDAQ;
	}
	else
	{
	    input_event = ITTDA;
	}
	break;
    } /* end case T_DATA */
    case T_OPEN:
    {
	/* T_OPEN has only a request form */
	
	input_event = ITOPN;
	break;
    } /* end case T_OPEN */
    case T_XOFF:
    {
	/* T_XOFF has only a request form */
	
	input_event = ITXOF;
	break;
    } /* end case T_XOFF */
    case T_XON:
    {
	/* T_XON has only a request form */
	
	input_event = ITXON;
	break;
    } /* end case T_XON */
    case T_P_ABORT:
    {
	/* T_P_ABORT has only a request form */
	
	input_event = ITABT;
	break;
    } /* end case T_P_ABORT */
    default:
	{
	    *generic_status = E_GC2806_TL_FSM_INP;
	    break;
	}
    } /* end switch */
    return(input_event);
} /* gcc_tl_evinp */


/*
** Name: gcc_tlout_exec	- TL FSM output event execution routine
**
** Description:
**      gcc_tlout_exec accepts as input an output event identifier.
**	It executes this output event and returns.
**
**	It also accepts as input a pointer to an 
**	MDE or NULL.  If the pointer is non-null, the MDE pointed to is
**	used for the output event.  Otherwise, a new one is obtained.
** 
**
** Inputs:
**	output_event_id		Identifier of the output event
**      mde			Pointer to dequeued Message Data Element
**  	ccb 	    	    	Pointer to CCB
**
**	Returns:
**	    OK
**          FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      25-Jan-88 (jbowers)
**          Initial function implementation
**      16-Mar-89 (jbowers)
**          Fix to allow an MDE to be immediately deleted by
**	    an upper layer if required.  This is mainly used by output events
**	    which generate a new MDE such as OTTDI.
**      16-Sep-89 (ham)
**          Byte alignment.  Instead of using a C strucure pointer to
**	    address the pci in the mde, we use a PTR to build and strip
**	    out the pci byte by byte.  Furthermore sizeof was replaced
**	    use of defines. 
**      05-Mar-90 (cmorris)
**	    Pass back status from calling another layer; this prevents
**	    us ploughing on with events and actions oblvious to the fact
**	    that we've already hit a fatal error.
**  	28-Dec-92 (cmorris)
**  	    Removed tl_parms that are unused.
**  	28-Dec-92 (cmorris)
**  	    Handle XON/XOFF mde's consistently with AL.
**  	28_Dec-92 (cmorris)
**  	    Tidy up creation of pci's.
**  	08-Jan-93 (cmorris)
**  	    Supported ability to get version number from the CR TPDU.
**  	    Previously, we didn't get it because old code screwed up
**  	    sending it. However, by testing the length indicator in
**  	    the PCI, it's possible to deduce whether it's sent. We
**  	    don't need the ability to change the protocol (and hence
**  	    the version) right now, but we may do soon.
**	30-Jun-97 (rajus01)
**	    Added support to negotiate on version number, security
**	    mechanisms to use to provide data encryption and decryption.
**	    Re-worked on message exchanges during connection establishment
**	    phase. TL version number is increased 2. Major changes are 
**	    done in OTCR, OTTCC, OTTCI, OTCC. 
**	20-Aug-97 (gordy)
**	    Added OTTDR (superset of OTTDI) to process DR TPDU prior 
**	    to sending disconnect to SL.  Reworked MDE PCI and user 
**	    data processing to be more robust and permit future 
**	    extensions with minimal impact on backward compatibility.  
**	    Added support for encryption modes.
**	10-Oct-97 (gordy)
**	    Increased TL protocol level for SL password encryption.
**	17-Oct-97 (rajus01)
**	    Replaced IIGCc_global->gcc_ob_mode references with 
**	    ccb->ccb_hdr.emode. Replaced GCS_NO_MECH input to
**	    gcc_gcs_mechanisms() with ccb->ccb_hdr.emech
**	31-Mar-98 (gordy)
**	    Added default encryption mechanism.
**	16-Jan-04 (gordy)
**	    MDE's may not be upto 32K.
**	16-Jun-04 (gordy)
**	    Added session mask to CR and CC.
**	31-Mar-06 (gordy)
**	    Sanity check lengths read from IO buffers.
**	21-Jul-09 (gordy)
**	    Encryption mechanism dynamically allocated.
*/

static STATUS
gcc_tlout_exec( i4  output_event_id, GCC_MDE *mde, GCC_CCB *ccb )
{
    STATUS	    status = OK;
    i4	    layer_id;
    char	    *pci;

    GCX_TRACER_1_MACRO( "gcc_tlout_exec>", mde, 
			"output_event = %d", output_event_id );

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "gcc_tlout_exec: output %s\n",
		gcx_look( gcx_gcc_otl, output_event_id ) );
# endif /* GCXDEBUG */

    switch( output_event_id )
    {
    case OTTCI:
    {
	/*
	** A CR TPDU was received. Fill in the 
	** new CCB for the transport connection.  
	** Send T_CONNECT indication to SL.
	*/
	GCC_ERLIST	erlist;
	STATUS		tmp_gnrc_status;
	TL_CR_PCI	cr_pci;
	u_char		mech_cnt;
	u_char		mech_ids[ GCS_MECH_MAX ];

	/*
	** Get the address of the pci and strip it out.
	*/
	if ( (mde->p2 - mde->p1) < SZ_CR_FP_PCI )
	{
	    tmp_gnrc_status = E_GC281A_TL_INV_TPDU_DATA;
	    cr_pci.tpdu_cdt = TCR_TPDU;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( cr_pci.tpdu_cdt );
	    erlist.gcc_erparm[0].value = (PTR)&cr_pci.tpdu_cdt;
	    gcc_tl_abort( mde, ccb, &tmp_gnrc_status, NULL, &erlist );
	    return( FAIL );
	}

        mde->p1 += GCgeti1( mde->p1, &cr_pci.li );
	pci = mde->p1;
	mde->p1 += min( cr_pci.li, mde->p2 - mde->p1 );

	/*
        ** Get the fixed portion of the connection request pci.
	**
	** The TPDU type was validated in gcc_tl_evinp() to
	** generate ITCR input.  The source reference is used
	** as provided by partner.  Destination reference and
	** class/options are ignored.
	*/
	pci += GCgeti1(pci, &cr_pci.tpdu_cdt);
	pci += GCgeti2(pci, &cr_pci.dst_ref);
	pci += GCgeti2(pci, &cr_pci.src_ref);
	pci += GCgeti1(pci, &cr_pci.cls_option);

	/*
	** Read optional parameters of CR TPDU
	** (provide defaults for missing parms).
	*/
	cr_pci.tpdu_size.size = TPDU_8K_SZ;
	cr_pci.version_no.version_no = TCR_VN_1;
	cr_pci.protection_parm.options = 0;
	mech_cnt = 0;

	while( pci < mde->p1 )
	{
	    u_char	parm_code;
	    u_char	parm_lng = 255;			/* worst case */
	    char	*parm;

	    /*
	    ** Read the common header for each parameter.
	    */
	    pci += GCgeti1(pci, &parm_code);
	    if ( pci < mde->p1 )  pci += GCgeti1(pci, &parm_lng);
	    parm = pci;
	    pci += parm_lng;
	    if ( pci > mde->p1 )  break;		/* pci overrun */

	    /*
	    ** Process each parameter.
	    */
	    switch( parm_code )
	    {
		case TCR_SZ_TPDU :
		    if ( parm_lng )
			GCgeti1( parm, &cr_pci.tpdu_size.size );
		    break;

		case TCR_VERSION_NO :
		    if ( parm_lng )
			GCgeti1( parm, &cr_pci.version_no.version_no );
		    break;

		case TCR_SESS_MASK :
		    /*
		    ** Currently, only a fixed size mask is supported.
		    ** Anything less is ignored and no mask will be
		    ** returned to partner.  Anything greater will be
		    ** truncated.  For any future change in protocol,
		    ** our partner will be able to determine how to
		    ** handle the mask based on our response.
		    */
		    if ( parm_lng >= GCC_SESS_MASK_MAX )
		    {
			i4 index;

			for( index = 0; index < GCC_SESS_MASK_MAX; index++ )
			    parm += GCgeti1( parm, &ccb->ccb_hdr.mask[index] );

			ccb->ccb_hdr.mask_len = GCC_SESS_MASK_MAX;
		    }
		    break;

		case TCR_PROTECT_PARM :
		    if ( parm_lng )
		    {
			parm += GCgeti1(parm, &cr_pci.protection_parm.options);
			parm_lng--;
			for( mech_cnt = 0; mech_cnt < parm_lng; mech_cnt++ )
			    parm += GCgeti1( parm, &mech_ids[ mech_cnt ] );	
		    }
		    break;

		case TCR_CG_TSAP :	/* Ignored */
		case TCR_CD_TSAP :
		default :
		    break;
	    }
	}

	/*
	** Make sure we haven't overrun the pci.
	*/
	if ( pci > mde->p1 )
	{
	    tmp_gnrc_status = E_GC281A_TL_INV_TPDU_DATA;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( cr_pci.tpdu_cdt );
	    erlist.gcc_erparm[0].value = (PTR)&cr_pci.tpdu_cdt;
	    gcc_tl_abort( mde, ccb, &tmp_gnrc_status, NULL, &erlist );
	    return( FAIL );
	}

	/*
	** Set the ccb parameters.
	** Negotiate version number and TPDU size.
	*/
	ccb->ccb_tce.src_ref = ccb->ccb_hdr.lcl_cei;
	ccb->ccb_tce.dst_ref = cr_pci.src_ref;
	ccb->ccb_hdr.rmt_cei = ccb->ccb_tce.dst_ref;
	gcc_tpdu_size( ccb, cr_pci.tpdu_size.size );
	ccb->ccb_tce.version_no = min(cr_pci.version_no.version_no, TCR_VN_2);
	if ( ccb->ccb_tce.version_no >= TCR_VN_2 )
	    ccb->ccb_hdr.flags |= CCB_PWD_ENCRYPT;	/* SL capability */

	/*
	** Disable encryption if disabled locally or
	** if not requested by either partner.  Read 
	** the user data area if encryption desired.
	*/
	if ( IIGCc_global->gcc_ib_mode == GCC_ENCRYPT_OFF  ||
	     (IIGCc_global->gcc_ib_mode == GCC_ENCRYPT_OPT  && 
	      ! (cr_pci.protection_parm.options & TCR_ENCRYPT_REQUEST)) )
	    mech_cnt = 0;

	if ( mech_cnt  &&  gcc_ind_encrypt( mde, ccb, mech_cnt, mech_ids, 
					    IIGCc_global->gcc_ib_mech ) != OK )
	    return( FAIL );

	/* 
	** Log start of connection 
	*/
	tmp_gnrc_status = E_GC2810_INCMG_NTWK_CONNCTN;
	erlist.gcc_parm_cnt = 1;
	erlist.gcc_erparm[0].size = sizeof( ccb->ccb_tce.dst_ref );
	erlist.gcc_erparm[0].value = (PTR)&ccb->ccb_tce.dst_ref;
	gcc_er_log( &tmp_gnrc_status, NULL, mde, &erlist );

	/*
	** Construct the T_CONNECT SDU in the MDE and send the SDU to SL.
	*/
	mde->service_primitive = T_CONNECT;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    }  /* end case OTTCI */
    case OTTCC:
    {
	/*
	** A CC TPDU was received.  Complete filling 
	** in the CCB for the transport connection.
	** Send a T_CONNECT confirm to SL.
	*/
	GCC_ERLIST	erlist;
	STATUS		tmp_gnrc_status;
	TL_CC_PCI	cc_pci;

	/*
	** Get the address of the pci and strip it out.
	*/
	if ( (mde->p2 - mde->p1) < SZ_CC_FP_PCI )
	{
	    tmp_gnrc_status = E_GC281A_TL_INV_TPDU_DATA;
	    cc_pci.tpdu_cdt = TCC_TPDU;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( cc_pci.tpdu_cdt );
	    erlist.gcc_erparm[0].value = (PTR)&cc_pci.tpdu_cdt;
	    gcc_tl_abort( mde, ccb, &tmp_gnrc_status, NULL, &erlist );
	    return( FAIL );
	}

        mde->p1 += GCgeti1( mde->p1, &cc_pci.li );
	pci = mde->p1;
	mde->p1 += min( cc_pci.li, mde->p2 - mde->p1 );

	/*
	** NASTY HACK! Comm Servers at TCC_VN_1 sent 
	** an extra byte of data with the pci.  This 
	** was done even after the mistake was known!  
	** Some Comm Servers even have a nasty feature
	** of not emptying the MDE before responding,
	** resulting in invalid data in the user data
	** area.  Because of the restrictions imposed 
	** by the older Comm Servers in handling the 
	** variable parts of the pci, the length in 
	** question will never be a valid value.  We 
	** check for the invalid length and adjust 
	** things accordingly.
	*/ 
	if ( cc_pci.li == SZ_TL_CC_INVALID )  
	{
	    i4  offset = SZ_TL_CC_INVALID - (SZ_TL_CC_PCI - SZ_TL_li);
	    mde->p2 = mde->p1 -= offset;
	}

	/*
        ** Get the fixed portion of the connection confirm pci.
	** The TPDU type was validated in gcc_tl_evinp() to
	** generate ITCC input, as was the destination reference.
	** The source reference is used as provided by partner.
	** Class/options are ignored.
	*/
	pci += GCgeti1( pci, &cc_pci.tpdu_cdt );
	pci += GCgeti2( pci, &cc_pci.dst_ref );
	pci += GCgeti2( pci, &cc_pci.src_ref );
	pci += GCgeti1( pci, &cc_pci.cls_option );

	/*
	** Read optional parameters of CC TPDU
	** (provide defaults for missing parms).
	*/
	cc_pci.tpdu_size.size = TPDU_8K_SZ;
	cc_pci.version_no.version_no = TCR_VN_1;
	cc_pci.protection_parm.mech_id = GCS_NO_MECH; 
	cc_pci.session_mask.parm_lng = 0;

	while( pci < mde->p1 )
	{
	    u_char	parm_code;
	    u_char	parm_lng = 255;			/* worst case */
	    char	*parm;
	    i4		i;

	    /*
	    ** Read the common header for each parameter.
	    */
	    pci += GCgeti1( pci, &parm_code );
	    if ( pci < mde->p1 )  pci += GCgeti1( pci, &parm_lng );
	    parm = pci;
	    pci += parm_lng;
	    if ( pci > mde->p1 )  break;		/* pci overrun */

	    /*
	    ** Process each parameter.
	    */
	    switch( parm_code )
	    {
		case TCC_SZ_TPDU :
		    if ( parm_lng )
			GCgeti1( parm, &cc_pci.tpdu_size.size );
		    break;

		case TCC_VERSION_NO :
		    if ( parm_lng )
			GCgeti1( parm, &cc_pci.version_no.version_no );
		    break;

		case TCC_SESS_MASK :
		    /*
		    ** Partner must respond with matching mask
		    ** or not at all.
		    */
		    if ( parm_lng != ccb->ccb_hdr.mask_len )
		    {
			tmp_gnrc_status = E_GC281A_TL_INV_TPDU_DATA;
			erlist.gcc_parm_cnt = 1;
			erlist.gcc_erparm[0].size = sizeof( cc_pci.tpdu_cdt );
			erlist.gcc_erparm[0].value = (PTR)&cc_pci.tpdu_cdt;
			gcc_tl_abort(mde, ccb, &tmp_gnrc_status, NULL, &erlist);
			return( FAIL );
		    }
		    
		    /*
		    ** Read partner mask and combine with our own.
		    */
		    for( i = 0; i < ccb->ccb_hdr.mask_len; i++ )
		    {
			parm += GCgeti1( parm, &cc_pci.session_mask.mask[i] );
			ccb->ccb_hdr.mask[i] ^= cc_pci.session_mask.mask[i];
		    }

		    cc_pci.session_mask.parm_lng = parm_lng;
		    break;

		case TCC_PROTECT_PARM :
		    if ( parm_lng )
			GCgeti1( parm, &cc_pci.protection_parm.mech_id);
		    break;

		case TCC_CG_TSAP :	/* Ignored */
		case TCC_CD_TSAP :
		default :
		    break;
	    }
	}

	/*
	** Make sure we haven't overrun the pci.
	*/
	if ( pci > mde->p1 )
	{
	    tmp_gnrc_status = E_GC281A_TL_INV_TPDU_DATA;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( cc_pci.tpdu_cdt );
	    erlist.gcc_erparm[0].value = (PTR)&cc_pci.tpdu_cdt;
	    gcc_tl_abort( mde, ccb, &tmp_gnrc_status, NULL, &erlist );
	    return( FAIL );
	}

	/*
	** Set the ccb parameters.
	** Negotiate version number and TPDU size.
	*/
	ccb->ccb_tce.dst_ref = cc_pci.src_ref;
	ccb->ccb_hdr.rmt_cei = ccb->ccb_tce.dst_ref;
	gcc_tpdu_size( ccb, cc_pci.tpdu_size.size );
	ccb->ccb_tce.mech_id = cc_pci.protection_parm.mech_id; 
	ccb->ccb_tce.version_no = min(cc_pci.version_no.version_no, TCC_VN_2);

	if ( ccb->ccb_tce.version_no >= TCC_VN_2 )
	    ccb->ccb_hdr.flags |= CCB_PWD_ENCRYPT;	/* SL capability */

	/*
	** If partner did not send session mask,
	** then make sure we don't use ours either.
	*/
	if ( cc_pci.session_mask.parm_lng <= 0 )
	    ccb->ccb_hdr.mask_len = 0;

	/*
	** Read the user data area.  A TL abort will
	** be issued if an error occurs.
	*/
	if ( gcc_cnf_encrypt( mde, ccb ) != OK )
	    return( FAIL );

	/*
	** Shut down the connection if encryption is required
	** but could not be enabled (for whatever reason).
	*/
	if ( ccb->ccb_hdr.emode == GCC_ENCRYPT_REQ  &&
	     ! (ccb->ccb_tce.t_flags & GCCTL_ENCRYPT ) )
	{
	    /*
	    ** Send a T_DISCONNECT request to ourselves in a 
	    ** new MDE to shut down the connection.  Issue a
	    ** T_DISCONNECT indication to the SL to shut down 
	    ** the upper layers.  
	    */
	    tmp_gnrc_status = E_GC281C_TL_NO_ENCRYPTION;
	    gcc_tl_shut( mde, ccb, &tmp_gnrc_status, NULL, NULL );

	    /*
	    ** Construct the T_DISCONNECT SDU in the MDE.
	    */
	    mde->service_primitive = T_DISCONNECT;
	    mde->primitive_type = IND;
	    mde->mde_generic_status[0] = tmp_gnrc_status;
	}
	else
	{
	    /* 
	    ** Log start of connection 
	    */
	    tmp_gnrc_status = E_GC280F_OUTGNG_NTWK_CONNCTN;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof(ccb->ccb_tce.dst_ref);
	    erlist.gcc_erparm[0].value = (PTR)&ccb->ccb_tce.dst_ref;
	    gcc_er_log(&tmp_gnrc_status, NULL, mde, &erlist );

	    /*
	    ** Construct the T_CONNECT SDU in the MDE.
	    */
	    mde->service_primitive = T_CONNECT;
	    mde->primitive_type = CNF;
	}

	layer_id = UP;		/* Send the SDU to SL */
	break;
    } /* end case OTTCC */
    case OTTDR:
    {
	/*
	** A DR TPDU was received.  Send a T_DISCONNECT indication to SL.  
	*/
	GCC_MDE		*tdi_mde;
	TL_DR_PCI	dr_pci;
	STATUS		tmp_gnrc_status;
	GCC_ERLIST	erlist;

	/*
	** Get the address of the pci and strip it out.
	*/
	if ( (mde->p2 - mde->p1) < SZ_TL_DR_PCI )
	{
	    tmp_gnrc_status = E_GC281A_TL_INV_TPDU_DATA;
	    dr_pci.tpdu = TDR_TPDU;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( dr_pci.tpdu );
	    erlist.gcc_erparm[0].value = (PTR)&dr_pci.tpdu;
	    gcc_tl_abort( mde, ccb, &tmp_gnrc_status, NULL, &erlist );
	    return( FAIL );
	}

        mde->p1 += GCgeti1( mde->p1, &dr_pci.li );
	pci = mde->p1;
	mde->p1 += min( dr_pci.li, mde->p2 - mde->p1 );

	/*
	** NASTY HACK! Comm Servers at TCC_VN_1 included
	** the variable portion of the pci but didn't
	** actually set the data in the variable portion.
	** Luckily, they also sent an invalid length, so
	** we can detect the problem.  We check for the
	** invalid length and adjust things accordingly.
	*/
	if ( dr_pci.li == SZ_TL_DR_INVALID )
	{
	    i4  offset = SZ_TL_DR_INVALID - (SZ_TL_DR_PCI - SZ_TL_li);
	    mde->p1 -= offset;
	    mde->p2 -= offset;
	}

	/*
        ** Get the fixed portion of the disconnect request pci.
	** The TPDU type was validated in gcc_tl_evinp() to
	** generate ITDR input, as was the destination reference.
	*/
	pci += GCgeti1( pci, &dr_pci.tpdu );
	pci += GCgeti2( pci, &dr_pci.dst_ref );
	pci += GCgeti2( pci, &dr_pci.src_ref );
	pci += GCgeti1( pci, &dr_pci.reason );

	/*
	** Read optional parameters of DR TPDU
	** (provide defaults for missing parms).
	*/
	dr_pci.sec_rsn.sec_rsn = OK;

	while( pci < mde->p1 )
	{
	    u_char	parm_code;
	    u_char	parm_lng = 255;			/* worst case */
	    char	*parm;

	    /*
	    ** Read the common header for each parameter.
	    */
	    pci += GCgeti1( pci, &parm_code );
	    if ( pci < mde->p1 )  pci += GCgeti1( pci, &parm_lng );
	    parm = pci;
	    pci += parm_lng;
	    if ( pci > mde->p1 )  break;		/* pci overrun */
	
	    /*
	    ** Process each parameter.
	    */
	    switch( parm_code )
	    {
		case TDR_SEC_RSN :
		    if ( parm_lng >= 4 )
			GCgeti4( parm, &dr_pci.sec_rsn.sec_rsn );
		    break;

		case TCC_CG_TSAP :	/* Ignored */
		case TCC_CD_TSAP :
		default :
		    break;
	    }
	}

	/*
	** Make sure we haven't overrun the pci.
	*/
	if ( pci > mde->p1 )
	{
	    tmp_gnrc_status = E_GC281A_TL_INV_TPDU_DATA;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( dr_pci.tpdu );
	    erlist.gcc_erparm[0].value = (PTR)&dr_pci.tpdu;
	    gcc_tl_abort( mde, ccb, &tmp_gnrc_status, NULL, &erlist );
	    return( FAIL );
	}

	/*
	** Construct the T_DISCONNECT SDU in a new MDE and send the SDU to SL.
	*/
	if ( ! (tdi_mde = (GCC_MDE *)gcc_add_mde( ccb, 0 )) )
	{
	    /* 
            ** Unable to allocate an MDE. Log an error and abort
	    ** the connection.
	    */
	    tmp_gnrc_status = E_GC2004_ALLOCN_FAIL;
	    gcc_tl_abort( mde, ccb, &tmp_gnrc_status, NULL, NULL );
	    return( FAIL );
	}

	tdi_mde->ccb = ccb;
	tdi_mde->mde_generic_status[0] = dr_pci.sec_rsn.sec_rsn;
	mde = tdi_mde;
	mde->service_primitive = T_DISCONNECT;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OTTDR */
    case OTTDI:
    {
	GCC_MDE	*tdi_mde;
	STATUS	tmp_gnrc_status;
	i4	i;

	/*
	** Send a T_DISCONNECT indication to SL.  
	** Construct the T_DISCONNECT SDU in a 
	** new MDE Send the SDU to SL.
	*/
	if ( ! (tdi_mde = gcc_add_mde( ccb, 0 )) )
	{
	    /* 
            ** Unable to allocate an MDE. Log an error and abort
	    ** the connection.
	    */
	    tmp_gnrc_status = E_GC2004_ALLOCN_FAIL;
	    gcc_tl_abort( mde, ccb, &tmp_gnrc_status, NULL, NULL );
	    return( FAIL );
	}

	tdi_mde->ccb = ccb;
	for( 
	     i = 0;
	     i < GCC_L_GEN_STAT_MDE  &&  mde->mde_generic_status[i] != OK;
	     i++
	   )
	    tdi_mde->mde_generic_status[i] = mde->mde_generic_status[i];

	mde = tdi_mde;
	mde->service_primitive = T_DISCONNECT;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OTTDI */
    case OTTDA:
    {
	/*
	** A normal data TPDU was received.
	** Send a T_DATA indication to SL.
	*/
	GCC_ERLIST	erlist;
	STATUS		tmp_gnrc_status;
	TL_DT_PCI	dt_pci;

	/*
	** Get the address of the pci and strip it out.
	*/
	if ( (mde->p2 - mde->p1) < SZ_TL_DT_PCI )
	{
	    tmp_gnrc_status = E_GC281A_TL_INV_TPDU_DATA;
	    dt_pci.tpdu = TDT_TPDU;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( dt_pci.tpdu );
	    erlist.gcc_erparm[0].value = (PTR)&dt_pci.tpdu;
	    gcc_tl_abort( mde, ccb, &tmp_gnrc_status, NULL, &erlist );
	    return( FAIL );
	}

        mde->p1 += GCgeti1( mde->p1, &dt_pci.li );
	pci = mde->p1;
	mde->p1 += min( dt_pci.li, mde->p2 - mde->p1 );

	/*
        ** Get the fixed portion of the normal data pci.
	** The TPDU type was validated in gcc_tl_evinp() 
	** to generate ITDT input, as was the destination 
	** reference.  The end-of-transmission flag is
	** ignored.
	*/
	pci += GCgeti1( pci, &dt_pci.tpdu );
	pci += GCgeti2( pci, &dt_pci.dst_ref );
	pci += GCgeti1( pci, &dt_pci.nr_eot );

	/*
	** Read optional parameters of CC TPDU (no
	** optional parameters are expected, so we 
	** currently ignore all optional parameters).
	*/
	while( pci < mde->p1 )
	{
	    u_char	parm_code;
	    u_char	parm_lng = 255;			/* worst case */
	    char	*parm;

	    /*
	    ** Read the common header for each parameter.
	    */
	    pci += GCgeti1( pci, &parm_code );
	    if ( pci < mde->p1 )  pci += GCgeti1( pci, &parm_lng );
	    parm = pci;
	    pci += parm_lng;
	    if ( pci > mde->p1 )  break;		/* pci overrun */

	    /*
	    ** Process each parameter.
	    */
	    switch( parm_code )
	    {
		default : /* Ignored */
		    break;
	    }
	}

	/*
	** Make sure we haven't overrun the pci.
	*/
	if ( pci > mde->p1 )
	{
	    tmp_gnrc_status = E_GC281A_TL_INV_TPDU_DATA;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( dt_pci.tpdu );
	    erlist.gcc_erparm[0].value = (PTR)&dt_pci.tpdu;
	    gcc_tl_abort( mde, ccb, &tmp_gnrc_status, NULL, &erlist );
	    return( FAIL );
	}

	/*
	** Construct the T_DATA SDU in the MDE and send the SDU to SL
	*/
	mde->service_primitive = T_DATA;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OTTDA */
    case OTCR:
    {
	TL_CR_PCI	cr_pci;
	i4		i;
	u_char		mech_ids[ GCS_MECH_MAX ];
	u_char		mech_count;
	char		*mech = ccb->ccb_hdr.emech ? ccb->ccb_hdr.emech
						   : IIGCc_global->gcc_ob_mech;

	/*
	** Determine number of possible encryption mechanisms.
	*/
	mech_count = ( ccb->ccb_hdr.emode == GCC_ENCRYPT_OFF )
		     ? 0 : gcc_gcs_mechanisms( mech, mech_ids );
	
	/*
	** Send a CR (Connection Request) TPDU to the peer TL.
	** Construct the CR TPDU in the MDE.  The MDE should be
	** empty at this point.  Reset mde->p1 at mde->ptpdu to
	** utilize the maximum buffer size for constructing the
	** CR TPDU with protection parameters.
	*/
	cr_pci.li = SZ_TL_CR_PCI;
	if ( mech_count )  cr_pci.li += SZ_CR_PROT_PARM + mech_count;
	mde->p1 = pci = mde->ptpdu;
	mde->p2 = mde->p1 + cr_pci.li;

	/*
	** Set the fixed portion of the connection request pci.
	*/
	cr_pci.li -= SZ_TL_li;		/* Length indicator not included */
	pci += GCaddn1( cr_pci.li, pci );
    	pci += GCaddn1( TCR_TPDU, pci );
	pci += GCaddn2( 0, pci );
	ccb->ccb_tce.src_ref = ccb->ccb_hdr.lcl_cei;
	pci += GCaddn2( ccb->ccb_tce.src_ref, pci );
	pci += GCaddn1( TCR_CLS_OPT, pci );
 
	/*
	** Set the variable portion of the connection request pci.
	** Older Comm Servers expect the first three parameters to
	** be the calling TSAP, called TSAP and TPDU size.  Some
	** older Comm Servers also expect the version number to be
	** the fourth parameter.  Newer Comm Servers permit the 
	** parameters in any order and will ignore parameters they 
	** do not recognize (as long as the length indicators are 
	** correct).
	*/
	pci += GCaddn1( TCR_CG_TSAP, pci );	/* Set cg_tsap_id structure */
	pci += GCaddn1( TCR_L_CG_TSAP, pci );
	pci += GCaddn2( 0, pci );

	pci += GCaddn1( TCR_CD_TSAP, pci ); 	/* Set cd_tsap_id structure */
	pci += GCaddn1( TCR_L_CD_TSAP, pci );
	pci += GCaddn2( 0, pci );

	gcc_tpdu_size( ccb, TPDU_32K_SZ );	/* Set tpdu_size structure */
	pci += GCaddn1( TCR_SZ_TPDU, pci );
	pci += GCaddn1( TCR_L_SZ_TPDU, pci );
	pci += GCaddn1( ccb->ccb_tce.tpdu_size, pci );

	pci += GCaddn1( TCR_VERSION_NO, pci );	/* Set version_no structure */
	pci += GCaddn1( TCR_L_VRSN_NO, pci );
	pci += GCaddn1( TCR_VN_2, pci );

	pci += GCaddn1( TCR_SESS_MASK, pci );	/* Set session mask structure */
	pci += GCaddn1( TCR_L_SESS_MASK, pci );

	for( i = 0; i < TCR_L_SESS_MASK; i++ )
	{
	    ccb->ccb_hdr.mask[i] = (u_i1)(MHrand() * 256);
	    pci += GCaddn1( ccb->ccb_hdr.mask[i], pci );
	}

	ccb->ccb_hdr.mask_len = TCR_L_SESS_MASK;

	if ( mech_count )
	{
	    /* 
	    ** Set protection structure
	    */
	    cr_pci.protection_parm.options = 0;
	    if ( ccb->ccb_hdr.emode != GCC_ENCRYPT_OPT )
		cr_pci.protection_parm.options |= TCR_ENCRYPT_REQUEST;

	    pci += GCaddn1( TCR_PROTECT_PARM, pci );
	    pci += GCaddn1( 1 + mech_count, pci );
	    pci += GCaddn1( cr_pci.protection_parm.options, pci );
	    for( i = 0; i < mech_count; i++ )
		pci += GCaddn1( mech_ids[ i ], pci );

	    /* 
	    ** Set the user data area of the MDE with the 
	    ** initial tokens for the encryption mechanisms.
	    */
	    gcc_rqst_encrypt( mde, ccb, mech_count, mech_ids );
	}

    	mde->mde_srv_parms.tl_parms.ts_flags = TS_FLUSH;
	layer_id = DOWN;
	break;
    } /* end case OTCR */
    case OTCC:
    {
	TL_CC_PCI	cc_pci;
	STATUS		tmp_gnrc_status;

	/*
	** Send a CC (Connection Confirm) TPDU to the peer TL.
	** Construct the CC TPDU in the MDE.  The MDE should be
	** empty at this point.  Reset mde->p1 at mde->ptpdu to
	** utilize the maximum buffer size for constructing the
	** CC TPDU with protection parameters.
	*/
	cc_pci.li = SZ_TL_CC_PCI;
	if ( ccb->ccb_hdr.mask_len > 0 )
	    cc_pci.li += SZ_SESS_MASK;
	if ( ccb->ccb_tce.t_flags & GCCTL_ENCRYPT )
	    cc_pci.li += SZ_CC_PROT_PARM;
	mde->p1 = pci = mde->ptpdu;
	mde->p2 = mde->p1 + cc_pci.li;

	/*
	** Set the fixed portion of the connection confirm pci.
	*/
	cc_pci.li -= SZ_TL_li;		/* Length indicator not included */
	pci += GCaddn1( cc_pci.li, pci );
	pci += GCaddn1( TCC_TPDU, pci );
	pci += GCaddn2( ccb->ccb_tce.dst_ref, pci );
	pci += GCaddn2( ccb->ccb_tce.src_ref, pci );
	pci += GCaddn1( TCC_CLS_OPT, pci );

	/*
	** Set the variable portion of the connection confirm pci.
	** Older Comm Servers expect the first four parameters to
	** be the calling TSAP, called TSAP, TPDU size and version.
	** Newer Comm Servers permit the parameters in any order
	** and will ignore parameters they do not recognize (as
	** long as the length indicators are correct).
	*/
	pci += GCaddn1( TCC_CG_TSAP, pci );	/* Set cg_tsap_id structure */
	pci += GCaddn1( TCC_L_CG_TSAP, pci );
	pci += GCaddn2( 0, pci );

	pci += GCaddn1( TCC_CD_TSAP, pci );	/* Set cd_tsap_id structure */
	pci += GCaddn1( TCC_L_CD_TSAP, pci );
	pci += GCaddn2( 0, pci );

	pci += GCaddn1( TCC_SZ_TPDU, pci );	/* Set tpdu_size structure */
	pci += GCaddn1( TCC_L_SZ_TPDU, pci );
	pci += GCaddn1( ccb->ccb_tce.tpdu_size, pci );

	pci += GCaddn1( TCC_VERSION_NO, pci );	/* Set version_no structure */
	pci += GCaddn1( TCC_L_VRSN_NO, pci );
	pci += GCaddn1( ccb->ccb_tce.version_no, pci );

	if ( ccb->ccb_hdr.mask_len > 0 )	/* Set session mask structure */
	{
	    i4 index;
	    u_i1 val;

	    pci += GCaddn1( TCC_SESS_MASK, pci );
	    pci += GCaddn1( ccb->ccb_hdr.mask_len, pci );

	    /*
	    ** Generate our own session mask and send to partner.
	    ** Our mask is combined with partner mask for actual use.
	    */
	    for( index = 0; index < ccb->ccb_hdr.mask_len; index++ )
	    {
		val = (u_i1)(MHrand() * 256);
		pci += GCaddn1( val, pci );
		ccb->ccb_hdr.mask[ index ] ^= val;
	    }
	}

	if ( ccb->ccb_tce.t_flags & GCCTL_ENCRYPT )
	{
	    /*
	    ** Set protection structure.
	    */
	    pci += GCaddn1( TCC_PROTECT_PARM, pci );
	    pci += GCaddn1( TCC_L_PROTECT, pci );
	    pci += GCaddn1( ccb->ccb_tce.mech_id, pci );

	    /*
	    ** Set the user data area of the MDE with 
	    ** the confirmation token for the selected 
	    ** encryption mechanism.
	    */
 	    gcc_rsp_encrypt( mde, ccb );
	}

	/*
	** Shut down the connection if encryption is required
	** but could not be enabled (for whatever reason).
	*/
	if ( IIGCc_global->gcc_ib_mode == GCC_ENCRYPT_REQ  &&
	     ! (ccb->ccb_tce.t_flags & GCCTL_ENCRYPT ) )
	{
	    /*
	    ** Send a T_DISCONNECT request to ourselves in a
	    ** new MDE to shut down the connection.  Continue
	    ** with standard connection processing until the
	    ** disconnect request can be processed.
	    */
	    tmp_gnrc_status = E_GC281C_TL_NO_ENCRYPTION;
	    gcc_tl_shut( mde, ccb, &tmp_gnrc_status, NULL, NULL );
	}

    	mde->mde_srv_parms.tl_parms.ts_flags = TS_FLUSH;
    	layer_id = DOWN;
	break;
    } /* end case OTCC */
    case OTDR:
    {
	TL_DR_PCI	dr_pci;

	/*
	** Send a DR (Disconnect Request) TPDU to the 
	** peer TL.  Construct the DR TPDU in the MDE.
	*/
	dr_pci.li = SZ_TL_DR_PCI;
	if ( mde->mde_generic_status[0] != OK )  dr_pci.li += SZ_TL_DR_PARM;
	pci = (mde->p1 -= dr_pci.li);

	/*
	** Set the fixed portion of the disconnect request pci.
	*/
	dr_pci.li -= SZ_TL_li;		/* Length indicator not included */
	pci += GCaddn1( dr_pci.li, pci );
	pci += GCaddn1( TDR_TPDU, pci );
	pci += GCaddn2( ccb->ccb_tce.dst_ref, pci );
	pci += GCaddn2( ccb->ccb_tce.src_ref, pci );
	pci += GCaddn1( TDR_NORMAL, pci );

	/*
	** Set the variable portion of the disconnect 
	** request pci.  Older Comm Servers validated 
	** the TPDU type and destination reference in
	** the fixed portion of the pci, but ignored 
	** everything else.  Newer Comm Servers will
	** Look for the variable section of the pci.
	*/
	if ( mde->mde_generic_status[0] != OK )
	{
	    pci += GCaddn1( TDR_SEC_RSN, pci );
	    pci += GCaddn1( 4, pci );
	    pci += GCaddn4( mde->mde_generic_status[0], pci );
	}

    	mde->mde_srv_parms.tl_parms.ts_flags = TS_FLUSH;
	layer_id = DOWN;
	break;
    } /* end case OTDR */
    case OTDC:
    {
	/*
	** Send a DC (Disconnect Confirm) TPDU to the peer TL.
	** Construct the DC TPDU in the MDE.
	*/
	pci = (mde->p1 -= SZ_TL_DC_PCI); 
	pci += GCaddn1( SZ_TL_DC_PCI - SZ_TL_li, pci );
	pci += GCaddn1( TDC_TPDU, pci );
	pci += GCaddn2( ccb->ccb_tce.dst_ref, pci );
	pci += GCaddn2( ccb->ccb_tce.src_ref, pci );
    	mde->mde_srv_parms.tl_parms.ts_flags = TS_FLUSH;
	layer_id = DOWN;
	break;
    } /* end case OTDC */
    case OTDT:
    {
	/*
	** Send a DT (Data) TPDU to the peer TL. 
	** Construct the DT TPDU in the MDE 
	*/
	pci = (mde->p1 -= SZ_TL_DT_PCI);
	pci += GCaddn1( SZ_TL_DT_PCI - SZ_TL_li, pci );
	pci += GCaddn1( TDT_TPDU, pci );
	pci += GCaddn2( ccb->ccb_tce.dst_ref, pci );
	pci += GCaddn1( TDT_EOT_MASK, pci );
	layer_id = DOWN;
	break;
    } /* end case OTDT */
    case OTXOF:
    {
    	GCC_MDE		*xof_mde;
	STATUS		tmp_gnrc_status;

	/*
	** Send a T_XOFF indication to SL
	** Set the "Sent XOF" indicator in the GCC TL flags.
	*/
	if ( ! (xof_mde = gcc_add_mde( ccb, 0 )) )
	{
	    /* 
            ** Unable to allocate an MDE. Log an error and abort
	    ** the connection.
	    */
	    tmp_gnrc_status = E_GC2004_ALLOCN_FAIL;
	    gcc_tl_abort( xof_mde, ccb, &tmp_gnrc_status, NULL, NULL );
	    return( FAIL );
	}

	ccb->ccb_tce.t_flags |= GCCTL_SXOF;
	xof_mde->ccb = ccb;
    	mde = xof_mde;
	mde->service_primitive = T_XOFF;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OTXOF */
    case OTXON:
    {
    	GCC_MDE		*xon_mde;
	STATUS		tmp_gnrc_status;

	/*
	** Send a T_XON indication to SL.  
	** Reset the "Sent XOF" indicator in the GCC TL flags.
	*/
	if ( ! (xon_mde = gcc_add_mde( ccb, 0 )) )
	{
	    /* 
            ** Unable to allocate an MDE. Log an error and abort
	    ** the connection.
	    */
	    tmp_gnrc_status = E_GC2004_ALLOCN_FAIL;
	    gcc_tl_abort( xon_mde, ccb, &tmp_gnrc_status, NULL, NULL );
	    return( FAIL );
	}

	ccb->ccb_tce.t_flags &= ~GCCTL_SXOF;
	xon_mde->ccb = ccb;
    	mde = xon_mde;
	mde->service_primitive = T_XON;
	mde->primitive_type = IND;
	layer_id = UP;
	break;
    } /* end case OTXON */
    default:
    {
	STATUS		tmp_gnrc_status;

	/*
	** Unknown event ID.  This is a program bug. Log an error
	** and abort the connection.
	*/
	tmp_gnrc_status = E_GC2805_TL_INTERNAL_ERROR;
	gcc_tl_abort( mde, ccb, &tmp_gnrc_status, NULL, NULL );
	return( FAIL );
    }
    } /* end switch */

# ifdef GCXDEBUG
    if ( IIGCc_global->gcc_trace_level >= 2  &&  layer_id == UP )
    {
	TRdisplay( "gcc_tlout_exec: service_primitive %s %s\n",
		   gcx_look( gcx_mde_service, mde->service_primitive ),
		   gcx_look( gcx_mde_primitive, mde->primitive_type ) );
    }
# endif /* GCXDEBUG */

    /*
    ** Determine whether this output event goes to layer N+1 or	N-1 and send it.
    */
    switch (layer_id)
    {
	case UP:	/* Send to layer N+1 */
	    status = gcc_sl(mde);
	    break;

	case DOWN:	/* Send to layer N-1 */
	    /*
	    ** TL has no N-1 layer.  Output events designated "DOWN" 
	    ** are bound for the network, and no action is required.  
	    ** They will presumably be sent by an action routine.  
	    ** If we had a network layer, they would be sent down 
	    ** at this point.
	    */
	
	    break;

	default:
	    break;
    } /* end switch */

    return( status );
} /* gcc_tlout_exec */


/*
** Name: gcc_tlactn_exec	- TL FSM action execution routine
**
** Description:
**      gcc_tlactn_exec accepts as input an action identifier.  It
**      executes the action correspoinding to the identifier and returns.
**
**	It also accepts as input a pointer to an 
**	MDE or NULL.  If the pointer is non-null, the MDE pointed to is
**	used for the action.  Otherwise, a new one is obtained.
** 
**      All actions executed by this routine are invocations of services from 
**      the appropriate protocol handler.  The protocol handler is identified 
**      by the index into the PCT contained in the CCB for this connection. 
**
** Inputs:
**	action_code		Identifier of the action to be performed
**      mde			Pointer to dequeued Message Data Element
**	ccb			Pointer to the CCB.
**
**	Returns:
**	    STATUS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      25-Jan-88 (jbowers)
**          Initial function implementation
**	14-jun-89 (seiwald)
**	    Change E_GC2803_PROT_ID to a parameterless 2804 for handing up 
**	    to AL.  There is no mechanism for messages with parameters.
**      06-Sep-89 (cmorris)
**          Don't free the CCB until you've finished referencing it.
**      13-Dec-89 (cmorris)
**          Fixed up error handling in ATCNC case.
**	28-May-90 (seiwald)
**	    Initialize open.lsn_port when issuing GCC_OPEN for old protocol
**	    drivers which don't know to return the actual listen port.
**      27-Dec-90 (seiwald)
**          Added needed parameters to E_GC2813_NTWK_LSN_FAIL message.
**	16-Jul-91 (cmorris)
**	    Return FAIL status if event reintroduced into protocol
**	    machine. This prevents the possibility of further actions 
**          being executed.
**      30-Jul-91 (cmorris)
**	    If protocol driver request fails with error, fetch status
**	    out of parameter list for logging purposes.
**	18-oct-91 (seiwald) bug #40468
**	    Don't return failure after reintroducing an event unless
**	    an abort is generated.
**      2-apr-92 (fraser)
**          Removed ifdef PMFE which inhibited the search of GCC_PCT for
**          the entry with the specified protocol id.
**  	04-Jan-93 (cmorris)
**  	    Fetch Network Address from CCB rather than from MDE.
**	09-Jun-97 (rajus01)
**	    Added new action ATSDP for sending encrypted data and
**	    to synchronize the encrypted sends and receives between
**	    the client and server.
**	20-Aug-97 (gordy)
**	    Combined gcc_tsnd(), gcc_e_tsnd() into single routine, 
**	    gcc_tl_send().
**	22-Oct-97 (rajus01)
**	    Extra buffer space is provided to receive the encrypted 
**	    data.( mde->pxtend - mde->p1 ).
**	 6-Jun-03 (gordy)
**	    Protocol information now saved in PIB.  Remote target address
**	    moved to trg_addr.  The target address of a LISTEN request is
**	    our own listen address.
**	 5-Aug-04 (gordy)
**	    If requested protocol is "DEFAULT", use first active protocol.
*/

static STATUS
gcc_tlactn_exec( u_i4 action_code, GCC_MDE *mde, GCC_CCB *ccb )
{
    GCC_P_PLIST		*p_list;
    STATUS		status = OK;
    GCC_PCE		*pce;
    u_i4		i;

    GCX_TRACER_1_MACRO("gcc_tlactn_exec>", mde,
	    "action = %d", action_code);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "gcc_tlactn_exec: action %s\n",
		gcx_look( gcx_gcc_atl, action_code ) );
# endif /* GCXDEBUG */

    switch (action_code)
    {
    case ATCNC:
    {
	char *n_sel = ccb->ccb_hdr.trg_addr.n_sel;

	/*
	** Invoke the CONNECT function from the appropriate protocol handler.
	*/

	CVupper( n_sel );

	/*
	** First, find the entry in the PCT corresponding to the protocol
	** specified in the T_CONNECT service primitive.  Then construct an
	** N_CONNECT confirm in the MDE.
	*/

	if ( ! STcompare( n_sel, ERx("DEFAULT") ) )
	{
	    /* Find first active entry */
	    for( i = 0; i < IIGCc_global->gcc_pct->pct_n_entries; i++ )
	    {
		pce = &IIGCc_global->gcc_pct->pct_pce[i];
		if ( ! (pce->pce_flags & PCT_OPN_FAIL) )  break;
	    }
	}
	else
	{
	    /* search for entry */
	    for( i = 0; i < IIGCc_global->gcc_pct->pct_n_entries; i++ )
	    {
		pce = &IIGCc_global->gcc_pct->pct_pce[i];
		if ( ! STcompare( n_sel, pce->pce_pid ) )  break;
	    }
	}

	if ( i == IIGCc_global->gcc_pct->pct_n_entries )
	{
	    GCC_ERLIST		erlist;
	    STATUS		temp_status;
		
	    /*
	    ** The specified protocol id was not found in the PCT. 
            ** Generate an N_RESET to indicate this. Log an error
	    ** containing 1 parm: the protocol id.
	    */

	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].value = ccb->ccb_hdr.trg_addr.n_sel;
	    erlist.gcc_erparm[0].size = 
		STlength(erlist.gcc_erparm[0].value);
	    temp_status = E_GC2803_PROT_ID;
	    gcc_er_log(&temp_status, (CL_ERR_DESC *)NULL, mde, &erlist);
 	    mde->service_primitive = N_RESET;
            mde->primitive_type = IND;
            mde->mde_generic_status[0] = E_GC2804_PROT_ID;
            mde->mde_generic_status[1] = OK;
            status = gcc_tl(mde);
	} 
	else if ( pce->pce_flags & PCT_OPN_FAIL )
	{
	    GCC_ERLIST		erlist;
	    STATUS		temp_status;
		
	    /*
	    ** The user attempted an outgoing connection on a protocol
	    ** which failed to open.
            ** Generate an N_RESET to indicate this. Log an error
	    ** containing 1 parm: the protocol id.
	    */

	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].value = ccb->ccb_hdr.trg_addr.n_sel;
	    erlist.gcc_erparm[0].size = 
		STlength(erlist.gcc_erparm[0].value);
	    temp_status = E_GC2816_NOT_OPEN;
	    gcc_er_log(&temp_status, (CL_ERR_DESC *)NULL, mde, &erlist);
 	    mde->service_primitive = N_RESET;
            mde->primitive_type = IND;
            mde->mde_generic_status[0] = E_GC2817_NOT_OPEN;
            mde->mde_generic_status[1] = OK;
            status = gcc_tl(mde);
	}
	else
	{
	    /* We have a winner!  Invoke CONNECT from the protocol handler  */

	    ccb->ccb_tce.prot_pce = pce;
	    mde->service_primitive = N_CONNECT;
	    mde->primitive_type = CNF;

	    p_list = &mde->mde_act_parms.p_plist;
	    p_list->compl_exit = gcc_tl_exit;
	    p_list->compl_id = (PTR)mde;
	    p_list->pcb = NULL;
	    p_list->pce = pce;
	    p_list->buffer_ptr = mde->p1;
	    p_list->buffer_lng = mde->p2 - mde->p1;
	    p_list->options = 0;
	    p_list->function_invoked = (i4) GCC_CONNECT;
	    p_list->function_parms.connect.node_id = 
    	    	ccb->ccb_hdr.trg_addr.node_id;
	    p_list->function_parms.connect.port_id =
    	    	ccb->ccb_hdr.trg_addr.port_id;
	    p_list->function_parms.connect.lcl_addr = NULL;
	    p_list->function_parms.connect.rmt_addr = NULL;

	    status = (*pce->pce_protocol_rtne)(GCC_CONNECT, p_list);
	    if (status != OK)
	    {
		GCC_ERLIST		erlist;
		
		/*
		** Log the error.  The message contains 3 parms: the protocol
		** id, the node and the listen port.
		*/

		erlist.gcc_parm_cnt = 3;
		erlist.gcc_erparm[0].value = ccb->ccb_hdr.trg_addr.n_sel;
		erlist.gcc_erparm[1].value = ccb->ccb_hdr.trg_addr.node_id;
		erlist.gcc_erparm[2].value = ccb->ccb_hdr.trg_addr.port_id;
		erlist.gcc_erparm[0].size = 
		    STlength(erlist.gcc_erparm[0].value);
		erlist.gcc_erparm[1].size = 
		    STlength(erlist.gcc_erparm[1].value);
		erlist.gcc_erparm[2].size = 
		    STlength(erlist.gcc_erparm[2].value);
		status = E_GC2809_NTWK_CONNECTION_FAIL;
	        gcc_er_log(&status, (CL_ERR_DESC *)NULL, mde, &erlist);

		/* Abort the connection */
                gcc_tl_abort(mde, ccb, &p_list->generic_status, 
		  	     &p_list->system_status, (GCC_ERLIST *) NULL);
	    } /* end if */
	}
        break;
    } /* end case ATCNC */
    case ATDCN:
    {
	/* Invoke the DISCONNECT function from the appropriate protocol handler */
	pce = ccb->ccb_tce.prot_pce;
	mde->service_primitive = N_DISCONNECT;
	mde->primitive_type = CNF;
	p_list = &mde->mde_act_parms.p_plist;
	p_list->compl_exit = gcc_tl_exit;
	p_list->compl_id = (PTR)mde;
	p_list->pcb = ccb->ccb_tce.pcb;
	p_list->pce = pce;
	p_list->buffer_ptr = mde->p1;
	p_list->buffer_lng = mde->p2 - mde->p1;
	p_list->options = 0;
	p_list->function_invoked = (i4) GCC_DISCONNECT;
	status = (*pce->pce_protocol_rtne)(GCC_DISCONNECT, p_list);
	if (status != OK)
	{
	    /* Log an error and abort the connection */
            gcc_tl_abort(mde, ccb, &p_list->generic_status, 
			 &p_list->system_status, (GCC_ERLIST *) NULL);
	}
	break;
    } /* end case ATDCN */
    case ATMDE:
    {
	/*
	** Discard the MDE.
	*/

	gcc_del_mde( mde );
	break;
    } /* end case ATMDE */
    case ATCCB:	/* Return the CCB */
    {
	/*
	** Flag the CCB for release.  The actual
	** release will occur in gcc_tl() when
	** all processing has been completed.
	*/
	ccb->ccb_tce.t_flags |= GCCTL_CCB_RELEASE;
	break;
    } /* end case ATCCB */
    case ATLSN:
    {

	/*
	** Issue LISTEN request to the protocol module.
	** Create a new CCB for the potential connection to be established
	** with the completion of GCC_LISTEN.  Create an MDE to contain
	** the T_CONNECT indication which will result.
	*/

	GCC_MDE		*lsn_mde;
	GCC_CCB		*lsn_ccb;
	GCC_PCE		*pce;
	GCC_PIB		*pib;

	lsn_ccb = gcc_add_ccb();
	if (lsn_ccb == NULL)
	{
	    /* log an error and abort the connection */
	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_tl_abort(mde, lsn_ccb, &status, 
    	    	    	 (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	} /* end if */
	lsn_ccb->ccb_hdr.async_cnt +=1;

	/* Copy listen info out of existing CCB */
	lsn_ccb->ccb_tce.lsn_retry_cnt = ccb->ccb_tce.lsn_retry_cnt;
	pce = lsn_ccb->ccb_tce.prot_pce = ccb->ccb_tce.prot_pce;
	pib = lsn_ccb->ccb_tce.pib = ccb->ccb_tce.pib;
	lsn_ccb->ccb_tce.t_state = STCLD;
	lsn_ccb->ccb_tce.pcb = NULL; 
	STcopy( pib->pid, lsn_ccb->ccb_hdr.trg_addr.n_sel );
	STcopy( pib->host, lsn_ccb->ccb_hdr.trg_addr.node_id );
	STcopy( pib->addr, lsn_ccb->ccb_hdr.trg_addr.port_id );

	lsn_mde = gcc_add_mde( ccb, 0 );
	if (lsn_mde == NULL)
	{
	    /* log an error and abort the connection */
	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_tl_abort(lsn_mde, lsn_ccb, &status, 
    	    	    	 (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	} /* end if */
	lsn_mde->ccb = lsn_ccb;
	lsn_mde->service_primitive = N_CONNECT;
	lsn_mde->primitive_type = IND;
	lsn_mde->p1 = lsn_mde->p2 = lsn_mde->ptpdu;

	p_list = &lsn_mde->mde_act_parms.p_plist;
	p_list->compl_exit = gcc_tl_exit;
	p_list->compl_id = (PTR)lsn_mde;
	p_list->pcb = NULL;
	p_list->pce = pce;
	p_list->options = 0;
	p_list->buffer_ptr = (PTR)lsn_mde->p1;
	p_list->buffer_lng = lsn_mde->pend - lsn_mde->p1;
	p_list->function_invoked = (i4) GCC_LISTEN;
	p_list->function_parms.listen.port_id = pib->port;
	p_list->function_parms.listen.node_id = NULL;
	p_list->function_parms.listen.lcl_addr = NULL;
	p_list->function_parms.listen.rmt_addr = NULL;

	/*
	** Invoke GCC_LISTEN service.  
	*/

	status = (*pce->pce_protocol_rtne)(GCC_LISTEN, p_list);
        if (status != OK)
        {
            GCC_ERLIST          erlist;

            /* Log an error and abort the connection */

            erlist.gcc_parm_cnt = 2;
            erlist.gcc_erparm[0].value = pce->pce_pid;
            erlist.gcc_erparm[0].size = STlength(pce->pce_pid);
            erlist.gcc_erparm[1].value = pib->port;
            erlist.gcc_erparm[1].size = STlength(pib->port);
            status = E_GC2813_NTWK_LSN_FAIL;
	    gcc_er_log(&status, (CL_ERR_DESC *)NULL, mde, &erlist);
            gcc_tl_abort(mde, ccb, &p_list->generic_status, 
			 &p_list->system_status, (GCC_ERLIST *) NULL);
        }
	break;
    } /* end case ATLSN */
    case ATRVN:
    {
	GCC_MDE	    *rvn_mde;
	u_i4	    i;
	
	/*
	** Invoke the RECEIVE function for normal data from the appropriate
	** protocol handler.  The buffer begins at the start of the TL PCI area.
	** This will accommodate the largest PDU which could be sent.  When
	** receiving, a new MDE is obtained, since the old one will be used for
	** other purposes.  The PCT index is in the current MDE, in
	** the srv_parms.
	*/

	rvn_mde = gcc_add_mde( ccb, ccb->ccb_tce.tpdu_size );

	if ( rvn_mde == NULL)
	{
	    /* Unable to allocate an MDE */

	    status = E_GC2004_ALLOCN_FAIL;
	    
	    /* Log an error and abort the connection */
	    gcc_tl_abort(rvn_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	}
	
	pce = ccb->ccb_tce.prot_pce;
	rvn_mde->ccb = ccb; /* use the arg instead of mde->ccb */
	rvn_mde->service_primitive = N_DATA;
	rvn_mde->primitive_type = IND;
	rvn_mde->p1 = rvn_mde->p2 = rvn_mde->ptpdu;
	p_list = &rvn_mde->mde_act_parms.p_plist;
	p_list->options = 0;
	p_list->compl_exit = gcc_tl_exit;
	p_list->compl_id = (PTR)rvn_mde;
	p_list->buffer_ptr = (PTR)rvn_mde->p1;
	p_list->buffer_lng = rvn_mde->pxtend - rvn_mde->p1;
	p_list->pcb = ccb->ccb_tce.pcb;
	p_list->pce = pce;
	p_list->function_invoked = (i4) GCC_RECEIVE;
	status = (*pce->pce_protocol_rtne)(GCC_RECEIVE, p_list);
	if (status != OK)
	{
	    /* Log an error and abort the connection */
	    gcc_tl_abort(rvn_mde, ccb, &p_list->generic_status, 
                         &p_list->system_status, (GCC_ERLIST *) NULL);
	}
	break;
    } /* end case ATRVN */
    case ATSDN:
    {
	/*
	** Invoke the SEND function for normal data from the appropriate
	** protocol handler. Encryption may be turned on during this action.
	*/
	if ( (status = gcc_tl_send( TRUE, mde )) != OK )
	{
	    /* Log an error and abort the connection */
            p_list = &mde->mde_act_parms.p_plist;
	    gcc_tl_abort(mde, ccb, &p_list->generic_status, 
                         &p_list->system_status, (GCC_ERLIST *) NULL);
	}
	break;
    } /* end case ATSDN */
    case ATSDP: /* Send protection Data... */
    {
	/*
	** Invoke the SEND function for protection data from the appropriate
	** protocol handler. Encryption is not turned on at the time when we 
	** perform this action.
	*/
	if ( (status = gcc_tl_send( FALSE, mde )) != OK )
	{
	    /* Log an error and abort the connection */
            p_list = &mde->mde_act_parms.p_plist;
	    gcc_tl_abort(mde, ccb, &p_list->generic_status, 
                         &p_list->system_status, (GCC_ERLIST *) NULL);
	}
	break;
    } /* end case ATSDN */
    case ATOPN:
    {
	/*
	** Issue OPEN request to the protocol module.
	*/
	GCC_PCE	*pce = ccb->ccb_tce.prot_pce;

	mde->service_primitive = N_OPEN;
	mde->primitive_type = CNF;
	p_list = &mde->mde_act_parms.p_plist;
	p_list->compl_exit = gcc_tl_exit;
	p_list->compl_id = (PTR)mde;
	p_list->pcb = NULL;
	p_list->pce = pce;
	p_list->buffer_ptr = NULL;
	p_list->buffer_lng = 0;
	p_list->options = 0;
	p_list->function_invoked = (i4)GCC_OPEN;
	p_list->function_parms.open.port_id = ccb->ccb_tce.pib->addr;
	p_list->function_parms.open.lsn_port = NULL;
	p_list->function_parms.open.actual_port_id = NULL;

	/*
	** Invoke GCC_OPEN service.  
	*/
	status = (*pce->pce_protocol_rtne)(GCC_OPEN, p_list);
	if (status != OK)
	{
    	    GCC_ERLIST	    erlist;

	    /* Log an error and abort the connection */
	    erlist.gcc_parm_cnt = 2;
	    erlist.gcc_erparm[0].value = pce->pce_pid;
	    erlist.gcc_erparm[1].value = ccb->ccb_tce.pib->addr;
	    erlist.gcc_erparm[0].size = 
		STlength(erlist.gcc_erparm[0].value);
	    erlist.gcc_erparm[1].size = 
		STlength(erlist.gcc_erparm[1].value);
	    status = E_GC2808_NTWK_OPEN_FAIL;
	    gcc_er_log(&status, (CL_ERR_DESC *)NULL, 
    	    	       (GCC_MDE *) NULL, &erlist);
	    gcc_tl_abort(mde, ccb, &p_list->generic_status, 
                         &p_list->system_status, (GCC_ERLIST *) NULL);
	}
	break;
    } /* end case ATOPN */
    case ATSNQ:
    {
	/*
	** Enqueue the MDE containing a normal send request.  A send is
	** currently in progess with completion pending.  This MDE will be sent
	** when its turn comes.
	*/

	QUinsert( (QUEUE *)mde, ccb->ccb_tce.t_snd_q.q_prev );
	ccb->ccb_tce.t_sq_cnt +=1;
	break;
    } /* end case ATSNQ */
    case ATSDQ:
    {
	GCC_MDE		*sdq_mde;

	/*
	** Dequeue and send an enqueued MDE.
	*/

	sdq_mde = (GCC_MDE *)QUremove(ccb->ccb_tce.t_snd_q.q_next);
	ccb->ccb_tce.t_sq_cnt -=1;
	sdq_mde->mde_q_link.q_next = NULL;
	sdq_mde->mde_q_link.q_prev = NULL;

	/*
	** Invoke the SEND function for normal data from the appropriate
	** protocol handler.
	*/
	if ( (status = gcc_tl_send( TRUE, sdq_mde )) != OK )
	{
	    /* Log an error and abort the connection */
            p_list = &mde->mde_act_parms.p_plist;
	    gcc_tl_abort(sdq_mde, ccb, &p_list->generic_status, 
                         &p_list->system_status, (GCC_ERLIST *) NULL);
	}
	break;
    } /* end case ATSDQ */
    case ATPGQ:
    {
	/*
	** Purge the send queue in the CCB.  Presumably we are aborting the
	** connection and are going to simply throw away any enqueued messages.
	*/

	GCC_MDE	    *purg_mde;

	while ((purg_mde =
	     (GCC_MDE *) QUremove((QUEUE *) ccb->ccb_tce.t_snd_q.q_next))
		!= NULL)
	{
	    /*
	    ** Return the MDE to the scrapyard
	    */

	    gcc_del_mde( purg_mde );
	} /* end while */
	break;
    } /* end case ATPGQ */
    case ATSXF:
    {
	/*
	** Set the "Received XOF" indicator in the GCC TL flags.
	*/

	ccb->ccb_tce.t_flags |= GCCTL_RXOF;
	break;
    } /* end case ATSXF */
    case ATRXF:
    {
	/*
	** Reset the "Received XOF" indicator in the GCC TL flags.
	*/

	ccb->ccb_tce.t_flags &= ~GCCTL_RXOF;
	break;
    } /* end case ATSXF */
    default:
	{
	    status = E_GC2805_TL_INTERNAL_ERROR;

	    /* Log an error and abort the connection */
	    gcc_tl_abort(mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	}
    } /* end switch */
    return(status);
} /* gcc_tlactn_exec */


/*
** Name: gcc_tl_send
**
** Description:
**	Perform optional encryption on a message and send the
**	message using the appropriate protocol.
**
** Inputs:
**	encrypt		TRUE if encryption may be performed, FALSE otherwise.
**      mde             Message Data Element.
**
** Outputs:
**      none
**
** Returns:
**	STATUS		OK or FAIL.
**
** History:
**      30-Jun-97 (rajus01)
**          Created
**	20-Aug-97 (gordy)
**	    Combined gcc_tsnd(), gcc_e_tsnd() into (this) single routine;
**	22-Oct-97 (rajus01)
**	    Extra buffer space is provided to perform encryption. 
**	    ( mde->pxtend - mde->p1 ).
*/

static STATUS
gcc_tl_send( bool encrypt, GCC_MDE *mde )
{
    STATUS status = OK; 

# ifdef GCXDEBUG
    if ( IIGCc_global->gcc_trace_level >= 3 )
    {
	TRdisplay( "gcc_tl_send: sending %d bytes\n", 
		   (i4)(mde->p2 - mde->p1) );
	gcx_tdump( mde->p1, (i4)(mde->p2 - mde->p1) );
    }
# endif /* GCXDEBUG */

    if ( encrypt  && (mde->ccb->ccb_tce.t_flags & GCCTL_ENCRYPT) )
    {
	GCS_EENC_PARM	enc_parm;
	STATUS		gcs_status;

	enc_parm.escb   = mde->ccb->ccb_tce.gcs_cb;
	enc_parm.size   = (i4)(mde->pxtend - mde->p1);
	enc_parm.length = (i4)(mde->p2 - mde->p1);
	enc_parm.buffer = mde->p1;

	gcs_status = IIgcs_call( GCS_OP_E_ENCODE,
				 mde->ccb->ccb_tce.mech_id, (PTR)&enc_parm );

	if ( gcs_status == OK )
	{
	    /* Reset the pend to p2.
	    */
	    if( mde->pend < mde->p2 )
		mde->pend = mde->p2;

	    mde->p2 = mde->p1 + enc_parm.length;

# ifdef GCXDEBUG
	    if( IIGCc_global->gcc_trace_level >= 4 )
	    {
		TRdisplay( "gcc_tl_send: sending %d encrypted bytes\n",
			   (i4)(mde->p2 - mde->p1) );
		gcx_tdump( mde->p1, (i4)(mde->p2 - mde->p1) );
	    }
#endif /*GCXDEBUG */
	}
	else
	{
	    /* This will be considered internal abort */
	    status = E_GC281D_TL_ENCRYPTION_FAIL;
	    gcc_tl_abort(mde, mde->ccb, &status, NULL, NULL );
	}
    }

    if ( status == OK )
    {
	GCC_PCE		*pce = mde->ccb->ccb_tce.prot_pce;
	GCC_P_PLIST	*p_list = &mde->mde_act_parms.p_plist;

	mde->service_primitive = N_DATA;
	mde->primitive_type = CNF;
	p_list->function_invoked = (i4)GCC_SEND;
	p_list->compl_exit = gcc_tl_exit;
	p_list->compl_id = (PTR)mde;
	p_list->buffer_ptr = mde->p1;
	p_list->buffer_lng = mde->p2 - mde->p1;
	p_list->pcb = mde->ccb->ccb_tce.pcb;
	p_list->pce = pce;
	if ( mde->mde_srv_parms.tl_parms.ts_flags & TS_FLUSH )
	    p_list->options = GCC_FLUSH_DATA;

	status = (*pce->pce_protocol_rtne)(GCC_SEND, p_list);
    }

    return( status );
} /* gcc_tl_send */


/*
** Name: gcc_tpdu_size: negotiate and set tpdu max size
**
** Description:
**	Compute the tpdu max size as a minimum of what is requested
**	(presumably by the peer TL) and what the protocol driver
**	table indicates the protocol will support.
**
**	Note that pre 6.4 was bit broken:
**	    CR req: pci.tpdu_size = TPDU_1K_SZ
**	    CR ind: ccb.tpdu_size = 1<<min( pci.tpdu_size, TPDU_1K_SZ )
**	    CC req: pci.tpdu_size = ccb.tpdu_size(==0(!))
**	    CC ind: ccb.tpdu_size = min( 1<<pci.tpdu_size, ccb.tpdu_size(==0)) 
**
**	Now we do this:
**	    CR req: pci.tpdu_size = min( TPDU_8K_SZ, log2(protocol_size)
**	    CR ind: ccb.tpdu_size = min( pci.tpdu_size, log2(protocol_size) )
**	    CC req: pci.tpdu_size = ccb.tpdu_size
**	    CC ind: ccb.tpdu_size = min( pci.tpdu_size, log2(protocol_size) )
**
**      This routine computes the log2(protocol_size), the min, and sets
**	the result into the ccb.
**
** History:
**	09-Dec-89 (seiwald)
**	    Written.
**	30-Aug-90 (sweeney)
**	    Print the TPDU size, not its log2.
*/

static VOID							 
gcc_tpdu_size( GCC_CCB *ccb, u_char max_tpdu_size )
{
    u_char 	i = 0;
    i4	size = ccb->ccb_tce.prot_pce->pce_pkt_sz;

    while( size >>= 1 )  i++;		/* i = log2(size) */

    ccb->ccb_tce.tpdu_size = min( i, max_tpdu_size );

# ifdef GCXDEBUG
    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay("gcc_tpdu_size = %d\n", (i4)(1 << ccb->ccb_tce.tpdu_size));
# endif /* GCXDEBUG */

    return;
} /* gcc_tpdu_size */


/*
** Name: gcc_tl_abort	- Transport Layer internal abort
**
** Description:
**	gcc_tl_abort handles the processing of internally generated aborts.
** 
** Inputs:
**      mde		    Pointer to mde
**      ccb		    Pointer to ccb
**	generic_status      Mainline status code
**      system_status       System specfic status code
**      erlist		    Error parameter list
**
** Outputs:
**      none
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-Mar-90 (cmorris)
**          Initial routine creation
**	09-May-91 (cmorris)
**	    Handle orphaned MDE's 
**      16-Jul-91 (cmorris)
**	    Fixed system status argument to gcc_tl call
**  	15-Jan-93 (cmorris)
**  	    Always reset p1, p2 before processing internal abort.
**  	    The pointers could be set to anything when the abort
**  	    occurred.
*/

VOID
gcc_tl_abort( GCC_MDE *mde, GCC_CCB *ccb, STATUS *generic_status, 
	      CL_ERR_DESC *system_status, GCC_ERLIST *erlist )
{
    /*
    ** We have to be careful here. In the TL it's possible that we 
    ** have an "orphaned" MDE:- ie we got an incoming TPDU that did
    ** not refer to an existing connection. Consequently, there's no
    ** CCB associated with the MDE and we can't reenter the TL with
    ** an event. 
    */
    if (ccb == NULL)
    {
	/* Free up the mde and get out of here */
	gcc_del_mde( mde );
	return;
    }

    if (!(ccb->ccb_tce.t_flags & GCCTL_ABT))
    {
	/* Log an error */
	gcc_er_log(generic_status, system_status, mde, erlist);

	/* Set abort flag so we don't get in an endless aborting loop */
        ccb->ccb_tce.t_flags |= GCCTL_ABT;    

        /* Issue the abort */
    	if (mde != NULL)
    	{
    	    mde->service_primitive = T_P_ABORT;
	    mde->primitive_type = RQS;
    	    mde->p1 = mde->p2 = mde->papp;
	    mde->mde_generic_status[0] = *generic_status;
	    (VOID) gcc_tl(mde);
	}
    }

    return;
} /* gcc_tl_abort */


/*
** Name: gcc_tl_shut
**
** Description:
**	Issue a T_DISCONNECT request with error status to the
**	TL to shutdown the connection firmly but nicely.
** 
** Inputs:
**      mde		    Pointer to mde
**      ccb		    Pointer to ccb
**	generic_status      Mainline status code
**      system_status       System specfic status code
**      erlist		    Error parameter list
**
** Outputs:
**      none
**
** Returns:
**	VOID
**
** History:
**	19-Aug-97 (gordy)
**	    Created.
*/

static VOID
gcc_tl_shut
( 
    GCC_MDE	*mde, 
    GCC_CCB	*ccb, 
    STATUS	*generic_status, 
    CL_ERR_DESC	*system_status, 
    GCC_ERLIST	*erlist
)
{
    GCC_MDE *shut_mde;

    /* 
    ** Log the error 
    */
    gcc_er_log( generic_status, system_status, mde, erlist );

    if ( (shut_mde = gcc_add_mde( ccb, 0 )) )
    {
	shut_mde->service_primitive = T_DISCONNECT;
	shut_mde->primitive_type = RQS;
	shut_mde->ccb = ccb;
	shut_mde->mde_generic_status[0] = *generic_status;
	gcc_tl( shut_mde );
    }
    else
    {
	/*
	** Since we can't cleanly shut down 
	** the connection, abort instead.
	*/
	*generic_status = E_GC2004_ALLOCN_FAIL;
	gcc_tl_abort( mde, ccb, generic_status, NULL, NULL );
    }

    return;
} /* gcc_tl_shut */


/*
** Name: gcc_tl_release_ccb
**
** Description:
**	Release, and possibly free, a Connection Control Block.
**	Free the TL resources associated with the CCB.
**
**	Check the queues in the CCB.  If not empty, purge them 
**	and release the MDE's.  Decrement the use count in the 
**	CCB.  If it is 0, all users are finished with it, and 
**	it can be released.  Otherwise, someone is still using 
**	it, and it will be deleted when all others are through 
**	with it.  When the CCB is actually deleted, the global 
**	connection counts are decmemented.
**
** Input:
**	ccb		Connection Control Block
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	13-Aug-97 (gordy)
**	    Created.
*/

static VOID
gcc_tl_release_ccb( GCC_CCB *ccb )
{
    GCS_ETERM_PARM	eterm_parm;
    GCS_CBQ             *cbque;
    STATUS              gcs_status;

# ifdef GCXDEBUG
    if ( IIGCc_global->gcc_trace_level >= 2 ) TRdisplay( "gcc_tl: releasing the CCB\n" );
# endif /* GCXDEBUG */

    ccb->ccb_tce.t_flags &= ~GCCTL_CCB_RELEASE;

    for( cbque  = (GCS_CBQ *)ccb->ccb_tce.t_gcs_cbq.q_next;
	 cbque != (GCS_CBQ *)&ccb->ccb_tce.t_gcs_cbq;
	 cbque  = (GCS_CBQ *)ccb->ccb_tce.t_gcs_cbq.q_next
       )
    {
	QUremove( (QUEUE *)cbque );
	eterm_parm.escb = cbque->cb;
	IIgcs_call( GCS_OP_E_TERM, cbque->mech_id, (PTR)&eterm_parm );
	gcc_free( (PTR)cbque );
    }

    if ( ccb->ccb_tce.t_flags & GCCTL_ENCRYPT )
    {
	ccb->ccb_tce.t_flags &= ~GCCTL_ENCRYPT;
	eterm_parm.escb = ccb->ccb_tce.gcs_cb;
	IIgcs_call( GCS_OP_E_TERM, ccb->ccb_tce.mech_id, (PTR)&eterm_parm );
    }

    while( ccb->ccb_tce.tl_evq.evq.q_next != &ccb->ccb_tce.tl_evq.evq )
	gcc_del_mde( (GCC_MDE *)QUremove(ccb->ccb_tce.tl_evq.evq.q_next) );

    while( ccb->ccb_tce.t_snd_q.q_next != &ccb->ccb_tce.t_snd_q )
	gcc_del_mde ( (GCC_MDE *)QUremove(ccb->ccb_tce.t_snd_q.q_next) );

    if ( --ccb->ccb_hdr.async_cnt <= 0 )
    {
	gcc_del_ccb( ccb );

# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "gcc_tl: %d inbound connections, %d total connections\n",
			IIGCc_global->gcc_ib_conn_ct,
			IIGCc_global->gcc_conn_ct );
# endif /* GCXDEBUG */

	/*
	** Check for Comm Server in "quiescing" state.  If so, and this is
	** the last connection, shut the Comm Server down.
	*/
	if ( ! IIGCc_global->gcc_conn_ct  && 
	     IIGCc_global->gcc_flags & GCC_QUIESCE )  GCshut();
    }

    return;
} /* gcc_tl_release_ccb */

