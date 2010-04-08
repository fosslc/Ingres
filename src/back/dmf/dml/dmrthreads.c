/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <ex.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <st.h>
#include    <tr.h>
#include    <pc.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm2r.h>
#include    <dmrthreads.h>
#include    <dmd.h>
#include    <scf.h>

/**
** Name: DMRTHREADS.C - Functions to support DMR threading
**
** Description:
**      This file contains the functions to support the
**	threading of DMR record requests.
**
**      This file defines:
**
**	dmrThrOp	- called from dmr_? to execute a row
**			  operation in parallel using Agents,
**			  or serially without Agents.
**	dmrThrSync	- function to sync up all Agents and
**			  return the resulting dmr_? operation
**			  status and error code.
**	dmrThrError	- signals all Agents involved in the
**			  operation that an error has occurred.
**	dmrThrClose	- called from dm2t_close to close all
**			  the RCBs involved in the operation,
**			  disconnect the Agents from the
**			  log/lock context, and put the
**			  the Agents on the TCB's idle agent
**			  queue.
**	dmrThrTerminate	- called from dm2t_release_tcb to
**			  terminate the agents and free the
**			  associated RTB.
**	dmrAgent	- The main code thread of the Agent(s),
**			  invoked when an Agent's factotum
**			  thread is started.
**
**	Static functions:
**
**	dmrThrAttach	- allocates an RTB for Agent synchronization,
**			  spawns Agent threads.
**	dmrAgEx		- EX handler for Agent threads.
**
** History:
**	11-Nov-2003 (jenjo02)
**	    Created for Partitioned Table Project
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2r_? functions converted to DB_ERROR *
**	    rtb_err_code replaced with rtb_dberr.
**	25-Aug-2009 (kschendel) 121804
**	    Need dm0m.h to satisfy gcc 4.3.
*/
static STATUS	 dmrAgEx(
				EX_ARGS		*ex_args);

static DB_STATUS dmrThrAttach(
				DMP_RCB		*r,
				DMP_TCB		*it,
				i4		SpawnAgent,
				DB_ERROR	*dberr);

#define		TID_NOT_ALLOCATED	((u_i4) 0xffffffff)

#define		DEBUG	0

