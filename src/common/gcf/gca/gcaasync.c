/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <erglf.h>

#include    <at.h>
#include    <cs.h>
#include    <cv.h>
#include    <gc.h>
#include    <id.h>
#include    <lo.h>
#include    <me.h>
#include    <mo.h>
#include    <mu.h>
#include    <nm.h>
#include    <qu.h>
#include    <si.h>
#include    <sp.h>
#include    <st.h>
#include    <tr.h>

#include    <iicommon.h>

#include    <gca.h>
#include    <gcn.h>
#include    <gcu.h>
#include    <gcs.h>
#include    <gcm.h>
#include    <gcaint.h>
#include    <gcaimsg.h>
#include    <gcatrace.h>
#include    <gcocomp.h>
#include    <gcmint.h>
#include    <gcxdebug.h>
#include    <gcr.h>

/*
** gcaasync.c - implementation of GCA's asynchronous services
**
** Description:
**
** 	This file contains the mondo asynchronous program for all
** 	GCA services.
**
** 	It defines the single entry point gca_service(), which executes
**	an interpreted "program" to carry out the services.
**
**	The body of gca_service() is a simple state machine controlled
**	by the array of states gca_states.  Each state has one
**	action, #define'd below and named by gca_names for tracing 
**	purposes, and a potential branch label.
**
**	Some actions may be async (returning immediately and signaling 
**	completion by reinvoking gca_service()).  The state machine is 
**	a loop as such:
**
**	     top:
**		print tracing info
**		run action for current state
**		branch to next state
**		goto top
**
**	The current state of the state machine is stored in svc_parms->state.
**	A stack, svc_parms->ss and svc_parms->sp, permit GOSUB/RETURN 
**	operations with limited nesting capability.  An alternate service 
**	parms, svc_parms->call_parms, permit FORK and CALL operations 
**	to perform independent processing on a separate SVC_PARMS.  FORK
**	routines, which are terminated by EXIT, allow the original svc_parms 
**	to continue processing without blocking.  CALL routines block the
**	original svc_parms until the routine completes with CALLRET.
**
**      The file is broken into four sections:
**
** 	1) Enumeration of actions of the program
**	2) Names for actions of the program
**	3) State table for gca_service() ("the program")
**	4) gca_service() ("the interpreter")
**
** History:
**	14-Mar-94 (seiwald)
**	    Created.
**	17-Mar-94 (seiwald)
**	    Provisional support for message encoding and decoding.
**	18-Mar-94 (seiwald)
**	    GCM handling in GCA_LISTEN was wrong.  Righted.
**	4-apr-94 (seiwald)
**	    At API_LEVEL_5, no GCA_FORMAT, GCA_INTERPRET.
**	5-apr-94 (seiwald)
**	    Handle send encodings spanning multiple invocations.
**	11-apr-94 (seiwald)
**	    Handle recv decodings spanning multiple invocations (untested).
**	11-apr-94 (seiwald)
**	    Punt on handling decoding tuple data -- disable decoding for
**	    tuple data and just pass it straight through.
**	13-Apr-1994 (daveb) 62240
**	    Keep traffic statistics for MO.
**	21-apr-94 (seiwald)
**	    Incorporated GCA_REGISTER, minus the bug fix that allowed
**	    splitting a long gca_server_object_vector into multiple
**	    messages.  Reorganized calls to CL.
**	22-apr-94 (seiwald)
**	    Reinstate indavertantly eliminated calls to GC_SEND.
**	24-apr-94 (seiwald)
**	    Shrink code by eliminating extra switch() on action code
**	    to pick up branch flag.  Otherwise atempt to speed things
**	    up.
**	24-apr-94 (seiwald)
**	    Use the new GCC_DOC->state flag to communicate a little
**	    more affirmatively about the results of perform_encoding
**	    and perform_decoding().
**	24-Apr-94 (seiwald)
**	    Now that perform_deconding() can handle tuple data, remove
**	    special exclusion of tuple data from RV_ISFMTED.
**	04-May-94 (seiwald)
**	    Don't send failed receives through RV_DONE, just head straight
**	    for completion.
**	16-May-94 (seiwald)
**	    Optimized branches in state machine; new jump table for
**	    service_code.
**	16-May-94 (seiwald)
**	    All operations now go through gca_service().  Leading comments
**	    changed to reflect this.
**	16-May-94 (seiwald)
**	    Use peersend for all outbound peer data.
**	16-May-94 (seiwald) bug #63600
**	    Connect directly to the name server if the target address
**	    is recognised to be "/@name-server-address" as well as "/iinmsvr".
**	 9-Nov-94 (gordy)
**	    Anytime a new receive begins, reload the caller's output buffer.
**	23-Nov-94 (brucel)
**	    Don't set rqcb ptr unless acb exists; GCA_INITIATE call was 
**          taking a segment violation when setting rqcb from acb 
**          (which is 0).
**	18-Apr-95 (canor01)
**	    Add server calss to IIGCa_static.
**	21-Apr-95 (gordy)
**	    It is possible for perform_decoding() to leave unprocessed
**	    data in the buffer and still request more input (only part
**	    of an atomic data value is present).  GC_RECEIVE and its
**	    predecessors must handle this situation by setting rsm->bufptr
**	    and rsm->bufend correctly.  Adding GC_RECVPRG which branches
**	    to GC_RECEIVE so that GC_RECVCMP never does.
**      12-jun-1995 (chech02)
**          Added semaphores to protect critical sections (MCT).
**	20-Jun-95 (gordy)
**	    When formatted/unformatted message are intermixed (as is
**	    required when sending BLOBs in GCA_DATA_VALUEs), the encoder
**	    must be properly shut down at the end of the message.  If
**	    the last part is sent unformatted, the encoder state must
**	    be set to DOC_DONE.  This is now done in SD_NOFLUSH since
**	    this is the first state after message processing and it
**	    already tests for end-of-data.
**	 1-Sep-95 (gordy) bug #67475
**	    Adding GCA_RQ_SERVER and GCA_LS_SERVER flags.
**	 6-Oct-95 (gordy)
**	    GCA_REGISTER needs own RG_FMT_PEER action since RQ_FMT_PEER2
**	    uses the GCA_REQUEST parm list.
**	10-Oct-95 (gordy)
**	    GCA_RESTORE needs to call SA_BUFFERS which would normally be
**	    done in GCA_REQUEST.  Also check for errors from gca_restore().
**	13-Oct-95 (gordy)
**	    Add SA_EXIT for functions which do not call completion routines.
**	 3-Nov-95 (gordy)
**	    Removed unneeded MCT ifdefs.
**	14-Nov-95 (gordy)
**	    Cleaned up fast select kludge at API level 5, info now returned
**	    in listen parms rather than appended to aux data.  Make sure
**	    encode/decoder shut down when purge occurs.  
**	16-Nov-95 (gordy)
**	    Bumped protocol level.
**	20-Nov-95 (gordy)
**	    Check for proper initialization.  Move remaining GCA_INITIATE
**	    code to gca_init() (excluding labels[] initialization).
**	21-Nov-95 (gordy)
**	    Added gcr.h for embedded Comm Server configuration.  Set
**	    the new remote flag in ACB.
**	 4-Dec-95 (gordy)
**	    Added support for embedded Name Server configuration.
**	22-Dec-95 (gordy)
**	    Be sure to call RQ_FREE if an error occurs after calling RQ_ALLOC.
**	12-Jan-96 (gordy)
**	    GCM requests were sometimes failing because of limited output
**	    buffer space.  Pass correct end of output buffer to gcm_response().
**	 6-Mar-96 (gordy)
**	    Sending formatted messages requires the message header to be
**	    updated with the message length once the encoding occurs.
**	    Add SD_BUFF1 so that header is not split if buffer is almost
**	    full.  Also need to place a new header in output stream after
**	    filling the output buffer (splitting the current message).
**	    Receiving formatted messages needs to detect and read message
**	    headers in the input stream.  Unformatted receives handle
**	    this by limiting the current receive to what remains in the
**	    current segment.  Formatted receives cannot do this since only 
**	    properly built C data structures may be returned to the caller, 
**	    which state is maintained at a lower level (gcd_decode()).  
**	    Added a check for end-of-segment and processing loop to read
**	    the header (can't jump back up to the original header loop 
**	    since it subsequently resets the output buffers).
**	 4-Apr-96 (gordy)
**	    RV_DONE should branch to receive next message when purging
**	    is active (as well as for GCA_GOTOHET messages).  This
**	    restores functionality which was apparently lost when the
**	    various state machines were combined into gca_service().
**	28-May-96 (gordy)
**	    Added tracing of peerinfo contents.
**	10-jun-96 (emmag for gordy)
**	    Server was including data in the buffer before an IACK which
**	    could cause a problem while purging on NT.
**	21-Jun-96 (gordy)
**	    Changes in the formatted interface processing of tuples
**	    removed the need to track tuple descriptors, so removed 
**	    the calls to gcd_td_mgr().
**	 1-Aug-96 (gordy)
**	    If a password is provided, it should be validated even if
**	    the user does not seem to be attempting to change ID.
**	 1-Aug-96 (gordy)
**	    Clean-up GCM protocol by allowing a GCA_RELEASE message to
**	    terminate the protocol, rather than just breaking/failing
**	    the connection.  Send GCA_RELEASE message for FASTSELECT.
**	19-Aug-96 (gordy)
**	    Added ability to concatenate message segments during non-
**	    formatted GCA_RECEIVE.
**	 3-Sep-96 (gordy)
**	    Restructured GCA global data.  Added GCA control blocks.
**      18-Nov-96 (fanra01)
**          Add an action GC_DELPASSWD to the LBL_RQ_DISCON state.  The action
**          takes the partner name and removes the vnode entry from the vnode
**          cache.  Windows 95 only.
**      20-Nov-96 (fanra01)
**          Add conditional compile for the GC_DELPASSWD case as the
**          GCpasswd_delete_vnode_info call will not be resolved on other
**          platforms.
**	18-Dec-96 (gordy)
**	    More adjustments to purging.  Make sure buffered data is cleared
**	    after chop marks.
**	31-Jan-97 (gordy)
**	    If client is using the formatted interface, the het-net tuple
**	    descriptor, gca_descriptor, should also be formatted so that
**	    the client will not have to build an unformatted descriptor
**	    just for het-net support.
**	 3-Feb-97 (gordy)
**	    Closing a small window between GC_SEND and SD_DONE in which a
**	    GCA_ATTENTION message has been sent but we have not entered the
**	    purging state.  Added action SD_CKATTN to set the purging state
**	    prior to flushing the attention message.
**	 4-Feb-97 (gordy)
**	    Fix to gca_descriptor handling.  Rearranged SD_NEEDTD so that
**	    gca_descriptor is not de-referrenced prior to checking for NULL.
**	 5-Feb-97 (gordy)
**	    Move chop_mark handling back to SD_DONE so that it is after
**	    GC_SEND completes.  Reset user buffer after purge received.
**	    Reduced usage of SA_IFERR by moving branch testing into the
**	    action where possible.  Make sure SA_BUFFERS gets called 
**	    only after an action which sets size_advise (GCrequest() or 
**	    GClisten()).  Since GCA_REQUEST may call SA_BUFFERS twice,
**	    SA_BUFFERS checks to see if allocation already completed.
**	18-Feb-97 (gordy)
**	    Name Server violates GCA protocol by using the end-of-data flag
**	    to indicate if more message are to follow, rather than just
**	    part of the current message.  Adding buffer concatenation broke
**	    the Name Server communication by concatenating messages rather
**	    than parts of a single message.  Adding a flag to disable the
**	    concatenation when connected directly to the Name Server.  A
**	    new protocol level will need to be added to fix the Name Server
**	    protocol.  Reworked other ACB booleans as flags as well.
**      19-Feb-97 (fanra01)
**          Bug 80681
**          Pass original service parms when calling GCpurge.  Exception
**          caused by uninitialised acb from the svc.
**	21-Feb-97 (gordy)
**	    Changed gca_formatted to gca_modifiers for support of future
**	    extensions.  This change was done in the scope of API_LEVEL_5
**	    since very few GCA clients use this API level at this time.
**	24-Feb-97 (gordy)
**	    Made buffer_length in GCA header to be actual message length
**	    (header + data).  Make sure end-of-group is always set.  On
**	    receive, end-of-group passed in GCA header at IPC level 5.
**	    Allow client to set end-of-group explicitly on send.
**	26-Feb-97 (gordy)
**	    Extracted utility functions to gcu.
**	 9-Apr-97 (gordy)
**	    GCsendpeer() and GCrecvpeer() are gone, RIP.  Peer info now
**	    exchanged with GCsend() and GCreceive().
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
**	23-May-97 (thoda04)
**	    Fix to GCA_DEBUG_MACRO call to cast arguments correctly for WIN16.
**	30-May-97 (gordy)
**	    New GCF security handling.  Messy GCA security replaced with
**	    GCS security interface.  GCA now uses GCS to generate/validate
**	    authentications.  GCA_PEER_INFO reworked for new security and
**	    future extensibility.
**      09-jun-1997 (rosga02)
**          Clear buffer from security label info, otherwize it will be
**          treated as message header
**	 8-Jul-97 (gordy)
**	    Allow for appended messages when reading peer info.
**      24-jul-97 (stial01)
**          Pass correct end of output buffer to gcm_response().
**      07-aug-97 (harpa06)
**         Initialize Name Server address prior to testing for direct Name 
**         Server connection requests. b84231.
**	17-Oct-97 (rajus01)
**	   GCA_RMT_ADDR is replaced by GCA_RMT_INFO. 
**	 4-Dec-97 (rajus01)
**	   Use 'sbuffer' instead of 'rbuffer' to build the GCA_RMT_INFO.
**	   Added remote authentication to GCA_RMT_INFO.
**	18-Dec-97 (gordy)
**	    ACB now deleted in gca_complete(), flag for deletion.
**	29-Dec-97 (gordy)
**	    Reworked send/receive to respect CL size_advise while
**	    maximizing our buffering.  Now buffer in some multiple
**	    of CL size_advise upto GCA_FS_MAX_DATA (4096) plus room
**	    for header.  Guarantees that FASTSELECT can process
**	    GCA_FS_MAX_DATA independent of CL size_advise.
**	12-Jan-98 (gordy)
**	    Added host name for local connections to support direct GCA
**	    network connections.  SA_BUFFERS must come after SA_RESTORE
**	    because acb is allocated by SA_RESTORE action.
**	27-Jan-98 (gordy)
**	    Moved GCM privilege check from LS_CHK_PEER to new action 
**	    and duplicated in gcm_response().  A privilege failure 
**	    still returns error code in peer info, but now enters the 
**	    GCM loop.  If a GCM request then comes in, gcm_response() 
**	    also checks the privileges and returns a suitable response.  
**	    The original check in LS_CHK_PEER was correct, but the 
**	    change overcomes a CL protocol driver bug which causes the 
**	    client to miss the peer info when the server drops the 
**	    connection after sending a peer info error status.  This
**	    was particularily annoying for iinamu, netu, and netutil
**	    which do Name Server bedchecks using GCM.  A failure due
**	    to no privileges was acceptable, but a read failure had
**	    to be treated as a bedcheck failure and unprivileged
**	    users were frequently unable to use these utilities.
**	17-Mar-98 (gordy)
**	    Use GCusername() to get effective user ID.  If the CL size
**	    advise is zero, user our desired buffer size.
**	18-Mar-98 (gordy)
**	    Be sure and return errors to client from processing peer info.
**	 5-May-98 (gordy)
**	    Client user ID (not alias) now returned in gca_account_name.
**	14-May-98 (gordy)
**	    Added support for remote authentication.
**	15-May-98 (gordy)
**	    Make sure FASTSELECT info is received before peer info returned.
**	19-May-98 (gordy)
**	    Remove buffers from rqcb and use existing rsm/ssm buffers.
**	26-May-98 (gordy)
**	    Added config param for ignore/fail on remote auth errors.
**	    Name Server no longer generates error if no login info, so
**	    it is done here if no remote auth or password is available.
**	27-May-98 (gordy)
**	    gca_rem_auth() no longer returns FAIL ('none' handle by IIGCN).
**	    Handle stack overflow/underflow in SA_GOSUB, SA_RETURN.
**	    Validate FASTSELECT buffer/message lengths.
**	 4-Jun-98 (gordy)
**	    No timeout after GCrequest()/GClisten() complete.  Restore
**	    timeout for each new connection request.
**	26-Aug-98 (gordy)
**	    Delegate user authentication if requested by GCA client
**	    and return as aux data.
**	15-Sep-98 (gordy)
**	    Added flags to permit servers to control GCM access.
**	 6-Oct-98 (gordy)
**	    When NS provided remote auth was moved to take precedence
**	    over locally generated remote auth, failed to set status OK.
**	17-Nov-98 (gordy)
**	    Don't test remote index vs remote count when doing NEXT_ADDR
**	    loop if the request in not remote.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	30-Jun-99 (rajus01)
**	    Set the remote flag in gc_parms for Embedded Comm Server 
**	    support. gca_is_eog() takes protocol version as input 
**	    instead of acb. Added #ifdef GCF_EMBEDDED_GCC.
**	 4-Aug-99 (rajus01)
**	    Check gca_embed_gcc_flag before turning on embedded comsvr
**	    support. 
**	 6-Nov-99 (gordy)
**	    Connection failures with invalid password, E_GC000B, should
**	    skip all remaining local as well as remote targets and fail.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-Mar-02 (gordy)
**	    Added routine to access tuple descriptor ID.
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	12-May-03 (gordy)
**	    Renamed DOC->last_seg to eod to match message flag.  
**	    Initialize DOC state with DOC_IDLE (DOC_DONE now distinct).
**	 6-Jun-03 (gordy)
**	    Enhanced MIB information.  Attach association MIB object
**	    when association exposed to application (LISTEN/REQUEST
**	    completion).  Split REQUEST completion into fastselect,
**	    normal, and error actions.
**	15-Oct-03 (gordy)
**	    REGISTER & FASTSELECT no longer need to mark ACB for deletion
**	    (now done automatically in gca_call() when request invoked).
**	28-May-04 (gordy)
**	    Support Name Server only registration (re-register after
**	    successful reguster).
**	 9-Aug-04 (gordy)
**	    Use ACB flag to indicate active send rather than the
**	    more kludgy testing of normal channel state.
**	18-Oct-04 (wansh01)
**	    Check user privilege for admin session (GCA_ID_CMDSESS);
**	18-Jan-05 (wansh01)
**	    supress [uid,pwd] info in gca_partner_name in trace msg.
**	    supress GC_SEND buffer dump if msg type is GCN_NS_RESOLVE
**	    or GCN_NS_2_RESOLVE for security concern.  
**	    it might contains login info (uid,pwd) in plain text.
**	28-Jan-05 (wansh01)
**	    supress pwd only in login info for vnodeless connection. 
**	01-Feb-05 (wansh01)
**	    added {} around case statement variables to correct syntax errors.
**	29-Jan-2005 (schka24)
**	    Sun C requires a block for the decls added above for RQ_INIT case.
**	30-Nov-05 (gordy)
**	    Fixed non-critical message lengths in formatted processing.
**	    Removed some redundant actions in state machine for formatted
**	    processing.
**	 6-Dec-05 (gordy)
**	    Ensure all local server connections are attempted when no
**	    remote connections exist.
**	 1-Mar-06 (gordy)
**	    Added FORK/CALL/CALLRET mechanism for servicing alternate
**	    SVC_PARMS.  Name Server registration connection now kept
**	    open from GCA_REGISTER to GCA_TERMINATE using registration
**	    SVC_PARMS in GCA control block.
**	 8-May-06 (gordy)
**	    Duplicate request detection on the registration ACB (CALL
**	    operations) was originally added to detect logic errors.  
**	    When used in a OS multi-threaded server, it is quite likely
**	    that duplicate update requests will occur.  The duplicate
**	    request error is now used to handle logic flow so as to
**	    properly ignore the duplicate request and the error is
**	    only exposed where appropriate (GCA_REGISTER).  A server
**	    crash has occured which may be caused by the window between
**	    duplicate request detection and the flagging of an active
**	    request.  The active flag (in_use) is now set immediately 
**	    after testing (SA_CALL, SA_FORK).  A semaphore may be 
**	    required (test-and-set operation would be sufficient) to
**	    properly handle a multi-threaded environment.
**	11-May-06 (gordy)
**	    As noted above, it appears that a semaphore is necessary to
**	    protect the registration ACB.  Rather than add a semaphore 
**	    to every ACB (or svc_parms), just add a single semaphore to
**	    the GCA control block.  Unfortunately, this means the dup
**	    request test must be moved out of SA_CALL, a generic op,
**	    to a registration specific op, in this case RG_INIT_CONN,
**	    and RG_INIT_SEND.
**	 8-Jun-06 (gordy)
**	    When freeing registration ACB, mark for deletion if active.
**      13-Sep-2007 (ashco01) Bug 119120
**          ** Note: This change is an extension of r3 fix for b116046. **
**          Protect normal-channel SEND operations against AST initiated
**          gca_purge() attempts from SD_INIT thru SD_DONE to preserve
**          ssm buffer pointers. Replaced and extended check for normal
**          channel 'state' with check for additional ACB flag 'send_active'.
**          Also, in SD_CKIACK check if 'chop marks' owed iimediately prior
**          to calling gca_purge(). If so, then a GCA_ATTENTION has been
**          recieved, so unprotect 'send' operation and send chop marks now.
**      04-Dec-2007 (horda03/ashco01) Bug 119120
**          Re-instate check for outstanding GC_SEND and do not call
**          gca_purge() if this is the case.
**      13-dec-2007 (horda03) Bug 119120
**          Added send_active_set variable to identify that the call was an SEND
**          which set the SEND_ACTIVE flag. In SA_COMPLETE, the SEND_ACTIVE
**          flag is cleared if "send_active_set" set. This prevents FE hangs.
**	27-Apr-2008 (rajus01) SD issue 126704 Bug(s) 120316, 120317.
**	    While testing manual X-integration of the change 491592 from II 2.6
**	    codeline for the bugs mentioned above the GCD server experienced 
**	    another SEGV when longer database names are provided in the JDBC
**	    URL. Limiting the length of partner_name to be copied into 
**	    conn_buff to MAX_LOC seems a reasonable thing to do in this case. 
**	   
**	15-May-08 (gordy)
**	    Request completion, gca_complete(), is now done in gca_terminate().
**      30-Jan-09 (gordy & rajus01) SD issue 132766, Bug #121341
**	    The STAR server sessions to DBMS server hung in system call 
**	    select() under heavily concurrent DISASSOC sync requests.It has
**	    been identified that GC_drive_complete() routine is
**	    not thread safe/aware and it attempts to limit recursion to protect
**	    the stack thus resulting in callbacks occuring on a wrong
**	    thread under heavily multi-threaded situations that lead to 
**	    hung sessions in STAR server. Lack of good solution to make 
**	    GC_drive_complete() thread aware/safe it has been decided to
**	    move the recursion block from GC layer to GCA layer by defining
**	    a new action called GC_ASYNC to allow one active operation 
**	    of a given type on a connection. It is believed that this
**	    approach should avoid thread contention issue and will provide
**	    even better stack protection since there will be less on the
**	    stack at the point where a recursion is detected by a thread.
**	17-Jul-09 (gordy)
**	    Peer info enhanced to allow long variable length user names.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**	11-Feb-2010 (kschendel) SIR 123275
**	    Buffer sizes have been wired to GCA_FS_MAX_DATA because SA_BUFFERS
**	    is (usually) called before SA_SAV_PARMS.  It gets complicated
**	    because SA_SAV_PARMS has to wait for the CL request or listen.
**	    Change BUFFERS to use GCA_NORM_SIZE (but always at least
**	    GCA_FS_MAX_DATA), and at SAV_PARMS time, if the size-advise
**	    is now larger than the normal buffer, reallocate to the new
**	    larger size.
**      02-Jun-2010 (horda03) b123840
**          Include at.h to gain access to atomic instructions.
**          If GCA_ATOMIC_INCREMENT_STATE defined then use atomic instruction 
**          to increment state variable. The svc_parms->state parameter on VMS
**          can be updated by code running within an AST at the same time that
**          the session is updating it - this then causes the session to hang.
**      04-Jun-2010 (horda03) b123840
**          Reworked change for b123840 so that #ifdef done in gc.h and not here.
*/

/*
**  Forward and/or External function references.
*/

static bool	gca_check_peer( char *, i4 );
static i4	gca_frmt_v5_peer( GCA_ACB *, GCA_PEER_INFO *, PTR );
static STATUS	gca_load_v5_peer( GCA_ACB *, char **, GCA_PEER_INFO * );
static i4	gca_frmt_v6_peer( GCA_ACB *, GCA_PEER_INFO *, PTR );
static STATUS	gca_load_v6_peer( GCA_ACB *, char **, GCA_PEER_INFO * );
static i4	gca_frmt_v7_peer( GCA_ACB *, GCA_PEER_INFO *, PTR );
static STATUS	gca_load_v7_peer( GCA_ACB *, char **, GCA_PEER_INFO * );
static VOID	gca_purge( GCA_SVC_PARMS * );
static VOID	gca_acb_buffers( GCA_ACB * );

/* 
** Enumerations of actions of the program 
*/

