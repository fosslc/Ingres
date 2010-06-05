/* Copyright (c) 1987, 2004 Ingres Corporation
**
**
*/


#include    <compat.h>
#include    <erglf.h>
#include    <gl.h>
#include    <cv.h>
#include    <cs.h>
#include    <sl.h>
#include    <er.h>
#include    <me.h>
#include    <st.h>
#include    <tr.h>
#include    <ex.h>
#include    <pc.h>
#include    <tm.h>
#include    <st.h>
#include    <sp.h>
#include    <mo.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <scf.h>
#include    <gca.h>
#include    <ddb.h>
#include    <qefrcb.h>
#include    <psfparse.h>

#include    <duf.h>
#include    <dudbms.h>
#include    <dmccb.h>
#include    <dmrcb.h>

#include <copy.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scserver.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>

#include    <sc0e.h>
#include    <sc0m.h>
#include    <scfcontrol.h>
#include    <sxf.h>
#include    <sc0a.h>

/**
**
**  Name: SCSMNTR.C - Monitor functions for the SCS module
**
**  Description:
**      This file contains a collection of routines used to by the 
**      system monitor to obtain human readable information about 
**      the state of the system.
**
**	    scs_msequencer() - Operate the monitor session
[@func_list@]...
**
**
**  History:    $Log-for RCS$
**      24-Mar-1987 (fred)
**          Created.
**	25-oct-89 (paul)
**	    changed scs_tput to sc0e_tput
**	10-oct-90 (ralph)
**	    6.3->6.5 merge
**	15-jan-91 (ralph)
**	    Correct TRdisplay for session group id
**	13-jul-92 (rog)
**	    Moved scs_format out of this file and into scssvc.c so that I
**	    could have scf_call call it and be able to link on VMS.
**	    Included ddb.h and qefrcb.h for Sybil.
**	21-jul-92 (rog)
**	    Fix compilation error: qefrcb.h was included too early.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	30-nov-92 (robf)
**	    Remove references to SUPERUSER, check for appropriate 
**	    combos of SECURITY, OPERATOR etc
**	2-Jul-1993 (daveb)
**	    prototyped.
**      6-Sep-1993 (smc)
**          Added st.h include.
**	8-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	26-Jul-1993 (daveb)
**	    Create scs_monitor function to interpret (some) iimonitor
**	    commands here.
**	16-Sep-1993 (daveb)
**	    tweaked to match details of integration plan.  Turn off
**	    "show connection" and "kill" done here.
**	25-oct-93 (robf)
**	    Made scs_scan_connections/drop_connection external
**	1-Nov-1993 (daveb)
**	    Match LRC proposal.  Drop becomes remove, remove becomes kill.
**      10-Nov-1993 (daveb)
**          Match approved proposal.  Kill becomes "crash session SESSIONID"
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT. This results in owner within sc0m_allocate having to
**	    be PTR.
**          In addition calls to sc0m_allocate within this module had the
**          3rd and 4th parameters transposed, this has been remedied.
**          Also fixed up cast of validate in scs_scan_connections call.
**	03-dec-93 (swm)
**	    Bug #58641
**	    Use CVaxptr for hex string to pointer conversion, not CVahxl and
**	    remove unnecessary cast of result parameter.
**	09-mar-94 (swm)
**	    Bug #60425
**	    CS_SID values were cast to i4  which truncate on 64-bit platforms.
**	    Also, CS_SIDs were displayed with %x rather than %p, %x truncates
**	    on 64-bit platforms.
**      14-jun-95 (chech02)
**          Added SCF_misc_sem semaphore to protect critical sections.(MCT)
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	03-jun-1996 (canor01)
**	    Semaphore protect sc_proxy_scb while it is being swapped.
**	12-dec-1996 (canor01)
**	    "Start sampling" will create a new internal thread to sample
**	    sessions at specified times.
**    18-feb-1997 (hanch04)
**        As part of merging of Ingres-threaded and OS-threaded servers,
**        check to see which threads are running.
**      25-Feb-98 (fanra01)
**          Modified the scs_scan_scbs to use a new index method based on scb
**          pointer since scb_index is now thread id.
**	31-Mar-1998 (jenjo02)
**	    Added *thread_id output parm to CSadd_thread() prototype.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Pass session's SCB* to scc_trace rather than using
**	    sc_proxy_scb and sc_misc_semaphore.
**	19-Jan-2001 (jenjo02)
**	    Stifle calls to sc0a_write_audit() if not C2SECURE.
**	11-Mar-2003 (hanje04)
**	    Include sc0a.h so that sc0a_write_audit() is correctly prototyped
**	    also change type of error from i4 to DB_ERROR in scs_mon_audit()
**    04-Jan-2004 (hanje04)
**        Removed line break causing compilation problems on Linux.
**      05-May-2005 (horda03) Bug 114453/INGSRV 3290
**          Add message to indicate server control command being executed.
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Forward and/or External function references.
*/

typedef struct
{
    SCD_SCB	*scb;
    bool	result;
} SCS_VALIDATE_ARGS;    

static void scs_crash_sess( SCD_SCB *scb ) ;

static void scs_kill_query( SCD_SCB *scb );