/**
** Name: dmrThrOp - Perform DMR_? operation in parallel.
**
** Description:
**	
**	Called from various DMR functions to execute 
**	an operation in parallel.
**
**	Currently called only from dmr_put(), dmr_delete(),
**	dmr_replace(), specifically to execute index
**	updates in parallel with the base table update.
**
**	One Agent thread is created for each index to
**	be updated, and optionally one for the base
**	table update.
**
** Inputs:
**	dmr_op		The DMR_? operation to perform.
**	dmr		Pointer to the DMR_CB with:
**	    dmr_flags_mask 	DMR_AGENT if the base
**				operation is to be performed
**				in parallel.
**
**				DMR_NO_SIAGENTS if 
**				secondary index updates
**				are -not- to be done in
**				parallel. The default
**				is to do index updates
**				in parallel even if the
**				base table update is not
**				(DMR_AGENT is off).
** Outputs:
**	dmr_flags_mask	If we fail to spawn the
**			necessary Agents, DMR_AGENT
**			is turned off and
**			DMR_NO_SIAGENTS is turned
**			on to prevent further
**			attempts on this DMR_CB,
**			and the operation will
**			be performed serially
**			by the caller.
**	
**
** History:
**	11-Nov-2003 (jenjo02)
**	    Created for Parallel/Partitioning Project
**	26-Jan-2004 (jenjo02)
**	    Index TCBs live on Master's TCB, not Partition's
**	03-Feb-2004 (jenjo02)
**	    Clean residual rtb_status before starting new 
**	    operation on RTB.
*/
DB_STATUS
dmrThrOp(
i4		dmr_op,
DMR_CB		*dmr)
{
    DMP_RCB	*r = (DMP_RCB*)dmr->dmr_access_id;
    DMP_TCB	*t = r->rcb_tcb_ptr, *it;
    DMR_RTB	*rtb;
    DB_STATUS	status;

    CLRDBERR(&dmr->error);

    /* If neither dmrAgent nor siAgents wanted, do nothing */
    if ( (dmr->dmr_flags_mask & (DMR_AGENT | DMR_NO_SIAGENTS)) ==
			DMR_NO_SIAGENTS )
    {
	return(E_DB_OK);
    }

    status = E_DB_OK;

    /* If wanted, spawn a dmrAgent thread for this Base operation */
    /* ...may be Base or Index RCB (if GET, POSITION) */
    if ( (rtb = (DMR_RTB*)r->rcb_rtb_ptr) == (DMR_RTB*)NULL &&
		dmr->dmr_flags_mask & DMR_AGENT )
    {
	/*
	** Allocate RTB, spawn dmrAgent for this RCB.
	**
	** If we can't get a thread started, 
	** dmrThrAttach will turn rcb_dmrThread off and
	** we'll simply return to the DMR_? function 
	** which called us to have the operation 
	** executed synchronously by its own thread.
	*/
	if ( dmrThrAttach(r, (DMP_TCB*)NULL, TRUE, &dmr->error) )
	{
	    /* Suggest the caller not try again */
	    dmr->dmr_flags_mask &= ~DMR_AGENT;
	    dmr->dmr_flags_mask |= DMR_NO_SIAGENTS;
	    return(E_DB_OK);
	}
    }

    if ( rtb )
    {
	if ( rtb->rtb_async_fcn )
	{
	    CSp_semaphore(1, &rtb->rtb_cond_sem);
	    /* Must wait for all Active agents to catch up */
	    while ( rtb->rtb_active )
	    { 
		rtb->rtb_cond_waiters++;
		CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
		rtb->rtb_cond_waiters--;
	    }
	    CSv_semaphore(&rtb->rtb_cond_sem);
	}

	/* All dmrAgents quiesced, reconstruct RTB */
	rtb->rtb_state |= RTB_BUSY;
	/* Clear residual status from previous operation */
	rtb->rtb_status = E_DB_OK;
	CLRDBERR(&rtb->rtb_error);
    }


    /*
    ** If siAgents are wanted and this is an operation involving
    ** updates to indexes, spawn an siAgent thread for each
    ** index.
    */
    if ( !(dmr->dmr_flags_mask & DMR_NO_SIAGENTS) && t->tcb_update_idx )
	switch ( dmr_op )
    {
	case DMR_PUT:
	case DMR_DELETE:
	case DMR_REPLACE:
	    /*
	    ** Allocate an RTB on the Base RCB if we don't already
	    ** have one.
	    **
	    ** This RTB will be shared by all Agents and used also
	    ** by dm2r to synchronize the base table update with
	    ** the siAgents (and dmrAgent, if there is one).
	    */
	    if ( !rtb && dmrThrAttach(r, (DMP_TCB*)NULL, FALSE, &dmr->error) )
		break;

	    rtb = (DMR_RTB*)r->rcb_rtb_ptr;

	    /* Prepare the Base RCB for this update */
	    r->rcb_siAgents = TRUE;
	    r->rcb_si_flags = RCB_SI_BUSY;
	    r->rcb_si_oldtid.tid_i4 = TID_NOT_ALLOCATED;
	    r->rcb_si_newtid.tid_i4 = TID_NOT_ALLOCATED;
	    r->rcb_compare = 0;
	    rtb->rtb_si_dupchecks = 0;

	    /*
	    ** If all index agents haven't yet been spawned,
	    ** spawn them now.
	    ** 
	    ** If they've been spawned but are idle, we
	    ** wake them up.
	    */

	    /* 
	    ** Until we figure out how to deal with local 
	    ** indexes, we'll assume that all indexes
	    ** are linked from the Partition Master.
	    **
	    ** Our "tcb_index_count" is a copy of 
	    ** Master's tcb_index_count.
	    */

	    for ( it = t->tcb_pmast_tcb->tcb_iq_next;
		  it != (DMP_TCB*)&t->tcb_pmast_tcb->tcb_iq_next &&
		      (rtb->rtb_si_agents != t->tcb_index_count ||
		       rtb->rtb_state & RTB_IDLE);
		  it = it->tcb_q_next )
	    {
		/*
		** Connect to Base RTB, spawn (or wake up ) a SI thread.
		*/
		if ( status = dmrThrAttach(r, it, TRUE, &dmr->error) )
		    break;
	    }

	    if ( status )
	    {
		TRdisplay("%@ t@%d rtb %x dmrThrOp IT op %d seq %d agents %d s %d e %d %t\n",
			CS_get_thread_id(), rtb,
			dmr_op, rtb->rtb_dmr_seq,
			rtb->rtb_agents,
			status, dmr->error.err_code,
			it->tcb_relid_len,
			it->tcb_rel.relid.db_tab_name);

		/* Tell all running agents to terminate */
		if ( rtb->rtb_agents )
		{
		    CSp_semaphore(1, &rtb->rtb_cond_sem);

		    if ( rtb->rtb_status == E_DB_OK )
		    {
			rtb->rtb_status = status;
			rtb->rtb_error = dmr->error;
		    }

		    rtb->rtb_state |= RTB_TERMINATE;
		    r->rcb_si_flags &= ~RCB_SI_BUSY;

		    if ( rtb->rtb_cond_waiters )
			CScnd_broadcast(&rtb->rtb_cond);

		    /* Wait for all to end */
		    while ( rtb->rtb_agents )
		    {
			rtb->rtb_cond_waiters++;
			CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
			rtb->rtb_cond_waiters--;
		    }
		    CSv_semaphore(&rtb->rtb_cond_sem);
		}

		/*
		** Clean up and free the RTB and cancel
		** any furthur threading on this DMR_CB.
		**
		** We'll return to DMR? with a good status
		** to execute the DMR request non-threaded.
		*/
		CScnd_free(&rtb->rtb_cond);
		CSr_semaphore(&rtb->rtb_cond_sem);
		dm0m_deallocate((DM_OBJECT**)&r->rcb_rtb_ptr);
		rtb = (DMR_RTB*)NULL;

		dmr->dmr_flags_mask &= ~DMR_AGENT;
		dmr->dmr_flags_mask |= DMR_NO_SIAGENTS;
		status = E_DB_OK;
		CLRDBERR(&dmr->error);
	    }
	    break;

	default:
	    break;
    }


    /* Now wake up the dmrAgent(s) to do the operation */

    if ( status == E_DB_OK && rtb )
    {
	CSp_semaphore(1, &rtb->rtb_cond_sem);

	rtb->rtb_state &= ~RTB_IDLE;

	/* Wait for the dmrAgents(s) to become active */
	/* They should all be waiting on RTB_BUSY now */
	while ( rtb->rtb_active )
	{
	    rtb->rtb_cond_waiters++;
	    CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
	    rtb->rtb_cond_waiters--;
	}

	/* If the thread(s) failed to activate give up */
	if ( (status = rtb->rtb_status) == E_DB_OK )
	{
	    /* Signal a new operation to the Agent(s) */
	    if ( ++rtb->rtb_dmr_seq == 0 )
		 ++rtb->rtb_dmr_seq;

#if DEBUG
	    TRdisplay("%@ t@%d rtb %x dmrThrOp sig wait %d op %d seq %d agents %d state %x %t\n",
			CS_get_thread_id(), rtb,
			rtb->rtb_cond_waiters,
			dmr_op, rtb->rtb_dmr_seq,
			rtb->rtb_agents,
			rtb->rtb_state,
			t->tcb_relid_len,
			t->tcb_rel.relid.db_tab_name);
#endif

	    rtb->rtb_dmr_op = dmr_op;
	    rtb->rtb_dmr = dmr;
	    rtb->rtb_dm2r_flags = 0;

	    /* Don't support async functions, yet */
	    rtb->rtb_async_fcn = NULL;

	    rtb->rtb_state &= ~RTB_BUSY;

	    /* Activate all agents */
	    rtb->rtb_active = rtb->rtb_agents;

	    /* Wake up dmrAgent(s) */
	    if ( rtb->rtb_cond_waiters )
		CScnd_broadcast(&rtb->rtb_cond);
	}

	CSv_semaphore(&rtb->rtb_cond_sem);

	/* Wait for synchronous completion */
	if ( r->rcb_dmrAgent && !rtb->rtb_async_fcn )
	    status = dmrThrSync(dmr, r);
    }

    return(status);
}