# define SA_LABEL	0	/* label */
# define SA_NOOP	1	/* no operation - place holder */
# define SA_INIT	2	/* general initialization */
# define SA_COMPLETE	3	/* drive gca_complete */
# define SA_JUMP	4	/* Jump to a GCA function entry point*/
# define SA_EXIT	5	/* Exit gca_service() */
# define SA_GOTO	6	/* goto unconditionally */
# define SA_GOSUB	7	/* go subroutine */
# define SA_RETURN	8	/* return from subroutine */
# define SA_CALL	9	/* call routine (context switch) */
# define SA_CALLRET	10	/* return from call (restore context) */
# define SA_FORK	11	/* fork new service */
# define SA_CLRERR	12	/* clear error */
# define SA_IFERR	13	/* branch of status != OK */
# define SA_IS_ERRMSG	14	/* is GCA_ERROR message */
# define SA_SAV_PARMS	15	/* Save GC parameters in ACB */
# define SA_BUFFERS	16	/* allocate gca buffers */
# define SA_RECV_BUFF	17	/* Prepare for receive */
# define SA_RECV_DONE	18	/* Receive complete */
# define SA_NOCHOP	19	/* if no chop mark */
# define SA_RECV_PEER	20	/* Read peer info */
# define SA_SEND_PEER	21	/* Prepare to send peer info */
# define RG_IF_NSONLY	22	/* if Name Serve only registration */
# define RG_IF_NONS	23	/* if skipping Name Server registration */
# define RG_IF_REG	24	/* if duplicate register */
# define RG_IF_NOREG	25	/* if not registered */
# define RG_IF_SELF	26	/* if partner address matches our address */
# define RG_IF_NSOK	27	/* if connection to NS OK */
# define RG_IF_NSFAIL	28	/* if connection to NS failed */
# define RG_IF_NSDISC	29	/* if no NS connection */
# define RG_NS_MORE	30	/* if not EOG */
# define RG_FMT_PEER	31	/* format peer info for Name Server */
# define RG_NS_OPER	32	/* format GCN_NS_OPER message */
# define RG_FMT_ACK	33	/* format GCA_ACK message */
# define RG_INIT_CONN	34	/* init connection register service parms */
# define RG_INIT_SEND	35	/* init send register service parms */
# define RG_INIT_EXP	36	/* init expedited register service parms */
# define RG_SAVE	37	/* save registration info */
# define RG_FREE	38	/* free registration info */
# define RG_FREE_ACB	39	/* free registration ACB */
# define RG_NS_OK	40	/* mark NS connection OK */
# define RG_NS_FAIL	41	/* mark NS connection failed */
# define LS_INIT	42	/* init GCA_LISTEN call */
# define RR_INIT	43	/* init GCA_RQRESP call */
# define LS_IS_NOSL	44	/* if no security label */
# define LS_CHK_SL	45	/* validate security label */
# define LS_CHK_PEER	46	/* check peer info contents */
# define LS_CHKBED	47	/* check for BEDCHECK connection */
# define LS_CHKGCM	48	/* check for GCM connection */
# define LS_CHKFAST	49	/* check for FASTSELECT connection */
# define LS_CHKAPI	50	/* check for API level above listen confirm */
# define LS_LSNCNFM	51	/* do old listen confirm */
# define LS_FMT_PEER	52	/* post CL GCsendper call */
# define LS_GET_PSTAT	53	/* get status from peer info */
# define LS_SET_PSTAT	54	/* set status in peer info */
# define LS_DONE	55	/* fill in GCA_LISTEN output parms */
# define LS_GCM_PRIV	56	/* check privileges to do GCM */
# define LS_RECV_GCM	57	/* prepare to receive GCM query */
# define LS_IF_RELEASE	58	/* if GCA_RELEASE message received */
# define LS_RESP_GCM	59	/* generate GCM response */
# define LS_NO_PEER	60	/* set NO PEER status */
# define LS_REMOTE_GCM	61	/* check if remote connection */
# define LS_HIJACK	62	/* take over first listen thread for traps */
# define LS_DELIVER	63	/* deliver GCM trap indications */
# define RQ_INIT	64	/* initialize request */
# define RQ_ALLOC	65	/* allocate RQCB */
# define RQ_FREE	66	/* free RQCB */
# define RQ_IS_NOXLATE	67	/* check if GCA_NO_XLATE flag is set */
# define RQ_IS_RSLVD	68	/* check if GCA_RQ_RSLVD flag is set */
# define RQ_IS_NMSVR	69	/* check if partner is /iinmsvr */
# define RQ_USE_NMSVR	70	/* check if embedded Name Server disabled */
# define RQ_RESOLVE	71	/* Embedded Name Server resolve */
# define RQ_ADDR_NOXL	72	/* get NO_XLATE addr for GCrequest */
# define RQ_ADDR_NSID	73	/* get Name Server addr for GCrequest */
# define RQ_FMT_RSLV 	74	/* fmt GCN_2_RESOLVE message for GCsend */
# define RQ_FMT_RLS	75	/* fmt GCA_RELEASE message for GCsend */
# define RQ_RECV	76	/* call GCreceive */
# define RQ_LOAD_RSLVD	77	/* load GCN_RESOLVED from Name Server */
# define RQ_LOAD_PRTNR	78	/* load GCN_RESOLVED from partner_name */
# define RQ_ADDR_NEXT	79	/* get next addr for GCrequest */
# define RQ_CHK_FAIL	80	/* determine why connect failed */
# define RQ_PROT_MINE	81	/* set protocol to GCA_PROTOCOL_LEVEL */
# define RQ_PROT_USER	82	/* set protocol to rq_parms->peer_protocol */
# define RQ_FMT_PEER1	83	/* fmt peer info for server connect */
# define RQ_FMT_PEER2	84	/* fmt peer info for Name Serve resolve */
# define RQ_CHECKPEER	85	/* propagate status from peer info */
# define RQ_SAVERR	86	/* call GCdisconn */
# define RQ_GETERR	87	/* failed completion */
# define RQ_CHK_FAST	88	/* check for GCA_FASTSELECT service */
# define RQ_SEND_FAST	89	/* prepare to send FASTSELECT query */
# define RQ_RECV_FAST	90	/* prepare to receive FASTSELECT response */
# define RQ_COMP_FAST	91	/* FASTSELECT completion */
# define RQ_COMP_ERR	92	/* error completion */
# define RQ_COMPLETE	93	/* successful completion */
# define RV_INIT	94	/* setup GCA_READ */
# define RV_IS_HDR 	95	/* no action - check for header expected */
# define RV_IS_MORE	96	/* no action - check for more data */
# define RV_IS_FMT	97	/* no action - check GCA_FORMATTED */
# define RV_IS_HET	98	/* no action - check for GCA_GOTOHET */
# define RV_IS_PRG	99	/* no action - check {r,s}purging flag */
# define RV_IS_SPRG	100	/* no action - check spurging flag */
# define RV_CONCAT	101	/* no action - check for end_of_data */
# define RV_DOUSR	102	/* set pointer to user's buffer */
# define RV_SVCHDR	103	/* prepare to fill header */
# define RV_SVCUSR	104	/* prepare to fill user data */
# define RV_BUFF	105	/* copy buffered data */
# define RV_CLRPRG	106	/* clear purging flag */
# define RV_PURGED	107	/* purge request; clear purging flags */
# define RV_FMT		108	/* perform data decoding */
# define RV_DONE	109	/* messages received: do msg specific actions */
# define SD_INIT	110	/* setup for send */
# define SD_CKIACK	111	/* check if message is IACK */
# define SD_CKATTN	112	/* check if message is ATTENTION */
# define SD_NEEDTD	113	/* check if we need to send GCA_TO_DESCR */
# define SD_IS_FMT	114	/* if GCA_FORMATTED set */
# define SD_NOFLUSH	115	/* no action: check if we can skip flushing */
# define SD_MORE	116	/* check if more data to send */
# define SD_DOTD	117	/* prepare to send tuple descriptor */
# define SD_DOUSR	118	/* fill message header for user data */
# define SD_SVCHDR	119	/* prepare to buffer message header */
# define SD_SVCUSR	120	/* prepare to buffer user data */
# define SD_BUFF	121	/* copy partial data into buffer */
# define SD_BUFF1	122	/* copy entire data into buffer */
# define SD_FMT		123	/* perform data-encoding */
# define SD_DONE	124	/* send post-processing */
# define DA_DISCONN	125	/* init GCA_DISCONN */
# define DA_COMPLETE	126	/* finish up GCA_DISCONN */
# define SA_INITIATE	127	/* GCA_INITIATE */
# define SA_TERMINATE	128	/* GCA_TERMINATE */
# define SA_SAVE	129	/* GCA_SAVE */
# define SA_RESTORE	130	/* GCA_RESTORE */
# define SA_FORMAT	131	/* GCA_FORMAT */
# define SA_INTERPRET	132	/* GCA_INTERPRET */
# define SA_ATTRIBS	133	/* GCA_ATTRIBS */
# define SA_REG_TRAP	134	/* GCA_REG_TRAP */
# define SA_UNREG_TRAP	135	/* GCA_UNREG_TRAP */
# define GC_REGISTER	136	/* call GCregister */
# define GC_LISTEN	137	/* call GClisten */
# define GC_REQUEST	138	/* call GCreqest */
# define GC_SEND	139	/* call GCsend */
# define GC_RECEIVE	140	/* call GCreceive */
# define GC_DISCONN	141	/* call GCdisconn */
# define GC_RELEASE	142	/* call GCrelease */
# define GC_ASYNC	143	/* Handle async GC call */
# define GC_DELPASSWD	144	/* call GCpasswd_delete_vnode_info */

/* 
** Names for actions of the program 
*/

static char *gca_names[] = 
{
    "SA_LABEL", "SA_NOOP", "SA_INIT", "SA_COMPLETE", 
    "SA_JUMP", "SA_EXIT", "SA_GOTO", "SA_GOSUB", "SA_RETURN", 
    "SA_CALL", "SA_CALLRET", "SA_FORK", "SA_CLRERR",
    "SA_IFERR", "SA_IS_ERRMSG", "SA_SAV_PARMS", "SA_BUFFERS", 
    "SA_RECV_BUFF", "SA_RECV_DONE", "SA_NOCHOP", 
    "SA_RECV_PEER", "SA_SEND_PEER",

    "RG_IF_NSONLY", "RG_IF_NONS", "RG_IF_REG", "RG_IF_NOREG", 
    "RG_IF_SELF", "RG_IF_NSOK", "RG_IF_NSFAIL", "RG_IF_NSDISC", 
    "RG_NS_MORE", "RG_FMT_PEER", "RG_NS_OPER", "RG_FMT_ACK",
    "RG_INIT_CONN", "RG_INIT_SEND", "RG_INIT_EXP", 
    "RG_SAVE", "RG_FREE", "RG_FREE_ACB", "RG_NS_OK", "RG_NS_FAIL",

    "LS_INIT", "RR_INIT", "LS_IS_NOSL", "LS_CHK_SL", "LS_CHK_PEER", 
    "LS_CHKBED", "LS_CHKGCM", "LS_CHKFAST", "LS_CHKAPI", "LS_LSNCNFM", 
    "LS_FMT_PEER", "LS_GET_PSTAT", "LS_SET_PSTAT", "LS_DONE", 
    "LS_GCM_PRIV", "LS_RECV_GCM", "LS_IF_RELEASE", "LS_RESP_GCM", 
    "LS_NO_PEER", "LS_REMOTE_GCM", "LS_HIJACK", "LS_DELIVER", 

    "RQ_INIT", "RQ_ALLOC", "RQ_FREE", "RQ_IS_NOXLATE", "RQ_IS_RSLVD", 
    "RQ_IS_NMSVR", "RQ_USE_NMSVR", "RQ_RESOLVE", "RQ_ADDR_NOXL", 
    "RQ_ADDR_NSID", "RQ_FMT_RSLV", "RQ_FMT_RLS", "RQ_RECV", 
    "RQ_LOAD_RSLVD", "RQ_LOAD_PRTNR", "RQ_ADDR_NEXT", "RQ_CHK_FAIL", 
    "RQ_PROT_MINE", "RQ_PROT_USER", "RQ_FMT_PEER1", "RQ_FMT_PEER2", 
    "RQ_CHECKPEER", "RQ_SAVERR", "RQ_GETERR", "RQ_CHK_FAST", 
    "RQ_SEND_FAST", "RQ_RECV_FAST", "RQ_COMP_FAST", "RQ_COMP_ERR",
    "RQ_COMPLETE", 

    "RV_INIT", "RV_IS_HDR", "RV_IS_MORE", "RV_IS_FMT", "RV_IS_HET", 
    "RV_IS_PRG", "RV_IS_SPRG", "RV_CONCAT", "RV_DOUSR", "RV_SVCHDR", 
    "RV_SVCUSR", "RV_BUFF", "RV_CLRPRG", "RV_PURGED", "RV_FMT", "RV_DONE", 

    "SD_INIT", "SD_CKIACK", "SD_CKATTN", "SD_NEEDTD", "SD_IS_FMT", 
    "SD_NOFLUSH", "SD_MORE", "SD_DOTD", "SD_DOUSR", "SD_SVCHDR", 
    "SD_SVCUSR", "SD_BUFF", "SD_BUFF1", "SD_FMT", "SD_DONE", 

    "DA_DISCONN", "DA_COMPLETE",

    "SA_INITIATE", "SA_TERMINATE", "SA_SAVE", "SA_RESTORE", 
    "SA_FORMAT", "SA_INTERPRET", "SA_ATTRIBS", 
    "SA_REG_TRAP", "SA_UNREG_TRAP",
    
    "GC_REGISTER", "GC_LISTEN", "GC_REQUEST", "GC_SEND", "GC_RECEIVE", 
    "GC_DISCONN", "GC_RELEASE", "GC_ASYNC", "GC_DELPASSWD", 
};


/*
** Service program labels.
*/

# define	LBL_NONE		0

# define	LBL_REGISTER		1	/* GCA_REGISTER */
# define	LBL_RG_NS		2
# define	LBL_REG_NS		3	/* Register NS subroutine */
# define	LBL_RG_CONN		4
# define	LBL_RG_REG		5
# define	LBL_REG_CONN		6	/* Reg connect CALL routine */
# define	LBL_RG_PEER		7
# define	LBL_RG_NSERR		8
# define	LBL_REG_EXP		9	/* Reg expedited CALL routine */
# define	LBL_NS_FAIL		10
# define	LBL_REG_REG		11	/* Reg register CALL routine */
# define	LBL_RG_RECV		12
# define	LBL_RG_NSERR1		13
# define	LBL_REG_UPD		14	/* Reg update  CALL routine */
# define	LBL_REG_DISC		15	/* Reg disconnect CALL rtn */
# define	LBL_RG_DISC		16

# define	LBL_LISTEN		17	/* GCA_LISTEN */
# define	LBL_LS_PEER		18
# define	LBL_LS_PEER1		19
# define	LBL_LS_SL		20
# define	LBL_LS_CHECK		21
# define	LBL_LS_FAST		22
# define	LBL_LS_GCM		23
# define	LBL_GCM_LOOP		24
# define	LBL_GCM_DONE		25
# define	LBL_NO_PEER		26
# define	LBL_BEDCHECK		27
# define	LBL_LS_DONE		28
# define	LBL_LS_CMPLT		29
# define	LBL_LS_TRAP		30

# define	LBL_RQRESP		31	/* GCA_RQRESP */

# define	LBL_REQUEST		32	/* GCA_REQUEST */
# define	LBL_RQ_NOXL		33
# define	LBL_RQ_RSLVD		34
# define	LBL_RQ_NMSVR		35
# define	LBL_RQ_NEXT		36
# define	LBL_RQ_CONN		37
# define	LBL_RQ_PEER		38
# define	LBL_RQ_PEER1		39
# define	LBL_RQ_PEER2		40
# define	LBL_RQ_DONE		41
# define	LBL_RQ_DISCON		42
# define	LBL_RQ_ERROR		43
# define	LBL_RQ_FAST		44
# define	LBL_RQ_EXIT		45

# define	LBL_SEND		46	/* GCA_SEND */
# define	LBL_SD_DOTD		47
# define	LBL_SD_HDR		48
# define	LBL_SD_HDR1		49
# define	LBL_SD_USR		50
# define	LBL_SD_USR1		51
# define	LBL_SD_FMT		52
# define	LBL_SD_FLUSH		53
# define	LBL_SD_DONE		54

# define	LBL_RECEIVE		55	/* GCA_RECEIVE */
# define	LBL_RV_READ		56
# define	LBL_RV_HDR		57
# define	LBL_RV_HDR1		58
# define	LBL_RV_USR		59
# define	LBL_RV_USR1		60
# define	LBL_RV_CONCAT		61
# define	LBL_RV_FMT		62
# define	LBL_RV_DONE		63
# define	LBL_RV_PURGE		64
# define	LBL_RV_PURGED		65

# define	LBL_DISASSOC		66	/* GCA_DISASSOC */
# define	LBL_DA_DISC		67

# define	LBL_TERMINATE		68	/* GCA_TERMINATE */
# define	LBL_TM_TERM		69

# define	LBL_INITIATE		70	/* GCA_INITIATE */
# define	LBL_SAVE		71	/* GCA_SAVE */
# define	LBL_RESTORE		72	/* GCA_RESTORE */
# define	LBL_FORMAT		73	/* GCA_FORMAT */
# define	LBL_INTERPRET		74	/* GCA_INTERPRET */
# define	LBL_ATTRIBS		75	/* GCA_ATTRIBS */
# define	LBL_REG_TRAP		76	/* GCA_REG_TRAP */
# define	LBL_UNREG_TRAP		77	/* GCA_UNREG_TRAP */

# define	LBL_DONE		78	/* Generic complete */

# define	LBL_SEND_MSG		79	/* Send Bufferring Subroutine */
# define	LBL_SEND_HDR		80
# define	LBL_SEND_USR		81
# define	LBL_SEND_USR1		82

# define	LBL_SEND_BUF		83	/* Send Data Subroutine */

# define	LBL_RECV_MSG		84	/* Generic receive subroutine */
# define	LBL_RECV_HDR		85
# define	LBL_RECV_USR		86
# define	LBL_RECV_USR1		87
# define	LBL_RECV_DONE		88

# define	LBL_RETURN		89

# define	LBL_MAX			90

static unsigned short labels[ LBL_MAX ] = {0};


/*
** gca_states[] - GCA asynchronous service program
*/

