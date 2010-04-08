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
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <dmf.h>
#include    <ulf.h>
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
#include    <dm0l.h>
#include    <dmftrace.h>
#include    <lgdstat.h>
#include    <dml.h>
#include    <scf.h>
#include    <dm0llctx.h>

/**
** Name: DMCWRITE.C  - Fast Commit write behind thread.
**
** Description:
**    
**      dmc_write_behind_primary()   -  DMF Primary write behind procedure.
**      dmc_write_behind_clone()     -  DMF Cloned write behind procedure.
**
** History:
**      30-jun-1988 (rogerk)
**          Created for Jupiter.      
**      30-Jan-1989 (ac)
**          Added arguments to LGbegin().      
**	15-may-1989 (rogerk)
**	    Return resource errors if resource limit is exceeded.
**	 8-jul-1992 (walt)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	    Fix prototype errors.
**	26-apr-1993 (bryanp)
**	    Fix parameter passing errors revealed by prototyping LK.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	18-oct-1993 (rogerk)
**	    Add check for LOGFULL status.  We don't execute write behind when
**	    in logfull to avoid background log forces which wreak havoc on
**	    the recovery logspace reservation algorithms.
**	31-jan-1994 (bryanp) B58380, B58381
**	    Log LG/LK status code if LG or LK call fails.
**	    Check return code from CSsuspend.
**	10-jan-1996 (dougb)
**	    To get this file to allow links on VMS platform, use CSswitch()
**	    not the internal Unix CL routine CL_swuser().
**	10-Mar-1998 (jenjo02)
**	    Support for demand-driven, cache-specific WriteBehind threads.
**	    Added new external entry point dmc_write_behind_clone() which
**	    is invoked when dm0p_flush_pages() decides to start a Factotum
**	    thread. The main entry point, dmc_write_behind_primary(), is
**	    invoked during server startup to start a durable "primary"
**	    WriteBehind thread in each designated cache.
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**      21-apr-1999 (hanch04)
**	    Use unsigned i4 for large value
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**          
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	31-Jan-2003 (jenjo02)
**	    Adapted for Cache Flush Agents.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**/

/*
**  Definition of static variables and forward static functions.
*/
static DB_STATUS dmc_write_behind_common(
		    i4	    	i_am_a_clone,
		    char    	*cfa,
		    DB_ERROR    *dmf_err);