/**
** Name: dmrThrAttach - Allocate an RTB and/or spawn an Agent 
**			thread.
**
** Description:
**	Called when a new Agent thread is needed.
**	
**	If an RTB (RCB Threading Block) has not yet been 
**	allocated for the Base RCB, either picks an
**	idle one from the TCB's idle list or allocates
**	a new one.
**
**	With an RTB in hand, a Factotum thread is created
**	and begins execution in dmrAgent, where it waits
** 	until signalled that all preparation is complete
**	and its task may proceed.
**
**	If spawning a Secondary Index update Agent, a
**	RCB is allocated for the Agent's use and linked
**	to the RTB.
**
** Inputs:
**	r		RCB of Table on which DMR operation
**			is being performed.
**	it		If spawning a SI update Agent,
**			this will point to the Index TCB.
**	SpawnAgent	Boolean indicating if an Agent is
**			to be spawned; if OFF, only an
**			RTB is acquired.
**
** Outputs:
**	r		rcb_rtb_ptr points to the acquired
**			RTB.
**
** Side Effects:
**			A new thread is added to the Server.
**			If an error occurs, threading is
**			reset in the RCB.
**
** History:
**	11-Nov-2003 (jenjo02)
**	    Created for Partitioned Table Project
*/
static DB_STATUS
dmrThrAttach(
DMP_RCB		*r,
DMP_TCB		*it,
i4		SpawnAgent,
DB_ERROR	*dberr)
{
    /* Called from dmrThrOp on first use of RCB */
    DMP_TCB	*t = r->rcb_tcb_ptr;
    DMP_RCB	*ir = (DMP_RCB*)NULL;
    DB_STATUS	status = E_DB_OK;
    STATUS	scf_status;
    SCF_CB	scf_cb;
    SCF_FTC	ftc;
    DMR_RTB	*rtb;
    DMR_RTL	*rtl;
    i4		was_allocated = FALSE;
    char	sem_name[CS_SEM_NAME_LEN+16];
    char	thread_name[DB_MAXNAME*2];
    DB_ERROR	local_dberr;

    /* Find or allocate an RTB for the Base RCB */
    if ( (rtb = (DMR_RTB*)r->rcb_rtb_ptr) == (DMR_RTB*)NULL )
    {
	/* Look first for an idle RTB on the TCB */
	CSp_semaphore(1, &t->tcb_mutex);
	if ( rtb = (DMR_RTB*)t->tcb_rtb_next )
	    t->tcb_rtb_next = rtb->rtb_q_next;
	CSv_semaphore(&t->tcb_mutex);

#if DEBUG
	if ( rtb )
	    TRdisplay("%@ t@%d rtb %x dmrThrAttach found on TCB agents %d state %x %t\n",
		    CS_get_thread_id(), rtb,
		    rtb->rtb_agents,
		    rtb->rtb_state,
		    t->tcb_relid_len,
		    t->tcb_rel.relid.db_tab_name);
#endif

	/* If none, allocate one */
	if ( !rtb && (status = dm0m_allocate(sizeof(DMR_RTB), (i4)0,
			    (i4)RTB_CB, (i4)RTB_ASCII_ID,
			    (char*)t, (DM_OBJECT**)&rtb, 
			    dberr)) == E_DB_OK )
	{
	    was_allocated = TRUE;
	    
	    /* Intialize the RTB */
	    rtb->rtb_rcb = r;
	    rtb->rtb_rtl = (DMR_RTL*)NULL;
	    rtb->rtb_state = 0;
	    rtb->rtb_si_agents = 0;
	    rtb->rtb_agents = 0;
	    rtb->rtb_cond_waiters = 0;
	    rtb->rtb_active = 0;

	    /* Init the condition variable and semaphore */
	    CScnd_init(&rtb->rtb_cond);
	    CSw_semaphore(&rtb->rtb_cond_sem, CS_SEM_SINGLE,
				STprintf(sem_name,
				  "RTB %*s",
				  t->tcb_relid_len,
				  t->tcb_rel.relid.db_tab_name));
	}
	    
	if ( rtb )
	{
	    /* Init these parts for new or idle RTBs */
	    r->rcb_rtb_ptr = (PTR)rtb;
	    rtb->rtb_dmr = (DMR_CB*)NULL;
	    rtb->rtb_dmr_op = 0;
	    rtb->rtb_dm2r_flags = 0;
	    rtb->rtb_dmr_seq = 0;
	    rtb->rtb_async_fcn = NULL;
	    rtb->rtb_si_dupchecks = 0;
	    rtb->rtb_status = E_DB_OK;
	    CLRDBERR(&rtb->rtb_error);
	}
    }

    if ( rtb )
    {
	/* Then we're attaching to Base RCB's RTB */
	/* Allocate an RCB for the Index */
	if ( it && (status = dm2r_rcb_allocate(it, r, &r->rcb_tran_id,
						r->rcb_lk_id,
						r->rcb_log_id,
						&ir,
						dberr)) == E_DB_OK )
	{
	    /* Connect the SI RCB to RTB */
	    ir->rcb_rtb_ptr = (PTR)rtb;
	    ir->rcb_si_flags = RCB_SI_UPDATE;

	    /* Point "t" to index TCB */
	    t = it;
	}

	if ( status == E_DB_OK )
	{
	    CSp_semaphore(1, &rtb->rtb_cond_sem);

	    /* Mark RTB "under construction" */
	    rtb->rtb_state |= RTB_BUSY;

	    /* Want an Agent, or just an RTB? */
	    if ( SpawnAgent )
	    {
		/* Look first for an idle agent on RTB */
		for ( rtl = rtb->rtb_rtl; 
		      rtl && rtl->rtl_tcb != t; 
		      rtl = rtl->rtl_next );

		if ( rtl )
		{
#if DEBUG
		    TRdisplay("%@ t@%d rtb %x dmrThrAttach found RTL %d active %d state %x %t\n",
			    CS_get_thread_id(), rtb, rtl->rtl_thread_id,
			    rtb->rtb_active,
			    rtb->rtb_state,
			    t->tcb_relid_len,
			    t->tcb_rel.relid.db_tab_name);
#endif

		    /* Count an "active" agent */
		    rtb->rtb_active++;

		    /* Point agent at it's RCB, wake it up */
		    if ( ir )
		    {
			ir->rcb_dmrAgent = TRUE;
			rtl->rtl_rcb = ir;
		    }
		    else
		    {
			r->rcb_dmrAgent = TRUE;
			rtl->rtl_rcb = r;
		    }

		    /* Signal only this Agent */
		    CScnd_signal(&rtb->rtb_cond, rtl->rtl_sid);
		    CSv_semaphore(&rtb->rtb_cond_sem);
		}
		else
		{
		    /* Need to spawn a new agent */

		    /* Assume we'll succeed: */
		    rtb->rtb_active++;
		    rtb->rtb_agents++;
		    if ( ir )
		    {
			rtb->rtb_si_agents++;
			ir->rcb_dmrAgent = TRUE;
			ftc.ftc_data = (PTR)ir;
		    }
		    else
		    {
			r->rcb_dmrAgent = TRUE;
			ftc.ftc_data = (PTR)r;
		    }
		    CSv_semaphore(&rtb->rtb_cond_sem);

		    scf_cb.scf_type = SCF_CB_TYPE;
		    scf_cb.scf_length = sizeof(SCF_CB);
		    scf_cb.scf_session = DB_NOSESSION;
		    scf_cb.scf_facility = DB_DMF_ID;
		    scf_cb.scf_ptr_union.scf_ftc = &ftc;

		    ftc.ftc_thread_exit = NULL;
		    ftc.ftc_data_length = 0;
		    ftc.ftc_priority = SCF_CURPRIORITY;
		    ftc.ftc_facilities = 0;
		    STprintf(thread_name,
			      "<%sdmrAgent %*s>",
			      (ir) ? "SI" : "",
			      t->tcb_relid_len, 
			      t->tcb_rel.relid.db_tab_name);
		    ftc.ftc_thread_name = thread_name;
		    ftc.ftc_thread_entry = dmrAgent;

		    /* Create and initiate the thread */
		    if ( scf_status = scf_call(SCS_ATFACTOT, &scf_cb) )
		    {
			CSp_semaphore(1, &rtb->rtb_cond_sem);

			status = E_DB_ERROR;
			*dberr = scf_cb.scf_error;

			/* If SI attach, dump the RCB */
			if ( ir )
			{
			    rtb->rtb_si_agents--;
			    rtb->rtb_agents--;
			    rtb->rtb_active--;
			    ir->rcb_rtb_ptr = NULL;
			    dm2r_return_rcb(&ir, &local_dberr);

			    if ( rtb->rtb_status == E_DB_OK )
			    {
				rtb->rtb_status = status;
				rtb->rtb_error = *dberr;
			    }

			    /* Signal termination to other SI agents */
			    rtb->rtb_state &= ~RTB_BUSY;
			    rtb->rtb_state |= RTB_TERMINATE;
			    if ( rtb->rtb_cond_waiters )
				CScnd_signal(&rtb->rtb_cond, rtl->rtl_sid);
			    CSv_semaphore(&rtb->rtb_cond_sem);
			}
			else if ( was_allocated )
			{
			    /* Not index attach, deallocate the RTB */
			    CSv_semaphore(&rtb->rtb_cond_sem);
			    CScnd_free(&rtb->rtb_cond);
			    CSr_semaphore(&rtb->rtb_cond_sem);
			    dm0m_deallocate((DM_OBJECT**)&r->rcb_rtb_ptr);
			    rtb = (DMR_RTB*)NULL;
			}
		    }
		}
	    }
	    else
		CSv_semaphore(&rtb->rtb_cond_sem);
	}
    }

    /* If we failed, cancel threading in this RCB */
    if ( status )
    {
	r->rcb_dmrAgent = FALSE;
	r->rcb_siAgents = FALSE;
    }

    return(status);
}