static struct {
	unsigned char		action;
	unsigned char		label;
} gca_states[] = 
{
    /* state	action		branch		when branched */

    /* 
     * Initialize and dispatch request: states following SA_JUMP 
     * are a service_code jump table
     */

		SA_INIT,	LBL_NONE,
		SA_JUMP,	LBL_NONE,	/* Jump table follows below */

    		SA_NOOP,	LBL_INITIATE,	/* GCA_INITIATE */
    		SA_NOOP,	LBL_TERMINATE,	/* GCA_TERMINATE */
    		SA_NOOP,	LBL_REQUEST,	/* GCA_REQUEST */
    		SA_NOOP,	LBL_LISTEN,	/* GCA_LISTEN */
    		SA_NOOP,	LBL_SAVE,	/* GCA_SAVE */
    		SA_NOOP,	LBL_RESTORE,	/* GCA_RESTORE */
    		SA_NOOP,	LBL_FORMAT,	/* GCA_FORMAT */
    		SA_NOOP,	LBL_SEND,	/* GCA_SEND */
    		SA_NOOP,	LBL_RECEIVE,	/* GCA_RECEIVE */
    		SA_NOOP,	LBL_INTERPRET,	/* GCA_INTERPRET */
    		SA_NOOP,	LBL_REGISTER,	/* GCA_REGISTER */
    		SA_NOOP,	LBL_DISASSOC,	/* GCA_DISASSOC */
    		SA_NOOP,	LBL_DONE,	/* defunct */
    		SA_NOOP,	LBL_ATTRIBS,	/* GCA_ATTRIBS */
    		SA_NOOP,	LBL_RQRESP,	/* GCA_RQRESP */
    		SA_NOOP,	LBL_REQUEST,	/* GCA_FASTSELECT */
    		SA_NOOP,	LBL_REG_TRAP,	/* GCA_REG_TRAP */
    		SA_NOOP,	LBL_UNREG_TRAP,	/* GCA_UNREG_TRAP */

		SA_COMPLETE,	LBL_NONE,

    /*
    ** GCA_REGISTER
    */

    SA_LABEL,	LBL_REGISTER,

		RG_IF_NSONLY,	LBL_RG_NS,	/* Name Server only */
		RG_IF_REG,	LBL_DONE,	/* Duplicate register */
        	SA_BUFFERS,	LBL_DONE,	/* if no mem */
		GC_REGISTER,	LBL_NONE,
		SA_IFERR,	LBL_DONE,	/* on error */
		RG_SAVE,	LBL_DONE,	/* if no mem */
		RG_IF_NONS,	LBL_DONE,	/* if skipping Name Server */
		SA_GOSUB,	LBL_REG_NS,
		SA_GOTO,	LBL_DONE,

    /* NS register */

    SA_LABEL,	LBL_RG_NS,

		RG_IF_NONS,	LBL_DONE,	/* if skipping Name Server */
		RG_IF_NOREG,	LBL_DONE,	/* No prior register */
		RG_FREE,	LBL_NONE,
		RG_SAVE,	LBL_DONE,	/* if no mem */
		SA_GOSUB,	LBL_REG_NS,
		SA_GOTO,	LBL_DONE,

    /*
    ** Subroutine: register with Name Server
    */

    SA_LABEL,	LBL_REG_NS,

		RG_IF_NSDISC,	LBL_RG_CONN,	/* if no NS connection */
		RG_IF_NSOK,	LBL_RG_REG,	/* if connection to NS OK */
		RG_INIT_CONN,	LBL_RETURN,
		SA_CALL,	LBL_REG_DISC,
		SA_IFERR,	LBL_RETURN,

    SA_LABEL,	LBL_RG_CONN,

		RG_INIT_CONN,	LBL_RETURN,
    		SA_CALL,	LBL_REG_CONN,	/* connect to NS */
		SA_IFERR,	LBL_RETURN,
		RG_IF_NSFAIL,	LBL_RETURN,	/* if NS connection failed */
		RG_INIT_EXP,	LBL_NONE,
		SA_FORK,	LBL_REG_EXP,

    SA_LABEL,	LBL_RG_REG,

		RG_INIT_CONN,	LBL_RETURN,
		SA_CALL,	LBL_REG_REG,	/* register with NS */
		SA_RETURN,	LBL_DONE,
    
    /* Establish NS connection */

    SA_LABEL,	LBL_REG_CONN,

		RQ_ALLOC,	LBL_RG_NSERR,	/* if no mem */
        	RQ_ADDR_NSID,	LBL_RG_NSERR,
		RG_IF_SELF,	LBL_RG_NSERR,	/* if target is our address */
		RG_NS_OK,	LBL_NONE,
        	GC_REQUEST,	LBL_NONE,
		SA_SAV_PARMS,	LBL_NONE,
		SA_IFERR,	LBL_RG_NSERR,	
        	RG_FMT_PEER,	LBL_RG_NSERR,	/* Send peer info */
        	RQ_PROT_MINE,	LBL_NONE,	
		SA_SEND_PEER,	LBL_NONE,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_RG_NSERR,	
		SA_RECV_BUFF,	LBL_NONE,

    SA_LABEL,	LBL_RG_PEER,			/* Get peer info */

		GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_RG_NSERR,
		SA_RECV_DONE,	LBL_NONE,
		SA_RECV_PEER,	LBL_RG_PEER,	/* if need more data */
		SA_IFERR,	LBL_RG_NSERR,
        	RQ_CHECKPEER,	LBL_RG_NSERR,
		RQ_FREE,	LBL_NONE,
		SA_CALLRET,	LBL_DONE,

    SA_LABEL,	LBL_RG_NSERR,

		RG_NS_FAIL,	LBL_NONE,
		RQ_FREE,	LBL_NONE,
		SA_CALLRET,	LBL_DONE,

    /* Check NS connection status */

    SA_LABEL,	LBL_REG_EXP,

		SA_RECV_BUFF,	LBL_NONE,
    		GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_NS_FAIL,
		SA_RECV_DONE,	LBL_NONE,
		SA_GOTO,	LBL_REG_EXP,

    SA_LABEL,	LBL_NS_FAIL,

		RG_NS_FAIL,	LBL_NONE,
		SA_EXIT,	LBL_NONE,

    /* Register server */

    SA_LABEL,	LBL_REG_REG,

        	RG_NS_OPER,	LBL_RG_NSERR1,	
		SA_GOSUB,	LBL_SEND_MSG,
		SA_IFERR,	LBL_RG_NSERR1,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_RG_NSERR1,

    SA_LABEL,	LBL_RG_RECV,

        	RQ_RECV,	LBL_NONE,	
		SA_GOSUB,	LBL_RECV_MSG,
		SA_IFERR,	LBL_RG_NSERR1,
		SA_IS_ERRMSG,	LBL_NONE,
		RG_NS_MORE,	LBL_RG_RECV,	/* if not EOG */
		SA_CALLRET,	LBL_DONE,	/* leave connection active */

    SA_LABEL,	LBL_RG_NSERR1,

		RG_NS_FAIL,	LBL_NONE,
		SA_CALLRET,	LBL_DONE,

    /* Update NS */

    SA_LABEL,	LBL_REG_UPD,

		RG_FMT_ACK,	LBL_NONE,
		SA_GOSUB,	LBL_SEND_MSG,
		SA_IFERR,	LBL_RG_NSERR1,
		SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_RG_NSERR1,
    		SA_CALLRET,	LBL_DONE,
    
    /* Shutdown NS connection */

    SA_LABEL,	LBL_REG_DISC,

		RG_IF_NSFAIL,	LBL_RG_DISC,
        	RQ_FMT_RLS,	LBL_NONE,	
		SA_GOSUB,	LBL_SEND_MSG,
		SA_IFERR,	LBL_RG_DISC,
		SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_RG_DISC,

    SA_LABEL,	LBL_RG_DISC,

		GC_DISCONN,	LBL_NONE,
        	GC_RELEASE,	LBL_NONE,	
		SA_CALLRET,	LBL_DONE,

    /* 
    ** GCA_LISTEN: extend a listen and receive peer info. 
    */

    SA_LABEL,	LBL_LISTEN,

        	LS_INIT,	LBL_LS_TRAP,	/* if 1st listen thread */
        	SA_BUFFERS,	LBL_DONE,	/* if no mem */
        	GC_LISTEN,	LBL_NONE,
		SA_SAV_PARMS,	LBL_NONE,
		SA_IFERR,	LBL_DONE,	

    SA_LABEL,	LBL_LS_PEER,			/* Get peer info */

		GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_DONE,
		SA_RECV_DONE,	LBL_NONE,
		SA_RECV_PEER,	LBL_LS_PEER,	/* if need more data */
		SA_IFERR,	LBL_LS_PEER1,
		LS_IS_NOSL,	LBL_LS_CHECK,

    SA_LABEL,	LBL_LS_SL,			/* Security Label */

		GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_DONE,
        	SA_RECV_DONE,	LBL_NONE,
		LS_CHK_SL,	LBL_LS_SL,	/* if need more data */
		SA_IFERR,	LBL_LS_PEER1,

    SA_LABEL,	LBL_LS_CHECK,

        	LS_CHKFAST,	LBL_LS_FAST,	/* if FASTSELECT connection */
        	LS_CHKGCM,	LBL_LS_GCM,	/* if GCM connection */
        	LS_CHK_PEER,	LBL_LS_PEER1,	/* if fail authentication */
		LS_CHKBED,	LBL_LS_PEER1,	/* if BEDCHECK connection */
        	LS_CHKAPI,	LBL_LS_DONE,	/* if API >= 1 */
        	LS_LSNCNFM,	LBL_NONE,	/* peerinfo status */

    /* Send peer info to requestor. */

    SA_LABEL,	LBL_LS_PEER1,

        	LS_SET_PSTAT,	LBL_NONE,	
        	LS_FMT_PEER,	LBL_NONE,
		SA_SEND_PEER,	LBL_NONE,
		SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_DONE,	
        	LS_GET_PSTAT,	LBL_DONE,
		SA_GOTO,	LBL_LS_CMPLT,

    /* Handle incoming FASTSELECT */

    SA_LABEL,	LBL_LS_FAST,

        	LS_RECV_GCM,	LBL_NONE,
		SA_GOSUB,	LBL_RECV_MSG,
		SA_IFERR,	LBL_DONE,
        	LS_CHK_PEER,	LBL_LS_PEER1,	/* if fail authentication */
		LS_REMOTE_GCM,	LBL_LS_DONE,	/* if remote fastselect */
		LS_GCM_PRIV,	LBL_NONE,
        	LS_FMT_PEER,	LBL_NONE,
		SA_SEND_PEER,	LBL_NONE,
		SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_GCM_DONE,	
        	LS_RESP_GCM,	LBL_NONE,
		SA_GOSUB,	LBL_SEND_MSG,
		SA_IFERR,	LBL_GCM_DONE,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_GCM_DONE,
		SA_GOTO,	LBL_GCM_LOOP,

    /* Carry on GCM conversation */

    SA_LABEL,	LBL_LS_GCM,

        	LS_CHK_PEER,	LBL_LS_PEER1,	/* if fail authentication */
		LS_GCM_PRIV,	LBL_NONE,
        	LS_FMT_PEER,	LBL_NONE,
		SA_SEND_PEER,	LBL_NONE,
		SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_GCM_DONE,	

    SA_LABEL,	LBL_GCM_LOOP,

        	LS_RECV_GCM,	LBL_NONE,	
		SA_GOSUB,	LBL_RECV_MSG,
		SA_IFERR,	LBL_GCM_DONE,
		LS_IF_RELEASE,	LBL_GCM_DONE,	/* GCA_RELEASE received */
        	LS_RESP_GCM,	LBL_NONE,
		SA_GOSUB,	LBL_SEND_MSG,
		SA_IFERR,	LBL_GCM_DONE,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_GCM_DONE,
		SA_GOTO,	LBL_GCM_LOOP,

    SA_LABEL,	LBL_GCM_DONE,			/* Complete with NO_PEER */

		LS_CHKBED,	LBL_BEDCHECK,	/* if BEDCHECK connection */
    		LS_NO_PEER,	LBL_NONE,
		SA_COMPLETE,	LBL_NONE,

    /* Bedcheck processing */

    SA_LABEL,	LBL_BEDCHECK,

		SA_GOSUB,	LBL_REG_NS,
    		LS_NO_PEER,	LBL_NONE,
		SA_COMPLETE,	LBL_NONE,

    /* Update NS statistics */

    SA_LABEL,	LBL_LS_DONE,

		RG_IF_NSDISC,	LBL_LS_CMPLT,
		RG_IF_NSFAIL,	LBL_LS_CMPLT,
		RG_INIT_SEND,	LBL_LS_CMPLT,
		SA_CALL,	LBL_REG_UPD,
		SA_CLRERR,	LBL_NONE,

    /* Listen completes. */

    SA_LABEL,	LBL_LS_CMPLT,

        	LS_DONE,	LBL_NONE,	
        	SA_COMPLETE,	LBL_NONE,

    /* Trap Delivery Thread */

    SA_LABEL,	LBL_LS_TRAP,

        	LS_HIJACK,	LBL_NONE,	/* initialize trap thread */
        	LS_DELIVER,	LBL_NONE,

    /* 
    ** GCA_RQRESP: send peer info to requestor. 
    */

    SA_LABEL,	LBL_RQRESP,

        	RR_INIT,	LBL_NONE,	
        	LS_FMT_PEER,	LBL_NONE,	
		SA_SEND_PEER,	LBL_NONE,
		SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_DONE,
        	SA_COMPLETE,	LBL_NONE,

    /* 
    ** GCA_REQUEST 
    */

    SA_LABEL,	LBL_REQUEST,

        	RQ_INIT,	LBL_DONE,	
		RQ_ALLOC,	LBL_DONE,	/* if no mem */
        	SA_BUFFERS,	LBL_RQ_EXIT,	/* if no mem */
        	RQ_IS_NOXLATE,	LBL_RQ_NOXL,	
        	RQ_IS_RSLVD,	LBL_RQ_RSLVD,	
		RQ_ADDR_NSID,	LBL_RQ_EXIT,
        	RQ_IS_NMSVR,	LBL_RQ_CONN,
		RQ_USE_NMSVR,	LBL_RQ_NMSVR,	/* Name Server Resolve */

		RQ_RESOLVE,	LBL_RQ_EXIT,	/* Embedded GCN Resolve */
		SA_GOTO,	LBL_RQ_NEXT,

    /* Connect directly to address given (GCA_NO_XLATE) */

    SA_LABEL,	LBL_RQ_NOXL,

        	RQ_ADDR_NOXL,	LBL_NONE,	
        	SA_GOTO,	LBL_RQ_CONN,	

    /* Connect to remote address in partner_name (GCA_RQ_RSLVD) */

    SA_LABEL,	LBL_RQ_RSLVD,

        	RQ_LOAD_PRTNR,	LBL_RQ_EXIT,	
        	SA_GOTO,	LBL_RQ_NEXT,	

    /* Connect to name server to resolve target */

    SA_LABEL,	LBL_RQ_NMSVR,

        	GC_REQUEST,	LBL_NONE,
		SA_SAV_PARMS,	LBL_NONE,
		SA_IFERR,	LBL_RQ_ERROR,	

        	RQ_FMT_PEER2,	LBL_RQ_ERROR,	/* if no mem */
        	RQ_PROT_MINE,	LBL_NONE,	
		SA_SEND_PEER,	LBL_NONE,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_RQ_ERROR,	

    SA_LABEL,	LBL_RQ_PEER,			/* Get peer info */

		GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_RQ_ERROR,
		SA_RECV_DONE,	LBL_NONE,
		SA_RECV_PEER,	LBL_RQ_PEER,	/* if need more data */
		SA_IFERR,	LBL_RQ_ERROR,
        	RQ_CHECKPEER,	LBL_RQ_ERROR,

        	RQ_FMT_RSLV,	LBL_RQ_ERROR,
		SA_GOSUB,	LBL_SEND_MSG,
		SA_IFERR,	LBL_RQ_ERROR,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_RQ_ERROR,

        	RQ_RECV,	LBL_NONE,	
		SA_GOSUB,	LBL_RECV_MSG,
		SA_IFERR,	LBL_RQ_ERROR,
		SA_IS_ERRMSG,	LBL_NONE,
        	RQ_LOAD_RSLVD,	LBL_NONE,
        	RQ_SAVERR,	LBL_NONE,

        	RQ_FMT_RLS,	LBL_NONE,	
		SA_GOSUB,	LBL_SEND_MSG,
		SA_IFERR,	LBL_RQ_ERROR,
		SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_RQ_ERROR,

		GC_DISCONN,	LBL_NONE,
        	GC_RELEASE,	LBL_NONE,	
		RQ_GETERR,	LBL_NONE,
		SA_IFERR,	LBL_RQ_EXIT,

    /* Connect to next in address list */

    SA_LABEL,	LBL_RQ_NEXT,

        	RQ_ADDR_NEXT,	LBL_RQ_EXIT,	/* if error */

    SA_LABEL,	LBL_RQ_CONN,

        	GC_REQUEST,	LBL_NONE,
		SA_SAV_PARMS,	LBL_NONE,
		SA_IFERR,	LBL_RQ_DISCON,	

        	RQ_FMT_PEER1,	LBL_RQ_DISCON,	
        	RQ_PROT_USER,	LBL_NONE,	
		SA_SEND_PEER,	LBL_NONE,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_RQ_DISCON,	
        	RQ_CHK_FAST,	LBL_RQ_FAST,	

    SA_LABEL,	LBL_RQ_PEER1,			/* Get peer info */

		GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_RQ_DISCON,
		SA_RECV_DONE,	LBL_NONE,
		SA_RECV_PEER,	LBL_RQ_PEER1,	/* if need more data */
		SA_IFERR,	LBL_RQ_DISCON,
        	RQ_CHECKPEER,	LBL_RQ_DISCON,

		/* Connection established */
    		RG_IF_NOREG,	LBL_RQ_DONE,
		RG_IF_NSDISC,	LBL_RQ_DONE,
		RG_IF_NSFAIL,	LBL_RQ_DONE,
		RG_INIT_SEND,	LBL_RQ_DONE,
		SA_CALL,	LBL_REG_UPD,
		SA_CLRERR,	LBL_NONE,

    SA_LABEL,	LBL_RQ_DONE,

		RQ_FREE,	LBL_NONE,
        	RQ_COMPLETE,	LBL_NONE,	
		SA_COMPLETE,	LBL_NONE,

    /* Failed connect - disconn, release, & try again */

    SA_LABEL,	LBL_RQ_DISCON,

        	RQ_SAVERR,	LBL_NONE,
# ifdef NT_GENERIC
                GC_DELPASSWD,   LBL_NONE,
# endif
		GC_DISCONN,	LBL_NONE,
        	GC_RELEASE,	LBL_NONE,	
		RQ_GETERR,	LBL_NONE,
        	RQ_CHK_FAIL,	LBL_RQ_EXIT,	/* if done, s = c */
        	SA_GOTO,	LBL_RQ_NEXT,	

    /* Handle GCA_REQUEST FASTSELECT exchange */

    SA_LABEL,	LBL_RQ_FAST,

        	RQ_SEND_FAST,	LBL_NONE,	
		SA_GOSUB,	LBL_SEND_MSG,
		SA_IFERR,	LBL_RQ_ERROR,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_RQ_ERROR,

    SA_LABEL,	LBL_RQ_PEER2,			/* Get peer info */

		GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_RQ_ERROR,
		SA_RECV_DONE,	LBL_NONE,
		SA_RECV_PEER,	LBL_RQ_PEER2,	/* if need more data */
		SA_IFERR,	LBL_RQ_ERROR,
        	RQ_CHECKPEER,	LBL_RQ_ERROR,

        	RQ_RECV_FAST,	LBL_NONE,	
		SA_GOSUB,	LBL_RECV_MSG,
		SA_IFERR,	LBL_RQ_ERROR,

        	RQ_FMT_RLS,	LBL_NONE,	
		SA_GOSUB,	LBL_SEND_MSG,
		SA_GOSUB,	LBL_SEND_BUF,

		GC_DISCONN,	LBL_NONE,
        	GC_RELEASE,	LBL_NONE,	
		RQ_FREE,	LBL_NONE,
		RQ_COMP_FAST,	LBL_NONE,
		SA_COMPLETE,	LBL_NONE,

    /* Name server error - disconn, release, & complete */

    SA_LABEL,	LBL_RQ_ERROR,

        	RQ_SAVERR,	LBL_NONE,
		GC_DISCONN,	LBL_NONE,
        	GC_RELEASE,	LBL_NONE,	
        	RQ_GETERR,	LBL_NONE,	

    /* Request completion (error) */

    SA_LABEL,	LBL_RQ_EXIT,

		RQ_FREE,	LBL_NONE,
        	RQ_COMP_ERR,	LBL_NONE,	
		SA_COMPLETE,	LBL_NONE,

    /* 
    ** GCA_SEND
    */

    SA_LABEL,	LBL_SEND,

        	SD_INIT,	LBL_NONE,
           	SD_CKIACK,	LBL_DONE,	/* if need IACK and not IACK */
        	SD_NEEDTD,	LBL_SD_DOTD,	/* if GCA_TO_DESCR owed */

    SA_LABEL,	LBL_SD_HDR,

         	SD_DOUSR,	LBL_NONE,
        	SD_SVCHDR,	LBL_NONE,

    SA_LABEL,	LBL_SD_HDR1,

        	SD_BUFF1,	LBL_SD_USR,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_DONE,
        	SA_GOTO,	LBL_SD_HDR1,

    SA_LABEL,	LBL_SD_USR,

		SD_IS_FMT,	LBL_SD_FMT,
        	SD_SVCUSR,	LBL_NONE,

    SA_LABEL,	LBL_SD_USR1,			/* send unformatted data */

        	SD_BUFF,	LBL_SD_FLUSH,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_DONE,
        	SA_GOTO,	LBL_SD_USR1,

    SA_LABEL,	LBL_SD_FMT,			/* send formatted data */

		SD_FMT,		LBL_SD_FLUSH,	/* doc->state != DOC_MOREOUT */
		SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_DONE,
        	SD_SVCHDR,	LBL_NONE,	/* Header for next segment, */
        	SD_BUFF1,	LBL_SD_FMT,	/* should always fit/branch! */
        	SA_GOTO,	LBL_SD_FMT,	/* Shouldn't get here, don't
						   know what to do if we do. */
    SA_LABEL,	LBL_SD_FLUSH,

		SD_CKATTN,	LBL_NONE,	/* check for ATTENTION */
        	SD_NOFLUSH,	LBL_SD_DONE,	/* if we can skip flushing */
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_DONE,	/* on error */

    /* Send post-processing. */

    SA_LABEL,	LBL_SD_DONE,

        	SD_DONE,	LBL_NONE,	
        	SA_COMPLETE,	LBL_NONE,	

    /* Send GCA_TO_DESCR message.  */

    SA_LABEL,	LBL_SD_DOTD,

         	SD_DOTD,	LBL_DONE,
		SA_GOSUB,	LBL_SEND_MSG,
		SA_IFERR,	LBL_DONE,
        	SA_GOTO,	LBL_SD_HDR,	

    /* 
    ** GCA_RECEIVE 
    */

    SA_LABEL,	LBL_RECEIVE,

        	RV_INIT,	LBL_NONE,
        	RV_IS_SPRG,	LBL_RV_PURGE,	/* if we sent ATTN message */

    SA_LABEL,	LBL_RV_READ,

		RV_DOUSR,	LBL_NONE,
		RV_IS_MORE,	LBL_RV_USR,	/* if mid-message */

    SA_LABEL,	LBL_RV_HDR,

        	RV_SVCHDR,	LBL_NONE,	

    SA_LABEL,	LBL_RV_HDR1,

        	RV_BUFF,	LBL_RV_USR,	/* if request filled */
        	GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_DONE,	/* if error */
        	SA_RECV_DONE,	LBL_NONE,
        	SA_NOCHOP,	LBL_RV_HDR1,	/* if no chop mark */
		RV_IS_PRG,	LBL_RV_PURGED,	/* if purging */
		RV_CLRPRG,	LBL_NONE,
		SA_GOTO,	LBL_RV_READ,

    SA_LABEL,	LBL_RV_USR,

		RV_IS_FMT,	LBL_RV_FMT,	/* if GCA_FORMATTED */
        	RV_SVCUSR,	LBL_NONE,	

    SA_LABEL,	LBL_RV_USR1,

        	RV_BUFF,	LBL_RV_CONCAT,	/* if request filled */
        	GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_DONE,	/* if error */
        	SA_RECV_DONE,	LBL_NONE,
        	SA_NOCHOP,	LBL_RV_USR1,	/* if no chop mark */
		RV_IS_PRG,	LBL_RV_PURGED,	/* if purging */
		RV_CLRPRG,	LBL_NONE,
		SA_GOTO,	LBL_RV_READ,

    SA_LABEL,	LBL_RV_CONCAT,

		RV_CONCAT,	LBL_RV_HDR,	/* if not end_of_data */
		SA_GOTO,	LBL_RV_DONE,

    SA_LABEL,	LBL_RV_FMT,

        	RV_FMT,		LBL_RV_DONE,	/* if request filled */
		RV_IS_HDR,	LBL_RV_HDR,	/* if header expected */
        	GC_RECEIVE,	LBL_NONE,	/* read rest of message */
		SA_IFERR,	LBL_DONE,	/* if error */
        	SA_RECV_DONE,	LBL_NONE,
        	SA_NOCHOP,	LBL_RV_FMT,	/* if no chop mark */
		RV_IS_PRG,	LBL_RV_PURGED,	/* if purging */
		RV_CLRPRG,	LBL_NONE,
		SA_GOTO,	LBL_RV_READ,

    SA_LABEL,	LBL_RV_DONE,

		RV_IS_PRG,	LBL_RV_READ,	/* if purging */
		RV_IS_HET,	LBL_RV_READ,	/* if GCA_GOTOHET */
        	RV_DONE,	LBL_NONE,
        	SA_COMPLETE,	LBL_NONE,

    /* request issued after ATTN message sent - read until chop */

    SA_LABEL,	LBL_RV_PURGE,

		SA_RECV_BUFF,	LBL_NONE,
        	GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_DONE,	/* if error */
		SA_RECV_DONE,	LBL_NONE,
		SA_NOCHOP,	LBL_RV_PURGE,	/* if no chop mark */
        	RV_CLRPRG,	LBL_NONE,	
		SA_GOTO,	LBL_RV_READ,

    /* request purged */

    SA_LABEL,	LBL_RV_PURGED,

		RV_CLRPRG,	LBL_NONE,
        	RV_PURGED,	LBL_NONE,
		SA_COMPLETE,	LBL_NONE,

    /* 
    ** GCA_DISASSOC 
    */

    SA_LABEL,	LBL_DISASSOC,

    		RG_IF_NOREG,	LBL_DA_DISC,
		RG_IF_NSDISC,	LBL_DA_DISC,
		RG_IF_NSFAIL,	LBL_DA_DISC,
		RG_INIT_SEND,	LBL_DA_DISC,
		SA_CALL,	LBL_REG_UPD,
		SA_CLRERR,	LBL_NONE,

    SA_LABEL,	LBL_DA_DISC,

        	DA_DISCONN,	LBL_NONE,
		GC_DISCONN,	LBL_NONE,
        	GC_RELEASE,	LBL_NONE,	
        	DA_COMPLETE,	LBL_NONE,	
		SA_COMPLETE,	LBL_NONE,

    /*
    ** GCA_INITIATE
    */
    
    SA_LABEL,	LBL_INITIATE,		

		SA_INITIATE,	LBL_NONE,
        	SA_COMPLETE,	LBL_NONE,

    /*
    ** GCA_TERMINATE 
    */

    SA_LABEL,	LBL_TERMINATE,		

		RG_IF_NSDISC,	LBL_TM_TERM,
		RG_INIT_CONN,	LBL_TM_TERM,
		SA_CALL,	LBL_REG_DISC,
		SA_CLRERR,	LBL_NONE,

    SA_LABEL,	LBL_TM_TERM,

		RG_FREE,	LBL_NONE,
		RG_FREE_ACB,	LBL_NONE,
		SA_TERMINATE,	LBL_NONE,

    /*
    ** GCA_SAVE 
    */

    SA_LABEL,	LBL_SAVE,		

		SA_SAVE,	LBL_NONE,
        	SA_COMPLETE,	LBL_NONE,

    /*
    ** GCA_RESTORE 
    */

    SA_LABEL,	LBL_RESTORE,		

		SA_RESTORE,	LBL_NONE,
		SA_BUFFERS,	LBL_DONE,	/* if no mem */
		SA_SAV_PARMS,	LBL_NONE,
		SA_IFERR,	LBL_DONE,
        	SA_COMPLETE,	LBL_NONE,

    /*
    ** GCA_FORMAT 
    */

    SA_LABEL,	LBL_FORMAT,		

		SA_FORMAT,	LBL_NONE,
        	SA_EXIT,	LBL_NONE,

    /*
    ** GCA_INTERPRET 
    */

    SA_LABEL,	LBL_INTERPRET,		

		SA_INTERPRET,	LBL_NONE,
        	SA_EXIT,	LBL_NONE,

    /*
    ** GCA_ATTRIBS 
    */

    SA_LABEL,	LBL_ATTRIBS,		

		SA_ATTRIBS,	LBL_NONE,
        	SA_COMPLETE,	LBL_NONE,

    /*
    ** GCA_REG_TRAP 
    */

    SA_LABEL,	LBL_REG_TRAP,		

		SA_REG_TRAP,	LBL_NONE,
        	SA_EXIT,	LBL_NONE,

    /*
    ** GCA_UNREG_TRAP 
    */

    SA_LABEL,	LBL_UNREG_TRAP,		

		SA_UNREG_TRAP,	LBL_NONE,
        	SA_EXIT,	LBL_NONE,

    /* 
    ** Generic complete 
    */

    SA_LABEL,	LBL_DONE,

        	SA_COMPLETE,	LBL_NONE,	/* never */

    /*
    ** Subroutines 
    */

    /* 
    ** Send Buffering Subroutine 
    **
    ** Buffers GCA message header and user data.  Sends 
    ** data if buffer fills before all data is buffered.  
    ** Call LBL_SEND_BUF following this routine if all
    ** data is to be flushed.
    */

    SA_LABEL,	LBL_SEND_MSG,

        	SD_SVCHDR,	LBL_NONE,

    SA_LABEL,	LBL_SEND_HDR,

        	SD_BUFF,	LBL_SEND_USR,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_RETURN,
        	SA_GOTO,	LBL_SEND_HDR,

    SA_LABEL,	LBL_SEND_USR,

        	SD_SVCUSR,	LBL_NONE,

    SA_LABEL,	LBL_SEND_USR1,

        	SD_BUFF,	LBL_RETURN,
        	SA_GOSUB,	LBL_SEND_BUF,
		SA_IFERR,	LBL_RETURN,
        	SA_GOTO,	LBL_SEND_USR1,

    /*
    ** Send Data Subroutine.
    **
    ** Send buffered data to partner in chunks
    ** acceptable to the underlaying IPC mechanism.
    */

    SA_LABEL,	LBL_SEND_BUF,

		GC_SEND,	LBL_NONE,
		GC_ASYNC,	LBL_NONE,
		SD_MORE,	LBL_SEND_BUF,
		SA_RETURN,	LBL_DONE,

    /* 
    ** Generic Receive Subroutine.
    */

    SA_LABEL,	LBL_RECV_MSG,

        	RV_SVCHDR,	LBL_NONE,	

    SA_LABEL,	LBL_RECV_HDR,

        	RV_BUFF,	LBL_RECV_USR,	/* if request filled */
        	GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_RETURN,	/* if error */
        	SA_RECV_DONE,	LBL_NONE,
		SA_GOTO,	LBL_RECV_HDR,

    SA_LABEL,	LBL_RECV_USR,

        	RV_SVCUSR,	LBL_NONE,	

    SA_LABEL,	LBL_RECV_USR1,

        	RV_BUFF,	LBL_RECV_DONE,	/* if request filled */
        	GC_RECEIVE,	LBL_NONE,
		SA_IFERR,	LBL_RETURN,	/* if error */
        	SA_RECV_DONE,	LBL_NONE,
		SA_GOTO,	LBL_RECV_USR1,

    SA_LABEL,	LBL_RECV_DONE,

		RV_CONCAT,	LBL_RECV_MSG,	/* if not end_of_data */
    
    /* Subroutine return */

    SA_LABEL,	LBL_RETURN,

		SA_RETURN,	LBL_DONE,

};


/*
** Macros for accessing GCA function parameters.
*/
# define IN_PARMS_MACRO ((GCA_IN_PARMS *)(svc_parms->parameter_list))
# define DA_PARMS_MACRO	((GCA_DA_PARMS *)(svc_parms->parameter_list))
# define LS_PARMS_MACRO	((GCA_LS_PARMS *)(svc_parms->parameter_list))
# define RR_PARMS_MACRO	((GCA_RR_PARMS *)(svc_parms->parameter_list))
# define RV_PARMS_MACRO	((GCA_RV_PARMS *)(svc_parms->parameter_list))
# define SD_PARMS_MACRO	((GCA_SD_PARMS *)(svc_parms->parameter_list))
# define RQ_PARMS_MACRO	((GCA_RQ_PARMS *)(svc_parms->parameter_list))
# define FS_PARMS_MACRO	((GCA_FS_PARMS *)(svc_parms->parameter_list))
# define RG_PARMS_MACRO	((GCA_RG_PARMS *)(svc_parms->parameter_list))
# define AT_PARMS_MACRO	((GCA_AT_PARMS *)(svc_parms->parameter_list))
# define FO_PARMS_MACRO	((GCA_FO_PARMS *)(svc_parms->parameter_list))
# define IT_PARMS_MACRO	((GCA_IT_PARMS *)(svc_parms->parameter_list))


/*
** Misc constants.
*/

static char	*Yes = "Y";
static char	*No = "N";


/*
** gca_service() - GCA asynchronous service program interpreter
*/