static SCS_SES_OP_FUNC scs_kill_conn;

static STATUS scs_monitor( i4  mode,
			  CS_SCB *emp_scb,
			  i4  *nmode,
			  char *command,
			  i4 powerful,
			  i4 (*output_fcn)(PTR, i4, char *) );

static STATUS scs_show_func( SCD_SCB *scb, PTR powerfulp );

static STATUS scs_broadcast_mesg( SCD_SCB *scb, PTR mesgstring );

static bool scs_is_user_scb( SCD_SCB *scb );

static STATUS scs_validate_scb( SCD_SCB *scb, PTR arg );

static  VOID scs_mon_audit( SCD_SCB* scb, SXF_ACCESS access, i4  mesgid);

/*{
** Name: scs_msequencer	- Sequence monitor calls
**
** Description:
**      Acts just like the "regular" sequencer, except that it
**      interfaces with the CS code to get monitor functions.
**
** Inputs:
**      op_code                         What operation you are called for
**      scb                             Pointer to your SCB
**      ioptr                           where the input is.
**
** Outputs:
**      next_op                         what to do next
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-Jul-1987 (fred)
**          Created.
**	21-may-91 (andre)
**	    renamed some fields in SCS_ICS to make it easier to determine by the
**	    name whether the field contains info pertaining to the system (also
**	    known as "Real"), Session ("he who invoked us"), or the Effective
**	    identifier:
**		ics_ustat	-->	    ics_rustat
**	2-Jul-1993 (daveb)
**	    prototyped, use proper PTR for allocation.
**	6-jul-93 (shailaja)
**	   Fixed prototype incompatibility.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**      19-Oct-2004 (hanal04) Bug 113259 INGSRV3005
**          If the user hits CTRL-C in iimonitor without issuing any previous
**          commands scb->scb_sscb.sscb_troot will be NULL. The call to
**          scs_monitor() references through scb->scb_sscb.sscb_troot and
**          was hitting a SIGSEGV.
**	15-Sep-2004 (schka24)
**	    The code to run "remove" hasn't worked since 1994, when someone
**	    changed DROP to REMOVE and forgot to fix the STscompare call.
**	    Therefore, removes were being done (harshly) from the CS.
**	    Fix, so that we actually take an attn interrupt and give
**	    the targeted session a chance to shut itself down nicely.
[@history_template@]...
*/

