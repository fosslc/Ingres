/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
#include    <cv.h>
#include    <cs.h>
#include    <er.h>
#include    <ex.h>
#include    <tm.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <gca.h>
#include    <lk.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sceshare.h>	/* must be before sce.h */
#include    <sce.h>
#include    <scfcontrol.h>

/**
**
**  Name: scenotify.c - SCF alert notification/state procedures for a session
**
**  Description:
**      This module contains the procedures required to notify a client
**	of an alert.  Everything else has been done and now the session
**	must transmit the alert to the client.  Refer to SCE module for
**	the rest of the server alert managent.
**
**	NOTE: The introductory (and large) comment in this file is
**	*required* reading for anyone who wants to modify or add alert
**	states, or who is interested in adding more cases to the sending
**	and receiving of GCA messages.
**
**  Trace Points Supported in module (use SET TRACE TERMINAL):
**	All trace points in this module MUST go through TRdisplay, because
**	the routines in this module may be called after all normal data has
**	been written to the client and we cannot then add more user trace data
**	as it may cause the client/server-session to hang in write-write
**	deadlock:
**
**	SC5			Trace session notification of events.
**	SC6			Trace alert-state transitions.
**
**  Routines:
**	sce_chkstate	- Check/set the alert notification state
**	sce_trstate   	- Trace alert state transitions.
**	sce_notify    	- Queue a GCA alert message to a registered client
**
**  Internal Routines:
**	sce_gcevent	- Package GCA_EVENT message and queue.
**
**  History:
**	21-mar-90 (neil)
**	    First written to support Alerters.
**	13-jul-92 (rog)
**	    Included ddb.h and er.h.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	29-Jun-1993 (daveb)
**	    include <tr.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	1-Sep-93 (seiwald)
**	    CS option cleanup: csdbms.h is no more.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types/pointer sized types. We need to distinguish 
**	    this on, eg 64-bit platforms, where sizeof(PTR) > sizeof(int).
**	30-May-1995 (jenjo02)
**	    Replaced CSp|v_semaphore statements with function calls
**	    to sce_mlock(), sce_munlock().
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Dec-2000 (horda03) Bug 103596 INGSRV 1349
**          sscb_last alert and sscb_num_alerts now used to identify the
**          last alert in the queue and the number of alerts in the queue
**          respectively.
**	30-Nov-2006 (kschendel) b122041
**	    Fix ult_check_macro usage.
**/