VOID
gca_service( PTR closure )
{
    GCA_SVC_PARMS	*svc_parms = (GCA_SVC_PARMS *)closure;
    GCA_CB		*gca_cb = (GCA_CB *)svc_parms->gca_cb;
    GCA_ACB             *acb = svc_parms->acb;
    GCA_PEER_INFO 	*peerinfo = NULL;
    GCA_PEER_INFO 	*peersend = NULL;
    GCA_SR_SM		*rsm = NULL;
    GCA_SR_SM		*ssm = NULL;
    GCA_MSG_HDR		*rsmhdr = NULL;
    GCA_MSG_HDR		*ssmhdr = NULL;
    GCA_RQCB		*rqcb = NULL;
    bool	  	branch;
    i4			len;
    bool                send_active_set = FALSE;
    i4                  state;

    if ( acb )
    {
	peerinfo = &acb->gca_peer_info;
	peersend = &acb->gca_my_info;
	rsm = &acb->recv[ svc_parms->gc_parms.flags.flow_indicator ];
	ssm = &acb->send[ svc_parms->gc_parms.flags.flow_indicator ];
	rsmhdr = &rsm->hdr;
	ssmhdr = &ssm->hdr;
	rqcb = acb->connect;
    }

    /*
    ** (Re)set parmlist status on initial entry or GCA_RESUME
    */
    svc_parms->in_use = TRUE;
    ((GCA_ALL_PARMS *)(svc_parms->parameter_list))->gca_status = 
							E_GCFFFF_IN_PROCESS;
top:

    state = GCA_INCREMENT_STATE( svc_parms->state );

    /*
    ** Peform this state's action.
    */
    if ( gca_states[ state ].action != SA_LABEL )
	GCA_DEBUG_MACRO(3)( "%04d     GCA %s status %x (%d)\n",
			    (i4)(acb ? acb->assoc_id : -1),
			    gca_names[ gca_states[ state ].action ],
			    svc_parms->gc_parms.status,
			    state );

    branch = FALSE;

    switch( gca_states[ state ].action )
    {
    case SA_NOOP:
	/* Do nothing */
	break;

    case SA_INIT:
	svc_parms->sp = 0;

	if ( ! labels[1] )
	{
	    i4  i;

	    for( i = 0; i < sizeof( gca_states ) / sizeof( *gca_states ); i++ )
		if( gca_states[i].action == SA_LABEL )
		    labels[ gca_states[i].label ] = i + 1;
	}
	break;

    case SA_COMPLETE:
	gca_complete( svc_parms );

        if (send_active_set)
           acb->flags.send_active = FALSE;

	return;

    case SA_JUMP:
	/* 
	** The states after SA_JUMP are  
	** a jump table based on request.
	*/
	svc_parms->state += svc_parms->service_code;
	branch = TRUE;
	break;

    case SA_EXIT:
	/*
	** Exit point for routines which don't make callbacks.
	*/
	svc_parms->in_use = FALSE;
	return;

    case SA_GOTO:
	branch = TRUE;
	break;

    case SA_CALL:
	/*
	** Save current service parms and call routine
	** with an alternate service parms (context switch).
	**
	** Note: No context switch occurs if alternate service
	** parms is NULL or same as current service parms.  In
	** this case CALL/CALLRET is same as GOSUB/RETURN.
	*/
	if ( svc_parms->call_parms  &&  svc_parms != svc_parms->call_parms )
	{
	    /*
	    ** Activate alternate service parms (switch context)
	    ** and start processing at the call branch label.
	    */
	    svc_parms->call_parms->call_parms = svc_parms;
	    svc_parms->call_parms->state = 
		labels[ gca_states[ state ].label ];
	    svc_parms->call_parms->service_code = svc_parms->service_code;
	    svc_parms->call_parms->parameter_list = svc_parms->parameter_list;
	    svc_parms->call_parms->gc_parms.gca_cl_completion = 
		svc_parms->gc_parms.gca_cl_completion;
	    svc_parms->call_parms->gc_parms.closure = 
		(PTR)svc_parms->call_parms;
	    svc_parms->call_parms->gc_parms.flags.run_sync = 
		svc_parms->gc_parms.flags.run_sync;
	    svc_parms->call_parms->gc_parms.flags.new_chop = FALSE;
	    svc_parms->call_parms->gc_parms.flags.remote = FALSE;
	    svc_parms->call_parms->gc_parms.sys_err = 
		svc_parms->gc_parms.sys_err;
	    svc_parms->call_parms->gc_parms.status = 0;
	    gca_service( (PTR)svc_parms->call_parms );
	    return;
	}

	svc_parms->call_parms = NULL;	/* No context switch */

	/*
	** Fall through to do GOSUB.
	*/

    case SA_GOSUB:
	if ( svc_parms->sp >= GCA_STACK_SIZE )
	    svc_parms->gc_parms.status = E_GC0001_ASSOC_FAIL;
	else
	{
	    svc_parms->ss[ svc_parms->sp++ ] = svc_parms->state;
	    branch = TRUE;
	}
	break;

    case SA_CALLRET:
	/*
	** Restore original context and return from call.
	** Status of call is returned in original context.
	**
	** If no context switch occured in CALL, then same as RETURN.
	*/
	if ( svc_parms->call_parms )
	{
	    GCA_SVC_PARMS *call_parms = svc_parms->call_parms;

	    /* Return status */
	    call_parms->gc_parms.status = svc_parms->gc_parms.status;

	    /*
	    ** Clear context switch and return from call.
	    */
	    call_parms->call_parms = NULL;
	    svc_parms->call_parms = NULL;
	    svc_parms->in_use = FALSE;
	    gca_service( (PTR)call_parms );
	    return;
	}

	/*
	** Fall through to do RETURN.
	*/

    case SA_RETURN:
	/*
	** Return from GOSUB.
	*/
	if ( svc_parms->sp > 0 )
	    svc_parms->state = svc_parms->ss[ --svc_parms->sp ];
	else
	{
	    svc_parms->gc_parms.status = E_GC0001_ASSOC_FAIL;
	    branch = TRUE;
	}
	break;

    case SA_FORK:
	/*
	** Fork (independently) alternate service parms.
	** Clear association with alternate service parms.
	*/
	if ( ! svc_parms->call_parms  ||  svc_parms == svc_parms->call_parms )
	{
	    svc_parms->call_parms = NULL;
	    break;
	}

	if ( ! svc_parms->call_parms->in_use )
	    svc_parms->call_parms->in_use = TRUE;
	else 
	{
	    svc_parms->gc_parms.status = E_GC0024_DUP_REQUEST;
	    svc_parms->call_parms = NULL;
	    break;
	}

	svc_parms->call_parms->state = 
	    labels[ gca_states[ state ].label ];
	svc_parms->call_parms->gc_parms.gca_cl_completion = gca_service;
	svc_parms->call_parms->gc_parms.closure = (PTR)svc_parms->call_parms;
	svc_parms->call_parms->gc_parms.flags.run_sync = FALSE;
	svc_parms->call_parms->gc_parms.flags.new_chop = FALSE;
	svc_parms->call_parms->gc_parms.flags.remote = FALSE;
	svc_parms->call_parms->gc_parms.status = 0;
	gca_service( (PTR)svc_parms->call_parms );
	svc_parms->call_parms = NULL;
	break;

    case SA_CLRERR:
    	svc_parms->gc_parms.status = OK;
	break;

    case SA_IFERR:
	/*
	** Branch if error.
	*/
	branch = (svc_parms->gc_parms.status != OK);
	break;

    case SA_IS_ERRMSG:
	/*
	** Load status from GCA_ERROR message
	*/
	if ( rsmhdr->msg_type == GCA_ERROR )
	{
	    char	*p = rsm->usrbuf;
	    i4		val;

	    p += gcu_get_int( p, &val );	/* toss gca_l_e_element */
	    p += gcu_get_int( p, &svc_parms->gc_parms.status );
	}
	break;

    case SA_SAV_PARMS:
	/*
	** Save GC parameters in ACB.
	*/
	svc_parms->gc_parms.time_out = -1;
	acb->gc_cb = svc_parms->gc_parms.gc_cb;
	acb->gc_size_advise = svc_parms->gc_parms.size_advise
			      ? svc_parms->gc_parms.size_advise 
			      : acb->size_advise;

	/*
	** Reconcile CL and our size advise.  If the CL size is smaller,
	** arrange for our size to be a multiple of the CL size (making
	** sure that it's large enough for a FASTSELECT msg).  If the
	** CL size is larger, we might as well just use it.
	*/
	if ( acb->gc_size_advise < acb->size_advise )
	{
	    acb->size_advise -= acb->size_advise % acb->gc_size_advise;
	    while (acb->size_advise < GCA_FS_MAX_DATA + sizeof(GCA_MSG_HDR))
		acb->size_advise += acb->gc_size_advise;
	}
	else if (acb->size_advise < acb->gc_size_advise)
	    acb->size_advise = acb->gc_size_advise;

	/* If there are buffers now, and if our size advice changed
	** now that we know what the CL wants, reallocate buffers to
	** the new size.
	*/
	if (acb->buffers != NULL
	  && acb->size_advise != acb->recv[ GCA_NORMAL ].bufsiz)
	{
	    acb->recv[ GCA_NORMAL ].bufsiz =
	    acb->send[ GCA_NORMAL ].bufsiz = acb->size_advise;
	    /* GCA_EXPEDITED stays the same size */
	    gca_acb_buffers(acb);	/* Reallocate */
	    if (acb->buffers == NULL)
		svc_parms->gc_parms.status = E_GC0013_ASSFL_MEM;
	}
	break;

    case SA_BUFFERS:
	/*
	** Allocate GCA standard size buffers.  Later, if request or listen
	** is advising larger ones, SA_SAV_PARMS can reallocate.  Make sure
	** data part of size is large enough for a FASTSELECT message.
	** for a FASTSELECT message.
	**
	** We don't know if our CL needs headers, assume it does.  (bufsiz
	** does not include the CL header.)
	*/
	len = GCA_NORM_SIZE - GCA_CL_HDR_SZ;
	if (len < GCA_FS_MAX_DATA + sizeof(GCA_MSG_HDR))
	    len = GCA_FS_MAX_DATA + sizeof(GCA_MSG_HDR);
	acb->size_advise = 
	acb->recv[ GCA_NORMAL ].bufsiz = 
	acb->send[ GCA_NORMAL ].bufsiz = len;
	acb->recv[ GCA_EXPEDITED ].bufsiz = 
	acb->send[ GCA_EXPEDITED ].bufsiz = GCA_EXP_SIZE;

	gca_acb_buffers(acb);

	/* Allocate DOC's for message encoding/decoding */
	acb->recv[ GCA_NORMAL ].doc = gca_alloc( sizeof( GCO_DOC ) );
	acb->recv[ GCA_EXPEDITED ].doc = gca_alloc( sizeof( GCO_DOC ) );
	acb->send[ GCA_NORMAL ].doc = gca_alloc( sizeof( GCO_DOC ) );
	acb->send[ GCA_EXPEDITED ].doc = gca_alloc( sizeof( GCO_DOC ) );

	if ( ! acb->buffers  ||
	     ! acb->recv[GCA_NORMAL].doc || ! acb->recv[GCA_EXPEDITED].doc  ||
	     ! acb->send[GCA_NORMAL].doc || ! acb->send[GCA_EXPEDITED].doc )
	{
	    svc_parms->gc_parms.status = E_GC0013_ASSFL_MEM;
	    branch = TRUE;
	    break;
	}

	((GCO_DOC*)acb->recv[ GCA_NORMAL ].doc)->state = 
	((GCO_DOC*)acb->recv[ GCA_EXPEDITED ].doc)->state = 
	((GCO_DOC*)acb->send[ GCA_NORMAL ].doc)->state = 
	((GCO_DOC*)acb->send[ GCA_EXPEDITED ].doc)->state = DOC_IDLE;
	break;

    case SA_RECV_BUFF:
	/*
	** Prepare the receive buffer.
	*/
	rsm->bufptr = rsm->bufend = rsm->buffer;
	break;

    case SA_RECV_DONE:
	/*
	** GCreceive complete.
	*/

	if ( IIGCa_global.gca_trace_level >= 5 )
	    gcx_tdump( svc_parms->gc_parms.svc_buffer, 
		       svc_parms->gc_parms.rcv_data_length );

	IIGCa_global.gca_msgs_in++;
	IIGCa_global.gca_data_in += ( IIGCa_global.gca_data_in_bits + 
				      svc_parms->gc_parms.rcv_data_length ) 
				    / 1024;
	IIGCa_global.gca_data_in_bits = ( IIGCa_global.gca_data_in_bits + 
					  svc_parms->gc_parms.rcv_data_length ) 
					% 1024;

	if ( rsm->bufend > rsm->buffer )
	{
	    /*
	    ** Restore the data saved in GC_RECEIVE.
	    */
	    u_i2 len = min( rsm->bufend - rsm->buffer, GCA_CL_HDR_SZ );
	    MEcopy( (PTR)(rsm->buffer - GCA_CL_HDR_SZ), len, 
		    (PTR)(rsm->bufend - len) ); 
	}

	svc_parms->gc_parms.time_out = -1;
	rsm->bufend += svc_parms->gc_parms.rcv_data_length;
	break;

    case SA_NOCHOP:
	/*
	** Branch if no chop mark.
	*/
	branch = ( ! svc_parms->gc_parms.flags.new_chop );
	break;

    case SA_RECV_PEER:
    {
	/*
	** Validate and save peer info.
	*/
	i4 version;

	if ( ! gca_check_peer( rsm->bufptr, rsm->bufend - rsm->bufptr ) )
	{
	    branch = TRUE;
	    break;
	}

	/*
	** All peer info structures begin with two i4's.  
	** The second i4 is always the peer info version.
	*/
	MEcopy( (PTR)(rsm->bufptr + sizeof( i4 )), sizeof(i4), (PTR)&version );

	switch( version )
	{
	case GCA_IPC_PROTOCOL_6 :
	    svc_parms->gc_parms.status = 
		gca_load_v6_peer( acb, &rsm->bufptr, peerinfo );
	    break;

	case GCA_IPC_PROTOCOL_7 :
	    svc_parms->gc_parms.status = 
		gca_load_v7_peer( acb, &rsm->bufptr, peerinfo );
	    break;

	default :
	    svc_parms->gc_parms.status =
		gca_load_v5_peer( acb, &rsm->bufptr, peerinfo );
	    break;
	}

	/*
	** Empty buffer if an error has occured.
	*/
	if ( svc_parms->gc_parms.status != OK  ||  rsm->bufptr >= rsm->bufend )
	    rsm->bufptr = rsm->bufend = rsm->buffer;
	break;
    }
    case SA_SEND_PEER:		/* Prepare to send the peer info.  */
	/*
	** The send buffer may contain fastselect info, so
	** we need to use the receive buffer to hold the
	** peer info.  The following is technically wrong,
	** since ssm->bufptr should point in ssm->buffer,
	** but will be handled properly by GC_SEND which
	** will set ssm->bufptr = ssm->buffer when the
	** peer info has been sent.
	*/
	ssm->bufptr = rsm->buffer;

	switch( peerinfo->gca_version )
	{
	case GCA_IPC_PROTOCOL_6 :
	    ssm->bufend = ssm->bufptr + 
	    		  gca_frmt_v6_peer( acb, peersend, ssm->bufptr );
	    break;

	case GCA_IPC_PROTOCOL_7 :
	    ssm->bufend = ssm->bufptr + 
	    		  gca_frmt_v7_peer( acb, peersend, ssm->bufptr );
	    break;

	default :
	    ssm->bufend = ssm->bufptr + 
	    		  gca_frmt_v5_peer( acb, peersend, ssm->bufptr );
	    break;
	}
	break;

    case RG_IF_NSONLY:
	/* 
	** GCA_RG_NS_ONLY: re-register with Name Server.
	*/
	branch = ((RG_PARMS_MACRO->gca_modifiers & GCA_RG_NS_ONLY) != 0);
	break;

    case RG_IF_NONS:
	/* 
	** GCA_RG_NO_NS: don't register with name server.  
	*/
	branch = ((RG_PARMS_MACRO->gca_modifiers & GCA_RG_NO_NS) != 0);
	break;

    case RG_IF_REG:
	/*
	** Branch if registered.
	**
	** Error is returned during REGISTER requests,
	** otherwise silent.
	*/
	if ( *gca_cb->gca_listen_address != '\0' )
	{
	    if ( svc_parms->service_code == GCA_REGISTER )
		svc_parms->gc_parms.status = E_GC001A_DUP_REGISTER;
	    branch = TRUE;
	}
        break;

    case RG_IF_NOREG:
	/*
	** Branch if not registered.
	**
	** Error is returned during REGISTER requests,
	** otherwise silent.
	*/
	if ( *gca_cb->gca_listen_address == '\0' )
	{
	    if ( svc_parms->service_code == GCA_REGISTER )
		svc_parms->gc_parms.status = E_GC000F_NO_REGISTER;
	    branch = TRUE;
	}
        break;

    case RG_IF_SELF:
	/*
	** Branch if partner address is our address.
	*/
    	branch = ( ! STcompare( svc_parms->gc_parms.partner_name, 
				gca_cb->gca_listen_address ) );
	break;

    case RG_IF_NSOK:
    	/*
	** Branch if NS connection is OK (see RG_NS_FAIL).
	*/
	branch = (gca_cb->gca_reg_parm.gca_status != E_GC0023_ASSOC_RLSED);
	break;

    case RG_IF_NSFAIL:
    	/*
	** Branch if NS connection has failed (see RG_NS_FAIL).
	*/
	branch = (gca_cb->gca_reg_parm.gca_status == E_GC0023_ASSOC_RLSED);
	break;

    case RG_IF_NSDISC:
    	/*
	** Branch if no Name Server connection.
	*/
	branch = ! (gca_cb->gca_reg_acb  &&  gca_cb->gca_reg_acb->gc_cb);
	break;

    case RG_NS_MORE:
    	/*
	** Branch if more NS messages (not EOG).
	*/
	branch = (! rsmhdr->flags.end_of_group);
	break;

    case RG_FMT_PEER:
	/*
	** Init peersend for register to Name Server.
	*/
	peersend->gca_flags = 0;
	peersend->gca_status = OK;
	gca_save_peer_user( peersend, NULL );
	peersend->gca_aux_count = 0;
	acb->gca_aux_len = 0;

	/*
	** For register, we authenticate the user ID we are
	** running under.  Also generate and send a server
	** key for future server authentication.
	*/
	if ( (svc_parms->gc_parms.status = 
		gca_usr_auth( acb, ssm->bufsiz, (PTR)ssm->buffer )) == OK  &&
	     (svc_parms->gc_parms.status = 
		gca_srv_key( acb, gca_cb->gca_listen_address )) == OK )
	    peersend->gca_aux_count += 2;
	else
	    branch = TRUE;
	break;

    case RG_NS_OPER:
	/* 
	** Construct the GCN_NS_OPER message for Name Server.	
	*/
	{
	    char **vec;
	    i4  l_vec;

	    if( gca_cb->gca_reg_parm.gca_l_so_vector )
	    {
		vec = gca_cb->gca_reg_parm.gca_served_object_vector;
		l_vec = gca_cb->gca_reg_parm.gca_l_so_vector;
	    }
	    else
	    {
		static char *star = "*";
		vec = &star;
		l_vec = 1;
	    }

	    /*
	    ** Construct the GCN_NS_OPER message from the parm list.
	    ** Build the message into the receive buffer, 'cause it's there.
	    */
	    {
		char	*p = rsm->buffer;		/* steal this */
		char	*pend = p + rsm->bufsiz;
		char	*q;
		i4	vec_base;
		i4	i;
		i4	flag = GCN_DEF_FLAG | GCN_PUB_FLAG;

		if ( gca_cb->gca_reg_parm.gca_modifiers & GCA_RG_SOLE_SVR )
		    flag |= GCN_SOL_FLAG;

		flag |= gca_cb->gca_reg_parm.gca_modifiers & GCA_RG_MIB_MASK;

		p += gcu_put_int( p, flag );
		p += gcu_put_int( p, GCN_ADD );
		p += gcu_put_str( p, IIGCa_global.gca_install_id );
		q = p;	/* save for later updating */
		p += gcu_put_int( p, l_vec );

		vec_base = 
		    STlength( gca_cb->gca_srvr_class ) + 
		    STlength( IIGCa_global.gca_uid ) + 
		    STlength( gca_cb->gca_listen_address ) +
		    (4 * sizeof(i4)) + 4;	/* Lengths & null terminators */

		for( i = 0; i < l_vec; i++ )
		{
		    if ( p + vec_base + STlength( vec[i] ) > pend )
			break;
		    p += gcu_put_str( p, gca_cb->gca_srvr_class );
		    p += gcu_put_str( p, IIGCa_global.gca_uid );
		    p += gcu_put_str( p, vec[i] );
		    p += gcu_put_str( p, gca_cb->gca_listen_address );
		}

		gcu_put_int( q, i );

		/* Send message with GCA_SEND. */
		ssmhdr->msg_type = GCN_NS_OPER;
		ssmhdr->data_offset = sizeof( GCA_MSG_HDR );
		ssmhdr->data_length = p - rsm->buffer;
		ssmhdr->buffer_length = ssmhdr->data_offset + 
					ssmhdr->data_length;
		ssmhdr->flags.flow_indicator = GCA_NORMAL;
		ssmhdr->flags.end_of_data = TRUE;
		ssmhdr->flags.end_of_group = TRUE;
		ssm->usrbuf = ssm->usrptr = rsm->buffer;
		ssm->usrlen = ssmhdr->data_length;
	    }
	}
	break;

    case RG_FMT_ACK:
    {
	/*
	** Format GCA_ACK message for sending.
	*/
	GCA_AK_DATA *ack = (GCA_AK_DATA *)rsm->buffer;

	/*
	** Send the current load (active connections) to the
	** Name Server.  If this is a DISASSOC request, be
	** sure to account for the fact that the ACB is not
	** dropped until the request completes.
	*/
	if ( svc_parms->service_code == GCA_DISASSOC )
	    ack->gca_ak_data = IIGCa_global.gca_acb_active - 1;
	else
	    ack->gca_ak_data = IIGCa_global.gca_acb_active;

	ssmhdr->msg_type = GCA_ACK;
	ssmhdr->data_offset = sizeof( GCA_MSG_HDR );
	ssmhdr->data_length = sizeof( ack->gca_ak_data );
	ssmhdr->buffer_length = ssmhdr->data_offset + ssmhdr->data_length;
	ssmhdr->flags.flow_indicator = GCA_NORMAL;
	ssmhdr->flags.end_of_data = TRUE;
	ssmhdr->flags.end_of_group = TRUE;
	ssm->usrbuf = ssm->usrptr = rsm->buffer;
	ssm->usrlen = ssmhdr->data_length;
	break;
    }

    case RG_INIT_CONN:
    {
        /*
	** Set-up NS Connection service parms.
	*/
	GCA_SVC_PARMS *call_parms = &gca_cb->gca_reg_acb->con_parms;
	
	/*
	** Nothing to do if currently running in desired context
	** (called during GCA_REGISTER processing).
	*/
	if ( svc_parms == call_parms )  break;

	/*
	** Check for duplicate request (alternate service parms in use).
	*/
	MUp_semaphore( &gca_cb->gca_reg_semaphore );

	if ( call_parms->in_use )
	    branch = TRUE;
	else
	{
	    call_parms->in_use = TRUE;
	    call_parms->gca_cb = gca_cb;
	    call_parms->acb = gca_cb->gca_reg_acb;
	    call_parms->time_out = -1;
	    call_parms->gc_parms.gc_cb = gca_cb->gca_reg_acb->gc_cb;
	    call_parms->gc_parms.flags.flow_indicator = GCA_NORMAL;
	    call_parms->gc_parms.time_out = -1;
	    svc_parms->call_parms = call_parms;
	}

	MUv_semaphore( &gca_cb->gca_reg_semaphore );
	break;
    }

    case RG_INIT_SEND:
    {
        /*
	** Set-up NS Send (normal) service parms.
	** Branch if active (duplicate request).
	*/
	GCA_SVC_PARMS *call_parms = &gca_cb->gca_reg_acb->snd_parms[GCA_NORMAL];

	/*
	** Check for duplicate request (alternate service parms in use).
	*/
	MUp_semaphore( &gca_cb->gca_reg_semaphore );

	if ( call_parms->in_use )
	    branch = TRUE;
	else
	{
	    call_parms->in_use = TRUE;
	    call_parms->gca_cb = gca_cb;
	    call_parms->acb = gca_cb->gca_reg_acb;
	    call_parms->time_out = -1;
	    call_parms->gc_parms.gc_cb = gca_cb->gca_reg_acb->gc_cb;
	    call_parms->gc_parms.flags.flow_indicator = GCA_NORMAL;
	    call_parms->gc_parms.time_out = -1;
	    svc_parms->call_parms = call_parms;
	}

	MUv_semaphore( &gca_cb->gca_reg_semaphore );
	break;
    }

    case RG_INIT_EXP:
	/*
	** Set-up NS Expedited service parms.
	*/
	svc_parms->call_parms = &gca_cb->gca_reg_acb->rcv_parms[GCA_EXPEDITED];
	svc_parms->call_parms->gca_cb = gca_cb;
	svc_parms->call_parms->acb = gca_cb->gca_reg_acb;
	svc_parms->call_parms->service_code = GCA_REGISTER;
	svc_parms->call_parms->parameter_list = (PTR)&gca_cb->gca_reg_parm;
	svc_parms->call_parms->time_out = -1;
	svc_parms->call_parms->gc_parms.gc_cb = gca_cb->gca_reg_acb->gc_cb;
	svc_parms->call_parms->gc_parms.flags.flow_indicator = GCA_EXPEDITED;
	svc_parms->call_parms->gc_parms.sys_err = 
	    &gca_cb->gca_reg_parm.gca_os_status;
	svc_parms->call_parms->gc_parms.time_out = -1;
	break;

    case RG_SAVE:
    	/*
	** Save registration info.
	*/
	RG_PARMS_MACRO->gca_assoc_id = -1;
	gca_cb->gca_reg_parm.gca_srvr_class = gca_cb->gca_srvr_class;
	gca_cb->gca_reg_parm.gca_listen_id = gca_cb->gca_listen_address;
	gca_cb->gca_reg_parm.gca_modifiers = RG_PARMS_MACRO->gca_modifiers;
	gca_cb->gca_reg_parm.gca_l_so_vector = 
					RG_PARMS_MACRO->gca_l_so_vector;

	if ( gca_cb->gca_reg_parm.gca_l_so_vector )
	{
	    i4 i;

	    gca_cb->gca_reg_parm.gca_served_object_vector = 
		(char **)gca_alloc( sizeof( char * ) * 
				    gca_cb->gca_reg_parm.gca_l_so_vector );

	    if ( ! gca_cb->gca_reg_parm.gca_served_object_vector )
	    {
		gca_cb->gca_reg_parm.gca_l_so_vector = 0;
		svc_parms->gc_parms.status = E_GC0013_ASSFL_MEM;
		branch = TRUE;
	        break;
	    }

	    for( i = 0; i < gca_cb->gca_reg_parm.gca_l_so_vector; i++ )
	    {
		gca_cb->gca_reg_parm.gca_served_object_vector[i] = 
		    (char *)gca_alloc( STlength(
			RG_PARMS_MACRO->gca_served_object_vector[i] ) + 1 );

		if ( ! gca_cb->gca_reg_parm.gca_served_object_vector[i] )
		{
		    if ( ! (gca_cb->gca_reg_parm.gca_l_so_vector = i) )
		    {
			gca_free( (PTR)
			    gca_cb->gca_reg_parm.gca_served_object_vector );
			gca_cb->gca_reg_parm.gca_served_object_vector = NULL;
		    }

		    svc_parms->gc_parms.status = E_GC0013_ASSFL_MEM;
		    branch = TRUE;
		    break;
		}

		STcopy( RG_PARMS_MACRO->gca_served_object_vector[i],
			gca_cb->gca_reg_parm.gca_served_object_vector[i] );
	    }
	}
	break;

    case RG_FREE:
	/*
	** Free registration info.
	*/
	if ( gca_cb->gca_reg_parm.gca_l_so_vector )
	{
	    i4 i;

	    for( i = 0; i < gca_cb->gca_reg_parm.gca_l_so_vector; i++ )
	    {
		gca_free( gca_cb->gca_reg_parm.gca_served_object_vector[i] );
		gca_cb->gca_reg_parm.gca_served_object_vector[i] = NULL;
	    }

	    gca_free( (PTR)gca_cb->gca_reg_parm.gca_served_object_vector );
	    gca_cb->gca_reg_parm.gca_served_object_vector = NULL;
	    gca_cb->gca_reg_parm.gca_l_so_vector = 0;
	}
	break;

    case RG_FREE_ACB:
	/*
	** Free registration ACB.
	*/
	if ( gca_cb->gca_reg_acb )
	{
	    /*
	    ** If we are using the registration ACB, flag
	    ** it for deletion when complete.  Otherwise,
	    ** free the ACB now.
	    */
	    if ( gca_cb->gca_reg_acb == acb )
		acb->flags.delete_acb = TRUE;
	    else
		gca_del_acb( gca_cb->gca_reg_acb->assoc_id );

	    gca_cb->gca_reg_acb = NULL;
	}
        break;

    case RG_NS_OK:
    	/*
	** Mark NS connection OK.
	*/
	gca_cb->gca_reg_parm.gca_status = OK;
	break;

    case RG_NS_FAIL:
	/*
	** Mark NS connection failed and clear current status.
	*/
	gca_cb->gca_reg_parm.gca_status = E_GC0023_ASSOC_RLSED;
	break;

    case LS_INIT:
	/* 
	** Set initial state.
	** Stash peer info for sending.
	*/
	peersend->gca_status = OK;
	peersend->gca_flags = 0;

	/*
	** Hijack thread.
	*/
	if ( 
	     gca_cb->gca_api_version >= GCA_API_LEVEL_4  &&
	     gca_cb->gca_flags & GCA_RG_TRAP  &&
	     gcm_set_delivery( svc_parms ) == OK 
	   )  
	    branch = TRUE;
	break;
    
    case RR_INIT:
	/*
	** Fill in peer information.  This updates/overrides settings
	** done by gca_listen().
	*/
	peersend->gca_partner_protocol = RR_PARMS_MACRO->gca_local_protocol;
	peersend->gca_status = RR_PARMS_MACRO->gca_request_status;

	if ( RR_PARMS_MACRO->gca_modifiers & GCA_RQ_DESCR )
	    peersend->gca_flags |= GCA_DESCR_RQD;
	break;

    case LS_IS_NOSL:
	/* 
	** If no security label.
	**
	** Note, the GCA_LS_SECLABEL flag is only sent 
	** by the client if the security label is sent 
	** separatly (not in the aux data).
	*/
	branch = ! (peerinfo->gca_partner_protocol >= GCA_PROTOCOL_LEVEL_61  &&
		    peerinfo->gca_flags & GCA_LS_SECLABEL);
	break;

    case LS_CHK_PEER:
	/*
	** Check peer info.  Leave status in svc_parms.
	*/
	{
	    GCA_AUX_DATA	*aux = (GCA_AUX_DATA *)acb->gca_aux_data;
	    GCS_VALID_PARM	valid;
	    bool		trusted = svc_parms->gc_parms.flags.trusted;

	    /*
	    ** Negotiate protocol level.
	    */
	    if ( gca_cb->gca_local_protocol < peerinfo->gca_partner_protocol )
		peersend->gca_partner_protocol = gca_cb->gca_local_protocol;
	    else
		peersend->gca_partner_protocol = peerinfo->gca_partner_protocol;

	    /*
	    ** If no alias, use the CL provided client user ID.
	    */
	    if ( ! peerinfo->gca_user_id )
		gca_save_peer_user( peerinfo, svc_parms->gc_parms.user_name );

	    /*
	    ** CL level trust only extends to the CL
	    ** level user ID.  We remove CL level trust
	    ** when connecting with an alias.
	    */
	    if ( STcompare( svc_parms->gc_parms.user_name, 
			    peerinfo->gca_user_id ) )
		svc_parms->gc_parms.flags.trusted = FALSE;

	    /*
	    ** Validated the users authentication.
	    */
	    if ( ! peerinfo->gca_aux_count  || 
		 aux->type_aux_data != GCA_ID_AUTH )
	    {
		svc_parms->gc_parms.status = E_GC1007_NO_AUTHENTICATION;
		branch = TRUE;
		break;
	    }

	    valid.user = svc_parms->gc_parms.user_name;
	    valid.alias = peerinfo->gca_user_id;
	    valid.length = aux->len_aux_data - sizeof( GCA_AUX_DATA );
	    valid.auth = (PTR)((char *)aux + sizeof( GCA_AUX_DATA ));

	    if ( ! (gca_cb->gca_flags & GCA_AUTH_DELEGATE) )
		valid.size = 0;
	    else
	    {
		valid.size = rsm->bufsiz;
		valid.buffer = rsm->buffer;
	    }

	    if ( (svc_parms->gc_parms.status = 
		  IIgcs_call(GCS_OP_VALIDATE, GCS_NO_MECH, (PTR)&valid)) != OK )
	    {
		branch = TRUE;
		break;
	    }

	    /*
	    ** Remove the authentication from the aux data.
	    ** If auth delegated, save as aux data.  Check
	    ** for a security label.
	    */
	    gca_del_aux( acb );
	    peerinfo->gca_aux_count--;

	    if ( valid.size > 0  &&
		 gca_aux_element( acb, GCA_ID_DELEGATED, 
				  valid.size, rsm->buffer ) == OK )
	    {
		peersend->gca_aux_count++;
		aux = (GCA_AUX_DATA *)acb->gca_aux_data;
	    }

	    if ( peerinfo->gca_aux_count  &&  
		 aux->type_aux_data == GCA_ID_SECLAB )
	    {
		peerinfo->gca_flags |= GCA_LS_SECLABEL;
		gca_del_aux( acb );
		peerinfo->gca_aux_count--;
	    }

	    /*
	    ** Check privileges for monitoring or server shutdown.
	    ** This is not necessary if client is trusted (has CL
	    ** level TRUST, has been granted TRUSTED privilege or
	    ** authenticated user ID is same as our user ID).
	    */
	    if ( 
		 svc_parms->gc_parms.flags.trusted  ||
		 gca_chk_priv(peerinfo->gca_user_id, GCA_PRIV_TRUSTED) == OK ||
		 ! STcompare( peerinfo->gca_user_id, IIGCa_global.gca_uid )
	       )
		peerinfo->gca_flags |= GCA_LS_TRUSTED;
	    else  if ( peerinfo->gca_aux_count  &&
		       (aux->type_aux_data == GCA_ID_SHUTDOWN  ||
			    aux->type_aux_data == GCA_ID_QUIESCE || 
			    aux->type_aux_data == GCA_ID_CMDSESS)  &&
		       (svc_parms->gc_parms.status = 
			    gca_chk_priv( peerinfo->gca_user_id,
					  GCA_PRIV_SERVER_CONTROL)) != OK )
	    {
		branch = TRUE;
		break;
	    }

	    /*
	    ** Check service levels.
	    */
	    if ( peerinfo->gca_flags & GCA_LS_GCM &&
		 gca_cb->gca_api_version < GCA_API_LEVEL_4 )
	    {
		svc_parms->gc_parms.status = E_GC0033_GCM_NOSUPP;  
		branch = TRUE;
		break;
	    }
	}
	break;

    case LS_CHKBED:
	/*
	** Check if this is a BEDCHECK connection.
	*/
	if ( peerinfo->gca_aux_count  && 
	     ((GCA_AUX_DATA *)acb->gca_aux_data)->type_aux_data
							== GCA_ID_BEDCHECK )
	{
	    svc_parms->gc_parms.status = E_GC0032_NO_PEER;
	    branch = TRUE;
	}
        break;

    case LS_CHKGCM:
	/*
	** Check if this is an incoming GCM connection
	*/ 
	branch = ( (peerinfo->gca_flags & GCA_LS_GCM)  &&
	           ! (peerinfo->gca_flags & GCA_LS_REMOTE) );
	break;

    case LS_CHKFAST:
	/*
	** Check if this is an incoming FASTSELECT connection
	*/ 
	branch = ((peerinfo->gca_flags & GCA_LS_FASTSELECT) != 0);
	break;

    case LS_CHKAPI:
	/* 
	** if not emulating RQRESP 
	** GCA_API_LEVEL_0: does full peer info exchange.
	** GCA_API_LEVEL_1+: completes after peer info is received.
	*/
	branch = (gca_cb->gca_api_version >= GCA_API_LEVEL_1);
	break;

    case LS_LSNCNFM:
	/*
	** Invoke the server's callback routine to determine if he wants 
	** continue.  If he declines, set status in peer_info.
	*/
	if ( gca_cb->lsncnfrm  &&  ! (*gca_cb->lsncnfrm)() )
	    svc_parms->gc_parms.status = E_GC000D_ASSOCN_REFUSED;
	break;

    case LS_FMT_PEER:
	/*
	** Caller of GCA_RQ_RESP should indicate a het connection if
	** the GCA_LISTEN so indicates, but we can't trust them very much.
	** So if the incoming flags indicates het, we set it in the outgoing
	** flags.  We also set the het flag in the acb now, since this
	** is the last time we'll look at the flag.
	*/
	if ( peerinfo->gca_flags & GCA_DESCR_RQD )
	    peersend->gca_flags |= GCA_DESCR_RQD;

	if ( peersend->gca_flags & GCA_DESCR_RQD )
	    acb->flags.heterogeneous = TRUE;

	/*
	** Clear the unused peer info.
	*/
	gca_save_peer_user( peersend, NULL );
	peersend->gca_aux_count = 0;
	acb->gca_aux_len = 0;
	break;

    case LS_GET_PSTAT:
	/* 
	** Reload the status from the peer information, so the
	** listen fails if we refused a connection.
	*/
	if ( (svc_parms->gc_parms.status = peersend->gca_status) != OK )
	    branch = TRUE;
	break;

    case LS_SET_PSTAT:
	/*
	** Set peer status to what's in svc_parms, to propate status
	** from GCusrpwd or listen confirm.
	*/
	peersend->gca_status = svc_parms->gc_parms.status;
	break;

    case LS_DONE:
	/*
	** Stuff results of GCA_LISTEN into client's parms.
	*/
	acb->inbound = Yes;
	acb->user_id = gca_str_alloc( peerinfo->gca_user_id );
	acb->client_id = gca_str_alloc( svc_parms->gc_parms.user_name );
	acb->partner_id = gca_str_alloc(
				svc_parms->gc_parms.access_point_identifier );
	acb->gca_protocol = peersend->gca_partner_protocol;
	acb->req_protocol = peerinfo->gca_partner_protocol;
	acb->gca_flags = peerinfo->gca_flags;

	{
	    char buff[16];
	    MOulongout( 0, (u_i8)acb->assoc_id, sizeof(buff), buff );
	    if ( MOattach( MO_INSTANCE_VAR, 
			   GCA_MIB_ASSOCIATION, buff, (PTR)acb ) == OK )
		acb->flags.mo_attached = TRUE;
	}

	LS_PARMS_MACRO->gca_user_name = acb->user_id;
	LS_PARMS_MACRO->gca_password = NULL;	/* Deprecated! */
	LS_PARMS_MACRO->gca_account_name = acb->client_id;
	LS_PARMS_MACRO->gca_access_point_identifier = acb->partner_id;
	LS_PARMS_MACRO->gca_sec_label = svc_parms->gc_parms.sec_label;
	LS_PARMS_MACRO->gca_partner_protocol = acb->gca_protocol;
	LS_PARMS_MACRO->gca_flags = acb->gca_flags;
		

	/*
	** Remove GCA message header length from size advice
	** if headers are not visible externally.
	*/
	LS_PARMS_MACRO->gca_size_advise = acb->size_advise - 
					  (gca_cb->gca_header_length 
					   ? 0 : sizeof( GCA_MSG_HDR ));

	if ( 
	     peerinfo->gca_flags & GCA_LS_FASTSELECT  &&
	     peerinfo->gca_flags & GCA_LS_REMOTE 
	   )
	    if ( gca_cb->gca_api_version >= GCA_API_LEVEL_5 )
	    {
		/*
		** At API level 5+, remote fast select 
		** info is returned in the listen parms.  
		** No headers at API level 5+.
		*/
		LS_PARMS_MACRO->gca_message_type = rsmhdr->msg_type;
		LS_PARMS_MACRO->gca_fs_data = (PTR)rsm->usrbuf;
		LS_PARMS_MACRO->gca_l_fs_data = rsm->usrptr - rsm->usrbuf;
	    }
	    else
	    {
		/* 
		** Prior to API level 5, remote fast select info 
		** was appended to the aux data.
		*/
		if ( gca_cb->gca_header_length )
		    gca_append_aux( acb, gca_cb->gca_header_length, 
				    (PTR)rsmhdr );

		svc_parms->gc_parms.status = 
		    gca_append_aux( acb, rsm->usrptr - rsm->usrbuf, 
				    (PTR)rsm->usrbuf );
	    }

	LS_PARMS_MACRO->gca_aux_data = acb->gca_aux_data;
	LS_PARMS_MACRO->gca_l_aux_data = acb->gca_aux_len;
	break;

    case LS_GCM_PRIV:
	/*
	** Check if user has monitor privileges 
	** and set peer status.  Trusted users
	** may also make GCM requests.
	*/
	if ( gca_cb->gca_flags & GCA_GCM_READ_NONE  &&
	     gca_cb->gca_flags & GCA_GCM_WRITE_NONE )
	    peersend->gca_status = E_GC003F_PM_NOPERM;
	else  if ( peerinfo->gca_flags & GCA_LS_TRUSTED  ||
		   gca_cb->gca_flags & GCA_GCM_READ_ANY  ||
	           gca_cb->gca_flags & GCA_GCM_WRITE_ANY )
	{
	    /* Let GCM determine privileges */
	}
	else
	    peersend->gca_status = gca_chk_priv( peerinfo->gca_user_id, 
						 GCA_PRIV_MONITOR );
	break;

    case LS_RECV_GCM:
	/*
	** Prepare to receive GCM query
	**
	** We use the send buffer since 
	** it is unused during a receive.
	*/
	rsm->usrbuf = rsm->usrptr = ssm->buffer;
	rsm->usrlen = ssm->bufsiz;
	break;

    case LS_IF_RELEASE:
        /*
	** Branch if GCA_RELEASE message received.
	*/
	branch = (rsmhdr->msg_type == GCA_RELEASE);
    	break;

    case LS_RESP_GCM:
	/*
	** Generate response to GCM query
	**
	** Note that we've read the message into the send buffer
	** which was unused by read buffering, and will put the 
	** response into the receive buffer, which will be unused 
	** by send buffering. 
	*/
	gcm_response( gca_cb, acb, 
		      ((peerinfo->gca_flags & GCA_LS_TRUSTED) != 0),
		      peerinfo->gca_user_id, rsmhdr, rsm->usrbuf, 
		      rsm->usrptr - rsm->usrbuf, ssmhdr, rsm->buffer,
		      min( rsm->bufsiz, GCA_FS_MAX_DATA ) );
	ssm->usrbuf = ssm->usrptr = rsm->buffer;
	ssm->usrlen = ssmhdr->data_length;
	break;

    case LS_NO_PEER:
	svc_parms->gc_parms.status = E_GC0032_NO_PEER;
	break;

    case LS_REMOTE_GCM:
	/*
	** This is a fastselect - is it remote?
	*/ 
	branch = ((peerinfo->gca_flags & GCA_LS_REMOTE) != 0);
	break;

    case LS_HIJACK:
	/* drive initial completion for trap delivery thread */
	gca_resume( (PTR)svc_parms );
	return;

    case LS_DELIVER:
	svc_parms->gc_parms.status = gcm_deliver();
	return;

    case RQ_INIT:
	{
	/*
	** Starting a connection request.
	*/

	/*   supress pwd in info in gca_partner_name in trace msg.  
	*/
	{
	    char *pleft, *pright, *pcomma;
	    int len;
	    char conn_buff[ MAX_LOC +1 ];

	    if (( RQ_PARMS_MACRO->gca_partner_name[0] == '@' ) &&
		(pleft = STchr(RQ_PARMS_MACRO->gca_partner_name,'[')) &&
		(pright = STchr(RQ_PARMS_MACRO->gca_partner_name,']')) && 
		((pleft) && (pcomma= STchr(pleft,','))))
	    {
		len = pcomma - RQ_PARMS_MACRO->gca_partner_name +1;
		STncpy(conn_buff, RQ_PARMS_MACRO->gca_partner_name, len);
		conn_buff[len]='\0';
		STcat( conn_buff, "****");
		STcat( conn_buff, pright);
	    }
	    else
	    {
		STncpy(conn_buff, RQ_PARMS_MACRO->gca_partner_name, MAX_LOC-1);
		conn_buff[MAX_LOC]='\0';
	    }

	    GCA_DEBUG_MACRO(2)( "%04d   GCA_%s target='%s' prot=%d\n",
				(i4)(acb ? acb->assoc_id : -1),
				( svc_parms->service_code == GCA_FASTSELECT
				  ? "FASTSELECT" : "REQUEST" ),
				(RQ_PARMS_MACRO->gca_modifiers & GCA_RQ_RSLVD)
				    ? "<GCA_RQ_RSLVD>" 
				    : conn_buff,
				RQ_PARMS_MACRO->gca_peer_protocol );

	    /*
	    ** We clear the aux data at this point so that
	    ** we can tell the difference between a direct
	    ** connect request and a Name Server resolved
	    ** request.  Name Server resolution always adds
	    ** aux data prior to the connection loop (see 
	    ** RQ_ADDR_NEXT).  The connection loop builds
	    ** the peer info for direct connects if not
	    ** previously done (see RQ_FMT_PEER1).  Direct
	    ** connects cannot build the peer until the
	    ** association buffers have been allocated
	    ** (SA_BUFFERS), which only occurs after the
	    ** connection loop is entered.
	    */
	    gca_save_peer_user( peersend, NULL );
	    peersend->gca_status = OK;
	    peerinfo->gca_aux_count = 0;
	    acb->gca_aux_len = 0;

	    if ( svc_parms->service_code == GCA_FASTSELECT )
	    {
		if ( GCA_FS_MAX_DATA > (FS_PARMS_MACRO->gca_b_length - 
					gca_cb->gca_header_length) )
		{
		    svc_parms->gc_parms.status = E_GC0010_BUF_TOO_SMALL;
		    branch = TRUE;
		}
		else  if ( GCA_FS_MAX_DATA < FS_PARMS_MACRO->gca_msg_length )
		{
		    svc_parms->gc_parms.status = E_GC0011_INV_CONTENTS;
		    branch = TRUE;
		}
	    }
	}
	break;
	}

    case RQ_ALLOC:
	/*
	** Allocate RQCB control block, overriding assignment above.
	*/
	if ( ! (acb->connect = (GCA_RQCB *)gca_alloc( sizeof( GCA_RQCB ) )) )
	{
	    svc_parms->gc_parms.status = E_GC0013_ASSFL_MEM;
	    branch = TRUE;
	}
	else
	{
	    /* Zero important parts of the rqcb. */
	    acb->connect->rmt_indx = acb->connect->lcl_indx = 0;
	    acb->connect->rslv_buff = NULL;
	    acb->connect->grcb = NULL;
	    rqcb = acb->connect;
	}
	break;

    case RQ_FREE:
	/*
	** Free RQCB
	*/
	if ( acb->connect )
	{
	    if ( acb->connect->rslv_buff )
	    {
		gca_free( acb->connect->rslv_buff );
		acb->connect->rslv_buff = NULL;
	    }

	    if ( acb->connect->grcb )  
	    {
		gca_free( acb->connect->grcb );
		acb->connect->grcb = NULL;
	    }

	    gca_free( (PTR)acb->connect );
	    acb->connect = rqcb = NULL;
	}
	break;

    case RQ_IS_NOXLATE:
	/* If user specified direct connect (no NS lookup). */
	branch = ((RQ_PARMS_MACRO->gca_modifiers & GCA_NO_XLATE) != 0); 
	break;

    case RQ_IS_RSLVD:
	/* If user specified pre-resolved partner name. */
	branch = ((RQ_PARMS_MACRO->gca_modifiers & GCA_RQ_RSLVD) != 0);
	break;

    case RQ_IS_NMSVR:
	/* If target name is the local name server (/iinmsvr). */
	if ( ! STcasecmp( RQ_PARMS_MACRO->gca_partner_name, "/iinmsvr") ||
	     ( ! STncmp( RQ_PARMS_MACRO->gca_partner_name, "/@", 2 )	&&
	       ! STbcompare( &RQ_PARMS_MACRO->gca_partner_name[2], 0,
			     svc_parms->gc_parms.partner_name, 0, TRUE)) )
	{
	    branch = TRUE;
	    acb->flags.concatenate = FALSE;
	}
	break;

    case RQ_USE_NMSVR:
	/* If embedded Name Server interface disabled */
	branch = ( ! IIGCa_global.gca_embedded_gcn );
	break;

    case RQ_RESOLVE:
	/*
	** Embedded Name Server.
	*/
	svc_parms->gc_parms.status = 
	    gca_ns_resolve( RQ_PARMS_MACRO->gca_partner_name,
			    RQ_PARMS_MACRO->gca_user_name,
			    RQ_PARMS_MACRO->gca_password,
			    (RQ_PARMS_MACRO->gca_modifiers & GCA_RQ_REMPW)
				    ? RQ_PARMS_MACRO->gca_rem_username : NULL,
			    (RQ_PARMS_MACRO->gca_modifiers & GCA_RQ_REMPW)
				    ? RQ_PARMS_MACRO->gca_rem_password : NULL,
			    rqcb );

	branch = (svc_parms->gc_parms.status != OK);
	break;
    
    case RQ_ADDR_NOXL:
	/*
	** Load address directly from request.
	*/
	svc_parms->gc_parms.partner_host = NULL;
	svc_parms->gc_parms.partner_name = RQ_PARMS_MACRO->gca_partner_name;
	rqcb->ns.lcl_count = 1;
	break;

    case RQ_ADDR_NSID:
	/* 
	** Get name server address. 
	*/
	svc_parms->gc_parms.partner_host = NULL;
	svc_parms->gc_parms.partner_name = ssm->buffer;
	rqcb->ns.lcl_count = 1;

	if ( (svc_parms->gc_parms.status = 
		GCnsid( GC_FIND_NSID, ssm->buffer, ssm->bufsiz, 
			svc_parms->gc_parms.sys_err )) != OK )
	    branch = TRUE;
	break;

    case RQ_FMT_RSLV:
	/*
	** Format GCN_NS_RESOLVE or GCN_NS_2_RESOLVE.
	*/
	if ( ( RQ_PARMS_MACRO->gca_modifiers & GCA_RQ_REMPW ) &&
	    peerinfo->gca_partner_protocol < GCA_PROTOCOL_LEVEL_60 )
	{
	    svc_parms->gc_parms.status = E_GC003C_PWPROMPT_NOTSUP;
	    branch = TRUE;
	}
	else
	{
	    char *p = rsm->buffer;

	    /* 
	    ** Wrap up message for GCN_NS_RESOLVE message
	    ** unwrapped by gcn_resolve(). 
	    */
	    p += gcu_put_str( p, "" );
	    p += gcu_put_str( p, "" );
	    p += gcu_put_str( p, RQ_PARMS_MACRO->gca_partner_name );
	    p += gcu_put_str( p, "" );
	    p += gcu_put_str( p, "" );

	    if ( RQ_PARMS_MACRO->gca_modifiers & GCA_RQ_REMPW )
	    {
		p += gcu_put_str( p, RQ_PARMS_MACRO->gca_rem_username);
		p += gcu_put_str( p, RQ_PARMS_MACRO->gca_rem_password);
	    }

	    /* 
	    ** Setup GCA header. 
	    */
	    ssmhdr->msg_type = (RQ_PARMS_MACRO->gca_modifiers & GCA_RQ_REMPW)
				? GCN_NS_2_RESOLVE : GCN_NS_RESOLVE;
	    ssmhdr->data_offset = sizeof( GCA_MSG_HDR );
	    ssmhdr->data_length = p - rsm->buffer;
	    ssmhdr->buffer_length = ssmhdr->data_offset + ssmhdr->data_length;
	    ssmhdr->flags.flow_indicator = GCA_NORMAL;
	    ssmhdr->flags.end_of_data = TRUE;
	    ssmhdr->flags.end_of_group = TRUE;
	    ssm->usrbuf = ssm->usrptr = rsm->buffer;
	    ssm->usrlen = ssmhdr->data_length;
	}
	break;

    case RQ_FMT_RLS:
	/*
	** Format GCA_RELEASE message for sending.
	*/
	{
	    GCA_ER_DATA *release = (GCA_ER_DATA *)rsm->buffer;

	    release->gca_l_e_element = 0;	/* no error data included */
	    ssmhdr->msg_type = GCA_RELEASE;
	    ssmhdr->data_offset = sizeof( GCA_MSG_HDR );
	    ssmhdr->data_length = sizeof( release->gca_l_e_element );
	    ssmhdr->buffer_length = ssmhdr->data_offset + ssmhdr->data_length;
	    ssmhdr->flags.flow_indicator = GCA_NORMAL;
	    ssmhdr->flags.end_of_data = TRUE;
	    ssmhdr->flags.end_of_group = TRUE;
	    ssm->usrbuf = ssm->usrptr = rsm->buffer;
	    ssm->usrlen = ssmhdr->data_length;
	}
	break;

    case RQ_RECV:
	/*
	** Prepare to receive message from Name Server.
	** We use the send buffer since it is unused 
	** during a receive.
	*/
	rsm->usrbuf = rsm->usrptr = ssm->buffer;
	rsm->usrlen = ssm->bufsiz;
	break;

    case RQ_LOAD_RSLVD:
	/*
	** Interpret GCN_RESOLVE response.
	*/
	if ( rsmhdr->msg_type != GCA_ERROR )
	    svc_parms->gc_parms.status = 
		gca_resolved( rsm->usrptr - rsm->usrbuf, rsm->usrbuf, rqcb, 
			      peerinfo->gca_partner_protocol );
	break;

    case RQ_LOAD_PRTNR:
	/*
	** When GCA_RQ_RSLVD is specified, target name is a GCN_RESOLVED
	** message; unpack it.
	*/
	svc_parms->gc_parms.status = 
		gca_resolved( 0, RQ_PARMS_MACRO->gca_partner_name, rqcb, 
			      (gca_cb->gca_api_version < GCA_API_LEVEL_1)
				  ? gca_cb->gca_local_protocol 
				  : RQ_PARMS_MACRO->gca_peer_protocol );

	branch = (svc_parms->gc_parms.status != OK);
	break;

    case RQ_ADDR_NEXT:
	/*
	** Name server returned a list of servers.  We are now looping 
	** through list trying to connect.  If this list is non-empty and
	** the last connect failed, we set up for another connection. 
	** For remote connections, the Name Server also returned a list
	** of connection info which we package as aux data for the local
	** communications server.
	*/
	svc_parms->gc_parms.partner_host = rqcb->ns.lcl_host[ rqcb->lcl_indx ];
	svc_parms->gc_parms.partner_name = rqcb->ns.lcl_addr[ rqcb->lcl_indx ];
	svc_parms->gc_parms.time_out = svc_parms->time_out;
	peersend->gca_aux_count = 0;
	acb->gca_aux_len = 0;

	if ( acb->partner_id )  gca_free( (PTR)acb->partner_id );
	if ( ! svc_parms->gc_parms.partner_host  ||
	     ! *svc_parms->gc_parms.partner_host )
	    acb->partner_id = gca_str_alloc( svc_parms->gc_parms.partner_name );
	else
	{
	    i4 len = STlength( svc_parms->gc_parms.partner_host ) +
	    	     STlength( svc_parms->gc_parms.partner_name ) + 2;

	    if ( (acb->partner_id = gca_alloc( len )) )
		STpolycat( 3, svc_parms->gc_parms.partner_host, ":",
			   svc_parms->gc_parms.partner_name, acb->partner_id );
	}

	/*
	** Authenticate the connection.  Use Name Server
	** authentication if available, or generate an 
	** auth as if we are connecting directly to the 
	** server without Name Server resolution.
	*/
	if ( rqcb->ns.lcl_user[0]  &&  rqcb->ns.lcl_l_auth[ rqcb->lcl_indx ] )
	{
	    gca_save_peer_user( peersend, rqcb->ns.lcl_user );

	    if ( (svc_parms->gc_parms.status = 
		  gca_aux_element( acb, GCA_ID_AUTH,
				   rqcb->ns.lcl_l_auth[rqcb->lcl_indx],
				   rqcb->ns.lcl_auth[rqcb->lcl_indx] )) == OK )
		peersend->gca_aux_count++;
	    else
	    {
		branch = TRUE;
		break;
	    }
	}
	else  if ( (svc_parms->gc_parms.status = 
			gca_auth( acb, peersend, RQ_PARMS_MACRO, FALSE,
				  ssm->bufsiz, (PTR)ssm->buffer )) != OK )
	{
	    branch = TRUE;
	    break;
	}

	if ( (svc_parms->gc_parms.status = 
	      gca_seclab( acb, peersend, RQ_PARMS_MACRO )) != OK )
	{
	    branch = TRUE;
	    break;
	}

	if ( ! rqcb->ns.rmt_count )
	{
	    GCA_DEBUG_MACRO(3)( "%04d   GCA_%s laddr='%s'\n",
				acb ? acb->assoc_id : -1, 
				( svc_parms->service_code == GCA_FASTSELECT
				  ? "FASTSELECT" : "REQUEST" ),
				acb->partner_id );

	    acb->flags.remote = FALSE;
	    svc_parms->gc_parms.flags.remote = FALSE;

	}
	else
	{
	    /* 
	    ** Send buffer is used to build the peer info.
	    */
	    char	*rmt_buf = ssm->buffer;
	    STATUS	status;

	    if ( rqcb->ns.rmt_db  &&  *rqcb->ns.rmt_db )
	    {
		rmt_buf += gcu_put_int( rmt_buf, GCA_RMT_PRTN_ID );
		rmt_buf += gcu_put_str( rmt_buf, rqcb->ns.rmt_db );
	    }

	    if ( rqcb->ns.protocol[ rqcb->rmt_indx ]  &&
		 *rqcb->ns.protocol[ rqcb->rmt_indx ] )
	    {
		rmt_buf += gcu_put_int( rmt_buf, GCA_RMT_PROTO_ID );
		rmt_buf += gcu_put_str( rmt_buf, 
					rqcb->ns.protocol[ rqcb->rmt_indx ] );
	    }

	    if ( rqcb->ns.node[ rqcb->rmt_indx ]  &&
		 *rqcb->ns.node[ rqcb->rmt_indx ] )
	    {
		rmt_buf += gcu_put_int( rmt_buf, GCA_RMT_NODE_ID );
		rmt_buf += gcu_put_str( rmt_buf, 
					rqcb->ns.node[ rqcb->rmt_indx ] );
	    }

	    if ( rqcb->ns.port[ rqcb->rmt_indx ]  &&
		 *rqcb->ns.port[ rqcb->rmt_indx ] )
	    {
		rmt_buf += gcu_put_int( rmt_buf, GCA_RMT_PORT_ID );
		rmt_buf += gcu_put_str( rmt_buf, 
					rqcb->ns.port[ rqcb->rmt_indx ] );
	    }
	    
	    /*
	    ** Generate remote authentication if requested
	    ** by the Name Server.
	    */
	    if ( rqcb->ns.rmt_auth_len )
	    {
		rmt_buf += gcu_put_int( rmt_buf, GCA_RMT_AUTH );
		rmt_buf += gcu_put_int( rmt_buf, rqcb->ns.rmt_auth_len );
		MEcopy( rqcb->ns.rmt_auth, rqcb->ns.rmt_auth_len, rmt_buf );
		rmt_buf += rqcb->ns.rmt_auth_len;
		status = OK;
	    }
	    else  if ( ! rqcb->ns.rmt_mech  ||  ! *rqcb->ns.rmt_mech )
		status = FAIL;		/* trigger default action below */
	    else
	    {
		i4 len = ssm->bufsiz - (rmt_buf - ssm->buffer);

		status = gca_rem_auth( rqcb->ns.rmt_mech, 
				       rqcb->ns.node[ rqcb->rmt_indx ], 
				       &len, (PTR)rmt_buf );

		if ( status == OK )
		{
		    /* Remote auth is for the current user ID */
		    rmt_buf += len;
		    rqcb->ns.rmt_user = NULL;
		}
		else  if ( IIGCa_global.gca_rmt_auth_fail )
		{
		    svc_parms->gc_parms.status = status;
		    branch = TRUE;
		    break;
		}
	    }

	    /*
	    ** If no remote auth, see if password available.
	    */
	    if ( status != OK )
		if ( rqcb->ns.rmt_pwd  &&  *rqcb->ns.rmt_pwd )
		{
		    rmt_buf += gcu_put_int( rmt_buf, GCA_RMT_PASSWD );
		    rmt_buf += gcu_put_str( rmt_buf, rqcb->ns.rmt_pwd );
		}
		else
		{
		    svc_parms->gc_parms.status = E_GC0133_GCN_LOGIN_UNKNOWN;
		    branch = TRUE;
		    break;
		}

	    rmt_buf += gcu_put_int( rmt_buf, GCA_RMT_USER );

	    if ( rqcb->ns.rmt_user  &&  *rqcb->ns.rmt_user )
		rmt_buf += gcu_put_str( rmt_buf, rqcb->ns.rmt_user );
	    else
		rmt_buf += gcu_put_str( rmt_buf, IIGCa_global.gca_uid );

	    if ( rqcb->ns.rmt_emech  &&  *rqcb->ns.rmt_emech )
	    {
	        rmt_buf += gcu_put_int( rmt_buf, GCA_RMT_EMECH );
	        rmt_buf += gcu_put_str( rmt_buf, rqcb->ns.rmt_emech );
	    }

	    if ( rqcb->ns.rmt_emode  &&  *rqcb->ns.rmt_emode )
	    {
	        rmt_buf += gcu_put_int( rmt_buf, GCA_RMT_EMODE );
	        rmt_buf += gcu_put_str( rmt_buf, rqcb->ns.rmt_emode );
	    }

	    acb->flags.remote = TRUE;
	    svc_parms->gc_parms.flags.remote = TRUE;

	    if ( ! STcasecmp( rqcb->ns.rmt_db, "/iinmsvr" ) )
	        acb->flags.concatenate = FALSE;

	    svc_parms->gc_parms.status = 
		gca_aux_element( acb, GCA_ID_RMT_INFO, 
				 rmt_buf - ssm->buffer, (PTR)ssm->buffer );

	    if ( svc_parms->gc_parms.status == OK )
		peersend->gca_aux_count++;
	    else
	    {
		branch = TRUE;
		break;
	    }

	    GCA_DEBUG_MACRO(3)( "%04d   GCA_%s laddr='%s' raddr='%s,%s,%s'\n",
			        (i4)(acb ? acb->assoc_id : -1), 
				( svc_parms->service_code == GCA_FASTSELECT
				  ? "FASTSELECT" : "REQUEST" ),
			        acb->partner_id,
			        rqcb->ns.node[rqcb->rmt_indx],
			        rqcb->ns.protocol[rqcb->rmt_indx],
			        rqcb->ns.port[rqcb->rmt_indx] );
	}
	break;

    case RQ_CHK_FAIL:
	/*
	** Determine source of failure from status, and either proceed
	** to the next local address or the next remote address.
	*/
	switch( svc_parms->gc_parms.status & 0xFFFFFF )
	{
	    case E_GC000B_RMT_LOGIN_FAIL:
	    case E_GC0100_GCN_SVRCLASS_NOTFOUND:
		if ( rqcb->ns.rmt_count )
		    rqcb->rmt_indx++;
		else
		    rqcb->lcl_indx = rqcb->ns.lcl_count;
	        break;

	    case E_GCF_MASK + 0x2214:	/* E_GC2214_MAX_IB_CONNS */
	    case E_GCF_MASK + 0x280D:	/* E_GC280D_NTWK_ERROR */
	    case E_GCF_MASK + 0x2819:	/* E_GC2819_NTWK_CONNECTION_FAIL */
		if ( rqcb->ns.rmt_count )
		    rqcb->rmt_indx++;
		else
		    rqcb->lcl_indx++;
		break;

	    default:	
		rqcb->lcl_indx++;
		break;
	}

	/* 
	** If all addresses (local or remote) are exhausted. 
	*/
	branch = (rqcb->lcl_indx >= rqcb->ns.lcl_count  ||
		  (rqcb->ns.rmt_count && rqcb->rmt_indx >= rqcb->ns.rmt_count));
	break;

    case RQ_PROT_MINE:
	/*
	** Use the current protocol level when talking 
	** internally to the Name Server.
	*/
	peersend->gca_partner_protocol = GCA_PROTOCOL_LEVEL_63;
	break;

    case RQ_PROT_USER:
	/*
	** Use the user's protocol level when talking to the target.
	** GCA_API_LEVEL_0: gca_peer_protocol was output only.
	*/
	peersend->gca_partner_protocol = 
		gca_cb->gca_api_version < GCA_API_LEVEL_1 
		    ? gca_cb->gca_local_protocol 
		    : RQ_PARMS_MACRO->gca_peer_protocol;
	break;

    case RQ_FMT_PEER1:
	/*
	** Init peer info for general server connection.
	**
	** Default initialization occurs in RQ_INIT and
	** SA_SEND_PEER.  GCA protocol level is set in 
	** RQ_PROT_USER.  RQ_ADDR_NEXT builds aux data
	** (default aux data built here if not present).
	*/
	peersend->gca_flags = 0;

	if( RQ_PARMS_MACRO->gca_modifiers & GCA_RQ_DESCR )
	    peersend->gca_flags |= GCA_DESCR_RQD;

	if ( acb->flags.remote )  peersend->gca_flags |= GCA_LS_REMOTE;

	if ( RQ_PARMS_MACRO->gca_modifiers & GCA_RQ_GCM )
	{
	    peersend->gca_flags |= GCA_LS_GCM;
	    if ( svc_parms->service_code == GCA_FASTSELECT )
		peersend->gca_flags |= GCA_LS_FASTSELECT;
	}

	/*
	** If partner was resolved using the Name Server,
	** RQ_ADDR_NEXT added aux data for the connection.  
	** Otherwise, we are making a direct connection to 
	** the server and need to initialize the peer info 
	** aux data accordingly.
	*/
	if ( ! peersend->gca_aux_count )
	{
	    if ( (svc_parms->gc_parms.status = 
			gca_auth( acb, peersend, RQ_PARMS_MACRO, FALSE,
				  ssm->bufsiz, (PTR)ssm->buffer )) != OK )
	    {
		branch = TRUE;
		break;
	    }

	    if ( (svc_parms->gc_parms.status = 
		  gca_seclab( acb, peersend, RQ_PARMS_MACRO )) != OK )
	    {
		branch = TRUE;
		break;
	    }
	}

	/* 
	** If connecting directly, we can invoke server's 
	** special services.  SHUTDOWN, QUIESCE, and BEDCHECK 
	** are fast selects.  The connection terminates after 
	** the remote listen completes.
	*/
	if ( RQ_PARMS_MACRO->gca_modifiers & (GCA_NO_XLATE | GCA_RQ_RSLVD) )
	{
	    if ( RQ_PARMS_MACRO->gca_modifiers & GCA_CS_SHUTDOWN )
		if ( (svc_parms->gc_parms.status =
		      gca_aux_element( acb, GCA_ID_SHUTDOWN, 0, NULL )) == OK )
		    peersend->gca_aux_count++;
		else
		{
		    branch = TRUE;
		    break;
		}

	    if ( RQ_PARMS_MACRO->gca_modifiers & GCA_CS_QUIESCE )
		if ( (svc_parms->gc_parms.status =
		      gca_aux_element( acb, GCA_ID_QUIESCE, 0, NULL )) == OK )
		    peersend->gca_aux_count++;
		else
		{
		    branch = TRUE;
		    break;
		}

	    if ( RQ_PARMS_MACRO->gca_modifiers & GCA_CS_CMD_SESSN )
		if ( (svc_parms->gc_parms.status =
		      gca_aux_element( acb, GCA_ID_CMDSESS, 0, NULL )) == OK )
		    peersend->gca_aux_count++;
		else
		{
		    branch = TRUE;
		    break;
		}

	    if ( RQ_PARMS_MACRO->gca_modifiers & GCA_CS_BEDCHECK )
		if ( (svc_parms->gc_parms.status =
		      gca_aux_element( acb, GCA_ID_BEDCHECK, 0, NULL )) == OK )
		    peersend->gca_aux_count++;
		else
		{
		    branch = TRUE;
		    break;
		}
	}
	break;

    case RQ_FMT_PEER2:
	/*
	** Init peer info for Name Server resolution.
	**
	** Default initialization occurs in RQ_INIT and
	** SA_SEND_PEER.  GCA protocol level is set in 
	** RQ_PROT_MINE.
	**
	** Generate the appropriate authentication
	** for connecting to the Name Server.
	*/

	peersend->gca_flags = 0;

#ifdef GCF_EMBEDDED_GCC

	if( IIGCa_global.gca_embed_gcc_flag )
	    peersend->gca_flags |= GCA_LS_EMBEDGCC;
#endif
	if ( (svc_parms->gc_parms.status = 
			gca_auth( acb, peersend, RQ_PARMS_MACRO, TRUE,
				  ssm->bufsiz, (PTR)ssm->buffer )) != OK )
	    branch = TRUE;
	else  if ( (svc_parms->gc_parms.status = 
	            gca_seclab( acb, peersend, RQ_PARMS_MACRO )) != OK )
	    branch = TRUE;
	break;

    case RQ_CHECKPEER:
	/*
	** Check status in peer info.
	*/
	if ( peerinfo->gca_status == OK )
	{
	    /*
	    ** Restore concatenation capability at 6.3 and above.
	    ** The name server protocol below 6.3 did not support
	    ** concatenation because the end-of-data indicator was
	    ** used as end of message group.  Concatenation was
	    ** unconditionally turned off before the Name Server
	    ** connection request was made.
	    */
	    if ( peerinfo->gca_partner_protocol >= GCA_PROTOCOL_LEVEL_63 )
		acb->flags.concatenate = TRUE;

	    if ( peerinfo->gca_flags & GCA_DESCR_RQD )
		acb->flags.heterogeneous = TRUE;
	}
	else
	{
	    svc_parms->gc_parms.status = peerinfo->gca_status;
	    branch = TRUE;
	}
	break;

    case RQ_SAVERR:
	/*
	** Save current connect status.
	*/
	rqcb->connect_status = svc_parms->gc_parms.status;
	STRUCT_ASSIGN_MACRO(*svc_parms->gc_parms.sys_err,rqcb->connect_syserr);
	break;

    case RQ_GETERR:
	/*
	** Restore connect status (ignore current status).
	*/
	svc_parms->gc_parms.status = rqcb->connect_status;
	STRUCT_ASSIGN_MACRO( rqcb->connect_syserr, 
			     *svc_parms->gc_parms.sys_err );
	break;

    case RQ_CHK_FAST:
	/* if operation is FASTSELECT */
	branch = (svc_parms->service_code == GCA_FASTSELECT);
	break;

    case RQ_SEND_FAST:
	/*
	** Prepare to send FASTSELECT message.
	*/
	ssmhdr->msg_type = FS_PARMS_MACRO->gca_message_type;
	ssmhdr->data_offset = sizeof(GCA_MSG_HDR);
	ssmhdr->data_length = min( FS_PARMS_MACRO->gca_msg_length, 
				   GCA_FS_MAX_DATA );
	ssmhdr->buffer_length = ssmhdr->data_offset + ssmhdr->data_length;
	ssmhdr->flags.flow_indicator = GCA_NORMAL;
	ssmhdr->flags.end_of_data = TRUE;
	ssmhdr->flags.end_of_group = TRUE;
	ssm->usrbuf = ssm->usrptr = FS_PARMS_MACRO->gca_buffer + 
				    gca_cb->gca_header_length;
	ssm->usrlen = ssmhdr->data_length;
	break;

    case RQ_RECV_FAST:
	/*
	** Prepare to receive message into user's buffer.
	*/
	rsm->usrbuf = rsm->usrptr = FS_PARMS_MACRO->gca_buffer + 
				    gca_cb->gca_header_length;
	rsm->usrlen = FS_PARMS_MACRO->gca_b_length - gca_cb->gca_header_length;
	break;

    case RQ_COMP_FAST:
	/*
	** Return FASTSELECT info and complete
	*/
	FS_PARMS_MACRO->gca_message_type = rsmhdr->msg_type;
	FS_PARMS_MACRO->gca_msg_length = rsm->usrptr - rsm->usrbuf;
	FS_PARMS_MACRO->gca_assoc_id = -1;	/* Clear ACB (no longer used) */

	if ( gca_cb->gca_api_version < GCA_API_LEVEL_5 )
	{
	    GCA_MSG_HDR hdr;

	    hdr.msg_type = rsmhdr->msg_type;
	    hdr.data_offset = sizeof( GCA_MSG_HDR );
	    hdr.data_length = rsm->usrptr - rsm->usrbuf;
	    hdr.buffer_length = hdr.data_offset + hdr.data_length;
	    hdr.flags.flow_indicator = GCA_NORMAL;
	    hdr.flags.end_of_data = TRUE;
	    hdr.flags.end_of_group = TRUE;

	    MECOPY_CONST_MACRO( (PTR)&hdr, sizeof( hdr ), 
				(PTR)FS_PARMS_MACRO->gca_buffer );
	}
	break;

    case RQ_COMP_ERR:
	GCA_DEBUG_MACRO(2)( "%04d   GCA_%s status=%x\n",
			    (i4)(acb ? acb->assoc_id : -1),
			    ( svc_parms->service_code == GCA_FASTSELECT
			      ? "FASTSELECT" : "REQUEST" ),
			    svc_parms->gc_parms.status );
	break;

    case RQ_COMPLETE:
	/*
	** Return REQUEST info
	*/
	acb->inbound = No;
	acb->user_id = gca_str_alloc( peersend->gca_user_id 
				      ? peersend->gca_user_id 
				      : acb->client_id );
	{ 
	    char buffer[ GC_USERNAME_MAX + 1 ];
	    GCusername( buffer, sizeof( buffer ) );
	    acb->client_id = gca_str_alloc( buffer );
	}

	acb->gca_protocol = peerinfo->gca_partner_protocol;
	acb->req_protocol = peersend->gca_partner_protocol;
	acb->gca_flags = peerinfo->gca_flags;

	{
	    char buff[16];
	    MOulongout( 0, (u_i8)acb->assoc_id, sizeof(buff), buff );
	    if ( MOattach( MO_INSTANCE_VAR, 
			   GCA_MIB_ASSOCIATION, buff, (PTR)acb ) == OK )
		acb->flags.mo_attached = TRUE;
	}

	RQ_PARMS_MACRO->gca_peer_protocol = acb->gca_protocol;
	RQ_PARMS_MACRO->gca_flags = acb->gca_flags;

	/*
	** Remove GCA message header length from size advice
	** if headers are not visible externally.
	*/
	RQ_PARMS_MACRO->gca_size_advise = acb->size_advise - 
		(gca_cb->gca_header_length ? 0 : sizeof( GCA_MSG_HDR ));

	GCA_DEBUG_MACRO(2)( "%04d   GCA_%s prot=%d flags=%x\n",
			    (i4)(acb ? acb->assoc_id : -1),
			    ( svc_parms->service_code == GCA_FASTSELECT
			      ? "FASTSELECT" : "REQUEST" ),
			    RQ_PARMS_MACRO->gca_peer_protocol,
			    RQ_PARMS_MACRO->gca_flags );
	break;

    case RV_INIT:

	GCA_DEBUG_MACRO(3)
	    ( "%04d   GCA_RECEIVE rcv_len=%d flow=%s %s %s\n",
	      (i4)(acb ? acb->assoc_id : -1), 
	      RV_PARMS_MACRO->gca_b_length,
	      svc_parms->gc_parms.flags.flow_indicator ? "exp" : "nor",
	      acb->flags.rpurging ? "rpurging" : "",
	      acb->flags.spurging ? "spurging" : "" );

	break;

    case RV_IS_HDR:
	/* if header expected */
	branch = (rsmhdr->data_length == 0);
	break;

    case RV_IS_MORE:
	/* if mid-message */
	branch = (rsmhdr->data_length != 0);
	break;

    case RV_IS_FMT:
	/* if GCA_FORMATTED */
	branch = ( (gca_cb->gca_api_version >= GCA_API_LEVEL_5)  &&
		   (RV_PARMS_MACRO->gca_modifiers & GCA_FORMATTED) );
	break;

    case RV_IS_HET:
	/* if GCA_GOTOHET received */
	if ( rsmhdr->msg_type == GCA_GOTOHET  &&  rsmhdr->flags.end_of_data )
	{
	    acb->flags.heterogeneous = TRUE;
	    branch = TRUE;
	}
	break;

    case RV_IS_PRG:
	/* if purging the channel after sending/receiving ATTN */
	branch = ( (acb->flags.spurging  ||  acb->flags.rpurging)  &&  
		   ! svc_parms->gc_parms.flags.flow_indicator );
	break;

    case RV_IS_SPRG:
	/* if purging the channel after sending ATTN */
	branch = ( acb->flags.spurging  && 
		   ! svc_parms->gc_parms.flags.flow_indicator );
	break;

    case RV_CONCAT:
	/* if user buffer not full and not end-of-data */
	branch = ( acb->flags.concatenate  &&  
		   rsm->usrlen  &&  ! rsmhdr->flags.end_of_data );
	break;

    case RV_DOUSR:
	/*
	** Prepare to receive message into user's buffer.
	*/

	rsm->usrbuf = rsm->usrptr = RV_PARMS_MACRO->gca_buffer +
				    gca_cb->gca_header_length;
	rsm->usrlen = RV_PARMS_MACRO->gca_b_length - 
				    gca_cb->gca_header_length;
	break;

    case RV_SVCHDR:
	/*
	** Prepare to read header into rsm header area.
	*/
	rsm->svcbuf = (char *)rsmhdr;
	rsm->svclen = sizeof( GCA_MSG_HDR );
	break;

    case RV_SVCUSR:
	/*
	** Prepare to read data into user buffer.
	*/
	rsm->svcbuf = rsm->usrptr;
	rsm->svclen = min( rsm->usrlen, rsmhdr->data_length );
	rsmhdr->data_length -= rsm->svclen;
	rsm->usrptr += rsm->svclen;
	rsm->usrlen -= rsm->svclen;
	break;

    case RV_BUFF:
	/*
	** Copy buffered data from rsm buffer.
	*/
	{
	    u_i2 len = min( rsm->bufend - rsm->bufptr, rsm->svclen );

	    GCA_DEBUG_MACRO(4)
		( "recv buffering %d of %d\n", (i4)len, rsm->svclen );

	    if ( len )
	    {
		MEcopy( rsm->bufptr, len, rsm->svcbuf );
		rsm->svcbuf += len;
		rsm->svclen -= len;
		rsm->bufptr += len;
	    }

	    if ( rsm->bufptr >= rsm->bufend )	/* Setup for GC_RECEIVE */
		rsm->bufptr = rsm->bufend = rsm->buffer;

	    branch = (! rsm->svclen);		/* Request filled */
	}
	break;

    case RV_CLRPRG:
	/*
	** Clear purging flag after receipt of chop marks.
	** Make sure decoder is no longer processing the
	** message just purged.  The next thing expected
	** is a message header so make sure we don't keep
	** trying to read data for current header.
	*/
	acb->flags.spurging = FALSE;
	acb->flags.rpurging = FALSE;
	((GCO_DOC*)rsm->doc)->state = DOC_IDLE;
	rsmhdr->data_length = 0;
	break;

    case RV_PURGED:
	/* Purge the request. */
	svc_parms->gc_parms.status = E_GC0027_RQST_PURGED;
	break;

    case RV_FMT:
    {
	/*
	** Decode data from rsm buffer
	**
	** Data flows like this:
	**
	** From
	**	rsmhdr->msg_type	message type
	**	rsmhdr->data_length	amount remaining in message
	**	rsm->bufptr		beginning of (remainder of) message
	**	rsm->bufend		end of buffer -- may include other msgs
	** To
	**	rsm->usrptr		where the user wants it
	**	rsm->usrlen		how big usrptr is
	**				modified to reflect actual use
	**
	** Set pointers into client and source buffers
	*/
	u_i2 len = min( rsm->bufend - rsm->bufptr, rsmhdr->data_length );

	((GCO_DOC*)rsm->doc)->message_type = rsmhdr->msg_type;
	((GCO_DOC*)rsm->doc)->eod = ((len == rsmhdr->data_length)  && 
				     rsmhdr->flags.end_of_data);
	((GCO_DOC*)rsm->doc)->src1 = rsm->bufptr;
	((GCO_DOC*)rsm->doc)->src2 = rsm->bufptr + len;
	((GCO_DOC*)rsm->doc)->dst1 = rsm->usrptr;
	((GCO_DOC*)rsm->doc)->dst2 = rsm->usrptr + rsm->usrlen;

	svc_parms->gc_parms.status = gco_decode( (GCO_DOC*)rsm->doc );

	GCA_DEBUG_MACRO(4)
	    ( "recv buffering %d of %d (%d output bytes)\n", 
	      (i4)(((GCO_DOC*)rsm->doc)->src1 - rsm->bufptr), 
	      rsmhdr->data_length, rsm->usrlen - 
		  (((GCO_DOC*)rsm->doc)->dst2 - ((GCO_DOC*)rsm->doc)->dst1) );

	/* 
	** Update source for input consumed. 
	*/
	rsmhdr->data_length -= ((GCO_DOC*)rsm->doc)->src1 - rsm->bufptr;

	if ( ((GCO_DOC*)rsm->doc)->src1 >= rsm->bufend )
	    rsm->bufptr = rsm->bufend = rsm->buffer;
	else
	    rsm->bufptr = ((GCO_DOC*)rsm->doc)->src1;

	/*
	** Update output buffer.
	*/
	rsm->usrptr = ((GCO_DOC*)rsm->doc)->dst1;
	rsm->usrlen = ((GCO_DOC*)rsm->doc)->dst2 - ((GCO_DOC*)rsm->doc)->dst1;

	/* 
	** Branch = 0 when input is depleted.
	** Branch = 1 when output is depleted or end of message. 
	*/
	if ( ((GCO_DOC*)rsm->doc)->state != DOC_MOREIN )
	    branch = TRUE;
	else  if ( rsm->bufend > rsm->bufptr )
	{
	    /*
	    ** There is unconsumed data remaining in
	    ** the input buffer.  Move it to the start
	    ** of the buffer so that it will be pre-
	    ** pended to the new input.
	    */
	    len = rsm->bufend - rsm->bufptr;
	    GCA_DEBUG_MACRO(4)( "carry over pdd %d\n", len );
	    MEcopy( rsm->bufptr, len, rsm->buffer );
	    rsm->bufptr = rsm->buffer;
	    rsm->bufend = rsm->bufptr + len;
	}
	break;
    }

    case RV_DONE:
	/*
	** messages received: do msg specific actions 
	*/

	/*
	** End-of-group determined locally 
	** prior to IPC level 5.
	*/
	if ( peerinfo->gca_version < GCA_IPC_PROTOCOL_5 )
	    rsmhdr->flags.end_of_group = 
				gca_is_eog( rsmhdr->msg_type,
					acb->gca_my_info.gca_partner_protocol );

	/* 
	** Fill in user's buffer header for GCA_INTERPET 
	*/
	if ( gca_cb->gca_api_version < GCA_API_LEVEL_5 )
	{
	    GCA_MSG_HDR hdr;

	    hdr.msg_type = rsmhdr->msg_type;
	    hdr.data_offset = sizeof( GCA_MSG_HDR );
	    hdr.data_length = rsm->usrptr - rsm->usrbuf;
	    hdr.buffer_length = hdr.data_offset + hdr.data_length;
	    hdr.flags.flow_indicator = rsmhdr->flags.flow_indicator;
	    hdr.flags.end_of_data = (rsmhdr->flags.end_of_data  && 
				     ! rsmhdr->data_length);
	    hdr.flags.end_of_group = rsmhdr->flags.end_of_group;

	    MECOPY_CONST_MACRO( (PTR)&hdr, sizeof( GCA_MSG_HDR ), 
				(PTR)RV_PARMS_MACRO->gca_buffer );
	}

	/* 
	** do work of GCA_INTERPET at API_LEVEL_4 
	*/
	if ( gca_cb->gca_api_version >= GCA_API_LEVEL_4 )
	{
	    RV_PARMS_MACRO->gca_message_type = rsmhdr->msg_type;
	    RV_PARMS_MACRO->gca_d_length = rsm->usrptr - rsm->usrbuf;
	    RV_PARMS_MACRO->gca_data_area = RV_PARMS_MACRO->gca_buffer + 
					    gca_cb->gca_header_length;
	    RV_PARMS_MACRO->gca_end_of_data = ( rsmhdr->flags.end_of_data  && 
					        ! rsmhdr->data_length );
	    RV_PARMS_MACRO->gca_end_of_group = rsmhdr->flags.end_of_group;
	}

	GCA_DEBUG_MACRO(2)
	    ( "%04d   GCA_RECEIVE msg=%s data_len=%d eod=%d eog=%d\n",
	      (i4)(acb ? acb->assoc_id : -1), 
	      gcx_look( gcx_gca_msg, rsmhdr->msg_type ),
	      (i4)(rsm->usrptr - rsm->usrbuf),
	      (i4)(rsmhdr->flags.end_of_data  &&  ! rsmhdr->data_length),
	      (i4)(rsmhdr->flags.end_of_group) );

	/* 
	** Check for attention message. 
	*/
	if ( rsmhdr->msg_type == GCA_ATTENTION  &&  rsmhdr->flags.end_of_data )
	{
	    acb->flags.iack_owed = TRUE;
	    acb->flags.rpurging = TRUE;
	    acb->flags.chop_mks_owed = TRUE;
	}

	/*
	** Try to flush normal send channel with chop marks.  
	** We owe chop marks after the sending or receipt of 
	** an GCA_ATTENTION message.
	*/
	if ( acb->flags.chop_mks_owed )  gca_purge( svc_parms );
	break;

    case SD_INIT:
        /* protect normal-channel SEND operations from AST initiated
        ** gca_purge() attempts while SEND in progress, until we reach
        ** SD_MORE processing - so protecting ssm buffer */
        if ( ! svc_parms->gc_parms.flags.flow_indicator )
        {
            send_active_set = acb->flags.send_active = TRUE;
        }

	GCA_DEBUG_MACRO(2)
	    ( "%04d   GCA_SEND msg=%s snd_len=%d eod=%d flow=%s snd_act=%s\n",
	      (i4)(acb ? acb->assoc_id : -1), 
	      gcx_look( gcx_gca_msg, SD_PARMS_MACRO->gca_message_type ),
	      SD_PARMS_MACRO->gca_msg_length,
	      (i4)(SD_PARMS_MACRO->gca_end_of_data),
	      svc_parms->gc_parms.flags.flow_indicator ? "exp" : "nor",
              acb->flags.send_active ? "true" : "false" );

	break;

    case SD_CKIACK:
	/* 
	** Check if we "owe" a GCA_IACK message due to interrupt receipt.
	** If so, reject any non-GCA_IACK messages with the E_GC0022_NOT_IACK
	** message.  
	*/

	if ( acb->flags.iack_owed )
	{
	    if ( SD_PARMS_MACRO->gca_message_type != GCA_IACK )
	    {
		svc_parms->gc_parms.status = E_GC0022_NOT_IACK;
		branch = TRUE;
	    } 
	    else 
	    {
		/*
		** We only want to send the IACK message,
		** so make sure the send buffer is empty.
		*/
		ssm->bufptr = ssm->bufend = ssm->buffer;
		acb->flags.iack_owed = FALSE;

                /* if 'chop marks' owed here, then a GCA_ATTENTION has been
                ** received so unprotect 'send' operation and send
                ** chop marks now.
                */
                if ( acb->flags.chop_mks_owed )
                {
                    acb->flags.send_active = FALSE;
                    gca_purge( svc_parms );
                    acb->flags.send_active = send_active_set;
                }
	    }
	}
	break;

    case SD_CKATTN:
	/* 
	** If sending the final (or only) segment of an interrupt 
	** message set association to the "purging" state.  We are 
	** then no longer interested in how much more of an incoming 
	** message was still to come.
	*/
	if ( SD_PARMS_MACRO->gca_message_type == GCA_ATTENTION && 
	     SD_PARMS_MACRO->gca_end_of_data )
	    acb->flags.spurging = TRUE;
	break;

    case SD_NEEDTD:
	/* if we need to send GCA_TO_DESCR message */
	if ( 
	     acb->flags.heterogeneous  &&  acb->flags.initial_seg  &&
	     ( SD_PARMS_MACRO->gca_message_type == GCA_TUPLES  ||
	       SD_PARMS_MACRO->gca_message_type == GCA_CDATA )  &&
	     SD_PARMS_MACRO->gca_descriptor != NULL
	   )
	{
	    PTR	 descr = SD_PARMS_MACRO->gca_descriptor;
	    bool frmtd = ( (gca_cb->gca_api_version >= GCA_API_LEVEL_5)  &&
			   (SD_PARMS_MACRO->gca_modifiers & GCA_FORMATTED) );

	    branch = (acb->id_to_descr != gca_descr_id( frmtd, descr ));
	}
	break;

    case SD_IS_FMT:
	/* if GCA_FORMATTED */
	branch = ( (gca_cb->gca_api_version >= GCA_API_LEVEL_5)  &&
		   (SD_PARMS_MACRO->gca_modifiers & GCA_FORMATTED) );
	break;

    case SD_NOFLUSH:
	/* if we can skip flushing */
	branch = ( ! ssmhdr->flags.end_of_data  || 
		   ! ssmhdr->flags.end_of_group );
	break;

    case SD_MORE:
	/* if buffered data and no error */
	branch = ( svc_parms->gc_parms.status == OK  &&
		   ssm->bufptr < ssm->bufend );

	if ( ! svc_parms->gc_parms.flags.flow_indicator )
	      acb->flags.send_active = FALSE;
	break;

    case SD_DOTD:
	/*
	** In a heterogeneous connection, we must prepend a description
	** of any tuple data the client sends.  The comm server PL needs 
	** this description to decompose the message for conversion.  For 
	** the GCA_TUPLES and GCA_CDATA messages, we will send our own 
	** GCA_TO_DATA message built from the descriptor supplied as 
	** SD_PARMS_MACRO->gca_descriptor.  We only need do this for the first
	** segment of the tuple data message, and only if the descriptor
	** is different from the last tuple data message.
	*/
	{
	    PTR	 descr = SD_PARMS_MACRO->gca_descriptor;
	    bool frmtd = ( (gca_cb->gca_api_version >= GCA_API_LEVEL_5)  &&
			   (SD_PARMS_MACRO->gca_modifiers & GCA_FORMATTED) );

	    acb->id_to_descr = gca_descr_id( frmtd, descr );

	    /*
	    ** Compute length of GCA_TO_DESCR message.  Computing the length
	    ** of the message and formatting the message are broken apart
	    ** so we can take care of the allocation in this routine.
	    */
	    ssmhdr->msg_type = GCA_TO_DESCR;
	    ssmhdr->data_offset = sizeof( GCA_MSG_HDR );
	    ssmhdr->data_length = gca_to_len( frmtd, descr );
	    ssmhdr->buffer_length = ssmhdr->data_offset + ssmhdr->data_length;
	    ssmhdr->flags.end_of_data = TRUE;
	    ssmhdr->flags.end_of_group = FALSE;
	    ssmhdr->flags.flow_indicator = 
				    svc_parms->gc_parms.flags.flow_indicator;
	    /* 
	    ** Allocate GCA_TO_DESCR message area.  If the message is bigger 
	    ** than the previous ones sent, we'll need to reallocate a larger 
	    ** buffer.  Since these descriptors are small and sent often, it
	    ** makes more sense to keep the buffer around than allocating and
	    ** freeing it each time.  It is finally freed in gca_del_acb(),
	    ** when the association closes.
	    */

	    if ( ssmhdr->data_length > acb->sz_to_descr )
	    {
		if ( acb->sz_to_descr )  gca_free( acb->to_descr );

		if (! (acb->to_descr = (char *)gca_alloc(ssmhdr->data_length)))
		{
		    svc_parms->gc_parms.status = E_GC0013_ASSFL_MEM;
		    branch = TRUE;
		    break;
		}

		acb->sz_to_descr = ssmhdr->data_length;
	    }

	    /* 
	    ** Fill in the TD in the new message area 
	    */
	    gca_to_fmt( frmtd, descr, (GCA_OBJECT_DESC *)acb->to_descr );

	    /* 
	    ** Save request for SD_SVCUSR 
	    */
	    ssm->usrbuf = ssm->usrptr = acb->to_descr;
	    ssm->usrlen = ssmhdr->data_length;
	}
	break;

    case SD_DOUSR:
	/*
	** Fill in message header for sending user data.
	*/
	ssmhdr->msg_type = SD_PARMS_MACRO->gca_message_type;
	ssmhdr->data_offset = sizeof( GCA_MSG_HDR );
	ssmhdr->data_length = SD_PARMS_MACRO->gca_msg_length;
	ssmhdr->buffer_length = ssmhdr->data_offset + ssmhdr->data_length;
	ssmhdr->flags.flow_indicator = svc_parms->gc_parms.flags.flow_indicator;
	ssmhdr->flags.end_of_data = SD_PARMS_MACRO->gca_end_of_data;

	/*
	** Determine end-of-group if not explicitly given.
	*/
	if ( gca_cb->gca_api_version < GCA_API_LEVEL_5 )
	    ssmhdr->flags.end_of_group = 
				gca_is_eog( ssmhdr->msg_type,
					acb->gca_my_info.gca_partner_protocol );
	else  if ( SD_PARMS_MACRO->gca_modifiers & GCA_EOG )
	    ssmhdr->flags.end_of_group = TRUE;
	else  if ( SD_PARMS_MACRO->gca_modifiers & GCA_NOT_EOG )
	    ssmhdr->flags.end_of_group = FALSE;
	else
	    ssmhdr->flags.end_of_group =
				gca_is_eog( ssmhdr->msg_type,
					acb->gca_my_info.gca_partner_protocol );

	/*
	** The initial segment flag is set when we see the last
	** segment of the current message so that it will be set
	** for the first segment of the next message.  It will
	** be cleared if this is not the last segment since the
	** next segment will be a part of the current message.
	*/
	acb->flags.initial_seg = SD_PARMS_MACRO->gca_end_of_data;

	/*
	** Prepare to send message in user's buffer.
	*/
	ssm->usrbuf = ssm->usrptr = SD_PARMS_MACRO->gca_buffer + 
				    gca_cb->gca_header_length;
	ssm->usrlen = SD_PARMS_MACRO->gca_msg_length;

	break;

    case SD_SVCHDR:
	/*
	** Prepare to send header from ssm header area.
	*/
	ssm->svcbuf = (char *)ssmhdr;
	ssm->svclen = sizeof( GCA_MSG_HDR );
	break;

    case SD_SVCUSR:
	/*
	** Prepare to send data from user buffer.
	*/
	ssm->svcbuf = ssm->usrptr;
	ssm->svclen = ssm->usrlen;
	ssm->usrptr += ssm->svclen;
	ssm->usrlen -= ssm->svclen;
	break;

    case SD_BUFF:
	/*
	** Copy data to send buffer.  If insufficient space
	** for current request, copy what will fit and adjust
	** request parameters accordingly.
	*/

	len = min(acb->size_advise - (ssm->bufend - ssm->buffer), ssm->svclen);
	GCA_DEBUG_MACRO(4)( "send buffering %d of %d\n", len, ssm->svclen );

	if ( len )
	{
	    MEcopy( ssm->svcbuf, len, ssm->bufend );
	    ssm->bufend += len;
	    ssm->svcbuf += len;
	    ssm->svclen -= len;
	}

	/* if we buffered it all */
	branch = ( ! ssm->svclen );
	break;

    case SD_BUFF1:
	/*
	** Copy data to send buffer.  If insufficient space
	** for current request, wait until room is available.
	*/

	len = (ssm->svclen <= (acb->size_advise - (ssm->bufend - ssm->buffer))) 
	      ? ssm->svclen : 0;
	GCA_DEBUG_MACRO(4)( "send buffering %d of %d\n", len, ssm->svclen );

	if ( len )
	{
	    MEcopy( ssm->svcbuf, len, ssm->bufend );
	    ssm->bufend += len;
	    ssm->svcbuf += len;
	    ssm->svclen -= len;
	}

	/* if we buffered it all */
	branch = ( ! ssm->svclen );
	break;

    case SD_FMT:
	/*
	** Encode data into ssm buffer
	**
	** Note that the doc->state flag is assumed correct.
	** The header EOD flag may change if we need to split
	** the message, so the correct input for gco_encode() 
	** is the sender's EOD setting.
	*/
	((GCO_DOC*)ssm->doc)->message_type = ssmhdr->msg_type;
	((GCO_DOC*)ssm->doc)->eod = SD_PARMS_MACRO->gca_end_of_data;
	((GCO_DOC*)ssm->doc)->src1 = ssm->usrptr;
	((GCO_DOC*)ssm->doc)->src2 = ssm->usrptr + ssm->usrlen;
	((GCO_DOC*)ssm->doc)->dst1 = ssm->bufend;
	((GCO_DOC*)ssm->doc)->dst2 = ssm->buffer + acb->size_advise;
	len = ((GCO_DOC*)ssm->doc)->src2 - ((GCO_DOC*)ssm->doc)->src1;

	svc_parms->gc_parms.status = gco_encode( ((GCO_DOC*)ssm->doc) );

	GCA_DEBUG_MACRO(4)( "send buffering %d of %d (%d output bytes)\n", 
			    len - (((GCO_DOC*)ssm->doc)->src2 - 
			           ((GCO_DOC*)ssm->doc)->src1), len,
			    (i4)(((GCO_DOC*)ssm->doc)->dst1 - ssm->bufend) );

	/*
	** The message header procedes the message.  SD_BUFF1 ensures
	** that the entire header proceeds the message while the state
	** table ensures that there is always a message header in the
	** buffer prior to execution of SD_FMT.  We were not able to 
	** place an accurate data length in the message header since 
	** we did not know the encoded length until now.  Also, the
	** end-of-data flag may be incorrect since it was set based on
	** input conditions and may need adjusting for the output state.
	**
	** Update the message header with the correct info.
	*/
	ssmhdr->data_length = ((GCO_DOC*)ssm->doc)->dst1 - ssm->bufend;
	ssmhdr->buffer_length = ssmhdr->data_offset + ssmhdr->data_length;
	ssmhdr->flags.end_of_data = 
			(((GCO_DOC*)ssm->doc)->state == DOC_MOREOUT) 
			? 0 : SD_PARMS_MACRO->gca_end_of_data;

	MEcopy( (PTR)ssmhdr, sizeof( GCA_MSG_HDR ),
		(PTR)(ssm->bufend - sizeof( GCA_MSG_HDR )) );
	ssm->bufend = ((GCO_DOC*)ssm->doc)->dst1;

	/*
	** Update input buffer.
	*/
	ssm->usrptr = ((GCO_DOC*)ssm->doc)->src1;
	ssm->usrlen = ((GCO_DOC*)ssm->doc)->src2 - ((GCO_DOC*)ssm->doc)->src1;

	/* 
	** Branch = 1 when input is depleted.
	** Branch = 0 when output is depleted. 
	*/
	if( ((GCO_DOC*)ssm->doc)->state != DOC_MOREOUT )  branch = TRUE;
	break;

    case SD_DONE:
	/* 
	** Make sure encoder is shut-down.  When formatted
	** and unformatted messages are intermixed, the
	** encoder will be left in an invalid state if the
	** last part of the message is sent unformatted.
	*/
	if ( SD_PARMS_MACRO->gca_end_of_data )
	    ((GCO_DOC*)ssm->doc)->state = DOC_IDLE;

	/*
	** Try to flush normal send channel with chop marks.  We
	** owe chop marks after the sending or receipt of an GCA_ATTENTION
	** message.
	*/
	if ( SD_PARMS_MACRO->gca_message_type == GCA_ATTENTION && 
	     SD_PARMS_MACRO->gca_end_of_data )
	    acb->flags.chop_mks_owed = TRUE;

	if ( acb->flags.chop_mks_owed )  gca_purge( svc_parms );

	break;

    case DA_DISCONN:
	/* 
	** Force completion of outstanding requests 
	*/
	{
	    i4  i;

	    for( i = GCA_NORMAL; i <= GCA_EXPEDITED; i++ )
	    {
		if ( acb->snd_parms[i].in_use )
		    ((GCA_SD_PARMS *)(acb->snd_parms[i].parameter_list))-> 
					gca_status = E_GC0023_ASSOC_RLSED;

		if ( acb->rcv_parms[i].in_use )
		    ((GCA_RV_PARMS *)(acb->rcv_parms[i].parameter_list))->
					gca_status = E_GC0023_ASSOC_RLSED;
	    }
	}
	break;

    case DA_COMPLETE:
	/* 
	** Free the acb and complete. 
	*/
	acb->flags.delete_acb = TRUE;
	break;

    case SA_INITIATE:
	/* 
	** Do GCA_INITIATE 
	*/
	gca_initiate( svc_parms );
	break;

    case SA_TERMINATE:
	/* 
	** Do GCA_TERMINATE 
	*/
	gca_terminate( svc_parms );
	return;

    case SA_SAVE:
	/* 
	** Do GCA_SAVE 
	*/
	gca_save( svc_parms );
	break;

    case SA_RESTORE:
	/* 
	** Do GCA_RESTORE 
	*/
	gca_restore( svc_parms );

	/*
	** GCA_RESTORE does not initially have an ACB, but
	** gca_restore() allocates one if successful.
	*/
	acb = svc_parms->acb;
	break;

    case SA_FORMAT:
	/* 
	** Do GCA_FORMAT 
	*/
	if ( gca_cb->gca_api_version >= GCA_API_LEVEL_5 )
	{
	    FO_PARMS_MACRO->gca_status = E_GC0003_INV_SVC_CODE;
	}
	else  if ( FO_PARMS_MACRO->gca_b_length < sizeof(GCA_MSG_HDR) )
	{
	    /* Big enough buffer?  Proper pointer? */

	    FO_PARMS_MACRO->gca_status = E_GC0010_BUF_TOO_SMALL;
	}
	else  if ( ! FO_PARMS_MACRO->gca_buffer  )
	{
	    FO_PARMS_MACRO->gca_status = E_GC0009_INV_BUF_ADDR;
	}
	else
	{
	    FO_PARMS_MACRO->gca_data_area = FO_PARMS_MACRO->gca_buffer + 
					    sizeof( GCA_MSG_HDR );
	    FO_PARMS_MACRO->gca_d_length = FO_PARMS_MACRO->gca_b_length - 
					   sizeof( GCA_MSG_HDR );
	    FO_PARMS_MACRO->gca_status = OK;
	}
	break;

    case SA_INTERPRET:
	/* 
	** Do GCA_INTERPRET 
	*/
	if ( gca_cb->gca_api_version >= GCA_API_LEVEL_5 )
	{
	    IT_PARMS_MACRO->gca_status = E_GC0003_INV_SVC_CODE;
	}
	else  if ( !IT_PARMS_MACRO->gca_buffer )
	{
	    IT_PARMS_MACRO->gca_status = E_GC0009_INV_BUF_ADDR;
	}
	else
	{
	    GCA_MSG_HDR	hdr;

	    /* fill in parm list from message buffer header */

	    MECOPY_CONST_MACRO( (PTR)IT_PARMS_MACRO->gca_buffer,
				sizeof( hdr ), (PTR)&hdr);

	    IT_PARMS_MACRO->gca_message_type = hdr.msg_type;
	    IT_PARMS_MACRO->gca_data_area = IT_PARMS_MACRO->gca_buffer + 
					    hdr.data_offset;
	    IT_PARMS_MACRO->gca_d_length = hdr.data_length;
	    IT_PARMS_MACRO->gca_end_of_data = hdr.flags.end_of_data;
	    IT_PARMS_MACRO->gca_end_of_group = hdr.flags.end_of_group;
	    IT_PARMS_MACRO->gca_status = OK;
	}
	break;

    case SA_ATTRIBS:
	/* 
	** Do GCA_ATTRIBS 
	*/
	AT_PARMS_MACRO->gca_flags = acb->flags.heterogeneous ? GCA_HET : 0;
	break;

    case SA_REG_TRAP:
	/* 
	** Do GCA_REG_TRAP 
	*/
	gcm_reg_trap( svc_parms );
	break;

    case SA_UNREG_TRAP:
	/* 
	** Do GCA_UNREG_TRAP 
	*/
	gcm_unreg_trap( svc_parms );
	break;

    case GC_REGISTER:
	/*
	** Call GCregister() to establish a local IPC endpoint.
	*/

	svc_parms->gc_parms.num_connected = RG_PARMS_MACRO->gca_no_connected;
	svc_parms->gc_parms.num_active = RG_PARMS_MACRO->gca_no_active;
	svc_parms->gc_parms.svr_class = RG_PARMS_MACRO->gca_srvr_class;
	STcopy( svc_parms->gc_parms.svr_class, gca_cb->gca_srvr_class );

	GCregister( &svc_parms->gc_parms );

	if ( svc_parms->gc_parms.status != OK )
	    break;

	/*
	** Extract server configuration parameters, 
	** and save in GCA control block.
	*/
	gca_cb->gca_flags |= RG_PARMS_MACRO->gca_modifiers & GCA_RG_MIB_MASK;
	STcopy( svc_parms->gc_parms.listen_id, gca_cb->gca_listen_address );
	RG_PARMS_MACRO->gca_listen_id = svc_parms->gc_parms.listen_id;

	break;

    case GC_LISTEN:
	/* 
	** Call lower level to post listen.
	*/

	GClisten( &svc_parms->gc_parms );
	return;

    case GC_REQUEST:
	/*
	** Connect to target.
	*/

	GCrequest( &svc_parms->gc_parms );
	return;

    case GC_SEND:
	/* 
	** Invoke GCsend to send buffer.
	*/
	svc_parms->gc_parms.svc_buffer = ssm->bufptr;
	svc_parms->gc_parms.svc_send_length = 
	    min( ssm->bufend - ssm->bufptr, acb->gc_size_advise );
	ssm->bufptr += svc_parms->gc_parms.svc_send_length;
	if ( ssm->bufptr >= ssm->bufend )
	   ssm->bufptr = ssm->bufend = ssm->buffer;

	if ( IIGCa_global.gca_trace_level >= 5 )
	 if  ((( (GCA_MSG_HDR *)svc_parms->gc_parms.svc_buffer)->msg_type 
		!= GCN_NS_RESOLVE ) && 
	     (( (GCA_MSG_HDR *)svc_parms->gc_parms.svc_buffer)->msg_type 
		!= GCN_NS_2_RESOLVE ) ) 
	   gcx_tdump( svc_parms->gc_parms.svc_buffer, 
		       svc_parms->gc_parms.svc_send_length );
#ifdef  xDEBUG
	 else 
	   gcx_tdump( svc_parms->gc_parms.svc_buffer, 
		       svc_parms->gc_parms.svc_send_length );
#endif

	IIGCa_global.gca_msgs_out++;
	IIGCa_global.gca_data_out += ( IIGCa_global.gca_data_out_bits +
				       svc_parms->gc_parms.svc_send_length ) 
				     / 1024;
	IIGCa_global.gca_data_out_bits = ( IIGCa_global.gca_data_out_bits + 
					   svc_parms->gc_parms.svc_send_length )
					 % 1024;

	if ( ! svc_parms->gc_parms.flags.flow_indicator )
	      acb->flags.send_active = TRUE;

	GCsend( &svc_parms->gc_parms );
	break;

    case GC_ASYNC:
	/*
	** This action is used to handle async GC requests.  It helps
	** to limit recursion when requests complete immediately.  
	** This action should immediately follow a GC action which
	** does not itself contain a return statement.
	**
	** When an async GC request is made, one of two results may
	** occur:
	**   1. The request is initiated and the call returns without 
	**	being complete.
	**   2. The request completes immediately and the callback
	**	occurs.
	**
	** In both of these case, this action is the next to execute.
	**   1. After returning from a GC call which didn't complete, 
	**	this action executes and control returns out of 
	**	gca_service.
	**   2. When the immediate callback is made, this action
	**	executes and control returns out of the callback.
	**
	** Processing of subsequent actions then occurs:
	**   1. When the request finally completes, the callback
	**	is made and the action following this one is 
	**	executed.
	**   2. The return from the callback results in the return 
	**	from the GC call.  In the incomplete case, it is 
	**	this action which would then execute.  But in the 
	**	complete case, this action has already executed 
	**	during the callback.  Therefore, it is the next 
	**	action which is executed.
	*/
	return;

    case GC_RECEIVE:
	/*
	** Fill buffer by calling GCreceive.
	** rsm->bufend should point to where 
	** new input should be placed in the 
	** buffer.  Any data before rsm->bufend 
	** will be included in the resulting 
	** input.
	*/
	svc_parms->gc_parms.svc_buffer = rsm->bufend;
	svc_parms->gc_parms.rcv_data_length = 
	    svc_parms->gc_parms.reqd_amount = rsm->bufsiz - 
					      (rsm->bufend - rsm->buffer);

	if ( rsm->bufend > rsm->buffer )
	{
	    /*
	    ** GCA guarantees room preceding buffers for 
	    ** GC CL usage.  We allocated space preceding
	    ** our buffers for the CL when the buffers are
	    ** empty.  When the buffers are not empty, the
	    ** pre-existing data in the region which may
	    ** be used by the CL must be temporarily saved.
	    ** The data will be restored in SA_RECV_DONE.  
	    ** Because of the way the existing data offsets
	    ** the region which may be used by the CL, there
	    ** is just enough room in the space preceding 
	    ** the buffer (the area reserved for the CL) to 
	    ** save our data,
	    */
	    u_i2 len = min( rsm->bufend - rsm->buffer, GCA_CL_HDR_SZ );
	    MEcopy( (PTR)(rsm->bufend - len), len, 
		    (PTR)(rsm->buffer - GCA_CL_HDR_SZ) ); 
	}

	GCreceive( &svc_parms->gc_parms );
	return;

    case GC_DISCONN:
	/*
	** Disconnect with GCdisconn.
	*/
	if ( acb->gc_cb )
	{
	    GCdisconn( &svc_parms->gc_parms );
	    return;
	}
	break;

    case GC_RELEASE:
	/* 
	** Free OS resources. 
	*/
	if ( acb->gc_cb )
	{
	    GCrelease( &svc_parms->gc_parms );
	    acb->gc_cb = NULL;
	}
	break;

    case GC_DELPASSWD:
# ifdef NT_GENERIC
        /* remove password entry from cached list */
        GCpasswd_delete_vnode_info(RQ_PARMS_MACRO->gca_partner_name);
# endif
        break;
    }

    /* Take branch if set */

    if ( branch )
	svc_parms->state = labels[ gca_states[ svc_parms->state - 1 ].label ];

    goto top;
}