/**
** Name: dmrThrSync - Wait for all agents to complete their
**		      work, return collective status.
**
** Description:
**	
**	Waits for all active Agents to finish their work,
**	then returns the collective status (rtb_status)
**	and error code (rtb_error) to the caller.
**
** Inputs:
**	r		The DMR operation's RCB.
**
** History:
**	11-Nov-2003 (jenjo02)
**	    Created for Partitioned Table Project
*/
DB_STATUS
dmrThrSync(
DMR_CB		*dmr,
DMP_RCB		*r)
{
    DMR_RTB	*rtb;
    DB_STATUS	status = E_DB_OK;

    if ( rtb = (DMR_RTB*)r->rcb_rtb_ptr )
    {
	CSp_semaphore(1, &rtb->rtb_cond_sem);

	/* Wait until operation completes */
	while ( rtb->rtb_active )
	{
	    rtb->rtb_cond_waiters++;
	    CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
	    rtb->rtb_cond_waiters--;
	}

	if ( status = rtb->rtb_status )
	    dmr->error = rtb->rtb_error;

	rtb->rtb_status = E_DB_OK;
	CLRDBERR(&rtb->rtb_error);

	CSv_semaphore(&rtb->rtb_cond_sem);
    }

    return(status);
}


/**
** Name: dmrThrError - Send a non-zero status and error code
**		       to the Agents.
**
** Description:
**	Called externally to signal the Agents that some
**	error has occurred.
**	
**	The Agents are expected to quit what they are doing
**	and return to an idle state.
**
** Inputs:
**	dmr		The DMR_CB.
**	r		The DMR operation's RCB.
**	status		The status to be sent to the Agents,
**
** Returns:		The same status as was called with.
**
** History:
**	11-Nov-2003 (jenjo02)
**	    Created for Partitioned Table Project
*/
DB_STATUS
dmrThrError(
DMR_CB		*dmr,
DMP_RCB		*r,
DB_STATUS	status)
{
    DMR_RTB	*rtb;

    if ( status && (rtb = (DMR_RTB*)r->rcb_rtb_ptr) )
    {
	CSp_semaphore(1, &rtb->rtb_cond_sem);

	if ( rtb->rtb_status == E_DB_OK )
	{
	    rtb->rtb_status = status;
	    rtb->rtb_error = dmr->error;
	}

	if ( rtb->rtb_cond_waiters )
	    CScnd_broadcast(&rtb->rtb_cond);

	CSv_semaphore(&rtb->rtb_cond_sem);
    }

    return(status);
}