/* #ifdef SCE_INTRO_COMMENT

    History:
	21-mar-90 (neil)
	   Written to explain what I'm doing.


		Alert State Management and their Interaction with
		-------------------------------------------------
		    the Sequencer, SCC and CS Flow
		    ------------------------------

    INTRODUCTION:
    ------------

    This note describes the modifications made to the external sequencer
    flow when notifying clients of alerts.  These modifications affect 
    SCS and SCC, but take advantage of existing CS and GCA flow.  Note
    that this comment describes a total of about 60 lines of carefully
    placed code.  The problem is a complex one, the solution is a
    relatively simple one.

    Alerts are attached to a special queue in the session control block by
    the event thread (a new thread introduced for the alerters project).
    The event thread checks if the session is interruptable and, if it is,
    issues a CSattn to that session when attaching the alert.  The setting
    and unsetting of the session's "alert interruptable state" is carefully
    coded in the CS "about to read" routine, scc_recv, and in this module.
    Typically, if we are about to turn around to start waiting for a client
    query (or continue waiting for one) we want to make sure that the event
    thread interrupts this "waiting" if an alert arrives.  Race conditions
    associated with state changes are closed using a correctly placed
    CScancelled call and/or semaphores.

    Let's assume that a thread was "alert-interrupted" by the event thread.
    It would have been interrupted while it was "waiting" for user input.
    This thread will then be made runnable, will return to the sequencer
    where we will process any queued alerts on behalf of the registered
    client.  This specialized thread "resumption" is much like a thread
    that resumes because of an interrupt of a force-abort.  Within the 
    sequencer the thread will notice that it was woken up to process alerts
    and return CS_EXCHANGE.  CS uses this status to first call the CS
    "write" routine, scc_send, and follow all writes with another read.
    When scc_send is called in this context, it notices that it has alerts
    queued, converts the next alert to a GCA alert message (GCA_EVENT) and
    sends (GCA_SEND) that message (GCA_EVENT) to the client.  When writing
    one of these special alert messages, that is not in the context of
    typical query results, a special return status is returned from the
    CS "write" routine, scc_send/E_CS001D_NO_MORE_WRITES, so that we can
    resume the original read operation from which we were interrupted.
    Obviously, this flow is somewhat of an extension to the current model
    and introduces a few problems (like the interrupt and force-abort
    counterparts do).  These problems can all be summarized as a collection of
    GCA- and CS-related race conditions, as well as some GCA functionality
    that is documented to work but is not certain to work on all environments.

    For example, what happens when the event thread interrupts the thread
    when we are just about to process user input - this can be resolved with a 
    simple state change (and the closure of a few race conditions with a
    semaphore and call to CScancelled).

    Another problem is how to handle alerts sent to the client as a result
    of alert notification, and NOT in the context of query results.  Do 
    we differentiate between those and ones sent with query results?  Of
    course, we must (and ones sent with query results have no special
    "privileges"), but it is not obvious before reading on.

    All of this, and more, is discussed (in some detail) below.

    Dev Note: Make sure to read AND understand these notes before modifying
	      the value of the session alert state (sscb_astate) - it is
    this alert state that is the key to controlling when different
    event-related sequencer operations may be scheduled and when various
    completion routines may be driven.

    EXTERNAL SUMMARY:
    ----------------

    The following external description is extracted out of the Alerters
    Product Specification.  This are described here to emphasize some key
    requirements of the alerters feature:

    [E1] A client must be able to issue the ESQL GET EVENT statement without
	 polling the DBMS.  This feature is described in the external 
    specification of events.  The GET EVENT statement is not a query
    and, thus, the client must be able to read data without ever having
    sent a query.  For example, a monitor may REGISTER for the notification
    of a few alerts and then turn around and just issue the GET EVENT 
    statement in a loop (with and without timeout specifications) until the
    monitor program ends, without any queries in between.

    [E2] A client must also be able to issue queries interleaved with the 
	 reception of alerts.  For example, each time the monitor receives
    and event (through the GET EVENT statement) it may issue some query to
    update a table.  Thus, subsequent alerts may arrive just before, during
    or after the query is issued.  The point at which the event thread
    notifies the target thread, and the target thread's reaction to those
    alerts is the complex problem we are trying to solve by the state changes.

    [E3] This document does *not* address environments where CS calls the
	 read and write routines with the "sync" flags set.

    PRE-ALERT FLOW SUMMARY:
    ----------------------

    Before trying to describe the problem in any more detail it would be
    helpful to understand how the DBMS server works with a client and how
    the DBMS works with CS prior to the introduction of alerters (in very
    general terms).

    When thinking of a client front-end and DBMS server the following
    schema can help (it is somewhat different for COPY processing):

	Client				    DBMS
	------				    -----

	Work				    Nothing
					    Wait on Read (suspend thru CS)
	Send Query to DBMS -------------->  Read  completes
					    +- Enter Sequencer -----+
					    |  Process	            | (loop)
					    |  Write to Client      |
	Read Results       <--------------  +- CS suspend on Write -+

    
    Within the server itself it is useful to understand the interface
    between CS (the dispatcher), SCS (the sequencer) and SCC (sequencer
    communication support routines).  Typically, the session suspends
    reading (after scc_recv), gets input and enters the sequencer, after
    which it usually then writes everything it has to the user (and
    suspends after each scc_send).  I say "usually" as the sequencer may
    notice that it only read one small part of a large query and needs to
    read more.  The following is a conceptual outline of the CS loop that
    controls this flow (this is part of CS_setup elaborated for the 
    sequencer):

	mode = CS_INPUT;
	while (true) 				-- loop until termination
	{
	    if (mode = CS_INPUT)
	    {
		status = scc_recv();		-- post read
		if (status == OK)
		    status = CSsuspend();
		check for termination;
	    }
	    scs_sequencer(mode, &next_mode);
	    check for termination;
	    if (next_mode == CS_OUTPUT || CS_EXCHANGE)
	    {
		do
		{
		    status = scc_send();	-- post write
		    if (status == OK)
			status = CSsuspend();
		} while (status != OK);
		check for termination;
	    }
	    if (next_mode = CS_EXCHANGE)	-- just a short cut
		mode = CS_INPUT;
	    else
		mode = next_mode;
	}

    The CS_EXCHANGE mode is the "typical" mode that is returned when all 
    the user results have been queued for the session and we want CS to
    cause all of them to be returned to the user.  The CS_OUTPUT mode
    is used when, in order to complete the actual request, we must return
    to the sequencer to continue processing (i.e. middle of a SELECT loop
    or COPY statement).

    With the above loop (and short explanation) in mind a lot of processing
    can be understood.  In particular processing that involves multiple
    messages and possible return to the sequencer (such as SELECT loops,
    COPY processing, etc.) Alerts follow the above model but modify some of
    the internal workings of various SCC routines (scc_send, in particular)
    to allow clients to read data without having to issue a query (a key
    requirement of this feature).

    HIGH LEVEL FACT SUMMARY:
    -----------------------

    Given the description above the following is a summary of various
    "facts" that must always be taken into account when thinking about the
    "sending-of-alert-messages-to-user" problem.  None of these "facts"
    assume a particular solution:

    [F1] A session must be made interruptable (for alerts) so that it may
	 be woken up to process events.  This would occur when it is
    sitting waiting for a query.  This is to allow alerts to be sent to a
    program as soon as possible, in particular for programs that are just
    reading alerts (GET EVENT), such as event monitors. 

    [F2] The session can be in a pending-read state (for a query) when it
	 is made runnable to process and "send" alerts.  It is exactly 
    during this time that various problems arise, described later.  And,
    it is during this time when it is most common for alerts to be sent.

    [F3] Alerts that the event thread attaches while the target thread is
	 processing a query are no problem.  These alerts are eventually
    added as GCA messages that are sent to the client with the rest of the
    query results, like trace messages but in front of query results (just
    for faster availability).  Since these are no problem they are not
    really discussed here.  In fact, if query results were the only time
    we sent already queued alerts to the client this design note would not
    be necessary.

    [F4] GCA Alert messages, because of their size and nature, may not be sent
	 on the GCA expedited channel.  This was agreed upon during a
    design review for "GCA alert message sending".

    [F5] When the session is sending a GCA alert message to the client that
	 is not in the context of query (see [F3] above for *in* query
    context), the session MUST NOT suspend after the send operation.  Why?
    If we suspend waiting for the client to read the just-sent alert message,
    we may cause a write/write deadlock with the client who is really trying
    to send the DBMS a query to process.  Even though the client may intend to
    turn around and read the alert message, we have to be able to read the
    query first to allow them to read the alert (if the client is synchronous 
    this is even more critical).  The CS "write" routine (scc_send) return
    status, E_CS001D_NO_MORE_WRITES, comes in handy for this "no-suspend" case.
    This is all described later.  As mentioned above [F3] when alert messages
    are sent in the context of query results we want to treat them like normal
    data and they follow the regular query result processing.

    [F6] When the sending of a GCA alert message (that is not in the
	 context of query results) completes, we always want to return to
    the sequencer to send more (if there are any more alerts) and to clean
    up (if there are no more alerts).  This return to the sequencer also
    plays an important role in the modification of newly introduced session
    alert state that each session has associated with alerts.  (This fact
    is somewhat dependent on the solution below).

    [F7] If an earlier sending of a GCA alert message completes while we
	 we are processing a query we must inhibit the GCA completion
    routine for that alert (so as not to conflict with later CSsuspend
    calls for other resources).  Why?  CSresume calls are sort of "queued"
    and saved for a particlar thread - a subsequent CSsuspend to an orphan
    CSresume notification will immediately resume.  Note that completion
    routines associated with single GCA calls, and GCA_NO_ASYNC_EXIT flags
    do not modify the solution.

    [F8] When a session is invoked to process a query, an alert may already
	 be sent to the client (though may not necessarily have been read).
    We make no attempt to flush all queued alerts to the client, as they
    may not yet be ready to read them.  Any already queued alerts (and those
    that are raised during this session's query processing) will eventually
    be transmitted to the client with the GCA query results.  This was done
    so as not to complicate the problem we already have.  Eventually this
    may be modified as a performance enhancement.

    [F9] Whenever possible we should try to get the alert to the client
	 without it being purged by GCA.  Consequently, between the time
    that a user interrupt arrives and the time that the GCA_IACK message
    is sent to the client, no queued alerts should be transformed into
    GCA alert messages (they will be purged by SCC).  This is just a
    performance enhancement, and references to GCA_IACK can be ignored
    when trying to understand the total solution.

    PROBLEMS TO DEAL WITH:
    ---------------------

    This section attempts to summarize the key problems that I am trying to
    solve.  All of the problems arise from the requirement that we must
    send the alert to the client outside of a query-result context.
    Later in this note is an "alert state" description.  This state solves
    these problems and more:

    [P1] Concurrent sending of an alert message and the reading of a query
	 is a complex problem.  This is the key situation that we are trying
    to resolve in this design note.  Typically, a client alert monitor will
    be reading alerts and for each event will be issuing some kind of query.
    While this query is running another alert may arrive.  The arrival of
    the alert may be just before, during or after the processing of the
    query in the sequencer.  Note that alerts that arrive during query
    processing are no problem - they are sent off with the query results.

    [P2] Concurrent writes by the DBMS must be avoided.  We have to make
	 sure not to send more than one message to the client.  This problem
    must be avoided for multiple alerts (i.e. we must serialize sending each
    individual alert to the client) and for alerts followed by query results
    (an alert that is pending must make sure to complete before we can send
    any query results).

    [P3] Sending alerts to the client uses GCA (and its completion exit
	 scc_gcomplete).  Typically this completion routine resumes the
    thread that was issuing the operation.  As mentioned above, we do not
    suspend the thread on the send of the alert message when it is outside
    of query result context.  Consequently, the CSresume normally driven by
    the GCA completion exit (scc_gcomplete) may "land" us in unexpected
    contexts if allowed to execute while we are inside the sequencer.  For
    example, we may already have advanced into query processing when the
    previous sending of the alert message completes.  We need to inhibit
    the CSresume call to avoid leaving the CS "event done" flag set
    (CS_EDONE_MASK) - this would cause the next CSsuspend to immediately
    complete, probably without the necessary resource (i.e. a requested lock).
    On the other hand, if no query is happening and the client does read
    the alert, we would like to wake up the session so that it can continue
    with the next alert (if there are any).

    [P4] Most of the solution is based on a new session alert state field
	 (sscb_astate).  The modification of this field must be done under
    strict control, and its various settings will guarantee that the session
    is processing alerts as expected.  This state is synchronized with
    the SCS query processing state, the GCA send & receive message buffer
    states, and with the current queue of session-owned alerts.

    OUTLINE OF ALERT FLOW OPERATIONS:
    --------------------------------

    It is useful to follow this alert flow description while thinking of the
    CS_setup control flow described earlier:

	Session is initially a pending read state - suspended after scc_recv
	    and marks itself alert-interruptable;
	Event thread notices session is alert-interruptable, interrupts the
	    session (CSattn), and marks it alert-notified;
	Session wakes up from suspend operation;
	Session enters sequencer;
	From sequencer session acknowledges the interrupt notification;
	Sequencer calls scs_input and notices GCA read still IN_PROCESS;
	Since no input found control is returned to CS with mode CS_EXCHANGE;
	CS calls the write routine - scc_send;
	scc_send notices that there are alerts and GCA_SEND's them;
	scc_send notices that there are no other queued messages and
	    decides that this must be an alert out-of-query-context;
	scc_send returns a status (NO_MORE_WRITES) which does not suspend
	    the GCA_SEND;
	CS changes to next mode (CS_EXCHANGE->CS_INPUT) and returns control
	    to scc_recv;
	scc_recv continues to wait until the next GCA completion that drives
	    scc_gcomplete.  Of course, the completion could have occurred
	    immediately after the send - this does not matter.  Also the
	    completion could be that of the just-sent alert message or the
	    outstanding read;
	CS returns control to the sequencer and processing continues.
	    This processing may be a query (with or without an alert
	    message send still pending), or may be to get the next alert
	    (as when the alert send completed and the read-for-query is still
	    pending).

    One can picture the flow between the sequencer, scc_send and scc_recv as
    the following (assume no queries while alerts being sent and read by
    client):

	sequencer--->scc_send (do not suspend)--->scc_recv (suspend)
	   ^				     	      |
	   |					   CSresumes when alert
	   |					   is read by client (**)
	   |					      |
	   |				     	      Y
	   +--------------------<---------------------+

	(**) After the last alert is processed, the posting of the next read
	     will suspend until the next alert or a user query.


    SESSION ALERT STATE SUMMARY (sscb.sscb_astate/SCS_ASxxxx):
    ---------------------------------------------------------

    This section describes a single state field, its values and how they
    are modified in order to solve, what seems to be a concurrency
    problem.  Modification of these states must be carefully controlled.

    One state switch is controlled by a semaphore (this is only done
    because it is a "shared" state between the event thread and the session
    and includes access to shared resources - the session alert queue).

    Some state modifications require the closing of potential race
    considerations of events that occur during the state change.  For example,
    when we change from state "alert send pending" to "ignore GCA completions"
    (described in more detail below), we must avoid any GCA completions that
    occurred just before (or during) the state change.  For example:

	if (starting_query & alert_state = alert_GCA_send_pending) then
	    alert_state = ignore_GCA_completion;
	    CScancelled();		-- in case it just completed
	end if;
	continue processing query;
    
    States are modified by the sce_chkstate routine on behalf of the 
    sequencer and the SCC routines (scc_recv and scc_write).  State
    transitions can be traced by using the trace point SC6 (SCS_TASTATE):

	SET TRACE TERMINAL;
	SET TRACE POINT SC6;

    New states added must be reflected in this comment and in the state
    transition tracing routine, sce_trstate.

    SCS_ASDEFAULT
    
	This state is the zero state.  Nothing is happening or everything
	has already happened.  It is normally set when no alerts were sent
	to the client and there are no pending alerts to be processed.
	When this state is set alerts may be queued (by the event thread)
	but these alerts are handled as query results (for example, if
	alerts were queued while in the middle of processing queries).

    SCS_ASWAITING
    
	This is the alert-interruptable state.  This state is set by
	scc_recv, when it "knows" that it can go into the "wait for query"
	mode (or the "continue to wait for query" mode) and that there are
	no alerts on the session's alert queue.  This state is set for the
	event thread to notice and indicates that this session wants to be
	woken up when an event arrives.  When the event thread does detect
	this state it will modify the state to "notified of alert",
	SCS_ASNOTIFIED, after it adds the alert to the session alert queue.
	The setting to this state, and the transition from this state to
	the next state is controlled by a semaphore because it relies on a
	resource shared by the event thread and a session (the alert queue).
	When the sequencer is entered with the SCS_WAITING state the state
	is modified to SCS_DEFAULT.  Thus, a session that is not registered
	for any alerts will toggle its state between SCS_DEFAULT and
	SCS_WAITING.

    SCS_ASNOTIFIED
    
	This state is set by the event thread after it finds a session that
	is in SCS_ASWAITING state and that needs to have an alert queued.
	The event thread adds the alert to the session alert queue and then
	calls CSattn.  After that it sets the alert state to SCS_ASNOTIFIED.
	When the sequencer is re-entered in this state it knows that it must
	acknowledge the CSattn call and set the state to the initial default,
	SCS_ASDEFAULT.  Note that if the sequencer is entered in the
	SCS_WAITING state it also acknowledges interrupts to eliminate the
	race condition where the event thread "just" notified it but before
	it could change the state.

    SCS_ASPENDING
    
	This state is the follow state of SCS_ASNOTIFIED.  This state would
	occur if the thread was woken up to send events to the client (as a
	result of SCS_ASWAITING being modified to SCS_ASNOTIFIED, and upon
	re-entry alerts were found queued).  This state is not set if
	alerts arrived with or during a query request.  The way this state is
	set is the following:
	
	The sequencer will notice there is no input and there are alerts
	and return to CS with mode CS_EXCHANGE.  This mode indicates to CS
	to call scc_send.  When scc_send is entered it notices that there
	are no other queued messages, and there are alerts queued - this
	implies that it must have been called to send the alert by itself -
	and scc_send then sets the state to SCS_ASPENDING.  The single
	alert is sent and control returns to CS which then continues with
	the pending read (which may or may not have completed).  Upon
	completion of the GCA_SEND operation of the alert message, the
	thread would be made runnable and will return to the sequencer
	to process the next alert through scc_send.  This state remains set
	until either all alerts are sent (and then is modified to
	SCS_ASDEFAULT) or, if during the "send phase" of an alert message,
	a pending read-of-a-query-request completed (in this case the state
	is modified to SCS_ASIGNORE - described next).
	
	Note that this "pending" state implies that there are either:
	    (1) more alerts to process in the session alert queue, or
	    (2) there is a pending GCA_SEND of an alert message (that may
		not yet have completed).
	While in this state we may send at most one alert message to the
	client.  During this state we also may be returned to "session wait
	for query" state, which will complete after the sending of the alert
	message completes or after a read completes.  This state (together
	with SCS_ASIGNORE) is the most complex of alert states and is
	checked in a variety of places:
	    scc_send 	Sets SCS_ASPENDING if alert is only thing on
			SCC queue (must have been an alert notification).
			Upon entry scc_send always resets back to
			SCS_ASDEFAULT to take care of all other cases.
			If in SCS_ASPENDING state when exiting scc_send
			returns E_CS001D_NO_MORE_WRITES -  this avoids
			suspending on the special send of the alert message.
	    scc_recv 	If it finds queued alerts when about to read it
	    	        does not return E_CS001E_SYNC_COMPLETION if the
			state is SCS_ASPENDING.  E_CS001E_SYNC_COMPLETION
			is normally used for alerts to allow scc_recv to
			get the session back into the sequencer as soon as
			possible to process alerts.  In the "pending"
			state it is important to return to read and wait
			for a read completion or the write completion (of
			the alert message).
	    sequencer	(sce_chkstate) If a query input arrived while we
			are in SCS_ASPENDING state then make sure to "ignore"
			any pending completions of the alert message that
			was just sent (it may have already been completed
			and this race condition is closed).  This transition
			to the "ignore" state is to avoid leaving the
			CS_EDONE_MASK bit set for a subsequent CSsuspend
			(see state SCS_ASIGNORE next).
	    sequencer	If a read is still pending (GC_IN_PROCESS) and
			we're in SCS_ASPENDING state then this means that
			our send completed.  Then return CS_EXCHANGE
			regardless if there are alerts or not.  This forces
			us back to scc_send where we can clean up, reset
			the state and possibly send any more.

    SCS_ASIGNORE
    
	This state may follow SCS_ASPENDING.  This state is set to indicate
	that there is a pending alert (possibly the send of an  alert message
	that has not completed) but a query (or other user request) has come
	in and has caused us to continue (or start) processing the request.
	When the sequencer detects this situation it sets the SCS_ASIGNORE
	state (via sce_chkstate). 
	
	This special state is somewhat similar to the SCS_FORCE_ABORT
	state - it indicates to the GCA completion routine (scc_gcomplete)
	that if something completes while this state is set then do not
	CSresume the thread.  Why?  Assume we had a pending send of alert 
	message which had not yet completed.  A query arrives and we start
	processing.  In the meantime the send of the alert message
	completes and we call CSresume, which has the effect of turning
	on the CS_EDONE_MASK flag.  Later a DI request (may use CSsuspend)
	will complete immediately (because of the "dirty" EDONE bit) without
	really acquiring the resources.  Note that after setting the
	SCS_ASIGNORE state we must close the race condition of when it may
	have just completed (done by calling CScancelled).

	It is also important that we inhibit the CSresume of the pending
	send of the alert message but not of any remaining reads.  For
	example, what if the read that completed during a SCS_ASPENDING
	state was just the first GCA buffer of a multi-buffer (long) query.
	We want to make sure that we allow subsequent "continuation" reads
	to complete and get processed.  Thus, this state is only set when
	we know that we have "done" with all input (i.e. the SCS state is
	no longer SCS_INPUT) and while in "pending" state.  If we set this
	state as soon as any input arrived we would possibly hang when we
	tried to read more input for a long query.  Note that during the long
	query input collection the SCS_ASPENDING state will remain set (and,
	in fact, the pending send of the alert message may actually complete).
	The sequencer knows how to recover from cases where it called
	scs_input and found that there it is still reading or needs to read
	more.  Typically, however, we need not worry about this as a
	synchronous client does not usually read between multiple buffers of
	a single message.  Note that if we had GCA completion routines on a per
	call basis then this paragraph could be ignored, as we would easily
	be able to differentiate between whether we are completing the read
	or the write -- but the above argues why we need not differentiate
	at this time.

	We must also make sure to "unset" this SCS_ASIGNORE before leaving
	the sequencer (note that this is only set if we have a user request
	and we know that we will leave the sequencer eventually).  This 
	unsetting is done because we do not want to remain in a state where
	GCA completion routines do not resume the session.  Therefore this
	state is always unset whenever we re-enter scc_send.  Why scc_send
	and not when we leave the sequencer?  All queries initially return
	something, even COPY FROM sends a COPY map (see later for a COPY
	issue).  Thus, something is to be sent to the user and we can treat
	our pending send (if it is still pending) as though it was first
	in line.  We must be very careful when turning this state off
	to close all race conditions during which the original pending send
	may have completed. Consequently, we avoid turning this state off
	anywhere but scc_send.  If we actually turn up in scc_recv with the
	SCS_ASIGNORE state on then something went wrong (i.e. we didn't
	funnle through scc_send), an error is logged and the session is
	terminated.


    SEQUENCER STATE MODIFICATIONS (input states for sce_chkstate):
    -------------------------------------------------------------
	
    The above already described which states the sequencer sets.  This
    section repeats that description but in the context of the sequencer.
    Note that sequencer state changes actually occur in a separate routine
    sce_chkstate that is in this file:

    SCS_ASNOTIFIED, SCS_ASWAITING  ->	SCS_ASDEFAULT
    [or sce_chkstate(SCS_ASNOTIFIED)]
    
	All entries into the sequencer immediately apply this state
	transition.  If the current state is SCS_ASNOTIFIED then acknowledge
	any pending interrupts that may have been set by the event thread.
	This transition is done under semaphore as it is a shared state and
	is related to the event thread updating the alert queue.  The
	acknowledge operation (CSintr_ack, CScancelled) is done to avoid
	subsequent CSsuspend calls from picking up our interrupt.  If, upon
	entry, it finds this state then it sets the alert state to
	SCS_ASDEFAULT:

	    if (alert_state = SCS_ASNOTIFIED | SCS_ASWAITING)
		- This check is under semaphore control (for EV thread)
		if (alert_state = SCS_ASNOTIFIED)
		    acknowledge interrupt;
		end if;
		alert_state = SCS_ASDEFAULT;
	    end if;

    SCS_ASPENDING		  ->    SCS_ASIGNORE
    [or sce_chkstate(SCS_ASPENDING)]

	When the sequencer notices that we are starting to actually process
	user input (not, say, a multi-GCA-buffer query) then it must turn
	on the SCS_ASIGNORE state if it is in the SCS_ASPENDING state (to
	avoid the send completing and leaving the CS_EDONE_MASK bit set for
	a later resource request).  Normally we will enter the sequencer
	with a query (state set to SCS_ASDEFAULT above) and the starting of
	a query has not effect on alerts (in this case any alerts that are
	on the alert queue by the time we turn around to call scc_send any
	time later will be sent just in front of the query data).  But if
	we were in a send pending mode we do want to make sure to ignore
	potential GCA completions:

	    if (alert_state = SCS_ASPENDING)
		alert_state = SCS_ASIGNORE;
		CScancelled;		-- Close race condition
	    end if;


    SEQUENCER ALERT-RELATED MODIFICATIONS:
    -------------------------------------

    Before any sequencer alert-related modifications the sequencer
    input-related flow was:

	*next_op = CS_INPUT;

	case (CS op_code)
	  CS_INPUT:
	    call scs_input();
	    if (scs_state != SCS_INPUT) 	-- Start query/don't need more
		break;
	    else if (input_state = request_purged)
		if (first_queued_message != queue_head)
		    *next_op = CS_EXCHANGE;	-- To send GCA_IACK
		return (OK);
	    else
		-- Must need more input
		return (OK);
	    end if;
	    ...
	end case;

    This pseudo code is extended further on down.

    Most of the sequencer changes are a direct result of the required state
    changes described in the previous section.  There are some changes,
    however, which are not obvious results.  The description of these
    special cases may be better understood after reading some of the later
    flow examples in the following sections.  The special cases are
    described here because we are trying to show the typical interaction that
    the sequencer has with alert state changes:

    [SQ1] User Interrupts and Alerts:

    (This problem is raised and solved only as an availability enhancement). 
    User interrupts may arrive any time. They confuse alert processing when
    (1) the interrupt-acknowledge message (GCA_IACK) is about to be sent
    when there are queued alerts, (2) they arrive when there is already a
    pending send of an alert message, and (3) when the sequencer is entered
    after a read message has been purged and an alert message is pending:
    - Case (1) is a relatively simple problem and resolved by checking to
      see that we are not in "user interrupt" mode (sscb_interrupt) and
      that there is no "purging" message on the queue - a GCA_IACK message
      has been put on the queue (or the SCS_AFPURGE flag).  This is checked
      in scc_send before we pick off and send an alert to GCA.  This is 
      just to make alerts more available and to lose as few as possible
      through GCA purging.
    - Case (2) is really just a statement of problem - if there is already
      a pending send of an alert message and we then send a GCA_IACK, the GCA
      semantics are that an already sent alert message will be purged.
      It will be documented that some alerts may be lost due to interrupt
      processing.  This really depends on the GCA implementation on the
      system (a buffered GCA will lose more).
    - Case (3) is the case that affects the sequencer.  In this case an
      input message was purged (E_GC0027_RQST_PURGED), probably just the
      pending read that never completed (note that the client interrupt
      comes through the expedited channel), and the sequencer has to decide
      if it needs to send a GCA_IACK.  Currently (pre-alerts) it assumes
      that if the first queued GCA message is not the queue head (i.e. the
      GCA message queue is not empty) then it must be a GCA_IACK to flush to
      the user and thus returns CS_EXCHANGE.  This check now explicitly
      confirms that the pending message type is GCA_IACK.  Otherwise it can
      assume that the message should remain and return to CS (this message
      may, in fact, be a pending alert message that was sent *after* the
      GCA_IACK).

    [SQ2] When should Sequencer "Really" Send Alerts (or Return CS_EXCHANGE):

    Earlier, we mentioned that the sequencer can simply check if the original
    user read operation is still "in process" and there are alerts then
    return CS_EXCHANGE to flush the next alert to the client.  This is 
    described in a bit more detail here:
    - When the state is set to SCS_ASPENDING and we notice a query we modify
      it to SCS_ASIGNORE and leave it to be handled later (see SCS_ASIGNORE
      description above).  We know that we have found a query when the
      scs_input does *not* return the SCS_INPUT state (with status OK).
    - The second case is figuring out why we are still in SCS_INPUT state.
      We may be in that session state because of a multi-buffer query (that
      needs more input) or because there was no read-related change but
      the reading of an alert by the client (GCA read completion) resumed our
      thread.  In the case of a multi-buffer query, not only do we want
      to return to read more input but we also want to make sure not to set
      the SCS_ASIGNORE state (because we want to recognize subsequent read
      completions).  Consequently, we modify scs_input slightly to indicate
      if it really read new input or found nothing new to read (a new
      alert-related flag is set in by scs_input when it detects that it
      really read some user input (flag = SCS_AFINPUT).  

    Considering the alert state and the GCA input state that must be
    checked, the sequencer has the following modifications marked by (*).
    Note that both sequencer state transitions described earlier 
    are shown here as their respective sce_chkstate calls:

	*next_op = CS_INPUT;
	sce_chkstate(SCS_ASNOTIFIED);		-- To clean up notifications

	case (op_code)
	  CS_INPUT:
	    call scs_input(&read_flag);		-- Read_flag/SCS_AFINPUT*
						--    = read data?	*
	    if (scs_state != SCS_INPUT) 	-- Starting a query
		sce_chkstate(SCS_ASPENDING);	-- May "ignore" a send?	*
		break;
	    else if (input_state = request_purged)
		if (first_queued_message != queue_head) and 
		   (message_type = GCA_IACK)	-- See [SQ1] - alert	*
		    normal stuff;
		return (OK);
	    else
		-- If we did not read user data (read_flag) then	*
		-- if alerts or pending, return to scc_send (to send	*
		-- next or clean up).  Otherwise leave state as		*
		-- the CS_INPUT default to get more input.		*
		if (read_flag = 0) and					*
		   ((alerts) or (alert_state = SCS_ASPENDING))		*
		    *next_op = CS_EXCHANGE;	-- To call scc_send	*
		else							*
		    *next_op = CS_INPUT;	-- Get more input
		return (OK);
	    end if;
	    ...
	end case;


    SOME SEQUENCER/SCC STATE FLOW EXAMPLES:
    --------------------------------------

    These section tries to follow the processing of alerts and their 
    states through the sequencer, CS and SCC.  Some details about SCC
    are provided later:

    [E1] Session running queries w/o any alert registrations:

	Wait for query - scc_recv puts state into SCS_ASWAITING;
	Query arrives - wake up thread;
	Enter Seq - sce_chkstate(SCS_ASNOTIFIED) sets state SCS_ASDEFAULT;
	Read query and calls sce_chkstate(SCS_ASPENDING) which does nothing;
	Results are sent to client - scc_send (no alerts);
	Return to wait - scc_recv resets state to SCS_ASWAITING;

    [E2] Session interrupted for alert notification[s] w/o concurrent query:

	Wait for query - scc_recv puts state into SCS_ASWAITING;
	Event thread adds event - calls CSattn & sets state to SCS_ASNOTIFIED;
	Session awakens - sce_chkstate(SCS_ASNOTIFIED) sets to SCS_ASDEFAULT;
	For each locally queued alert (one at a time):
	    Sequencer notices alerts & returns to CS with CS_EXCHANGE;
	    CS call scc_send;
	    scc_send deallocates previous message (if there was one);
	    scc_send notices alerts & (calls sce_notify to) add to GCA queue;
	    scc_send notices that alert is only thing & sets SCS_ASPENDING;
	    scc_send calls GCA_SEND for alert and notices SCS_ASPENDING;
	    scc_send returns E_CS001D_NO_MORE_WRITES which does not suspend;
	    CS returns to wait for read (scc_recv);
	    scc_recv notices SCS_ASPENDING & does not set state to
		SCS_ASWAITING (normally set to allow event thread to interrupt);
	    At some point GCA_SEND of alert message completes & resumes thread;
	    Return to sequencer - sce_chkstate(SCS_ASNOTIFIED) which does
		not modify state;
	    Sequencer notices more alerts or still in SCS_ASPENDING state
		and returns CS_EXCHANGE;
	end;
	After last alert:
	    Sequencer notices no alerts but still in SCS_ASPENDING state;
	    Returns to CS with CS_EXCHANGE, which calls scc_send;
	    scc_send deallocates previous message (last alert-send);
	    scc_send notices no more alerts - sets state SCS_ASDEFAULT;
	    scc_send notices nothing to send - returns E_CS001D_NO_MORE_WRITES;
	    CS does not suspend & returns to wait for read (scc_recv);
	    Return to wait - scc_recv puts state into SCS_ASWAITING and
		resumes original read;

    [E3] Session interrupted for 2 alerts (EV1 and EV2).  Query arrives
	 after processing EV1:

	Wait for query - scc_recv puts state into SCS_ASWAITING;
	Event thread adds EV1 - calls CSattn & sets state to SCS_ASNOTIFIED;
	Event thread adds EV2;
	Session awakens - sce_chkstate(SCS_ASNOTIFIED) sets to SCS_ASDEFAULT;
	First alert (EV1):
	    Sequencer notices alerts & returns to CS with CS_EXCHANGE;
	    CS calls scc_send;
	    scc_send notices 1 alert, sets to SCS_ASPENDING before GCA_SEND;
	    scc_send returns E_CS001D_NO_MORE_WRITES which does not suspend;
	    CS returns to wait for read (scc_recv);
	    scc_recv notices SCS_ASPENDING & does not set to SCS_ASWAITING;
	    Pending read completes - query has arrived;
	    GCA_SEND of EV1 may not yet have completed!
	Return to sequencer (EV2 still needs processing):
	    Sequencer call sce_chkstate(SCS_ASNOTIFIED) which does nothing;
	    Calls scs_input and notices a read completed (assume all read);
	    Calls sce_chkstate(SCS_ASPENDING) because entering query;
	    sce_chkstate sets state from SCS_ASPENDING to SCS_ASIGNORE;
	    Pending GCA_SEND completes but is ignored (this could occur
		anywhere up to -- and including -- the next scc_send);
	Query is processed:
	    Sequencer returns to CS (with CS_EXCHANGE) which calls scc_send;
	    scc_send notices SCS_ASIGNORE and carefully modifies to
		SCS_ASDEFAULT and closes race conditions;
	    scc_send finishes sending all alerts (EV2) & then query result data
		(each call will deallocate the last message);
	CS returns to wait for read (scc_recv);
	Return to wait - scc_recv puts state into SCS_ASWAITING;

    [E4] Session running queries when some alerts arrive:

	Wait for query - scc_recv puts state into SCS_ASWAITING;
	Query read - sce_chkstate(SCS_ASNOTIFIED) sets state to SCS_ASDEFAULT;
	Sequencer calls sce_chkstate(SCS_ASPENDING) which does nothing;
	Query is being processed and event thread wants to notify;
	Event thread adds some alerts to the queue - does NOT call CSattn;
	Upon completion we return to scc_send;
	All queued alerts are sent off to GCA (followed by query results);
	Return to wait - scc_recv puts state into SCS_ASWAITING;


    CS/DISPATCHER READS (scc_recv) AND THE ALERT STATE:
    --------------------------------------------------

    This is a detailed summary of state changes described earlier.  Normally,
    when scc_recv is entered from CS for a read operation one can assume
    that if there are no alerts on the session queue the session should be
    put into the waiting/alert-interruptable state (SCS_ASWAITING). Otherwise
    if there are alerts then we just return and continue to process them.
    This assumption holds with a few exceptions:

    [R1] The alert state is already set to SCS_ASPENDING. 

    As described earlier the system must "flush" all alerts until the
    state is reset to the SCS_ASDEFAULT state (all alerts have been
    processed), or until it transitions from SCS_ASPENDING to SCS_ASIGNORE
    (i.e. a query is being processed and all alerts will be sent at the end
    of the query).  Consequently, when scc_recv is about to enter its "read"
    state, knowing that it will be suspended by CS (or to continue a previous
    read state - E_GCFFFF_IN_PROCESS) it makes sure to suppress normal
    operations if the alert state is in "pending" mode.  Note that if we
    allowed it to return immediately when it found alerts on the queue and
    it was in "pending" state, we would end up returning to the sequencer,
    and eventually to scc_send, possibly with an already pending send of
    an alert message.  The second alert send would then be forced to suspend
    to wait for the first - something we must NEVER do.

    [R2] Suppress alert call-back when processing (reading) COPY data:

    COPY processing sets various sequencer states, all of which are handled
    as special cases.  As far as scc_recv is concerned, by nature of the
    COPY FROM statement (from client to server), the session will be switching
    between scc_recv and the sequencer (to consume client data).  When in
    this mode we should make sure not to delay the read in order to send
    alerts to the client.  Also, the client may not be expecting to read any
    alerts (on the normal GCA channel) while the COPY FROM is going on and
    the client is sending data to the server.  Since it is scc_send that 
    eventually decides when to send alerts, we may end up sending alerts
    while processing a COPY INTO (that is just an "availability" enhancement).
    So, rather than check for COPY we check to make sure that before we call
    back for more alerts we are in the "normal read" SCS_INPUT state.

    [R3] Reading while in "ignore" state:

    It should NEVER be the case that we try to read data from the user
    while alerts are in the SCS_ASIGNORE state.  Note that this state
    is only set once we *start* a query (not once we just read a user buffer
    and may require more).  We "know" that all queries (even COPY FROM)
    end up first sending some data to the user.. and from there may read
    more or do more work.  This sending operation (scc_send) operation
    should always turn off the "ignore" state (described earlier).  In order
    to confirm that we never try to read from the user while in SCS_ASIGNORE
    state, scc_recv always logs an error and terminates the session:

    The above cases modify the scc_recv control to the following.  New lines
    are marked with (*):

	if (alert_state = SCS_ASIGNORE)					*
	    log/return error (E_SC0275_IGNORE_ASTATE)			*
	if (read_status = not_yet_processed)
	    return (E_CS001E_SYNC_COMPLETION);
	if (alert_state != SCS_ASPENDING && scs_state == SCS_INPUT) 	*
	    -- Under semaphore to avoid conflicts with EV thread	*
	    if (alerts on queue)					*
		return (E_CS001E_SYNC_COMPLETION);			*
	    alert_state = SCS_ASWAITING;				*
	end if;								*
	if (read_status = E_GCFFFF_IN_PROCESS)
	    return (OK);	-- suspend till next GCA completion

	CScancelled();
	post GCA_RECEIVE;
	return to be suspended;

    CS/DISPATCHER WRITES (scc_send) AND THE ALERT STATE:
    ---------------------------------------------------

    Normally, when scc_send is entered from CS for a write operation it
    continues writing until there are no more messages.  Each write
    operation starts out:

	if (write_status = E_GCFFFF_IN_PROCESS)
	    return (OK);			-- To resuspend write
	deallocate old message;
	if (no_more_messages)
	    return (E_CS001D_NO_MORE_WRITES);
	check for FAPENDING;
	post GCA_SEND;
	return to be suspended;

    This processing is modified to support alerts.  First it is scc_send
    that notices that we have alerts to send and calls sce_notify to add
    the alert to the GCA queue.  In fact, whenever scc_send is sending
    anything it always looks for alerts first.  The following points must
    now be accounted for in scc_send: 
    
    [W1] Notice that there are alerts and call sce_notify:

    When scc_send is about to send a new message, or is about to decide
    that it does not have anything to send it checks first to see if there
    are any alerts queued.  Alerts could be present when there's nothing
    else on the GCA queue (as when the thread is notifying the client w/o
    any query results) or with other query data (as when alerts are being
    sent with query results).  On all accounts we always try to send the
    alerts as soon as possible. 
    
    (Error recovery condition): Upon return from sce_notify we must check
    the error status.  Why? There could be a case where we had alerts
    pending, but the event thread was taken down.  In this case sce_notify
    will NOT add any alert message and return an error.  Thus, we have to
    handle the error status to remain in the original state.
    
    (Availability enhancement): There is one exception as to when we call
    sce_notify. In order not to lose alerts in the purging of a GCA_IACK
    message, we suppress sending any alerts (even if there are some) if
    "user interrupts" are set (sscb_interrupt) or there is a queued
    GCA_IACK message (indicated by the SCS_AFPURGE flag).  

    [W2] Set state to SCS_ASPENDING/SCS_ASDEFAULT depending on GCA queue:

    Just before sending the alert we check if the GCA queue is empty or
    not.  If it is then (as mentioned above) this must be a client
    notification and we set the state to SCS_ASPENDING.  Note that
    after some initial checks for the SCS_ASIGNORE flag we make sure to 
    reset the state (possibly temporarily) to SCS_ASDEFAULT.  This is done
    so that we do not ever leave the sequencer with the state
    SCS_ASPENDING, if we are not sure we really have a pending send.

    [W3] Inhibit CSsuspend if sending an alert-message outside of a query
	 result context:

    We must deal with alert messages that are sent in the SCS_ASPENDING
    state, where we do not want to suspend the thread (to avoid write-write
    deadlock).   For example, if we return OK (and cause CS to suspend us)
    then if the client tries to write a query, we will end up dead-locked.
    By returning E_CS001D_NO_MORE_WRITES, CS will not suspend on the
    scc_send call, and will immediately go back into the original (or new)
    scc_recv call.  When our thread resumes from that call it will either
    be because a read arrived and we should process the next query or to
    continue with the next alert (the send of the alert message completed).

    [W4] Make sure never to leave the "ignore GCA completions" state on:

    Earlier processing may have turned on the SCS_ASIGNORE state which
    inhibits the GCA completions routine (scc_gcomplete) from calling
    CSresume.  Once this is set (when a user query arrives to the
    sequencer) we will eventually turn up to scc_send to send results.
    At this time we want to turn it off.

    The above cases modify scc_send in the following way.  New lines
    are marked by (*):

	-- Handle reset from "ignore completions" to "default". Close	*
	-- race conditions where send completes just as we change 	*
	if (alert_state = SCS_ASIGNORE) then				*
	    was_in_progress = (write_status == E_GCFFFF_IN_PROCESS);	*
	    alert_state = SCS_ASDEFAULT;				*
	    if (write_status = E_GCFFFF_IN_PROCESS) then		*
		return (OK);	-- Continue suspending			*
	    if (was_in_progress)					*
		CScancelled();	-- Clear just-resumed send		*
	else								*
	    -- Reset alert state to "default" 				*
	    alert_state = SCS_ASDEFAULT;				*
	endif;								*
	if (write_status = E_GCFFFF_IN_PROCESS)
	    return (OK);			-- To resuspend write
	if (old stream)
	    if (old message = GCA_IACK) turn off "purging flag";	*
	    deallocate old message;
	end if;
	if (no more messages) & ((no alerts) | (user interrupt))	*
	    return (E_CS001D_NO_MORE_WRITES);
	if (more messages)						*
	    check for FAPENDING;
	else if (alerts) & (no more messages | not purging)		*
	    sce_notify to add message to "head" of queue;		*
	    check errors;						*
	    if (no more messages)					*
		alert_state = SCS_ASPENDING;				*
	    adjust current message and status;				*
	end if;								*
	post GCA_SEND;
	if (alert_state = SCS_ASPENDING)				*
	    return (E_CS001D_NO_MORE_WRITES); -- Don't suspend send	*
	return to be suspended;

#endif SCE_INTRO_COMMENT */