i4
scs_msequencer(i4  op_code,
	       SCD_SCB  *scb,
	       i4  *next_op )
{
    DB_STATUS		status;
    DB_STATUS		error;
    SCC_GCMSG		*message;
    QSF_RCB		*qs_ccb;
    GCA_TD_DATA		*tdescr;
    PTR			block;

    scb->scb_cscb.cscb_gci.gca_status = E_SC1000_PROCESSED;
    scb->cs_scb.cs_mask &= ~(CS_NOXACT_MASK);

    for (;;)
    {
	status = sc0m_allocate(0,
			    (i4)(sizeof(SCC_GCMSG)
				+ sizeof(GCA_TD_DATA)
				- sizeof(tdescr->gca_col_desc[0].gca_attname)),
 			    DB_SCF_ID,
			    (PTR)SCS_MEM,
			    CV_C_CONST_MACRO('m','n','t','r'),
			    &block);
	message = (SCC_GCMSG *)block;
	if (status)
	{
	    sc0e_0_put(status, 0);
	    sc0e_0_uput(status, 0);
	    scb->scb_sscb.sscb_state = SCS_TERMINATE;
	    *next_op = CS_TERMINATE;
	    status = E_DB_SEVERE;
	    break;
	}
	message->scg_mask = 0;
	message->scg_mtype = GCA_TDESCR;
	message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));
	message->scg_msize = sizeof(GCA_TD_DATA)
			- sizeof(tdescr->gca_col_desc[0].gca_attname);
	tdescr = (GCA_TD_DATA *)message->scg_marea;
	tdescr->gca_result_modifier = 0;
	tdescr->gca_l_col_desc = 1;
	tdescr->gca_col_desc[0].gca_l_attname = 0;
	tdescr->gca_col_desc[0].gca_attdbv.db_prec = 0;
	tdescr->gca_col_desc[0].gca_attdbv.db_data = 0;
	tdescr->gca_col_desc[0].gca_attdbv.db_datatype = DB_VCH_TYPE;
	message->scg_next =
	    (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	message->scg_prev =
	    scb->scb_cscb.cscb_mnext.scg_prev;
	scb->scb_cscb.cscb_mnext.scg_prev->scg_next =
	    message;
	scb->scb_cscb.cscb_mnext.scg_prev =
	    message;

/*	status = sc0m_allocate(0,
			    (i4)(sizeof(SCC_GCMSG)
				+ scb->scb_cscb.cscb_comm_size),
			    SCS_MEM,
			    DB_SCF_ID,
			    CV_C_CONST_MACRO('m','n','t','r'),
			    &message);
	if (status)
	{
	    sc0e_put(status, 0);
	    sc0e_uput(status, 0);
	    scb->scb_sscb.sscb_state = SCS_TERMINATE;
	    *next_op = CS_TERMINATE;
	    status = E_DB_SEVERE;
	    break;
	}
	message->scg_mask = 0;
	message->scg_mtype = GCA_TUPLES;
	message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));
	message->scg_msize = scb->scb_cscb.cscb_comm_size;
	text_string =
	    (DB_TEXT_STRING *)message->scg_marea;
	MEcopy(((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_qrytext,
		((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_qrysize,
		text_string->db_t_text);
	text_string->db_t_text[
		((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_qrysize] = '\0';
*/
	qs_ccb = scb->scb_sscb.sscb_qsccb;
	/*
	**	execute monitor. user is "powerful" if has either
	**	SECURITY or OPERATOR privileges
	*/
	if(scb->scb_sscb.sscb_troot)
	{
	    status = scs_monitor(op_code,
			    (CS_SCB *)scb,
			    next_op,
			    ((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_qrytext,
			    scb->scb_sscb.sscb_ics.ics_rustat &
					(DU_USECURITY|DU_UOPERATOR)?1:0,
			    sc0e_tput);
        }
/*	message->scg_mask = 0;
	message->scg_mtype = GCA_TUPLES;
	text_string->db_t_count = STlength(text_string->db_t_text);
	message->scg_msize = text_string->db_t_count + 2;
	tdescr->gca_col_desc[0].gca_attdbv.db_length = message->scg_msize;
	tdescr->gca_tsize = message->scg_msize;
	message->scg_next =
	    (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	message->scg_prev =
	    scb->scb_cscb.cscb_mnext.scg_prev;
	scb->scb_cscb.cscb_mnext.scg_prev->scg_next =
	    message;
	scb->scb_cscb.cscb_mnext.scg_prev =
	    message;
*/	status = sc0m_allocate(0,
			    (i4)(sizeof(SCC_GCMSG) + sizeof(GCA_RE_DATA)),
			    DB_SCF_ID,
			    (PTR)SCS_MEM,
			    CV_C_CONST_MACRO('m','n','t','r'),
			    &block);
	message = (SCC_GCMSG *)block;
	if (status)
	{
	    sc0e_0_put(status, 0);
	    sc0e_0_uput(status, 0);
	    scb->scb_sscb.sscb_state = SCS_TERMINATE;
	    *next_op = CS_TERMINATE;
	    status = E_DB_SEVERE;
	    break;
	}
	message->scg_mask = 0;
	message->scg_mtype = GCA_RESPONSE;
	message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));
	message->scg_msize = sizeof(GCA_RE_DATA);
	message->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
	scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
	scb->scb_cscb.cscb_mnext.scg_prev = message;

	break;
    }
    if (scb->scb_sscb.sscb_thandle)
    {
	qs_ccb = scb->scb_sscb.sscb_qsccb;
	qs_ccb->qsf_lk_id = scb->scb_sscb.sscb_tlk_id;
	qs_ccb->qsf_obj_id.qso_handle = scb->scb_sscb.sscb_thandle;
	status = qsf_call(QSO_DESTROY, qs_ccb);
	if (DB_FAILURE_MACRO(status))
	{
	    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
					E_SC0210_SCS_SQNCR_ERROR);
	}
	scb->scb_sscb.sscb_thandle = 0;
    }
    scb->scb_sscb.sscb_state = SCS_INPUT;
    return(status);
}


