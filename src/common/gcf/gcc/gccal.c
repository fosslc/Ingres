/*
** Copyright (c) 1987, 2009 Ingres Corporation All Rights Reserved.
*/

#include    <compat.h>
#include    <gl.h>

#include    <cv.h>
#include    <er.h>
#include    <gc.h>
#include    <gcccl.h>
#include    <lo.h>
#include    <me.h>
#include    <mu.h>
#include    <pm.h>
#include    <qu.h>
#include    <si.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <lo.h>
#include    <sp.h>
#include    <mo.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <sqlstate.h>
#include    <generr.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcm.h>
#include    <gcu.h>
#include    <gcc.h>
#include    <gcaint.h>
#include    <gcaimsg.h>
#include    <gcatrace.h>
#include    <gcnint.h>
#include    <gccpci.h>
#include    <gccal.h>
#include    <gccer.h>
#include    <gccgref.h>
#include    <gcxdebug.h>
#include    <gcr.h>

/*{
**
**  Name: GCCAL.C - GCF communication server Application Layer module
**
**  Description:
**      GCCAL.C contains all routines which constitute the Application Layer 
**      of the GCF communication server. 
**
**          gcc_al - Main entry point for the Application Layer.
**	    gcc_al_event - FSM processor
**          gcc_alinit - AL initialization routine
**          gcc_alterm - AL termination routine
**	    gcc_gca_exit - first level exit routine for GCA request completions
**          gcc_al_evinp - FSM input event evaluator
**          gcc_alout_exec - FSM output event execution processor
**          gcc_alactn_exec - FSM action execution processor
**          gcc_aerr - Construct a GCA_ER_DATA object (local)
**          gcc_asnd - GCA_SEND a message
**          gcc_aaerr - Construct a GCA_ER_DATA object (remote abort)
**	    gcc_al_abort - AL internal abort routine
**
**
**  History:
**      18-Jan-88 (jbowers)
**          Initial module creation
**      27-Oct-88 (jbowers)
**          Added code to support shutdown commands through aux_data, and the
**	    skeleton code to support a local command session.
**      08-Nov-88 (jbowers)
**          Add support for user_id/password validation in gcc_alout_exec and
**	    gcc_alactn_exec.
**      10-Nov-88 (jbowers)
**	    Added logic in FSM and in  gcc_al_evinp to support comm server
**	    quiescing: reject new session requests.
**      07-Dec-88 (jbowers)
**	    Add additional error status to GCA_REJECT and GCA_RELEASE messages
**	    in gcc_alactn_exec().
**	    Add improved internal FSM error handling in gcc_alfsm().
**      14-Dec-88 (jbowers)
**	    Convert Application Entity Title (DB name) & other A_ASSOCIATE
**	    elements to & from Xfer syntax.
**      10-Jan-89 (jbowers)
**          Fixed bugs involving the coordination of the release of CCB's and
**	    MDE's.  In the CCB, the ccb_hdr.async_cnt represents a use count to
**	    allow coordinated release of the CCB when all users are finished
**	    with it.  In the MDE, a flag MDE_SAVE is set to prevent another
**	    layer from deleting it.
**      13-Jan-89 (jbowers)
**	    Added lsncnfrm callback routine on GCA_INITIATE call to support
**	    server rollover.
**      16-Jan-89 (jbowers)
**          Documentation change to FSM to document fix for an invalid
**	    intersection.
**	    Changes to gcc_alactn_exec() and gcc_asnd() to fix problem w/ not
**	    releasing a session w/ a server under certain abort conditions.
**      30-Jan-89 (jbowers)
**          Fix for bug which appends extra characters to the password when
**	    translating from network format prior to passing it as a GCA_REQUEST
**	    parameter.
**      10-Feb-89 (jbowers)
**          Expanded FSM to invoke GCA_DCONN service when aborting a connection
**	    to a server, to prevent a hung session when the server fails to
**	    recognize a GCA_ATTENTION and/or a GCA_RELEASE message.
**      16-Feb-89 (jbowers)
**          Add local flow control: expand FSM to handle XON and XOFF
**	    primitives.  Separate a GCA_RELEASE message from the P_RELEASE
**	    request generated as a conseuence.  The GCA_RELEASE flows as a
**	    normal message in a P_DATA request.
**      22-Feb-89 (jbowers)
**          Add inbound and outbound session limitations.  Modifications are in
**	    gcc_lsncnfrm, gcc_out_exec and as an additional FSM input event.
**      07-Mar-89 (jbowers)
**          Correct a documentation error in the FSM.
**      13-Mar-89 (jbowers)
**	    Fix for Bug 4985, failure to clean up aborted sessions: added input
**	    event IAPAPS to force disconnect when local session is with a
**	    server.
**      16-Mar-89 (jbowers)
**	    Fix for failure to release an MDE on receipt of P_U_ABORT and
**	    P_P_ABORT (FSM documentation only here.  Fix is in the FSM in
**	    GCCAL.H).
**      17-Mar-89 (jbowers)
**	    Fix for race condition resulting in occasional failure to clean up
**	    connection w/ server.
**      21-Mar-89 (jbowers)
**	    Fix for bug # 5101:
**	    Fix in gcc_gca_exit for processing a failed listen: insert the
**	    association id in the CCB even if the listen fails.  This fixes the
**	    accvio failure when max outbound connections is exceeded.  Added
**	    error logout in gcc_lsncnfrm and gcc_alout_exec when inbound or
**	    outbound connections are refused.
**      04-Apr-89 (jbowers)
**	    Miscellaneous stylistic cleanup and addition of error handling for
**	    obscure error conditions.
**      17-APR-89 (jbowers)
**	    Fix for a termination race condition (FSM entry (13, 0)), and for
**	    failure to release MDE's on heterogeneous connections (Documentation
**	    only: fix is in FSM tables in GCCAL.H).
**      17-APR-89 (jbowers)
**	    Fix for a termination race condition (FSM entry (11, 0)).
**	    (Documentation only: fix is in FSM tables in GCCAL.H).
**      17-APR-89 (jbowers)
**	    Modified gcc_aerr() extensively to use generic error codes, and to
**	    send the correct stuff to the FE, as LIBQ is expecting to see it.
**	3-May-89 (Colin/Jorge)
**	    added A_SHUT case to gcc_aerr(); apparently broke when
**	    the Code review stuff was incorporated. Also improved comments.
**	22-may-89 (pasker) v1.001
**	    The RAL should check both the username AND password for
**	    the null string.
**	25-May-89 (jorge)
**	    Made correct test for user_name/passwd after the the conversion
**	    from Net standard encoding in GCC_ALACTN_EXEC. Also made sure
**	    GCC_GCA_EXEC LISTEN converted terminating NULL of usre_name
**	    pasword strings, etc.
**	25-May-89 (ham)
**	    Changed gcc_aerr() to put out error message when an ERlookup
**	    couldn't find an error message.  Then cleared the status
**	    so the invalid message about AL invalid state wouldn't get
**	    put out.
**	31-May-89 (ham)
**	    Changed gcc_aerr() to save mde->pci_offset to point
**	    at the remote error messages.
**	12-Jun-89 (jorge)
**	    Added include descrip.h
**	19-jun-89 (seiwald)
**	    Added GCXDEBUGING
**	19-jun-89 (pasker)
**	    Added GCCAL GCATRACing.
**	22-jun-89 (seiwald)
**	    Straightened out gcc[apst]l.c tracing.
**	16-Jul-89 (jorge)
**	    Updated for another ule_format interface change and uncommented
** 	    the generic error code.
**      19-Jul-89 (cmorris)
**          Changed gcc_alinit to set up new parameters in GCA_INITIATE call.
**	24-Jul-89 (heather)
**	    improved the err msg. reporting so that the error detecting
**	    AL does the errlookup and sends text to the other GCC. 
**      01-Aug-89 (cmorris)
**          Changes to FSM for end to end bind. (Documentation only, fix
**          is in FSM tables in GCCAL.H).
**      02-Aug-89 (cmorris)
**          Folded logic of gcc_lsncnfrm into listen case of gcc_gca_exit.
**          In the process, fixed bug whereby comm. server couldn't pick
**          up datagram from name server if it was already servicing the
**          maximum number of outbound connections.
**	18-Aug-89 (jorge)
**	    Added initialization of GCA_INITIATE parameters
**      18-Aug-89 (cmorris)
**          Corrected calls to gcc_rel_obj that weren't casting 2nd arg.
**      24-Aug-89 (cmorris)
**          Improved GCX tracing.
**      08-Sep-89 (cmorris)
**          Changes to FSM for fixing fast selects from name server and
**          local failures after listen completion. (Documentation only,
**          fix is in FSM tables in GCCAL.H).
**      18-Sep-89 (ham)
**          Put in Byte alignment when building the PCIs in the mde.
**      26-Sep-89 (seiwald)
**          Removed AADIS, AACCB actions from IAPRI event: they are driven
**          by the completion of the AADCN action.
**      02-Oct-89 (heather/jorge)
**           Moved the 2 lines that set er_data & er_elem to the begining
**           of the routine. Used to get ACCVIO in 6.3-6.2 connections.
**      04-Oct-89 (cmorris)
**          Added missing argument to some calls to GCcshut.
**	12-Oct-89 (ham)
**	    Added byte aligcnment for the user data.  The calls to gcc_convec
**	    to pre dorm conversion should be turned into a macro.
**      12-OCT-89 (cmorris)
**          Fixed a couple of memory leaks:- The MDEs of the GOTO_HET and
**          GCA_RELEASE messages were not being freed.
**	16-Oct-89 (seiwald)
**	     Ifdef'ed GCF62 call to ule_format.  -DGCF62 to get 62 behavior.
**      31-Oct-89 (cmorris)
**           Removed define for MAX_CT_STATUS to provide consistency with
**           length of status array in mde.
**      02-Nov-89 (cmorris)
**           Removed IAPCIQ and IAPCIM input events and OAIMX output event.
**           (Documentation only:- fix is in FSM in gccal.h).
**      08-Nov-89 (cmorris)
**           State transition changes for disassociate/dconn cleanup.
**           (Documentation only:- fix is in FSM in gccal.h).
**           Other changes for the same thing.
**      17-Nov-89 (cmorris)
**           Don't log errors after gcc_get_obj calls.
**	07-Dec-89 (seiwald)
**	     GCC_MDE reorganised: mde_hdr no longer a substructure;
**	     some unused pieces removed; pci_offset, l_user_data,
**	     l_ad_buffer, and other variables replaced with p1, p2, 
**	     papp, and pend: pointers to the beginning and end of PDU 
**	     data, beginning of application data in the PDU, and end
**	     of PDU buffer.
**	09-Dec-89 (seiwald)
**	     Allow for variously sized MDE's: specify size when allocating
**	     one.  Use pointer mde->pdata as base for issuing GCA_READ
**	     calls: it maximizes use of the MDE by allowing for only
**	     the smaller data carrying PCI's.
**      11-Dec-89 (cmorris)
**           State transition change to inhibit AADIS action until send
**           of GCA_RELEASE message completes. (Documentation only:- fix
**           is in FSM in gccal.h).
**      08-Jan-90 (cmorris)
**           Spruced up real-time GCA tracing.
**	09-Jan-90 (seiwald)
**	     Remove MEfill of system_status.  It is costly and should always
**	     be set (when necessary) by CL code.
**	12-Jan-90 (seiwald)
**	     Ironed out ownership of MDE's: an MDE now belongs to exactly 
**	     one layer at a time.  
**	     
**	     Removed MDE flags (DELETE, SAVE, FREE).  That hocus-pocus is no
**	     longer necessary.  The "dooming" actions AAMDM, ASMDM, ATCKM,
**	     which checked the MDE flags word to determine whether to
**	     release the MDE, have been eliminated.  Now the state cell will
**	     contain an A*MDE action to release the MDE if and only if the
**	     MDE has not been sent to another layer.
**	     
**	     Output events and actions now are marked with a '*' in the
**	     documentation if they pass the incoming MDE to another layer, 
**	     or if they delete the MDE.  There should be exactly one such 
**	     output event or action per state cell.
**	     
**	     The various gcc_*lfsm's now pass the CCB pointer to the output
**	     and action routines, so that they need not reference through
**	     the MDE to access the CCB.  The MDE may have been deleted by a
**	     previous output event or action.
**	     
**	     The OAXOF, OAXON, OTXON, OTXOF output events now allocate their
**	     own MDE's, rather than borrowing the input one.
**	23-Jan-90 (cmorris)
**           More tracing stuff.
**	25-Jan-90 (seiwald)
**           Don't log E_GC0032_NO_PEER messages on listen failure.
**           These are bedchecks from the name server.
**      05-Feb-90 (cmorris)
**           Use new GCC_LGLVL_STRIP_MACRO macro to get rid of log level from
**           statii.
**	05-Feb-90 (seiwald)
**	     Log failure of GCA_REGISTER.
**	22-Feb-90 (seiwald)
**	    GCcexec and GCcshut removed.  IIGCC now shares GCexec and 
**	    GCshut with IIGCN.
**	01-Mar-90 (cmorris)
**	    Fixed two abort state transitions (documentation only; chnages
**	    are in FSM in gccal.h).
**	07-Mar-90 (cmorris)
**	    Restructured internal abort handling.
**	24-May-90 (seiwald)
**	    Avoid calling gcc_aaerr() unless the A_ABORT it formats is going 
**	    to be sent.  Formatting without sending wastes expensive calls 
**	    to ERlookup().
**	1-Jul-90 (seiwald)
**	    Replace GCladdnat and GCladdlong with new GCA_ADDNAT_MACRO and 
**	    GCA_ADDLONG_MACRO from gca.h.
**      3-Aug-90 (seiwald)
**	    Introduced a new state SARAB (GCA_REQUEST completion
**	    pending, P_ABORT received) to handle aborts during during
**	    SARAL (waiting for GCA_REQUEST to complete).  Since it
**	    currently is not possible to disconnect during GCA_REQUEST,
**	    we must hang onto the abort and disconnect when GCA_REQUEST
**	    completes.  (N.B. Version 6.3 handles this differently.)
**	8-Aug-90 (seiwald)
**	    Normalized handling of outbound connection counts: they are
**	    incremented upon GCA_LISTEN completion and decremented upon
**	    GCA_DISCONN completion (both in gcc_gca_exit).  Introduced
**	    a new global flag GCC_AT_OBMAX and two new input events,
**	    IAREJM (A_I_REJECT && OB_MAX exceeded) and IAAFNM (A_FINISH
**	    request && OB_MAX exceeded), to defer posting a GCA_LISTEN
**	    when OB_MAX has been exceeded.  The listen is posted when
**	    the next GCA_DISCONN completes.  This scheme allows for one
**	    active connection in excess of OB_MAX: the connection being
**	    rejected with E_GC2215_MAX_OB_CONNS.
**	4-Dec-90 (seiwald)
**	    Null terminate as_cd_ae_title after converting it from
**	    network syntax - we specifically don't convert the null.
**	    Unfortunately, we still convert the null for other fields.
**  	28-Dec-90 (cmorris)
**  	    If we are flow-controlled when we are to issue a P_U_ABORT, we
**  	    XON the lower layers in order for a receive to be posted to the
**  	    protocol driver. This receive is needed to pick up the transport
**  	    disconnect request. However, the posted receive may complete
**  	    synchronously causing _any_ previosuly flow controlled data to
**  	    filter up to the AL. This data must be discarded. Documentation
**  	    only:- fix is in GCCAL.H.
**  	03-Jan-91 (cmorris)
**  	    Fix several state transitions to purge pending GCA_SEND message
**  	    queue on receipt of aborts. Documentation only:- fix is in
**  	    GCCAL.H.
**  	03-Jan-91 (cmorris)
**  	    Fixed SARAB handling. Documentation only:- fix is in GCCAL.H.
**  	03-Jan-91 (cmorris)
**  	    Fixed passing of mdes to gcc_al_abort. Pass the right one...
**  	04-Jan-91 (cmorris)
**  	    Handle internal aborts in state SACLD.
**  	10-Jan-91 (cmorris)
**  	    Handle internal aborts in command session. Documentation
**  	    only:- fix is in GCCAL.H.
**	27-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	02-May-91 (cmorris)
**	    Chuck away received GCA messages and XON requests in
**          state SACTA; XOF requests in state SACTP. Documentation 
**	    only:- fix is in GCCALFSM.ROC
**	09-May-91 (cmorris)
**	    Bizarrely, we can get XON/XOF requests in state SACLD. This is
**	    because the underlying transport connection is not released
**	    simultaneously with the session connection et al.
**	10-May-91 (cmorris)
**	    Chuck away XOFF requests in state SACTA, XON requests in 
**	    state SACTP, and both XON/XOFF in state SACLG. Documentation
**	    only:- fix is in GCCALFSM.ROC.
**	14-May-91 (cmorris)
**	    If we're waiting for completion of send of GCA_RELEASE message,
**	    repost receives to avoid potential for hanging servers who have
**	    no normal receive posted. Documentation only:- fix is in 
**	    GCCALFSM.ROC.
**	20-May-91 (cmorris)
**	    Purge receive queue when we disassociate in state SACTA.
**	21-May-91 (cmorris)
**	    Added support for release collision handling.
**	03-Jun-91 (cmorris)
**	    Inhibit issuing GCA Disassociate if we've already called
**	    GCA Terminate: the terminate is already doing a disassociate.
**	05-Jun-91 (seiwald)
**	    AARQS was using the wrong pointer (p1 instead of p2) when
**	    stuffing the failure code E_GC220D_AL_PASSWD_FAIL, causing it
**	    to appear to the client GCC as E_US000C "You are not a valid 
**	    ingres user."
**	17-Jul-91 (seiwald)
**	    New define GCC_GCA_PROTOCOL_LEVEL, the protocol level at
**	    which the Comm Server is running.  It may be distinct from
**	    the current GCA_PROTOCOL_LEVEL.
**	31-Jul-91 (cmorris)
**	    Check status of gcc_add_ccb calls.
**  	13-Aug-91 (cmorris)
**  	    Added include of tm.h.
**  	14-Aug-91 (cmorris)
**  	    State transition fix:- a GCA send can complete after a P_Release
**  	    has been requested.
**	14-Aug-91 (seiwald)
**	    GCaddn2() takes a i4, so use 0 instead of NULL.  NULL is
**	    declared as a pointer on OS/2.
**  	15-Aug-91 (cmorris)
**  	    We can no longer receive XON/XOFF in SACLD due to lower layer fix.
**	17-aug-91 (leighb) DeskTop Porting Change:
**	    GCshut is a VOID, not a STATUS.
**  	06-Sep-91 (cmorris)
**  	    Don't free CCB on abort when we're already in SACLD; the CCB has
**  	    already been freed to get to this state.
**	8-oct-91 (seiwald) bug #40226
**	    Cheesy hack to ensure AALSN is issued even if the output
**	    actions fail.  When a GCA_LISTEN completes, the output actions 
**	    of IAAAQ in SACLD can fail if the TL doesn't recognise the 
**	    protocol name; we have to make sure the GCA_LISTEN is reissued.
**	18-oct-91 (seiwald) bug #40468
**	    Don't return failure after reintroducing an event unless
**	    an abort is generated.  This allows important subsequent 
**	    actions (like repostings listens and receives) to take place
**	    when certain actions fail.
**  	27-Dec-91 (cmorris)
**  	    Added tracing for GCA message type, length and flags.
**	2-Sep-92 (seiwald)
**	    Replaced calls to ule_format with calls to new gcu_erfmt().
**  	06-Oct-92 (cmorris)
**  	    Got rid of states, events and actions associated with command
**          session.
**  	07-Oct-92 (cmorris)
**  	    Dynamically allocate parameter lists for GCA_LISTENs to allow
**  	    for multiple listens to be in progress.
**  	09-Oct-92 (cmorris)
**  	    Fix unlikely but possible memory leaks. If a GCA Request, Listen,
**  	    Response or Disassociate fails synchronously, then the associated
**  	    parameter lists will never get freed. Additionally, in the
**  	    disassociate case, _no_ receive/send parameter lists will get freed.
**  	    Fix is to make sure all parameter lists are freed when the CCB is
**  	    freed.
**  	27-Nov-92 (cmorris)
**  	    No longer execute AALSN from the state machine of another
**  	    connection. This tidies up all the problems associated with
**  	    prior actions/events failing and also allows for support for
**  	    reposting listens when a GCA_LISTEN is resumed. Also  changed
**  	    IAINT to IALSN, and A_NOOP to A_LISTEN for readability.
**  	02-Dec-92 (cmorris)
**  	    Added GCM support.
**  	08-Dec-92 (cmorris)
**  	    Added GCM fast select support.
**  	04-Jan-93 (cmorris)
**  	    Store Remote Network address in CCB rather than MDE. This
**  	    stops having to copy it between all the layers and also
**  	    allows it to be kept for possible management use.
**  	07-Jan-93 (cmorris)
**  	    Store called AE Title in CCB rather than MDE.
**  	08-Jan-93 (cmorris)
**  	    Got rid of call to GCA_FORMAT:- mate it's useless!!
**  	19-Jan-93 (cmorris)
**  	    Abort association within gcc_asnd() if error occurs.
**  	01-Feb-93 (brucek)
**  	    Changed status handed back on shutdown to E_GC0040_CS_OK;
**	10-Feb-93 (seiwald)
**	    Removed generic_status and system_status from gcc_xl(),
**	    gcc_xlfsm(), and gcc_xlxxx_exec() calls.
**  	10-Feb-93 (cmorris)
**  	    Removed status argument from GCngets, GCnadds calls.
**	11-Feb-93 (seiwald)
**	    Merge the gcc_xlfsm()'s into the gcc_xl()'s.
**	11-Feb-93 (seiwald)
**	    Use QU routines rather than the roll-your-own gca_qxxx ones.
**	16-Feb-93 (seiwald)
**	    GCA service parameters have been moved from hanging off the
**	    ccb to overlaying the protocol driver's p_plist in the MDE.
**	17-Feb-93 (seiwald)
**	    The async_id closure parameter given to GCA is now simply the
**	    MDE casted to a i4, rather than a wild bit-shuffle of
**	    lcl_cei and gca_rq_type.  Gca_rq_type is now stored for all
**	    operations in the MDE.  The outstanding GCA requests are no
**	    longer tracked in the CCB.
**	17-Feb-93 (seiwald)
**	    gcc_find_ccb() is no more.
**	17-Feb-93 (seiwald)
**	    At GCA_API_LEVEL_4, we no longer need to call GCA_INTERPRET.
**	25-Feb-93 (seiwald)
**	    Marked each output event or action that generates an MDE with 
**	    a '='.
**	22-Mar-93 (seiwald)
**	    Fix to change of 11-feb-93: call QUinsert with a pointer to the
**	    last element on the queue, so that the MDE is inserted at the end.
**	24-mar-93 (smc)
**	    Removed forward declaration of MEreqmem() now it is declared
**	    (and prototyped) in me.h.
**	5-apr-93 (robf)
**	    Add handling for security label on connections
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	08-sep-93 (johnst/swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	    Changed gcc_gca_exit() argument async_id type from i4 to PTR.
**	22-sep-93 (johnst)
**	    Bug #56444
**	    Changed two tmp variables from type long to i4. They are used 
**	    for GCA message manipulation and GCA "longs" are actually 
**	    i4-sized. On DEC alpha native long's are 64-bit and i4s
**	    are 32-bit.
**	5-Oct-93 (seiwald)
**	    Defunct gca_cb_decompose, gca_decompose, gca_lsncnfrm now gone.
**	16-Nov-93 (brucek)
**	    Restore GCA_INTERPRET for received fastselect messages.
**	24-Feb-94 (brucek)
**	    In FASTSELECT processing, put up expedited receive on completion
**	    of GCA_RQRESP, rather than of GCA_SEND, to prevent putting up
**	    multiple receives.  (Documentation only -- fix is in gccalfsm.roc)
**	14-Nov-95 (gordy)
**	    Added support for GCA_API_LEVEL_5 which removes the
**	    need for GCA_INTERPRET on fast select messages.
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	21-Nov-95 (gordy)
**	    Added gcr.h for embedded Comm Server configuration.  Don't
**	    do any GCA operations during init/term if embedded.
**	22-Nov-95 (gordy)
**	    Set the gca_formatted flag for send/receive at API level 5.
**	 4-Dec-95 (gordy)
**	    Remove GCN function definitions and include gcnint.h instead.
**	04-apr-96 (emmag)
**	    Pass the length of tmp_sec_label to GCnsadd to avoid reading
**	    past the end of the data segment, in the case where no
**	    security label has been specified.
**	24-apr-96 (chech02, lawst01)
**	    cast %d arguments to i4  in TRdisplay for windows 3.1 port.
**	19-Jun-96 (gordy)
**	    Recursive requests are now queued and processed by the first
**	    instance of gcc_al().  This ensures that all outputs of an 
**	    input event are executed prior to processing subsequent requests.
**	23-Aug-96 (gordy)
**	    Fast Select now sends a GCA_RELEASE message to end exchange.
**	    Added GCA normal channel receive to Fast Select action list.
**	    (IAACEF, SARAA).  Added handling of A_RELEASE during fast
**	    select (IAARQ, SAFSL).
**	21-Feb-97 (gordy)
**	    Changed gca_formatted flag to gca_modifiers.
**	25-Feb-97 (gordy)
**	    Set end-of-group on GCA_SEND so indicator flows between
**	    client and server.
**	11-Mar-97 (gordy)
**	    Added gcu.h for utility function declarations.
**	23-May-97 (thoda04)
**	    Included pm.h header for function prototypes.
**	29-May-97 (gordy)
**	    GCA aux data structure now only defines the aux data header.
**	    GCA aux data area must be handled separately.  GCA_RQ_SERVER 
**	    deprecated with new GCF security handling.
**	02-jun-1997 (canor01)
**	    Do not log the expedited receive failure in gcc_gca_exit().
**	 3-Jun-97 (gordy)
**	    Building of initial connection info moved to SL.  It, along
**	    with the AL and PL pci were being packaged as SL user data
**	    anyway.  This will allow the SL to conditionally package the
**	    info based on negotiated protocol level from the T_CONNECT
**	    request.
**      18-jun-1997 (canor01)
**          Use SystemVarPrefix to compose the server name used in the
**          errlog.log.
**	 5-Sep-97 (gordy)
**	    Added new AL protocol level in which the initial connection
**	    info is now processed by the SL.
**	21-Oct-97 (rajus01)
**	    Replaced gcc_rel_obj(), gcc_get_obj() with gcc_del_mde(), 
**	    gcc_add_mde() respectively.
**	29-Dec-97 (gordy)
**	    Save FASTSELECT messages in dynamically allocated buffer
**	    (GCA may step on its buffer before we are ready).
**	27-Feb-98 (loera01) Bug 89246
**	    For gcc_gca_exit(), load security labels with a length of
**	    sizeof(SL_LABEL), rather than GCC_L_SECLABEL.  For most platforms,
**	    the internal security label is much smaller than GCC_L_SECLABEL,
**	    and can cause an access violation in gcnadds().
**	31-Mar-98 (gordy)
**	    Added default encryption mechanism.
**	25-Aug-98 (gordy)
**	    Fix building of GCA_ER_DATA object.
**	 8-Oct-98 (gordy)
**	    Allow anyone to do GCM read operations to support
**	    installation registry.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	13-Jul-00 (gordy)
**	    Logging of association aborts was removed because of timing
**	    problems (expected aborts during disconnect were being logged).
**	    The problem was that logging was being done in gcc_gca_exit()
**	    before the correct state was established in the AL FSM.  Move
**	    logging to OAPAU for A_ABORT and A_P_ABORT.  OAPAU is not
**	    executed during shutdown so only real aborts will be logged.
**	28-jul-2000 (somsa01)
**	    In gcc_alinit(), write out the GCC server id to standard output.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-oct-2000 (somsa01)
**	    In gcc_al_abort(), we should be setting the contents of
**	    generic_status to OK, not the pointer itself.
**	16-mar-2001 (somsa01)
**	    In gcc_gca_exit() and gcc_alactn_exec(), in the case of
**	    E_GC000B_RMT_LOGIN_FAIL, log E_GC000E_RMT_LOGIN_FAIL_INFO as
**	    well.
**      25-Jun-2001 (wansh01) 
**           replace gcxlevel with IIGCc_global->gcc_trace_level
**	22-Mar-02 (gordy)
**	    Additional nat -> i4 changes.
**	20-Aug-02 (gordy)
**	    Check for acceptance/rejection of P_CONNECT request by PL
**	    in AARQS.
**	10-Dec-02 (gordy)
**	    GCA_ERROR message format is protocol level depedent (gcc_aerr()).
**	 6-Jun-03 (gordy)
**	    Remote target address info now placed in trg_addr, local/remote
**	    actual addresses placed in lcl_addr and rmt_addr.  Attach CCB
**	    MIB object after successful listen.
**	21-Jan-04 (gordy)
**	    Registry control can occur during GCM operations which
**	    cause GCA_LISTEN to fail with E_GC0032_NO_PEER.  Check
**	    for registry startup flag after such a failure and
**	    initialize the registry.
**  28-Jan-2004 (fanra01)
**      Moved gcc_init_registry from iigcc.c for inclusion in
**      the gcf library.
**	15-Jul-04 (gordy)
**	    Enhanced encryption of passwords between gcn and gcc.
**      25-Sept-2004 (wansh01) 
**           Added GCADM/GCC interface. 
**  18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**      Use gcx_getGCAMsgMap() to retrieve gcx_gca_msg, otherwise Windows
**      cannot resolve the global reference.
**       4-Nov-05 (gordy)
**          Originally, FASTSELECT processing was handled by a single
**          data transfer state (SAFSL).  This didn't allow for message 
**	    queuing (SADSP) and send collisions could occur when the 
**	    FASTSELECT data was just large enough to overflow an internal 
**	    buffer.  This change switches the FASTSELECT state to listen 
**	    completion (instead of SAIAL) and adds a companion state 
**	    (SARAF instead of SARAA) for the response.  A new input
**	    (IAAAF) drives the transition on the listen completion, but 
**	    then standard inputs (IAPCA, IAACE, IAANQ, etc) drive the 
**	    proper actions (AAFSL, AARVN) from the new states.  The 
**	    normal data transfer states (SADTI, SADSP) are now used, 
**	    after the initial FASTSELECT data is used to 'prime the pump'
**	    (AAFSL).  The original special input (IAACEF) which drove 
**	    the transition to the FASTSELECT state after the listen 
**	    response is no longer used.
**	31-Mar-06 (gordy)
**	    Sanity check lengths read from IO buffers.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
*/