/* External and Forward declarations */

GLOBALREF 	SC_MAIN_CB	*Sc_main_cb;	/* Root for SCF data */

static VOID sce_gcevent(SCD_SCB *scb, SCE_AINSTANCE *inst );

/*{
** Name: sce_chkstate	- Check/set the state of alerts for a session
**
** Description:
**      This routine provides some sequencer support transition between
**	various states.  A detailed description of when these transitions
**	are made and what the input states mean is described in the
**	introductory comment at the start of this file.  Please do not
**	add/modify state transitions (or new states) without updating the
**	comment.
**
** Inputs:
**	input_state		Input state to check for:
**				SCS_ASNOTIFIED - Acknowledge notification
**						 and set SCS_ASDEFAULT.
**				SCS_ASPENDING  - Transition to SCS_ASIGNORE.
**	scb.scb_sscb		SCS CB for this session.
**	   .sscb_astate		Current state of alerts.
**
** Outputs:
**	scb.scb_sscb		SCS CB for this session:
**	   .sscb_astate		May be modified based on value input_state and
**				the current state.
**	Returns:
**	    VOID
**
** Side Effects:
**	The transition to state SCS_ASIGNORE inhibits GCA completions from
**	resuming the thread.  Be careful when setting this state.
**
** History:
**	21-mar-90 (neil)
**	    Written for alert notification.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	04-Jan-2001 (jenjo02)
**	    Removed unneeded CSget_sid().
*/