/*{
** Name: scs_monitor	- Implement the SCS part of the monitor task
**
** Description:
**	This routine is called ala the regular thread processing routine.
**	It parses its input, decides what to do, and returns the output.
**
**	The commmands completely interpreted here are:
**
**		set server shut	(CS_CLOSE)
**			Disallow new connections, shutdown when
**			last current session exits.
**          
**          	remove SESSION
**          	    	New improved "safe" version, acts the same
**  	    	    	as front-end exiting and dropping GCA connection.
**	
**		kill SESSION
**			Signal CS_KILL_EVENT to a user session actively
**			running a query.
**          
**	NEW:
**		set server closed
**			Disallow new regular user sessions.
**          
**		set server open
**			Reallow new regular user sessions.  Cancel
**			any pending 'set server shut' shutdown.
**
**		show server listen
**
**		crash session SESSIONID
**
**	Commands partially handled here and partially in CSmonitor are:
**
**		stop server	(CS_KILL)
**			Kill user sessions, shutdown when they're gone.
**          
**		stop server conditional (CS_COND_CLOSE)
**			Shutdown if no sessions, which never works because
**			this session always exists.
**          
**		crash server	(CS_CRASH)
**			Take the server down immediately.
**
**	All other commands are passed to CSmonitor for handling.
**
** Inputs:
**	mode				Mode of operation
**					    CS_INPUT, _OUPUT, ...
**	scb				Sessions control block to monitor
**	*command			Text of the command
**	powerful			Is this user powerful
**	output_fcn			Function to call to perform the output.
**					This routine will be called as
**					    (*output_fcn)(newline_present,
**							    length,
**							    buffer)
**					where buffer is the length character
**					output string, and newline_present
**					indicates whether a newline needs to
**					be added to the end of the string.
**
** Outputs:
**	next_mode			As above.
**	Returns:
**	    OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-Jul-1993 (daveb)
**	    created.
**	15-Sep-1993 (daveb)
**	    ifdef out all but drop connection for now.
**	1-Nov-1993 (daveb)
**	    Match LRC proposal.  Drop becomes remove, remove becomes kill.
**      10-Nov-1993 (daveb)
**          Match approved proposal.  Kill becomes "crash session SESSIONID"
**	15-dec-93 (robf)
**          Add prototype "broadcast" message request.
**	4-mar-94 (robf)
**          Add initial security auditing to iimonitor events.
**	12-dec-1996 (canor01)
**	    Add support for sampler thread.
**	24-Apr-2003 (jenjo02)
**	    Added "kill" command to abort eligible queries
**	    while leaving the session intact, SIR 110141.
**	17-Sep-2004 (schka24)
**	    Manual fix to remove command so that it fires.
**      05-may-2005 (horda03) Bug 114453/INGSRV 3290
**          For server/session changes log the command.
**      22-may-2007 (horda03) Bug 117966
**          Log the command before calling CSmonitor, as
**          the CS could be crashing the server.
**      23-Sep-2009 (hanal04) Bug 115316
**          Added "SHOW SERVER CAPABILITIES".
*/
static STATUS
scs_monitor( i4 mode, CS_SCB *scb, i4 *nmode, char *command,
	    i4 powerful, i4 (*output_fcn)(PTR, i4, char *) )
{
    STATUS		ret_stat;
    char		buf[81];
    bool		completely_done;
    PTR			ptr_scb;
    SCD_SCB		*an_scb;
    i4                  log_cmd = 0;
    i4                  local_error;
    SCD_SCB		*my_scb = (SCD_SCB*)scb;
    
    *nmode = CS_EXCHANGE;
    completely_done = FALSE;
    ret_stat = OK;
    
    switch (mode)
    {
    case CS_INITIATE:
	*nmode = CS_INPUT;
	break;
	
    case CS_TERMINATE:
	break;
	
    case CS_INPUT:
		
	CVlower(command);
	if (STscompare("setservershut", 0, command, 0) == 0)
	{
	    /* Audit log the attempt here? */
	    if (!powerful)
	    {
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Superuser status required to stop servers");
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		    scs_mon_audit((SCD_SCB*)scb, 
			    SXF_A_CONTROL|SXF_A_FAIL,
			    I_SX274D_SET_SERVER_SHUT
			    );
	    }
	    else /* disallow regular listens, exit when last conn exits */
	    {
		Sc_main_cb->sc_listen_mask = 
		    (SC_LSN_TERM_IDLE |SC_LSN_SPECIAL_OK) ;
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Server will stop. %d. sessions remaining",
			 Sc_main_cb->sc_current_conns );
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		    scs_mon_audit((SCD_SCB*)scb, 
			    SXF_A_CONTROL|SXF_A_SUCCESS,
			    I_SX274D_SET_SERVER_SHUT
			    );
                log_cmd = 1;
	    }
	    completely_done = TRUE;
	}
	else if (STscompare("setserverclosed", 0, command, 0) == 0)
	{
	    /* Audit log the attempt here? */
	    if (!powerful)
	    {
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Superuser status required to disable connections");
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		    scs_mon_audit((SCD_SCB*)scb, 
			    SXF_A_CONTROL|SXF_A_FAIL,
			    I_SX2748_SET_SERVER_CLOSED
			    );
	    }
	    else		/* Disallow regular listens */
	    {
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		    scs_mon_audit((SCD_SCB*)scb, 
			    SXF_A_CONTROL|SXF_A_SUCCESS,
			    I_SX2748_SET_SERVER_CLOSED
			    );
		Sc_main_cb->sc_listen_mask &= (~SC_LSN_REGULAR_OK);
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "User connections now disabled" );
                log_cmd = 1;
	    }
	    completely_done = TRUE;
	}
	else if (STscompare("setserveropen", 0, command, 0) == 0)
	{
	    /* Audit log the attempt here? */
	    if (!powerful)
	    {
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Superuser status required to enable connections");
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		    scs_mon_audit((SCD_SCB*)scb, 
			    SXF_A_CONTROL|SXF_A_FAIL,
			    I_SX2749_SET_SERVER_OPEN
			    );
	    }
	    else  /* allow all listens, cancel any impending shutdown */
	    {
		Sc_main_cb->sc_listen_mask =
		    (SC_LSN_REGULAR_OK | SC_LSN_SPECIAL_OK);
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "User connections now allowed" );
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		    scs_mon_audit((SCD_SCB*)scb, 
			    SXF_A_CONTROL|SXF_A_SUCCESS,
			    I_SX2749_SET_SERVER_OPEN
			    );
                log_cmd = 1;
	    }
	    completely_done = TRUE;
	}
	else if (STncasecmp("broadcast", command, 9) == 0)
	{
	    /* Audit log the attempt here? */
	    if (!powerful)
	    {
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Superuser status required to broadcast messages");
	    }
	    else  
	    {
		/*
		** Broadcast the message to any connected sessions
		*/
		scs_scan_scbs(scs_broadcast_mesg, (PTR)(command+10));
	    }
	    completely_done = TRUE;
	}
	else if (STscompare("stopserverconditional", 0, command, 0) == 0 ||
		 STscompare("stopserver", 0, command, 0) == 0 ||
		 STscompare("crashserver", 0, command, 0) == 0 )
	{
	    /* Audit log the attempt here? */
	    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		scs_mon_audit((SCD_SCB*)scb, 
			powerful?
			    SXF_A_CONTROL|SXF_A_SUCCESS:
			    SXF_A_CONTROL|SXF_A_FAIL,         /* Action */
			I_SX274A_STOP_SERVER
			);
                log_cmd = powerful;
	}
        else if (STscompare("showservercapabilities", 0, command, 0) == 0)
        {
            TRformat(output_fcn, 1, buf, sizeof(buf) - 1, "%v",
                     SC_CAPABILITIES_FLAGS, Sc_main_cb->sc_capabilities );
            completely_done = TRUE;
        }
	else if (STscompare("showserverlisten", 0, command, 0) == 0)
	{
	    TRformat(output_fcn, 1, buf, sizeof(buf) - 1, "%s", 
		     Sc_main_cb->sc_listen_mask & SC_LSN_REGULAR_OK ?
		     "OPEN" : "CLOSED" );
	    completely_done = TRUE;
	}
	else if (STscompare("showservershutdown", 0, command, 0) == 0)
	{
	    TRformat(output_fcn, 1, buf, sizeof(buf) - 1, "%s", 
		     Sc_main_cb->sc_listen_mask & SC_LSN_TERM_IDLE ?
		     "PENDING" : "OPEN" );
	    completely_done = TRUE;
	}
