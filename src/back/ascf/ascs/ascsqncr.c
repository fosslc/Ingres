/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <sl.h>
#include    <tr.h>
#include    <er.h>
#include    <ex.h>
#include    <id.h>
#include    <me.h>
#include    <cm.h>
#include    <cs.h>
#include    <lk.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <adp.h>
#include    <gca.h>
#include    <copy.h>
#include    <generr.h>
#include    <sqlstate.h>
#include    <usererror.h>

#include    <dmf.h>
#include    <dmacb.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>

#include    <ddb.h>
#include    <qsf.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefqeu.h>
#include    <qefcopy.h>
#include    <qefnode.h>
#include    <opfcb.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefcb.h>
#include    <rdf.h>
#include    <scf.h>
#include    <sxf.h>
#include    <duf.h>
#include    <dudbms.h>

#include    <sc.h>
#include    <sc0m.h>
#include    <sc0a.h>
#include    <sc0e.h>
#include    <scc.h>
#include    <scfscf.h>
#include    <scs.h>
#include    <scd.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <scserver.h>
#include    <cui.h>

#include    <urs.h>
#include    <ascd.h>
#include    <ascs.h>
#include    <ascf.h>

#if defined(hp3_us5)
#pragma OPT_LEVEL 2
#endif /* hp3_us5 */


/**
**
**  Name: SCSQNCR.C - This file contains the code for the Ingres Sequencer
**
**  Description:
**      This file contains the Ingres DBMS Sequencer and associated supporting
**      routines.  The sequencer is that part of the ingres system which
**	orchestrates the processing of queries and other database activity.
**
**          ascs_sequencer() - the sequencer itself
**	    scs_input() - classify input an place it in QSF.
**          scs_qef_error() - Process a qef error.  This is done quite often,
**			so it has a separate routine.
**
**	    scs_qsf_error() - Process a qsf error.  This is done quite often,
**			so it has a separate routine.
**          scs_gca_error() - Process a gcf error.  This is done quite often,
**			so it has a separate routine.
**          scs_fetch_data() - GCA data into usable format.
**	    scs_qt_fetch() - Extract query text for PSF.
**	    sc0e_trace()	-   Send trace string to FE.
**          scs_tput() - Callback routine to aid in above.
[@func_list@]...
**
**
**  History:
**      18-Jan-1999 (fanra01)
**          Include ascf.h for prototypes.
**          Renamed scs_sequencer, scs_process_interrupt to ascs equivalents.
**          Removed scs_desc_send.
**          Removed forward references now done in header.
**      12-Feb-99 (fanra01)
**          Renamed scs_disassoc to ascs equivalent.
**      15-Feb-1999 (fanra01)
**          Add license checking.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**/