/*
** Name: gca_check_peer
**
** Description:
**	Check input buffer to ensure entire peer info has been received.
**
** Input:
**	buffer	Input buffer.
**	length	Length of available input.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if entire peer info has been received.
**
** History:
**	17-Jul-09 (gordy)
**	    Created.
*/

static bool
gca_check_peer( char *buffer, i4 length )
{
    i4			info[3], count;
    GCA_PEER_V6		peer_v6;
    GCA_AUX_DATA	hdr;

    /*
    ** The peer info structure starts with an array of i4's.  
    ** The peer info version is the second element.
    */
    if ( length < sizeof( info ) )  return( FALSE );
    MEcopy( (PTR)buffer, sizeof( info ), (PTR)info );

    /*
    ** At version 5 and less, peer info is fixed length.
    ** There are no other identfying characteristics.
    */
    if ( info[1] <= GCA_IPC_PROTOCOL_5 ) return(length >= sizeof(GCA_PEER_V5));

    /*
    ** At version 6 and greater, the first element is the 
    ** peer info size and the third is a constant ID.
    **
    ** We can't trust the size if the ID isn't valid.
    ** Validation isn't actually done here.  Sufficient 
    ** data has been received to permit validation, 
    ** which will be done in gca_load_peer_v*().
    */
    if ( info[2] != GCA_PEER_ID )  return( TRUE );

    /*
    ** We need to receive at least the size indicated.
    ** Starting at version 7, size is full peer info length.
    */
    if ( length < info[0] )  return( FALSE );
    if ( info[1] >= GCA_IPC_PROTOCOL_7 )  return( TRUE );

    /*
    ** At version 6, size refers only to fixed portion of 
    ** peer info.  Aux data follows fixed portion and we 
    ** need to make sure it has been entirely received.
    */
    MEcopy( (PTR)buffer, sizeof( peer_v6 ), (PTR)&peer_v6 );
    buffer += peer_v6.gca_length;
    length -= peer_v6.gca_length;

    for( count = peer_v6.gca_aux_count; count > 0; count-- )
    {
	if ( length < sizeof( hdr ) )  return( FALSE );
	MEcopy( (PTR)buffer, sizeof( hdr ), (PTR)&hdr );
	if ( length < hdr.len_aux_data )  return( FALSE );
	buffer += hdr.len_aux_data;
	length -= hdr.len_aux_data;
    }

    return( TRUE );
}