# ifdef NOT_SUPPORTED
	else if (STscompare("showconnections", 0, command, 0) == 0 )
	{
	    scs_scan_scbs( scs_show_func, (PTR)&powerful );
	    completely_done = TRUE;
	}
	else if (STscompare("show connection", 15, command, 15) == 0 )
	{
	    completely_done = TRUE;
	    STzapblank(command, command);
	    if (CVaxptr(command + 14, &ptr_scb) ||
		!scs_is_user_scb( (an_scb = (SCD_SCB *)ptr_scb) ) )
	    {
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Invalid connection id");
		break;
	    }
	    scs_show_func( an_scb, &powerful );
	}
# endif
	else if (STscompare("remove", 6, command, 6) == 0)
	{
	    completely_done = TRUE;
	    STzapblank(command, command);
	    if (CVaxptr(command + 6, &ptr_scb) || scb == NULL ||
		!scs_is_user_scb( (an_scb = (SCD_SCB *)ptr_scb) ) ||
		an_scb == my_scb )
	    {
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Invalid session id");
		break;
	    }
	    if ((MEcmp(an_scb->cs_scb.cs_username, scb->cs_username,
		       sizeof(scb->cs_username))) && !powerful)
	    {
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Superuser or owner status required to remove session");
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		    scs_mon_audit((SCD_SCB*)scb, 
			    SXF_A_CONTROL|SXF_A_FAIL,
			    I_SX274B_REMOVE_SESSION
			    );
		break;
	    }
	    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		scs_mon_audit((SCD_SCB*)scb, 
			SXF_A_CONTROL|SXF_A_SUCCESS,
			I_SX274B_REMOVE_SESSION
			);
	    scs_remove_sess( an_scb );
	    TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
		     "Session %p removed", an_scb);
            log_cmd = 1;
	}
	else if (STscompare("crash session", 13, command, 13) == 0)
	{
	    completely_done = TRUE;
	    STzapblank(command, command);
	    if (CVaxptr(command + 12, &ptr_scb) || scb == NULL ||
		!scs_is_user_scb( (an_scb = (SCD_SCB *)ptr_scb) ) ||
		an_scb == my_scb )
	    {
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Invalid session id");
		break;
	    }
	    if ((MEcmp(an_scb->cs_scb.cs_username, scb->cs_username,
		       sizeof(scb->cs_username))) && !powerful)
	    {
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Superuser or owner status required to crash session");
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		    scs_mon_audit((SCD_SCB*)scb, 
			SXF_A_CONTROL|SXF_A_FAIL,
			I_SX274C_CRASH_SESSION
			);
		break;
	    }
	    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		scs_mon_audit((SCD_SCB*)scb, 
			SXF_A_CONTROL|SXF_A_SUCCESS,
			I_SX274C_CRASH_SESSION
			);
	    scs_crash_sess( an_scb );
	    TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
		     "Session %p crashed", an_scb);
            log_cmd = 1;
	}
	else if (STscompare("kill", 4, command, 4) == 0)
	{
	    completely_done = TRUE;
	    STzapblank(command, command);
	    if (CVaxptr(command + 4, &ptr_scb) || scb == NULL ||
		!scs_is_user_scb( (an_scb = (SCD_SCB *)ptr_scb) ) ||
		an_scb == my_scb )
	    {
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Invalid session id");
	    }
	    else if ((MEcmp(an_scb->cs_scb.cs_username, scb->cs_username,
		       sizeof(scb->cs_username))) && !powerful)
	    {
		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Superuser or owner status required to kill query");
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		    scs_mon_audit((SCD_SCB*)scb, 
			SXF_A_CONTROL|SXF_A_FAIL,
			I_SX2755_KILL_QUERY
			);
	    }
	    else switch ( an_scb->scb_sscb.sscb_qmode )
	    {
		case 0:
		    TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			    "Session %p is not executing a query", an_scb);
		    break;

		/* Honor only "meaningful" types of queries: */
		case PSQ_RETRIEVE:
		case PSQ_RETINTO:
		case PSQ_APPEND:
		case PSQ_REPLACE:
		case PSQ_DELETE:
		case PSQ_COPY:
		case PSQ_MODIFY:
		case PSQ_EXECQRY:
		case PSQ_EXCURS:
		case PSQ_CALLPROC:
		case PSQ_REPCURS:
		case PSQ_RETCURS:
		case PSQ_EXEDBP:
		case PSQ_REGPROC:
		case PSQ_DDEXECPROC:
		case PSQ_CREATE:
		    if ( an_scb->scb_sscb.sscb_force_abort ||
			 an_scb->scb_sscb.sscb_interrupt )
		    {
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			     "Session %p query already aborting", an_scb);
		    }
		    else
		    {
			scs_kill_query( an_scb );
			if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
			    scs_mon_audit((SCD_SCB*)scb, 
				    SXF_A_CONTROL|SXF_A_SUCCESS,
				    I_SX2755_KILL_QUERY
				    );
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
				 "Session %p query killed", an_scb);
		    }
		    break;

		default:
		    TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			     "Session %p query cannot be killed", an_scb);
		    break;
	    }
	}
