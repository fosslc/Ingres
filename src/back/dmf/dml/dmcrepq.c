/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <st.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmftrace.h>
#include    <dm2rep.h>
#include    <dm2d.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <scf.h>
#include    <ddb.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <qefcb.h>
#include    <dudbms.h>

/*
** Name: dmcrepq.c - implimentation of the replicator queue management thread
**
** Description:
**	This file contains the routines that make up the replicator queue
**	management system thread within the DBMS, the threads initial task
**	(for phase 1) is to read the input queue (dd_input_queue) and
**	write the distribution queue.
**
**	This file contains the following external routines:
**
**	dmc_repq	- main entry point for queue management thread
**
** History:
**	22-may-96 (stephenb)
**	    Initial creation for DBMS replication phase 1
**	25-jul-96 (stephenb)
**	    Add code to send dbevent to distribute replicas, this is done
**	    for each database processed.
**	5-aug-96 (stephenb)
**	    Process queue immediately on startup for restart recovery.
**	13-jan-97 (stephenb)
**	    Fix up session ID, we can no longer assume session ID is a 
**	    pointer to an SCB, since we are now using OS threads.
**	17-Jan-1997 (jenjo02)
**	    Replaced svcb_mutex with svcb_dq_mutex (DCB queue) and
**	    svcb_sq_mutex (SCB queue).
**	30-oct-97 (stephenb)
**	    if dm2rep_qman() returns E_DB_WARN, then the processing has failed
**	    due to a non-fatal error (e.g. deadlock), deal with this case by
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Apr-2006 (kschendel)
**	    Get rid of a pile of compiler warnings from ule-format calls.
**	12-Apr-2008 (kschendel)
**	    Move adf.h sooner.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	18-Nov-2008 (jonj)
**	    dm2d_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm2rep_? functions converted to DB_ERROR *
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new LK_INTR_FA error.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	28-Apr-2010 (kschendel)
**	    Compiler warning fix to above.
*/
/*
** globals
*/
GLOBALREF	DMC_REP		*Dmc_rep; /* replicator shared memory segment */