/*
** Name: gca_frmt_v5_peer
**
** Description:
**	Formats a version 5 or lower peer info structure from standard 
**	peer info.  It is assumed that the old peer info is only sent 
**	server to client, so username, password and aux data are not 
**	processed.
**
** Input:
**	acb		Association Control Block.
**	peer		Peer info.
**
** Output:
**	buffer		Buffer to receive v5 peer info.
**
** Returns:
**	i4		Length of output.
**
** History:
**	 6-May-97 (gordy)
**	    Created.
**	17-Jul-09 (gordy)
**	    Standardized peer version processing.
*/

static i4
gca_frmt_v5_peer( GCA_ACB *acb, GCA_PEER_INFO *peer, PTR buffer )
{
    GCA_PEER_V5 peer_v5;

    MEfill( sizeof( peer_v5 ), EOS, (PTR)&peer_v5 );

    peer_v5.gca_version = GCA_IPC_PROTOCOL_5;
    peer_v5.gca_partner_protocol = peer->gca_partner_protocol;
    peer_v5.gca_status = peer->gca_status;
    peer_v5.gca_flags = peer->gca_flags;

    MEcopy( (PTR)&peer_v5, sizeof( peer_v5 ), buffer );
    return( sizeof( peer_v5 ) );
}


/*
** Name: gca_load_v5_peer
**
** Description:
**	Validate V5 peer info structure and load information.
**
**	Full peer info data must be available as checked by
**	gca_check_peer().
**
** Input:
**	acb		Association Control Block.
**	buffer		Input buffer.
**
** Output:
**	buffer		Updated buffer position
**	peerinfo	Loaded peer info.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 6-May-97 (gordy)
**	    Created.
**	18-Mar-98 (gordy)
**	    If no password is provided, ignore the peer user ID.
**	    GCA will use the CL provided user ID instead.  Do not
**	    need to explicitly use SYSTEM mechanism for passwords.
**	 9-Jul-04 (gordy)
**	    Do use SYSTEM mechanism for passwords to allow disable.
**	17-Jul-09 (gordy)
**	    Peer user ID's dynamically allocated.
*/