# ifdef NOT_SUPPORTED
	else if (STscompare("help", 0, command, 0) == 0)
	{
	    TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
		     "SCS monitor commands:\nset server shut\nset server closed\nset server open\nshow server listen\nshow server shutdown\nremove SESSION\ncrash session SESSION\n\nCS monitor commands:\n");
	}
# endif
        else if (! CS_is_mt())
	    /* OS Thread version will start an OS thread in the CS */
            if ((STscompare("start sampling", 14, command, 14) == 0))
            {
                if (!powerful)
                {
                    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
                        "Superuser status required to start sampling.", 0L);
                }
                else
                {
                    GCA_LS_PARMS        local_crb;
	            CL_ERR_DESC         errdesc;

                    local_crb.gca_status = 0;
                    local_crb.gca_assoc_id = 0;
                    local_crb.gca_size_advise = 0;
                    local_crb.gca_user_name = "<Sampler Thread>";
                    local_crb.gca_account_name = 0;
                    local_crb.gca_access_point_identifier = "NONE";
                    local_crb.gca_application_id = 0;

		    /* set up all the CS control blocks for the sampler */
	            ret_stat = CSmonitor( mode, scb, nmode, command,
				          powerful, output_fcn );

		    if ( ret_stat == OK )
                        ret_stat = CSadd_thread(CS_LIM_PRIORITY-1, (PTR) &local_crb,
                                                SCS_SSAMPLER, (CS_SID*)NULL, &errdesc);

                    if (ret_stat)
                    {
                        TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
                               "Sampling failed to start.");
                    }
                }
            }

        /* log the command before calling CSmonitor(), as the server could be left in an
        ** unknown state if the command CRASH SERVER or STOP SERVER are used
        */

        if (log_cmd)
        {
           ule_format( I_SC051E_IIMONITOR_CMD, 0, ULE_LOG, NULL, 0, 0, 0, &local_error,
                       3, STlength(command), command,
                       DB_OWN_MAXNAME, &((SCD_SCB *)scb)->scb_sscb.sscb_ics.ics_rusername,
                       sizeof(DB_TERM_NAME), &((SCD_SCB *)scb)->scb_sscb.sscb_ics.ics_terminal, 
                       0, 0);
        }

	if( !completely_done )
	    ret_stat = CSmonitor( mode, scb, nmode, command,
				 powerful, output_fcn );
	break;
	
    case CS_OUTPUT:
	break;
    }

    return( ret_stat );
}



/*{
** Name:	scs_remove_sess	-- remove connection on a session.
**
** Description:
**	Marks the session for the sequencer to remove it's GCA
**	connection, and raises an interrupt for it.  This should
**	break it out of what it's doing.
**
** Re-entrancy:
**	Questionable -- there's no lock on the flag word we're
**	modifying.  But nothing else in SCF seems to lock these
**	either.
**
** Inputs:
**	scb		the session to remove, assumed to be valid.
**
** Outputs:
**	scb->scb_sscb->sscb_flags	marked with SC_DROP_PENDING.
**
** Returns:
**	none.
**
** History:
**	16-Sep-1993 (daveb)
**	    documented.
**	25-oct-93 (robf)
**          made external
*/

void
scs_remove_sess( SCD_SCB *scb )
{
    /* don't allow kill of special sessions */

    if( scb->cs_scb.cs_client_type == SCD_SCB_TYPE &&
       scb->scb_sscb.sscb_stype == SCS_SNORMAL ||
       scb->scb_sscb.sscb_stype == SCS_SMONITOR )
    {
	/* FIXME:  may need to lock scb for MCT */
	scb->scb_sscb.sscb_flags |= SCS_DROP_PENDING;
	CSattn( CS_ATTN_EVENT, scb->cs_scb.cs_self );
    }
}    