/*{
**
** Name: dmc_write_behind_common  -  the guts of a write behind thread
**
** Description:
**
**	The dmc_write_behind routine is used for implementing an asynchronous
**	write behind thread.  It wakes up whenever signaled by an LK event
**	and writes dirty pages out of the cache to make room for new pages
**	to be read in.
**
**	The dmc_write_behind routine should only be called within a special
**	session that is dedicated for this purpose.  This routine will not
**	return under normal circumstances until server shutdown time.
**
**	This routine uses two routines in DM0P to drive the write behind
**	thread:
**	    DM0P_BMFLUSH_WAIT waits for a session in the buffer manager
**	    to signal the event to wake up the write behind threads.  This
**	    is signalled when some specified percent of the buffer manager
**	    is filled with dirty pages.
**
**	    DM0P_FLUSH_PAGES goes through the buffer manager modified queue
**	    in reverse priority order writing pages until some specified
**	    percentage of the buffer manager is free.
**
**	This routine will return only if the event wait in DM0P_BMFLUSH_WAIT
**	is cancelled by an interrupt.  At server shutdown time, the server
**	is expected to interrupt all the write behind threads.
**
**	This common code is executed by both Primary and Cloned
**	WriteBehind agents.
**
** Inputs:
**	i_am_a_clone		FALSE if this is the Primary WB Agent,
**				TRUE if a Clone.
**	cfa			Agent's data.
**
** Outputs:
**     dmf_err
** 	.error.err_code	    One of the following error numbers.
**			    E_DB_OK
**			    E_DM004A_INTERNAL_ERROR
**			    E_DM004B_LOCK_QUOTA_EXCEED
**			    E_DM0062_TRAN_QUOTA_EXCEED
**			    E_DM0117_WRITE_BEHIND
**
** Returns:
**     E_DB_OK
**     E_DB_FATAL
**
** History:
**      30-jun-1988 (rogerk)
**          Created for Jupiter.      
**      30-Jan-1989 (ac)
**          Added arguments to LGbegin().      
**	15-may-1989 (rogerk)
**	    Return resource errors if resource limit is exceeded.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	18-oct-1993 (rogerk)
**	    Add check for LOGFULL status.  We don't execute write behind when
**	    in logfull to avoid background log forces which wreak havoc on
**	    the recovery logspace reservation algorithms.
**	10-oct-93 (swm)
**	    Bug #56438
**	    Put LG_DBID into automatic variable lg_dbid rather than overloading
**	    dmc_cb->dmc_db_id.
**	31-jan-1994 (bryanp) B58380, B58381
**	    Log LG/LK status code if LG or LK call fails.
**	    Check return code from CSsuspend.
**	10-Mar-1998 (jenjo02)
**	    Support for demand-driven WriteBehind threads. Changed prototype
**	    to pass a boolean indicating whether this is the primary or
**	    cloned WB thread and a pointer to DB_ERROR instead of a pointer
**	    to DMC_CB.
**	    Made this a common function called by Primary and Cloned threads.
*/
static DB_STATUS
dmc_write_behind_common(
i4	    i_am_a_clone,
char	    *cfa,
DB_ERROR    *dmf_err)
{
    DM_SVCB	    *svcb = dmf_svcb;
    DB_TRAN_ID	    tran_id;
    LG_LXID	    lx_id;
    DM0L_ADDDB	    add_info;
    TIMERSTAT	    stat_block;
    i4	    lock_list;
    i4		    len_add_info;
    i4		    event_mask;
    i4		    events, wakeup_event;
    i4		    have_locklist = FALSE;
    i4		    have_transaction = FALSE;
    i4		    lg_added = FALSE;
    DB_STATUS	    status = E_DB_OK;
    i4	    wbcount = 0;
    i4	    wait_time = 0;
    i4	    base_time = 0;
    i4	    flush_time, new_time;
    i4	    length;
    i4	    lgd_status;
    STATUS	    stat;
    i4	    error;
    CL_ERR_DESC	    sys_err;
    DB_OWN_NAME	    user_name;
    LG_DBID	    lg_dbid;

#ifdef xDEBUG
    CS_SID	sid;
    i4	pid;

    PCpid(&pid);
    CSget_sid(&sid);

    TRdisplay("Starting Write Behind Thread %x in server process %d\n",
	sid, pid);
#endif

    CLRDBERR(dmf_err);

    if (status == E_DB_OK)
    {
	/*
	** Add write behind thread to logging system.
	** Write behind thread does not actually open a database, so use
	** the LG_NOTDB flag.
	*/
	STmove((PTR)DB_WRITEBEHIND_THREAD, ' ', sizeof(add_info.ad_dbname),
	    (PTR) &add_info.ad_dbname);
	MEcopy((PTR)DB_INGRES_NAME, sizeof(add_info.ad_dbowner),
	    (PTR) &add_info.ad_dbowner);
	MEcopy((PTR)"None", 4, (PTR) &add_info.ad_root);
	add_info.ad_dbid = 0;
	add_info.ad_l_root = 4;
	len_add_info = sizeof(add_info) - sizeof(add_info.ad_root) + 4;

	stat = LGadd(dmf_svcb->svcb_lctx_ptr->lctx_lgid, LG_NOTDB,
	    (char *)&add_info, 
	    len_add_info, &lg_dbid, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &error, 4, 0,
		dmf_svcb->svcb_lctx_ptr->lctx_lgid,
		sizeof(add_info.ad_dbname), (PTR) &add_info.ad_dbname,
		sizeof(add_info.ad_dbowner), (PTR) &add_info.ad_dbowner,
		4, (PTR) &add_info.ad_root);
	    if (stat == LG_EXCEED_LIMIT)
		SETDBERR(dmf_err, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	    else
		SETDBERR(dmf_err, 0, E_DM0117_WRITE_BEHIND);
	    status = E_DB_ERROR;
	}
	else
	    lg_added = TRUE;
    }

    if (status == E_DB_OK)
    {
	/*
	** Begin transaction in order to do LG and LK calls.
	** Must specify NOPROTECT transaction so that LG won't pick us
	** as a force-abort victim.  Also, the Log File BOF can be advanced
	** past this transaction's position in the log file, which means that
	** the Write Behind thread should do no logging nor work that could
	** require backout.
	*/
	STmove((PTR)DB_WRITEBEHIND_THROWN, ' ', sizeof(DB_OWN_NAME), 
							(PTR) &user_name);
	stat = LGbegin(LG_NOPROTECT, lg_dbid, &tran_id, &lx_id,
	    sizeof(DB_OWN_NAME), user_name.db_own_name, 
	    (DB_DIS_TRAN_ID*)NULL, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 
		0, lg_dbid);
	    if (stat == LG_EXCEED_LIMIT)
		SETDBERR(dmf_err, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	    else
		SETDBERR(dmf_err, 0, E_DM0117_WRITE_BEHIND);
	    status = E_DB_ERROR;
	}
	else
	    have_transaction = TRUE;
    }

    if (status == E_DB_OK)
    {
	/*
	** Create locklist to use to wait for Write Behind event.
	*/
	stat = LKcreate_list(LK_NONPROTECT, (i4) 0,
	    (LK_UNIQUE *)&tran_id, (LK_LLID *)&lock_list, (i4)0, 
	    &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    if (stat == LK_NOLOCKS)
		SETDBERR(dmf_err, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	    else
		SETDBERR(dmf_err, 0, E_DM0117_WRITE_BEHIND);
	    status = E_DB_ERROR;
	}
	else
	    have_locklist = TRUE;
    }

    if (status == E_DB_OK)
    {
	/*
	** Now begin loop of waiting for Write Behind event and flushing
	** the buffer manager.
	*/
	do
	{
	    if (DMZ_ASY_MACRO(2))
	    {
		new_time = TMsecs();
		flush_time = new_time - base_time - wait_time;
		base_time = new_time;

		/* Write Write Behind thread statistics. */
		stat = CSstatistics(&stat_block, 0);
		TRdisplay("%22*- DMF Write Behind Thread statistics %21*-\n");
		TRdisplay("    Write Behind wakeups: %d    Cpu : %d    Dio : %d\n",
		    wbcount, stat_block.stat_cpu, stat_block.stat_dio);
		TRdisplay("    Time waiting for event: %d seconds\n",
		    wait_time);
		TRdisplay("    Time to flush pages: %d seconds\n",
		    flush_time);
		TRdisplay("%79*-\n");
	    }

	    /*
	    ** Cloned threads don't wait for a signal, they just
	    ** help flush the cache, then go away.
	    */
	    if (i_am_a_clone == FALSE)
	    {
		/*
		** Wait for the next signal that the buffer manager needs to have
		** pages flushed.
		**
		** This routine will also clear the event from the previous 
		** signal.
		*/
		status = dm0p_wbflush_wait(cfa, lock_list, dmf_err);
		if (status != E_DB_OK)
		{
		    /*
		    ** If warning is returned, that's a signal that
		    ** this thread is to terminate.
		    */
		    if (status == E_DB_WARN)
		    {
			status = E_DB_OK;
			break;
		    }
		    else
		    {
			if (dmf_err->err_code > E_DM_INTERNAL)
			{
			    uleFormat(dmf_err, 0, NULL, ULE_LOG, NULL, 
			    	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
			    SETDBERR(dmf_err, 0, E_DM0117_WRITE_BEHIND);
			}
			break;
		    }
		}
	    }

	    /*
	    ** Check LOGFULL status.  We don't execute write behind when in
	    ** logfull to avoid background log forces which wreak havoc on
	    ** the recovery logspace reservation algorithms.
	    */
	    stat = LGshow(LG_S_LGSTS, (PTR)&lgd_status, 
			    sizeof(lgd_status), &length, &sys_err);
	    if (stat != OK)
	    {
		uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 
		    0, LG_S_LGSTS);
		SETDBERR(dmf_err, 0, E_DM0117_WRITE_BEHIND);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** If logfull, skip the cache flush.
	    */
	    if (lgd_status & LGD_LOGFULL)
	    {
		/*
		** Pause for a moment since the write-behind event will likely
		** be immediately resignaled. We expect that this 5-second
		** wait will return with "timed-out"; if it returns with
		** "interrupted", then the server is being shut down. If it
		** returns with any other return code, something is awry.
		*/
		stat = CSsuspend(CS_TIMEOUT_MASK | CS_INTERRUPT_MASK, 5, 0);
		if (stat == E_CS0008_INTERRUPTED)
		{
		    status = E_DB_OK;
		    break;
		}
		if (stat != E_CS0009_TIMEOUT)
		{
		    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    SETDBERR(dmf_err, 0, E_DM0117_WRITE_BEHIND);
		    status = E_DB_ERROR;
		    break;
		}
	    }
	    else
	    {
		/*
		** Flush some dirty pages out of the Buffer Manager.
		*/

		if (dmf_svcb->svcb_status & SVCB_IOMASTER) 
		{
		    /* in IOMASTER server use same func as write-along thread */
		    i4 numforce;
		    u_i4 duty = 0xffffffff;
		    status = dm0p_write_along(lock_list, (i4)lx_id, 
			    &numforce, duty, dmf_err);
		}
		else
		    status = dm0p_flush_pages(lock_list, (i4)lx_id, 
				    cfa,
				    dmf_err);

		if (status != E_DB_OK)
		{
		    if (dmf_err->err_code > E_DM_INTERNAL)
		    {
			uleFormat(dmf_err, 0, NULL, ULE_LOG, NULL, 
			    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
			SETDBERR(dmf_err, 0, E_DM0117_WRITE_BEHIND);
		    }
		    break;
		}
	    }

	    /*
	    ** If dumping statistics, save time for event to be signaled.
	    */
	    if (DMZ_ASY_MACRO(2))
		wait_time = TMsecs() - base_time;
	    wbcount++;

	} while (i_am_a_clone == FALSE);
    }

    if (i_am_a_clone == FALSE)
    {
	/* Write Fast Commit thread statistics. */
	stat = CSstatistics(&stat_block, 0);
	TRdisplay("\n%22*- DMF Write Behind Thread statistics %21*-\n");
	TRdisplay("    Write Behind wakeup: %d    Cpu : %d    Dio : %d\n",
	    wbcount, stat_block.stat_cpu, stat_block.stat_dio);
	TRdisplay("%79*-\n");
    }

    /*
    ** Clean up transaction and/or lock list left hanging around.
    */
    if (have_transaction)
    {
	stat = LGend(lx_id, 0, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lx_id);
	    if ( status == E_DB_OK )
	    {
		SETDBERR(dmf_err, 0, E_DM0117_WRITE_BEHIND);
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
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lock_list);
	    if ( status == E_DB_OK )
	    {
		SETDBERR(dmf_err, 0, E_DM0117_WRITE_BEHIND);
		status = E_DB_ERROR;
	    }
	}
	have_locklist = FALSE;
    }

    if (lg_added)
    {
	stat = LGremove(lg_dbid, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9016_BAD_LOG_REMOVE, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lg_dbid);
	    if ( status == E_DB_OK )
	    {
		SETDBERR(dmf_err, 0, E_DM0117_WRITE_BEHIND);
		status = E_DB_ERROR;
	    }
	}
    }

    return (status);
}

/*{
**
** Name: dmc_write_behind_primary  -  asynchronous write behind thread (Primary)
**
** Description:
**
**	This is the entry point for the Primary WriteBehind thread, 
**	started during server initialization.
**
** Inputs:
**	ftx		    Factotum Thread eXecution CB
**	 .data			Pointer to Agent data.
**
** Outputs:
**	none
**
** Returns:
**     status from dmc_write_behind_common()
**
** History:
**	01-Apr-1998 (jenjo02)
**	    Created
*/
DB_STATUS
dmc_write_behind_primary(
SCF_FTX		*ftx)
{
    /*
    ** Call the common write_behind function,
    ** identifying ourselves as the Primary WB thread.
    */
    return(dmc_write_behind_common(FALSE, ftx->ftx_data, &ftx->ftx_error));
}

/*{
**
** Name: dmc_write_behind_clone  -  Clone a WriteBehind thread
**
** Description:
**	When a WriteBehind thread's monitoring of the cache indicates that
**	additional assistance is needed, it clones a Factotum thread which
**	gets to this entry point after being created. 
**
**	Cloned WB threads assist in the flushing of the current WB cycle,
**	then terminate.
**
** 	When invoked, they are given the id of the cache which is currently
**	under duress and it's that cache that they begin their work in,
**	though they will wander about seeking other caches that may need
**	help with flushing. See dm0p.c
**
**	This function simply invokes the normal WriteBehind thread code
**	with the indication that it is a cloned thread, and passes the
**	cache in which its work is to begin.
**
** Inputs:
**
**     ftx		    Standard Factotum thread execution parms:
**	ftx_data	    pointer to Agent data.
**	ftx_data_length	    length of data
**	ftx_thread_id	    thread_id of parent thread.
**
** Outputs:
**	none
**
** Returns:
**	status from dmc_write_behind_common()
**
** History:
**	10-Mar-1998 (jenjo02)
**	    Created.
*/
DB_STATUS
dmc_write_behind_clone(
SCF_FTX		*ftx)
{
    /* Call the write_behind function, identifying ourselves as a clone */
    return(dmc_write_behind_common(TRUE, ftx->ftx_data, &ftx->ftx_error));
}


/*{
**
** Name: dmc_write_along   -  asynchronous, unobtrusive  write along thread
**
** EXTERNAL call format:	status = dmf_call(DMC_WRITE_ALONG, &dmc_cb);
**
** Description:
**	The dmc_write_along routine provides an unobtrusive version of the  
**	write behind threads found in normal servers.  It's job is similar,
**	writing dirty pages out of the cache to make room for new pages,
**	but the following differences exist:
**        Write Behind threads			Write Along Threads
**	  ---------------------------    -----------------------------------
**	Run as part of same servers	Run in a dedicated IO server so that SMP
**	that process user threads	CPUs are better utilized, and UNIX
**					priorities may be used for balancing
**
**	Runs on any type of UNIX	Will only come up on an SMP machine
**	hardware.			with 2 or more CPUs.
**
**	Contend against user threads 	If there is an available CPU, only
**	from same server, and cause	contention is against other service
**	context switching overhead.	threads in this IO server, eg the
**					new ReadAhead threads.
**
**	Are woken all at once, in a  	Are woken up periodically so that 
**	'panic' when low on buffers, 	I/O is spread out more evenly.
**	causing spikes in processing.
**	
**	All threads in all servers	The I/O master server is given a
**	check all modified buffers, 	list of databases it is to operate on
**	always attempting to 'fix' a   	and keeps them open. When a buffer
**	TBIO and ignoring this buf if	otherwise qualifies to be written, if
**	cant get TBIO. This is alot	the table is not already open (and its
**	of thrashing, eg if the same	one of the desired database) the table
**    	tables happen not to be open	is opened. Thus there is less senseless
**	in all servers.			spinning around the cache.
**
**	The modified queues are   	At the cost of some extra cpu time  	
**    	scanned, which is smart, but	(on the surface), the entire buffer
**    	requires holding the buffer	header array is scanned for candidates.
**    	manager mutex, which is bad !.	This kind of scan can be done without
**    	This causes dead waits.		the buffer manager mutex, so the
**    					scan does not get in the way of other
**    					concurrent operations. The mutex is
**    					taken only when needed to alter the
**    					status of a chosen buffer.
**    	
**    	Because WB threads operate in	Because WA can afford to be more
**    	panic mode, and must write out	picky, only buffers that will not
**    	buffers right away, no regard	cause a log force (old lsns) will be
**    	is given to the fact that a	picked to be written. This will
**    	log force may occur due to a	reduce the amount of log records
**    	new lsn on the buffer.		written, LG mutexes etc...
**    	
**    	
**	The dmc_write_along  routine should only be called within a special
**	session that is dedicated for this purpose.  This routine will not
**	return under normal circumstances until server shutdown time.
**
**	This routines wakes up periodically, and calls dm0p_write_along(),
**	which writes pages in a manner that is less intrusive than the
**	dm0p_flush_pages() routines used by normal write behind threads.
**	dm0p_write_along() will not request or hold the buffer manager
**	mutes while looking for victims, instead it does a sequential
**	scan through the main buffer array. Other differences are listed
**	in the above chart, and in the function header.
**
**	This routine will return only if the timer                          
**	is cancelled by an interrupt.  At server shutdown time, the server
**	is expected to interrupt all such service threads.      
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
**			    E_DM0163_WRITE_ALONG_FAILURE
**
** Returns:
**     E_DB_OK
**     E_DB_FATAL
**
** History:
**	05-Apr-1995 (cohmi01)
**	    Created, as part of the Manmanx research.
**	10-jan-1996 (dougb)
**	    To get this file to allow links on VMS platform, use CSswitch()
**	    not the internal Unix CL routine CL_swuser().  Also, printf()
**	    should never be called from generic code.
*/
DB_STATUS
dmc_write_along(
DMC_CB	    *dmc_cb)
{
    DMC_CB	    *dmc = dmc_cb;
    DM_SVCB	    *svcb = dmf_svcb;
    DB_TRAN_ID	    tran_id;
    LG_LXID	    lx_id;
    DM0L_ADDDB	    add_info;
    TIMERSTAT	    stat_block;
    i4	    lock_list;
    i4		    len_add_info;
    i4		    event_mask;
    i4		    events, wakeup_event;
    i4		    have_locklist = FALSE;
    i4		    have_transaction = FALSE;
    DB_STATUS	    status = E_DB_OK;
    i4	    wbcount = 0;
    i4	    base_time = 0;
    i4	    flush_time, new_time;
    i4	    length;
    i4	    lgd_status;
#define WA_RUNAGAIN     0     /* indicate no sleep desired */
#define WA_SLEEP        1     /* normal sleep interval if buffers empty */
#define WA_STALL        5     /* sleep interval for log full */
#define WA_YIELD       -1     /* yield to another thread */
    i4	    wa_interval = WA_SLEEP;         
    i4	    numforce = 0;
#define MAX_CLEAN   50
    i4         numclean = 0;
    STATUS	    stat;
    i4	    error;
    CL_ERR_DESC	    sys_err;
    DB_OWN_NAME	    user_name;
    LG_DBID	    lg_dbid;
    static i4  nextwa_threadno = 0;
    i4	    wa_threadno;
    i4	    duties[] = {DM0P_WA_SINGLE | DM0P_WA_GROUPS,
				DM0P_WA_SINGLE,
				DM0P_WA_SINGLE,
                                DM0P_WA_GROUPS};
    i4   	    duty;
#define NUM_DUTY (sizeof(duties)/sizeof (i4))

    CLRDBERR(&dmc->error);

    wa_threadno = nextwa_threadno++;
    duty = duties[wa_threadno % NUM_DUTY];

#ifdef xDEBUG
    TRdisplay(
	"Starting server Write Along Thread for server id 0x%x, duties 0x%x\n",
	      dmc_cb->dmc_id, duty );
#endif

    /*
    ** Add write along  thread to logging system.
    ** Write behind thread does not actually open a database, so use
    ** the LG_NOTDB flag.
    */
    STmove((PTR)DB_BWRITALONG_THREAD, ' ', sizeof(add_info.ad_dbname),
	(PTR) &add_info.ad_dbname);
    MEcopy((PTR)DB_INGRES_NAME, sizeof(add_info.ad_dbowner),
	(PTR) &add_info.ad_dbowner);
    MEcopy((PTR)"None", 4, (PTR) &add_info.ad_root);
    add_info.ad_dbid = 0;
    add_info.ad_l_root = 4;
    len_add_info = sizeof(add_info) - sizeof(add_info.ad_root) + 4;

    stat = LGadd(dmf_svcb->svcb_lctx_ptr->lctx_lgid, LG_NOTDB, (char *)&add_info, 
	len_add_info, &lg_dbid, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
	    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &error, 4, 0,
	    dmf_svcb->svcb_lctx_ptr->lctx_lgid,
	    sizeof(add_info.ad_dbname), (PTR) &add_info.ad_dbname,
	    sizeof(add_info.ad_dbowner), (PTR) &add_info.ad_dbowner,
	    4, (PTR) &add_info.ad_root);
	if (stat == LG_EXCEED_LIMIT)
	    SETDBERR(&dmc->error, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	else
	    SETDBERR(&dmc->error, 0, E_DM0163_WRITE_ALONG_FAILURE);
	return (E_DB_ERROR);
    }


    for(;;)
    {
	/*
	** Begin transaction in order to do LG and LK calls.
	** Must specify NOPROTECT transaction so that LG won't pick us
	** as a force-abort victim.  Also, the Log File BOF can be advanced
	** past this transaction's position in the log file, which means that
	** the Write Along thread should do no logging nor work that could
	** require backout.
	*/
	STmove((PTR)DB_BWRITALONG_THREAD, ' ', sizeof(DB_OWN_NAME), 
							(PTR) &user_name);
	stat = LGbegin(LG_NOPROTECT, lg_dbid, &tran_id, &lx_id,
	    sizeof(DB_OWN_NAME), user_name.db_own_name,
	    (DB_DIS_TRAN_ID*)NULL, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 
		0, lg_dbid);
	    if (stat == LG_EXCEED_LIMIT)
		SETDBERR(&dmc->error, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	    else
		SETDBERR(&dmc->error, 0, E_DM0163_WRITE_ALONG_FAILURE);
	    status = E_DB_ERROR;
	    break;
	}
	have_transaction = TRUE;

	/*
	** Create locklist to use to wait for Write Behind event.
	*/
	stat = LKcreate_list(LK_NONPROTECT, (i4) 0,
	    (LK_UNIQUE *)&tran_id, (LK_LLID *)&lock_list, (i4)0, 
	    &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    if (stat == LK_NOLOCKS)
		SETDBERR(&dmc->error, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	    else
		SETDBERR(&dmc->error, 0, E_DM0163_WRITE_ALONG_FAILURE);
	    status = E_DB_ERROR;
	    break;
	}
	have_locklist = TRUE;

	/*
	** Now begin timer loop for periodically flushing the buffer manager.
	*/
	for (;;)
	{
	    if (DMZ_ASY_MACRO(2))
	    {
		new_time = TMsecs();
		flush_time = new_time - base_time;
		base_time = new_time;

		/* Write Write Along  thread statistics. */
		stat = CSstatistics(&stat_block, 0);
		TRdisplay("%22*- DMF Write Along Thread statistics %21*-\n");
		TRdisplay("    Write Along wakeups: %d    Cpu : %d    Dio : %d\n",
		    wbcount, stat_block.stat_cpu, stat_block.stat_dio);
		TRdisplay("    Time to flush pages: %d seconds\n",
		    flush_time);
		TRdisplay("%79*-\n");
	    }

	    /*
	    ** Wait for some interval before the next pass over the buffers.
	    ** This should return with "timed-out"; if it returns with
	    ** "interrupted", then the server is being shut down. If it
	    ** returns with any other return code, something is awry.
	    */
	    if (wa_interval == WA_YIELD)
	    {
	        /*
	        ** Note:  This routine will yield only to other threads at
	        ** the same or higher priority.
		*/
		CS_swuser();
	    }
	    else
	    if (wa_interval != WA_RUNAGAIN)
	    {
		stat = CSsuspend(CS_TIMEOUT_MASK | CS_INTERRUPT_MASK, 
			    wa_interval, 0);
		if (stat == E_CS0008_INTERRUPTED)
		{
		    status = E_DB_OK;
		    break;  /* server shut-down */
		}
		if (stat != E_CS0009_TIMEOUT)
		{
		    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    SETDBERR(&dmc->error, 0, E_DM0163_WRITE_ALONG_FAILURE);
		    status = E_DB_ERROR;
		    break;
		}
	    }
	    
	    /*
	    ** Check LOGFULL status.  We don't execute write behind when in
	    ** logfull to avoid background log forces which wreak havoc on
	    ** the recovery logspace reservation algorithms.
	    */
	    stat = LGshow(LG_S_LGSTS, (PTR)&lgd_status, 
		    sizeof(lgd_status), &length, &sys_err);
	    if (stat != OK)
	    {
		uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 
		    0, LG_S_LGSTS);
		SETDBERR(&dmc->error, 0, E_DM0163_WRITE_ALONG_FAILURE);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** If logfull continue back to the top of the loop to bypass
	    ** the cache flush and re-wait for timer  interval.
	    ** Use MAX_WA_INTERVAL to pause while log-full is resolved.
	    */
	    if (lgd_status & LGD_LOGFULL)
	    {
		wa_interval = WA_STALL;        
		continue;   /* bypassing flush during logfull */
	    }

	    wbcount++;
	    
	    /*
	    ** Flush some dirty pages out of the Buffer Manager.
	    */
	    status = dm0p_write_along(lock_list, (i4)lx_id, 
		&numforce, duty, &dmc->error);
	    if (status != E_DB_OK)
	    {
		if (dmc->error.err_code > E_DM_INTERNAL)
		{
		    uleFormat(&dmc->error, 0, NULL, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    SETDBERR(&dmc->error, 0, E_DM0163_WRITE_ALONG_FAILURE);
		}
		break;
	    }

	    /* determine next wait period based on how busy we were */
	    /* if we didnt do much, sleep, else ru thru buffers again */
	    if (numforce == 0)
	    {
		wa_interval = WA_SLEEP;  /* things are calm, good nite*/
	    }
	    else
	    {
		wa_interval = WA_RUNAGAIN; /* hit the road again */
	    }

	}

	break;
    }

    /*
    ** Clean up transaction or lock list left hanging around.
    */
    if (have_transaction)
    {
	stat = LGend(lx_id, 0, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lx_id);
	    if ( status == E_DB_OK )
	    {
		SETDBERR(&dmc->error, 0, E_DM0163_WRITE_ALONG_FAILURE);
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
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lock_list);
	    if ( status == E_DB_OK )
	    {
		SETDBERR(&dmc->error, 0, E_DM0163_WRITE_ALONG_FAILURE);
		status = E_DB_ERROR;
	    }
	}
	have_locklist = FALSE;
    }

    stat = LGremove(lg_dbid, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, 
	    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM9016_BAD_LOG_REMOVE, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lg_dbid);
	if ( status == E_DB_OK )
	{
	    SETDBERR(&dmc->error, 0, E_DM0163_WRITE_ALONG_FAILURE);
	    status = E_DB_ERROR;
	}
    }

    /* Write thread statistics. */
    stat = CSstatistics(&stat_block, 0);
    TRdisplay("\n%22*- DMF Write Along Thread statistics %21*-\n");
    TRdisplay("    Write Along wakeup: %d    Cpu : %d    Dio : %d\n",
	wbcount, stat_block.stat_cpu, stat_block.stat_dio);
    TRdisplay("%79*-\n");

    return (status);
}