STATUS
gca_load_v5_peer( GCA_ACB *acb, char **buffer, GCA_PEER_INFO *peer )
{
    GCA_PEER_V5	peer_v5;
    STATUS	status = OK;
    u_i1	auth[ 1024 ];	/* TODO */
    i4		length;

    MEcopy( *buffer, sizeof( peer_v5 ), (PTR)&peer_v5 );
    *buffer += sizeof( peer_v5 );

    if ( peer_v5.gca_version < 0  ||  peer_v5.gca_version > GCA_IPC_PROTOCOL_5 )
    {
	GCA_DEBUG_MACRO(1)( "Invalid IPC protocol level: %d\n",
			 peer_v5.gca_version );
    	return( E_GC000A_INT_PROT_LVL );
    }

    peer->gca_version = peer_v5.gca_version;
    peer->gca_flags = peer_v5.gca_flags;
    peer->gca_partner_protocol = peer_v5.gca_partner_protocol;
    peer->gca_status = peer_v5.gca_status;
    peer->gca_aux_count = 0;
    acb->gca_aux_len = 0;

    /*
    ** The old protocol did not support authentication
    ** info.  We can generate authentication info using
    ** the security mechanism representing the old
    ** protocol.
    */
    if ( peer_v5.gca_password[ 0 ] != EOS )
    {
	GCS_PWD_PARM	pwd_parm;

	gca_save_peer_user( peer, peer_v5.gca_user_id );
	pwd_parm.user = peer_v5.gca_user_id;
	pwd_parm.password = peer_v5.gca_password;
	pwd_parm.length = sizeof( auth );
	pwd_parm.buffer = auth;

	status = IIgcs_call(GCS_OP_PWD_AUTH, GCS_MECH_SYSTEM, (PTR)&pwd_parm);
	length = pwd_parm.length;
    }
    else
    {
	GCS_USR_PARM	usr_parm;

	/*
	** Dump the requested user ID.  GCA will use the CL provided
	** user ID.  Use the SYSTEM mechanism to create a user AUTH
	** since it does not explicitly use the user ID of the current
	** process.
	*/
	gca_save_peer_user( peer, NULL );
	usr_parm.length = sizeof( auth );
	usr_parm.buffer = auth;

	status = IIgcs_call(GCS_OP_USR_AUTH, GCS_MECH_SYSTEM, (PTR)&usr_parm);
	length = usr_parm.length;
    }

    if ( status == OK )
    {
	status = gca_aux_element( acb, GCA_ID_AUTH, length, auth );
	peer->gca_aux_count++;
    }

    /*
    ** Convert aux data.
    */
    if ( status == OK  &&  peer_v5.gca_aux_data.type_aux_data )
    {
	if ( peer_v5.gca_aux_data.type_aux_data == GCA_ID_RMT_ADDR )
	    status = gca_aux_element( acb, GCA_ID_RMT_ADDR,
				      sizeof( GCA_RMT_ADDR ), 
				      (PTR)&peer_v5.gca_aux_data.rmt_addr);
	else
	    status = gca_aux_element( acb, peer_v5.gca_aux_data.type_aux_data, 
				      0, NULL );
	
	peer->gca_aux_count++;
    }

    return( status );
}