#define	GCC_GCN_PREFIX		"IINMSVR-"

static PTR	gca_cb;
static char	*No = "N";

/*
**  Forward and/or External function references.
*/

static STATUS	gcc_al_event( GCC_MDE *mde );
static u_i4	gcc_al_evinp( GCC_MDE *mde, STATUS *generic_status );
static STATUS	gcc_alout_exec( i4  output_id, GCC_MDE *mde, GCC_CCB *ccb );
static STATUS	gcc_alactn_exec(u_i4 action_code, GCC_MDE *mde, GCC_CCB *ccb);
static STATUS	gcc_aerr( GCC_MDE *mde, GCC_CCB *ccb );
static STATUS	gcc_asnd( GCC_MDE *mde);
static VOID	gcc_aaerr( GCC_MDE *mde, GCC_CCB *ccb, STATUS generic_status );
static VOID	gcc_al_abort( GCC_MDE *mde, GCC_CCB *ccb, STATUS *status, 
			      CL_ERR_DESC *system_status, GCC_ERLIST *erlist );
static void	gcc_init_mask( char *key, i4 len, u_i1 *mask );


/*{
** Name: gcc_al	- Entry point for the Application Layer
**
** Description:
**	gcc_al is the entry point for the application layer.  It invokes
**	the services of subsidiary routines to perform its requisite functions.
**	It is invoked with a Message Data Element (MDE) as its input.
**	The MDE may come from GCA and is outbound for the network, or
**	from PL and is inbound for a local client or server, sent from an
**	external node. The discussion that follows is
**	relevant to all layers.  It is done in verbose form here to document
**	the structure and operation.  The modules for the other layers are more
**	tersely described.  Reference should be made to the following tutorial
**	when reading the documentation for other layers.
**
**	AL operation may be divided into three phases: initialization,
**	association management and shutdown.  Initialization is performed once
**	at comm server startup time.  The static global table is built, a GCA
**	listen is established, and some miscellaneous other housekeeping is
**	done, in preparation for association management, the real meat of comm
**	server AL operation.
**
**	Shutdown phase is entered as a result of an external command, from the
**	comm server management utility.  Shutdown may be specified as immediate
**	or graceful.  Immediate shutdown simply terminates comm server
**	operation by invoking GCshut(), and all existing associations terminate
**	with error conditions.  It is a commanded comm server crash.  No
**	attempt is made to bring things down in an orderly manner.
**
**	If graceful shutdown is commanded, the comm server enters a quiescing
**	state.  No new associations will be initiated.  AL does not issue a new
**	listen, and if a P_CONNECT indication bubbles up from below, a P_CONNECT
**	response/reject is returned to refuse the connection.  Comm server
**	operation continues until the last existing session is complete, at
**	which time GCshut() is invoked to terminate execution.
**
**      The main phase of comm server AL operation is association management. In
**	this phase remote associations are established and managed. An association
**	may come from a local or remote source.  For a given association, there
**	are three phases of AL association management: 
** 
**      - Association initiation; 
**      - Data transfer;
**      - Association termination. 
**
**	Each of these is discussed in detail below.  Then the operation of the
**	AL is defined by a Finite State Machine (FSM) specification.  This is
**	the definitive description of AL behavior.  A discussion of this
**	representation is given here.
**
**	The FSM has four components: states, input events, output events and
**	actions.  The machine moves from state to state depending on its
**	current state and the input event.  This is known as state transition.
**	In the process of making a state transition, both an output event and
**	an action may optionally occur.  These are optional in the sense that
**	if they are defined for the particular state transition, they
**	invariably take place; however, for a particular state transtion, there
**	may not be an output event or an action defined.
**
**	The FSM is represented as a matrix with columns defining states and
**	rows defining input events.  The FSM starts in the initial state,
**	always represented as state 0.  Each matrix element defines, for a
**	particular combination of current state and input event the transition,
**	output event(s) and action(s) which occur.  Note that a state
**	characterizes an individual association, and that input events, output
**	events and actions which occur are associated with particular
**	associations.
**
**	One or more output event accompanies a state transition.  One or more
**	actions may be taken.  An output event is always a Service Data Unit
**	(SDU) sent to the Presentation Layer.  An action is a GCA service 
**      request or some management function such as freeing an MDE. 
**      An input event is embodied in an MDE that is passed
**	as an input parameter by the invoker, and is the result of the arrival
**	of an SDU from PL or the generation of one by the appropriate GCA
**	asynchronous service completion exit as a result of the completion of a
**	GCA service request such as GCA_LISTEN or GCA_RECEIVE.
**
**	The actual implementation of the FSM in code is a 2-dimensional array
**	whose elements are indexes into a linear array of stuctures specifying
**	the state transition, output event(s) and and action(s).  The header
**	file GCCAL.H contains these components, and a more detailed explanation
**	of the implementation and the execution time methodology.
**
**	States, input events, output events and actions are symbolized
**	respectively as SAxxx, IAxxx, OAxxx and AAxxx.
**
**	Output events and actions are marked with a '*' if they pass 
**	the incoming MDE to another layer, or if they delete the MDE.
**	There should be exactly one such output event or action per
**	state cell.  Output events and actions are marked with a '=' if
**	they allocate a new MDE and (necessarily) pass it to another layer.
**
**	In the FSM definition matrix which follows, the parenthesized integers
**	are the designators for the valid entries in the matrix, and are used in
**	the representation of the FSM in GCCAL.H as the entries in ALFSM_TBL
**	which are the indices into the entry array ALFSM_ENTRY.  See GCCAL.H
**	for further details.

**      Association initiation is indicated in one of two ways:
**
**	by receipt of an A_ASSOCIATE MDE constructed and sent by gcc_gca_exit
**	when a GCA_LISTEN has completed;
**
**	or by receipt of a P_CONNECT indication sent by PL as a result of a
**	GCA_REQUEST by a remote client.
**
**	In the first case, we are the Initiating Application Layer (IAL),
**	driven by a client.  A P_CONNECT request is constructed and sent to PL
**	using the parameters returned from GCA_LISTEN and packaged by
**	gcc_gca_exit in the A_ASSOCIATE request.  A P_CONNECT confirm is
**	awaited before proceeding.  When received, if it is ACCEPT, a 
**      GCA_RQRESP is issued and upon its completion the data transfer phase
**      is entered. If it is REJECT, a GCA_RQRESP is issued and upon its 
**      completion, GCA_DISASSOC is invoked to terminate the GCA association.
**
**	In the second case, we are the Responding Application Layer (RAL), in a
**	remote server node.  A GCA_REQUEST service request is invoked to
**	establish contact with the local server.  If the GCA_REQUEST completes
**	without error, normal and expedited GCA_RECEIVE's are issued, a
**	P_CONNECT response with a result of ACCEPT is generated and sent to PL
**	to be returned to the IAL.  If the GCA_REQUEST fails, a P_CONNECT
**	response with a result of REJECT is generated and sent to PL.  In the
**	former case, the Data Transfer phase is entered.  In the latter case,
**	the association is abandoned.
**
**      The data transfer phase is initiated by both IAL and RAL by invoking 
**      the GCA_RECEIVE service on the newly established association.  Each 
**      received GCA message is packaged into a P_DATA or P_EXPEDITED_DATA 
**      request and sent to PL.  Upon receipt of a P_DATA or 
**	P_EXPEDITED_DATA indication, a GCA_SEND is invoked to pass the
**      message to the recipient.  During this phase, there is no distinction 
**      between IAL and RAL, and hence between the client end and the server 
**      end of the association.  
** 
**	The receipt of an A_ABORT request indicates the failure of a GCA data 
**	transfer operation (GCA_SEND or GCA_RECEIVE), and hence the failure 
**	of the local association, was detected by the appropriate async 
**	completion exit.  A P_U_ABORT request is sent to PL, a GCA_DISASSOC
**	is issued and the the association is terminated. 
**	
**	The receipt of a GCA_RELEASE message indicates the sender (client or
**	server) is terminating the association.  A P_RELEASE request is sent to
**	PL following the GCA_RELEASE message.  GCA_DISASSOC is issued to clean
**	up the local association, and the association is terminated.  On receipt
**	of a P_RELEASE indication, GCA_DISASSOC is invoked to terminate the
**	local association.
**
**      The Association Termination phase is entered as a result of several 
**	possible sequences of events.  In the AI phase, an abort indication from
**	either the local user or from PL, or a rejection of the association 
**	request by the target server invokes the AT phase.  In the DT phase, 
**	an abort condition, a P_RELEASE indication or an A_RELEASE request
**	causes transition to the AT phase.  In this phase, 
**	completion of any outstanding failure notification is awaited, then the
**	local association is terminated by issuing GCA_DISASSOC.
**
**      The IAL also supports a minimal form of "fast select" protocol for the
**  	support of GCM fast selects. The principal of "fast select" is that the
**  	data to be sent on an association is part of the the establishment phase,
**  	and the data to be received in part of the termination phase. In other 
**  	words, there is no data transfer phase:- a single piece of data is sent
**  	as part of association establishment, and a single piece of data is 
**  	received as part of termination. Because of the limitations inherent in 
**  	the heterogeneous data conversion of the Presentation Layer (data is
**  	_only_ converted on a P_DATA or P_EXPEDITED_DATA request), a "true" 
**  	fast select cannot easily be implemented across the network. Hence it 
**  	is only implemented in the IAL and to the RAL the "fast select" looks
**  	like an ordinary association. 
**
**  	The association establishment phase is identical to that for a normal
**  	association, except that the data associated with the request is stored
**  	until the association has been established. When the GCA_RQRESP 
**  	completes, the fast select phase is entered and the stored data is
**      used to mimic completion of a GCA_RECEIVE. This is treated in the 
**  	same manner as for a normal association, except that no GCA_RECEIVE 
**      service is invoked. When a P_DATA indication is received, a GCA_SEND
**  	is invoked to pass on the returned data to the recipient. Once again, 
**  	the data coming back from the RAL cannot be part of the release of the
**  	application association because of the heterogeneous conversion
**  	limitations of the Presentation Layer. Once the GCA_SEND of the
**  	received data completes, the association termination phase is
**  	entered to release the association.

**	FOR the Application Layer, the following states, output events and
**	actions are defined, with the state characterizing an individual
**	association:
**
**	States:
**
**	    SACLD   - Idle, association does not exist.
**	    SAIAL   - IAL: P_CONNECT confirm pending
**	    SARAL   - RAL: GCA_REQUEST completion to target server pending
**	    SARAB   - RAL: GCA_REQUEST completion pending, abort received
**          SARAA   - IAL: GCA_RQRESP (accept) pending
**          SARJA   - IAL: GCA_RQRESP (reject) pending
**	    SADTI   - Data Transfer Idle
**	    SADSP   - Send pending
**	    SACTA   - A_COMPLETE request pending
**	    SACTP   - P_RELEASE confirm pending
**	    SASHT   - Server shutdown in process
**	    SADTX   - Idle, XOFF received, no rcv outstanding
**	    SADSX   - Send pending, XOFF received, no rcv outstanding
**          SAQSC   - Server quiesce in process
**          SACLG   - Association is closing
**	    SACAR   - (Release) Collision, association responder
**  	    SAFSL   - IAL: P_CONNECT confirm for FASTSELECT pending
**	    SARAF   - IAL: GCA_RQRESP (accept) of FASTSELECT pending
**	    SACMD   - Admin session
**
**	Input Events:
**
**	    IALSN   -  A_LISTEN request (place GCA_LISTEN)
**	    IAAAQ   -  A_ASSOCIATE request (GCA_LISTEN has completed)
**	    IAABQ   -  A_ABORT request (Local association has failed)
**	    IAASA   -  A_ASSOCIATE response/accept (RAL's GCA_REQUEST is OK)
**	    IAASJ   -  A_ASSOCIATE response/reject (RAL's GCA_REQUEST failed)
**	    IAPCI   -  P_CONNECT indication (Received by RAL from IAL)
**	    IAPCA   -  P_CONNECT confirm/accept (Received by IAL from RAL)
**	    IAPCJ   -  P_CONNECT confirm/reject (Received by IAL from RAL)
**	    IAPAU   -  P_U_ABORT indication (Sent by remote peer AL)
**	    IAPAP   -  P_P_ABORT indication (From local PL: local conn. failure)
**	    IAANQ   -  A_DATA request (GCA_RECEIVE (normal) has completed)
**	    IAAEQ   -  A_EXP_DATA request (GCA_RECEIVE (exp) has completed)
**	    IAACE   -  A_COMPLETE request && MSG_Q_Empty (GCA_SEND or 
**                     GCA_RQRESP has completed)                   
**	    IAACQ   -  A_COMPLETE request && MSG_Q_Not_Empty (GCA_SEND or
**                     GCA_RQRESP has completed)
**	    IAPDN   -  P_DATA indication
**	    IAPDE   -  P_EXPEDITED_DATA indication
**	    IAARQ   -  A_RELEASE request (GCA_RELEASE message received)
**	    IAPRII  -  P_RELEASE indication (initiator)
**	    IAPRA   -  P_RELEASE confirm/accept
**	    IAPRJ   -  P_RELEASE confirm/reject
**	    IALSF   -  A_LSNCLNUP request (local LISTEN failure)
**	    IAREJI  -  A_I_REJECT request (IAL local reject)
**          IAREJR  -  A_R_REJECT request (RAL local reject)
**	    IAAPG   -  A_PURGED request
**	    IAPDNH  -  P_DATA indication (GOTOHET to be sent first)
**	    IASHT   -  A_SHUTDOWN request (terminate execution NOW)
**	    IAQSC   -  A_QUIESCE request (no new sessions)
**	    IAABT   -  A_P_ABORT request (AL failure)
**	    IAXOF   -  P_XOFF indication (From PL: shut down data flow)
**	    IAXON   -  P_XON indication (From PL: resume data flow)
**	    IAANQX  -  A_DATA request && XOFF rcvd
**	    IAPDNQ  -  P_DATA ind && rcv queue is full && NOT XOF sent
**	    IAACEX  -  A_COMPLETE && Q is empty && XOF sent
**	    IAAPGX  -  A_PURGED request && XOFF rcvd
**	    IAABQX  -  A_ABORT request && XOF sent
**	    IAABQR  -  A_ABORT request && GCA_RELEASE message received
**          IAAFN   -  A_FINISH request (GCA_DISASSOC has completed)
**	    IAPRIR  -  P_RELEASE indication (responder)
**	    IAAAF   -  A_ASSOCIATE FASTSELECT request (GCA_LISTEN has completed)
**  	    IAPAM   -  P_(U P)_ABORT indication on a management association
**	    IACMD   -  A_CMDSESS request (new admin session)
**

**	Output Events:
**
**	    OAPCQ*  -  P_CONNECT request
**	    OAPCA*  -  P_CONNECT response/accept
**	    OAPCJ*  -  P_CONNECT response/reject
**	    OAPAU*  -  P_U_ABORT request
**	    OAPDQ*  -  P_DATA request 
**	    OAPEQ*  -  P_EXPEDITED_DATA request
**	    OAPRQ=  -  P_RELEASE request
**	    OAPRS*  -  P_RELEASE response/accept
**	    OAXOF=  -  P_XOFF request to PL: shut down data flow
**	    OAXON=  -  P_XON request to PL: resume data flow
**
**	Actions:
**
**	    AARQS*  -  GCA_REQUEST
**	    AARVN=  -  GCA_RECEIVE (normal)
**	    AARVE=  -  GCA_RECEIVE (expedited)
**	    AADIS=  -  GCA_DISASSOC
**	    AALSN   -  GCA_LISTEN
**	    AASND*  -  GCA_SEND 
**	    AARLS   -  Construct GCA_RELEASE message
**	    AAENQ*  -  Enqueue message for GCA_SEND
**	    AADEQ   -  Dequeue an MDE and GCA_SEND the message 
**	    AAPGQ   -  Purge message queues
**	    AAMDE*  -  Discard the input MDE
**	    AACCB   -  Doom or delete the CCB
**	    AASHT   -  Signal immediate Comm Server shutdown
**	    AAQSC   -  Set "quiesce and shutdown" indicator
**	    AASXF   -  Set the "XOFF-received" indicator in the CCB
**	    AARXF   -  Reset the "XOFF-received" indicator in the CCB
**          AARSP*  -  GCA_RQRESP
**          AASNDG=      Send go to het message
**          AAENQG=   -  Enqueue go to het message
**	    AACMD   -  Init admin session
**

**  Server Control State Table
**  --------------------------
**
**  State ||SACLD|SASHT|SAQSC|SACMD|SACLG|
**	  ||  00 |  07 |  12 |  18 |  13 |
**  ======================================
**  Event ||
**  ======================================
**  IALSN ||SACLD|     |     |     |     |
**  (00)  ||     |     |     |     |     |
**	  ||AALSN|     |     |     |     |
**	  || (01)|     |     |     |     |
**  ------++-----+-----+-----+-----+-----+
**  IASHT ||SASHT|     |     |     |     |
**  (24)  ||     |     |     |     |     |
**        ||AARSP*     |     |     |     |
**        || (26)|     |     |     |     |
**  ------++-----+-----+-----+-----+-----+
**  IAQSC ||SAQSC|     |     |     |     |
**  (25)  ||     |     |     |     |     |
**        ||AARSP*     |     |     |     |
**        || (27)|     |     |     |     |
**  ------++-----+-----+-----+-----+-----+
**  IACMD ||SACMD|     |     |     |     |
**  (40)  ||     |     |     |     |     |
**        ||AARSP*     |     |     |     |
**        || (81)|     |     |     |     |
**  ------++-----+-----+-----+-----+-----+
**  IAACE ||     |SACLG|SACLG|SACMD|SACLG|
**  (12)  ||     |     |     |     |     |
**  	  ||     |AASHT|AAQSC|AACMD|AAMDE*
**  	  ||     |AADIS=AADIS=     |     |
**        ||     |AAMDE*AAMDE*     |     |
**        ||     | (36)| (09)| (82)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAARQ ||     |SACLG|SACLG|     |SACLG|
**  (16)  ||     |     |     |     |     |
**        ||     |AASHT|AAQSC|     |AAMDE*
**        ||     |AADIS=AADIS=     |     |
**        ||     |AAMDE*AAMDE*     |     |
**        ||     | (36)| (09)|     | (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAAFN ||     |     |     |SACLD|SACLD|
**  (35)  ||     |     |     |     |     |
**        ||     |     |     |AACCB|AACCB|
**        ||     |     |     |AAMDE*AAMDE*
**        ||     |     |     | (44)| (44)|
**  ------++-----+-----+-----+-----+-----+
**  IAABQ ||     |SACLG|SACLG|SACLG|SACLG|
**  (02)  ||     |     |     |     |     |
**        ||     |AASHT|AAQSC|AADIS=AAMDE*
**	  ||     |AADIS=AADIS=AAMDE*     |
**	  ||     |AAMDE*AAMDE*     |     |
**	  ||     | (36)| (09)| (04)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAABT ||SACLD|SACLG|SACLG|SACLG|SACLD|
**  (27)  ||     |     |     |     |     |
**        ||     |AADIS=AADIS=AADIS=AACCB|
**        ||AAMDE*AAMDE*AAMDE*AAMDE*AAMDE*
**        || (48)| (04)| (04)| (04)| (44)|
**  ------++-----+-----+-----+-----+-----+

**  Session Initiation State Table
**  ------------------------------
**
**  State ||SACLD|SAIAL|SARAL|SARAA|SAFSL|SARAF|SACLG|
**	  || (00)| (01)| (02)| (11)| (16)| (17)| (13)|
**  ==================================================
**  Event ||
**  ==================================================
**  IAAAQ ||SAIAL|     |     |     |     |     |     |
**  (01)  ||     |     |     |     |     |     |     |
**        ||OAPCQ*     |     |     |     |     |     |
**	  || (02)|     |     |     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAPCI ||SARAL|     |     |     |     |     |     |
**  (05)  ||     |     |     |     |     |     |     |
**	  ||AARQS*     |     |     |     |     |     |
**	  || (07)|     |     |     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IALSF ||SACLG|     |     |     |     |     |     |
**  (20)  ||     |     |     |     |     |     |     |
**	  ||AADIS=     |     |     |     |     |     |
**        ||AAMDE*     |     |     |     |     |     |
**	  || (10)|     |     |     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAREJI||SARJA|     |     |     |     |     |     |
**  (21)  ||     |     |     |     |     |     |     |
**	  ||AARSP*     |     |     |     |     |     |
**	  || (11)|     |     |     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAAAF ||SAFSL|     |     |     |     |     |     |
**  (38)  ||     |     |     |     |     |     |     |
**        ||OAPCQ*     |     |     |     |     |     |
**	  || (40)|     |     |     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAASA ||     |     |SADTI|     |     |     |     |
**  (03)  ||     |     |     |     |     |     |     |
**        ||     |     |OAPCA*     |     |     |     |
**	  ||     |     |AARVN=     |     |     |     |
**	  ||     |     |AARVE=     |     |     |     |
**	  ||     |     | (05)|     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAASJ ||     |     |SACLG|     |     |     |     |
**  (04)  ||     |     |     |     |     |     |     |
**        ||     |     |OAPCJ*     |     |     |     |
**	  ||     |     |AADIS=     |     |     |     |
**	  ||     |     |     |     |     |     |     |
**	  ||     |     | (06)|     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAPCA ||     |SARAA|     |     |SARAF|     |     |
**  (06)  ||     |     |     |     |     |     |     |
**	  ||     |AARSP*     |     |AARSP*     |     |
**	  ||     | (93)|     |     | (41)|     |     |
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAPCJ ||     |SARJA|     |     |SARJA|     |     |
**  (07)  ||     |     |     |     |     |     |     |
**	  ||     |AARSP*     |     |AARSP*     |     |
**	  ||     | (94)|     |     | (94)|     |     |
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAREJR||     |     |SACLD|     |     |     |SACLD|
**  (36)  ||     |     |     |     |     |     |     |
**        ||     |     |OAPCJ*     |     |     |AACCB|
**        ||     |     |AACCB|     |     |     |AAMDE*
**        ||     |     | (47)|     |     |     | (44)|
**  ------++-----+-----+-----+-----+-----+-----+-----+

**  State ||SACLD|SAIAL|SARAL|SARAA|SAFSL|SARAF|SACLG|
**	  || (00)| (01)| (02)| (11)| (16)| (17)| (13)|
**  ==================================================
**  Event ||
**  ==================================================
**  IAACE ||     |     |     |SADTI|     |SADTI|SACLG|
**  (12)  ||     |     |     |     |     |     |     |
**	  ||     |     |     |AARVN=     |AAFSL=AAMDE*
**	  ||     |     |     |AARVE=     |AARVE=     |
**	  ||     |     |     |AAMDE*     |AAMDE*     |
**	  ||     |     |     | (88)|     | (12)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAACQ ||     |     |     |SADSP|     |SADSP|     |
**  (13)  ||     |     |     |     |     |     |     |
**	  ||     |     |     |AARVN=     |AAFSL=     |
**	  ||     |     |     |AARVE=     |AARVE=     |
**	  ||     |     |     |AADEQ|     |AADEQ|     |
**        ||     |     |     |AAMDE*     |AAMDE*     |
**	  ||     |     |     | (89)|     | (43)|     |
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAPDN ||     |     |     |SARAA|     |SARAF|SACLG|
**  (14)  ||     |     |     |     |     |     |     |
**	  ||     |     |     |AAENQ*     |AAENQ*AAMDE*
**	  ||     |     |     | (90)|     | (85)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAPDE ||     |     |     |SARAA|     |SARAF|SACLG|
**  (15)  ||     |     |     |     |     |     |     |
**	  ||     |     |     |AAENQ*     |AAENQ*AAMDE*
**	  ||     |     |     | (90)|     | (85)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAPRII||     |     |     |SACTA|     |SACTA|SACLG|
**  (17)  ||     |     |     |     |     |     |     |
**        ||     |     |     |OAPRS*     |OAPRS*AAMDE*
**	  ||     |     |     | (31)|     | (31)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAPDNH||     |     |     |SARAA|     |SARAF|SACLG|
**  (23)  ||     |     |     |     |     |     |     |
**	  ||     |     |     |AAENQ*     |AAENQ*     |
**	  ||     |     |     |AAENQG     |AAENQGAAMDE*
**	  ||     |     |     | (91)|     | (86)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAPDNQ||     |     |     |SARAA|     |SARAF|SACLG|
**  (30)  ||     |     |     |     |     |     |     |
**        ||     |     |     |OAXOF=     |OAXOF=     |
**	  ||     |     |     |AAENQ*     |AAENQ*AAMDE*
**	  ||     |     |     | (92)|     | (87)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAAFN ||     |     |     |     |     |     |SACLD|
**  (35)  ||     |     |     |     |     |     |     |
**        ||     |     |     |     |     |     |AACCB|
**        ||     |     |     |     |     |     |AAMDE*
**        ||     |     |     |     |     |     | (44)|
**  ------++-----+-----+-----+-----+-----+-----+-----+

**  State ||SACLD|SAIAL|SARAL|SARAA|SAFSL|SARAF|SACLG|
**	  || (00)| (01)| (02)| (11)| (16)| (17)| (13)|
**  ==================================================
**  Event ||
**  ==================================================
**  IAABQ ||     |SACLG|     |SACLG|SACLG|SACLG|SACLG|
**  (02)  ||     |     |     |     |     |     |     |
**        ||     |OAPAU*     |OAPAU*OAPAU*OAPAU*     |
**	  ||     |AADIS=     |AAPGQ|AADIS=AAPGQ|AAMDE*
**	  ||     |     |     |AADIS=     |AADIS=     |
**	  ||     | (03)|     | (13)| (03)| (13)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAPAU ||     |SARJA|SARAB|SACTA|     |     |SACLG|
**  (08)  ||     |     |     |     |     |     |     |
**	  ||     |AARSP*AAMDE*AAPGQ|     |     |AAMDE*
**        ||     |     |     |AARLS|     |     |     |
**        ||     |     |     |AAENQ*     |     |     |
**	  ||     | (94)| (95)| (15)|     |     | (08)|
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAPAP ||     |SARJA|SARAB|SACTA|     |     |SACLG|
**  (09)  ||     |     |     |     |     |     |     |
**	  ||     |AARSP*AAMDE*AAPGQ|     |     |AAMDE*
**	  ||     |     |     |AARLS|     |     |     |
**	  ||     |     |     |AAENQ*     |     |     |
**	  ||     | (94)| (95)| (15)|     |     | (08)|
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAABT ||SACLD|SACLG|SARAB|SACLG|SACLG|SACLG|SACLD|
**  (26)  ||     |     |     |     |     |     |     |
**        ||     |OAPAU*OAPAU*OAPAU*OAPAU*OAPAU*     |
**	  ||     |AADIS=     |AAPGQ|AADIS=AAPGQ|AACCB|
**	  ||AAMDE*     |     |AADIS=     |AADIS|AAMDE*
**	  || (48)| (50)| (46)| (13)| (50)| (13)| (44)|
**  ------++-----+-----+-----+-----+-----+-----+-----+
**  IAPAM ||     |SARJA|SARAB|SACLG|SARJA|SACLG|SACLG|
**  (39)  ||     |     |     |     |     |     |     |
**	  ||     |AARSP*AAMDE*AAPGQ|AARSP*AAPGQ|AAMDE*
**        ||     |     |     |AADIS=     |AADIS=     |
**        ||     |     |     |AAMDE*     |AAMDE*     |
**	  ||     | (94)| (95)| (15)| (94)| (15)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+-----+

**  Session Termination State Table
**  -------------------------------
**
**  State ||SACTA|SACTP|SARJA|SARAB|SACAR|SACLG|
**	  ||  05 |  06 |  10 |  14 |  15 |  13 |
**  ============================================
**  Event ||
**  ============================================
**  IAASA ||     |     |     |SACLG|     |     |
**  (03)  ||     |     |     |     |     |     |
**	  ||     |     |     |AADIS=     |     |
**	  ||     |     |     |AAMDE*     |     |
**	  ||     |     |     | (04)|     |     |
**  ------++-----+-----+-----+-----+-----+-----+
**  IAASJ ||     |     |     |SACLG|     |     |
**  (04)  ||     |     |     |     |     |     |
**	  ||     |     |     |AADIS=     |     |
**	  ||     |     |     |AAMDE*     |     |
**	  ||     |     |     | (04)|     |     |
**  ------++-----+-----+-----+-----+-----+-----+
**  IAANQ ||SACTA|     |     |     |     |     |
**  (10)  ||     |     |     |     |     |     |
**	  ||AARVN=     |     |     |     |     |
**        ||AAMDE*     |     |     |     |     |
**	  || (49)|     |     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+
**  IAAEQ ||SACTA|     |     |     |     |     |
**  (11)  ||     |     |     |     |     |     |
**	  ||AARVE=     |     |     |     |     |
**        ||AAMDE*     |     |     |     |     |
**	  || (73)|     |     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+
**  IAACE ||SACLG|SACTP|SACLG|     |     |SACLG|
**  (12)  ||     |     |     |     |     |     |
**	  ||AADIS=AAMDE*AADIS=     |     |AAMDE*
**	  ||AAMDE*     |AAMDE*     |     |     |
**	  || (04)| (33)| (04)|     |     | (08)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAACQ ||SACTA|     |     |     |     |     |
**  (13)  ||     |     |     |     |     |     |
**	  ||AADEQ|     |     |     |     |     |
**	  ||AAMDE*     |     |     |     |     |
**	  || (37)|     |     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+
**  IAARQ ||SACLG|     |     |     |     |SACLG|
**  (16)  ||     |     |     |     |     |     |
**        ||AAPGQ|     |     |     |     |AAMDE*
**	  ||AADIS=     |     |     |     |     |
**	  ||AAMDE*     |     |     |     |     |
**	  || (32)|     |     |     |     | (08)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAPRII||     |SACTP|     |     |     |SACLG|
**  (17)  ||     |OAPRS*     |     |     |     |
**	  ||     |     |     |     |     |AAMDE*
**	  ||     | (39)|     |     |     | (08)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAPRA ||     |SACLG|     |     |SACLG|     |
**  (18)  ||     |     |     |     |     |     |
**	  ||     |AADIS=     |     |OAPRS*     |
**        ||     |AAMDE*     |     |AADIS=     |
**	  ||     | (34)|     |     | (30)|     |
**  ------++-----+-----+-----+-----+-----+-----+

**  State ||SACTA|SACTP|SARJA|SARAB|SACAR|SACLG|
**	  ||  05 |  06 |  10 |  14 |  15 |  13 |
**  ============================================
**  Event ||
**  ============================================
**  IAAPG ||SACLG|     |     |     |     |SACLG|
**  (22)  ||     |     |     |     |     |     |
**        ||AAPGQ|     |     |     |     |     |
**	  ||AADIS=     |     |     |     |AAMDE*
**	  ||AAMDE*     |     |     |     |     |
**	  || (32)|     |     |     |     | (08)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAXOF ||SACTA|SACTP|     |     |SACAR|SACLG|
**  (27)  ||     |     |     |     |     |     |
**	  ||AAMDE*AAMDE*     |     |AAMDE*AAMDE*
**	  || (35)| (33)|     |     | (45)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAXON ||SACTA|SACTP|     |     |SACAR|SACLG|
**  (28)  ||     |     |     |     |     |     |
**	  ||AAMDE*AAMDE*     |     |AAMDE*AAMDE*
**	  || (35)| (33)|     |     | (45)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAANQX||SACTA|     |     |     |     |     |
**  (29)  ||     |     |     |     |     |     |
**        ||AARVN=     |     |     |     |     |
**	  ||AAMDE*     |     |     |     |     |
**	  || (49)|     |     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+
**  IAACEX||SACLG|     |     |     |     |     |
**  (31)  ||     |     |     |     |     |     |
**        ||OAXON=     |     |     |     |     |
**	  ||AAPGQ|     |     |     |     |     |
**	  ||AADIS=     |     |     |     |     |
**        ||AAMDE*     |     |     |     |     |
**	  || (70)|     |     |     |     |     |
**  ------++-----+-----+-----+-----+-----+-----+
**  IAAPGX||SACLG|     |     |     |     |SACLG|
**  (32)  ||     |     |     |     |     |     |
**	  ||AAPGQ|     |     |     |     |AAMDE*
**	  ||AADIS=     |     |     |     |     |
**        ||AAMDE*     |     |     |     |     |
**	  || (32)|     |     |     |     | (08)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAAFN ||     |     |     |     |     |SACLD|
**  (35)  ||     |     |     |     |     |     |
**        ||     |     |     |     |     |AACCB|
**        ||     |     |     |     |     |AAMDE*
**        ||     |     |     |     |     | (44)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAREJR||     |     |     |SACLD|     |SACLD|
**  (36)  ||     |     |     |     |     |     |
**        ||     |     |     |AACCB|     |AACCB|
**        ||     |     |     |AAMDE*     |AAMDE*
**        ||     |     |     | (44)|     | (44)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAPRIR||     |SACAR|     |     |     |SACLG|
**  (37)  ||     |     |     |     |     |     |
**	  ||     |AAMDE*     |     |     |AAMDE*
**	  ||     | (45)|     |     |     | (08)|
**  ------++-----+-----+-----+-----+-----+-----+

**  State ||SACTA|SACTP|SARJA|SARAB|SACAR|SACLG|
**	  ||  05 |  06 |  10 |  14 |  15 |  13 |
**  ============================================
**  Event ||
**  ============================================
**  IAABQ ||SACLG|SACTP|SACLG|     |SACAR|SACLG|
**  (02)  ||     |     |     |     |     |     |
**        ||     |     |     |     |     |     |
**	  ||AAPGQ|AAMDE*AADIS=     |AAMDE*AAMDE*
**	  ||AADIS=     |AAMDE*     |     |     |
**        ||AAMDE*     |     |     |     |     |
**	  || (32)| (33)| (04)|     | (45)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAPAU ||SACTA|SACLG|     |     |SACLG|SACLG|
**  (08)  ||     |     |     |     |     |     |
**	  ||AAMDE*AADIS=     |     |AADIS=AAMDE*
**        ||     |AAMDE*     |     |AAMDE*     |
**        ||     |     |     |     |     |     |
**	  || (35)| (04)|     |     | (04)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAPAP ||SACTA|SACLG|     |     |SACLG|SACLG|
**  (09)  ||     |     |     |     |     |     |
**	  ||AAMDE*AADIS=     |     |AADIS=AAMDE*
**	  ||     |AAMDE*     |     |AAMDE*     |
**	  ||     |     |     |     |     |     |
**	  || (35)| (04)|     |     | (04)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAABT ||SACLG|SACLG|SACLG|SACLG|SACLG|SACLD|
**  (26)  ||     |     |     |     |     |     |
**        ||OAPAU*OAPAU*     |     |OAPAU*     |
**	  ||AAPGQ|AADIS=AADIS=AADIS=AADIS=AACCB|
**	  ||AADIS=     |AAMDE*AAMDE*     |AAMDE*
**	  || (13)| (50)| (04)| (04)| (50)| (44)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAABQR||SACLG|SACTP|     |     |SACAR|SACLG|
**  (34)  ||     |     |     |     |     |     |
**	  ||AAPGQ|AAMDE*     |     |AAMDE*AAMDE*
**	  ||AADIS=     |     |     |     |     |
**        ||AAMDE*     |     |     |     |     |
**	  || (32)| (33)|     |     | (45)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+
**  IAPAM ||     |     |     |     |SACLG|SACLG|
**  (39)  ||     |     |     |     |     |     |
**	  ||     |     |     |     |AADIS=AAMDE*
**        ||     |     |     |     |AAMDE*     |
**	  ||     |     |     |     | (04)| (08)|
**  ------++-----+-----+-----+-----+-----+-----+

**  Data Transfer State Table
**  -------------------------
**
**  State ||SADTI|SADSP|SADTX|SADSX|SACLG|
**	  || (03)| (04)| (08)| (09)| (13)|
**  ======================================
**  Event ||
**  ======================================
**  IAANQ ||SADTI|SADSP|     |     |     |
**  (10)  ||     |     |     |     |     |
**        ||OAPDQ*OAPDQ*     |     |     |
**	  ||AARVN=AARVN=     |     |     |
**	  || (18)| (19)|     |     |     |
**  ------++-----+-----+-----+-----+-----+
**  IAAEQ ||SADTI|SADSP|SADTX|SADSX|     |
**  (11)  ||     |     |     |     |     |
**        ||OAPEQ*OAPEQ*OAPEQ*OAPEQ*     |
**	  ||AARVE=AARVE=AARVE=AARVE=     |
**	  || (20)| (21)| (53)| (54)|     |
**  ------++-----+-----+-----+-----+-----+
**  IAACE ||SADTI|SADTI|     |SADTX|SACLG|
**  (12)  ||     |     |     |     |     |
**	  ||AAMDE*AAMDE*     |AAMDE*AAMDE*
**	  || (22)| (22)|     | (55)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAACQ ||     |SADSP|     |SADSX|     |
**  (13)  ||     |     |     |     |     |
**	  ||     |AADEQ|     |AADEQ|     |
**	  ||     |AAMDE*     |AAMDE*     |
**	  ||     | (23)|     | (56)|     |
**  ------++-----+-----+-----+-----+-----+
**  IAPDN ||SADSP|SADSP|SADSX|SADSX|SACLG|
**  (14)  ||     |     |     |     |     |
**	  ||AASND*AAENQ*AASND*AAENQ*AAMDE*
**	  || (24)| (25)| (51)| (52)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAPDE ||SADSP|SADSP|SADSX|SADSX|SACLG|
**  (15)  ||     |     |     |     |     |
**	  ||AASND*AAENQ*AASND*AAENQ*AAMDE*
**	  || (24)| (25)| (51)| (52)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAARQ ||SACTP|SACTP|     |     |SACLG|
**  (16)  ||     |     |     |     |     |
**        ||OAPDQ*OAPDQ*     |     |     |
**        ||OAPRQ=OAPRQ=     |     |     |
**	  ||     |AAPGQ|     |     |AAMDE*
**	  || (28)| (29)|     |     | (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAPRII||SACLG|SACTA|SACLG|SACTA|SACLG|
**  (17)  ||     |     |     |     |     |
**        ||OAPRS*OAPRS*OAPRS*OAPRS*     |
**	  ||AADIS=     |AADIS=AARVN=AAMDE*
**	  || (30)| (31)| (30)| (80)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAAPG ||SADTI|SADSP|     |     |SACLG|
**  (22)  ||     |     |     |     |     |
**	  ||AARVN=AARVN=     |     |AAMDE*
**	  ||AAMDE*AAMDE*     |     |     |
**	  || (16)| (17)|     |     | (08)|
**  ------++-----+-----+-----+-----+-----+

**  State ||SADTI|SADSP|SADTX|SADSX|SACLG|
**	  || (03)| (04)| (08)| (09)| (13)|
**  ======================================
**  Event ||
**  ======================================
**  IAPDNH||SADSP|     |     |     |SACLG|
**  (23)  ||     |     |     |     |     |
**	  ||AAENQ*     |     |     |AAMDE*
**	  ||AASDG|     |     |     |     |
**	  || (42)|     |     |     | (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAXOF ||SADTI|SADSP|SADTX|SADSX|SACLG|
**  (27)  ||     |     |     |     |     |
**	  ||AASXF|AASXF|AASXF|AASXF|     |
**	  ||AAMDE*AAMDE*AAMDE*AAMDE*AAMDE*
**	  || (57)| (58)| (59)| (60)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAXON ||SADTI|SADSP|SADTI|SADSP|SACLG|
**  (28)  ||     |     |     |     |     |
**	  ||AARXF|AARXF|AARXF|AARXF|     |
**	  ||     |     |AARVN=AARVN=     |
**	  ||AAMDE*AAMDE*AAMDE*AAMDE*AAMDE*
**	  || (61)| (62)| (63)| (64)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAANQX||SADTX|SADSX|     |     |     |
**  (29)  ||     |     |     |     |     |
**        ||OAPDQ*OAPDQ*     |     |     |
**	  || (65)| (66)|     |     |     |
**  ------++-----+-----+-----+-----+-----+
**  IAPDNQ||     |SADSP|     |SADSX|SACLG|
**  (30)  ||     |     |     |     |     |
**        ||     |OAXOF=     |OAXOF=     |
**	  ||     |AAENQ*     |AAENQ*AAMDE*
**	  ||     | (67)|     | (68)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAACEX||     |SADTI|     |SADTX|     |
**  (31)  ||     |     |     |     |     |
**        ||     |OAXON=     |OAXON=     |
**	  ||     |AAMDE*     |AAMDE*     |
**	  ||     | (69)|     | (74)|     |
**  ------++-----+-----+-----+-----+-----+
**  IAAPGX||SADTX|SADSX|     |     |SACLG|
**  (32)  ||     |     |     |     |     |
**	  ||AAMDE*AAMDE*     |     |AAMDE*
**	  || (71)| (72)|     |     | (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAAFN ||     |     |     |     |SACLD|
**  (35)  ||     |     |     |     |     |
**        ||     |     |     |     |AACCB|
**        ||     |     |     |     |AAMDE*
**        ||     |     |     |     | (44)|
**  ------++-----+-----+-----+-----+-----+
**  IAPRIR||SACLG|SACTA|SACLG|SACTA|SACLG|
**  (37)  ||OAPRS*OAPRS*OAPRS*OAPRS*     |
**	  ||AADIS=     |AADIS=AARVN=AAMDE*
**	  || (30)| (31)| (30)| (80)| (08)|
**  ------++-----+-----+-----+-----+-----+

**  State ||SADTI|SADSP|SADTX|SADSX|SACLG|
**	  || (03)| (04)| (08)| (09)| (13)|
**  ======================================
**  Event ||
**  ======================================
**  IAABQ ||SACLG|SACLG|SACLG|SACLG|SACLG|
**  (02)  ||     |     |     |     |     |
**        ||OAPAU*OAPAU*OAPAU*OAPAU*     |
**	  ||AADIS=AAPGQ|AADIS=AAPGQ|AAMDE*
**	  ||     |AADIS=     |AADIS=     |
**	  || (03)| (13)| (03)| (13)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAPAU ||SACTA|SACTA|SACTA|SACTA|SACLG|
**  (08)  ||     |     |     |     |     |
**	  ||AARLS|AAPGQ|AARVN=AARVN=AAMDE*
**        ||AASND*AARLS|AARLS|AAPGQ|     |
**        ||     |AAENQ*AASND*AARLS|     |
**        ||     |     |     |AAENQ*     |
**	  || (14)| (15)| (75)| (79)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAPAP ||SACTA|SACTA|SACTA|SACTA|SACLG|
**  (09)  ||     |     |     |     |     |
**	  ||AARLS|AAPGQ|AARVN=AARVN=AAMDE*
**	  ||AASND*AARLS|AARLS|AAPGQ|     |
**	  ||     |AAENQ*AASND*AARLS|     |
**	  ||     |     |     |AAENQ*     |
**	  || (14)| (15)| (75)| (79)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAABT ||SACLG|SACLG|SACLG|SACLG|SACLD|
**  (26)  ||OAPAU*OAPAU*OAPAU*OAPAU*     |
**	  ||AADIS=AAPGQ|AADIS=AAPGQ|AACCB|
**	  ||     |AADIS=     |AADIS=AAMDE*
**	  || (50)| (13)| (50)| (13)| (44)|
**  ------++-----+-----+-----+-----+-----+
**  IAABQX||SACLG|SACLG|SACLG|SACLG|SACLG|
**  (33)  ||OAXON=OAXON=OAXON=OAXON=     |
**        ||OAPAU*OAPAU*OAPAU*OAPAU*     |
**	  ||AADIS=AAPGQ|AADIS=AAPGQ|AAMDE*
**	  ||     |AADIS=     |AADIS=     |
**	  || (76)| (77)| (78)| (77)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAABQR||SADTI|SADTI|SADTX|SADSX|SACLG|
**  (34)  ||     |     |     |     |     |
**	  ||AAMDE*AAPGQ|AAMDE*AAPGQ|AAMDE*
**	  ||     |AAMDE*     |AAMDE*     |
**	  || (22)| (83)| (71)| (84)| (08)|
**  ------++-----+-----+-----+-----+-----+
**  IAPAM ||SACLG|SACLG|SACLG|SACLG|SACLG|
**  (39)  ||     |     |     |     |     |
**	  ||AADIS=AAPGQ|AADIS=AAPGQ|AAMDE*
**        ||AAMDE*AADIS=AAMDE*AADIS=     |
**        ||     |AAMDE*     |AAMDE*     |
**	  || (04)| (32)| (04)| (32)| (08)|
**  ------++-----+-----+-----+-----+-----+

**
** Inputs:
**      mde			Pointer to dequeued Message Data Element
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
**	    Renamed gcc_al() to gcc_al_event() and created gcc_al()
**	    to disallow nested processing of events.  The first
**	    call to this routine sets a flag which causes recursive
**	    calls to queue the MDE for processing sequentially.
**	29-Dec-97 (gordy)
**	    Save FASTSELECT messages in dynamically allocated buffer
**	    (GCA may step on its buffer before we are ready).
*/