/*
**  FLOW SUMMARY
**  ------------
**
**  When thinking of a client front-end and Frontier server, the following
**  scheme can help:
**
**      Client				    Server
**      ------				    ------
**
**      Work				    Nothing
**      				    Wait on Read (suspend thru CS)
**      Send Query to DBMS -------------->  Read  completes
**      				    +- Enter Sequencer -----+
**      				    |  Process	            | (loop)
**      				    |  Write to Client      |
**      Read Results       <--------------  +- CS suspend on Write -+
**
**
**  Within the server itself it is useful to understand the interface
**  between CS (the dispatcher), ASCS (the sequencer) and SCC (sequencer
**  communication support routines).  Typically, the session suspends
**  reading (after scc_recv), gets input and enters the sequencer, after
**  which it usually then writes everything it has to the user (and
**  suspends after each scc_send).  I say "usually" as the sequencer may
**  notice that it only read one small part of a large query and needs to
**  read more.  The following is a conceptual outline of the CS loop that
**  controls this flow (this is part of CS_setup elaborated for the
**  sequencer):
**
**      mode = CS_INPUT;
**      while (true) 				-- loop until termination
**      {
**          if (mode = CS_INPUT)
**          {
**      	status = scc_recv();		-- post read
**      	if (status == OK)
**      	    status = CSsuspend();
**      	check for termination;
**          }
**          ascs_sequencer(mode, &next_mode);
**          check for termination;
**          if (next_mode == CS_OUTPUT || CS_EXCHANGE)
**          {
**      	do
**      	{
**      	    status = scc_send();	-- post write
**      	    if (status == OK)
**      		status = CSsuspend();
**      	} while (status != OK);
**      	check for termination;
**          }
**          if (next_mode = CS_EXCHANGE)	-- just a short cut
**      	mode = CS_INPUT;
**          else
**      	mode = next_mode;
**      }
**
**  The CS_EXCHANGE mode is the "typical" mode that is returned when all
**  the user results have been queued for the session and we want CS to
**  cause all of them to be returned to the user.  The CS_OUTPUT mode
**  is used when, in order to complete the actual request, we must return
**  to the sequencer to continue processing (i.e. middle of a SELECT loop
**  or COPY statement).
**
**  With the above loop (and short explanation) in mind a lot of processing
**  can be understood.  In particular processing that involves multiple
**  messages and possible return to the sequencer (such as SELECT loops,
**  COPY processing, etc.) Alerts follow the above model but modify some of
**  the internal workings of various SCC routines (scc_send, in particular)
**  to allow clients to read data without having to issue a query (a key
**  requirement of this feature).
**
**  HIGH LEVEL FACT SUMMARY:
**  -----------------------
**
**  Given the description above the following is a summary of various
**  "facts" that must always be taken into account when thinking about the
**  "sending-of-alert-messages-to-user" problem.  None of these "facts"
**  assume a particular solution:
**
**  [F1] A session must be made interruptable (for alerts) so that it may
**       be woken up to process events.  This would occur when it is
**  sitting waiting for a query.  This is to allow alerts to be sent to a
**  program as soon as possible, in particular for programs that are just
**  reading alerts (GET EVENT), such as event monitors.
**
**  [F2] The session can be in a pending-read state (for a query) when it
**       is made runnable to process and "send" alerts.  It is exactly
**  during this time that various problems arise, described later.  And,
**  it is during this time when it is most common for alerts to be sent.
**
**  [F3] Alerts that the event thread attaches while the target thread is
**       processing a query are no problem.  These alerts are eventually
**  added as GCA messages that are sent to the client with the rest of the
**  query results, like trace messages but in front of query results (just
**  for faster availability).  Since these are no problem they are not
**  really discussed here.  In fact, if query results were the only time
**  we sent already queued alerts to the client this design note would not
**  be necessary.
**
**  [F4] GCA Alert messages, because of their size and nature, may not be sent
**       on the GCA expedited channel.  This was agreed upon during a
**  design review for "GCA alert message sending".
**
**  [F5] When the session is sending a GCA alert message to the client that
**       is not in the context of query (see [F3] above for *in* query
**  context), the session MUST NOT suspend after the send operation.  Why?
**  If we suspend waiting for the client to read the just-sent alert message,
**  we may cause a write/write deadlock with the client who is really trying
**  to send the DBMS a query to process.  Even though the client may intend to
**  turn around and read the alert message, we have to be able to read the
**  query first to allow them to read the alert (if the client is synchronous
**  this is even more critical).  The CS "write" routine (scc_send) return
**  status, E_CS001D_NO_MORE_WRITES, comes in handy for this "no-suspend" case.
**  This is all described later.  As mentioned above [F3] when alert messages
**  are sent in the context of query results we want to treat them like normal
**  data and they follow the regular query result processing.
**
**  [F6] When the sending of a GCA alert message (that is not in the
**       context of query results) completes, we always want to return to
**  the sequencer to send more (if there are any more alerts) and to clean
**  up (if there are no more alerts).  This return to the sequencer also
**  plays an important role in the modification of newly introduced session
**  alert state that each session has associated with alerts.  (This fact
**  is somewhat dependent on the solution below).
**
**  [F7] If an earlier sending of a GCA alert message completes while we
**       we are processing a query we must inhibit the GCA completion
**  routine for that alert (so as not to conflict with later CSsuspend
**  calls for other resources).  Why?  CSresume calls are sort of "queued"
**  and saved for a particlar thread - a subsequent CSsuspend to an orphan
**  CSresume notification will immediately resume.  Note that completion
**  routines associated with single GCA calls, and GCA_NO_ASYNC_EXIT flags
**  do not modify the solution.
**
**  [F8] When a session is invoked to process a query, an alert may already
**       be sent to the client (though may not necessarily have been read).
**  We make no attempt to flush all queued alerts to the client, as they
**  may not yet be ready to read them.  Any already queued alerts (and those
**  that are raised during this session's query processing) will eventually
**  be transmitted to the client with the GCA query results.  This was done
**  so as not to complicate the problem we already have.  Eventually this
**  may be modified as a performance enhancement.
**
**  [F9] Whenever possible we should try to get the alert to the client
**       without it being purged by GCA.  Consequently, between the time
**  that a user interrupt arrives and the time that the GCA_IACK message
**  is sent to the client, no queued alerts should be transformed into
**  GCA alert messages (they will be purged by SCC).  This is just a
**  performance enhancement, and references to GCA_IACK can be ignored
**  when trying to understand the total solution.
**
**  PROBLEMS TO DEAL WITH:
**  ---------------------
**
**  This section attempts to summarize the key problems that I am trying to
**  solve.  All of the problems arise from the requirement that we must
**  send the alert to the client outside of a query-result context.
**  Later in this note is an "alert state" description.  This state solves
**  these problems and more:
**
**  [P1] Concurrent sending of an alert message and the reading of a query
**       is a complex problem.  This is the key situation that we are trying
**  to resolve in this design note.  Typically, a client alert monitor will
**  be reading alerts and for each event will be issuing some kind of query.
**  While this query is running another alert may arrive.  The arrival of
**  the alert may be just before, during or after the processing of the
**  query in the sequencer.  Note that alerts that arrive during query
**  processing are no problem - they are sent off with the query results.
**
**  [P2] Concurrent writes by the DBMS must be avoided.  We have to make
**       sure not to send more than one message to the client.  This problem
**  must be avoided for multiple alerts (i.e. we must serialize sending each
**  individual alert to the client) and for alerts followed by query results
**  (an alert that is pending must make sure to complete before we can send
**  any query results).
**
**  [P3] Sending alerts to the client uses GCA (and its completion exit
**       scc_gcomplete).  Typically this completion routine resumes the
**  thread that was issuing the operation.  As mentioned above, we do not
**  suspend the thread on the send of the alert message when it is outside
**  of query result context.  Consequently, the CSresume normally driven by
**  the GCA completion exit (scc_gcomplete) may "land" us in unexpected
**  contexts if allowed to execute while we are inside the sequencer.  For
**  example, we may already have advanced into query processing when the
**  previous sending of the alert message completes.  We need to inhibit
**  the CSresume call to avoid leaving the CS "event done" flag set
**  (CS_EDONE_MASK) - this would cause the next CSsuspend to immediately
**  complete, probably without the necessary resource (i.e. a requested lock).
**  On the other hand, if no query is happening and the client does read
**  the alert, we would like to wake up the session so that it can continue
**  with the next alert (if there are any).
**
**  [P4] Most of the solution is based on a new session alert state field
**       (sscb_astate).  The modification of this field must be done under
**  strict control, and its various settings will guarantee that the session
**  is processing alerts as expected.  This state is synchronized with
**  the SCS query processing state, the GCA send & receive message buffer
**  states, and with the current queue of session-owned alerts.
**
*/