VOID
sce_chkstate( i4   input_state, SCD_SCB  *scb )
{
    SCE_HASH	    	*sce_hash;	/* SCE alert structures */
    SCS_SSCB	    	*sscb;		/* SCS CB with session instance queue */
    SCE_AINSTANCE   	*inst;		/* Instance to trace */
    char		trcbuf[80];
    bool		notified;	/* For tracing of notifications */

    sscb = &scb->scb_sscb;

    /*
    ** Upon initial sequencer entry acknowledge any notifications.
    ** Lock up alert list AND the alerts state setting to check.  The
    ** event thread cannot add alerts (or interrupt us because it needs the
    ** lock to do so) while we are doing this check.  Note that the order
    ** of calling this routine in the sequencer is important, as we do not
    ** want to leave outstanding CS notifications (which will end up being
    ** processed as an interrupt or force_abort).
    */
    if (   (input_state == SCS_ASNOTIFIED)
	&& (   sscb->sscb_astate == SCS_ASNOTIFIED
	    || sscb->sscb_astate == SCS_ASWAITING
	   )
       )
    {
	notified = FALSE;
	sce_mlock(&sscb->sscb_asemaphore);
	if  (sscb->sscb_astate == SCS_ASNOTIFIED)
	{
	    notified = TRUE;
	    CSintr_ack();	/* We were notified (via CSattn) of an alert */
	    CScancelled(0);
	}
	SCS_ASTATE_MACRO("sce_chkstate/entry", *sscb, SCS_ASDEFAULT);
	sce_munlock(&sscb->sscb_asemaphore);

	/* Trace acknowledgement for event thread - if alert system still up */
	if (   (notified)
	    && (sce_hash = Sc_main_cb->sc_alert) != NULL
	    && (sce_hash->sch_flags & SCE_FEVPRINT)
	   )
	{
	    STprintf(trcbuf,
		     "E>  (session 0x%p acknowledged event notification)", scb);
	    if ((inst = (SCE_AINSTANCE *)sscb->sscb_alerts) != NULL)
		sce_trname(FALSE, trcbuf, &inst->scin_name);
	    else
		TRdisplay("%s: NO ALERTS (error)\n", trcbuf);
	} /* If tracing event thread */
    }
    /*
    ** If we have a pending alert-message-send and we are about to start
    ** processing a query then make sure to ignore the GCA completion of that
    ** send.  No need to lock list here as this state is not shared by the
    ** event thread.
    */
    else if (   input_state == SCS_ASPENDING
	     && sscb->sscb_astate == SCS_ASPENDING)
    {
	SCS_ASTATE_MACRO("sce_chkstate/processing", *sscb, SCS_ASIGNORE);
	CScancelled(0);			/* In case it just completed */
    }
} /* sce_chkstate */

