/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tr.h>
#include    <tm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <adf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm0p.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm1h.h>
#include    <dm1i.h>
#include    <dm0l.h>
#include    <dm0s.h>
#include    <dm1c.h>
#include    <dm2t.h>
#include    <dmftrace.h>
#include    <lgdstat.h>
#include    <dm0llctx.h>
#include    <dml.h>

/* 
** structure used for passing contect of this thread instance to the 
** function handling a specific readahead request for this thread 
*/
struct  _racontext
{
    i4	locklist;
    DB_TRAN_ID	tran_id;
    i4	log_id;
};



static DB_STATUS prefet_secidx(
	DMC_CB		*dmc,
	struct  _racontext *racontext,
	DMP_PFREQ       *pf,
	i4		curr_ix);

static DB_STATUS prealloc_secidx(
	DMC_CB		*dmc,
	struct  _racontext *racontext,
	DMP_PFREQ       *pf,
	i4		curr_ix);

static DB_STATUS prefet_page(
	DMC_CB		*dmc,
	struct  _racontext *racontext,
	DMP_PFREQ       *pf);


/*{
**
** Name: dmc_read_ahead    -  asynchronous, read ahead (pre fetch) thread   
**
** EXTERNAL call format:	status = dmf_call(DMC_READ_AHEAD, &dmc_cb);
**
** Description:
**
** OVERVIEW          
** 
** The read-ahead threads provide asynchronous read ahead (pre fetch) services 
** to user threads for specific operations that the user thread can 
** predict will be needed in the future, preferably long enough in the
** future that there will some wait/sleep/user-think time during which
** the pre-fetch operation can be carried out. The user thread requests
** a read-ahead service by obtaining the read-ahead mutex, enqueueing
** an event block that describes the request, and signalling the read-ahead
** thread to wake up. The read-ahead thread takes the request and processes
** it, running at the lowest priority, so that any real user work that
** needs to be done will take priority.
** 
** The effect of the read-ahead thread is to reduce the likelyhood that a user
** thread will have to execute a page fault to obtain a block of data.
** Although a page fault is an asynchronous operation with regards to other
** threads in the server, it is synchronous with regards to the user thread
** that requested it, as the thread must wait on the event before continuing.
** By anticipating such reads, and executing them while otherwise idle
** (waiting on other (eg. logging) IO, or waiting for user communication),
** the number of read requests made synchronously for that thread is decreased.
** The reads are done by the read-ahead thread, only physical locks are
** taken, no logical locks are done by the read-ahead thread. If the data
** is already in the buffers  (small data large buffers) then there is no
** performance improvement.
** 
** While there are a number of potential opportunities to make use of
** read-ahead, the current implementation handles the following events 
** that may be anticiptated and requested by user threads:
** 
** 1. Prefetch of sec-ind tuples likely to be deleted or updated.
** 
** Though it may not be intuitive that 'prefetch' could improve performance
** of updates, it turns out that when a table has several secondary indexes,
** delete/update time largely consists of reading in the pages of the
** indexes that contain the index entries that are affected. Since sec-ind
** fields are usualy non-clustered with regard to the order that the
** table is traversed (except for the 1 index that may be involved in the
** traversal), there tends to be much random I/O, with few buffer hits.
** To improve this situation, the dm2r_get() function may choose to
** request (queue up) a prefetch request of some or all 
** related secondary index entries for a tuple that was just read for update.
** If this tuple will indeed shortly be deleted or updated, its secondary
** index entries (all for delete, unknown for update) will have to be
** read eventually by dm2r_delete or dm2r_replace. Instead, dm2r_get signals
** the readahead thread to perform these reads, so that when dm2r_delete
** or dm2r_replace are called by the user threads, no real I/O need be
** performed. 
** 
** More event types could be added in the future, such as for allocating
** space for several new sec-ind tuples at once, or reading the next
** page of data when reading sequentially.
** 
** INTERNALS
** 
** There are 3 main activities involved in the use of prefetch:
** 
** 	1. Requesting (enqueing or scheduling) a prefetch request
** 		- Done by user thread, typically in a dm2r_xxx() function.
** 		- DMP_PFREQ block enqueued on circular queue anchored
** 		  by the DMP_PFH structure (dmf_svcb->svcb_prefetch_hdr)
** 		- May be a guess at best.
** 		- rcb_state is marked RCB_PREFET_PENDING.
** 
** 	2. Executing the prefetch request
** 		- Done by one or more readahead threads
** 		- Multiple RA threads may operate on one or more DMP_PFREQ
** 		  requests concurrently, for example, each thread processing
** 		  a seperate secondary index for the same table.
** 		- rcb_state RCB_PREFET_PENDING is cleared when fully
** 		  executed.
** 
** 	3. Cancelling a prefetch request
** 		- Only done if user thread recognizes that its RCB
** 		  still has an un-processed request outstanding that
** 		  no longer makes sense.
** 		- RCB_PREFET_PENDING checked/removed.
** 
** 
** The user thread must decide whether it 'pays' to request a prefetch,
** as there is some expense associated with requesting and cancelling
** it if not needed. Some of the statistic fields of the RCB, formerly used
** just if DEBUG was on, are used now to determine access characteristics
** for this purpose (rcb_s_get, rcb_s_del).
** 
** In addition, since (for sec index update prefetch) not all fields
** and their secondary indexes may get updated, a new field, rcb_ra_idxupd,
** has been added to the RCB to record the sec indexes that were affected
** by the most recent UPDATE request, in the form of a bitmap that keeps
** track of the first 32 secondary indexes of the table. This field
** is used by subsequent  requests of which sec indexes should or should
** not be prefetched. This assumes that the same fields will tend to
** be updated during consecutive queries.
** 
** When a user thread requests a prefetch event, it adds the DMP_PFREQ
** block to the queue and then signals the RA threads using the same
** mechanism as for the write-behind threads. Note that there are two 
** major differences in scope between the readahead and writebehind threads,
** as readahead threads are only woken up in the server that requested
** it, as opposed to across all servers for WB threads; and readahead
** threads run at a priority lower than that of users threads, whereas
** WB threads run at a higher priority than user threads.
** 
** ENABLING READAHEAD
** 
** To enable readahead, the server must be started with the following
** new config.dat parameter:
** 
** ii.<node>.dbms.<flavor>.read_ahead: <num_threads>
** 
** 
** where <node> and <flavor> are the usual, and <num_threads> is
** an integer indicating how many read ahead threads are desired.
** Note that prefetch operations relating to the delete/update
** of tuples from one table with multiple indexes will benefit from
** multiple readahead threads, in order to process these indexes concurrently.
** 
** 
** It is adviseable to have at least as many IO slaves (II_NUM_SLAVES)
** as the total number of the following thread types:     
** 	write-behind threads
** 	log-writer threads
** 	read-ahead 
** 	ACTIVE, I/O BOUND user threads (1 when in batch mode)
** 
** So if you have 2 log-writers and 4 writebehinds, you should set II_NUM_SLAVES
** to 8, or the readahead thread may not be able to immediately
** schedule a read, and it will 'fall behind'. (this touches on another
** I/O problem, which I hope to address in the future).a This all assumes
** that the underlying I/O subsystem (location/device allocation etc) is 
** capable of handling multiple concurrent I/O, otherwise there are more
** basic tuning problems to be dealt with in the installation.
** 
** Note that readahead events will not get scheduled if at least twice the 
** number of free buffers specified in the 'dmf_free_limit' config parm are 
** not available.
** 
** 
** 
** TUNING/EVALUATING READAHEAD OPERATION
** 
** The DM101 trace point will now print out statistics on the readahead
** events as part of the dump of the new PFH structure. Interesting
** items are as follows:
** 	Status: Whether RA threads are currently running or waiting (PFWAIT)
** 	num blocks:   Total num of available request blocks.
** 	num sched:    Number of requests  currently scheduled.	
**         sched:        Total # of requests that have been sceduled.
**         bufstress:    # of times we avoided readahead due to low number
** 		       of free cache buffers
** 	noblocks:     # of times there were no available request blocks
** 			available in the queue.
** 	canc:        number of request cancelled before or during execution of
** 		     of the request because the requesting thread determined
** 		     that the requested data was no longer needed.
** 	fallbehind:  number of readahead operations that failed to read
** 		     the requested data, presumably because the user thread
** 		     managed to do an update/delete of the data before the
** 		     readahead thread could recognize that the request had
** 		     been cancelled.
** 
** Note that comp+canc can add up to more than sched, because some requests
** may only partially complete before being cancelled, still yielding 
** some benefit to the user thread.
**
**  ------- END LENGTHY DISSERTATION	
**
** Inputs:
**     dmc_cb
** 	.type		    Must be set to DMC_CONTROL_CB.
** 	.length		    Must be at least sizeof(DMC_CB).
**
** Outputs:
**     dmc_cb
** 	.error.err_code	    One of the following error numbers.
**			    E_DB_OK
**			    E_DM004A_INTERNAL_ERROR
**			    E_DM004B_LOCK_QUOTA_EXCEED
**			    E_DM0062_TRAN_QUOTA_EXCEED
**
** Returns:
**     E_DB_OK
**     E_DB_FATAL
**
** History:
**	Apr-05-1995 (cohmi01)
**	    Created, to increase batch oriented performance.
**	Jun-07-1995 (cohmi01)
**	    Add support for multiple prefetch requests, multiple
**	    readahead threads, and the ability for multiplea RA threads
**	    to process the same request concurrently.
**	Jun-09-1995 (cohmi01)
**	    Do not reposition freehdr when de-queueing a request.
**	Jun-26-1995 (cohmi01)
**	    Execute operations against sec index TCB queue in opposite
**	    order that user thread will execute, to allow concurrent IO.
**	    Support additional prefetch requests (secix insert allocation,
**	    page prefetch, overflow chain prefetch).
**	Jul-17-1995 (cohmi01)
**	    Simplify queue management - no need to reorder queue items.
**	    Init the rcb_uiptr to the appropriate element of the array
**	    allocated for this purpose in the prefetch request.
**	    Change lock specifications to indicate PHYSICAL/READ locks
**	    desired, rather than faking that we have a table lock.
**	 1-jul-96 (nick)
**	    Tidy.
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**      12-Apr-1999 (stial01)
**          Distinguish leaf/index info for BTREE/RTREE indexes
**      05-may-1999 (nanpr01)
**          Pass rcb ptr address to dm2r_return_rcb
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2r_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1i_?, dm1h_?, dm1b_? functions 
**	    converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**	25-Aug-2009 (kschendel) 121804
**	    Need dm1h.h to satisfy gcc 4.3.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**	05-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Include dml.h to get prototype.
*/
DB_STATUS
dmc_read_ahead(
DMC_CB	    *dmc_cb)
{
    DMC_CB	    *dmc = dmc_cb;
    DM_SVCB	    *svcb = dmf_svcb;
    LG_LXID	    lx_id;
    DM0L_ADDDB	    add_info;
    i4	    lock_list;
    i4		    len_add_info;
    i4		    have_locklist = FALSE;
    i4		    have_transaction = FALSE;
    DB_STATUS	    status = E_DB_OK;
    i4	    	    error_code;
    struct  _racontext	racontext;
    STATUS	    stat;
    i4	    error;
    CL_ERR_DESC	    sys_err;
    DB_OWN_NAME	    user_name;
    LG_DBID	    lg_dbid;
    DMP_PFH	    *dm_prefet;
    DMP_PFH	    *xph;
    DMP_PFREQ	    *pf;
    bool 	    more_events;
    i4	    curr_ix;
    i4	    count;


#ifdef xDEBUG
    TRdisplay("Starting server Read Ahead Thread for server id 0x%x\n",
	dmc->dmc_id);
#endif

    CLRDBERR(&dmc->error);

    /*
    ** Add read ahead thread to logging system.
    ** Read Ahead thread does not actually open a database, so use
    ** the LG_NOTDB flag.
    */
    STmove((PTR)DB_BREADAHEAD_THREAD, ' ', sizeof(add_info.ad_dbname),
	(PTR) &add_info.ad_dbname);
    MEcopy((PTR)DB_INGRES_NAME, sizeof(add_info.ad_dbowner),
	(PTR) &add_info.ad_dbowner);
    MEcopy((PTR)"None", 4, (PTR) &add_info.ad_root);
    add_info.ad_dbid = 0;
    add_info.ad_l_root = 4;
    len_add_info = sizeof(add_info) - sizeof(add_info.ad_root) + 4;

    stat = LGadd(svcb->svcb_lctx_ptr->lctx_lgid, LG_NOTDB, (char *)&add_info, 
	len_add_info, &lg_dbid, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &sys_err, ULE_LOG, NULL,
	    (char *)0, 0L, (i4 *)0, &error, 4, 0,
	    svcb->svcb_lctx_ptr->lctx_lgid,
	    sizeof(add_info.ad_dbname), (PTR) &add_info.ad_dbname,
	    sizeof(add_info.ad_dbowner), (PTR) &add_info.ad_dbowner,
	    4, (PTR) &add_info.ad_root);
	if (stat == LG_EXCEED_LIMIT)
	    SETDBERR(&dmc->error, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	else
	    SETDBERR(&dmc->error, 0, E_DM0164_READ_AHEAD_FAILURE);
	return (E_DB_ERROR);
    }


    for(;;)
    {
	/*
	** Begin transaction in order to do LG and LK calls.
	** Must specify NOPROTECT transaction so that LG won't pick us
	** as a force-abort victim.  Also, the Log File BOF can be advanced
	** past this transaction's position in the log file, which means that
	** the Read Ahead thread should do no logging nor work that could
	** require backout.
	*/
	STmove((PTR)DB_BREADAHEAD_THREAD, ' ', sizeof(DB_OWN_NAME), 
							(PTR) &user_name);
	stat = LGbegin(LG_NOPROTECT, lg_dbid, &racontext.tran_id, &lx_id,
	    sizeof(DB_OWN_NAME), user_name.db_own_name, 
	    (DB_DIS_TRAN_ID*)NULL, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 
		0, lg_dbid);
	    if (stat == LG_EXCEED_LIMIT)
		SETDBERR(&dmc->error, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	    else
		SETDBERR(&dmc->error, 0, E_DM0164_READ_AHEAD_FAILURE);
	    status = E_DB_ERROR;
	    break;
	}
	have_transaction = TRUE;

	/*
	** Create locklist to use to wait for Read Ahead event.
	*/
	stat = LKcreate_list(LK_NONPROTECT, (i4) 0,
	    (LK_UNIQUE *)&racontext.tran_id, (LK_LLID *)&lock_list, 
	    (i4)0, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL,
	    	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    if (stat == LK_NOLOCKS)
		SETDBERR(&dmc->error, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	    else
		SETDBERR(&dmc->error, 0, E_DM0164_READ_AHEAD_FAILURE);
	    status = E_DB_ERROR;
	    break;
	}
	have_locklist = TRUE;

	/* record items needed by rcb functions, trans_id already saved  */
	racontext.locklist = lock_list;
	racontext.log_id = 0;            

	dm_prefet  = svcb->svcb_prefetch_hdr;	/* request block queue header */
	xph = dm_prefet;

	/* let other threads know that readahead functionality is available */
	svcb->svcb_status |= SVCB_READAHEAD;

	/*
	** Now begin loop of waiting for Read Ahead event and flushing
	** the buffer manager.
	*/
	for (;;)
	{
	    /* Set up a wait event for read-ahead thread to sleep on */   
	    dm0s_mlock(&dm_prefet->pfh_mutex);
	    dm_prefet->pfh_status |= DMP_PFWAIT;
	    dm0s_eset(lock_list, DM0S_READAHEAD_EVENT, svcb->svcb_id);
	    dm0s_munlock(&dm_prefet->pfh_mutex);

	    /* now wait on it till user threads asks for a read-ahead */
	    status =
	    	dm0s_ewait(lock_list, DM0S_READAHEAD_EVENT, svcb->svcb_id);

	    /*
	    ** If warning is returned, then the wait was interrupted
	    ** which signals this thread to terminate.
	    */
	    if (status == E_DB_WARN)
	    {
		status = E_DB_OK;
		break;
	    }

	    /* 
	    ** we have woken up now, handle event that was queued up 
	    ** get prefetch queue mutex, release before operation  
	    */
	    for (more_events = TRUE; more_events;)
	    {
	    	dm0s_mlock(&dm_prefet->pfh_mutex);
		/* 
		** 'next' will normally be pointing at a block that still
		** has items to be done, but if not, traverse till we find
		** one to work on, terminate if none, and re-wait.
		*/
		for (pf = dm_prefet->pfh_next, count = dm_prefet->pfh_numreqs;
		    count--; pf = pf->pfr_next)
		{
		    if (pf->pfr_status & PFR_FREE)
			continue;

		    /* evaluate what work can be done, based on request type */
		    if (pf->pfr_type == PFR_PREFET_SECIDX ||
			pf->pfr_type == PFR_PREALLOC_SECIDX)
		    {
			if (pf->t.si.userix >= pf->t.si.raix)
			{
			    /* 
			    ** Either RA thread did all items, or user thread
			    ** and RA 'met' each other. At this point, RA may
			    ** no longer dispatch requests, but those running
			    ** (ie individual sec ix ops) may continue until
			    ** a cancel request sets that item's interupt bit
			    */
			    if (!(pf->pfr_status & PFR_STALE_REQ))
			    {
			    	pf->pfr_status  |= PFR_STALE_REQ;
				if ((pf->pfr_type == PFR_PREFET_SECIDX &&
				    pf->t.si.userix > PFR_PREFET_MIN) ||
				    (pf->pfr_type == PFR_PREALLOC_SECIDX &&
				    pf->t.si.userix > PFR_PREALLOC_MIN))
			    	    	dm_prefet->pfh_lagbehind++;
			    }
			    if ( !pf->pfr_pincount)
				break; /* we can be the hero to dequeue it */
			}
			else
			{
			    /* 
			    ** this one is runnable (unless already STALE)
			    ** RA thread processes items in reverse order of
			    ** user thread to avoid collisions. Identify
			    ** item we will run, decr cntr for next RA thr
			    */
	    		    curr_ix = pf->t.si.raix--;  
			    break;	/* we've got a live one */
			}
		    }
		    else if (pf->pfr_type == PFR_PREFET_PAGE)     
		    {
			if (pf->t.dp.pageno == 0)
			    pf->pfr_status  |= PFR_STALE_REQ;
			break; /* cant have been pinned, run and/or deque it */
		    }
		    else if (pf->pfr_type == PFR_PREFET_OVFCHAIN)
		    {
			if (pf->t.oc.userpcnt >= pf->t.oc.rapcnt)
			{
			    /* 
			    ** time to give up, either we are done with all,
			    ** or user caught up to RA thread (if userix >0)
			    */
			    if (!(pf->pfr_status & PFR_STALE_REQ))
			    {
			    	pf->pfr_status  |= PFR_STALE_REQ;
				if (pf->t.oc.userpcnt)
			    	    dm_prefet->pfh_lagbehind++;
			    }
			    break; /* cant be pinned, so dequeue it */
			}
		    }
		}  /* END for each request block in queue */

		/* 
		** if went thru all reqs without early break of loop to
		** process a req, there are no more reqs to process now.
		*/
		if (count < 0)
		{
		    more_events = FALSE;
	    	    dm0s_munlock(&dm_prefet->pfh_mutex);
		    break;
		}

		/* 
		** poll to make sure that rcb's owner not waiting for the rcb
		** or that the RA request hasnt become stale due to the user
		** having done another get/position with the rcb. If it is no
		** longer a valid request, skip the 'switch' and fall thru to
		** the code that dequeues this request and continue.
		*/
		if ((pf->pfr_status & (PFR_WAITED_ON | PFR_STALE_REQ)) == 0)
		{
		    pf->pfr_pincount++;   /* mark as busy by a(nother) thread */
		    dm0s_munlock(&dm_prefet->pfh_mutex);

	    	    switch (pf->pfr_type)
	    	    {
		        case PFR_PREFET_SECIDX:   
			    /* read tuples sec indx entries */
		    	    status = prefet_secidx(dmc, &racontext, pf, curr_ix);
		    	    break;

			case PFR_PREALLOC_SECIDX:
			    /* 
			    ** find where sec indx entry belongs. will
			    ** cause necessary pages to be read in
			    */
			    status = prealloc_secidx(dmc, &racontext, pf, curr_ix);
			    break;

			case PFR_PREFET_PAGE:
			    /* read in the requested page */
			    status = prefet_page(dmc, &racontext, pf);
			    break;
		
		        default:
		    	    break;
	    	    }
	    	    dm0s_mlock(&dm_prefet->pfh_mutex);
	    	    pf->pfr_pincount--;	/* this thread is done */
		}


		/* 
		** we wish to deque this request if its done. However, even
		** if our 'cur_ix' was the last/highest one, it could be
		** that a previously selected one is not finished executing
		** yet, so use a pin count of 0 as an indication that no
		** other RA thread is processing it, AND make sure that
		** all ix's have been done, as indicated by the nextix.
		*/
		if ( !pf->pfr_pincount &&
		   (pf->t.si.userix >= pf->t.si.raix ||
		   (pf->pfr_status & (PFR_WAITED_ON | PFR_STALE_REQ))))
		{
		    /* 
		    ** all indexes have been selected and completed or the
		    ** entire request has been cancelled by the user thread
		    */
		    if (pf->pfr_type == PFR_PREFET_SECIDX &&
			pf->t.si.raix == PFR_PREFET_MIN)
		    	    dm_prefet->pfh_racomplete++;
		    else
		    if (pf->pfr_type == PFR_PREALLOC_SECIDX &&
			pf->t.si.raix == PFR_PREALLOC_MIN)
		    	    dm_prefet->pfh_racomplete++;

		    dm_prefet->pfh_next = pf->pfr_next; /* advance */
		    dm_prefet->pfh_freehdr = pf; /* keep track of known free */
    		    /* 
		    ** indicate that the rcb is no longer pinned by a 
		    ** pending RA operation
		    */
    		    pf->t.si.rcb->rcb_state &= ~RCB_PREFET_PENDING;
	    	    pf->pfr_status = PFR_FREE;	/* indicate available */
		    pf->t.si.rcb = NULL;	/* to detect errors */

		    if (--(dm_prefet->pfh_numsched) == 0)
		    	more_events = FALSE;
		}
	    	dm0s_munlock(&dm_prefet->pfh_mutex);

	    }   /* END - loop while there are more_events to handle */
	}  /* END  - loop waiting  for events to be signalled  */

	break;
    }   /* END  - for(;;)  till thread is terminated */


    /*
    ** Clean up transaction or lock list left hanging around.
    */
    if (have_transaction)
    {
	stat = LGend(lx_id, 0, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lx_id);
	    if ( status == E_DB_OK )
	    {
		SETDBERR(&dmc->error, 0, E_DM0164_READ_AHEAD_FAILURE);
		status = E_DB_ERROR;
	    }
	}
	have_transaction = FALSE;
    }
    if (have_locklist)
    {
	stat = LKrelease(LK_ALL, lock_list, (LK_LKID *)0, (LK_LOCK_KEY *)0,
	    (LK_VALUE *)0, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lock_list);
	    if ( status == E_DB_OK )
	    {
		SETDBERR(&dmc->error, 0, E_DM0164_READ_AHEAD_FAILURE);
		status = E_DB_ERROR;
	    }
	}
	have_locklist = FALSE;
    }

    stat = LGremove(lg_dbid, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM9016_BAD_LOG_REMOVE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lg_dbid);
	if ( status == E_DB_OK )
	{
	    SETDBERR(&dmc->error, 0, E_DM0164_READ_AHEAD_FAILURE);
	    status = E_DB_ERROR;
	}
    }

    return (status);
}