STATUS
gcc_al( GCC_MDE *mde )
{
    GCC_CCB	*ccb = mde->ccb;
    STATUS	status = OK;
    
    /*
    ** If the event queue active flag is set, this
    ** is a recursive call and AL events are being
    ** processed by a previous instance of this 
    ** routine.  Queue the current event, it will
    ** be processed once we return to the active
    ** processing level.
    */
    if ( ccb->ccb_aae.al_evq.active )
    {
# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 2 )
	{
            TRdisplay( "gcc_al: queueing event %s %s\n",
                   gcx_look( gcx_mde_service, mde->service_primitive ),
                   gcx_look( gcx_mde_primitive, mde->primitive_type ));
	}
# endif /* GCXDEBUG */

	QUinsert( (QUEUE *)mde, ccb->ccb_aae.al_evq.evq.q_prev );
	return( OK );
    }

    /*
    ** Flag active processing so any subsequent 
    ** recursive calls will be queued.  We will 
    ** process all queued events once the stack
    ** unwinds to us.
    */
    ccb->ccb_aae.al_evq.active = TRUE;

    /*
    ** Process the current input event.  The queue
    ** should be empty since no other instance of
    ** this routine is active, and the initial call
    ** always clears the queue before exiting.
    */
    status = gcc_al_event( mde );

    /*
    ** Process any queued input events.  These
    ** events are the result of recursive calls
    ** to this routine while we are processing
    ** events.
    */
    while( ccb->ccb_aae.al_evq.evq.q_next != &ccb->ccb_aae.al_evq.evq )
    {
	mde = (GCC_MDE *)QUremove( ccb->ccb_aae.al_evq.evq.q_next );

# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 2 )
	{
            TRdisplay( "gcc_al: processing queued event %s %s\n",
                   gcx_look( gcx_mde_service, mde->service_primitive ),
                   gcx_look( gcx_mde_primitive, mde->primitive_type ));
	}
# endif /* GCXDEBUG */

	status = gcc_al_event( mde );
    }

    ccb->ccb_aae.al_evq.active = FALSE;		/* Processing done */

    /*
    ** If a processed event resulted in the
    ** CCB being released, a flag was set so
    ** that we would perform the actual release
    ** once we were done processing the queue.
    */
    if ( ccb->ccb_aae.a_flags & GCCAL_CCB_RELEASE )
    {
	/* 
	** Discard the CCB pointed to by the input MDE.
    	** Decrement and check the CCB use count. If 0, the 
	** CCB is clean.  Release it. Otherwise, it is still 
	** in use, so let the other user(s) release it.  This 
	** is to prevent a race condition releasing the CCB 
	** when an association is complete. 
	*/

# ifdef GCXDEBUG
	if ( IIGCc_global->gcc_trace_level >= 2 )
            TRdisplay( "gcc_al: releasing the CCB\n" );
# endif /* GCXDEBUG */

	ccb->ccb_aae.a_flags &= ~GCCAL_CCB_RELEASE;

	while( ccb->ccb_aae.al_evq.evq.q_next != &ccb->ccb_aae.al_evq.evq )
	    gcc_del_mde( (GCC_MDE *)QUremove(ccb->ccb_aae.al_evq.evq.q_next) );

	if ( ccb->ccb_aae.fsel_ptr )
	{
	    gcc_free( ccb->ccb_aae.fsel_ptr );
	    ccb->ccb_aae.fsel_ptr = NULL;
	}

	if (--ccb->ccb_hdr.async_cnt <= 0)
	{
	    gcc_del_ccb(ccb);

	    /*
	    ** Check for Comm Server in "quiescing" state.  If so, and this is
	    ** the last association, shut the Comm Server down.
	    */

# ifdef GCXDEBUG
	    if ( IIGCc_global->gcc_trace_level >= 1 )
	    	TRdisplay( "gcc_alactn_exec: ob conn ct %d conn ct %d\n",
		           IIGCc_global->gcc_ob_conn_ct,
		           IIGCc_global->gcc_conn_ct );
# endif /* GCXDEBUG */

	    if ( ! IIGCc_global->gcc_conn_ct && 
		 ! IIGCc_global->gcc_admin_ct && 
		 IIGCc_global->gcc_flags & GCC_QUIESCE )  
	    {
		GCshut();
	    }
	}
    }

    return( status );
} /* end gcc_al */

/*
** Name: gcc_init_registry
**
** Description:
**	Initialize registry protocols.
**
** Input:
**	None.
**
** Output:
**	generic_status	Mainline status code for initialization error.
**	system_status	System_specific status code.
**
** History:
**	21-Jan-04 (gordy)
**	    Created.
*/

STATUS
gcc_init_registry( STATUS *generic_status, CL_ERR_DESC *system_status )
{
    STATUS	status;

    if ( (status = gcc_tl_registry()) != OK )
	*generic_status = status & ~GCLVL_MASK;
    else
	*generic_status = OK;

    return( status == OK ? OK : FAIL );
}


/*
** Name: gcc_al_event
**
** Description:
**	Application layer event processing.  An input MDE is evaluated
**	to a AL event.  The input event causes a state transition and
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
**      18-Aug-89 (cmorris)
**          Don't log errors returned by the FSM; the FSM's already logged 
**          them!
**      10-Nov-89 (cmorris)
**          Removed code that checked for orphan MDEs. Disassociate changes
**          do away with orphans.
**	19-Jun-96 (gordy)
**	    Renamed from gcc_al() to handle recursive requests.
**	    This routine will no longer be called recursively.
*/