/*{
** Name: sce_trstate	- Trace alert state transitions.
**
** Description:
**	This routine is called when the SCS_AFSTATE is set (as part of the
**	SCS_ASTATE_MACRO setting).  The routine enables you to trace state
**	transitions wherever they happen.  To use this (for your session)
**	execute:
**		SET TRACE TERMINAL;	-- To the terminal you want the display
**		SET TRACE POINT SC6;	-- To the session you want to trace
**
** Inputs:
**	title			Title string (source of change).
**	cur			Current alert state.
**	new			New alert state (not to set, just display)
**
** Outputs:
**	Returns:
**	    VOID
**
** History:
**	21-mar-90 (neil)
**	    Written for alert notification.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

VOID
sce_trstate(char  *title, i4   cur, i4  new )
{
    static char *states[] =
	{
	    "DEFAULT    ", "WAITING    ", "NOTIFIED   ",
	    "PENDING    ", "IGNORE     "
	};
    char	*curp, *newp;
    char	cur_unk[20], new_unk[20];	/* For unknown modes */
    char	title_buf[31];

    if (cur == new)		/* Don't trace this case */
	return;

    STmove(title, ' ', sizeof(title_buf)-1, title_buf);
    title_buf[sizeof(title_buf)-1] = EOS;
    if (cur >= SCS_ASDEFAULT && cur <= SCS_ASIGNORE)
    {
	curp = states[cur];
    }
    else
    {
	STprintf(cur_unk, "UNKNOWN(%d)", cur);
	curp = cur_unk;
    }
    if (new >= SCS_ASDEFAULT && new <= SCS_ASIGNORE)
    {
	newp = states[new];
    }
    else
    {
	STprintf(new_unk, "UNKNOWN(%d)", new);
	newp = new_unk;
    }
    TRdisplay("S> %s: %s==>    %s\n", title_buf, curp, newp);
    if (new == SCS_ASWAITING)			/* Usually completes a cycle */
    {
	MEfill(sizeof(title_buf)-1, '-', title_buf);
	TRdisplay("S> %s\n", title_buf);
    }
} /* sce_trstate */