/*
**  Forward and/or External function references.
*/


GLOBALREF SC_MAIN_CB *Sc_main_cb; /* server control block */

/*
** {
** Name: ascs_sequencer	- the sequencer for the INGRES DBMS
**
** Description:
**      This routine performs that actions necessary to orchestrate the 
**      actions of the remaining facilities to get queries accomplished. 
**      The routine is driven by the communication blocks it receives  
**      from the dispatcher.  When communication blocks come in from 
**      the front ends, the routine figures out (possibly with some aid 
**      from other facilities such as PSF) what the appropriate action 
**      is.  Once determined, the routine passes control to other facilities 
**      as appropriate to accomplish the appropriate action.
**
** Inputs:
**	op_code				what sort of operation is expected.
**      scb                             session control block for this session.
**
** Outputs:
**	next_op				what operation is expected next
**	scb				State within sequencer may be altered.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-May-1998 (shero03)
**	    Support one special thread - the event thread
**      15-Feb-1999 (fanra01)
**          Add license checking into ascs_dbms_task.
**	01-dec-1999 (somsa01)
**	    Manual integration of change 441277;
**	    If a new session is terminated before it has completed
**	    initialisation, an ACCVIO may occur. The ACCVIO occurs because
**	    the SCB for the session is removed before an asynchronous message
**	    completes. At the same time, a "ULE_FORMAT error 0" may be
**	    raised. Bug 90634.
*/