static DB_STATUS 
prefet_secidx(
DMC_CB		*dmc,
struct  	_racontext	*racontext,
DMP_PFREQ 	*pf,
i4		curr_ix)
{
    DMP_PFH       	*dm_prefet = dmf_svcb->svcb_prefetch_hdr;
    DMP_RCB             *r = pf->t.si.rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_TCB		*it;
    DMP_RCB		*ir = NULL;
    DM_TID		deltid;
    DM_TID		localtid;
    i4		i;
    DB_STATUS		status;
    DB_STATUS		local_status;
    i4			local_err_code;
    i4			error;
    i4			attno;
    i4			idxnum;	/* ordinal position of index in tcb queue */
    i4		idxupd;	/* bitmap of indexes to prefetch */
    DB_ATTS     **si_keyatts;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmc->error);

#ifdef xDEBUG
    if (r->rcb_type != RCB_CB)
	dmd_check(E_DM9311_DM2R_DELETE);
    if (t->tcb_type != TCB_CB)
	dmd_check(E_DM9311_DM2R_DELETE);
#endif

    deltid.tid_i4 = r->rcb_currenttid.tid_i4;

    /* 
    ** do a read-ahead for indexes on table. Do all indexes if user is
    ** typically deleting records, which affects all indexes, otherwise
    ** for updates, only do indexes that were needed for the previous
    ** update, as indicated by the bits stored in r->rcb_ra_idxupd.
    */
    idxupd = (r->rcb_s_del && (r->rcb_s_del >= (r->rcb_s_get/4))) ?
	0xFFFFFFFF  :  r->rcb_ra_idxupd;

    /* 
    ** Since the index TCB queue was sorted in order of increasing number
    ** of pages, loop thru index queue backwards in order to process items
    ** most likely to incurr IO.to find index at ordinal position 'curr_ix'
    */
    for (it = t->tcb_iq_prev, idxnum = pf->t.si.totix;
	it != (DMP_TCB*) &t->tcb_iq_next && idxnum != curr_ix;
	it = it->tcb_q_prev, --idxnum)
	;  /* just passing thru */

    if (idxnum != curr_ix)
	return (E_DB_WARN);	/* There was no index found in queue */
    /* 
    ** test if this index is desired (all for delete, some for update)
    */
    if (idxnum <= PFR_MAX_ITEMS)
	if ((idxupd & (1 << (idxnum -1))) == 0)
	    return (E_DB_OK);   /* skip this index */

    for (;;)	/* do once, break on error */
    {
    	/* 
	** Allocate RCB for the secondary index. Use the RA thread's lock
    	** list id in case user thread does an event-wait on IO completion
    	** on the buffer we are waiting on for IO completion. 
	*/
    	status = dm2r_rcb_allocate(it, r, &racontext->tran_id, 
	    racontext->locklist, racontext->log_id, &ir, &dmc->error);
    	if (status != E_DB_OK)
	    break;

	/* 
	** Re-poll to make sure that rcb's owner not waiting for the rcb
	** or that the RA request hasnt become stale due to the user
	** having done another get/position with the rcb
	*/
	if (pf->pfr_status & (PFR_WAITED_ON | PFR_STALE_REQ))
	{
	    status = E_DB_OK;   
	    break;
	}

	/*
	** Massage the sec index rcb so it behaves the way we want ...
	** - Use DIRECT cursor protocol, since using DEFERRED could
	**   cause us to skip over the secondary index. 
	** - Request physical share page  locks only.
	** - Just in case, turn logging off.
	** - Indicate that RCB is a prefetch req, for buffer priority.
	*/
	ir->rcb_update_mode = RCB_U_DIRECT;
	ir->rcb_access_mode = RCB_A_READ;
	ir->rcb_k_duration = RCB_K_PHYSICAL;
	ir->rcb_lk_type = RCB_K_PAGE;
	ir->rcb_k_mode  = RCB_K_IS;
	ir->rcb_lk_mode = RCB_K_IS;
	ir->rcb_logging = 0;  
	ir->rcb_state |=  RCB_PREFET_REQUEST;

	/* 
	** if within range of our interupt vector, init to point to our
	** vector so we can interrupt request in progress if user catches up
	*/
	if (curr_ix <= PFR_MAX_ITEMS)		/* curr_ix is '1' based */
	    ir->rcb_uiptr = &pf->pfr_intrupt[curr_ix - 1];

	/*
	** Build the secondary index key from the primary record and
	** tid.  Btree and Isam secondary indexes include the tidp
	** field as the last key entry, while hash indexes do not.
	**
	** NOTE:  The hash search routine below expects a tuple parameter
	** while the isam and btree search routines expect a key.  Here
	** we are building a key value, not a full tuple.  We are assuming
	** that we can send this key to dm1h_search rather than a full
	** tuple because of the following:
	**	    - dm1h_search only looks at the keyed fields (and we are
	**	      providing all of the keyed fields).
	**	    - secondary indexes must be keyed on a prefix of the full
	**	      secondary index tuple (thus the keyed fields have the
	**	      same offset in the key as they would in a full tuple).
	**	    - hash 2nd indexes do not use the tidp as part of the key.
	*/
	for (i = 0; i < it->tcb_keys; i++)
	{
	    attno = it->tcb_ikey_map[it->tcb_att_key_ptr[i] - 1];
	    if (attno > 0)
	    {
		MEcopy(&r->rcb_record_ptr[t->tcb_atts_ptr[attno].offset],
		    t->tcb_atts_ptr[attno].length,
		    &ir->rcb_s_key_ptr[it->tcb_key_atts[i]->key_offset]);
	    }
	    else
	    {
		/* 
		** If the attribute is the tidp, then get the value from
		** the delete tuple id.
		*/
		MEcopy((char *)&deltid, sizeof(DM_TID), 
		    &ir->rcb_s_key_ptr[it->tcb_key_atts[i]->key_offset]);
	    }
	}

	switch (it->tcb_rel.relspec)
	{
	case TCB_HASH:
	    status = dm1h_search(ir, &ir->rcb_data, 
		ir->rcb_s_key_ptr, DM1C_FIND, DM1C_EXACT, 
		&ir->rcb_lowtid, &dmc->error);
	    break;

	case TCB_ISAM:
	    status = dm1i_search(ir, &ir->rcb_data, 
		ir->rcb_s_key_ptr, it->tcb_keys,
		DM1C_FIND, DM1C_EXACT, &ir->rcb_lowtid, &dmc->error);
	    break;

	case TCB_BTREE:
	    ir->rcb_currenttid.tid_i4 = deltid.tid_i4;
	    status = dm1b_search(ir, ir->rcb_s_key_ptr, it->tcb_keys, 
		DM1C_TID, DM1C_EXACT,
		&ir->rcb_other, &ir->rcb_lowtid, 
		&ir->rcb_currenttid, &dmc->error);
	    break;
	}
	if (status != E_DB_OK)
	    break;

	/*	Deallocate the RCB for this index. */
	local_status = dm2r_return_rcb(&ir, &local_dberr);

	break;	/* we have done the read of the index entry */
    }

    if (status != E_DB_OK)
    {
	dm_prefet->pfh_fallbehind++;

	if (dmc->error.err_code > E_DM_INTERNAL)
	{
	    char	    *table_name = "";

	    uleFormat(&dmc->error, 0, NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    if (it)
		table_name = it->tcb_rel.relid.db_tab_name;
	    uleFormat(NULL, E_DM9386_SEC_INDEX_ERROR, NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 2,
		sizeof(DB_TAB_NAME), table_name, 0, deltid.tid_i4);
	    SETDBERR(&dmc->error, 0, E_DM0164_READ_AHEAD_FAILURE);
	}

    }

    /*
    ** Release the ir rcb and any pages fixed in it.
    ** If we broke out of above loop because of an error then we have
    ** not yet done this.
    */
    if (ir)
	local_status = dm2r_return_rcb(&ir, &local_dberr);

    return (status);
}