/**
** Name: dmrThrClose: Signal Agents to enter idle state
**
** Description:
**	Called from dm2t_close() to signal CLOSE to the
**	Agents just before it deallocates its RCB.
**
**	The Agents respond by disconnecting from their
**	LG/LK contexts and enter an idle state. 
**	siAgents also deallocate their RCBs.
**
**	When all Agents are idle, the RTB is added to
**	the TCB list of idle RTBs.
**	
** Inputs:
**	r		The DMR operation's RCB.
**
** History:
**	11-Nov-2003 (jenjo02)
**	    Created for Partitioned Table Project
*/
VOID
dmrThrClose(
DMP_RCB		*r)
{
    DMR_RTB	*rtb;
    DMP_TCB	*t;

    if ( rtb = (DMR_RTB*)r->rcb_rtb_ptr )
    {
	CSp_semaphore(1, &rtb->rtb_cond_sem);

	t = r->rcb_tcb_ptr;

	/* Signal the agent(s) */
	rtb->rtb_state |= RTB_CLOSE;
	rtb->rtb_active = rtb->rtb_agents;

#if DEBUG
	TRdisplay("%@ t@%d rtb %x dmrThrClose agents %d state %x %t\n",
		    CS_get_thread_id(), rtb,
		    rtb->rtb_agents,
		    rtb->rtb_state,
		    t->tcb_relid_len,
		    t->tcb_rel.relid.db_tab_name);
#endif

	if ( rtb->rtb_cond_waiters )
	    CScnd_broadcast(&rtb->rtb_cond);

	/* Wait for all agents to idle */
	while ( rtb->rtb_active )
	{
	    rtb->rtb_cond_waiters++;
	    CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
	    rtb->rtb_cond_waiters--;
	}

	rtb->rtb_state &= ~RTB_CLOSE;
	rtb->rtb_state |= RTB_IDLE;
	r->rcb_rtb_ptr = (PTR)NULL;
	r->rcb_dmrAgent = FALSE;

	CSv_semaphore(&rtb->rtb_cond_sem);

	/* Link the idle RTB to the TCB idle queue */
	CSp_semaphore(1, &t->tcb_mutex);
	rtb->rtb_q_next = t->tcb_rtb_next;
	t->tcb_rtb_next = (PTR)rtb;
	CSv_semaphore(&t->tcb_mutex);
    }

    return;
}

/**
** Name: dmrThrTerminate - Called from dm2t_release_tcb to
**		           terminate the Agents.
**
** Description:
**	Called from dm2t_release_tcb() just prior to the
**	TCB being deallocated.
**
**	The Agents are sent a TERMINATE signal, to which
** 	they respond by ending their threads.
**
**	Once all Agents have terminated, the RTB is
**	deallocated.
**
** Inputs:
**	t			The TCB being released with
**	    tcb_rtb_next 	anchoring the list of idle RTBs.
**
** History:
**	11-Nov-2003 (jenjo02)
**	    Created for Partitioned Table Project
*/
VOID
dmrThrTerminate(
DMP_TCB		*t)
{
    DMR_RTB	*rtb;

    /*
    ** It's presumed that the TCB is about to be 
    ** deallocated, hence is thread-safe, so
    ** we'll do this without taking the tcb_mutex.
    */

    /* For every RTB on the TCB list... */
    while ( rtb = (DMR_RTB*)t->tcb_rtb_next )
    {
	CSp_semaphore(1, &rtb->rtb_cond_sem);

	t->tcb_rtb_next = rtb->rtb_q_next;

	/* Signal the agent(s) */
	rtb->rtb_state |= RTB_TERMINATE;

#if DEBUG
	TRdisplay("%@ t@%d rtb %x dmrThrTerminate agents %d state %x %t\n",
		CS_get_thread_id(), rtb,
		rtb->rtb_agents,
		rtb->rtb_state,
		t->tcb_relid_len,
		t->tcb_rel.relid.db_tab_name);
#endif

	if ( rtb->rtb_cond_waiters )
	    CScnd_broadcast(&rtb->rtb_cond);

	/* Wait for all agents to terminate */
	while ( rtb->rtb_agents )
	{
	    rtb->rtb_cond_waiters++;
	    CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
	    rtb->rtb_cond_waiters--;
	}

	/* Destroy the condition variable and semaphore */
	CScnd_free(&rtb->rtb_cond);
	CSv_semaphore(&rtb->rtb_cond_sem);
	CSr_semaphore(&rtb->rtb_cond_sem);

	/* Free the RTB memory */
	dm0m_deallocate((DM_OBJECT**)&rtb);
    }

    return;
}