/*{
** Name: sce_notify	- Notify the session's client of a raised event
**
** Description:
**      This is where a session actually transmits the new event to the
**	client to notify the client of the occurrence of an event for which
**	the client has registered.
**
**	The events for this session have already been queued to the session
**	SCB.  We are asked to pick them off one at a time and to format an
**	appropriate GCA message to send.
**
**	This routine is called from scc_send when it notices that we have
**	alerts to send.  It is an error if we are called and there are no
**	alerts queued.  Because of the time when this routine is called be
**	caerful never to send user trace (sc0e_trace) or user errors
**	(sc0e_uput) from here.  It will cause the thread to hang as this
**	data will probably arrive after the GCA_RESPONSE block.
**
**	Algorithm:
**	    if alert on sscb_alerts instance queue do
**		(P)lock session-instance-queue (sscb_asemaphore);
**		remove instance from queue and adjust queue head;
**		(P)unlock session-instance-queue (sscb_asemaphore);
**		format instance into GCA_EVENT message;
**		(P)lock hash-lists (sch_semaphore)
**		add instance to free list or deallocate;
**		(V)unlock hash-lists (sch_semaphore)
**	    end if;
**
** Inputs:
**	scb.scb_sscb		SCS CB for this session:
**	   .sscb_asemaphore	Lock to use to unlink the instance.
**	   .sscb_alerts		List of alerts (first is to be transmitted).
**
** Outputs:
**	scb.scb_sscb		SCS CB for this session:
**	   .sscb_alerts		One less alert in the list.
**	Returns:
**	    DB_STATUS
**	Errors - are not sent to user as we may be AFTER the response block:
**	    E_SC0274_NOTIFY_EVENT	- No events on list
**
** Side Effects:
**	1. Queues a GCA_EVENT message to client
**	2. Frees an instance node.
**
** History:
**	21-mar-90 (neil)
**	    Written for alert notification.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/

DB_STATUS
sce_notify( SCD_SCB  *scb )
{
    SCE_HASH	    	*sce_hash;	/* SCE alert structures */
    SCS_SSCB	    	*sscb;		/* SCS CB with session instance queue */
    SCE_AINSTANCE   	*inst;		/* Instance to transmit */
    SCE_AINSTANCE   	*next_inst;	/* Instance to delete */
    DB_STATUS	    	status;

    sscb = &scb->scb_sscb;

    /* Should never be called if no alerts */
    status = E_DB_ERROR;
    if ((inst = (SCE_AINSTANCE *)sscb->sscb_alerts) == NULL)
    {
	sc0e_put(E_SC0274_NOTIFY_EVENT, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0 );
	return (status);
    }

    /*
    ** If no SCF alert system then forget it.  But event thread may have been
    ** taken down while we had instances queued.  Deallocate them (don't
    ** try to add them to the free lists - they aren't there anymore).
    */
    if (   (sce_hash = Sc_main_cb->sc_alert) == NULL
	|| (sce_hash->sch_flags == SCE_FINACTIVE)
       )
    {
	/* Lock list and free all alert instances */
	sce_mlock(&sscb->sscb_asemaphore);
	while (inst != NULL)
	{
	    /* Point to next one and free current one */
	    next_inst = inst->scin_next;
	    _VOID_ sc0m_deallocate(0, (PTR *)&inst);
	    inst = next_inst;
	}
	sscb->sscb_alerts = NULL;
        sscb->sscb_last_alert = NULL;
        sscb->sscb_num_alerts = 0;
	sce_munlock(&sscb->sscb_asemaphore);
	sc0e_put(E_SC0274_NOTIFY_EVENT, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0 );
	if (ult_check_macro(&sscb->sscb_trace, SCS_TNOTIFY, NULL, NULL))
	{
	    TRdisplay("N> SCE/Client Event Notification (ERROR):\n");
	    TRdisplay("N>  queued alert(s) found but no alert support\n"); 
	} /* If tracing session notification */
	return (status);
    } /* If no event system */

    /* Lock the list and detach the instance */
    status = E_DB_OK;
    sce_mlock(&sscb->sscb_asemaphore);
    sscb->sscb_alerts = (PTR)inst->scin_next;

    /* Decrement number of alerts. If this was the only alert in the
    ** queue, then update sscb_last_alert. NB, the Decrement MUST
    ** occur before the if comparisson is made.
    */
    if ( !(--sscb->sscb_num_alerts) ) sscb->sscb_last_alert = NULL;
    inst->scin_next = NULL;
    sce_munlock(&sscb->sscb_asemaphore);

    /*
    ** Format trace message - to compare against what's sent to the FE. Do
    ** not send as GCA trace as this may arrive AFTER end of message.
    ** Note to use SET TRACE TERMINAL.
    */
    if (ult_check_macro(&sscb->sscb_trace, SCS_TNOTIFY, NULL, NULL))
    {
	/* Prefix a "N>" to differentiate with event thread trace "E>" */
	TRdisplay("N> SCE/Client Event Notification:\n");
	sce_trname(FALSE, "N>  alert_name", &inst->scin_name);  /* Alert name */
	if (inst->scin_lvalue > 0)			        /*    & value */
	{
	    TRdisplay("N>  alert_value: %#s\n", inst->scin_lvalue,
		      inst->scin_value);
	}
    } /* If tracing session */

    /* Package alert into GCA_EVENT message */
    sce_gcevent(scb, inst);

    /* Free the instance entry - if has a user value then deallocate */
    sce_mlock(&sce_hash->sch_semaphore);
    _VOID_ sce_free_node(inst->scin_lvalue > 0, sce_hash->sch_nhashb,
			 &sce_hash->sch_nainst_free,
			 (SC0M_OBJECT **)&sce_hash->sch_ainst_free,
			 (SC0M_OBJECT *)inst);
    sce_munlock(&sce_hash->sch_semaphore);
    return (status);
} /* sce_notify */