static DB_STATUS 
prealloc_secidx(
DMC_CB		*dmc,
struct  	_racontext	*racontext,
DMP_PFREQ 	*pf,
i4		curr_ix)
{
    DMP_PFH       	*dm_prefet = dmf_svcb->svcb_prefetch_hdr;
    DMP_RCB             *r = pf->t.si.rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_TCB		*it;
    DMP_RCB		*ir = NULL;
    DM_TID		newtid;
    DM_TID		localtid;
    i4		i;
    DB_STATUS		status;
    DB_STATUS		local_status;
    i4			local_err_code;
    i4			error;
    i4			attno;
    i4			idxnum;	/* ordinal position of index in tcb queue */
    DB_ERROR		local_dberr;

    CLRDBERR(&dmc->error);

#ifdef xDEBUG
    if (r->rcb_type != RCB_CB)
	dmd_check(E_DM9311_DM2R_DELETE);
    if (t->tcb_type != TCB_CB)
	dmd_check(E_DM9311_DM2R_DELETE);
#endif

    newtid.tid_i4 = r->rcb_currenttid.tid_i4;


    /* 
    ** Since the index TCB queue was sorted in order of increasing number
    ** of pages, loop thru index queue backwards in order to process items
    ** most likely to incur IO, finding index at ordinal position 'curr_ix'
    */
    for (it = t->tcb_iq_prev, idxnum = pf->t.si.totix;
	it != (DMP_TCB*) &t->tcb_iq_next && idxnum != curr_ix;
	it = it->tcb_q_prev, --idxnum)
	;  /* just passing thru */

    if (idxnum != curr_ix)
	return (E_DB_WARN);	/* There was no index found in queue */

    for (;;)	/* do once, break on error */
    {
    	/* 
	** Allocate RCB for the secondary index. Use the RA thread's lock
    	** list id in case user thread does an event-wait on IO completion
    	** on the buffer we are waiting on for IO completion.
	*/
    	status = dm2r_rcb_allocate(it, r, &racontext->tran_id, 
	    racontext->locklist, racontext->log_id, &ir, &dmc->error);
    	if (status != E_DB_OK)
	    break;

	/* 
	** Re-poll to make sure that rcb's owner not waiting for the rcb
	** or that the RA request hasnt become stale due to the user
	** having done another get/position with the rcb
	*/
	if (pf->pfr_status & (PFR_WAITED_ON | PFR_STALE_REQ))
	{
	    status = E_DB_INFO; 
	    break;
	}

	/*
	** Massage the sec index rcb so it behaves the way we want ...
	** - Use DIRECT cursor protocol, since using DEFERRED could
	**   cause us to skip over the secondary index. 
	** - Request physical share page  locks only.
	** - Just in case, turn logging off.
	** - Indicate that RCB is a prefetch req, for buffer priority.
	*/
	ir->rcb_update_mode = RCB_U_DIRECT;
	ir->rcb_access_mode = RCB_A_READ;
	ir->rcb_k_duration = RCB_K_PHYSICAL;
	ir->rcb_lk_type = RCB_K_PAGE;
	ir->rcb_k_mode  = RCB_K_IS;
	ir->rcb_lk_mode = RCB_K_IS;
	ir->rcb_logging = 0;  
	ir->rcb_state |=  RCB_PREFET_REQUEST;

	/* 
	** if within range of our interupt vector, init to point to our
	** vector so we can interrupt request in progress if user catches up
	*/
	if (curr_ix <= PFR_MAX_ITEMS)		/* curr_ix is '1' based */
	    ir->rcb_uiptr = &pf->pfr_intrupt[curr_ix - 1];

	/*
	** Build the secondary index key from the primary record and
	** tid.  Btree and Isam secondary indexes include the tidp
	** field as the last key entry, while hash indexes do not.
	**
	** NOTE:  The hash search routine below expects a tuple parameter
	** while the isam and btree search routines expect a key.  Here
	** we are building a key value, not a full tuple.  We are assuming
	** that we can send this key to dm1h_search rather than a full
	** tuple because of the following:
	**	    - dm1h_search only looks at the keyed fields (and we are
	**	      providing all of the keyed fields).
	**	    - secondary indexes must be keyed on a prefix of the full
	**	      secondary index tuple (thus the keyed fields have the
	**	      same offset in the key as they would in a full tuple).
	**	    - hash 2nd indexes do not use the tidp as part of the key.
	*/
	for (i = 0; i < it->tcb_keys; i++)
	{
	    attno = it->tcb_ikey_map[it->tcb_att_key_ptr[i] - 1];
	    if (attno > 0)
	    {
		MEcopy(&r->rcb_srecord_ptr[t->tcb_atts_ptr[attno].offset],
		    t->tcb_atts_ptr[attno].length,
		    &ir->rcb_s_key_ptr[it->tcb_key_atts[i]->key_offset]);
	    }
	    else
	    {
		/* 
		** If the attribute is the tidp, then get the value from
		** the new tuple id.
		*/
		MEcopy((char *)&newtid, sizeof(DM_TID), 
		    &ir->rcb_s_key_ptr[it->tcb_key_atts[i]->key_offset]);
	    }
	}

	switch (it->tcb_rel.relspec)
	{
	case TCB_HASH:
	    status = dm1h_search(ir, &ir->rcb_data, 
		ir->rcb_s_key_ptr, DM1C_FIND, DM1C_EXACT, 
		&ir->rcb_lowtid, &dmc->error);
	    break;

	case TCB_ISAM:
	    status = dm1i_search(ir, &ir->rcb_data, 
		ir->rcb_s_key_ptr, it->tcb_keys,
		DM1C_FIND, DM1C_EXACT, &ir->rcb_lowtid, &dmc->error);
	    break;

	case TCB_BTREE:
	    ir->rcb_currenttid.tid_i4 = newtid.tid_i4;
	    status = dm1b_search(ir, ir->rcb_s_key_ptr, it->tcb_keys, 
		DM1C_TID, DM1C_EXACT,
		&ir->rcb_other, &ir->rcb_lowtid, 
		&ir->rcb_currenttid, &dmc->error);
	    break;
	}

	/*	Deallocate the RCB for this index. */
	local_status = dm2r_return_rcb(&ir, &local_dberr);

	/* 
	** error checking here is the reverse of prefet_secidx() as
	** here we do NOT expect the sec idx entry to exist, if exists,
	** its probably a 'fall behind situation, user thread already
	** managed to insert it.
	*/

	if (status == E_DB_OK)
	{
	    /* finding the index entry probably means user got there first */
	    if (it->tcb_rel.relspec == TCB_BTREE)
	    {
	    	dm_prefet->pfh_fallbehind++;
	    	status = E_DB_INFO;
	    }
	    break;
	}
	else
	if (status == E_DB_ERROR)
	{
	    if (dmc->error.err_code == E_DM0056_NOPART || 
	        dmc->error.err_code == E_DM0065_USER_INTR)
	    {
		/* not found, or interupted. this is OK. This item complete */
	    	status = E_DB_OK;
	    	break;
	    }
	    else
	    if (dmc->error.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
	    {
		/* 
		** We might consider cancelling entire prefetch request
		** when lock contention is detected, perhaps system is too
		** busy to allow prefetch. could just set PFR_STALE on.
		*/
		dm_prefet->pfh_lkcontend++;
		status = E_DB_INFO;
		break;
	    }
	}

	/* fall thru here for read errors */
	{
	    char	    *table_name = "";

	    uleFormat(&dmc->error, 0, NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    if (it)
		table_name = it->tcb_rel.relid.db_tab_name;
	    uleFormat(NULL, E_DM9386_SEC_INDEX_ERROR, NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 2,
		sizeof(DB_TAB_NAME), table_name, 0, newtid.tid_i4);
	    SETDBERR(&dmc->error, 0, E_DM0164_READ_AHEAD_FAILURE);
	    break;
	}

	break;	
    }

    /*
    ** Release the ir rcb and any pages fixed in it.
    ** If we broke out of above loop because of an error then we have
    ** not yet done this.
    */
    if (ir)
	local_status = dm2r_return_rcb(&ir, &local_dberr);

    return (status);
}


static DB_STATUS 
prefet_page(    
DMC_CB 		*dmc,
struct  	_racontext	*racontext,
DMP_PFREQ 	*pf)
{
    DMP_PFH       	*dm_prefet = dmf_svcb->svcb_prefetch_hdr;
    DMP_RCB             *r = pf->t.dp.rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DB_STATUS		status;
    i4			error;
    DMP_PINFO    	pinfo;

    CLRDBERR(&dmc->error);

    status = dm0p_fix_page(r, pf->t.dp.pageno,
    	DM0P_NOREADAHEAD, &pinfo, &dmc->error);

    if ( status == E_DB_OK )
	status = dm0p_unfix_page(r, DM0P_UNFIX, &pinfo, &dmc->error);

    return (status);

}