/*{
** Name:	scs_crash_sess	-- kill a session.
**
** Description:
**	Kill the thread, dead.  May have bad side effects if
**	the thread is holding semaphores or locks when this
**	happens.
**
** Re-entrancy:
**
** Inputs:
**	scb		the session to crash, assumed to be valid.
**
** Outputs:
**	none
**
** Returns:
**	none.
**
** Side effects:
**	The thread is CSremoved, and a CSattn event sent to
**	encourage it to take effect.
**
** History:
**	16-Sep-1993 (daveb)
**	    created.
**	1-Nov-1993 (daveb)
**	    updated.
*/


static void
scs_crash_sess( SCD_SCB *scb )
{
    /* don't allow kill of special sessions */

    if( scb->cs_scb.cs_client_type == SCD_SCB_TYPE &&
       scb->scb_sscb.sscb_stype == SCS_SNORMAL ||
        scb->scb_sscb.sscb_stype == SCS_SMONITOR )
    {
	/* FIXME:  this should be logged */

	CSremove(scb->cs_scb.cs_self);
	CSattn(CS_REMOVE_EVENT, scb->cs_scb.cs_self);
    }
}    
/*{
** Name:	scs_kill_query	-- interrupt a query.
**
** Description:
**	Signals a CS_KILL_EVENT to abort
**	a running query.
**
** Re-entrancy:
**
** Inputs:
**	scb		the session to interrupt, assumed to be valid.
**
** Outputs:
**	none
**
** Returns:
**	none.
**
** Side effects:
**	The query is interrupted and rolled back, the
**	session remains.
**
** History:
**	24-Apr-2003 (jenjo02)
**	    Written for SIR 110141.
**	10-May-2004 (jenjo02)
**	    Modified to signal CS_KILL_EVENT instead of CS_RCVR_EVENT.
*/


static void
scs_kill_query( SCD_SCB *scb )
{
    /* don't allow kill of special sessions */

    if ( scb->cs_scb.cs_client_type == SCD_SCB_TYPE &&
         scb->scb_sscb.sscb_stype == SCS_SNORMAL &&
	 scb->scb_sscb.sscb_qmode )
    {
	/* Signal KILL */
	CSattn(CS_KILL_EVENT, scb->cs_scb.cs_self);
    }
}    


/*{
** Name:	scs_is_user_scb -- is the scb a user session?
**
** Description:
**	Check various flag to see if this looks like a user scb,
**	then make sure it's one that is currently active by scanning
**	the list of known sessions.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	scb		the scb of interest.
**
** Outputs:
**	none.
**
** Returns:
**	TRUE		if this is known user session SCB.
**	FALSE		if it isn't.
**
** History:
**	16-Sep-1993 (daveb)
**	    documented.
*/

static bool
scs_is_user_scb( SCD_SCB *scb )
{
    SCS_VALIDATE_ARGS	validate;

    validate.scb = scb;
    validate.result = FALSE;

    /* first do a sanity check, then scan */

    if( scb != NULL && scb->cs_scb.cs_client_type == SCD_SCB_TYPE &&
       (scb->scb_sscb.sscb_stype == SCS_SNORMAL ||
	scb->scb_sscb.sscb_stype == SCS_SMONITOR ))
    {
	scs_scan_scbs( scs_validate_scb, (PTR)&validate );
    }
    return( validate.result );
}



/*{
** Name:	scs_validate_scb	-- see if scb is known.
**
** Description:
**	This is driven out of a scan of known scbs, and checks to
**	see if the one in the argument structure is known.  When
**	it is found, if ever, the function returns FAIL, and the
**	scan terminates.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	scb		the scb to consider this time.
**	arg		PTR to an SCS_VALIDATE_ARGS, containing
**	  ->scb		the scb we're trying to validate
**	  ->result	output state, true if found.
**
** Outputs:
**	arg
**	  ->result	set TRUE if this arg->scb is the scb.
**
** Returns:
**	OK	if the scb is not the one we're looking for.
**	FAIL	if it is and we should terminate the scan.
**
** History:
**	16-Sep-1993 (daveb)
**	    documented.
*/

static STATUS
scs_validate_scb( SCD_SCB *scb, PTR arg )
{
    SCS_VALIDATE_ARGS *validate = (SCS_VALIDATE_ARGS *)arg;

    validate->result = (scb == validate->scb);

    return( validate->result ? FAIL : OK );
}



# ifdef NOT_SUPPORTED

/*{
** Name:	scs_show_func	-- show function called back from scan.
**
** Description:
**	Called back by scs_scan_connection(), this implements
**	per-session formatting of the control block for an
**	iimonitor command routine.
**
**	Only shows SQL and MONITOR sessions; hides internal threads.
**
** Re-entrancy:
**	Describe whether the function is or is not re-entrant,
**	and what clients may need to do to work in a threaded world.
**
** Inputs:
**	scb		scb to consider
**	powerfullp	CS monitor-style priv user flag, 0 if not.
**
** Outputs:
**	none.
**
** Returns:
**	scs_iformat value, if any (error status).
**
** History:
**	16-Sep-1993 (daveb)
**	    documented.
*/