static STATUS
gcc_al_event( GCC_MDE *mde )
{
    u_i4	input_event;
    STATUS	status;

    /*
    ** Invoke  the FSM executor, passing it the current state and the 
    ** input event, to execute the output event(s), the action(s)
    ** and return the new state.  The input event is determined by the
    ** output of gcc_al_evinp().
    */

    status = OK;
    input_event = gcc_al_evinp(mde, &status);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "gcc_al: primitive %s %s event %s size %d\n",
		gcx_look( gcx_mde_service, mde->service_primitive ),
                gcx_look( gcx_mde_primitive, mde->primitive_type ),
		gcx_look( gcx_gcc_ial, input_event ),
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
	** Determine the state transition for this state and input event and
	** return the new state.
	*/

	old_state = ccb->ccb_aae.a_state;
	j = alfsm_tbl[input_event][old_state];

	/* Make sure we have a legal transition */
	if (j != 0)
	{
	    ccb->ccb_aae.a_state = alfsm_entry[j].state;

	    /* do tracing... */
	    GCX_TRACER_3_MACRO("gcc_alfsm>", mde, "event = %d, state = %d->%d",
			     input_event, old_state, (u_i4)(ccb->ccb_aae.a_state));

# ifdef GCXDEBUG
	    if ( IIGCc_global->gcc_trace_level >=1 )
		TRdisplay( "gcc_alfsm: new state=%s->%s\n", 
			   gcx_look(gcx_gcc_sal, old_state), 
			   gcx_look(gcx_gcc_sal, (u_i4)ccb->ccb_aae.a_state) );
# endif /* GCXDEBUG */

	    /*
	    ** For each output event in the list for this state and input event,
	    ** call the output executor.
	    */

	    for (i = 0;
		 i < AL_OUT_MAX
		 &&
		 (output_event =
		    alfsm_entry[j].output[i])
		 > 0;
		 i++)
	    {
		if ((status = gcc_alout_exec(output_event, mde, ccb)) != OK)
		{
		    /*
		    ** Execution of the output event has failed. Aborting the 
		    ** association will already have been handled so we just get
		    ** out of here.
		    */
		    return (status);
		} /* end if */
	    } /* end for */

	    /*
	    ** For each action in the list for this state and input event, call the
	    ** action executor.
	    */

	    for (i = 0;
		 i < AL_ACTN_MAX
		 &&
		 (action_code =
		    alfsm_entry[j].action[i])
		 != 0;
		 i++)
	    {
		if ((status = gcc_alactn_exec(action_code, mde, ccb)) != OK)
		{
		    /*
		    ** Execution of the action has failed. Aborting the 
		    ** association will already have been handled so we just get
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
	    ** protocol violation.
	    ** Log the error.  The message contains 2 parms: the input event and
	    ** the current state.
	    */

	    erlist.gcc_parm_cnt = 2;
	    erlist.gcc_erparm[0].size = sizeof(input_event);
	    erlist.gcc_erparm[0].value = (PTR)&input_event;
	    erlist.gcc_erparm[1].size = sizeof(old_state);
	    erlist.gcc_erparm[1].value = (PTR)&old_state;
	    status = E_GC220E_AL_FSM_STATE;

	    /* Abort the association */
	    gcc_al_abort(mde, ccb, &status, (CL_ERR_DESC *) NULL, &erlist);
	    status = FAIL;
	}
    }
    else
    {
	/*
	** Evaluation of the input event has failed.  Log the status
	** returned by gcc_al_evinp, and abort FSM execution.
	*/
	gcc_al_abort(mde, mde->ccb, &status, (CL_ERR_DESC *) NULL,
			(GCC_ERLIST *) NULL);
	status = FAIL;
    }
    return(status);
}  /* end gcc_al_event */

/*{
** Name: gcc_alinit	- Application Layer initialization routine
**
** Description:
**      gcc_alinit is invoked by ILC on comm server startup.  It initializes
**      the AL and returns.  The following functions are performed: 
** 
**      - Initialize use of GCA by issuing GCA_INITIATE 
**      - Register as a server by issuing GCA_REGISTER 
**      - Establish an outstanding GCA_LISTEN through the local GCA 
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
**      18-Aug-89 (cmorris)
**          Don't log errors returned by FSM; the FSM already logged them!
**      10-Nov-89 (cmorris)
**          Move up to GCA api version 2.
**      14-Nov-89 (cmorris)
**          Now executes gcc_al directly.
**      17-Nov-89 (cmorris)
**          Use gcc_get_obj to allocate a listen parameter list.
**  	06-Oct-92 (cmorris)
**  	    Register as COMSVR on GCA_REGISTER; initiate with API version 4.
**	21-Nov-95 (gordy)
**	    Don't do any GCA actions if embedded.
**	 8-Oct-98 (gordy)
**	    Allow anyone to do GCM read operations to support
**	    installation registry.
**	28-jul-2000 (somsa01)
**	    Write out the GCC server id to standard output.
*/
STATUS
gcc_alinit( STATUS *generic_status, CL_ERR_DESC *system_status )
{
    GCA_IN_PARMS        in_parms;
    GCA_RG_PARMS        rg_parms;
    GCC_CCB		*ccb;
    GCC_MDE		*mde;
    STATUS              status;
    STATUS              tmp_status;
    STATUS              call_status;
    STATUS              return_code = OK;
    char		server_name[16];
    char		buffer[512];

    *generic_status = OK;

    if ( ! (IIGCc_global->gcc_flags & GCC_STANDALONE) )
    {
	/*
	** AL initialization consists of starting up GCA as
	** a sub-component.  This is not applicable to the
	** embedded Comm Server configuration.
	*/
	return( OK );
    }

    /*
    ** Invoke GCA_INITIATE service to initialize use of GCA.
    ** Fill in the parm list, then call IIGCa_call.  
    */

    in_parms.gca_expedited_completion_exit = gcc_gca_exit;
    in_parms.gca_normal_completion_exit = gcc_gca_exit;
    in_parms.gca_alloc_mgr = NULL;
    in_parms.gca_dealloc_mgr = NULL;
    in_parms.gca_p_semaphore = NULL;
    in_parms.gca_v_semaphore = NULL;
 
    /*
    ** We need to specify the api version that we're using and that we're
    ** using asynchronous services.
    */

    in_parms.gca_modifiers = GCA_API_VERSION_SPECD | GCA_GCM_READ_ANY;
    in_parms.gca_local_protocol = GCC_GCA_PROTOCOL_LEVEL;
    in_parms.gca_api_version = GCA_API_LEVEL_5;
    call_status = gca_call( &gca_cb, 	GCA_INITIATE, 
				(GCA_PARMLIST *)&in_parms, 
				(i4) GCA_SYNC_FLAG,	/* Synchronous */
				(PTR) 0,		/* async id */
				(i4) -1,		/* no timeout */
				&status);

    /*
    ** If there is a failure as indicated by returned status, fill in the
    ** generic_status and system_status codes and return.
    */

    if (call_status != OK)
    {
        *generic_status = status;
	return FAIL;
    } /* end if */
    if (in_parms.gca_status != OK)
    {
	*generic_status = in_parms.gca_status;
	STRUCT_ASSIGN_MACRO(in_parms.gca_os_status, *system_status);
	return FAIL;
    } /* end if */
    else
    {
	IIGCc_global->gcc_gca_hdr_lng = in_parms.gca_header_length;
    } /* end else */

    /*
    ** Register as a server with Name Server.
    ** Fill in the parm list, then call IIGCa_call.  
    */

    rg_parms.gca_l_so_vector = 0;
    rg_parms.gca_served_object_vector = NULL;
    rg_parms.gca_srvr_class = GCA_COMSVR_CLASS;
    rg_parms.gca_installn_id = NULL;
    rg_parms.gca_process_id = NULL;
    rg_parms.gca_no_connected = 0;
    rg_parms.gca_no_active = 0;
    rg_parms.gca_modifiers = GCA_RG_COMSVR;
    rg_parms.gca_listen_id = NULL;
    call_status = gca_call( &gca_cb, 	GCA_REGISTER, 
				(GCA_PARMLIST *)&rg_parms, 
				(i4) GCA_SYNC_FLAG,	/* Synchronous */
				(PTR) 0,		/* async id */
				(i4) -1,		/* no timeout */
				&status);

    /*
    ** If there is a failure as indicated by returned status, fill in the
    ** generic_status and system_status codes and return.
    */

    if (call_status != OK)
    {
        *generic_status = status;
	return FAIL;
    } /* end if */
    if (rg_parms.gca_status != OK)
    {
	tmp_status = E_GC2219_GCA_REGISTER_FAIL;
	gcc_er_log(&tmp_status,
		   (CL_ERR_DESC *)NULL,
		   (GCC_MDE *)NULL,
		   (GCC_ERLIST *)NULL);
	*generic_status = rg_parms.gca_status;
	STRUCT_ASSIGN_MACRO(rg_parms.gca_os_status, *system_status);
	return FAIL;
    } /* end if */	

    /*
    ** Save the listen id in the global data area IIGCc_global.
    */

    if (rg_parms.gca_listen_id != NULL)
    {
	STncpy(IIGCc_global->gcc_lcl_id, rg_parms.gca_listen_id,
		GCC_L_LCL_ID);
    } /* end if */	

    /* Send the server id to the error logging system */

    STprintf( server_name, ERx("%sGCC"), SystemVarPrefix ); /* IIGCC/JASGCC */
    gcc_er_init( server_name, IIGCc_global->gcc_lcl_id );

    STprintf(buffer, "\nGCC Server = %s\n", IIGCc_global->gcc_lcl_id);
    SIstd_write(SI_STD_OUT, buffer);

    /*
    ** Invoke the AL FSM to initialize it and establish an outstanding
    ** GCA_LISTEN.
    */

    mde = gcc_add_mde( ( GCC_CCB * ) NULL, 0 );
    ccb = gcc_add_ccb();
    if ((mde == NULL) || (ccb == NULL))
    {
        return_code = FAIL;
	return(return_code);
    } /* end if */

    mde->ccb = ccb;
    mde->service_primitive = A_LISTEN;
    mde->primitive_type = RQS;
    ccb->ccb_aae.a_state = SACLD;
    call_status = gcc_al(mde);
    if (call_status != OK)
    {
	return_code = FAIL;
    } /* end if */

    /*
    ** Initialization is complete.  Return to caller.
    */

    return(return_code);
}  /* end gcc_alinit */

/*{
** Name: gcc_alterm	- Application Layer termination routine
**
** Description:
**      gcc_alterm is invoked on comm server shutdown.  It cleans up 
**      the AL and returns.  The following functions are performed: 
** 
**      - Issue GCA_TERMINATE to clean up GCA.
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
**	21-Nov-95 (gordy)
**	    Don't do any GCA actions if embedded.
*/
STATUS
gcc_alterm( STATUS *generic_status, CL_ERR_DESC *system_status )
{
    GCA_TE_PARMS        te_parms;
    STATUS              call_status;
    STATUS              status;
    STATUS              return_code = OK;

    *generic_status = OK;

    if ( ! (IIGCc_global->gcc_flags & GCC_STANDALONE) )
    {
	/*
	** AL termination consists of shutting down GCA as
	** a sub-component.  This is not applicable to the
	** embedded Comm Server configuration.
	*/
	return( OK );
    }

    /*
    ** Invoke GCA_TERMINATE service to terminate use of GCA.
    */

    IIGCc_global->gcc_flags |= GCC_STOP;
    call_status = gca_call( &gca_cb, 	GCA_TERMINATE, 
				(GCA_PARMLIST *)&te_parms, 
				(i4) GCA_SYNC_FLAG,	/* Synchronous */
				(PTR) 0,		/* async id */
				(i4) -1,		/* no timeout */
				&status);

    /*
    ** If there is a failure as indicated by returned status, fill in the
    ** generic_status and system_status codes and return.
    */

    if (te_parms.gca_status != OK)
    {
	*generic_status = te_parms.gca_status;
	STRUCT_ASSIGN_MACRO(te_parms.gca_os_status, *system_status);
	return_code = FAIL;
    } /* end if */

    /*
    ** Termination is complete.  Return to caller.
    */

    return(return_code);
}  /* end gcc_alterm */


/*{
** Name: gcc_gca_exit	- Asynchronous GCA service exit handler
**
** Description:
**      gcc_gca_exit is the first level exit handler invoked for completion 
**      of asynchronous GCA service requests.  It identifies the service 
**	and the association from the identifier passed to it by GCA.  
**	
**	The MDE is filled in from the GCA parm list as appropriate for the
**	particular GCA service which completed.  In some cases it may be
**	necessary to transform the MDE to a different SDU.  This would
**	typically be the case when a failure associated with the async event
**	has occurred, e.g. failure of a GCA service request.  AL entity
**	execution is then invoked by calling gcc_al.
**
** Inputs:
**      async_id                        The MDE associated with the request,
**					casted to a i4 for GCA.
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
**	    none
**
** History:
**      20-Jan-88 (jbowers)
**          Initial routine creation
**      08-Nov-88 (jbowers)
**          Added decryption of password for remote site login.
**      08-Dec-88 (jbowers)
**          Added transformation of uid, pwd and acct. name to xfer syntax.
**      21-Mar-89 (jbowers)
**	    Fix for processing a failed listen: insert the
**	    association id in the CCB even if the listen fails.  This fixes the
**	    accvio failure when max outbound connections is exceeded.
**      31-Jul-89 (cmorris)
**          Modified request case to handle AL and GCA version numbers; added
**          case to handle response completion.
**      02-Aug-89 (cmorris)
**          Fixed incorrect generic error logged on interpret fail after
**          expedited receive has completed.
**      03-Aug-89 (cmorris)
**          Removed "break" statements from code that processed interpret
**          failures on normal/expedited receives. These caused the async count
**          not to be decremented.
**      22-Aug-89 (cmorris)
**          Added "break" to response case.
**      05-Sep-89 (cmorris)
**          Negotiate down the requested GCA protocol if its greater than
**          what we support.
**      25-Oct-89 (cmorris)
**          Implemented calls to GCnxxx functions to convert individual types
**          to network xfer syntax.
**      08-Nov-89 (cmorris)
**          Added GCC_A_DISASSOC case. Added code to callback on request and
**          disassociate cases.
**      10-Nov-89 (cmorris)
**          Check for server quiescing in listen case.
**      13-Nov-89 (cmorris)
**          Store the association id in the CCB even if the GCA request
**          failed:- it's needed for the subsequent disassociate...
**      14-Nov-89 (cmorris)
**          On listen, reject aux data of unknown type.
**      16-Nov-89 (cmorris)
**          Don't log error if association has been released.
**      13-Aug-91 (cmorris)
**  	    Timestamp ccb at connection startup.
**  	06-Oct-92 (cmorris)
**  	    Added support for resuming GCA_LISTENs.
**  	20-Nov-92 (cmorris)
**  	    Use new "end-of-group" flag from GCA_INTERPRET.
**  	19-Jan-93 (cmorris)
**  	    If we fail to allocate parameter list, make sure
**  	    error that will be handed to GCA is initialized.
**  	11-Feb-93 (cmorris)
**  	    Reworked handling of OB_MAX:- previous code allowed
**  	    us to get into the situation of having no listen
**  	    posted.
**	5-apr-93 (robf)
**	    Add handling for security label on connections
**	14-Nov-95 (gordy)
**	    Added support for GCA_API_LEVEL_5 which removes the
**	    need for GCA_INTERPRET on fast select messages.
**	04-apr-96 (emmag)
**	    Pass the length of tmp_sec_label to GCnsadd to avoid reading
**	    past the end of the data segment, in the case where no
**	    security label has been specified.
**	29-May-97 (gordy)
**	    GCA aux data structure now only defines the aux data header.
**	    GCA aux data area must be handled separately.
**	02-jun-1997 (canor01)
**	    Do not log the expedited receive failure.
**	 3-Jun-97 (gordy)
**	    Building of initial connection info moved to SL.  Info now
**	    saved in the CCB.
**	17-Oct-97 (rajus01)
**	    CommServers with GCC_GCA_PROTOCOL_LEVEL greater or equal to
**	    GCA_PROTOCOL_LEVEL_63 support a new auxillary data type
**	    (GCA_ID_RMT_INFO). This new structure is easy to extend in the
**	    future. GCA_ID_RMT_ADDR didn't have this capability. This new
**	    type is added for adding encryption, authentication interface
**	    using external security environments. This replaces GCA_ID_RMT_ADDR.
**	 4-Dec-97 (rajus01)
**	    Added remote authentication.
**	29-Dec-97 (gordy)
**	    Save FASTSELECT messages in dynamically allocated buffer
**	    (GCA may step on its buffer before we are ready).
**	27-Feb-98 (loera01) Bug 89246
**	    Load security labels with a length of sizeof(SL_LABEL), rather 
**	    than GCC_L_SECLABEL.  For most platforms, the internal security 
**	    label is much smaller than GCC_L_SECLABEL, and can cause an 
**	    access violation in gcnadds().
**	20-Mar-98 (gordy)
**	    Cleaned up defaults for security labels.
**	13-Jul-00 (gordy)
**	    Logging of association aborts was removed because of timing
**	    problems (expected aborts during disconnect were being logged).
**	    The problem was that logging was being done in gcc_gca_exit()
**	    before the correct state was established in the AL FSM.  Move
**	    logging to OAPAU for A_ABORT and A_P_ABORT.  OAPAU is not
**	    executed during shutdown so only real aborts will be logged.
**	16-mar-2001 (somsa01)
**	    In the case of E_GC000B_RMT_LOGIN_FAIL, log
**	    E_GC000E_RMT_LOGIN_FAIL_INFO as well.
**	 6-Jun-03 (gordy)
**	    Use trg_addr for remote target address info.  Attach CCB
**	    as MIB object after GCA_LISTEN completes successfully.
**	24-Sep-2003 (rajus01) Bug #110936
**	    Repost GCA_LISTEN even when OBMAX is reached.
**	21-Jan-04 (gordy)
**	    Registry control can occur during GCM operations which
**	    cause GCA_LISTEN to fail with E_GC0032_NO_PEER.  Check
**	    for registry startup flag after such a failure and
**	    initialize the registry.
**	15-Jul-04 (gordy)
**	    Enhanced encryption of passwords between gcn and gcc.
**	21-Jul-09 (gordy)
**	    Username, password, target and encryption mechanism are 
**	    dynamically allocated.  Declare standard sized password
**	    buffer.  Use dynamic storage if size exceeds default.
*/

VOID
gcc_gca_exit( PTR async_id )
{
    i4                  cei;
    i4                  gca_rq_type;
    GCC_MDE             *mde;
    GCC_CCB             *ccb;
    STATUS		status = OK;
    STATUS		call_status;
    STATUS		generic_status;
    STATUS              tmp_status;

    /*
    ** Get the CCB, and the MDE.
    */

    mde = (GCC_MDE *)async_id;
    gca_rq_type = mde->gca_rq_type;
    ccb = mde->ccb;
    cei = ccb->ccb_hdr.lcl_cei;

    /*
    ** Perform the processing appropriate to the completed service 
    ** request.
    */

    /* do tracing... */
    GCX_TRACER_2_MACRO("gcc_gca_exit>", mde,
	"primitive = %d, cei = %d", gca_rq_type, cei);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
    {
	TRdisplay( "----- DOWN\n" );
	TRdisplay( "gcc_gca_exit: primitive %s cei %d\n",
		gcx_look( gcx_gca_rq, gca_rq_type ),
                cei );
    }
# endif /* GCXDEBUG */

    switch (gca_rq_type)
    {
    case GCC_A_LISTEN:
    {
	GCA_LS_PARMS	    *ls_parms =
				(GCA_LS_PARMS *) &mde->mde_act_parms.gca_parms;
    	GCA_AUX_DATA	    *aux_data;
    	GCC_MDE             *lsn_mde;
    	GCC_CCB	    	    *lsn_ccb;

        /* check to make sure request has completed */
        if (ls_parms->gca_status == E_GCFFFE_INCOMPLETE)
	{
# ifdef GCXDEBUG
            if( IIGCc_global->gcc_trace_level >= 1 )
            {
   	        TRdisplay( "gcc_gca_exit: resuming GCA_LISTEN operation...\n");
            }
# endif /* GCXDEBUG */

            /* call again... */
            call_status = gca_call( &gca_cb, GCA_LISTEN, 
				     (GCA_PARMLIST *)ls_parms,
				     (i4) GCA_ASYNC_FLAG|GCA_RESUME,
                                                            /* Asynchronous */
				     (PTR)mde, 	    	    /* async id */
				     (i4) -1,	    /* no timeout */
				     &status);

    	    /* 
    	    ** Check whether listen has yet to be resumed. Note: the check
    	    ** is done _after_ the call to avoid unneeded work in the case
    	    ** where GCA_LISTEN repeatedly completes synchronously. In that
    	    ** case, there is no need to repost the listen until such time
    	    ** as we unwind back to the return from the last resumption of
    	    ** the call.
    	    */
    	    if(! (ccb->ccb_aae.a_flags & GCCAL_LSNRES))
	    {
		ccb->ccb_aae.a_flags |= GCCAL_LSNRES;

    	    	/*
    	    	** Unless we're at OBMAX, invoke the AL FSM to establish
    	    	** another outstanding GCA_LISTEN.
    	    	*/

    	    	if (!(IIGCc_global->gcc_flags & GCC_AT_OBMAX))
		{
    		    lsn_mde = gcc_add_mde( ( GCC_CCB * ) NULL, 0 );
    	    	    lsn_ccb = gcc_add_ccb();
    	    	    if ((lsn_mde == NULL) || (lsn_ccb == NULL))
    	    	    {
 	            	/* Log an error and abort the association */
		    	generic_status = E_GC2004_ALLOCN_FAIL;
	            	gcc_al_abort(lsn_mde, lsn_ccb, &generic_status, 
    	    	    	    	     (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL);
    	    	    } /* end if */
    	    	    else
		    {
    	    	    	lsn_mde->ccb = lsn_ccb;
    	    	    	lsn_mde->service_primitive = A_LISTEN;
    	    	    	lsn_mde->primitive_type = RQS;
    	    	    	lsn_ccb->ccb_aae.a_state = SACLD;
    	    	    	(VOID) gcc_al(lsn_mde);
		    } /* end of else */

		}
		else
		{

		    /*
		     ** Set flag to repost GCA_LISTEN even if OBMAX is
		     ** reached.
		     ** Comm Server would allow to post one extra GCA_LISTEN
		     ** when OBMAX is reached. The condition when OBMAX is
		     ** reached and Name server bed check failed put the
		     ** Comm Server in a state NOT to accept any more
		     ** client requests or Name Server requests and sending 
		     ** no replies to those requests.
                     */

		     IIGCc_global->gcc_flags |= GCC_GCA_LSTN_NEEDED;
	        }

             }

	    /*
	    ** If there is a failure on the GCA_LISTEN resumption, log
	    ** an error and abort the association.
	    */

	    if (call_status != OK)
	    {
		generic_status = status;
	        gcc_al_abort(mde, ccb, &generic_status, (CL_ERR_DESC *) NULL,
			     (GCC_ERLIST *) NULL);
	    }
# ifdef GCXDEBUG
	    if( IIGCc_global->gcc_trace_level >= 1 )
		TRdisplay( "----- DOWN-END\n" );
# endif /* GCXDEBUG */
            return;
        }

        /* 
        ** Store the assoc id in the CCB regardless of whether there's an 
        ** error. We need to do this 'cos even on the error case we 
        ** subsequently do a disassociate on the association.
        */
	ccb->ccb_aae.assoc_id =
	    ls_parms->gca_assoc_id;

	if (ls_parms->gca_status == OK)
	{
	    GCC_ERLIST	erlist;
	    bool	remote = FALSE;
	    char	pwdbuf[ 128 ];
	    char	*password = NULL;

	    /*
	    ** determine from the aux_data type indication what sort of listen
	    ** completion this is.  The options are the following:
	    **
	    **	    GCA_ID_SHUTDOWN - This is a "Shut down now" datagram.
	    **	    GCA_ID_QUIESCE  - This is a "Quiesce, then shut down"
	    **				datagram.
	    **	    GCA_ID_RMT_ADDR - This is a normal external association
	    **				request (either GCA or GCM).
	    **	    GCA_ID_RMT_INFO - The new external 
	    **				association request (either GCA or GCM).
	    **	    GCA_ID_CMDSESS - This is a command session.
    	    **
	    */

	    aux_data = (GCA_AUX_DATA *)ls_parms->gca_aux_data;
	    switch (aux_data->type_aux_data)
	    {
	    case GCA_ID_SHUTDOWN:
	    {
		mde->service_primitive = A_SHUT;
                mde->mde_generic_status[0] = E_GC0040_CS_OK;
		break;
	    }
	    case GCA_ID_QUIESCE:
	    {
		mde->service_primitive = A_QUIESCE;
                mde->mde_generic_status[0] = E_GC0040_CS_OK;
		break;
	    }
	    case GCA_ID_CMDSESS:
	    {
		mde->service_primitive = A_CMDSESS;
		mde->mde_generic_status[0] = OK;

		if ( ls_parms->gca_user_name  &&  *ls_parms->gca_user_name )
		{
		    ccb->ccb_hdr.username = 
			gcc_str_alloc( ls_parms->gca_user_name,
				       STlength( ls_parms->gca_user_name ) );
		    ccb->ccb_hdr.flags |= CCB_USERNAME;
		}
		break;
	    }
	    case GCA_ID_RMT_ADDR:
	    {
		GCA_RMT_ADDR		*rmt_addr;

		/*
		** From the GCA_LS_PARMS parm list, get the addressing
		** information obtained from Name Server by the initiating
		** client's GCA.
		** 
		** The gca_aux_data element of the GCA_LS_PARMS parm list
		** structure points to the remote addressing information in the
		** format of a GCA_AUX_DATA structure containing a substructure
		** with all the requisite data.  The addressing data elements
		** are copied to the elements of the CCB.
		**
		** In the following sequence, the logical database name
		** and target addressing elements are filled in.
		*/
		rmt_addr = (GCA_RMT_ADDR *)
			   ((char *)aux_data + sizeof( GCA_AUX_DATA ));
		ccb->ccb_hdr.target = 
		    gcc_str_alloc( rmt_addr->partner_id, 
		    		   STlength( rmt_addr->partner_id ) );
		STcopy( rmt_addr->protocol_id, ccb->ccb_hdr.trg_addr.n_sel );
		STcopy( rmt_addr->node_id, ccb->ccb_hdr.trg_addr.node_id );
		STcopy( rmt_addr->port_id, ccb->ccb_hdr.trg_addr.port_id );

		if ( STlength( rmt_addr->user_id ) )
		{
		    i4 pwd_len = STlength( rmt_addr->password );

		    ccb->ccb_hdr.username = 
		    	gcc_str_alloc( rmt_addr->user_id,
				       STlength( rmt_addr->user_id ) );
		    ccb->ccb_hdr.flags |= CCB_USERNAME;

		    if ( pwd_len >= sizeof( pwdbuf ) )
		    	password = gcc_str_alloc(rmt_addr->password,
						 STlength(rmt_addr->password));
		    else  if ( pwd_len )
		    {
			password = pwdbuf;
			STcopy( rmt_addr->password, password );
		    }
		}
		else  if (ls_parms->gca_user_name && *ls_parms->gca_user_name)
		{
		    ccb->ccb_hdr.username = 
		    	gcc_str_alloc( ls_parms->gca_user_name,
				       STlength( ls_parms->gca_user_name ) );
		    ccb->ccb_hdr.flags |= CCB_USERNAME;
		}

		remote = TRUE;
		break;
	    }
	    case GCA_ID_RMT_INFO:
	    {
	      char 	*rmt_buf;
	      char	*buf_end;

	      /* CommServers with GCC_GCA_PROTOCOL_LEVEL greater or equal to
	      ** GCA_PROTOCOL_LEVEL_63 support this new auxillary data type.
	      ** This GCA_RMT_INFO structure is extensible. Encryption mode,
	      ** encryption mechanism, remote authentication are received in
	      ** addition to remote connection info.
	      **
	      ** Unpack the remote_info.
	      */

	      rmt_buf = ( char * )aux_data + sizeof( GCA_AUX_DATA );
	      buf_end = rmt_buf +
			( aux_data->len_aux_data - sizeof( GCA_AUX_DATA ) );

	      while( rmt_buf < buf_end )
	      {
	          i4 	id, len;
		  char	*val;

	      	  rmt_buf += gcu_get_int( rmt_buf, &id );
		  rmt_buf += gcu_get_int( rmt_buf, &len );
		  val = rmt_buf;
		  rmt_buf += len;

		  switch( id )
		  {
		  case GCA_RMT_PRTN_ID:
		  {
		      ccb->ccb_hdr.target = gcc_str_alloc( val, len );
		      break;
		  }
		  case GCA_RMT_PROTO_ID:
		  {
	      	      ( VOID ) MEcopy( val, len, ccb->ccb_hdr.trg_addr.n_sel );
		      break;
		  }
		  case GCA_RMT_NODE_ID:
		  {
	      	      ( VOID ) MEcopy(val, len, ccb->ccb_hdr.trg_addr.node_id);
		      break;
		  }
		  case GCA_RMT_PORT_ID:
		  {
	      	      ( VOID ) MEcopy(val, len, ccb->ccb_hdr.trg_addr.port_id);
		      break;
		  }
		  case GCA_RMT_USER:
		  {
		      ccb->ccb_hdr.username = gcc_str_alloc( val, len );
		      ccb->ccb_hdr.flags |= CCB_USERNAME;
		      break;
		  }
		  case GCA_RMT_PASSWD:
		  {
		      if ( len >= sizeof( pwdbuf ) )
		      	  password = gcc_str_alloc( val, len );
		      else  if ( len )
		      {
		          password = pwdbuf;
			  MEcopy( val, len, password );
			  password[ len ] = EOS;
		      }
		      break;

		  }
		  case GCA_RMT_AUTH:		/* remote auth */
		  {
		      ccb->ccb_hdr.authlen = (u_i4)len;

		      if( len )
		      {
		          ccb->ccb_hdr.auth = MEreqmem( 0, (u_i4)len, 
							FALSE, &status);
	      	          ( VOID ) MEcopy( val, len, ccb->ccb_hdr.auth );
		      }
		      break;
		  }
		  case GCA_RMT_EMECH:
		  {
		      ccb->ccb_hdr.emech = gcc_str_alloc( val, len );
		      break;
		  }
		  case GCA_RMT_EMODE:
		  {
		      status = gcc_encrypt_mode( val, &ccb->ccb_hdr.emode );
		      if( status != OK )
		      {
			  GCC_ERLIST	erlist;
			  STATUS 	tmp_status;
			  tmp_status = E_GC200A_ENCRYPT_MODE;
			  erlist.gcc_parm_cnt = 1;
			  erlist.gcc_erparm[0].size = STlength( val );
			  erlist.gcc_erparm[0].value = val;
			  gcc_er_log( &tmp_status, NULL, NULL, &erlist );
		      }
		      break;
		  }
		  default:
		  {
                    /* unrecognized data ID */
		      status = FAIL;
		      break;
		  }

		  }/* end switch */
	      }

	      if ( ! (ccb->ccb_hdr.flags & CCB_USERNAME)  &&  
	           ls_parms->gca_user_name  &&  *ls_parms->gca_user_name )
	      {
		  ccb->ccb_hdr.username = 
		      gcc_str_alloc( ls_parms->gca_user_name,
		      		     STlength( ls_parms->gca_user_name ) );
		  ccb->ccb_hdr.flags |= CCB_USERNAME;

                  if ( password )
		  {
		      if ( password != pwdbuf )  gca_free( (PTR)password );
		      password = NULL;
		  }
	      }

	      remote = TRUE;
	      break;
	    }
	    default:
		{
                    /* unrecognized aux data type */
                    status = FAIL;
		    break;
		}
	    } /* end switch */

	    /* Process the following if aux data type is GCA_ID_RMT_ADDR or
	    ** GCA_ID_REMOTE_INFO 
	    */
	    if ( remote  &&  status == OK )
	    {
		/*
		** This is an outbound association.
		** Bump counters and set CCB_OB_CONN 
		** for A_FINISH.  Timestamp the ccb.
		*/
		char buff[16];
		MOulongout(0, (u_i8)ccb->ccb_hdr.lcl_cei, sizeof(buff), buff);
		if ( MOattach( MO_INSTANCE_VAR, GCC_MIB_CONNECTION, 
				buff, (PTR)ccb ) == OK )
		    ccb->ccb_hdr.flags |= CCB_MIB_ATTACH;

		IIGCc_global->gcc_conn_ct++;
		IIGCc_global->gcc_ob_conn_ct++;
		ccb->ccb_hdr.flags |= CCB_OB_CONN;
		ccb->ccb_hdr.inbound = No;
    	    	TMnow(&ccb->ccb_hdr.timestamp);

	        ccb->ccb_aae.buf_size = ls_parms->gca_size_advise;

	    	/* 
		** Set up flags for GCA association in CCB 
		*/
	    	if ( ls_parms->gca_flags & GCA_LS_GCM )
		    ccb->ccb_aae.a_flags |= GCCAL_GCM;

    	    	if ( ls_parms->gca_flags & GCA_LS_FASTSELECT )
		{
		    ccb->ccb_aae.a_flags |= GCCAL_FSEL;

		    /* 
    	    	    ** Save the aux data as it will contain the
    	    	    ** actual data of the fast select. When a 
		    ** positive request response completes, we'll 
		    ** fake a normal receive using this data.
		    */
		    ccb->ccb_aae.fsel_type = ls_parms->gca_message_type;
    	    	    ccb->ccb_aae.fsel_len = (u_i4)ls_parms->gca_l_fs_data;
    	    	    ccb->ccb_aae.fsel_ptr = gcc_alloc( ccb->ccb_aae.fsel_len );
		    MEcopy( ls_parms->gca_fs_data, 
			    ccb->ccb_aae.fsel_len, ccb->ccb_aae.fsel_ptr );
		}

		/*
		** Fill in the user data section of the CCB.  These are the
		** data to be passed to the remote server to whom this request
		** is directed.  These are the elements returned from the
		** GCA_LISTEN: user_name, password, and initator's GCA protocol
		** version.  The password is decrypted here.  If a remote
		** user_id has been sent in aux_data, it and the corresponding
		** password (if any) are sent to be used at the remote target
		** site.  Otherwise, the user_id supplied in the completed 
		** GCA_LISTEN parm list is sent.
		*/

		ccb->ccb_hdr.gca_flags = ls_parms->gca_flags;

                /* 
		** Negotiate down the GCA protocol if its greater than what
                ** we support.
                */
                ccb->ccb_hdr.gca_protocol =
		    (ls_parms->gca_partner_protocol > GCC_GCA_PROTOCOL_LEVEL)
		    ? GCC_GCA_PROTOCOL_LEVEL : ls_parms->gca_partner_protocol;

                /* 
                ** Fill in the password
                */
                if ( ccb->ccb_hdr.flags & CCB_USERNAME  &&  password )
		{
		    char	ns_id[ GCC_L_LCL_ID ];
		    u_char	tmpbuf[ 128 ];
		    u_char	*tmp;
		    CL_ERR_DESC	sys_err;
		    i4		len;

		    /*
		    ** Encryption mask is based on Name Server
		    ** address which may (infrequently) change.
		    */
		    tmp_status = GCnsid( GC_FIND_NSID, 
					 ns_id, sizeof( ns_id ), &sys_err );
		    if ( tmp_status == OK  &&
			 STcompare( ns_id, IIGCc_global->gcc_ns_id ) )
		    {
			char	key[ GCC_L_LCL_ID + 10 ];

			STcopy( ns_id, IIGCc_global->gcc_ns_id );
			STpolycat( 2, GCC_GCN_PREFIX, ns_id, key );
			gcc_init_mask( key, sizeof(IIGCc_global->gcc_ns_mask),
					    IIGCc_global->gcc_ns_mask );
		    }

		    /* 
		    ** Encrypted password is passed as text.
		    ** Decoded password will not be larger
		    ** than encoded form.
		    */
		    len = STlength( password );
		    tmp = (len > sizeof( tmpbuf )) 
		    		? (u_char *)gcc_alloc( len ) : tmpbuf;

		    if ( tmp )
		    {
		    	CItobin( password, &len, tmp );
		    	ccb->ccb_hdr.password = (char *)gcc_alloc( len + 1 );
		    }

		    if ( ccb->ccb_hdr.password )
			gcc_decode( tmp, len, ccb->ccb_hdr.username, 
				    IIGCc_global->gcc_ns_mask,
				    ccb->ccb_hdr.password );

		    if ( tmp  &&  tmp != tmpbuf )  gcc_free( (PTR)tmp );
		}

		/* 
		** See whether another outbound association can be 
		** accepted.  The criteria are whether we are already 
		** at the maximum # of outbound associations or that 
		** the server is quiescing or closed. 
		*/
		if ( IIGCc_global->gcc_flags & GCC_AT_OBMAX ) 
		{
		    /* We can't accept another association right now */
		    tmp_status = E_GC2215_MAX_OB_CONNS;
		    gcc_er_log(&tmp_status, NULL, NULL, NULL);
		    mde->service_primitive = A_I_REJECT;
		    mde->primitive_type = RQS;
		    mde->mde_generic_status[0] = tmp_status;
		}
		else  if (( IIGCc_global->gcc_flags & GCC_QUIESCE ) || 
			 ( IIGCc_global->gcc_flags & GCC_CLOSED ) ) 
		{
		    /* We can't accept another association right now */
		    tmp_status = E_GC221B_AL_SVR_CLSD;
		    gcc_er_log(&tmp_status, NULL, NULL, NULL);
		    mde->service_primitive = A_I_REJECT;
		    mde->primitive_type = RQS;
		    mde->mde_generic_status[0] = tmp_status;
		}
		else
		{
		    /* See whether we are now at the maximum */
		    if ( IIGCc_global->gcc_ob_conn_ct == 
			 IIGCc_global->gcc_ob_max )
			IIGCc_global->gcc_flags |= GCC_AT_OBMAX;

		    /* 
		    ** Log session startup 
		    */
		    tmp_status = E_GC2211_INCMG_ASSOC;
		    erlist.gcc_parm_cnt = 2;
		    erlist.gcc_erparm[0].size = 
				    STlength( ls_parms->gca_user_name );
		    erlist.gcc_erparm[0].value = ls_parms->gca_user_name;
		    erlist.gcc_erparm[1].size = 
				    STlength( ccb->ccb_hdr.username );
		    erlist.gcc_erparm[1].value = ccb->ccb_hdr.username;
		    gcc_er_log( &tmp_status, NULL, mde, &erlist );
		}
	    }

	    /*
	    ** Free password resources allocated during processing.
	    */
	    if ( password  &&  password != pwdbuf )  
		gcc_free( (PTR)password );

	    if (status != OK)
	    {
		/*
		** The listen processing has failed.  Log out the failure
		** and set the primitive to A_I_REJECT request.
		*/

		tmp_status = E_GC2216_GCA_LSN_FAIL;
		gcc_er_log(&tmp_status,
			   (CL_ERR_DESC *)NULL,
			   (GCC_MDE *)NULL,
			   (GCC_ERLIST *)NULL);
		mde->service_primitive = A_I_REJECT;
		mde->primitive_type = RQS;
                mde->mde_generic_status[0] = tmp_status;
	    } /* end if */
	} /* end if */
	else
	{
	    /*
	    ** The listen has failed.  Log out the failure 
	    ** and set the primitive to A_LSNCLNUP request.
	    **
	    ** Internal GCA listen connections are not logged,
	    ** but they might result in a change of registry
	    ** state.
	    */
	    if ( ls_parms->gca_status != E_GC0032_NO_PEER )
		gcc_er_log( &ls_parms->gca_status, 
			    &ls_parms->gca_os_status, mde, NULL );
	    else  if ( IIGCc_global->gcc_reg_flags & GCC_REG_STARTUP )
	    {
		STATUS		status;
		CL_ERR_DESC	clerr;

		MEfill( sizeof( clerr ), 0, (PTR)&clerr );
		IIGCc_global->gcc_reg_flags &= ~GCC_REG_STARTUP;
		gcc_init_registry( &status, &clerr );
	    }

	    mde->service_primitive = A_LSNCLNUP;
	    mde->primitive_type = RQS;
	} /* end else */				      

	break;
    } /* end case GCC_A_LISTEN */
    case GCC_A_REQUEST:
    {
	GCA_RQ_PARMS	    *rqx_parms =
				(GCA_RQ_PARMS *) &mde->mde_act_parms.gca_parms;

        /* check to make sure request has completed */
        if (rqx_parms->gca_status == E_GCFFFE_INCOMPLETE)
	{
# ifdef GCXDEBUG
            if( IIGCc_global->gcc_trace_level >= 1 )
            {
   	        TRdisplay( "gcc_gca_exit: resuming GCA_REQUEST operation...\n");
            }
# endif /* GCXDEBUG */

            /* call again... */
            call_status = gca_call( &gca_cb, GCA_REQUEST, 
				     (GCA_PARMLIST *)rqx_parms,
				     (i4) GCA_ASYNC_FLAG|GCA_RESUME,
                                                            /* Asynchronous */
				     (PTR)mde,    	    /* async id */
				     (i4) -1,	    /* no timeout */
				     &status);
	    /*
	    ** If there was a failure on the GCA_REQUEST resumption, log
	    ** an error and abort the association.
	    */

	    if (call_status != OK)
	    {
		generic_status = status;
	        gcc_al_abort(mde, ccb, &generic_status, (CL_ERR_DESC *) NULL,
			     (GCC_ERLIST *) NULL);
	    }
# ifdef GCXDEBUG
	    if( IIGCc_global->gcc_trace_level >= 1 )
		TRdisplay( "----- DOWN-END\n" );
# endif /* GCXDEBUG */
            return;
        }

        /* 
        ** Store the assoc id in the CCB regardless of whether there's an 
        ** error. We need to do this 'cos even on the error case we 
        ** subsequently do a disassociate on the association.
        */
        ccb->ccb_aae.assoc_id = rqx_parms->gca_assoc_id;

	if (rqx_parms->gca_status == OK)
	{
	    /*
	    ** Fill in relevant AL elements of the CCB.
	    */

	    ccb->ccb_aae.buf_size = rqx_parms->gca_size_advise;
            ccb->ccb_hdr.gca_protocol = rqx_parms->gca_peer_protocol;
	    /*
	    ** Everything is OK. Mark that the request was accepted and
	    ** build the user data associated with an accepted request.
	    ** NOTE: No user data was sent prior to AL version 6.3.
	    */

	    mde->mde_srv_parms.al_parms.as_result = AARQ_ACCEPTED;
	    mde->p1 = mde->p2 = mde->papp;
	    if ((ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_61) &&
		(ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_62))
	    {
		tmp_status = OK;
		mde->p2 += GCaddi2(&rqx_parms->gca_peer_protocol, mde->p2);
		mde->p2 += GCnaddnat(&tmp_status, mde->p2, &status);
	    }
	} /* end if */
	else
	{
	    /*
	    ** The GCA_REQUEST has failed.  Fill in the status elements
	    ** returned from the GCA service request in the user data area of
	    ** the A_ASSOCIATE SDU, and set the result parm to REJECTED.  Log
	    ** out the failure. NOTE: The user data sent back at versions >= 6.3
            ** of the application layer is different than that used at versions
            ** prior to it.
	    */

	    gcc_er_log( &rqx_parms->gca_status,
			&rqx_parms->gca_os_status,
			mde,
			(GCC_ERLIST *)NULL );
	    if (rqx_parms->gca_status == E_GC000B_RMT_LOGIN_FAIL)
	    {
		int		errcode = E_GC000E_RMT_LOGIN_FAIL_INFO;
		GCC_ERLIST	erlist;
		char		*node_id = mde->ccb->ccb_hdr.trg_addr.node_id;

		erlist.gcc_parm_cnt = 2;
		erlist.gcc_erparm[0].value = node_id;
		erlist.gcc_erparm[0].size = STlength(node_id);
		erlist.gcc_erparm[1].value = rqx_parms->gca_user_name;
		erlist.gcc_erparm[1].size = STlength(rqx_parms->gca_user_name);
		gcc_er_log( &errcode,
			    &rqx_parms->gca_os_status,
			    mde,
			    &erlist );
	    }

	    mde->mde_srv_parms.al_parms.as_result = AARQ_REJECTED;
	    mde->p1 = mde->p2 = mde->papp;
            if ((ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_61) &&
                (ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_62))
            {
		mde->p2 += GCaddi2(&rqx_parms->gca_peer_protocol, mde->p2);
	    }
	    
            mde->p2 += GCnaddnat(&rqx_parms->gca_status, mde->p2, &status);
	} /* end else */

	break;
    } /* end case GCC_A_REQUEST */
    case GCC_A_RESPONSE:
    {
        GCA_RR_PARMS        *rr_parms =
                                (GCA_RR_PARMS *) &mde->mde_act_parms.gca_parms;

        if (rr_parms->gca_status != OK)
        {
            /* The GCA_RQRESP failed. Construct an A_ABORT SDU in the MDE for
            ** the failed request. Fill in the status elements returned from
            ** the GCA service request in the user data area of the SDU.
            */

	    mde->service_primitive = A_ABORT;
	    mde->primitive_type = RQS;
	    mde->mde_generic_status[0] = E_GC2217_GCA_RQRESP_FAIL; 
	    mde->mde_generic_status[1] = rr_parms->gca_status;
        }

        break;
    } /* end case GCC_A_RESPONSE */
    case GCC_A_SEND:
    case GCC_A_E_SEND:
    {
	GCA_SD_PARMS	    *sdx_parms =
				(GCA_SD_PARMS *) &mde->mde_act_parms.gca_parms;

	if ((sdx_parms->gca_status != OK) &&
	    (sdx_parms->gca_status != E_GC0022_NOT_IACK))
	{
	    /*
	    ** The GCA_SEND has failed.  Construct an A_ABORT SDU
	    ** in the MDE for the failed request.  Fill in the status
	    ** elements returned from the GCA service request in the
	    ** user data area of the A_ABORT SDU.
	    */

	    mde->service_primitive = A_ABORT;
	    mde->primitive_type = RQS;
	    mde->mde_generic_status[0] = E_GC2209_SND_FAIL;
	    mde->mde_generic_status[1] = sdx_parms->gca_status;
	} /* end if */
	break;
    } /* end case */
    case GCC_A_NRM_RCV:
    {
	GCA_RV_PARMS	    *rvnx_parms =
				(GCA_RV_PARMS *) &mde->mde_act_parms.gca_parms;

	if (rvnx_parms->gca_status == OK)
	{
	    mde->mde_srv_parms.al_parms.as_msg_type =
		    rvnx_parms->gca_message_type;
	    mde->mde_srv_parms.al_parms.as_flags = 0;

	    /* Set the end of data flag */
	    if (rvnx_parms->gca_end_of_data)
	    {
		mde->mde_srv_parms.al_parms.as_flags |= AS_EOD;

		/* 
		** We only set the end of group flag if this is the
		** last segment (ie end of data is true)
		*/
		if (rvnx_parms->gca_end_of_group)
		    mde->mde_srv_parms.al_parms.as_flags |= AS_EOG;
	    }

	    mde->p2 = mde->p1 + rvnx_parms->gca_d_length;

# ifdef GCXDEBUG
if( IIGCc_global->gcc_trace_level >= 2 )
    TRdisplay( "gcc_gca_exit: received msg %s length %d flags %x\n", 
		gcx_look( gcx_getGCAMsgMap(), 
			  mde->mde_srv_parms.al_parms.as_msg_type ),
		(i4)(mde->p2 - mde->p1), 
		mde->mde_srv_parms.al_parms.as_flags );
# endif /* GCXDEBUG */

	    /*
	    ** Check for a GCA_RELEASE message.  This is necessary
	    ** because there is no other way of ascertaining that the
	    ** sending partner is terminating the session.
	    */

	    if (rvnx_parms->gca_message_type == GCA_RELEASE)
	    {
		mde->service_primitive = A_RELEASE;
	    }
	} /* end if */
	else
	{
	    /*
	    ** The GCA_RECEIVE has failed.  If the status code is
	    ** E_GC0027_RQST_PURGED, the receive has failed because interrupt
	    ** purging is in process. The primitive is set to
	    ** A_PURGED to so indicate.  For any other failure, construct an
	    ** A_ABORT SDU in the MDE for the failed request.  Fill in the
	    ** status elements returned from the GCA service request in the
	    ** user data area of the A_ABORT SDU.
	    */

	    if (rvnx_parms->gca_status == E_GC0027_RQST_PURGED)
	    {
		mde->service_primitive = A_PURGED;
		mde->primitive_type = RQS;
	    }
	    else
	    {
		mde->service_primitive = A_ABORT;
		mde->primitive_type = RQS;
		mde->mde_generic_status[0] = E_GC220A_NRM_RCV_FAIL;
		mde->mde_generic_status[1] = rvnx_parms->gca_status;
	    } /* end else */
	} /* end else */
	break;
    } /* end case GCC_A_NRM_RCV */
    case GCC_A_EXP_RCV:
    {
	GCA_RV_PARMS	    *rvex_parms =
				(GCA_RV_PARMS *) &mde->mde_act_parms.gca_parms;

	if (rvex_parms->gca_status == OK)
	{
	    /*
	    ** Call GCA_INTERPRET to get message type, continuation indication
	    ** and length.  Pass the first 2 in the  AL primitive service parms.
	    ** Set the user data length in the MDE header.
	    */

	    mde->mde_srv_parms.al_parms.as_msg_type = 
		    rvex_parms->gca_message_type;
	    mde->mde_srv_parms.al_parms.as_flags = 0;
	    
	    /* Set the end of data flag */
	    if (rvex_parms->gca_end_of_data)
		mde->mde_srv_parms.al_parms.as_flags |= AS_EOD;

	    /* Always set the end of group flag for expedited data */
	    mde->mde_srv_parms.al_parms.as_flags |= AS_EOG;

	    mde->p2 = mde->p1 + rvex_parms->gca_d_length;

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 2 )
	TRdisplay( "gcc_gca_exit: received msg %s length %d flags %x\n", 
		    gcx_look( gcx_getGCAMsgMap(), 
			      mde->mde_srv_parms.al_parms.as_msg_type ),
		    (i4)(mde->p2 - mde->p1), 
		    mde->mde_srv_parms.al_parms.as_flags );
# endif /* GCXDEBUG */

	} /* end if */
	else
	{
	    /*
	    ** The GCA_RECEIVE has failed. Construct an
	    ** A_ABORT SDU in the MDE for the failed request.  Fill in the
	    ** status elements returned from the GCA service request in the
	    ** user data area of the A_ABORT SDU.
	    */
  	    mde->service_primitive = A_ABORT;
	    mde->primitive_type = RQS;
	    mde->mde_generic_status[0] = E_GC220B_EXP_RCV_FAIL;
	    mde->mde_generic_status[1] = rvex_parms->gca_status;
	} /* end else */
	break;
    } /* end case GCC_A_EXP_RCV */
    case GCC_A_DISASSOC:
    {
	GCA_DA_PARMS	*da_parms =
                             (GCA_DA_PARMS *) &mde->mde_act_parms.gca_parms;
    	GCC_MDE         *lsn_mde;
    	GCC_CCB	    	*lsn_ccb;

        /* check to make sure request has completed */
        if (da_parms->gca_status == E_GCFFFE_INCOMPLETE)
	{
# ifdef GCXDEBUG
            if( IIGCc_global->gcc_trace_level >= 1 )
            {
   	        TRdisplay( "gcc_gca_exit: resuming GCA_DISASSOC operation...\n");
            }
# endif /* GCXDEBUG */

            /* call again... */
            call_status = gca_call( &gca_cb, GCA_DISASSOC,
				     (GCA_PARMLIST *)da_parms,
				     (i4) GCA_ASYNC_FLAG|GCA_RESUME,
                                                            /* Asynchronous */
				     (PTR)mde,    	    /* async id */
				     (i4) GCC_TIMEOUT,
				     &status);
	    /*
	    ** If there was a failure on the GCA_DISASSOC resumption, log
	    ** an error and abort the association.
	    */

	    if (call_status != OK)
	    {
		generic_status = status;
	        gcc_al_abort(mde, ccb, &generic_status, (CL_ERR_DESC *) NULL,
			     (GCC_ERLIST *) NULL);
	    }
# ifdef GCXDEBUG
	    if( IIGCc_global->gcc_trace_level >= 1 )
		TRdisplay( "----- DOWN-END\n" );
# endif /* GCXDEBUG */
            return;
        }

        if (da_parms->gca_status != OK)
        {
            /* The GCA_DISASSOC failed. There's not much we can do about this
            ** other than log the error and carry on as though nothing had
            ** happened...
            */

	    tmp_status = E_GC2218_GCA_DISASSOC_FAIL;
	    gcc_er_log(&tmp_status,
		       (CL_ERR_DESC *)NULL,
		       mde,
		       (GCC_ERLIST *)NULL);
	    gcc_er_log(&da_parms->gca_status,
		       &da_parms->gca_os_status,
		       mde,
		       (GCC_ERLIST *)NULL);
        }
        else
	{

    	    /* Log session completion */
	
	    tmp_status = E_GC2201_ASSOC_END;
	    gcc_er_log(&tmp_status,
		       (CL_ERR_DESC *)NULL,
		       mde,
		       (GCC_ERLIST *)NULL);
	}

	/*
    	** Drop the conn count if this was an outgoing association.
    	*/

	if( ccb->ccb_hdr.flags & CCB_OB_CONN )
	{
	    IIGCc_global->gcc_conn_ct--;
	    IIGCc_global->gcc_ob_conn_ct--;

    	    if(IIGCc_global->gcc_flags & GCC_AT_OBMAX)
		if (IIGCc_global->gcc_ob_conn_ct < IIGCc_global->gcc_ob_max)
	    	    IIGCc_global->gcc_flags &= ~GCC_AT_OBMAX;
        } 

	/*
	** Repost the GCA_LISTEN even if OBMAX is reached or if it is
	** an outgoing connection.
	*/
	if( (IIGCc_global->gcc_flags & GCC_GCA_LSTN_NEEDED) &&
	     !(ccb->ccb_hdr.flags & CCB_IB_CONN) )

	{
	    IIGCc_global->gcc_flags &= ~GCC_GCA_LSTN_NEEDED;
		    
    	    /*
    	    ** Invoke the AL FSM to establish another outstanding 
    	    ** GCA_LISTEN.
    	    */
    	    lsn_mde = gcc_add_mde( ( GCC_CCB * ) NULL, 0 );
    	    lsn_ccb = gcc_add_ccb();
    	    if ((lsn_mde == NULL) || (lsn_ccb == NULL))
    	    {
 	       	/* Log an error and abort the association */
	   	generic_status = E_GC2004_ALLOCN_FAIL;
	       	gcc_al_abort(lsn_mde, lsn_ccb, &generic_status, 
    	    	    	     (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL);
    	    } /* end if */
    	    else
	    {
    	    	lsn_mde->ccb = lsn_ccb;
    	    	lsn_mde->service_primitive = A_LISTEN;
    	    	lsn_mde->primitive_type = RQS;
    	    	lsn_ccb->ccb_aae.a_state = SACLD;
    	    	(VOID) gcc_al(lsn_mde);
	    } /* end of else */
	}

        break;
    } /* end case GCC_A_DISASSOC */
    default:
	{
	    /*
	    ** Unrecognized GCA operation.  This is a serious program bug, of
	    ** the type that normally occurs only in early development.  However
	    ** if it ever did happen, there's really nothing to be done about
	    ** it.
	    ** All we can do is log it out and abort the association.
	    */

	    mde->service_primitive = A_ABORT;
	    mde->primitive_type = RQS;
	    mde->mde_generic_status[0] = E_GC2210_LCL_ABORT;
	    mde->mde_generic_status[1] = E_GC220F_AL_INTERNAL_ERROR;
	    break;
	} /* end default */
    }/* end switch */

    /*
    ** Send the MDE to AL.
    */

    (VOID) gcc_al(mde);
# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "----- DOWN-END\n" );
# endif /* GCXDEBUG */
    return;
} /* end gcc_gca_exit */

/*{
** Name: gcc_al_evinp	- AL FSM input event id evaluation routine
**
** Description:
**      gcc_al_evinp accepts as input an MDE received by the AL.  It  
**      evaluates the input event based on the MDE type and possibly on  
**      some other status information.  It returns as its function value the  
**      ID of the input event to be used by the FSM engine. 
** 
** Inputs:
**      mde			Pointer to dequeued Message Data Element
**
** Outputs:
**      generic_status		Mainline status code.
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
**      31-Jul-89 (cmorris)
**          Don't strip off PCI on P_CONNECT (reject) and P_U_ABORT 
**          events. It was poor programming practice anyway!!
**      18-Aug-89 (cmorris)
**          If we fail to evaluate the input event, return an illegal event
**          rather than 0 (which IS legal!).
**      02-Nov-89 (cmorris)
**          Removed processing for IAPCIQ and IAPCIM input events.
**      08-Nov-89 (cmorris)
**          Removed events IAPAPS and IAPAUS.
**      11-Nov-89 (cmorris)
**          Added event IAAFN.
**	07-Mar-90 (cmorris)
**	    Added handling of A_P_ABORT.
**  	04-Jan-91 (cmorris)
**  	    Added test for IAAPGX. Previously it was never generated!!
**  	06-Oct-92 (cmorris)
**  	    Removed IACMD input event.
**      19-Jan-93 (cmorris)
**  	    Got rid of system_status argument:- never used.
**	 4-Nov-05 (gordy)
**	    Added input IAAAF for FASTSELECT listen completion.
**	    Removed input IAACEF as response/send completions
**	    are now controlled by state and standard inputs.
*/
static u_i4
gcc_al_evinp( GCC_MDE *mde, STATUS *generic_status )
{
    u_i4		input_event;

    /* do tracing... */
    GCX_TRACER_3_MACRO("gcc_al_evinp>", mde,
	"primitive = %d/%d, size = %d",
	    mde->service_primitive,
	    mde->primitive_type,
	    (i4)(mde->p2 - mde->p1) );

    switch (mde->service_primitive)
    {
    case A_LISTEN:
    {
	/* Initial event */
	input_event = IALSN;
	break;
    }
    case A_ASSOCIATE:
    {
	if ( mde->primitive_type == RQS )
	{
	    if ( mde->ccb->ccb_aae.a_flags & GCCAL_FSEL )
		input_event = IAAAF;	/* A_ASSOCIATE FASTSELECT request */
	    else
		input_event = IAAAQ;	/* A_ASSOCIATE request */
	}
	else  if ( mde->mde_srv_parms.al_parms.as_result == AARQ_ACCEPTED )
	    input_event = IAASA;	/* A_ASSOCIATE response/ACCEPT */
	else
	    input_event = IAASJ;	/* A_ASSOCIATE response/REJECT */		
	break;
    }  /* end case A_ASSOCIATE */
    case A_SHUT:
    {
	input_event = IASHT;
	break;
    }
    case A_QUIESCE:
    {
	input_event = IAQSC;
	break;
    }
    case A_CMDSESS:
    {
	input_event = IACMD;
	break;
    }
    case A_ABORT:
    {
	if ( mde->ccb->ccb_aae.a_flags & GCCAL_SXOF )
	    input_event = IAABQX;
	else  if ( mde->ccb->ccb_aae.a_flags & GCCAL_RGRLS )
	    input_event = IAABQR;
	else
	    input_event = IAABQ;
	break;
    }
    case A_LSNCLNUP:
    {
	/*
	** The local listen has failed.  
	*/

	input_event = IALSF;
	break;
    }
    case A_I_REJECT:
    {
	/*
	** The IAL listen processing has failed.
	*/

	input_event = IAREJI;
	break;
    }
    case A_R_REJECT:
    {
	/* The RAL request processing has failed */

	input_event = IAREJR;
	break;
    }
    case A_P_ABORT:
    {
	input_event = IAABT;
	break;
    }
    case P_CONNECT:
    {
	if (mde->primitive_type == IND)
	{
	    /* P_CONNECT indication */
	    mde->ccb->ccb_hdr.async_cnt += 1;    
            input_event = IAPCI;
	} /* end if */
	else
	{
	    /* P_CONNECT confirm */
	    if (mde->mde_srv_parms.pl_parms.ps_result == PS_ACCEPT)
	    {
		/* P_CONNECT confirm/ACCEPT */
		input_event = IAPCA;
	    } /* end if */
	    else
	    {
		/* P_CONNECT confirm/REJECT */
		input_event = IAPCJ;
		break;
	    } /* end else */
	} /* end else */

	/* 
	** Valid P_CONNECT, check for heterogeneous and if so, note that
	** GOTOHET message is to be sent (preceding first GCA_SEND).
	*/
	if (mde->ccb->ccb_pce.het_mask != GCC_HOMOGENEOUS)
	{
	    mde->ccb->ccb_aae.a_flags |= GCCAL_GOTOHET;
	}
	break;
    }  /* end case P_CONNECT */
    case P_U_ABORT:
    {
        /* P_U_ABORT indication */
    	if (mde->ccb->ccb_aae.a_flags & GCCAL_GCM)
	    input_event = IAPAM;
	else
	    input_event = IAPAU;
	break;
    }
    case P_P_ABORT:
    {
	/* P_P_ABORT indication */
    	if (mde->ccb->ccb_aae.a_flags & GCCAL_GCM)
	    input_event = IAPAM;
	else
            input_event = IAPAP;
	break;
    }
    case A_DATA:
    {
	/* A_DATA has only a request form */
	
	if (mde->ccb->ccb_aae.a_flags & GCCAL_RXOF)
	{
	    input_event = IAANQX;
	}
	else
	{
	    input_event = IAANQ;
	}
	break;
    }
    case A_EXP_DATA:
    {
	/* A_EXP_DATA has only a request form */
	
	input_event = IAAEQ;
	break;
    }
    case A_PURGED:
    {
	if (mde->ccb->ccb_aae.a_flags & GCCAL_RXOF)
	{
	    input_event = IAAPGX; 
	}
	else
	{
	    input_event = IAAPG;
	}
	break;
    }
    case A_COMPLETE:
    {
	if ( mde->ccb->ccb_aae.a_rcv_q.q_next != &mde->ccb->ccb_aae.a_rcv_q )
	    input_event = IAACQ;
	else  if ( mde->ccb->ccb_aae.a_flags & GCCAL_SXOF )
	    input_event = IAACEX;
	else
	    input_event = IAACE;
	break;
    }  /* end case A_COMPLETE */
    case P_DATA:
    {
	/* P_DATA indication */
	if (mde->ccb->ccb_aae.a_flags & GCCAL_GOTOHET)
	{
	    /* We need to send the GCA_GOTOHET message. NOTE: when we stop
            ** supporting api version 0 (ie pre end-to-end bind) all this
            ** goto het nonsense can safely be garbaged...
            */

	    input_event = IAPDNH;
	    /* ... But never again */
	    mde->ccb->ccb_aae.a_flags &= ~GCCAL_GOTOHET;
	}
	else
	{
	    if ((mde->ccb->ccb_aae.a_rq_cnt >= GCCAL_RQMAX) &&
		!(mde->ccb->ccb_aae.a_flags & GCCAL_SXOF))
	    {
		input_event = IAPDNQ;
	    }
	    else
	    {
		input_event = IAPDN;
	    }
	}
	if (mde->mde_srv_parms.al_parms.as_msg_type == GCA_RELEASE)
	{
	    mde->ccb->ccb_aae.a_flags |= GCCAL_RGRLS;
	}
	break;
    }
    case P_EXP_DATA:
    {
	/* P_EXP_DATA indication */

	input_event = IAPDE;
	break;
    }
    case A_RELEASE:
    {
	input_event = IAARQ;
	break;
    }
    case P_RELEASE:
    {
	if (mde->primitive_type == IND)
	{
	    /* P_RELEASE indication - see whether initiator or responder */
	    if (mde->ccb->ccb_hdr.flags & CCB_OB_CONN)
	        input_event = IAPRII;
	    else
		input_event = IAPRIR;
	} /* end if */
	else
	{
	    /* P_RELEASE confirm */
	    input_event = IAPRA;
	} /* end else */
	break;
    }  /* end case P_RELEASE */
    case P_XOFF:
    {
	/* P_XOFF has only a indication form */
	
	input_event = IAXOF;
	break;
    } /* end case P_XOFF */
    case P_XON:
    {
	/* P_XON has only a indication form */
	
	input_event = IAXON;
	break;
    } /* end case P_XON */
    case A_FINISH:
    {
        /* A_FINISH has only a request form */

        input_event = IAAFN;
        break;
    } /* end case A_FINISH */
    default:
    {
	input_event = IAMAX;  /* an illegal event ... */
	*generic_status = E_GC2202_AL_FSM_INP;
	break;
    }
    }  /* end switch */
    return(input_event);
} /* end gcc_al_evinp */

/*{
** Name: gcc_alout_exec	- AL FSM output event execution routine
**
** Description:
**	gcc_alout_exec accepts as input an output event identifier.  It
**	executes this output event and returns.
**
**	It also accepts as input a pointer to an MDE or NULL.  If the pointer
**	is non-null, the MDE pointed to is used for the output event.
**	Otherwise, a new one is obtained.
** 
** Inputs:
**	output_event_id		Identifier of the output event
**      mde			Pointer to dequeued Message Data Element
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
**      14-Dec-88 (jbowers)
**	    Convert Application Entity Title (DB name) to & from Xfer syntax.
**      22-Feb-89 (jbowers)
**          Add support for outbound session limitations: add output event
**	    OAIMX.
**      21-Mar-89 (jbowers)
**	    Added error logout when inbound or outbound connections are
**	    refused.
**      31-Jul-89 (cmorris)
**          Added setting of AL protocol version in P_CONNECT (accept) and
**          P_CONNECT (reject) cases.
**      05-Oct-89 (cmorris)
**          Cleaned up byte alignment code.
**      25-Oct-89 (cmorris)
**          Implemented calls to GCnxxx functions to convert individual types
**          to network xfer syntax.
**      14-Nov-89 (cmorris)
**          Initialize status before first call to GCnxxx functions.
**      05-Mar-90 (cmorris)
**	    Pass back status from calling another layer; this prevents
**	    us ploughing on with events and actions oblvious to the fact
**	    that we've already hit a fatal error.
**	10-May-91 (cmorris)
**	    When we confirm the a P_RELEASE, make sure it's a P_RELEASE(accept)
**  	08-Dec-92 (cmorris)
**  	    When allocating an MDE, use local variable to store its address
**  	    rather than the mde argument to the function.
**  	19-Jan-93 (cmorris)
**  	    Got rid of layer id as all events go to only one layer - PL!
**  	25-Jan-93 (cmorris)
**  	    Use the right size for the A-Associate response pci!
**      16-Mar-94 (cmorris)
**  	    Got rid of system_status argument:- never used.
**	13-Jul-00 (gordy)
**	    Logging of association aborts was removed because of timing
**	    problems (expected aborts during disconnect were being logged).
**	    The problem was that logging was being done in gcc_gca_exit()
**	    before the correct state was established in the AL FSM.  Move
**	    logging to OAPAU for A_ABORT and A_P_ABORT.  OAPAU is not
**	    executed during shutdown so only real aborts will be logged.
**	21-Jul-09 (gordy)
**	    Target is dynamically allocated.
*/

static STATUS
gcc_alout_exec( i4  output_event_id, GCC_MDE *mde, GCC_CCB *ccb )
{
    STATUS	status;
    STATUS	call_status = OK;

    GCX_TRACER_1_MACRO("gcc_alout_exec>", mde,
	    "output_event = %d", output_event_id);

    switch (output_event_id)
    {
    case OAPCQ:	/* P_CONNECT request */
    {
	STATUS		status = OK;
	i4		str_len;
	char		*pci;

	/*
	** Send a P_CONNECT request to PL.  The following sequence occurs in
	** its construction:
	**
	** First, an AARQ APDU is constructed.  The AARQ PCI is moved in.  Then
	** the P_CONNECT service primitive is constructed.
	*/

	/*
	** Construct the AARQ APDU.
	** 
	** Set local pointer to start of AL PCI area, and set the offset in the
	** MDE.
	*/

	pci = ( mde->p1 -= SZ_AARQ_PCI );

        /*
        ** Set the application protocol version number in the PCI. We fetch
        ** this out of the ccb.
        */

	pci += GCaddn2(ccb->ccb_aae.a_version, pci);

	/*
	** Insert the Application Entity Title: this is the Partner ID string
	** passed by the initiating client in its GCA_REQUEST.  This must be
	** transformed to transfer syntax, since it is not known at this time
	** what the target local syntax is: same or different.  This is a bit 
        ** of a hack, actually, but it is simplest to do it here.
	*/

	str_len = ccb->ccb_hdr.target ? STlength( ccb->ccb_hdr.target ) : 0;
	str_len = min( str_len, GCC_L_AE_TITLE );
	pci += GCaddn2( str_len, pci );
        if ( str_len )  GCnadds( ccb->ccb_hdr.target, str_len, pci );

	/*
	** Fill in the primitive indicators.  The remaining elements of the
	** P_CONNECT request SDU are already filled in from the returned parms
	** from GCA_LISTEN, by gcc_gca_exit.
	*/

	mde->service_primitive = P_CONNECT;
	mde->primitive_type = RQS;

	/*
	** Send the SDU to PL 
	*/

	break;
    } 
    case OAPCA:	/* P_CONNECT response/ACCEPT */
    {
	char		*pci;

	/*
	** Send a P_CONNECT response/ACCEPT to PL.
	**
	** First, an AARE APDU is constructed.  The AARE PCI is moved in.  Then
	** the P_CONNECT service primitive is constructed.
	*/

	/*
	** Construct the AARE APDU.
	** 
	** Set local pointer to start of AL PCI area, and set the offset in the
	** MDE.
	*/

	pci = ( mde->p1 -= SZ_AARE_PCI );

	/* 
        ** Insert the AL protocol version and the result in the AARE APDU.
        ** The AL protocol version is that which was set up in the ccb when
        ** the association request was originally received.
        */

	pci += GCaddn2(ccb->ccb_aae.a_version, pci);
	pci += GCaddn2(AARQ_ACCEPTED, pci);

	/* Fill in the P_CONNECT response SDU */

	mde->service_primitive = P_CONNECT;
	mde->primitive_type = RSP;
	mde->mde_srv_parms.pl_parms.ps_result = PS_ACCEPT;

	/*
	** Send the SDU to PL:
	*/

	break;
    }
    case OAPCJ:	/* P_CONNECT response/REJECT */
    {
	char		*pci;

	/*
	** Send a P_CONNECT response/REJECT to PL.  
	**
	** First, an AARE APDU is constructed.  The AARE PCI is moved in.  Then
	** the P_CONNECT service primitive is constructed.
	*/

	/*
	** Construct the AARE APDU.
	** 
	** Set local pointer to start of AL PCI area, and set the offset in the
	** MDE.
	** Insert the AL protocol version and the result in the AARE APDU.
        ** The AL protocol version is that which was set up in the ccb when
        ** the association request was originally received.
        */

	pci = ( mde->p1 -= SZ_AARE_PCI );

	pci += GCaddn2(ccb->ccb_aae.a_version, pci);
	pci += GCaddn2(AARQ_REJECTED, pci);

	/* Fill in the P_CONNECT response SDU */

	mde->service_primitive = P_CONNECT;
	mde->primitive_type = RSP;
	mde->mde_srv_parms.pl_parms.ps_result = PS_USER_REJECT;

	/*
	** Send the SDU to PL:
	*/

	break;
    }
    case OAPAU:	/* P_U_ABORT request */
    {
	char	*pci;
	int	i;

	/*
	** Log status.
	*/
	for( i = 0; i < GCC_L_GEN_STAT_MDE; i++ )
	    if ( mde->mde_generic_status[i] != OK )
		gcc_er_log( &mde->mde_generic_status[i], NULL, mde, NULL );

	/*
	** Send a P_U_ABORT request to PL.
	*/
	mde->p1 = mde->p2 = mde->papp;

	/*
	** Send text for most specific status as the ABORT data.
	*/
	for( i = GCC_L_GEN_STAT_MDE; i-- > 0; )
	    if ( mde->mde_generic_status[i] != OK )
	    {
		gcc_aaerr( mde, ccb, mde->mde_generic_status[1] );
		break;
	    }

	/*
	** Construct the ABRT APDU.
	*/
	pci = ( mde->p1 -= SZ_ABRT_PCI );
	pci += GCaddn2(ABRT_REQUESTOR, pci);

	mde->service_primitive = P_U_ABORT;
	mde->primitive_type = RQS;
	break;
    }
    case OAPDQ:	/* P_DATA request */
    {
	/*
	** Send a P_DATA request to PL.
	**
	** This results from an A_DATA or an A_RELEASE request primitive.  The
	** message type is already in the service parms.
	*/

	mde->service_primitive = P_DATA;
	mde->primitive_type = RQS;

	/*
	** Send the SDU to PL:
	*/

	break;
    }
    case OAPEQ:	/* P_EXP_DATA request */
    {
	/*
	** Send a P_EXP_DATA request to PL.
	**
	** This results from an A_EXP_DATA request primitive.  The
	** message type is already in the service parms.
	*/

	mde->service_primitive = P_EXP_DATA;
	mde->primitive_type = RQS;

	/*
	** Send the SDU to PL:
	*/

	break;
    }
    case OAPRQ:	/* P_RELEASE request */
    {
	GCC_MDE		*prq_mde;
	char		*pci;

	/*
	** Send a P_RELEASE request to PL.  This results from an A_RELEASE
	** request primitive.
	*/

	/*
	** Construct an RLRQ APDU to be sent to the peer AL.
	** 
	** Insert the reason in the RLRQ APDU.
	*/

        prq_mde = gcc_add_mde( ccb, 0 );
	if (prq_mde == NULL)
	{
	    /* Log an error and abort the association */
	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_al_abort(prq_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    return(FAIL);
	} /* end if */
	prq_mde->ccb = ccb;

	pci = ( prq_mde->p1 -= SZ_RLRQ_PCI );

	pci += GCaddn2(RLRQ_NORMAL, pci);

	/*	
	** Fill in the P_RELEASE request SDU.
	*/

	prq_mde->service_primitive = P_RELEASE;
	prq_mde->primitive_type = RQS;

	/*
	** Send the SDU to PL
	*/

	mde = prq_mde;
	break;
    }
    case OAPRS:	/* P_RELEASE response */
    {
	char		*pci;

	/*
	** Send a P_RELEASE response to PL.
	** 
	** An RLRE APDU is constructed.  The RLRE PCI is moved in.
	** Then the P_RELEASE service primitive is constructed.
	*/

	pci = ( mde->p1 -= SZ_RLRE_PCI );

	pci += GCaddn2(RLRE_NORMAL, pci);

	/*	
	** Fill in the P_RELEASE response SDU.
	*/

	mde->service_primitive = P_RELEASE;
	mde->primitive_type = RSP;
	mde->mde_srv_parms.pl_parms.ps_result = PS_AFFIRMATIVE;
	/*
	** Send the SDU to PL
	*/

	break;
    }
    case OAXOF: /* P_XOFF request */
    {
    	GCC_MDE	    *xof_mde;

	/*
	** Send a P_XOFF request to PL.  
	** Set the "Sent XOF" indicator in the GCC AL flags.
	*/

	if( !( xof_mde = gcc_add_mde( ccb, 0 ) ) )
	{
	    /* Log an error and abort the association */
	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_al_abort(xof_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    return(FAIL);
	}

	xof_mde->ccb = ccb;
	xof_mde->service_primitive = P_XOFF;
	xof_mde->primitive_type = RQS;
	ccb->ccb_aae.a_flags |= GCCAL_SXOF;

	/*
	** Send the SDU to PL
	*/

    	mde = xof_mde;
	break;
    } /* end case OAXOF */
    case OAXON: /* P_XON request */
    {
    	GCC_MDE	    *xon_mde;

	/*
	** Send a P_XON request to PL.  
	** Reset the "Sent XOF" indicator in the GCC AL flags.
	*/

	if( !( xon_mde = gcc_add_mde( ccb, 0 ) ) )
	{
	    /* Log an error and abort the association */
	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_al_abort(xon_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    return(FAIL);
	}

	xon_mde->ccb = ccb;
	xon_mde->service_primitive = P_XON;
	xon_mde->primitive_type = RQS;
	ccb->ccb_aae.a_flags &= ~GCCAL_SXOF;

	/*
	** Send the SDU to PL
	*/

    	mde = xon_mde;
	break;
    } /* end case OAXON */
    default:
	{
	    /*
	    ** Unknown output event ID.  This is a program bug.
	    ** Log an error and abort the association.
	    */

	    status = E_GC220F_AL_INTERNAL_ERROR;
	    gcc_al_abort(mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    return(FAIL);
	}
    } /* end switch */

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 2 )
	TRdisplay( "gcc_alout_exec: service_primitive %s %s\n",
		gcx_look( gcx_mde_service, mde->service_primitive ),
                gcx_look( gcx_mde_primitive, mde->primitive_type ));
# endif /* GCXDEBUG */

    call_status = gcc_pl(mde);
    return(call_status);
} /* end gcc_alout_exec */

/*{
** Name: gcc_alactn_exec	- AL FSM action execution routine
**
** Description:
**      gcc_alactn_exec accepts as input an action identifier.  It
**      executes the action correspoinding to the identifier and returns.
**
**	It also accepts as input a pointer to an 
**	MDE or NULL.  If the pointer is non-null, the MDE pointed to is
**	used for the action.  Otherwise, a new one is obtained.
**
** Inputs:
**	action_code		Identifier of the action to be performed
**      mde			Pointer to dequeued Message Data Element
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
**      08-Nov-88 (jbowers)
**          Added check for null password and rejection of request under AARQS.
**      14-Dec-88 (jbowers)
**	    Convert Application Entity Title (DB name) to & from Xfer syntax.
**      16-Jan-89 (jbowers)
**	    Changes to case AARLS: to send both GCA_ATTENTION and GCA_RELEASE to
**	    a server to abnormally terminate a session.
**	14-jan-89 (seiwald)
**	    Some compatability for 6.1.  Use the null in the encoded data
**	    to trim the user name, password, etc. to its original length.
**      25-Jul-89 (cmorris)
**          Added AARSP case to issue a GCA_RQRESP.
**      31-Jul-89 (cmorris)
**          Modified AARQS case to handle differing versions of AL protocol.
**      02-Aug-89 (cmorris)
**          Added AAENQG case to enqueue goto het message; removed AAFSM
**          base - it was not used!
**      16-Aug-89 (cmorris)
**          Set the application layer protocol version number in the ccb 
**          when a ccb is first allocated.
**      25-Aug-89 (cmorris)
**          Don't set l_user_data to 0 before calling gcc_aerr (AARLS). This
**          prevents interpretation of remote error data!
**      05-Sep-89 (cmorris)
**          Negotiate down the requested GCA protocol if its greater than
**          what we support.
**      06-Sep-89 (cmorris)
**          When freeing CCB check async count <= 0 instead of == 0.
**      09-Sep-89 (cmorris)
**          Removed AAREJ case.
**      03-Oct-89 (cmorris)
**          Don't set GCA_DESCR_REQD flag on listen (this was illegal); do
**          set it on AARSP.
**      25-Oct-89 (cmorris)
**          Implemented calls to GCnxxx functions to convert individual types
**          from network xfer syntax.
**      03-Nov-89 (cmorris)
**          Added code to support checking for max inbound connection 
**          exceeded and server quiescing to AARQS case.
**      07-Nov-89 (cmorris)
**          Log an error in AARSP if remote connection request failed.
**          Fixed bug introduced with byte alignment whereby stack storage
**          was being used for parameters being given to GCA_REQUEST call.
**      08-Nov-89 (cmorris)
**          Removed AADCN action. Moved freeing of GCA parm lists from AADIS
**          action to gcc_gca_exit.
**      10-Nov-89 (cmorris)
**          No longer send GCA_ATTENTION messsage to server. Disassociate
**          chnages did away with the need for this.
**      16-Nov-89 (cmorris)
**          Fixed AAQSC to immediately shut down the server if there are no
**          extant connections.
**      17-Nov-89 (cmorris)
**          Check for errors after calls to gcc_get_obj.
**      13-Dec-89 (cmorris)
**          Distinguish between local and remote provider aborts.
**	14-May-91 (cmorris)
**	    Got rid of "Initiating AL" flag:- never used.
**	16-Jul-91 (cmorris)
**	    Return FAIL status if event reintroduced into protocol
**	    machine. This prevents the possibility of further actions 
**          being executed.
**	18-oct-91 (seiwald) bug #40468
**	    Don't return failure after reintroducing an event unless
**	    an abort is generated.
**  	06-Oct-92 (cmorris)
**  	    Removed AACMD and AAIAK actions.
**  	27-Nov-92 (cmorris)
**  	    No longer allocate a ccb and mde in AALSN:- use the one's
**  	    passed in.
**	5-apr-93 (robf)
**	    Add handling for security label on connections
**	 1-Sep-95 (gordy) bug #67475
**	    Adding GCA_RQ_SERVER flags.
**	14-Nov-95 (gordy)
**	    Added support for GCA_API_LEVEL_5 which removes the
**	    need for GCA_INTERPRET on fast select messages.
**	22-Nov-95 (gordy)
**	    Set the gca_formatted flag for send/receive at API level 5.
**	21-Feb-97 (gordy)
**	    Changed gca_formatted flag to gca_modifiers.
**	25-Feb-97 (gordy)
**	    Set end-of-group on GCA_RELEASE message.
**	30-May-97 (gordy)
**	    GCA_RQ_SERVER deprecated with new GCF security handling.
**	 3-Jun-97 (gordy)
**	    Extraction of initial connection info moved to separate
**	    routine (it is now built in the SL).
**	 5-Sep-97 (gordy)
**	    Added new AL protocol level in which the initial connection
**	    info is now processed by the SL.
**	 17-Oct-97(rajus01)
**	    Initialize encryption mode and encryption mechanism.
**	  4-Dec-97(rajus01)
**	    GCA_REQUEST parms now have two new additonal parameters
**	    such as length of remote auth, remote auth. GCA_RQ_AUTH
**	    flag is used to indicate that remote auth is present.
**	29-Dec-97 (gordy)
**	    Save FASTSELECT messages in dynamically allocated buffer
**	    (GCA may step on its buffer before we are ready).
**	31-Mar-98 (gordy)
**	    Added default encryption mechanism.
**	16-mar-2001 (somsa01)
**	    In the case of E_GC000B_RMT_LOGIN_FAIL, log
**	    E_GC000E_RMT_LOGIN_FAIL_INFO as well.
**	20-Aug-02 (gordy)
**	    Check for acceptance/rejection of P_CONNECT request by PL
**	    in AARQS.
**	 6-Jun-03 (gordy)
**	    Remote address info fills rmt_addr, so security host info 
**	    has own storage in CCB.
**	31-Mar-06 (gordy)
**	    Sanity check lengths read from IO buffers.
**	21-Jul-09 (gordy)
**	    Security label stuff removed. Username, password, target 
**	    and encryption mechanism are dynamically allocaated.
*/

static STATUS
gcc_alactn_exec( u_i4 action_code, GCC_MDE *mde, GCC_CCB *ccb )
{
    STATUS		status = OK;
    STATUS		call_status = OK;

    status = OK;

    GCX_TRACER_1_MACRO("gcc_alactn_exec>", mde,
	    "action = %d", action_code);

# ifdef GCXDEBUG
	    if( IIGCc_global->gcc_trace_level >= 1 )
		TRdisplay( "gcc_alactn_exec: action %s\n",
			gcx_look( gcx_gcc_aal, action_code ) );
# endif /* GCXDEBUG */
    switch (action_code)
    {
    case AARQS:	/* GCA_REQUEST */
    {
	AL_AARQ_PCI	    aarq_pci;
	GCA_RQ_PARMS	    *rqo_parms;
	GCC_ERLIST	    erlist;
	char		    *pci;
	i4		    str_len;
	STATUS		    tmp_gnrc_status = OK;
	char		    *tmp_hostname = NULL;

	/*
	** Issue GCA_REQUEST to local server.
	**
	** The MDE carrying a P_CONNECT IND contains, as the user data in the
	** PDU area an AARQ APDU.  The PCI and the user data areas of the AARQ
	** contain the parameters for the GCA_REQUEST.  The MDE becomes an
	** A_ASSOCIATE RSP when the GCA_REQUEST is complete.
	**
	** It is here that the check for PL acceptance, inbound limit and a
	** password is made.  If a check fails, the MDE becomes an A_R_REJECT 
	** request/reject and the FSM is driven to propogate the error 
	** notification back to the origin.
	*/

        /* 
	** Strip out the pci from the mde
	*/
	if ( (mde->p2 - mde->p1) < SZ_AARQ_PCI )
	{
	    tmp_gnrc_status = E_GC220F_AL_INTERNAL_ERROR;
	    mde->service_primitive = A_R_REJECT;
	    mde->primitive_type = RQS;
	    mde->p1 = mde->p2 = mde->papp;

            if ( ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_61  &&
                 ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_62 )
        	mde->p2 += GCaddi2( &ccb->ccb_hdr.gca_protocol, mde->p2 );

            mde->p2 += GCnaddnat( &tmp_gnrc_status, mde->p2, &status );
	    (VOID)gcc_al( mde );
            break;
	}

	pci = mde->p1;
	mde->p1 += SZ_AARQ_PCI;
	pci += GCgeti2(pci, &aarq_pci.prot_vrsn);
	ccb->ccb_aae.a_version = min( aarq_pci.prot_vrsn, AL_PROTOCOL_VRSN );
	pci += GCgeti2(pci, &aarq_pci.l_ae_title);

        /*
	** The Application Entity Title contains the Partner ID string
	** passed by the initiating client in its GCA_REQUEST.  This must
	** be transformed back from transfer syntax and null terminated, 
	** as we specifically did not convert the null.
        */
	ccb->ccb_hdr.target = (char *)gcc_alloc( aarq_pci.l_ae_title );

	if ( ccb->ccb_hdr.target )
	{
            GCngets( pci, aarq_pci.l_ae_title, ccb->ccb_hdr.target );
            ccb->ccb_hdr.target[ aarq_pci.l_ae_title ] = EOS;
	}

	pci += GCC_L_AE_TITLE;
	
	/*
	** Connection user data was originally processed by the AL.
	** At version 6.6 and above, it is now processed by the SL.
	*/
	if ( ccb->ccb_aae.a_version < AL_PROTOCOL_VRSN_66 )
	    gcc_get_conn_info( ccb, mde );

        /* 
        ** First look at the application layer version number from the PCI.
        ** This tells us whether our peer application layer has set the GCA
    	** flags protocol version in the user data part of the request. 
    	** If the application protocol version is < 6.5, no GCA flags will
    	** have been sent. If the application protocol is either version 6.1 
    	** or 6.2, no GCA protocol version will have been set. In those cases, 
    	** we deduce that it must be protocol version 2. Pure hacksville.
        */
    	if ( ccb->ccb_aae.a_version >= AL_PROTOCOL_VRSN_65 )
	{
	    /* Set up GCA flags for association in CCB */
	    if ( ccb->ccb_hdr.gca_flags & GCA_LS_GCM )
	    {
		ccb->ccb_aae.a_flags |= GCCAL_GCM;
		/* Force off ridiculous GOTOHET flag */
    	    	ccb->ccb_aae.a_flags &= ~GCCAL_GOTOHET;
	    }

    	    /* 
    	    ** As we don't currently implement the fast select protocol
    	    ** on the remote side, we ignore the fast select flag right
    	    ** now.
    	    */
    	}	    

	/* 
	** Negotiate down the GCA version 
	** if its more than we support.
	*/
        if ( ccb->ccb_aae.a_version == AL_PROTOCOL_VRSN_61  ||
             ccb->ccb_aae.a_version == AL_PROTOCOL_VRSN_62 )
            ccb->ccb_hdr.gca_protocol = GCA_PROTOCOL_LEVEL_2;
	else  if ( ccb->ccb_hdr.gca_protocol > GCC_GCA_PROTOCOL_LEVEL )
            ccb->ccb_hdr.gca_protocol = GCC_GCA_PROTOCOL_LEVEL;

	/*
	** Check for PL acceptance.
	*/
	if ( mde->mde_srv_parms.pl_parms.ps_result != PS_AFFIRMATIVE )
	{
	    tmp_gnrc_status = mde->mde_generic_status[0];
	}
	/* 
	** Check that inbound association limit has not been 
	** reached and that the server is not quiescing or closed.
	*/
	else  if ( IIGCc_global->gcc_ib_conn_ct > IIGCc_global->gcc_ib_max )
	{
   	    tmp_gnrc_status = E_GC2214_MAX_IB_CONNS;
	    gcc_er_log( &tmp_gnrc_status, NULL, mde, NULL );
	}
	/* 
	** Check that the server is not closed.
	*/
	else  if ( ( IIGCc_global->gcc_flags & GCC_QUIESCE ) ||   
	 	   ( IIGCc_global->gcc_flags & GCC_CLOSED ) )  
	{
   	    tmp_gnrc_status = E_GC221B_AL_SVR_CLSD;
	    gcc_er_log( &tmp_gnrc_status, NULL, mde, NULL );
	}
	/* 
	** If there is no user_name or authentication (password or GCS
	** object), reject this association with an error indicating 
	** User Authorization Info error.
	*/
	else  if ( ! (ccb->ccb_hdr.flags & CCB_USERNAME)  || 
	           (! ccb->ccb_hdr.password  &&  ! ccb->ccb_hdr.authlen) )
	{
	    tmp_gnrc_status = E_GC220D_AL_PASSWD_FAIL;
	    gcc_er_log( &tmp_gnrc_status, NULL, mde, NULL );
	} /* end if */

	/*
	** If any of the preceding checks failed, re-issue the MDE as
	** a connection request/reject.
	*/
	if ( tmp_gnrc_status != OK )
	{
	    mde->service_primitive = A_R_REJECT;
	    mde->primitive_type = RQS;
	    mde->p1 = mde->p2 = mde->papp;

            if ( ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_61  &&
                 ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_62 )
        	mde->p2 += GCaddi2( &ccb->ccb_hdr.gca_protocol, mde->p2 );

            mde->p2 += GCnaddnat( &tmp_gnrc_status, mde->p2, &status );
	    (VOID)gcc_al( mde );
            break;
	}

	/*
	** Invoke GCA_REQUEST service.  
	*/

	mde->service_primitive = A_ASSOCIATE;
	mde->primitive_type = RSP;
	mde->gca_rq_type = GCC_A_REQUEST;

	rqo_parms = (GCA_RQ_PARMS *)&mde->mde_act_parms.gca_parms;
	rqo_parms->gca_user_name = ccb->ccb_hdr.username;
	rqo_parms->gca_password = ccb->ccb_hdr.password;
	rqo_parms->gca_partner_name = ccb->ccb_hdr.target 
				      ? ccb->ccb_hdr.target : "";
	rqo_parms->gca_peer_protocol = ccb->ccb_hdr.gca_protocol;
	rqo_parms->gca_modifiers = 0;

	if (ccb->ccb_hdr.authlen )
	{
	    rqo_parms->gca_modifiers = GCA_RQ_AUTH;
	    rqo_parms->gca_l_auth = ccb->ccb_hdr.authlen;
	    rqo_parms->gca_auth = ccb->ccb_hdr.auth;
	}

	if (ccb->ccb_aae.a_flags & GCCAL_GOTOHET)
	    rqo_parms->gca_modifiers |= GCA_RQ_DESCR;

	if (ccb->ccb_aae.a_flags & GCCAL_GCM)
	    rqo_parms->gca_modifiers |= GCA_RQ_GCM;

	/* Log initiation attempt */
    
	tmp_gnrc_status = E_GC2212_OUTGNG_ASSOC;
	erlist.gcc_parm_cnt = 1;
	erlist.gcc_erparm[0].size = STlength(rqo_parms->gca_user_name);
	erlist.gcc_erparm[0].value = rqo_parms->gca_user_name;
	gcc_er_log( &tmp_gnrc_status, (CL_ERR_DESC *)NULL, mde, &erlist );
	call_status = gca_call( &gca_cb, GCA_REQUEST, (GCA_PARMLIST *)rqo_parms,
				 (i4)GCA_ASYNC_FLAG, (PTR)mde, 
				 (i4)(-1), &status);

	/*
	** If there is a failure as indicated by returned status, log
	** an error and abort the association.
	*/
	if (call_status != OK)
	{
	    gcc_al_abort(mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	}
	break;
    } /* end case AARQS */
    case AARSP:	/* GCA_RQRESP */
    {
	GCA_RR_PARMS	*rr_parms;
        AL_AARE_PCI     aare_pci;
        u_i2            req_result;
        STATUS          temp_gnrc_status;
	char		*pci;
        bool            log;

	/*
	** Issue GCA_RQRESP to association initiator. The MDE may be:
        **
        ** (i) a P_CONNECT (confirm) containing, as user data, an AARE APDU.
        ** The user data of the APDU will be non-existent (at 6.1/6.2) or
        ** will contain an associate response datastructure (at 6.3).
	**
        ** (ii) a P_CONNECT (reject) containing, as user data, an AARE APDU.
        ** The user data of the APDU will contain (at 6.1/6.2) an abort
        ** datastructure; (at 6.3) an associate response datastructure.
        **
        ** (iii) a P_U_ABORT containing, as user data, an ABORT APDU. The
        ** user data of the APDU will contain an abort datastructure; 
        ** at 6.3 it will be followed by a GCA_ER_DATA message (which we
        ** ignore).
        **
        ** (iv) a P_P_ABORT which will contain no user data. If the 
        ** provider abort occurred locally there will be a status in the mde.
        **
        ** THE PCI and the user data areas of the MDE contain parameters for
        ** the call to GCA_RQRESP. The MDE becomes an A_COMPLETE RQS when the
        ** GCA_RQRESP is complete.
        **
        ** (v) an A_SHUT, A_QUIESCE, A_CMDSESS or A_I_REJECT which are local 
	** errors with a generic error in the mde header.
        **
        ** Switch on the primitive type to see what application layer version
        ** our peer is at. 
	*/

        log = FALSE;
        switch (mde->service_primitive)
        {
        case P_CONNECT:
        {
            log = TRUE;

	    /*
	    ** Strip out the pci
	    */
            pci = mde->p1;

	    if ( (mde->p1 + SZ_AARE_PCI) > mde->p2 )
	    {
	        mde->p1 = mde->p2;
		aare_pci.prot_vrsn = AL_PROTOCOL_VRSN_61;
		req_result = AARQ_REJECTED;
	    }
	    else
	    {
		mde->p1 += SZ_AARE_PCI;
		pci += GCgeti2(pci, &aare_pci.prot_vrsn);
		pci += GCgeti2(pci, &req_result);
	    }

            /* 
            ** Get the application layer version number from the PCI. This
            ** tells us whether our peer application layer has set the GCA
            ** protocol version in the user data part of the response. If the
            ** application protocol is either version 6.1 or 6.2, no GCA
            ** protocol version will have been set. In those cases, we deduce
            ** that it must be protocol version 2. Pure hacksville.
            */
        
            ccb->ccb_aae.a_version = aare_pci.prot_vrsn;

            if ( (ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_61)  &&
                 (ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_62)  &&
		 ((mde->p1 + 2) <= mde->p2) )
            {
		mde->p1 += GCgeti2(mde->p1, &ccb->ccb_hdr.gca_protocol);
            }
            else
            {
                ccb->ccb_hdr.gca_protocol = (i4) GCA_PROTOCOL_LEVEL_2;
            }

            /*
            ** Look to see whether the association request was accepted or
            ** rejected. 
            */
            if (req_result == (u_i2) AARQ_ACCEPTED)
            {
                temp_gnrc_status = OK;
            }
            else
            {
                /* 
                ** the request was rejected. Look at the user data for the
                ** reject reason.
                */
		if ( (mde->p1 + 4) > mde->p2 )
		    temp_gnrc_status = E_GC220F_AL_INTERNAL_ERROR;
		else
		    mde->p1 += GCngetnat(mde->p1, &temp_gnrc_status, &status);
            }
            break;
        } /* end case P_CONNECT */
        case P_U_ABORT:
        {
            log = TRUE;

            /* Get the generic status from the user data */
	    if ( (mde->p1 + SZ_ABRT_PCI) > mde->p2 )
	        mde->p1 = mde->p2;
	    else
		mde->p1 += SZ_ABRT_PCI;

	    if ( (mde->p1 + 4) > mde->p2 )
		temp_gnrc_status = E_GC220F_AL_INTERNAL_ERROR;
	    else
		mde->p1 += GCngetnat(mde->p1, &temp_gnrc_status, &status);
            break;    
        } /* end case P_U_ABORT */
    
        case P_P_ABORT:
        {
 
            /* 
            ** The provider abort contains no data. In fact, the request may
            ** not even have got as far as the remote application layer.
            ** Check to see whether abort occurred locally or remotely.
            */
            temp_gnrc_status = mde->mde_generic_status[0];
            if (temp_gnrc_status == OK)
	    {
		/* remote provider abort */
	        log = TRUE;
                temp_gnrc_status = E_GC2205_RMT_ABORT;
	    }
            break;
        } /* end case P_P_ABORT */
 
        case A_SHUT:
        case A_QUIESCE:
        case A_I_REJECT:
        case A_CMDSESS:
        {
            /* Take the error out of the mde header */
            temp_gnrc_status = mde->mde_generic_status[0];
            break;
        } /* end cases A_SHUT, A_QUIESCE and A_I_REJECT */

        default:
        {
            break;
	} /* end default */
        } /* end switch */

        /* See whether we need to log an error */
        if ((log) && (temp_gnrc_status != OK))
	{
	    gcc_er_log( &temp_gnrc_status,
			(CL_ERR_DESC *) NULL,
			mde,
			(GCC_ERLIST *) NULL );
	    if (temp_gnrc_status == E_GC000B_RMT_LOGIN_FAIL)
	    {
		int		errcode = E_GC000E_RMT_LOGIN_FAIL_INFO;
		GCC_ERLIST	erlist;
		char		*node_id = ccb->ccb_hdr.trg_addr.node_id;

		erlist.gcc_parm_cnt = 2;
		erlist.gcc_erparm[0].value = node_id;
		erlist.gcc_erparm[0].size = STlength(node_id);
		erlist.gcc_erparm[1].value = ccb->ccb_hdr.username;
		erlist.gcc_erparm[1].size = STlength(ccb->ccb_hdr.username);
		gcc_er_log( &errcode,
			    (CL_ERR_DESC *) NULL,
			    mde,
			    &erlist );
	    }
        }

	/* Strip out the log level */
	temp_gnrc_status = GCC_LGLVL_STRIP_MACRO(temp_gnrc_status);

        /* Allocate the response parameter structure */


        /* fill in the parameter list */

	rr_parms = (GCA_RR_PARMS *)&mde->mde_act_parms.gca_parms;
        rr_parms->gca_assoc_id = ccb->ccb_aae.assoc_id;
        rr_parms->gca_local_protocol = ccb->ccb_hdr.gca_protocol;
        rr_parms->gca_request_status = temp_gnrc_status;
        rr_parms->gca_modifiers = 0;

        /* See whether this is a het. association */

        if (ccb->ccb_aae.a_flags & GCCAL_GOTOHET)
	{
	    if (ccb->ccb_aae.a_flags & GCCAL_GCM)

		/* Force off ridiculous GOTOHET flag */
    	    	ccb->ccb_aae.a_flags &= ~GCCAL_GOTOHET;
    	    else
    		rr_parms->gca_modifiers |= GCA_RQ_DESCR;
	}	    

	/*
	** Invoke GCA_RQRESP service.  
	*/

        mde->service_primitive = A_COMPLETE;
        mde->primitive_type = RQS;
	mde->gca_rq_type = GCC_A_RESPONSE;

        call_status = gca_call( &gca_cb, GCA_RQRESP, 
				 (GCA_PARMLIST *)rr_parms,
				 (i4) GCA_ASYNC_FLAG,  /* Asynchronous */
				 (PTR)mde,    	    	/* async id */
				 (i4) -1,		/* no timeout */
				 &status);

	/*
	** If there is a failure as indicated by returned status, log 
	** an error and abort the association.
	*/

	if (call_status != OK)
	{
            /* Log an error and abort the association */
	    gcc_al_abort(mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	}
	break;
    } /* end case AARSP */
    case AARVN:	/* GCA_RECEIVE (normal) */
    {
	GCC_MDE		*rvn_mde;
	GCA_RV_PARMS	*rvn_parms;

	/*
	** Issue GCA_RECEIVE (normal) to local server
	**
	** Allocate a new MDE for the receive operation.
	*/

	rvn_mde = gcc_add_mde( ccb, ccb->ccb_tce.tpdu_size );
	if (rvn_mde == NULL)
	{
            /* Log an error and abort the association */
            status = E_GC2004_ALLOCN_FAIL;
	    gcc_al_abort(rvn_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
            status = FAIL;
	    break;
	} /* end if */

	rvn_mde->ccb = ccb;
	rvn_mde->service_primitive = A_DATA;
	rvn_mde->primitive_type = RQS;
	rvn_mde->p1 = rvn_mde->p2 = rvn_mde->pdata;
	rvn_mde->gca_rq_type = GCC_A_NRM_RCV;

	/*
	** Invoke GCA_RECEIVE service.  
	** The pointer gca_buffer points to the start of where the received
    	** data, including the GCA header will start.  Therefore, it is
	** calculated so that the actual data will land at the start of the
	** application user data area.  The GCA header is then overwritten by
	** PCI data and is effectively discarded. Note the implicit 
    	** assumption that there is room in the MDE to do this.
	*/

	rvn_parms = (GCA_RV_PARMS *) &rvn_mde->mde_act_parms.gca_parms;
	rvn_parms->gca_association_id = ccb->ccb_aae.assoc_id;
	rvn_parms->gca_flow_type_indicator = GCA_NORMAL;
	rvn_parms->gca_buffer = rvn_mde->p1 - IIGCc_global->gcc_gca_hdr_lng;
	rvn_parms->gca_b_length = rvn_mde->pend - rvn_parms->gca_buffer;
	rvn_parms->gca_modifiers = 0;

	call_status = gca_call( &gca_cb, GCA_RECEIVE, 
				 (GCA_PARMLIST *)rvn_parms,
				 (i4) GCA_ASYNC_FLAG,  /* Asynchronous */
				 (PTR)rvn_mde,		/* async id */
				 (i4) -1,		/* no timeout */
				 &status);

	/*
	** If there is a failure as indicated by returned status, log
	** an error and abort the association.
	*/

	if (call_status != OK)
	{
            /* Log an error and abort the association */
	    gcc_al_abort(rvn_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	}
	break;
    } /* end case AARVN */
    case AARVE:	/* GCA_RECEIVE (expedited) */
    {
	GCC_MDE		*rve_mde;
	GCA_RV_PARMS	*rve_parms;

	/*
	** Issue GCA_RECEIVE (expedited) 
	*/

	/*
	** Allocate and initialize a new MDE for the receive operation.
	** The MDE becomes an A_DATA_EXP RQS.
	*/

	rve_mde = gcc_add_mde( ccb, ccb->ccb_tce.tpdu_size );
	if (rve_mde == NULL)
	{
            /* Log an error and abort the association */
            status = E_GC2004_ALLOCN_FAIL;
	    gcc_al_abort(rve_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	} /* end if */

	rve_mde->ccb = ccb;
	rve_mde->service_primitive = A_EXP_DATA;
	rve_mde->primitive_type = RQS;
	rve_mde->p1 = rve_mde->p2 = rve_mde->pdata;
	rve_mde->gca_rq_type = GCC_A_EXP_RCV;

	/*
	** Invoke GCA_RECEIVE service. 
	** The pointer gca_buffer points to the start of where the received
    	** data, including the GCA header will start.  Therefore, it is
	** calculated so that the actual data will land at the start of the
	** application user data area.  The GCA header is then overwritten by
	** PCI data and is effectively discarded. Note the implicit 
    	** assumption that there is room in the MDE to do this.
	*/

	rve_parms = (GCA_RV_PARMS *)&rve_mde->mde_act_parms.gca_parms;
	rve_parms->gca_association_id = ccb->ccb_aae.assoc_id;
	rve_parms->gca_flow_type_indicator = GCA_EXPEDITED;
	rve_parms->gca_buffer = rve_mde->p1 - IIGCc_global->gcc_gca_hdr_lng;
	rve_parms->gca_b_length = rve_mde->pend - rve_parms->gca_buffer;
	rve_parms->gca_modifiers = 0;

	call_status = gca_call( &gca_cb, GCA_RECEIVE,
				 (GCA_PARMLIST *)rve_parms,
				 (i4) GCA_ASYNC_FLAG,  /* Asynchronous */
				 (PTR)rve_mde,		/* async id */
				 (i4) -1,		/* no timeout */
				 &status);

	/*
	** If there is a failure as indicated by returned status, log
	** an error and abort the association.
	*/

	if (call_status != OK)
	{
            /* Log an error and abort the association */
	    gcc_al_abort(rve_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	}
	break;
    } /* end case AARVE */
    case AACMD:	/* command request  */
    {
     	call_status = gcc_adm_session ( ccb->ccb_aae.assoc_id, gca_cb, 
					ccb->ccb_hdr.username );
      
	if (call_status != OK)
	{ 
	    GCC_ERLIST		erlist;

	    /* Log an error and abort the association */

	    erlist.gcc_parm_cnt = 0;
	    status = E_GC2208_CONNCTN_ABORT;
	    gcc_al_abort(mde, ccb, &call_status, (CL_ERR_DESC *) NULL,
     			 (GCC_ERLIST *) &erlist);
	    status = FAIL;
	}   
	else 	
	{
	    /* disconnect the association */
	    mde->service_primitive = A_FINISH;
	    mde->primitive_type = RQS;
	    gcc_al( mde );
	} 
	break;
    } 
    case AADIS:	/* GCA_DISASSOC */
    {
        GCC_MDE         *da_mde;
	GCA_DA_PARMS	*dao_parms;

	/* If we're stopping the server, don't do anything */
	if (IIGCc_global->gcc_flags & GCC_STOP)
	    break;

	/*
	** Issue GCA_DISASSOC to clean up the local association.
        **
        ** Allocate a new MDE for the disassociate operation.
	*/
  	da_mde = gcc_add_mde( ccb, 0 );
	if (da_mde == NULL)
	{
            /* Log an error and abort the association */
            status = E_GC2004_ALLOCN_FAIL;
	    gcc_al_abort(da_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	} /* end if */


        da_mde->ccb = ccb;
	da_mde->service_primitive = A_FINISH;
	da_mde->primitive_type = RQS;
	da_mde->gca_rq_type = GCC_A_DISASSOC;

	dao_parms = (GCA_DA_PARMS *)&da_mde->mde_act_parms.gca_parms;
	dao_parms->gca_association_id = ccb->ccb_aae.assoc_id;

	call_status = gca_call( &gca_cb, GCA_DISASSOC, 
				 (GCA_PARMLIST *)dao_parms,
				 (i4) GCA_ASYNC_FLAG,   /* Asynchronous */
				 (PTR)da_mde,		 /* async id */
				 (i4) GCC_TIMEOUT,  
				 &status);

	/*
	** If there is a failure as indicated by returned status, log
	** an error and abort the association.
	*/

	if (call_status != OK)
	{
	    /* Log an error and abort the association */
	    gcc_al_abort(da_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	}
	break;
    } /* end case AADIS */
    case AALSN:	/* GCA_LISTEN */
    {
    	GCA_LS_PARMS    *ls_parms;
	i4		i;

	/*
	** Issue GCA_LISTEN to establish an open listen to allow clients 
	** to invoke comm server services.
	*/

	ccb->ccb_hdr.async_cnt += 1;
        ccb->ccb_hdr.emode     = IIGCc_global->gcc_ob_mode;

        /* Set the application layer protocol version number */

        ccb->ccb_aae.a_version = AL_PROTOCOL_VRSN;

	mde->service_primitive = A_ASSOCIATE;
	mde->primitive_type = RQS;
	mde->p1 = mde->p2 = mde->papp;
	mde->gca_rq_type = GCC_A_LISTEN;

	/*
	** Invoke GCA_LISTEN service. 
	*/

        ls_parms = (GCA_LS_PARMS *)&mde->mde_act_parms.gca_parms;

	call_status = gca_call( &gca_cb, GCA_LISTEN, 
				 (GCA_PARMLIST *)ls_parms,
				 (i4) GCA_ASYNC_FLAG,  /* Asynchronous */
				 (PTR)mde,		/* async id */
				 (i4) -1,		/* no timeout */
				 &status);

	/*
	** If there is a failure as indicated by returned status, log
        ** an error and abort ther association.
	*/

	if (call_status != OK)
	{
            /* Log an error and abort the association */
	    gcc_al_abort(mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	}
	break;
    } /* end case AALSN */
    case AASND:	/* GCA_SEND */
    {
	status = gcc_asnd(mde);
        break;
    } /* end case AASND */
    case AASNDG:	/* GCA_SEND of GCA_GOTOHET message */
	{
	GCC_MDE         *gotohet_mde;
	
	/*
	** Issue GCA_SEND to send a special (one time only) GCA_GOTOHET message.
	** 
	** The MDE becomes an A_COMPLETE RQS.
	*/

	/* First get a fresh mde */
	gotohet_mde = gcc_add_mde( ccb, 0 );
  	if (gotohet_mde == NULL)
	{
            /* Log an error and abort the association */
            status = E_GC2004_ALLOCN_FAIL;
	    gcc_al_abort(gotohet_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	} /* end if */
	gotohet_mde->ccb = ccb;
	gotohet_mde->mde_srv_parms.al_parms.as_msg_type = GCA_GOTOHET;
	gotohet_mde->mde_srv_parms.al_parms.as_flags = 0;
	gotohet_mde->mde_srv_parms.al_parms.as_flags |= AS_EOD;
	status = gcc_asnd(gotohet_mde);
	break;
    } /* end case AASNDG */
    case AAENQG:	/* Enqueue of GCA_GOTOHET message */
	{
	GCC_MDE         *gotohet_mde;
	
	/*
	** Enqueue a special (one time only) GCA_GOTOHET message.
	** The GCA message header is constructed in the PCI area
	** The MDE becomes an A_COMPLETE RQS.
	*/

	/* First get a fresh mde */
	gotohet_mde = gcc_add_mde( ccb, 0 );
  	if (gotohet_mde == NULL)
	{
	    /* Log an error and abort the association */
	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_al_abort(gotohet_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	} /* end if */
	gotohet_mde->ccb = ccb;
	gotohet_mde->mde_srv_parms.al_parms.as_msg_type = GCA_GOTOHET;
	gotohet_mde->mde_srv_parms.al_parms.as_flags = 0;
	gotohet_mde->mde_srv_parms.al_parms.as_flags |= AS_EOD;

        /* Now enqueue it */
        QUinsert( &gotohet_mde->mde_q_link, ccb->ccb_aae.a_rcv_q.q_prev );
	break;
    } /* end case AAENQG */
    case AARLS:	/* Build GCA_RELEASE message */
    {
	status = gcc_aerr(mde, ccb); /* Construct a GCA_ER_DATA object */
	if (status != OK)
	{
	    /* Log an error and abort the association */
	    status = E_GC220F_AL_INTERNAL_ERROR;
	    gcc_al_abort(mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    break;
	} /* end if */

	mde->mde_srv_parms.al_parms.as_msg_type = GCA_RELEASE;
	mde->mde_srv_parms.al_parms.as_flags = 0;
	mde->mde_srv_parms.al_parms.as_flags |= AS_EOD | AS_EOG;
	break;
    } /* end case AARLS */
    case AAENQ:	/* Enqueue a message */
    {
	/*
	** Enqueue a message received in a P_[EXPEDITED_]DATA indication to
	** be sent when a send currently in process completes (use the GCA
	** queue utility routines).  The message is inserted at the head of the
	** queue in the case of expedited data.
	*/

	if (mde->service_primitive == P_EXP_DATA)
	{
	    QUinsert( &mde->mde_q_link, &ccb->ccb_aae.a_rcv_q );
	}
	else
	{
	    QUinsert( &mde->mde_q_link, ccb->ccb_aae.a_rcv_q.q_prev );
	}
	ccb->ccb_aae.a_rq_cnt += 1;
	break;
    } /* end case AAENQ */
    case AADEQ:	/* Dequeue a message and send */
    {
	GCC_MDE		*send_mde;

	/*
	** Dequeue a message to be sent to the local association partner.
	** Issue GCA_SEND to send the message.
	*/

	send_mde = (GCC_MDE *) QUremove(ccb->ccb_aae.a_rcv_q.q_next);
	ccb->ccb_aae.a_rq_cnt -= 1;
	send_mde->mde_q_link.q_next = NULL;
	send_mde->mde_q_link.q_prev = NULL;
	status = gcc_asnd(send_mde);
	break;
    } /* end case AADEQ */
    case AAPGQ:	/* Purge the queue of messages */
    {
	/*
	** Purge the queue of messages intended for the local 
	** association partner.
	*/

	GCC_MDE	    *purg_mde;

	while ((purg_mde = (GCC_MDE *)QUremove( ccb->ccb_aae.a_rcv_q.q_next ))
		!= NULL)
	{
	    /*
	    ** Return the MDE to the scrapyard
	    */

	    gcc_del_mde( purg_mde );
	} /* end while */
	break;
    } /* end case AAPGQ */
    case AAMDE:	/* Return the MDE */
    {
	/*
	** Return the MDE to the scrapyard.
	*/
	gcc_del_mde( mde );
	break;
    } /* end case AAMDE */
    case AACCB:	/* Return the CCB */
    {
	/*
	** Flag the CCB for release.  The actual
	** release will occur in gcc_al() when
	** all processing has been completed.
	*/
	ccb->ccb_aae.a_flags |= GCCAL_CCB_RELEASE;
	break;
    } /* end case AACCB */
    case AASHT:	/* Shut down the Comm Server */
    {
	IIGCc_global->gcc_flags |= GCC_CLOSED;
	GCshut();
	break;
    } /* end case AASHT */
    case AAQSC:	/* Quiesce the Comm Server */
    {
        /* 
        ** If there are no extant associations we can shut down the server
        ** straight away as in the shutdown case. Otherwise we set the quiesce
        ** flag and wait for the last association to bite the dust...
        */
        if (IIGCc_global->gcc_conn_ct > 0)
    	    IIGCc_global->gcc_flags |= GCC_QUIESCE | GCC_CLOSED;
    	else
	{
	    IIGCc_global->gcc_flags |= GCC_CLOSED;
  	    GCshut();
	}
	break;
    } /* end case AAQSC */
    case AASXF:
    {
	/*
	** Set the "Received XOF" indicator in the CCG AL flags.
	*/

	ccb->ccb_aae.a_flags |= GCCAL_RXOF;
	break;
    } /* end case AASXF */
    case AARXF:
    {
	/*
	** Reset the "Received XOF" indicator in the CCG AL flags.
	*/

	ccb->ccb_aae.a_flags &= ~GCCAL_RXOF;
	break;
    } /* end case AARXF */
    case AAFSL:
    {
	GCC_MDE		*rvn_mde;
	GCA_RV_PARMS	*rvn_parms;
	STATUS		status;

   	/*
    	** As this is a fast select, the actual "data" associated
    	** with it will have been saved in the CCB. We set up the
    	** GCA normal receive parameter list as though we are about
    	** to issue a GCA normal receive. However, we'll drive the
    	** exit for the normal receive (ie gcc_gca_exit) directly.
    	** First, allocate an MDE for the receive operation.
    	*/
	rvn_mde = gcc_add_mde( ccb, ccb->ccb_tce.tpdu_size );
	if (rvn_mde == NULL)
	{
            /* Log an error and abort the association */
    	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_al_abort(rvn_mde, ccb, &status, 
    	    	    	 (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL);
    	    status = FAIL;
    	    break;
    	} /* end if */

	rvn_mde->ccb = ccb;
	rvn_mde->service_primitive = A_DATA;
	rvn_mde->primitive_type = RQS;
	rvn_mde->gca_rq_type = GCC_A_NRM_RCV;
        rvn_mde->p1 = rvn_mde->p2 = rvn_mde->pdata;

	rvn_parms = (GCA_RV_PARMS *)&rvn_mde->mde_act_parms.gca_parms;
	rvn_parms->gca_association_id = ccb->ccb_aae.assoc_id;
	rvn_parms->gca_flow_type_indicator = GCA_NORMAL;
	rvn_parms->gca_buffer = rvn_mde->p1 - IIGCc_global->gcc_gca_hdr_lng;
    	rvn_parms->gca_b_length = rvn_mde->pend -  rvn_parms->gca_buffer;
	rvn_parms->gca_modifiers = 0;
    	
    	/* Copy the fast select data into the mde */
	rvn_parms->gca_message_type = ccb->ccb_aae.fsel_type;
    	MEcopy( ccb->ccb_aae.fsel_ptr,
		ccb->ccb_aae.fsel_len, rvn_parms->gca_buffer );
	gcc_free( ccb->ccb_aae.fsel_ptr );
	ccb->ccb_aae.fsel_ptr = NULL;

	rvn_parms->gca_data_area = rvn_parms->gca_buffer + 
				   IIGCc_global->gcc_gca_hdr_lng;
	rvn_parms->gca_d_length = ccb->ccb_aae.fsel_len - 
				  IIGCc_global->gcc_gca_hdr_lng;
	rvn_parms->gca_end_of_data = TRUE;
	rvn_parms->gca_end_of_group = TRUE;

    	/* 
    	** And finally, there isn't an error when we drive
    	** the completion...
    	*/
    	rvn_parms->gca_status = OK;
    	gcc_gca_exit( (PTR)rvn_mde );
    	break;
    }
    default:
	{
	    /*
	    ** Unknown action event ID.  This is a program bug.
	    ** Log an error and abort the association 
            */
	    status = E_GC220F_AL_INTERNAL_ERROR;
	    gcc_al_abort(mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    status = FAIL;
	    break;
	}
    } /* end switch */
    return(status);
} /* end gcc_alactn_exec */

/*{
** Name: gcc_aerr	- Construct a GCA_ER_DATA object
**
** Description:
**      Construct a GCA_ER_DATA object to be sent with a GCA_RELEASE message.
**      The source of the error codes to be included in the GCA_ER_DATA data
**      object of the message depend on the source of the error.  If the error
**      was remote, the mde contains a PDU with abort data in the user data.
**      The remotely determined error code is obtained from there.  If the 
**      error was locally generated, the mde_generic_status array in the MDE
**      contains a list of error codes.  The specifics of the source are
**      determined based on the primitive type.  Then the GCA_ER_DATA object
**	is constructed, and consists of as many GCA_E_ELEMENT's as there are
**	error codes to be sent.  It is necessary to invoke ERlookup to get the
**	text of the messages which correspond to the error codes.  These are
**	inserted in the parameter field of each GCA_E_ELEMENT.
**
** Inputs:
**      mde                             Pointer to an MDE for the message
**
** Outputs:
**      none
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
**      08-Dec-88 (jbowers)
**          Created
**	19-jun-89 (seiwald)
**	    Added breaks to a switch, so E_GC2205_RMT_ABORT makes it through.
**      02-Aug-89 (cmorris)
**          Modified to reflect the fact that PCI is not stripped off prior
**          to entry to this routine. Additionally, as this function now only
**          supports the creation of release (and NOT reject) messages,
**          no P_CONNECT cases need now exist.
**      07-Aug-89 (cmorris)
**          Add A_QUIESCE case.
**      15-Aug-89 (heatherm)
**          Changes for encoding remote error messages.
**      16-Aug-89 (cmorris)
**          Merged in end to end bind changes.
**      25-Aug-89 (cmorris)
**          Only interpret non-encoded data values.
**      07-Sep-89 (cmorris)
**          Removed the A_P_ABORT case as this service primitive can never
**          exist as incoming PL events are not changed into their 
**          corresponding AL events. 
**      09-Sep-89 (cmorris)
**          Removed the A_SHUT, A_QUIESCE cases. These are now handled by
**          the AARSP action of gcc_alactn_exec.
**      04-Oct-89 (cmorris)
**          Initialize er_data and er_elem prior to the switch statement.
**      02-Oct-89 (heather/jorge)
**           Moved the 2 lines that set er_data & er_elem to the begining
**           of the routine. Used to get ACCVIO in 6.3-6.2 connections.
**      31-Oct-89 (cmorris)
**          Implemented calls to GCnxxx functions to convert individual types
**          from network xfer syntax.
**      13-Dec-89 (cmorris)
**          Distinguish between local and remote provider aborts.
**	22-Mar-90 (seiwald)
**	    Extend tmp_gnrc_status by 1 to allow for prepended 
**	    E_GC2210_LCL_ABORT status.
**	07-May-90 (seiwald)
**	    Reworked to format GCA_ER_DATA structure properly.
**	    GCA_ER_DATA contains variable length data, which was being
**	    formatted by casting structure pointers.  Now GCA_ER_DATA
**	    is built upon the new GCladd{nat,long} macros, which avoids
**	    hitting alignment restrictions.
**	28-Dec-90 (seiwald)
**	    Protect ourselves from insane 6.3 comm servers: sometimes
**	    a 6.3 comm server would generate a garbage P_U_ABORT without
**	    the proper GCC_ABT_DATA structure.  If the message looks
**	    bad, don't bother unpacking it.
**  	02-Jan-91 (cmorris)
**  	    Check for P_U_ABORT that has a null GCC_ABT_DATA.
**	25-Aug-98 (gordy)
**	    Fix building of GCA_ER_DATA object.  The first two integers
**	    were being lost because mde->p1 was being used for the firt
**	    integer value rather than mde->p2.
**	10-Dec-02 (gordy)
**	    GCA_ERROR message format is protocol level depedent.
*/

static STATUS
gcc_aerr( GCC_MDE *mde, GCC_CCB *ccb )
{
    i4			i;
    STATUS		gnrc_status[GCC_L_GEN_STAT_MDE + 1];
    i4             gca_l_e_element;
    STATUS		status = OK;
    bool		log;
    bool                resolve;

    log = FALSE; 
    resolve = FALSE;

    /*
    ** The source of error data is determined by the type of service primitive
    ** represented by the MDE.  If this is a P_U_ABORT , it was necessarily
    ** received from the remote partner.  If it is a P_P_ABORT
    ** it originated eitherlocally or remotely. If is was local, presumably a
    ** lower layer has alrready logged the original cause of the failure. 
    **
    ** In all cases, we construct a local array of all error codes to be sent
    ** in the message in tmp_gnrc_status[].  We first insert a locally
    ** generated status code, indicating the nature of the problem, e.g., local
    ** or remote.  Then, one or more error codes associated with the error
    ** event itself are moved into the array.  This array is then used to log
    ** out the error. If our peer is pre-6.3, it is also used to construct the
    ** GCA_ER_DATA data object.  Each error code in the array generates a
    ** GCA_E_ELEMENT object within the GCA_ER_DATA data object. If our peer is
    ** >= 6.3, the GCA_ER_DATA data object will have already been constructed
    ** by the remote comm. server and network encoded. We have to
    ** decode it. The indicator 'resolve' is set to indicate whether we
    ** should resolve the array into a GCA_ER_DATA data object here.
    **
    ** In the case of a remote error, we have to log it out locally
    ** at this point.  If it is a local error, it was presumably logged out at
    ** the point of ocurrence.  The indicator 'log' is set to indicate whether
    ** we should log the errors here.
    */

    switch (mde->service_primitive)
    {
    case P_P_ABORT:
    {
	/*
	** This may be either a local or remote abort indication. 
        ** If it's local, there will be error codes in the
	** mde_generic_status array in the MDE. If it's remote, there won't
        ** be.
	*/
        if (mde->mde_generic_status[0] != OK)
	{
 	    /* local abort */
	    resolve = TRUE;
	    gnrc_status[0] = E_GC2210_LCL_ABORT;
	    for (i = 0;
		 (i < GCC_L_GEN_STAT_MDE) && (mde->mde_generic_status[i] != OK);
		 i++)
	    {
		gnrc_status[i + 1] = mde->mde_generic_status[i];
	    } /* end for */
	    gca_l_e_element = i + 1;
	}
	else
	{
	    /* remote abort */
	    log = TRUE;
            gnrc_status[0] = E_GC2205_RMT_ABORT;
	    gca_l_e_element = 1;
	}
	break;
    } /* end case P_P_ABORT */
    case P_U_ABORT:
    {
        /* Strip off the ABRT PCI */

        mde->p1 += SZ_ABRT_PCI;
	log = TRUE; /* Log a remote error */

	/*
	** This is a remote abort indication. For backward compatability, 
        ** don't ya luv it, we must check the version number so that we will
        ** resolve messages for versions previous to 6.3. Back then the
        ** client side comm. server did all the error resolution.
        */ 
        if ((ccb->ccb_aae.a_version == AL_PROTOCOL_VRSN_61) ||
            (ccb->ccb_aae.a_version == AL_PROTOCOL_VRSN_62))
        {
            resolve = TRUE;
            gnrc_status[0] = E_GC2205_RMT_ABORT;
            (VOID) GCngetnat(mde->p1, &gnrc_status[1], &status);
            gca_l_e_element = 2;
	    break;
	}

	/*
	** Some 6.3 comm servers sent an P_U_ABORT without having
	** called gcc_aaerr to format the GCC_ABT_DATA first.  Do
	** a crude sanity check.
	*/

        if( ccb->ccb_aae.a_version == AL_PROTOCOL_VRSN_63 )
	{
	    char	*mde_p1 = mde->p1;
	    i4		tmp_status;
	    i4	tmp_gnrc_status;

            /* get and ignore the status from the GCC_ABT_DATA structure */
            mde_p1 += GCngetnat(mde_p1, &tmp_status, &status);

            /* Convert the count of error elements */
            mde_p1 += GCngetlong(mde_p1, &gca_l_e_element, &status);

	    /* Get first gca_id_error */
	    mde_p1 += GCngetlong(mde_p1, &tmp_gnrc_status, &status);

	    /* A proper P_U_ABORT from 6.3 will have these values. */

	    if( gca_l_e_element != 2 ||
		tmp_gnrc_status != E_GC2205_RMT_ABORT )
	    {
		resolve = TRUE;
		gnrc_status[0] = E_GC2205_RMT_ABORT;
		gca_l_e_element = 1;
		break;
	    }
	}

	/*
	**  6.3 or better P_U_ABORT with (potentially null) GCC_ABT_DATA. 
	**  This is a 6.3<->6.3 (or better) error. Check first for null
    	**  case.
    	*/

    	if (! (mde->p2 - mde->p1))
    	{
    	    resolve = TRUE;
	    gnrc_status[0] = E_GC2205_RMT_ABORT;
	    gca_l_e_element = 1;
	    break;
    	}

    	/*        
        **  Non-null GCC_ABT_DATA. The error messages were resolved by the 
    	**  remote comm. server then converted to network
	**  standard format.  We need to convert back.
	**  Get temporary space so that we may realign the remote error
	**  data. The error data begins at mde->p1.
	**  We'll copy it to temporary space then the conversion can go
	**  straight into the correct spot in the mde.
	*/

	{
	    char		*nmde_p1;	/* network format data */
	    char                *nmde_app;	/* tmp for network data */
	    i4			tmpnat;
	    i4		tmplong;

	    nmde_app = MEreqmem( 0, (u_i4)(mde->p2 - mde->p1), TRUE, &status);
	    MEcopy((PTR)mde->p1, (u_i4)(mde->p2 - mde->p1), (PTR)nmde_app);
         
            nmde_p1 = nmde_app;
	    mde->p1 = mde->p2 = mde->papp;

            /* get and ignore the status from the GCC_ABT_DATA structure */
            nmde_p1 += GCngetnat(nmde_p1, &tmpnat, &status);

            /* Convert the count of error elements */
            nmde_p1 += GCngetlong(nmde_p1, &gca_l_e_element, &status);
	    mde->p2 += GCA_PUTI4_MACRO( &gca_l_e_element, mde->p2 );

	    /*
	    **  Loop through the er_data to convert the er_data
	    **  that is in network standard format to the
	    **  local format.
	    */

	    for (i = 0; i < gca_l_e_element; i++)
	    {
		/* gca_id_error */
                nmde_p1 += GCngetlong(nmde_p1, &gnrc_status[i], &status);
		mde->p2 += GCA_PUTI4_MACRO( &gnrc_status[i], mde->p2 );

		/* gca_id_server */
                nmde_p1 += GCngetlong(nmde_p1, &tmplong, &status);
		mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );

		/* gca_server_type */
                nmde_p1 += GCngetlong(nmde_p1, &tmplong, &status);
		mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );

		/* gca_severity */
                nmde_p1 += GCngetlong(nmde_p1, &tmplong, &status);
		mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );

		/* gca_local_error */
                nmde_p1 += GCngetlong(nmde_p1, &tmplong, &status);
		mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );
			  	    
		/* gca_l_error_parm (evidently always 1) */
                nmde_p1 += GCngetlong(nmde_p1, &tmplong, &status);
		mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );

		/*
		**	Convert the components of the gca data value,
		**	type, precision, length of the value and the
		**	value.
		*/
				    
		/* gca_error_parm[0].gca_type */
                nmde_p1 += GCngetnat(nmde_p1, &tmpnat, &status);
		mde->p2 += GCA_PUTI4_MACRO( &tmpnat, mde->p2 );
	
		/* gca_error_parm[0].gca_precision */
                nmde_p1 += GCngetnat(nmde_p1, &tmpnat, &status);
		mde->p2 += GCA_PUTI4_MACRO( &tmpnat, mde->p2 );

		/* gca_error_parm[0].gca_l_value */
                nmde_p1 += GCngetnat(nmde_p1, &tmpnat, &status);
		mde->p2 += GCA_PUTI4_MACRO( &tmpnat, mde->p2 );

		/* gca_error_parm[0].gca_value */
                nmde_p1 += GCngets(nmde_p1, tmpnat, mde->p2);
		mde->p2 += tmpnat;

	    }

            /* Free up our temporary buffer */
            MEfree(nmde_app);
	}

	break;
    } /* end case P_U_ABORT */

    default:
	{
	    /*
	    ** Unknown primitive ID.  No error codes are to be included in the
	    ** GCA_ER_DATA object. This is not considered an error condition.
	    */

	    break;
	}
    } /* end switch */

    if (log)
    {
	/*
	** Log out the error.
	*/

	for (i = 0; i < gca_l_e_element; i++)
	{
	    gcc_er_log(&gnrc_status[i],
		       (CL_ERR_DESC *)NULL,
		       mde,
		       (GCC_ERLIST *)NULL);
	} /* end for */
    } /* end if */

    if (resolve)
    {
        /*
        ** Now construct the message.  The list of error codes is in
        ** gnrc_status[] and the count is in gca_l_e_element.  For all
        ** error codes, mask off the message level contained in the high order 
        ** byte. 
        */

	mde->p1 = mde->p2 = mde->papp;

	/* gca_l_e_element */
	mde->p2 += GCA_PUTI4_MACRO( &gca_l_e_element, mde->p2 );

	for (i = 0; i < gca_l_e_element; i++)
	{
	    i4		tmpnat, tmplong;
	    char	*id_error, *local_error;
	    char	*tmpp2;

	    /*
	    ** In what follows, it is useful to have available the data 
	    ** structures GCA_ER_DATA and GCA_E_ELEMENT, from GCA.H.  
	    ** The code is built upon the contents of these 2 structures.
	    ** 
	    ** The structure contents is protocol dependent.  SQLSTATE
	    ** values are supported at protocol level 60 and above and
	    ** are encoded into the gca_id_error element.  Generic error
	    ** codes were introduced at protocol level 3.  GCC utilizes 
	    ** the generic error code E_GE9088_COMM_ERROR which is alo
	    ** placed in the gca_id_error element.  In both these cases,
	    ** the actual error code is inserted in the gca_local_error 
	    ** element.  Prior to protocol level 3, the  actual error
	    ** code is placed in the gca_id_error element and the
	    ** gca_local_error element is unused.
	    **
	    ** The gca_severity element carries various flags, and is 
	    ** set to 0 to indicate that this is an "unformatted" message, 
	    ** i.e., it is in the raw format returned by ule_format.  The 
	    ** gca_l_error_parm element contains the count of parameters 
	    ** which follow.  There is always exactly 1, i.e. the text 
	    ** returned from ule_format. 
	    */

	    /* gca_id_error - will be updated later */
	    id_error = mde->p2;
	    tmplong = 0;
	    mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );

	    /* gca_id_server */
	    tmplong = 0; 
	    mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );

	    /* gca_server_type */
	    tmplong = 0; 
	    mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );

	    /* gca_severity */
	    tmplong = GCA_ERFORMATTED & 0xFFFFFF;
	    mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );

	    /* gca_local_error - will be updated later */
	    local_error = mde->p2;
	    tmplong = 0; 
	    mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );

	    /* gca_l_error_parm */
	    tmplong = 1; 
	    mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );

	    /* gca_error_parm[0].gca_type */
	    tmpnat = DB_CHA_TYPE;
	    mde->p2 += GCA_PUTI4_MACRO( &tmpnat, mde->p2 );

	    /* gca_error_parm[0].gca_precision */
	    tmpnat = 0;
	    mde->p2 += GCA_PUTI4_MACRO( &tmpnat, mde->p2 );

	    /*
	    ** Invoke gcu_erfmt to translate the error code into text and place
	    ** it in the buffer.  None of the error codes have parameters in 
	    ** the message which corresponds to it.  Each error goes into a 
	    ** gca_error_parm, which has a type, precision and length part.
	    ** The type is DB_CHA_TYPE.
	    */

	    tmpp2 = mde->p2 + sizeof( i4 );

	    gcu_erfmt( &tmpp2, mde->pend, 1,
		       GCC_LGLVL_STRIP_MACRO( gnrc_status[i] ),
		       NULL, 0, NULL );

	    /* gca_error_parm[0].gca_l_value */
	    tmplong = tmpp2 - mde->p2 - sizeof( i4 );
	    mde->p2 += GCA_PUTI4_MACRO( &tmplong, mde->p2 );
	
	    /* gca_error_parm[0].gca_value */
	    mde->p2 += tmplong;

	    /*
	    ** Go back and fixup the error code and SQLSTATE values
	    ** based on the protocol level.
	    */

	    if ( ccb->ccb_hdr.gca_protocol >= GCA_PROTOCOL_LEVEL_60 )
	    {
		/*
		** Encoded SQLSTATE in gca_id_error,
		** error code in gca_local_error.
		*/

		/* gca_id_error */
		tmplong = ss_encode( SS50000_MISC_ERRORS );
		GCA_PUTI4_MACRO( &tmplong, id_error );

		/* gca_local_error */
		tmplong = GCC_LGLVL_STRIP_MACRO(gnrc_status[i]);
		GCA_PUTI4_MACRO( &tmplong, local_error );
	    }
	    else  if ( ccb->ccb_hdr.gca_protocol >= GCA_PROTOCOL_LEVEL_3 )
	    {
		/*
		** Generic error in gca_id_error, 
		** error code in gca_local_error.
		*/

		/* gca_id_error */
		tmplong = E_GE9088_COMM_ERROR;
		GCA_PUTI4_MACRO( &tmplong, id_error );

		/* gca_local_error */
		tmplong = GCC_LGLVL_STRIP_MACRO(gnrc_status[i]);
		GCA_PUTI4_MACRO( &tmplong, local_error );
	    }
	    else
	    {
		/*
		** Error code in gca_id_error, gca_local_error unused.
		*/

		/* gca_id_error */
		tmplong = GCC_LGLVL_STRIP_MACRO(gnrc_status[i]);
		GCA_PUTI4_MACRO( &tmplong, id_error );

		/* gca_local_error */
		tmplong = 0; 
		GCA_PUTI4_MACRO( &tmplong, local_error );
	    }
	}
    }

    return(status);
} /* end aerr */

/*{
** Name: gcc_asnd	- GCA_SEND a message
**
** Description:
**      Invoke GCA_SEND to send the message contained in the MDE.
**
** Inputs:
**      mde                             Pointer to an MDE for the message
**
** Outputs:
**  	none
**
**	Returns:
**	    OK
**	    FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-Dec-88 (jbowers)
**          Created
**      16-Jan-89 (jbowers)
**	    Changes to accommodate normal and expedited GCA_SEND's.
**      17-Aug-89 (cmorris)
**          If the GCA_FORMAT fails, decrement the async count.
**      14-Nov-89 (cmorris)
**          Don't log errors that the FSM's going to log anyway. Return 
**          generic and system status.
**  	19-Jan-93 (cmorris)
**  	    Instead of returning error, abort the association.
**	22-Nov-95 (gordy)
**	    Set the gca_formatted flag for send/receive at API level 5.
**	21-Feb-97 (gordy)
**	    Changed gca_formatted flag to gca_modifiers.
**	25-Feb-97 (gordy)
**	    Set end-of-group on GCA_SEND so indicator flows between
**	    client and server.
*/
static STATUS
gcc_asnd( GCC_MDE *mde)
{
    GCA_SD_PARMS	*sds_parms;
    STATUS		call_status = OK;
    STATUS		status = OK;
    STATUS  	    	generic_status;
    GCC_CCB		*ccb = mde->ccb;

    /*
    ** Issue GCA_SEND to send the message received in a P_DATA, P_RELEASE
    ** or P_EXPEDITED_DATA indication.
    ** 
    ** The GCA message header is constructed in the PCI area, since we are
    ** finished with it when we are ready to issue the GCA_SEND.
    **
    ** The MDE becomes an A_COMPLETE RQS.
    */

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 2 )
	TRdisplay( "gcc_asnd: sending msg %s length %d flags %x\n", 
		    gcx_look( gcx_getGCAMsgMap(), 
    	    	              mde->mde_srv_parms.al_parms.as_msg_type ),
		    (i4)(mde->p2 - mde->p1), 
    	    	    mde->mde_srv_parms.al_parms.as_flags );
# endif /* GCXDEBUG */

    switch (mde->mde_srv_parms.al_parms.as_msg_type)
    {
    case GCA_ATTENTION:
    case GCA_NP_INTERRUPT:
	mde->gca_rq_type = GCC_A_E_SEND;
	break;
    default:
	mde->gca_rq_type = GCC_A_SEND;
	break;
    }
    mde->service_primitive = A_COMPLETE;
    mde->primitive_type = RQS;

    /* The GCA buffer header starts in the PCI area */ 

    /*
    ** Invoke GCA_SEND service.
    */

    sds_parms = (GCA_SD_PARMS *)&mde->mde_act_parms.gca_parms;
    sds_parms->gca_association_id = ccb->ccb_aae.assoc_id;
    sds_parms->gca_buffer = mde->p1 - IIGCc_global->gcc_gca_hdr_lng;
    sds_parms->gca_message_type = mde->mde_srv_parms.al_parms.as_msg_type;
    sds_parms->gca_msg_length = mde->p2 - mde->p1;
    sds_parms->gca_descriptor = 0;
    sds_parms->gca_end_of_data = 
		((mde->mde_srv_parms.al_parms.as_flags & AS_EOD) != 0);
    sds_parms->gca_modifiers = 
		(mde->mde_srv_parms.al_parms.as_flags & AS_EOG) 
		? GCA_EOG : GCA_NOT_EOG ;

    call_status = gca_call( &gca_cb, GCA_SEND, 
			     (GCA_PARMLIST *)sds_parms,
			     (i4) GCA_ASYNC_FLAG,	/* Asynchronous */
			     (PTR)mde,		/* async id */
			     (i4) -1,		/* no timeout */
			     &generic_status);
    if (call_status != OK)
    {
        /* Log an error and abort the association */
	gcc_al_abort(mde, ccb, &generic_status, (CL_ERR_DESC *) NULL,
		     (GCC_ERLIST *) NULL);
	status = FAIL;
    }
    return(status);
} /* end gcc_asnd */

/*{
** Name: gcc_aaerr	- Construct the text error messages associated
**                        with an A_ABORT rqst.
**
** Description:
**	This is a copy of gcc_aerr.  It has been introduced to INGRES/net
**	in release 6.3.  The routine is called by the comm. server to
**	lookup and format error messages to be sent back to the client
**	side comm. server when a local A_ABORT rqst has occurred.  Previously
**      all error message resolution was done by the client comm. server. 
**	
**
**     
** Inputs:
**      mde                             Pointer to an MDE for the message
**
** Outputs:
**      none
**
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-Dec-88 (jbowers)
**          Created
**	18-jul-89 (ham)
**	    Copied from gcc_aerr.
**	27-jul-89 (seiwald)
**	    Fix for compiler warnings: & before array.
**      03-Aug-89 (cmorris)
**          Somewhat simplified and renamed.
**      15-Aug-89 (heatherm)
**          Changes for encoding remote error messages.
**      16-Aug-89 (cmorris)
**          Merged in end to end bind changes.
**      25-Aug-89 (cmorris)
**          Fixed bug of using network encoded length as opposed to
**          pre-encoded one.
**      31-Oct-89 (cmorris)
**          Implemented calls to GCnxxx functions to convert individual types
**          from network xfer syntax.
*/
static VOID
gcc_aaerr( GCC_MDE *mde, GCC_CCB *ccb, STATUS generic_status )
{
    GCA_E_ELEMENT	er_elem;
    i4			tmp_gca_l_e_element;
    char		er_buffer[GCC_L_BUFF_DFLT];
    i4			i;
    STATUS		tmp_gnrc_status[GCC_L_GEN_STAT_MDE];
    STATUS		status = OK;

    /* Initialize the MDE. */

    mde->p1 = mde->p2 = mde->papp;

    /* 
    ** We do two different things depending on whether our peer is pre-6.3.
    ** If it is, then we build a GCC_ABT_DATA structure to hand back. If it
    ** isn't, we build not only a GCC_ABT_DATA structure but also a GCA_ER_DATA
    ** structure after it.
    */
    mde->p2 += GCnaddnat(&generic_status, mde->p2, &status);

    if ((ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_61) &&
        (ccb->ccb_aae.a_version != AL_PROTOCOL_VRSN_62))
    {
	/*
	**  This must be 6.3 to 6.3 (or greater version) code.
	**  We will build the messages on the comm. server that
	**  detected the error and convert them to network standard format
	**  and send back to the client comm. server. Set up the generic
        **  status array.
        */
        tmp_gnrc_status[0] = E_GC2205_RMT_ABORT;
        tmp_gnrc_status[1] = generic_status;
        tmp_gca_l_e_element = 2;

        /* Encode the the element count */
        mde->p2 += GCnaddlong(&tmp_gca_l_e_element, mde->p2, &status);

        for (i = 0; i < tmp_gca_l_e_element; i++)
        {
	    char *tmpp2;

	    /*
            ** In what follows, it is useful to have available the data
            ** structures GCA_ER_DATA and GCA_E_ELEMENT, from GCA.H.  The code
            ** is built upon the contents of these 2 structures.
            ** 
	    ** GCC utilizes the generic error code E_GE9088_COMM_ERROR.
            ** This is inserted in the gca_id_error element of each
            ** GCA_E_ELEMENT.  The actual GCC error code is inserted in the
            ** gca_local_error elment. gca_severity carries various flags, 
            ** and is set to 0 to indicate that this is an "unformatted" 
            ** message, i.e., it is in the raw format returned by ERlookup
            ** (we will use ule_format for consistency and simplicity; it
            ** returns the error text as it comes from ERlookup).
            ** The gca_l_error_parm element contains the count of parameters
            ** which follow.  There is always exactly 1, i.e. the text
            ** returned from ERlookup. 
            */
    	    er_elem.gca_id_server = 0;
	    er_elem.gca_server_type = 0;
	    er_elem.gca_id_error = GCC_LGLVL_STRIP_MACRO(tmp_gnrc_status[i]);
            er_elem.gca_local_error = 0;
	    er_elem.gca_severity = GCA_ERFORMATTED & 0xFFFFFF;
	    er_elem.gca_l_error_parm = 1;
	    er_elem.gca_error_parm[0].gca_type = DB_CHA_TYPE;
	    er_elem.gca_error_parm[0].gca_precision = 0;

	    /*
	    ** Invoke ule_format to translate the error code into text and
            ** place it in the buffer.  Use the returned text length to bump
            ** the er_elem pointer.  None of the error codes have parameters
            ** in the message which corresponds to it.  Each error goes into
            ** a GCA_ERROR_PARM, which has a type, precision and length part.
            ** The type is DB_CHA_TYPE.
            */

	    tmpp2 = er_buffer;

	    gcu_erfmt( &tmpp2, er_buffer + sizeof( er_buffer ), 1,
		       GCC_LGLVL_STRIP_MACRO( tmp_gnrc_status[i] ),
		       NULL, 0, NULL );

	    er_elem.gca_error_parm[0].gca_l_value = tmpp2 - er_buffer;

	    /*
	    **	Convert the elements of the er_element and put them
	    **	into the mde. Convert the id error
            */
            mde->p2 += GCnaddlong(&er_elem.gca_id_error, mde->p2, &status);

	    /* Convert the server id */
            mde->p2 += GCnaddlong(&er_elem.gca_id_server, mde->p2, &status);

	    /* Convert the server type */
            mde->p2 += GCnaddlong(&er_elem.gca_server_type, mde->p2, &status);
			  	    
	    /* Convert the severity */
            mde->p2 += GCnaddlong(&er_elem.gca_severity, mde->p2, &status);

	    /* Convert the local error */
            mde->p2 += GCnaddlong(&er_elem.gca_local_error, mde->p2, &status);

	    /* Convert the length error parm */
            mde->p2 += GCnaddlong(&er_elem.gca_l_error_parm, mde->p2, &status);

	    /*
	    **	Convert the commponents of the gca data value,
	    **	type, precision, length of the value and the
	    **	value.
	    */
            mde->p2 += GCnaddnat(&er_elem.gca_error_parm[0].gca_type,
                                  mde->p2, &status);
            mde->p2 += GCnaddnat(&er_elem.gca_error_parm[0].gca_precision,
                                  mde->p2, &status);
            mde->p2 += GCnaddnat(&er_elem.gca_error_parm[0].gca_l_value,
                                  mde->p2, &status);
            mde->p2 += GCnadds( er_buffer,
                                er_elem.gca_error_parm[0].gca_l_value,
                                mde->p2 );
        }
    }
} /* end aaerr */

/*{
** Name: gcc_al_abort	- Application Layer internal abort
**
** Description:
**	gcc_al_abort handles the processing of internally generated aborts.
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
**  	02-Jan-91 (cmorris)
**  	    Don't stick status into MDE; on internal aborts we
**  	    just issue a disassociate to GCA and a user abort
**  	    with no user data to the presentation layer.
**  	15-Jan-93 (cmorris)
**  	    Always reset p1, p2 before processing internal abort.
**  	    The pointers could be set to anything when the abort
**  	    occurred.
**	13-Jul-00 (gordy)
**	    Logging of association aborts was removed because of timing
**	    problems (expected aborts during disconnect were being logged).
**	    The problem was that logging was being done in gcc_gca_exit()
**	    before the correct state was established in the AL FSM.  Move
**	    logging to OAPAU for A_ABORT and A_P_ABORT.  OAPAU is not
**	    executed during shutdown so only real aborts will be logged.
**	26-oct-2000 (somsa01)
**	    We should be setting the contents of generic_status to OK, not
**	    the pointer itself.
*/
static VOID
gcc_al_abort( GCC_MDE *mde, GCC_CCB *ccb, STATUS *generic_status, 
	      CL_ERR_DESC *system_status, GCC_ERLIST *erlist )
{
    /* Make sure there's actually a CCB */
    if (ccb == NULL)
    {
	/* Free up the mde and get out of here */
	gcc_del_mde( mde );
	return;
    }

    if (!(ccb->ccb_aae.a_flags & GCCAL_ABT))
    {
	/* Log an error */
	if ( mde == NULL  ||  erlist != NULL )
	{
	    gcc_er_log(generic_status, system_status, mde, erlist);
	    *generic_status = OK;	/* only log once */
	}

	/* Set abort flag so we don't get in an endless aborting loop */
        ccb->ccb_aae.a_flags |= GCCAL_ABT;    

        /* Issue the abort */
    	if (mde != NULL)
    	{
    	    mde->service_primitive = A_P_ABORT;
	    mde->primitive_type = RQS;
	    mde->mde_generic_status[0] = E_GC2210_LCL_ABORT;
	    mde->mde_generic_status[1] = *generic_status;
            (VOID) gcc_al(mde);
    	}
    }

}  /* end gcc_al_abort */


/*
** Name: gcc_init_mask
**
** Description:
**	Initialize pseudo-random binary mask from key.
**
** Input:
**	key	
**	len	Number of bytes in mask.
**
** Output:
**	mask	Binary mask.
**
** Returns:
**	void
**
** History:
**	15-Jul-04 (gordy)
**	    Created.
*/

#define	GCC_MSK_HASH(hash, chr)	( (((u_i4)(hash) << 4) ^ ((chr) & 0xFF)) \
				  ^ ((u_i4)(hash) >> 28) )
#define	GCC_MSK_MOD( val )	((val) % 714025L)
#define GCC_MSK_SEED( seed )	GCC_MSK_MOD(seed)
#define	GCC_MSK_RAND( rand )	GCC_MSK_MOD((rand) * 4096L + 150889L)

static void
gcc_init_mask( char *key, i4 len, u_i1 *mask )
{
    u_i4	rand, carry, i;
    u_i4	seed = 0;

    for( i = 0; key[i]; i++ )  seed = GCC_MSK_HASH( seed, key[i] );
    rand = GCC_MSK_SEED( seed );

    for( i = 0; i < len; i++ )
    {
	carry = rand;
	rand = GCC_MSK_RAND( rand );
	mask[i] = (u_i1)((rand >> 8) ^ carry);
    }

    return;
}

/*
** Name: gcc_sess_abort
**
** Description:
**	 abort selected sesission 
**
** Input:
**	ccb 	session ccb 
**
** Output:
**	void 
**
** Returns:
**	status 
**
** History:
**	15-Oct-04 (wansh01)
**	    Created.
*/
STATUS
gcc_sess_abort( GCC_CCB * ccb )
{
    	GCC_MDE	    *abort_mde;
	STATUS 	    status; 


	if( !( abort_mde = gcc_add_mde( ccb, 0 ) ) )
	{
	    /* Log an error and abort the association */
	    status = E_GC2004_ALLOCN_FAIL;
	    gcc_al_abort(abort_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL);
	    return (FAIL);
	}

	abort_mde->ccb = ccb;
	abort_mde->service_primitive = A_ABORT;
	abort_mde->primitive_type = RQS;
	abort_mde->mde_generic_status[0] = E_GC200E_ADM_SESS_ABT; 

    	gcc_al ( abort_mde );
	return ( OK );
}