/*
** Name: gca_frmt_v6_peer
**
** Description:
**	Formats a version 6 peer info structure from standard peer info.  
**	It is assumed that the old peer info is only sent server to client, 
**	so username and aux data are not processed.
**
** Input:
**	acb		Association Control Block.
**	peer		Peer info.
**
** Output:
**	buffer		Buffer to receive v6 peer info.
**
** Returns:
**	i4		Length of output.
**
** History:
**	17-Jul-09 (gordy)
**	    Created.
*/

static i4
gca_frmt_v6_peer( GCA_ACB *acb, GCA_PEER_INFO *peer, PTR buffer )
{
    GCA_PEER_V6 peer_v6;
    int		length = sizeof( peer_v6 );

    MEfill( sizeof( peer_v6 ), EOS, (PTR)&peer_v6 );

    peer_v6.gca_length = sizeof( GCA_PEER_V6 );
    peer_v6.gca_version = GCA_IPC_PROTOCOL_6;
    peer_v6.gca_id = GCA_PEER_ID;
    peer_v6.gca_partner_protocol = peer->gca_partner_protocol;
    peer_v6.gca_status = peer->gca_status;
    peer_v6.gca_flags = peer->gca_flags;

    MEcopy( (PTR)&peer_v6, length, buffer );
    return( length );
}


/*
** Name: gca_load_v6_peer
**
** Description:
**	Validate peer info structure and load information.
**
**	Full peer info data must be available as checked by
**	gca_check_peer().
**
** Input:
**	acb		Association Control Block.
**	buffer		Input buffer.
**
** Output:
**	buffer		Updated buffer position
**	peer		Loaded peer info.
**
** Returns:
**	STATUS
**
** History:
**	17-Jul-09 (gordy)
**	    Created.
*/

static STATUS
gca_load_v6_peer( GCA_ACB *acb, char **buffer, GCA_PEER_INFO *peer )
{
    GCA_PEER_V6	peer_v6;
    char	*aux;
    i4		count;

    MEcopy( *buffer, sizeof( GCA_PEER_V6 ), (PTR)&peer_v6 );

    if ( peer_v6.gca_id != GCA_PEER_ID )
    {
	GCA_DEBUG_MACRO(1)( "Invalid peer info ID: 0x%x\n", peer_v6.gca_id );
	return( E_GC000A_INT_PROT_LVL );
    }

    if ( peer_v6.gca_version != GCA_IPC_PROTOCOL_6 )  
    {
	GCA_DEBUG_MACRO(1)( "Invalid IPC protocol level: %d\n",
			 peer_v6.gca_version );
	return( E_GC000A_INT_PROT_LVL );
    }

    peer->gca_version = GCA_IPC_PROTOCOL_6;
    peer->gca_flags = peer_v6.gca_flags;
    peer->gca_partner_protocol = peer_v6.gca_partner_protocol;
    peer->gca_status = peer_v6.gca_status;
    peer->gca_aux_count = peer_v6.gca_aux_count;

    /*
    ** Make sure user ID is null terminated.  Overwriting 
    ** gca_aux_count is OK because it has been saved.
    */
    peer_v6.gca_user_id[ sizeof( peer_v6.gca_user_id ) ] = EOS;
    gca_save_peer_user( peer, (STlength( peer_v6.gca_user_id ) > 0)
    			      ? peer_v6.gca_user_id : NULL );

    /*
    ** Aux data follows fixed portion of peer structure.
    ** Need to scan through aux data items to determine size.
    */
    *buffer += peer_v6.gca_length;
    aux = *buffer;

    for( count = peer->gca_aux_count; count > 0; count-- )
    {
	GCA_AUX_DATA	hdr;

    	MEcopy( *buffer, sizeof( hdr ), (PTR)&hdr );
	*buffer += hdr.len_aux_data;
    }

    return( (*buffer > aux) ? gca_append_aux( acb, *buffer - aux, aux ) : OK );
}


/*
** Name: gca_frmt_v7_peer
**
** Description:
**	Formats a version 7 peer info structure from standard peer info.  
**
** Input:
**	acb		Association Control Block.
**	peer		Peer info.
**
** Output:
**	buffer		Buffer to receive v5 peer info.
**
** Returns:
**	i4		Length of output.
**
** History:
**	17-Jul-09 (gordy)
**	    Created.
*/

static i4
gca_frmt_v7_peer( GCA_ACB *acb, GCA_PEER_INFO *peer, PTR buffer )
{
    GCA_PEER_V7	peer_v7;
    i4		peer_len = sizeof( GCA_PEER_V7 );
    i4		user_len = peer->gca_user_id	/* Include EOS */
    			   ? STlength( peer->gca_user_id ) + 1 : 0;

    peer_v7.gca_length = peer_len + user_len + acb->gca_aux_len;
    peer_v7.gca_version = GCA_IPC_PROTOCOL_7;
    peer_v7.gca_id = GCA_PEER_ID;
    peer_v7.gca_partner_protocol = peer->gca_partner_protocol;
    peer_v7.gca_status = peer->gca_status;
    peer_v7.gca_flags = peer->gca_flags;
    peer_v7.gca_user_len = user_len;
    peer_v7.gca_aux_count = peer->gca_aux_count;

    /*
    ** Build message buffer.  User ID and aux data follows 
    ** fixed portion of peer info.  Currently, no forseen 
    ** combination of AUX data will overflow the 
    ** GCA_FS_MAX_DATA size of our buffers.  This may need 
    ** to be addressed in the future.
    */
    MEcopy( (PTR)&peer_v7, peer_len, buffer );

    if ( user_len )
    {
    	MEcopy( peer->gca_user_id, user_len, buffer + peer_len );
	peer_len += user_len;
    }

    if ( acb->gca_aux_len )
    {
	MEcopy( (PTR)acb->gca_aux_data, acb->gca_aux_len, buffer + peer_len );
	peer_len += acb->gca_aux_len;
    }

    return( peer_len );
}


/*
** Name: gca_load_v7_peer
**
** Description:
**	Validate peer info structure and load information.
**
**	Full peer info data must be available as checked by
**	gca_check_peer().
**
** Input:
**	acb		Association Control Block.
**	buffer		Input buffer.
**
** Output:
**	buffer		Updated buffer position
**	peer		Loaded peer info.
**
** Returns:
**	STATUS
**
** History:
**	17-Jul-09 (gordy)
**	    Created.
*/

static STATUS
gca_load_v7_peer( GCA_ACB *acb, char **buffer, GCA_PEER_INFO *peer )
{
    GCA_PEER_V7	peer_v7;
    char	*ptr;
    i4		count;

    MEcopy( *buffer, sizeof( GCA_PEER_V7 ), (PTR)&peer_v7 );

    if ( peer_v7.gca_id != GCA_PEER_ID )
    {
	GCA_DEBUG_MACRO(1)( "Invalid peer info ID: 0x%x\n", peer_v7.gca_id );
	return( E_GC000A_INT_PROT_LVL );
    }

    if ( peer_v7.gca_version != GCA_IPC_PROTOCOL_7 )  
    {
	GCA_DEBUG_MACRO(1)( "Invalid IPC protocol level: %d\n", 
			    peer_v7.gca_version );
	return( E_GC000A_INT_PROT_LVL );
    }

    peer->gca_version = GCA_IPC_PROTOCOL_7;
    peer->gca_flags = peer_v7.gca_flags;
    peer->gca_partner_protocol = peer_v7.gca_partner_protocol;
    peer->gca_status = peer_v7.gca_status;
    peer->gca_aux_count = peer_v7.gca_aux_count;

    /*
    ** User ID follows fixed portion of peer info structure.
    */
    ptr = *buffer + sizeof( peer_v7 );
    gca_save_peer_user( peer, (peer_v7.gca_user_len > 0) ? ptr : NULL );

    /*
    ** Aux data follows user ID.
    */
    ptr += peer_v7.gca_user_len;
    *buffer += peer_v7.gca_length;

    return( (*buffer > ptr) ? gca_append_aux( acb, *buffer - ptr, ptr ) : OK );
}


/*
** Name: gca_purge - purge normal send channel
**
** Description:
**	Purging is necessary to ensure that when a GCA_ATTENTION
**	message is sent on the expedited channel, the outstanding
**	normal receives complete for both the sender and recipient of
**	the ATTENTION message.  The GCA interface dictates this: it is
**	not intrinsicly necessary.
**	
**	The method chosen was to send "chop marks," or messages which
**	by trickery below the CL would cause the peer's GCrecieve to
**	complete with the "newchop" indicator.  This causes the
**	GCA_RECEIVE to complete with the "request purged" status.
**	Until the local GCA message format is allowed to change, the
**	chop mark implementation remains.
**	
**	A general description of the current purging implementation
**	follows:  When a GCA_SEND completes sending an ATTENTION
**	message or a GCA_RECEIVE completes receiving one, two flags in
**	the ACB are set: chop_mks_owed and purging.
**	
**	    The chop_mks_owed flag controls the sending of the chop
**	    marks: whenever a GCA_SEND or GCA_RECEIVE completes
**	    (including the current one for the ATTENTION message), it
**	    calls gca_purge() if the chop_mks_owed flag is set.  In the
**	    simple case, gca_purge() calls GCpurge() to send the chop
**	    marks and then clears the chop_mks_owed flag.  For some
**	    historical reason, it is unsafe to call GCpurge() if a
**	    normal flow GCsend is outstanding; in this case,
**	    gca_purge() returns without clearing the chop_mks_owed
**	    flag;  presumably when the GCsend completes, the higher
**	    level GCA_SEND will then successfully call gca_purge().
**	
**	    The purging flag alters the actions of GCA_RECEIVE.
**	    Normally, GCA_RECEIVE swallows chop marks (reissues the
**	    GCreceive internally) and completes only for a non-chop
**	    receive.  When the purging flag is set, a chop mark will
**	    cause a GCA_RECEIVE request to complete if the request was
**	    issued before the purging flag was set.  If a GCA_RECEIVE
**	    request is issued while the purging flag is set,
**	    GCA_RECEIVE reads and discards data until it receives a
**	    chop mark.  The purging flag is cleared on receipt of the
**	    first chop mark; normal receive processing then continues.
**	    It's even more confusing to implement.
**
**	Note that at least one chop mark must arrive after the ATTENTION
**	message.  This seems to be difficult to arrange; hence the
**	preponderance of chop marks: GCpurge() sends two, not one, and
**	GCsend() sends a few on occassion.
**
** History:
**	31-Dec-89 (seiwald)
**	    Written.
**	18-Jun-89 (seiwald)
**	    GCpurge now takes a SVC_PARMS, not GCA_ACB.
**	18-Dec-96 (gordy)
**	    Use the normal channel service parms.
**      19-Feb-97 (fanra01)
**          Bug 80681
**          Original service parms should be used when calling GCpurge.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
**	 9-Aug-04 (gordy)
**	    Use ACB flag to indicate active send rather than the
**	    more kludgy testing of normal channel state.
**      04-Dec-2007 (horda03/ashco01) Bug 119120
**          Re-instate check for outstanding GC_SEND and do not call
**          gca_purge() if this is the case.
**	16-Nov-2010 (gordy & rajus01) Bug 124732
**	    An abf hang was reported when porting Ingres 10 on HP-UX PA-RISC
**	    platform. Investigation of the issue with GCA trace revealed that
**	    occurrence of an old problem and a normal channel GCA_SEND 
**	    operation has not happened. It was also noticed in the trace 
**	    that an interrupt (GCA_ATTENTION) had occurred immediately after 
**	    GCA_RESTORE. And GCpurge() was deferred. Since this is a 
**	    GCA_RESTORE, it doesn't really matter what state the snd_parms 
**	    in the parent is in because GCA_SAVE/GCA_RESTORE is
**	    not allowed with active operations. The svc_parms information is 
**	    really only valid if svc->in_use is true indicating that 
**	    a normal channel send is at least active. The test for active 
**	    normal send situation has been extended.	
*/

static VOID
gca_purge( GCA_SVC_PARMS *svc_parms )
{
    GCA_ACB		*acb = svc_parms->acb;
    GCA_SR_SM		*ssm = &acb->send[ GCA_NORMAL ];
    GCA_SVC_PARMS	*svc = &acb->snd_parms[ GCA_NORMAL ];
    bool normal_send_active = ( svc->in_use && 
			      gca_states[ svc->state-1 ].action == GC_SEND );

    GCA_DEBUG_MACRO(3)
	( "%04d   GCA_PURGE %s %s %s\n",
	  (i4)(acb ? acb->assoc_id : -1), 
	  (ssm->bufend > ssm->bufptr) ? "(unflushed data) " : "",
          ( normal_send_active )  ? "deferred" : "",
          (acb->flags.send_active)  ? "SendActive" : "" );

    /*
    ** Don't call GCpurge if a normal GCsend is outstanding.  
    */
    if (acb->flags.send_active || normal_send_active)  return;
    /* 
    ** Buffered send data wouldn't get flushed until after the channel
    ** was purged, so we discard it now.
    */
    ssm->bufptr = ssm->bufend = ssm->buffer;
    acb->flags.chop_mks_owed = FALSE;
 
    /* 
    ** Send "chop marks".  Make sure the
    ** service parms control block is upto
    ** date with ACB.
    */
    svc->gc_parms.gc_cb = acb->gc_cb;
    GCpurge( &svc->gc_parms );

    return;
}

/*
** Name: gca_acb_buffers - (Re)Allocate ACB buffers
**
** Description:
**	This routine (re)allocates the normal and expedited send and
**	receive buffers for the ACB.  The caller is expected to set
**	the four acb .bufsiz's.  If there is a non-NULL acb->buffers
**	at entry, that buffer is freed, and new ones allocated.  (The
**	situation presumably being that after initial buffer allocation
**	with the standard GCA sizes, a listen or request returned a
**	CL-advised size different from the standard.)
**
** Inputs:
**	acb		GCA_ACB pointer with send[].bufsiz and
**			recv[].bufsiz set
**
** Outputs:
**	acb		->buffers, send/recv[].bufptr/bufend all set
**
**	If an error occurs, acb->buffers will be left NULL.
**
** History:
**	11-Feb-2010 (kschendel) SIR 123275
**	    Extract buffer setup so that buffers can be reallocated
**	    with a different/larger size, if the CL asks for it.
*/

static void
gca_acb_buffers( GCA_ACB *acb)
{
    GCA_DEBUG_MACRO(2)( "%04d   %sallocating buffers, normal(+CL)=%d (%d) expedited=%d (%d)\n",
			(i4)acb->assoc_id,
			(acb->buffers != NULL) ? "re-" : "",
			acb->recv[GCA_NORMAL].bufsiz,
			acb->recv[GCA_NORMAL].bufsiz+GCA_CL_HDR_SZ,
			acb->recv[GCA_EXPEDITED].bufsiz,
			acb->recv[GCA_EXPEDITED].bufsiz+GCA_CL_HDR_SZ);

    if (acb->buffers != NULL)
    {
	/* Deallocate what's there.  (Caller has set new sizes and
	** decided that this step is desirable.)
	*/
	gca_free(acb->buffers);
	acb->buffers = NULL;
	acb->recv[ GCA_NORMAL ].bufptr =
	acb->recv[ GCA_NORMAL ].bufend =
	acb->recv[ GCA_NORMAL ].buffer = NULL;
	acb->send[ GCA_NORMAL ].bufptr =
	acb->send[ GCA_NORMAL ].bufend =
	acb->send[ GCA_NORMAL ].buffer = NULL;
	acb->recv[ GCA_EXPEDITED ].bufptr =
	acb->recv[ GCA_EXPEDITED ].bufend =
	acb->recv[ GCA_EXPEDITED ].buffer = NULL;
	acb->send[ GCA_EXPEDITED ].bufptr =
	acb->send[ GCA_EXPEDITED ].bufend =
	acb->send[ GCA_EXPEDITED ].buffer = NULL;
    }

    /* Note that the bufsiz includes any GCA-level header, but does not
    ** include the CL-level header (if any).  Room for a CL header will
    ** be allowed before each buffer.  At present (2010), only the Unix
    ** CL uses a CL header.
    */
    acb->buffers = gca_alloc( (2 * acb->recv[ GCA_NORMAL ].bufsiz) +
			      (2 * acb->recv[ GCA_EXPEDITED ].bufsiz) + 
			      (4 * GCA_CL_HDR_SZ) );
    if (acb->buffers == NULL)
	return;

    acb->recv[ GCA_NORMAL ].bufptr = 
    acb->recv[ GCA_NORMAL ].bufend = 
    acb->recv[ GCA_NORMAL ].buffer = acb->buffers + GCA_CL_HDR_SZ;

    acb->send[ GCA_NORMAL ].bufptr = 
    acb->send[ GCA_NORMAL ].bufend =
    acb->send[ GCA_NORMAL ].buffer = acb->buffers + (2 * GCA_CL_HDR_SZ) +
				     acb->recv[ GCA_NORMAL ].bufsiz;

    acb->recv[GCA_EXPEDITED].bufptr = 
    acb->recv[GCA_EXPEDITED].bufend = 
    acb->recv[GCA_EXPEDITED].buffer = acb->buffers + (3 * GCA_CL_HDR_SZ) +
				      (2 * acb->recv[ GCA_NORMAL ].bufsiz);

    acb->send[GCA_EXPEDITED].bufptr = 
    acb->send[GCA_EXPEDITED].bufend = 
    acb->send[GCA_EXPEDITED].buffer = acb->buffers + (4 * GCA_CL_HDR_SZ) +
				      (2 * acb->recv[GCA_NORMAL].bufsiz) +
				      acb->recv[ GCA_EXPEDITED ].bufsiz;

} /* gca_acb_buffers */