/*
** Name: dmc_repq - replication queue management thread main entry point
**
** Description:
**	This routine impliments the replication queue management system
**	thread, the initial implimentation of the thread will read the
**	replication input queue (currently the dd_input_queue table) and
**	write the replication distribution queue (currently the
**	dd_distribution_queue table). At startup the thread will create
**	or attach to the shared memory segment which will contain a list
**	of transactions to be distributed and will loop, waiting for
**	the distribute event and then read the shared memory segment
**	to grab a transaction to distribute. The input queue records for
**	this transaction are read and distribution queue records are
**	created, then the input queue records are deleted and the
**	thread checks for mote trnasactions when there are none the
**	thread waits for the next event.
**
**	NOTES: All access to the shared memory segment is semaphore protected.
**
**	       The thread may not necesarily work on the transaction that
**	       caused the wakeup event, but all transactions in the queue
**	       will have already been committed, avoiding lock contention
**	       on the dd_input_queue table. There will be one wakeup event
**	       per committed transaction.
**
**	WARNING: We should NEVER call this routine while holding either the
**		 replicator transaction queue mutex or the dmf_svcb mutex
**	   	 sice we obtain both in here and we could end up causing
**		 mutex deadlocks (permanent CS_MUTEX states) or worse.
**
** Inputs:
**	None
**
** Outputs:
**	dmc_cb.error_code
** 
** Returns:
**	Status
**
** History:
**	22-may-96 (stephenb)
**	    Initial creation for DBMS replication phase 1
**	25-jul-96 (stephenb)
**	    Add code to send dbevent to distribute replicas, this is done
**	    for each database processed.
**	5-aug-96 (stephenb)
**	    Restart recovery: process the queue before waiting for event
**	    so that outstanding transactions are processed immediately on
**	    startup.
**	30-oct-97 (stephenb)
**	    if dm2rep_qman() returns E_DB_WARN, then the processing has failed
**	    due to a non-fatal error (e.g. deadlock), deal with this case by
**	    re-trying.
**	9-jan-98 (stephenb)
**	    the database event "dd_distribute" should be "DD_DISTRIBUTE" for
**	    a FIPS (SQL 92) compliant database.
**	12-jan-98 (stephenb)
**	    Failure to add the distribution thread as a user of the database
**	    becomes a non-fatal error, we'll just skip the entry and continue.
**	1-jun-98 (stephenb)
**	    Add extra parm to dm2rep_qman() call
**	10-aug-98 (stephenb)
**	    Code tidy up, recode logging and locking calls to use the 
**	    session lock lists.
**      28-oct-1998 (gygro01)
**        bug93810/problem ingrep46, in a multi-server/shared_cache env,
**        error E_SC012D (Database not found in SCF) may occurs, if there
**        aren't anymore connection to the replicated databases in this
**        server. Reinitialized db_added before entering into the
**        transaction queue processing-loop, because we can't be sure that
**        db is still added.
**        Corrected Erslookup badparam for message E_DM9564.
**	4-dec-98 (stephenb)
**	    trans_time is now an HRSYSTIME pointer.
**	10-dec-98 (stephenb)
**	    REP_FULLQ_VAL in Jon's LKevent() change should be REP_READQ_VAL.
**	15-Jun_2000 (inifa01)
**	  bug101852/problem ingsrv1201 In a multiserver environment with
**	  disjoint database list per server the replicator queue management
**        threads die frequently with errors E_SC012D_DB_NOT_FOUND 
**        E_DM9564_REP_SCF_DBUSERADD & E_SC037C_REP_QMAN_EXIT.  The rep threads
** 	  always die while trying to add a db not on its servers database list.
**	  Re-initialized db_add for each iteration of the transaction processing loop.
**	08-Jan-2001 (jenjo02)
**	  Locate *DML_SCB with macro instead of SCU_INFORMATION.
**	  Deleted call to SCU_INFORMATION to get *QEF_CB; 
**	  it was actually returning *DML_SCB and qef_call() takes care 
**	  of this for itself anyway.
**	11-Jun-2003 (hanje04)
**	    Bug 110390
**	    Eliminate race condition where by if REP_READQ Event is raised
**	    whist the disribution thread is processing the input queue, it
**	    will never be received becuase we do not registrer for the event
** 	    until after the processing is complete.
**	    Now register for the event before begining the processing loop,
**	    wait on the event when completed and register as soon as we return 
**	    from the wait. This mean we always have LK_E_SET on the REP_READQ
**   	    event.
**	27-jul-2006 (kibro01) bug 114168
**	    Tell dm2rep_qman that this is a genuine qman thread, not a
**	    hijacked open_db call.
**	20-May-2008 (jonj)
**	    Fix to fix for Bug 110390, above. Create a distinct lock list
**	    for LKevent stuff rather than using the session's scb->scb_lock_list.
**	    With an always pending LK_E_SET on scb_lock_list, other lower-level
**	    LKevents that may occur (see dm2t_wait_for_tcb(), e.g.) during
**	    dm2d_close_db(), dm2d_open_db() will crash on a dmd_check because
**	    the scb_lock_list is already in a LK_E_SET state, causing Bug 118814.
**
**	    Don't signal idling repq threads using REP_FULLQ - no one is waiting
**	    on such an event. Use REP_READQ instead.
**	20-May-2010 (thaju02) Bug 123427
**	    Add lk_id param to dm2rep_qman().
*/
DB_STATUS
dmc_repq(DMC_CB		*dmc_cb)
{
    DMC_CB		*dmc = dmc_cb;
    CL_ERR_DESC		sys_err;
    bool		have_mutex = FALSE;
    bool		db_added = FALSE;
    bool		db_open = FALSE;
    bool		last = FALSE;
    DMC_REPQ		*repq = (DMC_REPQ *)(Dmc_rep + 1);
    DB_STATUS		status = E_DB_OK;
    i4			error;
    i4			locl_err;
    STATUS		cl_stat;
    i4			i, j;
    DMP_DCB		*next_dcb;
    DMP_DCB		*end_dcb;
    DB_STATUS		local_stat;
    SCF_CB		scf_cb;
    QEF_RCB		qef_rcb;
    DB_EVENT_NAME	event_name;
    DML_SCB		*scb;
    LK_EVENT		lk_event;
    LK_LLID		EventLockList;
    DB_ERROR		local_dberr;


    CLRDBERR(&dmc->error);

    /*
    ** consistency check
    */
    if (Dmc_rep == NULL)
    	return(E_DB_ERROR);

    /* Get pointer to DML_SCB */
    scb = GET_DML_SCB(dmc_cb->dmc_session_id);

    /*
    ** Create a lock list to use for LKevent.
    **
    ** We'll use the session's scb_lock_list
    ** for other stuff like opening and closing DBs.
    */
    cl_stat = LKcreate_list(LK_ASSIGN | LK_NONPROTECT, 
				(LK_LLID)0,
				(LK_UNIQUE*)NULL,
				&EventLockList,
				(i4)0,
				&sys_err);
    if ( cl_stat != OK )
    {
	uleFormat(NULL, cl_stat, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(&dmc->error, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	if (cl_stat == LK_NOLOCKS)
	    SETDBERR(&dmc->error, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	return(E_DB_ERROR);
    }

    /* Init LKevent for this thread */
    lk_event.type_high = REP_READQ;
    lk_event.type_low = 0;
    lk_event.value = REP_READQ_VAL;

    MEfill(DB_EVENT_MAXNAME - 13, ' ', event_name.db_ev_name + 13);
    qef_rcb.qef_length	= sizeof(QEF_RCB);
    qef_rcb.qef_type	= QEFRCB_CB;
    qef_rcb.qef_ascii_id	= QEFRCB_ASCII_ID;
    qef_rcb.qef_sess_id	= (CS_SID)dmc_cb->dmc_session_id;
    qef_rcb.qef_eflag	= QEF_EXTERNAL;
    qef_rcb.qef_cb		= (QEF_CB*)NULL;
    qef_rcb.qef_evname	= &event_name;
    qef_rcb.qef_evtext	= NULL;
    qef_rcb.qef_ev_l_text	= 0;

    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_session = (SCF_SESSION)dmc_cb->dmc_session_id;


    for (;;) /* main control loop */
    {
	/* Register with LK event REP_READQ */
	cl_stat = LKevent(LK_E_SET | LK_E_CROSS_PROCESS | LK_E_INTERRUPT, 
	    EventLockList, &lk_event, &sys_err); 
	if ((cl_stat == LK_INTERRUPT) || (cl_stat == LK_INTR_FA))
            /* terminate thread */
	    break;
	else if (cl_stat != OK)
	{
	    uleFormat(NULL, cl_stat, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(&dmc->error, E_DM904B_BAD_LOCK_EVENT, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 3,
		0, LK_E_SET, 0, REP_READQ, 0, EventLockList);
	    status = E_DB_ERROR;
	    break;
	}
	/*
	** check the transaction queue
	*/
	CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
	have_mutex = TRUE;
	db_added = FALSE;
	for (i = Dmc_rep->queue_start, last = FALSE;;i++)
	{
	    db_added = FALSE;
	    if (last)
		break;
	    if (i >= Dmc_rep->seg_size)
		i = 0; /* got to end of seg, wrap round */
	    if (i == Dmc_rep->queue_end)
		last = TRUE;
	    if (repq[i].active || repq[i].tx_id == 0)
		continue;
	    /*
	    ** a transaction to process, check if we have the db 
	    ** added to our server
	    ** WARNING: we hold both the dmf_svcb mutex and the replicator
	    ** transaction queue mutex here, if you change the code logic
	    ** watch out for semaphore deadlock.
	    */
	    dm0s_mlock(&dmf_svcb->svcb_dq_mutex);
	    for (
		next_dcb = dmf_svcb->svcb_d_next,
		end_dcb = (DMP_DCB*)&dmf_svcb->svcb_d_next;
		next_dcb != end_dcb;
		next_dcb = next_dcb->dcb_q_next
		)
	    { 
		if (MEcmp(next_dcb->dcb_name.db_db_name, 
		    repq[i].dbname.db_db_name, DB_DB_MAXNAME) == 0)
		{
		    db_added = TRUE;
		    break;
		}
	    }
	    dm0s_munlock(&dmf_svcb->svcb_dq_mutex);
	    if (!db_added) /* can't process this transaction */
		continue;
	    /*
	    ** if we got to here we have something to do, set active
	    ** release the sem and do the work.
	    */
	    repq[i].active = TRUE;
	    CSv_semaphore(&Dmc_rep->rep_sem);
	    have_mutex = FALSE;
	    /* 
	    ** open the db, we also need to tell SCF that we have the db
	    ** open to prevent it from trying to delete from the server
	    ** while we are refferencing it.
	    */
	    scf_cb.scf_dbname = &repq[i].dbname;
	    status = scf_call(SCU_DBADDUSER, &scf_cb);
	    if (status != E_DB_OK)
	    {
		char	dbname[DB_DB_MAXNAME + 1];

		MEcopy(scf_cb.scf_dbname->db_db_name, DB_DB_MAXNAME, dbname);
		dbname[DB_DB_MAXNAME] = 0;
		uleFormat(&dmc->error, E_DM9564_REP_SCF_DBUSERADD, 
		    NULL, ULE_LOG,
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &locl_err, (i4)2, STtrmwhite(dbname), dbname,
		    sizeof(i4), &repq[i].tx_id);
		CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
		repq[i].active = FALSE;
		CSv_semaphore(&Dmc_rep->rep_sem);
		continue;
	    }
	    status = dm2d_open_db(next_dcb, DM2D_A_WRITE, DM2D_IX, 
		scb->scb_lock_list, (i4)0, &dmc->error);
	    if (status != E_DB_OK)
	    {
		local_stat = scf_call(SCU_DBDELUSER, &scf_cb);
		if (local_stat == E_DB_WARN) /* last user in db */
		    local_stat = dm2d_del_db(next_dcb, &local_dberr);
		break;
	    }
	    db_open = TRUE;
	    /* process the input queue records */
	    status = dm2rep_qman(next_dcb,
		repq[i].tx_id, &repq[i].trans_time, NULL, 0,&dmc->error, FALSE);
	    if (status != E_DB_OK && status != E_DB_WARN)
	    {
		CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
		repq[i].active = FALSE;
		CSv_semaphore(&Dmc_rep->rep_sem);
		break;
	    }
	    else if (status == E_DB_WARN)
	    {
		/* 
		** input queue records could not be processed
		** due to a non-fatal error (e.g. deadlock)
		** skip the entry and continue
		*/
		status = dm2d_close_db(next_dcb, scb->scb_lock_list, 
		    (i4)0, &dmc->error);
		if (status != E_DB_OK)
		    break;
		db_open = FALSE;

		status = scf_call(SCU_DBDELUSER, &scf_cb);
		if ( status == E_DB_ERROR )
		{
		    dmc->error = scf_cb.scf_error;
		    break;
		}
		if (status == E_DB_WARN) /* last user in db */
		    status = dm2d_del_db(next_dcb, &dmc->error);
		if (status != E_DB_OK)
		    break;

		CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
		repq[i].active = FALSE;
		CSv_semaphore(&Dmc_rep->rep_sem);
		continue;
	    }
	    /*
	    ** signal distribution server to distribute replicas, this is 
	    ** done using a database event currently since the distribution
	    ** server is still an external ingres application
	    */
	    MEcopy((next_dcb->dcb_dbservice & 
		(DU_NAME_UPPER | DU_DELIM_UPPER)) ? 
		"DD_DISTRIBUTE" : "dd_distribute", 13, event_name.db_ev_name);
	    qef_rcb.qef_evowner = &next_dcb->dcb_db_owner;
	    status = qef_call(QEU_RAISE_EVENT, (PTR)&qef_rcb);
	    if (status != E_DB_OK)
	    {
		uleFormat(&dmc->error, E_DM9568_REP_DBEVENT_ERROR, 
		    NULL, ULE_LOG,
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &locl_err, (i4)0);
		break;
	    }
	    /* close the db */
	    status = dm2d_close_db(next_dcb, scb->scb_lock_list, 
		(i4)0, &dmc->error);
	    if (status != E_DB_OK)
		break;
	    db_open = FALSE;
	    /* and let scf know */
	    status = scf_call(SCU_DBDELUSER, &scf_cb);
	    if ( status == E_DB_ERROR )
	    {
		dmc->error = scf_cb.scf_error;
		break;
	    }
	    if (status == E_DB_WARN) /* last user in db */
		status = dm2d_del_db(next_dcb, &dmc->error);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** clear out this entry from the TX queue, move the start
	    ** forward if we are the first record
	    **
	    ** NOTE: this should be the only place we move the queue
	    ** start forward, that way we can guaruntee that no-one has
	    ** moved the queue under our feet when we didn't have the
	    ** semaphore
	    */
	    CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
	    have_mutex = TRUE;
	    repq[i].active = FALSE;
	    repq[i].tx_id = 0;
	    if (i == Dmc_rep->queue_start)
	    {
		for (j = i; repq[j].active == 0 && repq[j].tx_id == 0 &&
		  j != Dmc_rep->queue_end; j = (j + 1) % Dmc_rep->seg_size);
		Dmc_rep->queue_start = j;
		/*
		** signal threads waiting on REP_READQ queue entries
		** Note that we are also signaled.
		*/
		cl_stat = LKevent(LK_E_CLR | LK_E_CROSS_PROCESS, 
		    EventLockList, &lk_event, &sys_err);
		if (cl_stat != OK)
		{
		    uleFormat(NULL, cl_stat, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(&dmc->error, E_DM904B_BAD_LOCK_EVENT, &sys_err, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error,
			3, 0, LK_E_CLR, 0, REP_READQ, 0, 
			EventLockList);
		    status = E_DB_ERROR;
		    break;
		}
	    }
	} /* transaction queue processing */
	if (have_mutex)
	    CSv_semaphore(&Dmc_rep->rep_sem);
	if (status != E_DB_OK)
	    break;

	/*
	** wait for work to do
	*/

	cl_stat = LKevent( LK_E_WAIT | LK_E_CROSS_PROCESS | LK_E_INTERRUPT, 
	    EventLockList, &lk_event, &sys_err); 
	if ((cl_stat == LK_INTERRUPT) || (cl_stat == LK_INTR_FA))
            /* terminate thread */
	    break;
	else if (cl_stat != OK)
	{
	    uleFormat(NULL, cl_stat, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(&dmc->error, E_DM904B_BAD_LOCK_EVENT, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 3,
		0, LK_E_WAIT, 0, REP_READQ, 0, EventLockList);
	    status = E_DB_ERROR;
	    break;
	}

    } /* main control loop */

    /*
    ** cleanup
    */
    /* close db */
    if (db_open)
    {
    	local_stat = dm2d_close_db(next_dcb, scb->scb_lock_list, 
	    (i4)0, &local_dberr);
    }

    /* Release lock list */
    cl_stat = LKrelease(LK_ALL, EventLockList, 
			(LK_LKID*)NULL,
			(LK_LOCK_KEY*)NULL,
			(LK_VALUE*)NULL,
			&sys_err);
    if ( cl_stat != OK )
    {
	uleFormat(NULL, cl_stat, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &locl_err, 0);
	uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &locl_err, 1, 0, EventLockList);
    }

    return(status);
}