static STATUS
scs_show_func( SCD_SCB *scb, PTR powerfulp )
{
    STATUS stat = OK;

    if( scb->cs_scb.cs_client_type == SCD_SCB_TYPE &&
       scb->scb_sscb.sscb_stype == SCS_SNORMAL ||
        scb->scb_sscb.sscb_stype == SCS_SMONITOR )
	stat =  scs_iformat( scb, *(i4 *)powerfulp, 0, 0 );

    return( stat );
}

# endif



/*{
** Name:	scs_scan_scbs -- drive function with arg for each.
**
** Description:
**	Drive a callback function for each known SCD_SCB,
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	func	function to call back.
**	arg	arg to hand to the function.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	16-Sep-1993 (daveb)
**	    documented.
**	25-oct-93 (robf)
**          made external
**      25-Feb-98 (fanra01)
**          Modified the scs_scan_scbs to use a new index method based on scb
**          pointer since scb_index is now thread id.
*/

void
scs_scan_scbs( SCS_SCAN_FUNC *func, PTR arg )
{
    char classbuf[ 80 ];
    char instbuf[ 80 ];
    char valbuf[ 80 ];
    i4	lval;
    PTR	ptr;
    SCD_SCB *scb;
    i4	operms;
    
    instbuf[0] = EOS;
    STcopy( "exp.scf.scs.scb_ptr", classbuf );
    for( lval = sizeof(valbuf);
	OK == MOgetnext( ~0, sizeof(classbuf), sizeof(instbuf),
			classbuf, instbuf, &lval, valbuf, &operms ) &&
	STequal( "exp.scf.scs.scb_ptr", classbuf );
	lval = sizeof(valbuf) )
    {
	CVaptr( instbuf, &ptr );
	scb = (SCD_SCB *)ptr;
	if( OK != (*func)( scb, arg ) )
	    break;
    }
}

/*
** Name: scs_broadcast_mesg 
**
** Description:
**	Sends a broadcast message to a particular session
**
** 	The current implementation queues a TRACE message for the
**	session, sent out  during later communications.
**
** History:
**	15-dec-93 (robf)
**	    Created
**
*/
static STATUS
scs_broadcast_mesg(SCD_SCB *scb, PTR  mesgptr)
{
    char sendmesg[1024];
    SCF_CB	scf_cb;
    char *m=(char*)mesgptr;
    static char *broadcast_hdr = 
"********************************************************************\nBroadcast message:\n\t";

    DB_STATUS status;

   if( scb != NULL && scb->cs_scb.cs_client_type == SCD_SCB_TYPE &&
      	 (scb->scb_sscb.sscb_stype == SCS_SNORMAL ||
       	  scb->scb_sscb.sscb_stype == SCS_SMONITOR ))
    {
	if(STlength(mesgptr)>sizeof(sendmesg)-sizeof(broadcast_hdr))
		mesgptr[sizeof(sendmesg)-sizeof(broadcast_hdr)]= EOS;
	/*
	** Format broadcast
	*/
	STprintf(sendmesg,"%s%s", broadcast_hdr, m);
	/*
	** Send message to scc.
	*/
	scf_cb.scf_len_union.scf_blength=STlength(sendmesg);
	scf_cb.scf_ptr_union.scf_buffer=sendmesg;
	scf_cb.scf_session = (SCF_SESSION)scb;

	status = scc_trace(&scf_cb, scb);
	/* scc_trace already logs errors, so don't duplicate here */
    }
    return OK;
}

/*
** Name: scs_mon_audit
**
** Description:
**	Security auditing for activity within a monitor session
**
** Inputs:
**	scb 	- current scb
**
**	access 	- type of access
**
**	mesgid	- Message id
**
** Returns:
**	none
**
** History:
**	4-mar-94 (robf)
**	     Created
*/
static  VOID
scs_mon_audit(
	SCD_SCB* scb, 
	SXF_ACCESS access,
	i4	   mesgid)
{
	DB_STATUS status;
	DB_ERROR   error;

	/*
	** Note that s0a_write_audit() already does error logging so
	** don't repeat here
	*/
	status = sc0a_write_audit(
		scb,
		SXF_E_SECURITY,	/* Type */
		access,         /* Action */
		scb->scb_sscb.sscb_ics.ics_terminal.db_term_name,
		sizeof( scb->scb_sscb.sscb_ics.ics_terminal.db_term_name),
		NULL,			/* Object Owner */
		mesgid,	/* Mesg ID */
		TRUE,			/* Force record */
		0,
		&error			/* Error location */
	);	
}

/*
** Name: scs_sampler
**
** Description:
**	Start sampler thread to gather statistics for a monitor session
**
** Inputs:
**	scb 	- current scb
**
** Returns:
**	none
**
** History:
**	10-dec-1996 (canor01)
**	     Created
*/
STATUS
scs_sampler( SCD_SCB* scb ) 
{
    CS_sampler();

    /*
    ** If we return from CS_sampler, it means we want
    ** to remove the thread, so return non-OK to let
    ** sequencer know our intention.
    */
    return (FAIL);
}