/*{
** Name: sce_gcevent	- Package an alert message (GCA_EVENT) to SCC queue
**
** Description:
**      This routine provides a way to add alert messages (GCA_EVENT) to
**	the GCA queue (from within scc_send).   These messages are inserted
**	on the front of the queue - not the end.
**
** Inputs:
**	scb				Session CB
**	inst				SCE alert instance:
**	   .scin_name			Compound alert name
**	   .scin_when			Time when alert originator raised alert
**	   .scin_flags			Flags (not yet used)
**	   .scin_lvalue			Length of following field
**	   .scin_value			If length > 0 then this is the data.
** Outputs:
**	scb.scb_cscb			Message queue has new alert inserted
**	Returns:
**	    VOID
**
** History:
**	21-mar-90 (neil)
**	    Written for alerters.
*/
static VOID
sce_gcevent(SCD_SCB  *scb, SCE_AINSTANCE  *inst )
{
    DB_ALERT_NAME	*aname;		/* Actual alert name */
    GCA_EV_DATA		evdata;		/* Dummy for figuring out sizes */
    char		*blank;		/* Trim blanks off names */
    i4			dsize;		/* Size of gca user data */
    DB_TEXT_STRING	user_data;	/* Varying length user data */
    char		*cp;		/* Current copy spot */
    SCC_GCMSG		*evmsg;		/* SCC/GCA event message */
#define	DVAL_SIZE	(sizeof(i4) + sizeof(i4) + sizeof(i4))

    /*
    ** Figure out the size.  Calculate blank trimmed object names + time-stamp
    ** GCA_DATA_VALUE plus optional user value.
    */
    /* gca_evname */
    aname = &inst->scin_name;
    blank = STindex((char *)&aname->dba_alert, " ", sizeof(aname->dba_alert));
    if (blank == NULL)
	evdata.gca_evname.gca_l_name = sizeof(aname->dba_alert);
    else
	evdata.gca_evname.gca_l_name = blank - (char *)&aname->dba_alert;
    dsize =
	sizeof(evdata.gca_evname.gca_l_name) + evdata.gca_evname.gca_l_name;
    /* gca_evowner */
    blank = STindex((char *)&aname->dba_owner, " ", sizeof(aname->dba_owner));
    if (blank == NULL)
	evdata.gca_evowner.gca_l_name = sizeof(aname->dba_owner);
    else
	evdata.gca_evowner.gca_l_name = blank - (char *)&aname->dba_owner;
    dsize +=
	sizeof(evdata.gca_evowner.gca_l_name) +	evdata.gca_evowner.gca_l_name;
    /* gca_evdb */
    blank = STindex((char *)&aname->dba_dbname, " ", sizeof(aname->dba_dbname));
    if (blank == NULL)
	evdata.gca_evdb.gca_l_name = sizeof(aname->dba_dbname);
    else
	evdata.gca_evdb.gca_l_name = blank - (char *)&aname->dba_dbname;
    dsize +=
	sizeof(evdata.gca_evdb.gca_l_name) + evdata.gca_evdb.gca_l_name;
    /* gca_evtime */
    evdata.gca_evtime.gca_type = DB_DTE_TYPE;
    evdata.gca_evtime.gca_precision = 0;
    evdata.gca_evtime.gca_l_value = sizeof(inst->scin_when);
    dsize += DVAL_SIZE + evdata.gca_evtime.gca_l_value;
    /* gca_evvalue */
    dsize += sizeof(evdata.gca_l_evvalue);
    if (inst->scin_lvalue == 0)		/* Varying part of message */
    {
	evdata.gca_l_evvalue = 0;
    }
    else
    {
	evdata.gca_l_evvalue = 1;
	evdata.gca_evvalue[0].gca_type = DB_VCH_TYPE;
	evdata.gca_evvalue[0].gca_precision = 0;
	user_data.db_t_count = inst->scin_lvalue;
	evdata.gca_evvalue[0].gca_l_value =
	    sizeof(user_data.db_t_count) + user_data.db_t_count;
	dsize += DVAL_SIZE + evdata.gca_evvalue[0].gca_l_value;
    }

    /* The SCC message descriptor was already constructed at session startup */
    evmsg = &scb->scb_cscb.cscb_evmsg;
    evmsg->scg_mtype = GCA_EVENT;
    evmsg->scg_mask = SCG_NODEALLOC_MASK;
    evmsg->scg_msize = dsize;

    /*
    ** Now construct the GCA event message - some piece-meal copies, because
    ** the real GCA_EV_DATA includes varying length fields.  Note that the
    ** cscb_evdata field really points at a large enough buffer (SCC_GCEV_MSG).
    */
    cp = (char *)scb->scb_cscb.cscb_evdata;
    /* gca_evname */
    MEcopy((PTR)&evdata.gca_evname.gca_l_name,
	   sizeof(evdata.gca_evname.gca_l_name), cp);
    cp += sizeof(evdata.gca_evname.gca_l_name);
    MEcopy((PTR)&aname->dba_alert, evdata.gca_evname.gca_l_name, cp);
    cp += evdata.gca_evname.gca_l_name;
    /* gca_evowner */
    MEcopy((PTR)&evdata.gca_evowner.gca_l_name,
	   sizeof(evdata.gca_evowner.gca_l_name), cp);
    cp += sizeof(evdata.gca_evowner.gca_l_name);
    MEcopy((PTR)&aname->dba_owner, evdata.gca_evowner.gca_l_name, cp);
    cp += evdata.gca_evowner.gca_l_name;
    /* gca_evdb */
    MEcopy((PTR)&evdata.gca_evdb.gca_l_name,
	   sizeof(evdata.gca_evdb.gca_l_name), cp);
    cp += sizeof(evdata.gca_evdb.gca_l_name);
    MEcopy((PTR)&aname->dba_dbname, evdata.gca_evdb.gca_l_name, cp);
    cp += evdata.gca_evdb.gca_l_name;
    /* gca_evtime */
    MEcopy((PTR)&evdata.gca_evtime, DVAL_SIZE, cp);
    cp += DVAL_SIZE;
    MEcopy((PTR)&inst->scin_when, sizeof(inst->scin_when), cp);
    cp += sizeof(inst->scin_when);
    /* Optional gca_evvallue */
    MEcopy((PTR)&evdata.gca_l_evvalue, sizeof(evdata.gca_l_evvalue), cp);
    cp += sizeof(evdata.gca_l_evvalue);
    if (evdata.gca_l_evvalue > 0)
    {
	MEcopy((PTR)&evdata.gca_evvalue[0], DVAL_SIZE, cp);
	cp += DVAL_SIZE;
	MEcopy((PTR)&user_data.db_t_count,sizeof(user_data.db_t_count), cp);
	cp += sizeof(user_data.db_t_count);
	MEcopy((PTR)inst->scin_value, (i4)user_data.db_t_count, cp);
    }

    /* Now insert at "head" of queue - NOT at "tail" */ 
    evmsg->scg_next = scb->scb_cscb.cscb_mnext.scg_next;
    evmsg->scg_prev = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
    scb->scb_cscb.cscb_mnext.scg_next->scg_prev = evmsg;
    scb->scb_cscb.cscb_mnext.scg_next = evmsg;
} /* sce_gcevent */