/**
** Name: dmrAgent - The code that drives the Agents.
**
** Description:
**	This is the Factotum code executed by each Agent.
**
**	When the thread is started, ftx_data holds a
**	pointer to this Agent's RCB which in turn points
**	to the RTB shared by all Agents. The RCB also
**	contains a flag indicating whether this Agent's
**	scope is limited to secondary index updates.
**
**	The RTB identifies the DMR_CB operation being
**	performed.
**
**	When the Agent is first started, it links itself
**	to the RTB list of Agents (RTL), connects to
**	the RCB's log and lock contexts (OPEN), then goes
**	to sleep, waiting until signalled that there is
**	work to do.
**
**	The DMR operation to be performed is stored in
**	rtb_dmr_op by dmrThrOp().
**
**	Once the Agent has completed its work, it goes
**	back to sleep waiting until a new rtb_dmr_op
**	appears.
**
**	Other events may also wake the Agent up, such as
**	OPEN, TERMINATE or CLOSE.
**
**	An OPEN signal means that dmrThrOp has provided
**	a new RCB to the Agent (in it's RTL rtl_rcb), 
**	and the process starts anew.
**
**	A CLOSE signal is received when the DMR's RCB
**	is about to be deallocated, and the Agent 
**	responds by disconnecting from its log/lock
**	contexts, discarding its RCB,  and putting itself 
**	in an Idle state awaiting further signals.
**	(if an SIagent the SI RCB is deallocated)
**
**	A TERMINATE signal ends the thread.
**
**	Should an error occur while executing the DMR
**	operation, the error and error_code are transmitted
**	to the other Agents via the RTB; they are expected
**	to quit what they are doing, if anything, and
**	go back to sleep.
**
** Inputs:
**	ftx		SCF_FTX from SCS, prepared by
**			dmrThrOp with ftx_data pointing
**			to the RTB.
**	
**
** History:
**	11-Nov-2003 (jenjo02)
**	    Created for Partitioned Table Project
**	04-Mar-2004 (jenjo02)
**	    Don't check for CLOSE/TERMINATE after completing
**	    DMR operation, but loop back to wait for next
**	    signal. Prevents hang during, for example, 
**	    force abort.
**	08-Apr-2005 (jenjo02)
**	    Condition wait interrupt now causes TERMINATion
**	    rather than "CLOSE" to prevent hangs when real
**	    CLOSE is signalled.
*/
DB_STATUS
dmrAgent(
SCF_FTX	*ftx)
{
    DMR_RTL	rtl;
    DMP_RCB	*r;
    DMR_RTB	*rtb = (DMR_RTB*)NULL;
    DMP_TCB	*t;
    DMR_CB	*dmr;
    DB_STATUS	status;
    STATUS	lg_status;
    STATUS	lk_status;
    DB_ERROR	local_dberr;
    i4		seq;
    i4		rcb_log_id;
    i4		rcb_lk_id;
    i4		SIagent;
    i4		active;
    CL_ERR_DESC sys_err;
    EX_CONTEXT	context;

    if ( EXdeclare(dmrAgEx, &context) )
    {
	/* We're going down hard... */
	CSp_semaphore(1, &rtb->rtb_cond_sem);

	/* Force termination of this and all threads */
	rtb->rtb_state |= RTB_TERMINATE;

	if ( rtb->rtb_status == E_DB_OK )
	{
	    rtb->rtb_status = E_DB_ERROR;
	    SETDBERR(&rtb->rtb_error, 0, E_DM004A_INTERNAL_ERROR);
	}
	/*
	** If we crashed while "active", decrement the
	** count, wake up waiters
	*/
	if ( active && --rtb->rtb_active == 0 )
	{
	    if ( rtb->rtb_cond_waiters )
		CScnd_broadcast(&rtb->rtb_cond);
	}
    }
    else
    {
	ftx->ftx_status = E_DB_OK;
	CLRDBERR(&ftx->ftx_error);

	/* Get thread-specific stuff from RCB */
	r = (DMP_RCB*)ftx->ftx_data;
	t = r->rcb_tcb_ptr;
	rtb = (DMR_RTB*)r->rcb_rtb_ptr;

	/* Are we an SI agent? */
	SIagent = r->rcb_si_flags & RCB_SI_UPDATE;

	CSp_semaphore(1, &rtb->rtb_cond_sem);

	/* Link ourselves to the RTB->agent list */
	if ( rtl.rtl_next = rtb->rtb_rtl )
	    rtb->rtb_rtl->rtl_prev = &rtl;
	rtl.rtl_prev = (DMR_RTL*)NULL;
	rtb->rtb_rtl = &rtl;
	/* Tell all the RCB/TCB we're working on */
	rtl.rtl_tcb = t;
	rtl.rtl_rcb = r;
	/* Identify ourselves for CScond_signals */
	CSget_sid(&rtl.rtl_sid);
	/* This is just for debugging... */
	rtl.rtl_thread_id = CS_get_thread_id();

	/* This will induce an "open" on the RCB */
	r = (DMP_RCB*)NULL;
    }

    /* The big loop... */
    for (;;)
    {
	/*
	** Wait for the RTB to be initialized and
	** for a DMR to appear or termination, open,
	** or close to be signalled.
	*/
	while ( (!(rtb->rtb_state & RTB_TERMINATE) &&
		    ((rtb->rtb_state & RTB_CLOSE && !r) ||
			 (!rtl.rtl_rcb && !r)))
		  ||
		 (!(rtb->rtb_state & (RTB_TERMINATE | RTB_CLOSE)) &&
		     (r && (rtb->rtb_state & RTB_BUSY ||
			seq == rtb->rtb_dmr_seq))) )
	{
	    rtb->rtb_cond_waiters++;
	    if ( CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem) )
	    {
		/* Then we've been interrupted */
#if DEBUG
		TRdisplay("%@ t@%d rtb %x dmrAgent INTERRUPT op %d seq %d active %d %t\n",
			    CS_get_thread_id(), rtb,
			    rtb->rtb_dmr_op, 
			    seq,
			    rtb->rtb_active,
			    t->tcb_relid_len,
			    t->tcb_rel.relid.db_tab_name);
#endif
		rtb->rtb_state |= RTB_TERMINATE;
	    }
	    rtb->rtb_cond_waiters--;
	}


	if ( r &&
	     (rtb->rtb_state & (RTB_TERMINATE | RTB_CLOSE)) == 0 &&
	     seq != rtb->rtb_dmr_seq )
	{
	    /* Then there's something DMR-wise to do */

	    /* This is the one being worked on */
	    seq = rtb->rtb_dmr_seq;

	    /* SI Agents only do some operations... */
	    if ( SIagent &&
		rtb->rtb_dmr_op != DMR_DELETE &&
		rtb->rtb_dmr_op != DMR_PUT &&
		rtb->rtb_dmr_op != DMR_REPLACE )
	    {
		rtb->rtb_active--;
		continue;
	    }

	    dmr = (DMR_CB*)rtb->rtb_dmr;
	    dmr->dmr_flags_mask |= DMR_AGENT_CALLBACK;

	    /* In case we crash... */
	    active = TRUE;

#if DEBUG
	    TRdisplay("%@ t@%d rtb %x dmrAgent op %d seq %d active %d %t\n",
			    CS_get_thread_id(), rtb,
			    rtb->rtb_dmr_op, 
			    seq,
			    rtb->rtb_active,
			    t->tcb_relid_len,
			    t->tcb_rel.relid.db_tab_name);
#endif

	    CSv_semaphore(&rtb->rtb_cond_sem);

	    if ( SIagent )
		switch ( rtb->rtb_dmr_op )
	    {
		case DMR_DELETE:
		    status = dm2r_si_delete(r, &local_dberr);
		    break;
		case DMR_PUT:
		    status = dm2r_si_put(r, 
				    dmr->dmr_data.data_address,
				    &local_dberr);
		    break;
		case DMR_REPLACE:
		    status = dm2r_si_replace(r, 
				    dmr->dmr_data.data_address,
				    &local_dberr);
		    break;
	    }
	    else switch ( rtb->rtb_dmr_op )
	    {
		case DMR_DELETE:
		    status = dmr_delete(dmr);
		    local_dberr = dmr->error;
		    break;

		case DMR_GET:
		    status = dmr_get(dmr);
		    local_dberr = dmr->error;
		    break;

		case DMR_POSITION:
		    status = dmr_position(dmr);
		    local_dberr = dmr->error;
		    break;

		case DMR_PUT:
		    status = dmr_put(dmr);
		    local_dberr = dmr->error;
		    break;

		case DMR_REPLACE:
		    status = dmr_replace(dmr);
		    local_dberr = dmr->error;
		    break;

		case DMR_LOAD:
		    status = dmr_load(dmr);
		    local_dberr = dmr->error;
		    break;
		
		case DMR_DUMP_DATA:
		    status = dmr_dump_data(dmr);
		    local_dberr = dmr->error;
		    break;

		case DMR_ALTER:
		    status = dmr_alter(dmr);
		    local_dberr = dmr->error;
		    break;

		case DMR_AGGREGATE:
		    status = dmr_aggregate(dmr);
		    local_dberr = dmr->error;
		    break;
	    }

	    /*
	    ** Done with the operation.
	    **
	    ** Set the status and error code,
	    ** wake up anyone waiting for 
	    ** completion, then 
	    ** loop back for more.
	    */
	    CSp_semaphore(1, &rtb->rtb_cond_sem);

	    dmr->dmr_flags_mask &= ~DMR_AGENT_CALLBACK;

#if DEBUG
	    TRdisplay("%@ t@%d rtb %x dmrAgent done %d seq %d act %d s %d e %d %t\n",
			    CS_get_thread_id(), rtb,
			    rtb->rtb_dmr_op, 
			    seq,
			    rtb->rtb_active,
			    status, local_dberr.err_code,
			    t->tcb_relid_len,
			    t->tcb_rel.relid.db_tab_name);
#endif

	    /* We're done with this operation */
	    rtb->rtb_active--;
	    active = FALSE;

	    if ( status && (rtb->rtb_status == E_DB_OK) )
	    {
		rtb->rtb_status = status;
		rtb->rtb_error = local_dberr;

		/* Wake up any threads that may care about 
		** bad status */
		if ( rtb->rtb_active > 0 && rtb->rtb_cond_waiters )
		    CScnd_broadcast(&rtb->rtb_cond);
	    }

	    /* We're done with this operation */
	    if ( rtb->rtb_active <= 0 )
	    {
		/* All dmrAgents are done with operation */
		dmr->error = rtb->rtb_error;

		/* Wake up waiters on completion  */
		if ( rtb->rtb_cond_waiters )
		    CScnd_broadcast(&rtb->rtb_cond);
	    }
	}
	else if ( rtb->rtb_state & (RTB_TERMINATE | RTB_CLOSE) )
	{
	    /*
	    ** Time to close or terminate this agent.
	    **
	    ** Disconnect from logging, locking contexts.
	    **
	    ** Decrement the attached agent count,
	    ** notify anyone waiting for this change in condition.
	    */
	    
	    /* On normal termination, we shouldn't have an RCB */
	    if ( r )
	    {
		/* Release the sem while we disconnect LG/LK */
		CSv_semaphore(&rtb->rtb_cond_sem);

		/* If we connected to a log context, disconnect */
		if ( r->rcb_logging && r->rcb_log_id != rcb_log_id )
		{
		    LGend(r->rcb_log_id, 0, &sys_err);
		    r->rcb_log_id = rcb_log_id;
		}
		/* If we connected to a lock context, disconnect */
		if ( r->rcb_lk_id != rcb_lk_id )
		{
		    LKrelease(LK_ALL, r->rcb_lk_id, (LK_LKID*)0,
				    (LK_LOCK_KEY*)0, (LK_VALUE*)0,
				    &sys_err);
		    r->rcb_lk_id = rcb_lk_id;
		}

		/* This RCB is no longer connected to RTB */
		r->rcb_rtb_ptr = NULL;

		/* If SI update RCB, get rid of it */
		if ( SIagent )
		    (VOID)dm2r_return_rcb(&r, &local_dberr);

		r = (DMP_RCB*)NULL;
		rtl.rtl_rcb = (DMP_RCB*)NULL;

		CSp_semaphore(1, &rtb->rtb_cond_sem);
	    }

	    if ( rtb->rtb_state & RTB_TERMINATE )
	    {
#if DEBUG
		TRdisplay("%@ t@%d rtb %x dmrAgent TERMINATE act %d state %x %t\n",
			CS_get_thread_id(), rtb,
			rtb->rtb_active,
			rtb->rtb_state,
			t->tcb_relid_len,
			t->tcb_rel.relid.db_tab_name);
#endif
		/* Then the thread is ending */

		/* Disconnect from the RTB agent queue */
		if ( rtl.rtl_prev )
		    rtl.rtl_prev->rtl_next = rtl.rtl_next;
		if ( rtl.rtl_next )
		    rtl.rtl_next->rtl_prev = rtl.rtl_prev;

		if ( SIagent )
		    rtb->rtb_si_agents--;

		if ( --rtb->rtb_agents == 0 && rtb->rtb_cond_waiters )
			CScnd_broadcast(&rtb->rtb_cond);

		CSv_semaphore(&rtb->rtb_cond_sem);
		/* Break to end the thread */
		break;
	    }
#if DEBUG
	    TRdisplay("%@ t@%d rtb %x dmrAgent CLOSE act %d state %x %t\n",
		    CS_get_thread_id(), rtb,
		    rtb->rtb_active,
		    rtb->rtb_state,
		    t->tcb_relid_len,
		    t->tcb_rel.relid.db_tab_name);
#endif
	    /* We're just going to idle */

	    if ( --rtb->rtb_active == 0 && rtb->rtb_cond_waiters )
		CScnd_broadcast(&rtb->rtb_cond);
	}
	else if ( !r && rtl.rtl_rcb )
	{
#if DEBUG
	    TRdisplay("%@ t@%d rtb %x dmrAgent OPEN act %d state %x %t\n",
			CS_get_thread_id(), rtb,
			rtb->rtb_active,
			rtb->rtb_state,
			t->tcb_relid_len,
			t->tcb_rel.relid.db_tab_name);
#endif
	    /* Then this is an "open" signal on a new RCB */
	    r = rtl.rtl_rcb;

	    /* These are the LG/LK contexts to connect to */
	    rcb_log_id = r->rcb_log_id;
	    rcb_lk_id = r->rcb_lk_id;

	    SIagent = r->rcb_si_flags & RCB_SI_UPDATE;
	    active = FALSE;
	    seq = 0;

	    if ( lg_status = r->rcb_logging )
		lg_status = LGconnect(rcb_log_id, 
				       &r->rcb_log_id,
				       &sys_err);

	    if ( (lk_status = lg_status) == OK )
		lk_status = LKconnect((LK_LLID)rcb_lk_id, 
				       (LK_LLID*)&r->rcb_lk_id,
					&sys_err);

	    if ( lg_status || lk_status )
	    {
		rtb->rtb_state |= RTB_TERMINATE;
		rtb->rtb_status = E_DB_ERROR;
		if ( lg_status == LG_EXCEED_LIMIT )
		    SETDBERR(&rtb->rtb_error, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
		else if ( lk_status == LK_NOLOCKS )
		    SETDBERR(&rtb->rtb_error, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		else
		    SETDBERR(&rtb->rtb_error, 0, E_DM9500_DMXE_BEGIN);
	    }

	    /* Announce that we're ready for work */
	    if ( --rtb->rtb_active == 0 && rtb->rtb_cond_waiters )
		CScnd_broadcast(&rtb->rtb_cond);
	}
    }

    EXdelete();

    /* Return to SCF to end the thread */
    return(E_DB_OK);
}

/*{
** Name: dmrAgEx	- dmrAgent exception handler
**
** Description:
**	Catches exceptions that occur in the dmrAgent.
**
** Inputs:
**      ex_args                         Exception arguments.
**
** Outputs:
**	Returns:
**	    EXDECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-Nov-2003 (jenjo02)
**	    Created.
*/
static STATUS
dmrAgEx(
EX_ARGS		    *ex_args)
{
    i4	    err_code;
    i4	    status;
    
    if ( ex_args->exarg_num == EX_DMF_FATAL )
	err_code = E_DM904A_FATAL_EXCEPTION;
    else
	err_code = E_DM9049_UNKNOWN_EXCEPTION;

    ulx_exception( ex_args, DB_DMF_ID, err_code, FALSE );

    return (EXDECLARE);
}