DB_STATUS
ascs_sequencer(i4 op_code,
	      SCD_SCB *scb,
	      i4  *next_op )
{
    DB_STATUS           ret_val = E_DB_OK;
    DB_STATUS		status  = E_DB_OK;
    DB_STATUS		error;
    i4		ule_error;
    SCC_GCMSG		*message;
    i4			session_count = 0;
    i4			cont = 0;
    PTR			block;
    i4                  msg_type;

    *next_op = CS_INPUT;

    switch (op_code)
    {
    case CS_INPUT:
    case CS_INITIATE:

	if (   scb->scb_sscb.sscb_state == SCS_ACCEPT
	    && op_code == CS_INITIATE)
	{
	    /*
	    ** This is the first time the sequencer is called for 
	    ** this session. Respond to the client's association 
	    ** request now
	    */
	    GCA_RR_PARMS	rrparms;
	    STATUS		gca_status;
	    CS_SCB		*myscb;
	    
	    CSget_scb(&myscb);
	    
	    rrparms.gca_assoc_id = scb->scb_cscb.cscb_assoc_id;
	    rrparms.gca_local_protocol = scb->scb_cscb.cscb_version;
	    rrparms.gca_modifiers = 0;
	    rrparms.gca_request_status = OK;

	    /* If there is a FE associated with the session set the
	    ** DB_GCF_ID flag to ensure that the session is terminated
	    ** correctly even if the session is cancelled before
	    ** initialisation has completed.
	    */
	    scb->scb_sscb.sscb_facility |= (1 << DB_GCF_ID);
	    
	    status = IIGCa_call(GCA_RQRESP,
				(GCA_PARMLIST *)&rrparms,
				0,
				(PTR)myscb,
				(i4)-1,
				&gca_status);
	    
	    if (status != OK)
	    {
		/* The call to IIGCA_CALL has failed, so terminate the
		** session.
		*/
		sc0e_0_put(gca_status, 0);
		*next_op = CS_TERMINATE;
		ascd_note(E_DB_SEVERE, DB_GCF_ID);
		return(E_DB_SEVERE);
	    }

	    /* Message sent to FE, wait for response */

	    status = CSsuspend(CS_BIO_MASK, 0, 0);

	    if (status != OK)
	    {
		/* The session was Interrupted/Aborted so log state and
		** terminate.
		*/
		sc0e_0_put( status, 0);

		*next_op = CS_TERMINATE;
		ascd_note(E_DB_SEVERE, DB_SCF_ID);
		return(E_DB_SEVERE);
	    }

	    /* Check the status of the sent message */

	    if (rrparms.gca_status != OK)
	    {
		sc0e_0_put(rrparms.gca_status, &rrparms.gca_os_status);
		*next_op = CS_TERMINATE;

		if (rrparms.gca_status == E_GC0001_ASSOC_FAIL)
		{
		    /* The FE has terminated the connection, so no need to
		    ** notify that the connection has been lost.
		    */
		    scb->scb_sscb.sscb_state = SCS_TERMINATE;
		}
		ascd_note(E_DB_SEVERE, DB_SCF_ID);
		return(E_DB_SEVERE);
	    }
	    
	    scb->scb_cscb.cscb_gci.gca_status = E_SC1000_PROCESSED;
	    status = sc0m_allocate(0,
			(i4)(sizeof(SCC_GCMSG)),
			DB_SCF_ID,
			(PTR)SCS_MEM,
			SCG_TAG,
			&block);
	    message = (SCC_GCMSG *)block;
	    if (status)
	    {
		sc0e_0_put(status, 0);
		ascd_note(E_DB_SEVERE, DB_SCF_ID);
		return(E_DB_SEVERE);
	    }
	    message->scg_mask = 0;
	    message->scg_marea =
		((char *) message + sizeof(SCC_GCMSG));
	    message->scg_next = 
		(SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	    message->scg_prev = 
		scb->scb_cscb.cscb_mnext.scg_prev;
	    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = 
		message;
	    scb->scb_cscb.cscb_mnext.scg_prev = 
		message;

	    /*
	    ** Format the message 
	    */
	    message->scg_mtype = GCA_ACCEPT;
	    message->scg_msize = 0;

	    *next_op = CS_EXCHANGE;
	    scb->scb_sscb.sscb_state = SCS_INITIATE;
	    return(OK);
	}
	else if (scb->scb_sscb.sscb_state == SCS_INITIATE)
	{
	    /*
	    ** We have responded to the connection request and now it's
	    ** time to read the association parameters
	    */
	    ret_val = ascs_initiate(scb);
	    if ((!ret_val) && (scb->scb_sscb.sscb_state == SCS_INITIATE))
	    {
		/* In this case, we have more MD_ASSOC parms to read */
		*next_op = CS_INPUT;
		return(OK);
	    }
	    status = sc0m_allocate(0,
			(i4)(sizeof(SCC_GCMSG)
			    + sizeof(GCA_ER_DATA)
				+ (ret_val ? ER_MAX_LEN : 0)),
			DB_SCF_ID,
			(PTR)SCS_MEM,
			SCG_TAG,
			&block);
	    message = (SCC_GCMSG *)block;
	    if (status)
	    {
		    sc0e_0_put(status, 0 );
		    sc0e_0_uput(status, 0 );
		    ascd_note(E_DB_SEVERE, DB_SCF_ID);
		    message = (SCC_GCMSG *) scb->scb_cscb.cscb_inbuffer;
	    }
	    message->scg_mask = 0;
	    message->scg_marea =
		((char *) message + sizeof(SCC_GCMSG));
	    message->scg_next = 
		(SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	    message->scg_prev = 
		scb->scb_cscb.cscb_mnext.scg_prev;
	    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = 
		    message;
	    scb->scb_cscb.cscb_mnext.scg_prev = 
		    message;

	    if (ret_val)
	    {
		GCA_ER_DATA	    *emsg;
		GCA_DATA_VALUE  *gca_data_value;

		emsg = (GCA_ER_DATA *) message->scg_marea;
		gca_data_value = emsg->gca_e_element[0].gca_error_parm;

		emsg->gca_l_e_element = 1;
		emsg->gca_e_element[0].gca_id_server =
		    Sc_main_cb->sc_pid;
		emsg->gca_e_element[0].gca_severity = 0;
		emsg->gca_e_element[0].gca_l_error_parm = 1;
		  
		gca_data_value->gca_type = DB_CHA_TYPE;

		*next_op = CS_OUTPUT;    /* die if can't start up */
		scb->scb_sscb.sscb_state = SCS_TERMINATE;
		message->scg_mtype = GCA_RELEASE;
		message->scg_msize = sizeof(GCA_ER_DATA)
				    - sizeof(GCA_DATA_VALUE)
					+ sizeof(i4)
					+ sizeof(i4)
					+ sizeof(i4)
		    + gca_data_value->gca_l_value;
	    }
	    else
	    {
		    /*
		    ** Send accept message 
		    */
		    message->scg_mtype = GCA_ACCEPT;
		    message->scg_msize = 0;
		    scb->cs_scb.cs_mask |= CS_NOXACT_MASK;
		    *next_op = CS_EXCHANGE;
	    }

	    /*
	    ** Send out the message and break out to the normal
	    ** request processing loop 
	    */
	    status = ascs_gca_send(scb);
	    if (DB_FAILURE_MACRO(status))
	    {
		i4 line = __LINE__;
		sc0e_put(E_SC0502_GCA_ERROR, 0, 2,
			 sizeof(__FILE__), __FILE__,
			 sizeof(line), (PTR)&line,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		*next_op = CS_TERMINATE;
		scb->scb_sscb.sscb_state = SCS_TERMINATE;
		return(E_DB_SEVERE);
	    }
	    break;
	}

	else if (scb->scb_sscb.sscb_state == SCS_SPECIAL)
	{
	    /*
	    ** This is specialized dbms task - call routine to
	    ** do the specific task.
	    */

	    /*
	    ** Set up thread to terminate if task returns.
	    ** If this thread is necessary for the server to function
	    ** (sscb_flags is SCS_VITAL) then it is a server fatal
	    ** for this thread to fail to initiate.
	    */
	    *next_op = CS_TERMINATE;
	    scb->scb_sscb.sscb_state = SCS_TERMINATE;

	    ret_val = ascs_initiate(scb);
	    if (ret_val)
	    {
		sc0e_put(E_SC0240_DBMS_TASK_INITIATE, 0, 1,
			 sizeof(*scb->scb_sscb.sscb_ics.ics_eusername),
			 (PTR)scb->scb_sscb.sscb_ics.ics_eusername,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		if (scb->scb_sscb.sscb_flags & SCS_VITAL)
		    return (E_DB_FATAL);
		return (E_DB_ERROR);
	    }

	    /*
	    ** Scs_dbms_task should not return until the thread is
	    ** ready to exit.  If the thread exits abnormally, the
	    ** error should have been logged in scs_dbms_task.
	    ** If the thread is a vital thread and it exits before
	    ** server shutdown time, then we must bring down the server,
	    ** if the vital task is supposed to be permanent.
	    */
	    /*
            ** Add support for license thread
            ** Move SCS_EVENT handling back into ascs_dbms_task.
            */
            ret_val = ascs_dbms_task(scb);
			
	    if (	(ret_val == E_DB_FATAL)
		||
		    (   (	(scb->scb_sscb.sscb_flags &
					    (SCS_VITAL | SCS_PERMANENT))
				== (SCS_VITAL | SCS_PERMANENT)
			)
		    &&
			(Sc_main_cb->sc_state != SC_SHUTDOWN)
		    )
	       )
	    {
		sc0e_put(E_SC0241_VITAL_TASK_FAILURE, 0, 1,
			 sizeof(*scb->scb_sscb.sscb_ics.ics_eusername),
			 (PTR)scb->scb_sscb.sscb_ics.ics_eusername,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		CSterminate(CS_KILL, &session_count);
		return (E_DB_FATAL);
	    }
	    return (ret_val);
	}

	else if (scb->scb_sscb.sscb_state == SCS_INPUT)
	{
	    /*
	    ** Here we process messages received from the client
	    */
	    i4  line = __LINE__;
	    sc0e_put(E_SC0501_INTERNAL_ERROR, 0, 2,
		     sizeof(__FILE__), __FILE__,
		     sizeof(line), (PTR)&line,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
            scb->scb_sscb.sscb_state = SCS_TERMINATE;
	    *next_op = CS_TERMINATE;
	    return(E_DB_SEVERE);
	}
	else
	{
	    TRdisplay(">>>>>>>>20C error 1: sscb_state = %x (%<%d.)\n",
			scb->scb_sscb.sscb_state);
	    sc0e_0_put(E_SC020C_INPUT_OUT_OF_SEQUENCE, 0);
	    return(E_DB_ERROR);
	}

    case CS_OUTPUT:
	*next_op = CS_OUTPUT;	    /* assumption is to continue */
	    /* These take care of themselves below */
	if (scb->scb_sscb.sscb_state == SCS_TERMINATE)
	{
	    *next_op = CS_TERMINATE;
	    return(E_DB_OK);
	}
	break;

    case CS_TERMINATE:

	/*
	** Coordinate termination with GCA unless there is not front
	** end process associated with this thread.
	*/
	    if ( scb->scb_sscb.sscb_state != SCS_TERMINATE &&
		 scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID) )
	{
	    /* FIXME: The following static works only because the GCA
	    ** message that is placed in the buffer 'b' is the same
	    ** for all threads that will use it. This is no excuse however,
	    ** and we should implement a correct solution ASAP. The
	    ** static is used right now because it will work and we
	    ** are trying to go FCS.
	    ** Two possible correct solutions are: A) allocate the message
	    ** buffer from heap memory and have CS send and dealocate
	    ** the message (there must be something wrong here tho'. 
	    ** B) allocate and initialize the message at server startup
	    ** and have all threads use the common message.
	    ** Of the two, I like A.
	    **	    eric 15 Dec 88
	    */
	    static char		b[ER_MAX_LEN + sizeof(GCA_ER_DATA)];
	    GCA_ER_DATA		*ebuf = (GCA_ER_DATA *)b;
	    GCA_DATA_VALUE		*gca_data_value;

	    /* If not asked by FE to die, better tell them */
	    ebuf->gca_l_e_element = 1;
	    if (scb->scb_cscb.cscb_version <= GCA_PROTOCOL_LEVEL_2)
	    {
		ebuf->gca_e_element[0].gca_local_error = 0;
		ebuf->gca_e_element[0].gca_id_error =
		    E_SC0206_CANNOT_PROCESS;
	    }
	    else if (scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_60)
	    {
		ebuf->gca_e_element[0].gca_local_error =
		    E_SC0206_CANNOT_PROCESS;
		ebuf->gca_e_element[0].gca_id_error =
		    E_GE98BC_OTHER_ERROR;
	    }
	    else
	    {
		ebuf->gca_e_element[0].gca_local_error =
		    E_SC0206_CANNOT_PROCESS;
		ebuf->gca_e_element[0].gca_id_error =
		    ss_encode(SS50000_MISC_ERRORS);
	    }

	    ebuf->gca_e_element[0].gca_id_server = Sc_main_cb->sc_pid; 
	    ebuf->gca_e_element[0].gca_severity = 0;
	    ebuf->gca_e_element[0].gca_l_error_parm = 1;

	    gca_data_value = ebuf->gca_e_element[0].gca_error_parm;
	    
	    gca_data_value->gca_type = DB_CHA_TYPE;
	    
	    status = ule_format(E_SC0206_CANNOT_PROCESS,
		0,
		ULE_LOOKUP,
		(DB_SQLSTATE *) NULL,
		gca_data_value->gca_value,
		ER_MAX_LEN,
		&gca_data_value->gca_l_value,
		&ule_error,
		0);

	    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_association_id =
				    scb->scb_cscb.cscb_assoc_id;
	    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_buffer = b;
	    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_message_type =
		GCA_RELEASE;
	    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_msg_length =
				 sizeof(GCA_ER_DATA)
					- sizeof(GCA_DATA_VALUE)
					    + sizeof(i4)
					    + sizeof(i4)
					    + sizeof(i4)
					    + gca_data_value->gca_l_value;
	    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_end_of_data = TRUE;
	    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_modifiers = 0;
	    status = IIGCa_call(GCA_SEND,
				(GCA_PARMLIST *)&scb->scb_cscb.cscb_gcp.gca_sd_parms,
				GCA_NO_ASYNC_EXIT,
				(PTR)0,
				-1,
				&error);
	}
	CSintr_ack();
	/* Terminate the session only if has aleady started */
	if (scb->scb_sscb.sscb_state != SCS_INITIATE)
	    ret_val = ascs_terminate(scb);

	/*
	** If the thread being terminated is necessary in order for the
	** server to function, then signal a fatal error and shut down.
	*/
	if (((scb->scb_sscb.sscb_flags & (SCS_VITAL | SCS_PERMANENT))
	    == (SCS_VITAL | SCS_PERMANENT)) &&
	    (Sc_main_cb->sc_state != SC_SHUTDOWN))
	{
	    sc0e_put(E_SC0241_VITAL_TASK_FAILURE, 0, 1,
		     sizeof(*scb->scb_sscb.sscb_ics.ics_eusername),
		     (PTR)scb->scb_sscb.sscb_ics.ics_eusername,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
	    CSterminate(CS_KILL, &session_count);
	    return (E_DB_FATAL);
	}
	return(ret_val);
	break;

    default:
	sc0e_0_put(E_SC0024_INTERNAL_ERROR, 0);
	sc0e_put(E_SC0205_BAD_SCS_OPERATION, 0, 1,
		 sizeof (op_code), (PTR)&op_code);
	sc0e_0_put(E_SC021F_BAD_SQNCR_CALL, 0);
	ascd_note(E_DB_SEVERE, DB_SCF_ID);
    }

    /*
    ** Check if this is an iimonitor  connection
    */
    if (scb->scb_sscb.sscb_stype == SCS_SMONITOR)
    {
	return(ascs_msequencer(op_code, scb, next_op));
    }

    /*
    ** Now that we are done with the session start-up
    ** part, we fall into the request processing loop.
    ** It will continue until we receive the release
    ** request from the client, or until an error 
    ** has occurred
    */
    scb->scb_sscb.sscb_nostatements = 0;
    do 
    {
	/*
	** Receive and interpret the message from FE
	*/
	status = ascs_gca_recv(scb);
	if (DB_FAILURE_MACRO(status))
	    break;

	msg_type = scb->scb_cscb.cscb_gci.gca_message_type;

	/*
	** Process the request. This will also take care of
	** interrupts if there are any pending
	*/
	status = ascs_process_request(scb);
	if (status > E_DB_ERROR)
	    break;
        else 
            status = E_DB_OK;

	/*
	** Flush the excess from the receive buffer 
	*/
	if (msg_type != GCA_RELEASE)
	{
	    status = ascs_gca_flush(scb);
	    if (DB_FAILURE_MACRO(status))
		break;
	}

	/*
	** Increment the statement counter. It is useful for 
	** assigning ID's to tuple descriptors, etc.
	*/
	scb->scb_sscb.sscb_nostatements ++;

    } while (msg_type != GCA_RELEASE && status == E_DB_OK);

    /*
    ** Once we are here, the session has to be terminated
    */
    *next_op = CS_TERMINATE;
    scb->scb_sscb.sscb_state = SCS_TERMINATE;

    /*
    ** See if the server must be terminated. This may be
    ** the case if a fatal exception occurred while we
    ** were processing the previous request
    */
    if (status >= E_DB_FATAL)
	CSterminate(CS_CLOSE, &session_count);

    return ((status <= E_DB_ERROR) ? E_DB_OK : status);
}


/*
** Name: ascs_process_request -- dispatch request 
** Descripition:
** Input:
** Output:
** Return:
** History:
*/
DB_STATUS 
ascs_process_request(SCD_SCB *scb)
{
    DB_STATUS status;
    i4        mode;

    mode = scb->scb_cscb.cscb_gci.gca_message_type;

    /*
    Ntrace(88), "Starting SEQ ==> OMS: time=%ld, clock=%ld\n", 
	time(NULL), clock());
    */

    switch  (mode)
    {
    case GCA_QUERY:
	status = ascs_process_query(scb);
	break;
    
    case GCA1_INVPROC:
	status = ascs_process_procedure(scb);
	break;

    case GCA_RELEASE:
	scb->scb_sscb.sscb_state = SCS_TERMINATE;
	status = E_DB_OK;
	break;

    case GCA_FETCH:
    case GCA1_FETCH:
	status = ascs_process_fetch(scb);
	break;

    case GCA_CLOSE:
	status = ascs_process_close(scb);
	break;

    case GCA_COMMIT:
    case GCA_ABORT:
    case GCA_ROLLBACK:
	/*
	** Here we do nothing. All transaction management is done 
	** from odbExecODQL, which is sent via GCA_QUERY. Just 
	** format and send the response message
	*/
	status = ascs_format_response(scb, GCA_DONE, 
	    GCA_OK_MASK, GCA_NO_ROW_COUNT);
	if (status != E_DB_OK)
	    return (status);
	status = ascs_gca_send(scb);
	break;

    case GCA_ATTENTION:
	/* process the interrupt */
	status = ascs_process_interrupt(scb);
	if (status != E_DB_OK)
	    break;
	status = ascs_gca_send(scb);
	break;
 	
    default:
	{
	    i4  line = __LINE__;
	    sc0e_put(E_SC0501_INTERNAL_ERROR, 0, 2,
		     sizeof(__FILE__), __FILE__,
		     sizeof(line), (PTR)&line,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
            scb->scb_sscb.sscb_state = SCS_TERMINATE;
	    return(E_DB_SEVERE);
	}
    }

    /* 
    Ntrace(88), "Ending SEQ ==> OMS: time = %ld, clock = %ld\n",
	time(NULL), clock());
    */

    return (status);
}


/*
** Name: ascs_process_interrupt -- process a GCA interrupt
** Descripition:
** Input:
** Output:
** Return:
** History:
*/
DB_STATUS
ascs_process_interrupt(SCD_SCB *scb)
{
    DB_STATUS status = E_DB_OK;
    PTR       block;
    DB_STATUS error;
    SCC_GCMSG *message;
#if 0
    OMF_CB    *omf_cb = (OMF_CB *)scb->scb_sscb.sscb_omscb;
#endif /* 0 */

    CSintr_ack();

#if 0
    /*
     * Call OMF to do a clean up 
     * Now this is a no-op
     */
    status = omf_call(OMF_CANCEL, (PTR)omf_cb->omf_rcb);
    if (DB_FAILURE_MACRO(status))
	return (E_DB_SEVERE);
#endif /* 0 */

    if (scb->scb_cscb.cscb_gci.gca_status == E_GC0000_OK)
    {
	/* the request is processed already;
	   update the status accordingly */
	scb->scb_cscb.cscb_gci.gca_status = E_SC1000_PROCESSED;
    }
    else if (CLERROR(scb->scb_cscb.cscb_gci.gca_status))
    {
	/* The interrupt was caused by a communication
	   error -- drop the connection and end the session */
	scb->scb_sscb.sscb_flags |= SCS_DROPPED_GCA;
	ascs_disassoc(scb->scb_cscb.cscb_assoc_id);
	if (scb->scb_sscb.sscb_interrupt < 0)
	    scb->scb_sscb.sscb_interrupt = -scb->scb_sscb.sscb_interrupt;
	return (E_DB_SEVERE);
    }

    /*
     * Send the acknowledgment message
     * or terminate the session if the
     * connection was lost
     */
    if (scb->scb_sscb.sscb_flags & SCS_DROP_PENDING)
    {
	/* The session is being dropped by SCF */
	scb->scb_sscb.sscb_flags &= ~SCS_DROP_PENDING;
	scb->scb_sscb.sscb_flags |= SCS_DROPPED_GCA;
	ascs_disassoc(scb->scb_cscb.cscb_assoc_id);
	if (scb->scb_sscb.sscb_interrupt < 0)
	    scb->scb_sscb.sscb_interrupt = -scb->scb_sscb.sscb_interrupt;
	return (E_DB_SEVERE);
    }
    else
    {
	scb->scb_sscb.sscb_interrupt = 0;
	scc_iprime(scb);
	
	/*
	 * Send the acknowledgement
	 */
	status = sc0m_allocate(0,
			       (i4)(sizeof(SCC_GCMSG) + sizeof(GCA_AK_DATA)),
			       DB_SCF_ID,
			       (PTR)SCS_MEM,
			       SCG_TAG,
			       &block);
        if (status)
	{
	    sc0e_0_put(status, 0);
	    sc0e_0_uput(status, 0);
	    ascd_note(E_DB_SEVERE, DB_SCF_ID);
	}
	scc_dispose(scb);
	message = (SCC_GCMSG *)block;
	message->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
	scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
	scb->scb_cscb.cscb_mnext.scg_prev = message;
	message->scg_mask = 0;
	message->scg_mtype = GCA_IACK;
	message->scg_msize = sizeof(GCA_AK_DATA);

        /* Avoid purging alerts (1 may already be lost) */
        scb->scb_sscb.sscb_aflags |= SCS_AFPURGE;
	message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));
    }

    return (status);
}
