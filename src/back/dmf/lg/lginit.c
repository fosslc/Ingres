/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>
#include    <csp.h>
#ifdef VMS
#include    <starlet.h>
#include    <astjacket.h>
#endif

/**
**
**  Name: LGINIT.C - Implements the LGinitialize function of the logging system
**
**  Description:
**	This module contains the code which implements LGinitialize.
**	
**	    LGinitialize -- Prepare this process for using the logging system
**	    LGinit_lock  -- Prepare process for using the logging system and 
**				also lock LGK segment.
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-oct-1992 (jnash) merged 29-jul-1992 (rogerk)
**	    Reduced Logging Project: Changed actions associated with some of
** 	    the Consistency Point protocols. The system now requires
**	    that all servers participate in Consistency Point flushes, not
**	    just fast commit servers:
**            - Added notion of CPAGENT process status.  This signifies that
**              the process controls a data cache that must be flushed when
**              a consistency point is signaled.  Currently, all DBMS servers
**              will have this status and no other process type will.
**            - Added alter entry point LG_A_CPAGENT which causes the CPAGENT
**              process status to be defined in the current process.
**            - Changed the LG_A_BCPDONE case to signal the CPFLUSH event if
**              there are any CPAGENT servers active rather than just
**              fast commit ones.
**            - Changed check_fc_servers routine to check_cp_servers.
**            - Changed last_fc_server routine to last_cp_server.
**            - removed references to obsolete lgd->lgd_n_servers counter.
**	7-oct-1992 (bryanp)
**	    Error handling improvements. Call LGmo_attach_lg().
**	20-oct-1992 (bryanp)
**	    Initialize new LGD fields.
**	3-nov-1992 (bryanp)
**	    Check II_LG_TPOINT when initializing memory.
**	17-nov-1992 (bryanp)
**	    Use STgetwords() to support multiple test points in II_LG_TPOINT.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Init lgd_reserved_space.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (andys/bryanp)
**	    Cluster 6.5 Project I
**	    Renamed stucture members of LG_LA to new names. This means
**		replacing lga_high with la_sequence, and la_offset with
**		la_offset.
**	    When the master runs down, call PCforce_exit on slave processes.
**	    Set lgd_cnodeid to 0 when initializing memory.
**	    Re-instate group commit.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Fix group commit rundown bug(s).
**	    Fix lfb cleanup logic.
**	    Re-arrange code into subroutines to lessen code duplication.
**	    Fix some more logwriter rundown problems.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	26-jul-1993 (jnash)
**	    Add LGinit_lock(), which calls LGK_initialize with flag to
**	    lock the LGK data segment.
**	23-aug-1993 (bryanp)
**	    Fix more group commit rundown bugs.
**	20-sep-1993 (bryanp)
**	    A new and different approach to logwriter rundown. There have been
**		several problems with logwriter rundown. Currently, the issue
**		is that if a process runs itself down, and discovers that it
**		has some logwriter thread(s) which have unwritten log page
**		buffers assigned to them, it needs to somehow process those
**		log pages prior to cleaning up the logwriters -- it can't just
**		abandon the pages, for then they'll never get written by
**		anyone. The first attempt to resolve this was to have the
**		process try to write out its pages when it ran itself down. But
**		this has proved problematic since by the time rundown is called,
**		DI processing may no longer be possible (the files may be
**		closed, the slaves may already have shut down, etc.). The new
**		approach is that if a process finds itself dying while having
**		responsibility for unwritten buffers, it will skip the rundown
**		and allow some other process's check-dead thread to perform the
**		rundown. That other process will have DI capability.
**	18-oct-1993 (rogerk)
**	    Removed unused lgd_dmu_cnt, lxb_dmu_cnt fields.
**	22-nov-1993 (bryanp) B56749
**	    Clean up 20-sep fix by turning off LPB_DEAD in addition to
**		LPB_RUNAWAY when a process skips its own rundown.
**      31-jan-1994 (mikem)
**          bug #56478
**          0 logwriter threads caused installation to hang, fixed by adding
**          special case code to handle synchonous "in-line" write's vs.
**          asynchronous writes.  The problem was that code that scheduled
**          "synchronous" writes with direct calls to "write_block" did not
**          handle correctly the mutex being released during the call.  In the
**          synchronous case one must hold the LG mutex across the DIwrite()
**          call.  Added "release_mutex" argument to LG_do_write().
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems, attach shared sems.
**	20-jun-1994 (jnash)
** 	    Fix lint detected problems: remove unused lxb_end_offset,
**	    lpb_offset, end_offset.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. lgd_mutex must now be explicitly
**	    named in all calls to LKMUTEX.C. Added a host of new
**	    semaphores to suppliment lgd_mutex.
**	05-Sep-1996 (jenjo02)
**	    Removed "release_mutex" argument from LG_do_write().
**	27-Sep-1996 (jenjo02)
**	    Moved acquisition of lgd_lxb_ac_mutex so that it's held when we
**	    decrement lgd_protect_count.
**	08-Oct-1996 (jenjo02)
**	    Set LGD_CLUSTER_SYSTEM in lgd_status if VAX cluster. 
**	    Gave more identifiable names to logging semaphores.
**	31-Oct-1996 (jenjo02)
**	    cleanup_process():
**	    When removing LXB from a wait queue, check that it's still
**	    waiting after getting the wait queue's mutex.
**	13-Dec-1996 (jenjo02)
**	    ldb_lxbo_count now counts active and inactive non-READONLY 
**	    protected transactions.
**	26-feb-1997 (cohmi01)
**		Use E_DMA415 instead of bogus retcode from PCforce_exit. Bug 79047
**	08-jul-1997 (canor01)
**	    Remove cross-process semaphores on rundown.
**	11-Nov-1997 (jenjo02)
**	    Redefined ldb_lxbo_count to include only active, protected
**	    transactions.
**	24-Nov-1997 (jenjo02)
**	    Use CSp|v_semaphore() functions instead of LG_(un)mutex and
**	    ignore status from CSp_semaphore() calls. The mutex may be owned
**	    by a dead process, but we want to continue our work.  
**	    (cleanup_process()).
**      08-Dec-1997 (hanch04)
**          New block field in LG_LA for support of logs > 2 gig
**	17-Dec-1997 (jenjo02)
**	  o LG_do_write() loops while lxb_assigned_buffer, so there's no need
**	    to do the same thing here.
**	  o Replaced idle logwriter (lgd_ilwt) with a logwriter queue,
**	    lgd_lwlxb.
**	  o Added LG_my_pid global.
**	26-Jan-1998 (jenjo02)
**	    Replaced lpb_gcmt_sid with lpb_gcmt_lxb, removed lpb_gcmt_asleep.
**	24-Aug-1998 (jenjo02)
**	    When terminating an active transaction, remove it from the
**	    active transaction hash queue.
**	26-Aug-1998 (jenjo02)
**	    When removing an LXB from a wait queue, make sure it's on one
**	    to avoid clobbering LGK_MEM!
**	22-Dec-1998 (jenjo02)
**	    Moved define of RAWIO_ALIGN to dicl.h (DI_RAWIO_ALIGN).
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    lfb_current_lbb_mutex replaced by individual LBB's lbb_mutex.
**	    Dropped lgd_lxb_aq_mutex and inactive LXB queue.
**	    Added LG_is_mt global.
**	    lgd_wait_free and lgd_wait_split queues consolidated into
**	    lgd_wait_buffer queue.
**	15-Dec-1999 (jenjo02)
**	    Added support for SHARED transactions.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Replaced CScp_resume() with LGcp_resume macro.
**	30-may-2001 (devjo01)
**	    Replace LGcluster with CXcluster_enabled. s103715.
**      11-Jun-2004 (horda03) Bug 112382/INGSRV2838
**          Ensure that the current LBB is stil "CURRENT" after
**          the lbb_mutex is acquired. If not, re-acquire.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**      11-may-2005 (stial01)
**          cleanup_process() send dbms recovery complete msg.
**      26-may-2005 (stial01)
**          cleanup_process() reassign log buffers assigned to dead dbms
**	28-may-2005 (devjo01)
**	    Rename dmfcsp_dbms_rcvry_cmpl to LG_rcvry_cmpl() as code
**	    has been moved to here from DMFCSP to prevent linkage
**	    problems in IPM.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
*/

/*
** Forward declarations of static functions:
*/
static void	cleanup_process(LGD *lgd, LPB *lpb, i4 abort_all);
static void	cleanup_lxb(LGD *lgd, LDB *ldb, LXB **lxb, i4 abort_all, 
			    bool *db_undo_recover);

/*{
** Name: LGinitialize	- Initialize the Logging system.
**
** Description:
**      Must be called prior to any other LG calls.
**
**	This routine calls LGK_initialize to connect this process to the LG/LK
**	shared memory segment (this may have already occurred if LKinitialize
**	has already been called).
**
**	Depending on the return from LGK_initialize(), we may need to initialize
**	the LGD structure.
**
**	For some processes (the RCP, in particular), this routine
**	is called twice (as of Nov, 1990) 
**
** Inputs:
**      none
** Outputs:
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**	    LG_CANTINIT
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	7-oct-1992 (bryanp)
**	    Error handling improvements. Call LGmo_attach_lg().
** 	26-jul-1993 (jnash)
**	    Add zero 'flag' param to LGK_initialize.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems, attach shared sems.
**	05-Jan-1996 (jenjo02)
**	    Removed CSa_semaphore() call for lgd_mutex.
**	18-Dec-1997 (jenjo02)
**	    Added initialization of LG_my_pid.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	06-Aug-2009 (wanfr01)
**	    Bug 122418 - Logstat requires attaching to a valid
**	    shared memory segment.
*/
STATUS
LGinitialize(CL_ERR_DESC *sys_err, char *lgk_info)
{
    STATUS	    status = OK;
    i4	    err_code;
    i4     flag;

    /* Set the globally available pid of this process */
    PCpid(&LG_my_pid);
    LG_is_mt = CS_is_mt();

    if (!STcompare(lgk_info,"logstat"))
	flag = LOCK_LGK_MUST_ATTACH;
    else
	flag = 0;
    if (status = LGK_initialize(flag, sys_err, lgk_info))
    {
	if (status != E_DMA812_LGK_NO_SEGMENT)
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return(E_DMA432_LG_BAD_INIT);
    }

    if (status = LGmo_attach_lg())
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return(E_DMA433_LG_INIT_MO_ERROR);
    }

    return(OK);
}
/*{
** Name: LGinit_lock	- Initialize the Logging system, lock the LGK segment.
**
** Description:
**	This routine is exactly the same as LGinitialize() except that 
**	it locks the LGK segment via the LOCK_LGK_MEMORY parameter to
**	LGK_initialize().
**
** Inputs:
**      none
** Outputs:
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**	    LG_CANTINIT
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
** 	26-jul-1993 (jnash)
**	    Created to support PM-lockable LGK segment.
**	11-nov-1998 (nanpr01)
**	    Initialize the pid.
**	27-Mar-2000 (jenjo02)
** 	    Init LG_is_mt as well.
*/
STATUS
LGinit_lock(CL_ERR_DESC *sys_err, char *lgk_info)
{
    STATUS	    status = OK;
    i4	    err_code;

    /* Set the globally available pid of this process */
    PCpid(&LG_my_pid);
    LG_is_mt = CS_is_mt();

    /*
    ** Request locking the shared lgk data segment.
    */
    if (status = LGK_initialize(LOCK_LGK_MEMORY, sys_err, lgk_info))
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return(E_DMA432_LG_BAD_INIT);
    }

    if (status = LGmo_attach_lg())
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return(E_DMA433_LG_INIT_MO_ERROR);
    }

    return(OK);
}

/*
** Name: LG_meminit	    - First-time only initialization of LG memory
**
** Description:
**	When the LG memory is first created, this routine is called to
**	initialize the various LGD fields.
**
** Inputs:
**	None
**
** Outputs:
**	sys_err		    - standard error handle.
**
** Returns:
**	STATUS
**
** History:
**	20-oct-1992 (bryanp)
**	    Initialize new LGD fields.
**	17-nov-1992 (bryanp)
**	    Use STgetwords() to support multiple test points in II_LG_TPOINT.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Init lgd_reserved_space.
**	26-apr-1993 (andys/bryanp)
**	    Cluster 6.5 Project I
**	    Renamed stucture members of LG_LA to new names. This means
**	    replacing lga_high with la_sequence, and la_offset with la_offset.
**	    Set lgd_cnodeid to 0 when initializing memory.
**	    Re-instate group commit.
**	18-oct-1993 (rogerk)
**	    Removed unused lgd_dmu_cnt, lxb_dmu_cnt fields.
**	25-Jan-1996 (jenjo02)
**	    Added a bunch of new mutexes to suppliment singular
**	    lgd_mutex.
**	08-Oct-1996 (jenjo02)
**	    Set LGD_CLUSTER_SYSTEM in lgd_status if VAX cluster. 
**	    Gave more identifiable names to logging semaphores.
**	17-Dec-1997 (jenjo02)
**	    Replaced idle logwriter (lgd_ilwt) with a logwriter queue,
**	    lgd_lwlxb.
**	15-Dec-1999 (jenjo02)
**	    Init new empty queues, lgd_slxb and lgd_lxb_abort.
**	08-Apr-2004 (jenjo02)
**	    Added lgd_wait_mini wait queue initialization.
**	21-Mar-2006 (jenjo02)
**	    Deleted lbb_space, wasn't really used anyway.
**	28-Aug-2006 (jonj)
**	    Initialize (new) lgd_lfdb_mutex.
**	20-Sep-2006 (kschendel)
**	    Removed free-buffer wait lwq from lgd.
**	11-Sep-2006 (jonj)
**	    Init (new) lgd_wait_commit queue for LOGFULL_COMMIT.
**	10-Nov-2009 (kschendel) SIR 122757
**	    Align buffers to larger of directio and raw-io alignment.
**	16-Dec-2009 (kschendel) SIR 122757
**	    Above needs to protect against no-alignment.
**	17-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Init lgd_rbblocks, lgd_rfblocks.
*/
STATUS
LG_meminit(CL_ERR_DESC *sys_err)
{
    LGD		    *lgd;
    STATUS	    status;
    LBB		    *header_lbb;
    LBH		    *aligned_buffer;
    char	    *tpoint_str;
    i4	    tpoint;
    i4		    num_words;
    i4		    i;
    i4		    align;
    char	    *wordarray[20];

    CL_CLEAR_ERR(sys_err);

    lgd = (LGD *)LGK_base.lgk_lgd_ptr;

    /* zero out the lg database before explicit initialization */
    MEfill(sizeof(LGD), (u_char) 0, (PTR) lgd);

    if (CXcluster_enabled())
	lgd->lgd_status = LGD_CLUSTER_SYSTEM;

    if (status = LG_imutex(&lgd->lgd_mutex, "LG LGD mutex"))
	return (status);
    if (status = LG_imutex(&lgd->lgd_lpbb_mutex, "LG LPBB mutex"))
	return (status);
    if (status = LG_imutex(&lgd->lgd_lxbb_mutex, "LG LXBB mutex"))
	return (status);
    if (status = LG_imutex(&lgd->lgd_lpdb_mutex, "LG LPDB mutex"))
	return (status);
    if (status = LG_imutex(&lgd->lgd_lfbb_mutex, "LG LFBB mutex"))
	return (status);
    if (status = LG_imutex(&lgd->lgd_ldbb_mutex, "LG LDBB mutex"))
	return (status);
    if (status = LG_imutex(&lgd->lgd_lfdb_mutex, "LG LFDB mutex"))
	return (status);
    if (status = LG_imutex(&lgd->lgd_spec_ldb.ldb_mutex, "LG NOTDB LDB mutex"))
	return (status);
    if (status = LG_imutex(&lgd->lgd_local_lfb.lfb_mutex, "LG Local LFB mutex"))
	return (status);
    if (status = LG_imutex(&lgd->lgd_local_lfb.lfb_fq_mutex, "LG Local LFB fq mutex"))
	return (status);
    if (status = LG_imutex(&lgd->lgd_local_lfb.lfb_wq_mutex, "LG Local LFB wq mutex"))
	return (status);


    lgd->lgd_gcmt_threshold = 1;
    lgd->lgd_gcmt_numticks = 5;

    /* Ensure these are never zero */
    lgd->lgd_rbblocks = 1;
    lgd->lgd_rfblocks = 1;

    lgd->lgd_cnodeid = 0;

    lgd->lgd_lpb_next = lgd->lgd_lpb_prev =
			    LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);
    if (status = LG_imutex(&lgd->lgd_lpb_q_mutex, "LG LPB q mutex"))
	return (status);

    lgd->lgd_ldb_next = lgd->lgd_ldb_prev = 
			    LGK_OFFSET_FROM_PTR(&lgd->lgd_ldb_next);
    if (status = LG_imutex(&lgd->lgd_ldb_q_mutex, "LG LDB q mutex"))
	return (status);

    lgd->lgd_lxb_next = lgd->lgd_lxb_prev =
			    LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next);
    if (status = LG_imutex(&lgd->lgd_lxb_q_mutex, "LG LXB q mutex"))
	return (status);

    lgd->lgd_slxb.lxbq_next = lgd->lgd_slxb.lxbq_prev =
			    LGK_OFFSET_FROM_PTR(&lgd->lgd_slxb);

    lgd->lgd_lxb_abort.lxbq_next = lgd->lgd_lxb_abort.lxbq_prev =
			    LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_abort);
    lgd->lgd_lxb_aborts = 0;

    lgd->lgd_lwlxb.lwq_lxbq.lxbq_next = lgd->lgd_lwlxb.lwq_lxbq.lxbq_prev = 
			    LGK_OFFSET_FROM_PTR(&lgd->lgd_lwlxb.lwq_lxbq);
    lgd->lgd_lwlxb.lwq_count = 0;
    if (status = LG_imutex(&lgd->lgd_lwlxb.lwq_mutex, "LG Logwriter q mutex"))
	return(status);

    lgd->lgd_wait_stall.lwq_lxbq.lxbq_next =
	lgd->lgd_wait_stall.lwq_lxbq.lxbq_prev =
		LGK_OFFSET_FROM_PTR(&lgd->lgd_wait_stall.lwq_lxbq);
    lgd->lgd_wait_stall.lwq_count = 0;
    if (status = LG_imutex(&lgd->lgd_wait_stall.lwq_mutex, "LG Wait stall mutex"))
	return (status);

    lgd->lgd_wait_event.lwq_lxbq.lxbq_next =
	lgd->lgd_wait_event.lwq_lxbq.lxbq_prev =
		LGK_OFFSET_FROM_PTR(&lgd->lgd_wait_event.lwq_lxbq);
    lgd->lgd_wait_event.lwq_count = 0;
    if (status = LG_imutex(&lgd->lgd_wait_event.lwq_mutex, "LG Wait event mutex"))
	return (status);

    lgd->lgd_wait_mini.lwq_lxbq.lxbq_next =
	lgd->lgd_wait_mini.lwq_lxbq.lxbq_prev =
		LGK_OFFSET_FROM_PTR(&lgd->lgd_wait_mini.lwq_lxbq);
    lgd->lgd_wait_mini.lwq_count = 0;
    if (status = LG_imutex(&lgd->lgd_wait_mini.lwq_mutex, "LG Wait Mini mutex"))
	return (status);

    lgd->lgd_wait_commit.lwq_lxbq.lxbq_next =
	lgd->lgd_wait_commit.lwq_lxbq.lxbq_prev =
		LGK_OFFSET_FROM_PTR(&lgd->lgd_wait_commit.lwq_lxbq);
    lgd->lgd_wait_commit.lwq_count = 0;
    if (status = LG_imutex(&lgd->lgd_wait_commit.lwq_mutex, "LG Wait commit mutex"))
	return (status);

    lgd->lgd_open_event.lwq_lxbq.lxbq_next =
	lgd->lgd_open_event.lwq_lxbq.lxbq_prev =
		LGK_OFFSET_FROM_PTR(&lgd->lgd_open_event.lwq_lxbq);
    lgd->lgd_open_event.lwq_count = 0;
    if (status = LG_imutex(&lgd->lgd_open_event.lwq_mutex, "LG Open event mutex"))
	return (status);

    lgd->lgd_ckpdb_event.lwq_lxbq.lxbq_next = 
	lgd->lgd_ckpdb_event.lwq_lxbq.lxbq_prev = 
		LGK_OFFSET_FROM_PTR(&lgd->lgd_ckpdb_event.lwq_lxbq);
    lgd->lgd_ckpdb_event.lwq_count = 0;
    if (status = LG_imutex(&lgd->lgd_ckpdb_event.lwq_mutex, "LG CKPDB event mutex"))
	return (status);

    lgd->lgd_serial_io = 0;

    align = DIget_direct_align();
    if (align < DI_RAWIO_ALIGN)
	align = DI_RAWIO_ALIGN;
    /* Size is 2048 because there are two 1k headers, one after the other.
    ** LG will actually write a complete log-block, but the junk that
    ** follows the pair of headers is irrelevant.
    */
    header_lbb = (LBB *)lgkm_allocate_ext( sizeof(LBB) + 2048 + align);
    if (header_lbb == (LBB *)NULL)
	return (LG_EXCEED_LIMIT);

    lgd->lgd_header_lbb = LGK_OFFSET_FROM_PTR(header_lbb);
    
    header_lbb->lbb_state = LBB_FREE;
    if (status = LG_imutex(&header_lbb->lbb_mutex, "LGD header lbb mutex"))
	return (status);
    aligned_buffer = (LBH *) ((char *)header_lbb + sizeof(LBB));
    if (align > 0)
	aligned_buffer = (LBH *) ME_ALIGN_MACRO(aligned_buffer, align);
    header_lbb->lbb_buffer = LGK_OFFSET_FROM_PTR(aligned_buffer);

    lgd->lgd_test_badblk = 0;
    MEfill(sizeof(lgd->lgd_test_bitvec), (u_char)0,
	    (PTR)lgd->lgd_test_bitvec);

    NMgtAt("II_LG_TPOINT", &tpoint_str);
    if (tpoint_str && *tpoint_str)
    {
	num_words = sizeof(wordarray)/sizeof(wordarray[0]);
	STgetwords(tpoint_str, &num_words, wordarray);
	for (i = 0; i < num_words; i++)
	{
	    if (CVal(wordarray[i], &tpoint) == OK)
	    {
	       BTset(  tpoint, &lgd->lgd_test_bitvec[0]);
	    }
	}
    }

    return (OK);
}

/*{
** Name: LG_rundown	- Call when a logging process exits.
**
** Description:
**      This routine is called by the interface code when a process
**	attached to the logging system is exiting.
**
** Inputs:
**      pid                             Process ID.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-oct-1992 (jnash) for rogerk
**	    Reduced logging project.  Elim decrement of fast commit
**	    server count.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:  Removed EBACKUP database state.
**	26-apr-1993 (bryanp)
**	    When the master runs down, call PCforce_exit on slave processes.
**	    If the process was using a non-local LFB, deallocate the LFB when
**		process cleanup completes.
**	26-jul-1993 (bryanp)
**	    Fix group commit rundown bug.
**	    Fix lfb cleanup logic.
**	    Re-arrange code into subroutines to lessen code duplication.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Use new journal log address fields.
**	25-Jan-1996 (jenjo02)
**	    Mutex granularity project. Lots of new mutexes in addition
**	    to lgd_mutex.
**	    If ACP is going away, clear lgd_archiver_lpb.
**	08-jul-1997 (canor01)
**	    Remove cross-process semaphores on rundown.
**	26-may-1999 (thaju02)
**	    In a shared cache configuation, If I detect that I need to 
**	    kill myself because I am a server connected to the shared
**	    buffer manager, kill myself after I have nuked the appropriate 
**	    servers and released all critical mutexes. (B96725)
**	10-Apr-2007 (jonj)
**	    If the RCP dies in a cluster, then all must die so that
**	    cluster recovery can take over.
*/
VOID
LG_rundown(pid)
i4             pid;
{
    register LGD *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB *lpb;
    register LPB *lpb_ptr;
    register LPB *master_lpb;
    SIZE_TYPE	end_offset;
    SIZE_TYPE	lpb_offset;
    SIZE_TYPE	other_offset;
    bool	abort_all = FALSE;
    i4		shrbuf_connect;
    i4		shrbuf_dead;
    i4		shrbuf_active;
    CL_ERR_DESC	sys_err;
    STATUS	status;
    i4	err_code;
    PID		nuke_me = 0;
    bool	RCPIsAlive = TRUE;

    if (lgd->lgd_master_lpb)
    {
	master_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb);

	if ( (RCPIsAlive = PCis_alive(master_lpb->lpb_pid)) == FALSE ||
	     pid == master_lpb->lpb_pid )
	{
	    /* if we are the RCP (ie. the master) or the RCP no longer exists
	    ** then stop all running processes in this rundown routine.
	    */

	    abort_all = TRUE;
	}
    }

    if (LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
	return;

    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);

    for (lpb_offset = lgd->lgd_lpb_next;
	lpb_offset != end_offset;
	)
    {
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb_offset);
	lpb_offset = lpb->lpb_next;

	/*  Find matching process. */

	if (!abort_all && lpb->lpb_pid != pid)
	    continue;

	/*  If the master is aborting, force LG offline. */

	if (lpb->lpb_status & LPB_MASTER)
	{
	    lgd->lgd_status &= ~LGD_ONLINE;	
	    lgd->lgd_master_lpb = 0;
	} 

	/*
	** Unlock the lpb_q before locking the LPB to avoid
	** deadlocking with code doing it the other way (LPB->lpb_q),
	** then relock the lpb_q.
	*/
	(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);
	if (LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
	    return;
	if (LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
	    return;

	/*
	** If LPB no longer looks like an LPB, it was
	** removed from the lpb_q while we waited for
	** the lpb_mutex. In that case, restart the 
	** search.
	*/
	if (lpb->lpb_type != LPB_TYPE)
	{
	    (VOID)LG_unmutex(&lpb->lpb_mutex);
	    if (abort_all)
	    {
		lpb_offset = lgd->lgd_lpb_next;
		continue;
	    }
	    break;
	}
	/*
	** If LG_rundown has already processed this lpb, then skip it.
	** This is not normally expected to happen, but just in case.
	*/
	if (lpb->lpb_status & LPB_RUNAWAY)
	{
	    (VOID)LG_unmutex(&lpb->lpb_mutex);
	    if (abort_all)
		continue;
	    break;
	}

	/*
	** Mark process as being rundown.
	*/

	lpb->lpb_status |= LPB_DEAD;

	/* if archiver, then signal that archiver needs to be started */

	if (lpb->lpb_status & LPB_ARCHIVER)
	{
	    lgd->lgd_archiver_lpb = 0;
	    LG_signal_event(LGD_START_ARCHIVER, 0, FALSE);
	}

	/*
	** If this is a server connected to a shared buffer manager,
	** then check for any other servers connected to the same
	** buffer manager.
	**
	** If there are still servers connected to this buffer manager
	** then recovery must be delayed until those servers are also
	** rundown - call kill() to expedite this.  In this case we will
	** return without completing rundown.  When the last connected server
	** is rundown, it will reinitiate rundown for this process to complete
	** it.
	*/
	shrbuf_connect = 0;
	shrbuf_dead = 0;
	shrbuf_active = 0;
	if (lpb->lpb_status & LPB_SHARED_BUFMGR)
	{
	    /*
	    ** Look for other servers connected to this buffer manager.
	    */
	    for (other_offset = lgd->lgd_lpb_next;
		 other_offset != end_offset;
		 other_offset = lpb_ptr->lpb_next)
	    {
		lpb_ptr = (LPB *)LGK_PTR_FROM_OFFSET(other_offset);

		if ((lpb_ptr->lpb_status & LPB_SHARED_BUFMGR) &&
		   (lpb_ptr->lpb_bufmgr_id == lpb->lpb_bufmgr_id) &&
		   (lpb_ptr != lpb))
		{
		    if (lpb_ptr->lpb_status & LPB_DEAD)
			shrbuf_dead++;
		    else
			shrbuf_connect++;

		    if ((lpb_ptr->lpb_status & (LPB_DEAD | LPB_DYING)) == 0)
			shrbuf_active++;
		}
	    }

	    /*
	    ** If there are active servers connected to this buffer manager
	    ** then we must kill them.
	    **
	    ** Note that we leave the lpb_q_mutex EXCL locked;
	    ** LG_lpb_shrbuf_nuke() expects this.
	    ** Release mutex on LPB; shrbuf_nuke will attempt to
	    ** lock all LPBs EXCL and we don't want to conflict.
	    */
	    if (shrbuf_active)
	    {
		(VOID)LG_unmutex(&lpb->lpb_mutex);
		LG_lpb_shrbuf_nuke(lpb->lpb_bufmgr_id, &nuke_me);
		(VOID)LG_mutex(SEM_EXCL, &lpb->lpb_mutex);
	    }

	    /*
	    ** If all servers connected to this buffer manager have not yet
	    ** been rundown then we cannot complete rundown.  LG_rundown will
	    ** be called again when the last server exits.
	    */
	    if (shrbuf_connect)
	    {
		(VOID)LG_unmutex(&lpb->lpb_mutex);
		if (abort_all)
		    continue;
		break;
	    }

	}
#if defined(conf_CLUSTER_BUILD)
	else if ( !RCPIsAlive && CXcluster_enabled() )
	{
	    /*
	    ** RCP has died in a cluster. Kill all servers, saving myself
	    ** for last.
	    */
	    (VOID)LG_unmutex(&lpb->lpb_mutex);
	    LG_lpb_node_nuke(&nuke_me);
	    (VOID)LG_mutex(SEM_EXCL, &lpb->lpb_mutex);
	}
#endif

	/*
	** Release the lpb queue mutex. Other functions called
	** below scan the queue and need the mutex to do so.
	*/
	(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

	/*
	** Mark this server as being recovered.  Consistency Points will
	** now be stalled until this LPB is removed (recovery is complete).
	*/
	if ((lpb->lpb_status &  (LPB_ARCHIVER | LPB_MASTER)) == 0)
	    lpb->lpb_status |= LPB_RUNAWAY;
	lpb->lpb_cond = LPB_RECOVER;

	/*
	** If this server is the last connected to a shared buffer manager
	** and there are servers which have died but have not been cleaned
	** up yet (because they had to wait till all servers connected to
	** the shared buffer manager had died), then call rundown for those
	** servers.
	**
	** Don't do this if we are in recursive call via lpb_shrbuf_disconnect.
	*/
	if ((shrbuf_dead) && (shrbuf_connect == 0) &&
	    ((lpb->lpb_status & LPB_FOREIGN_RUNDOWN) == 0))
	{
	    LG_lpb_shrbuf_disconnect(lpb->lpb_bufmgr_id);
	}

	/*
	** cleanup_process() wants the lpb_mutex held on entry;
	** it'll be responsible for freeing it, either by deallocating
	** the LPB or by other means.
	*/
	cleanup_process( lgd, lpb, abort_all );

	/*
	** Since the lpb queue may have been altered, 
	** reacquire the mutex and start the search again
	** from the top of the queue.
	*/
	(VOID)LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex);
	lpb_offset = lgd->lgd_lpb_next;

	if (!abort_all)
	    break;
    }

    (VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

    if (abort_all)
    {
	/* Cancel the piggy-back write timer if master exit. */

#ifdef VMS_CLUSTER_STUFF
	sys$cantim(PGY_WRITE_TIMID, 0);
	if (lk_cluster)
	{
	    sys$cantim(lgc_timerid, 0);	
	    sys$deq(lgc_stmpsb[1],0,0,0);	    
	}
#else /* VMS */

	/* Clean up cross-process mutexes */
	CSremove_all_sems( CS_SEM_MULTI );
#endif /* VMS */
    }       

    /* 
    ** Now that I have released the mutexes,
    ** if I am supposed to kill myself, let's do it now 
    ** and then another server will cleanup after me.
    ** After I issue the kill -9 for myself, sleep for
    ** a minute to be sure that I do not have time to 
    ** return up the call stack to LG_check_dead and get the
    ** LG lpb queue mutex in the small time frame between
    ** when the kill -9 is issued and is executed by the os.
    */
    if (nuke_me)
    {
	(void) PCforce_exit(nuke_me, &sys_err);
	PCsleep(60000);
    }
	
}

/*
** Name: cleanup_process		clean up this process's LG structures.
**
** Descrption:
**	This routine cleans up the LG structures for a process.
**
**	If the process is running itself down (as opposed to being run down by
**	some other process's check-dead thread), then if the process finds that
**	it still has some logwriter threads with unwritten log pages, then the
**	process doesn't try to do its own rundown; instead it waits for another
**	process to do the rundown; this is because the dying process may no
**	longer be able to make DI calls (LGclose may have already run, or the
**	slaves may have been stopped already, etc.)
**
** History:
**	26-jul-1993 (bryanp)
**	    Split this code out of LG_rundown in an attempt to make the overall
**		rundown code manageable. I don't think I entirely succeeded, but
**		maybe I made things better than they were...
**	23-aug-1993 (bryanp)
**	    Fix more group commit rundown bugs. If the process which died has
**	    	a group commit thread, then unconditionally set lpb_gcmt_sid
**		to 0, even if at this instant there is no timer_lbb. Once this
**		process gets rundown, we can't leave lpb_gcmt_sid set to
**		anything, because this process's group commit thread is no
**		longer responding to group commit requests (the process, after
**		all, is dead!)
**	20-sep-1993 (bryanp)
**	    A new and different approach to logwriter rundown. There have been
**		several problems with logwriter rundown. Currently, the issue
**		is that if a process runs itself down, and discovers that it
**		has some logwriter thread(s) which have unwritten log page
**		buffers assigned to them, it needs to somehow process those
**		log pages prior to cleaning up the logwriters -- it can't just
**		abandon the pages, for then they'll never get written by
**		anyone. The first attempt to resolve this was to have the
**		process try to write out its pages when it ran itself down. But
**		this has proved problematic since by the time rundown is called,
**		DI processing may no longer be possible (the files may be
**		closed, the slaves may already have shut down, etc.). The new
**		approach is that if a process finds itself dying while having
**		responsibility for unwritten buffers, it will skip the rundown
**		and allow some other process's check-dead thread to perform the
**		rundown. That other process will have DI capability.
**	    If we try to make a DI call and it fails, return immediately (don't
**		continue trying to perform rundown).
**	    If a DI call fails and we return without completing rundown, clear
**		the LPB runaway bit so that some other process can give it
**		another whirl.
**	22-nov-1993 (bryanp) B56749
**	    Clean up 20-sep fix by turning off LPB_DEAD in addition to
**		LPB_RUNAWAY when a process skips its own rundown.
**	25-Jan-1996 (jenjo02)
**	    lpb_mutex must be held on entry to this function, and
**	    will be freed upon exit.
**	26-Jun-1996 (jenjo02)
**	    Count only active, protected txns in lgd_protect_count.
**	31-Oct-1996 (jenjo02)
**	    When removing LXB from a wait queue, check that it's still
**	    waiting after getting the wait queue's mutex.
**	26-feb-1997 (cohmi01)
**		Use E_DMA415 instead of bogus retcode from PCforce_exit. Bug 79047
**	11-Nov-1997 (jenjo02)
**	    Redefined ldb_lxbo_count to include only active, protected
**	    transactions.
**	24-Nov-1997 (jenjo02)
**	    Use CSp|v_semaphore() functions instead of LG_(un)mutex and
**	    ignore status from CSp_semaphore() calls. The mutex may be owned
**	    by a dead process, but we want to continue our work.
**	17-Dec-1997 (jenjo02)
**	    LG_do_write() loops while lxb_assigned_buffer, so there's no need
**	    to do the same thing here.
**	24-Aug-1998 (jenjo02)
**	    When terminating an active transaction, remove it from the
**	    active transaction hash queue.
**	26-Aug-1998 (jenjo02)
**	    When removing an LXB from a wait queue, make sure it's on one
**	    to avoid clobbering LGK_MEM!
**	15-Dec-1999 (jenjo02)
**	    Add support for recovery of shared log transactions 
**	    (LXB_SHARED/LXB_SHARED_HANDLE).
**	20-May-2005 (jenjo02)
**	    Don't dmfcsp_dbms_rcvry_cmpl() if abort_all; no recovery
**	    has been done, let failover recovery figure it out.
**	    On abort_all, ignore status from PCforce_exit as the
**	    process may already be gone.
**	26-Jun-2006 (jonj)
**	    Tweaks to HANDLE transactions for XA support.
*/
static void
cleanup_process( register LGD *lgd, register LPB *lpb, i4  abort_all )
{
    LPB		*next_lpb;
    LPB		*prev_lpb;
    LFB		*lfb;
    LDB		*ldb;
    LDB		*next_ldb;
    LDB		*prev_ldb;
    LPD		*lpd;
    LPD		*next_lpd;
    LPD		*prev_lpd;
    LXB		*lxb, *slxb;
    LXBQ	*lxbq;
    LXBQ	*wait_lxbq;
    LXBQ	*next_lxbq;
    LXBQ	*prev_lxbq;
    LBB		*lbb;
    LWQ		*lwq;
    SIZE_TYPE	lpd_offset;
    SIZE_TYPE	lxbq_offset;
    SIZE_TYPE	lpd_end_offset;
    SIZE_TYPE	next_lxbq_offset;
    bool	db_undo_recover;
    bool	db_redo_recover;
    bool	proc_undo_recover;
    bool	proc_redo_recover;
    CL_ERR_DESC	sys_err;
    STATUS	status;
    i4	err_code;

    if (abort_all == 0)
    {
	/*
	** If this process is running itself down, and if it has a logwriter
	** thread with an assigned buffer, then rundown can't be performed,
	** because we'd need to perform DI calls and that may no longer be
	** possible. So instead we forfeit our attempt to perform our own
	** rundown, and leave it to be performed by some other process's
	** check-dead thread, which *will* have the capability of performing
	** DI calls.
	*/
	if (LG_my_pid == lpb->lpb_pid)
	{
	    for (lpd_offset = lpb->lpb_lpd_next;
		 lpd_offset != LGK_OFFSET_FROM_PTR(&lpb->lpb_lpd_next);
		 lpd_offset = lpd->lpd_next)
	    {
		lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd_offset);

		if (lpd->lpd_type != LPD_TYPE)
		    continue;

		for (lxbq_offset = lpd->lpd_lxbq.lxbq_next;
		     lxbq_offset != LGK_OFFSET_FROM_PTR(&lpd->lpd_lxbq.lxbq_next);
		     lxbq_offset = lxbq->lxbq_next)
		{
		    lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq_offset);
		    lxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB,lxb_owner));
		    if (lxb->lxb_status & LXB_LOGWRITER_THREAD &&
			lxb->lxb_assigned_buffer)
		    {
			lpb->lpb_status &= ~(LPB_RUNAWAY | LPB_DEAD);
			(VOID)LG_unmutex(&lpb->lpb_mutex);
			return;
		    }
		}
	    }
	}

	proc_undo_recover = FALSE;
	proc_redo_recover = FALSE;

	/*
	** If the process which exited is a checkpoint process, then
	** check to see if any orphaned databases where left in
	** mid-checkpoint.
	*/
	if (lpb->lpb_status & LPB_CKPDB)
	    LG_cleanup_checkpoint(lpb);

    }

    /*
    ** Go through each database that this process was using. Unless we're
    ** aborting all processes, compute whether recovery is needed, and signal
    ** the RCP if necessary. Clean up as much of the database and transaction
    ** context as possible.
    */

    lpd_offset = lpb->lpb_lpd_next;
    lpd_end_offset = LGK_OFFSET_FROM_PTR(&lpb->lpb_lpd_next);

    while (lpd_offset != lpd_end_offset)
    {
	/*	Remove next LPD from queue. */

	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd_offset);

	if (lpd->lpd_type != LPD_TYPE)
	{
	    lpd_offset = lpd->lpd_next;
	    continue;
	}

	ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

	(VOID)CSp_semaphore(SEM_EXCL, &ldb->ldb_mutex);

	if (!abort_all)
	{
	    db_undo_recover = FALSE;
	    db_redo_recover = FALSE;

	    /*
	    ** If the runaway server has FC capability and the pointed
	    ** database can support FC, then this database will require
	    ** redo recovery.
	    */
	    if ((lpb->lpb_status & LPB_FCT) &&
		(ldb->ldb_status & LDB_FAST_COMMIT))
	    {
		db_redo_recover = TRUE;
		ldb->ldb_status |= LDB_RECOVER;
	    }
	}

	/* 
	** Find a list of transactions that belong to the LPD
	** and mark them needed to be recovered.
	** Note that ldb_mutex protects lpd_lxbq 
	*/

	lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lpd->lpd_lxbq.lxbq_next);

	while (lxbq != (LXBQ *)&lpd->lpd_lxbq.lxbq_next)
	{
	    lxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB,lxb_owner));

	    lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_next);

	    (VOID)CSp_semaphore(SEM_EXCL, &lxb->lxb_mutex);

	    if (lxb->lxb_status & LXB_LOGWRITER_THREAD)
	    {
		/*
		** Note that LG_do_write() loops while lxb_assigned_buffer.
		** This is because
		** at the completion of the first write the logwriter may
		** attempt to reschedule itself for a subsequent write. It
		** continues performing the writes until there are no more, at
		** which point we remove the logwriter thread from the system.
		*/
		if (lxb->lxb_assigned_buffer)
		{
		    status = LG_do_write(lxb, &sys_err);
		    if (status)
		    {
			uleFormat(NULL, status, &sys_err, ULE_LOG,
				    NULL, NULL, 0, NULL, &err_code, 0);
			lpb->lpb_status &= ~LPB_RUNAWAY;
			(VOID)CSv_semaphore(&ldb->ldb_mutex);
			(VOID)CSv_semaphore(&lpb->lpb_mutex);
			(VOID)CSv_semaphore(&lxb->lxb_mutex);
			return;
		    }
		}
	    }

	    /*
	    **  Remove LXB from any wait queue. 
	    **
	    ** Note that this will remove LogWriter from lgd_lwlxb queue.
	    **
	    ** The GroupCommit thread marks itself as waiting, but it not
	    ** really on any wait queue (it's CSsuspend-ed), so check for 
	    ** a null lxb_wait_lwq as well.
	    */

	    /* If waiting on buffer I/O, remove LXB from buffer's wait queue */
	    if (lxb->lxb_status & LXB_WAIT_LBB)
	    {
		lbb = (LBB *)LGK_PTR_FROM_OFFSET(lxb->lxb_wait_lwq);

		(VOID)CSp_semaphore(SEM_EXCL, &lbb->lbb_mutex);

		if (lbb->lbb_wait_count)
		{
		    next_lxbq = 
			(LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_wait.lxbq_next);
		    prev_lxbq = 
			(LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_wait.lxbq_prev);
		    next_lxbq->lxbq_prev = lxb->lxb_wait.lxbq_prev;
		    prev_lxbq->lxbq_next = lxb->lxb_wait.lxbq_next;
		    lbb->lbb_wait_count--;
		}
		(VOID)CSv_semaphore(&lbb->lbb_mutex);
	    }
	    else if (lxb->lxb_status & LXB_WAIT && lxb->lxb_wait_lwq)
	    {
		lwq = (LWQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_wait_lwq);
		(VOID)CSp_semaphore(SEM_EXCL, &lwq->lwq_mutex);

		/* Check LXB again after getting wait queue mutex */
		if (lxb->lxb_status & LXB_WAIT)
		{
		    wait_lxbq = &lxb->lxb_wait;

		    next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(wait_lxbq->lxbq_next);
		    prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(wait_lxbq->lxbq_prev);
		    next_lxbq->lxbq_prev = wait_lxbq->lxbq_prev;
		    prev_lxbq->lxbq_next = wait_lxbq->lxbq_next;
		    lwq->lwq_count--;
		}
		(VOID)CSv_semaphore(&lwq->lwq_mutex);
	    }
	    lxb->lxb_status &= ~(LXB_WAIT | LXB_WAIT_LBB);
	    lxb->lxb_wait_lwq = 0;
	    lxb->lxb_wait_reason = 0;

	    /*
	    ** If this is a SHARED LXB or a HANDLE to a SHARED LXB
	    **
	    **	o manually abort each HANDLE txn which is in a process
	    **	  other than the one being run down. Recovery of the
	    **	  shared transaction will be instituted as those txns 
	    **	  abort.
	    **  o for those HANDLE txns which belong to the process
	    **	  being run down, remove them from the SHARED LXB's
	    **    list and fall thru to deallocate the HANDLE LXBs 
	    **	  (which are always marked NONPROTECT).
	    **	o If the SHARED handle count becomes zero, unSHARE
	    ** 	  the LXB and give it over to recovery.
	    **
	    ** RCP does not want to see HANDLES attached to a 
	    ** SHARED transaction - it won't know what to do with
	    ** them.
	    */
	    if ( lxb->lxb_status & (LXB_SHARED_HANDLE | LXB_SHARED) )
	    {
		LPD	*handle_lpd;
		LPB	*handle_lpb;
		LXB	*handle_lxb;
		LXBQ	*handle_lxbq;

		slxb = lxb;

		if ( lxb->lxb_status & LXB_SHARED_HANDLE )
		{
		    slxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);
		    (VOID)CSp_semaphore(SEM_EXCL, &slxb->lxb_mutex);
		}

#ifdef LG_SHARED_DEBUG
		TRdisplay("%@ cleanup_process %d ABORT PID %d on LXB %x sLXB %x handles %d\n",
		    LG_my_pid, lpb->lpb_pid,
		    *(u_i4*)&lxb->lxb_id, *(u_i4*)&slxb->lxb_id,
		    slxb->lxb_handle_count);
#endif /* LG_SHARED_DEBUG */
		
		handle_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(slxb->lxb_handles.lxbq_next);

		while ( handle_lxbq != (LXBQ *)&slxb->lxb_handles )
		{
		    handle_lxb = (LXB*)((char*)handle_lxbq - CL_OFFSETOF(LXB,lxb_handles));
		    handle_lpd = (LPD *)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_lpd);
		    handle_lpb = (LPB *)LGK_PTR_FROM_OFFSET(handle_lpd->lpd_lpb);

		    handle_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(handle_lxbq->lxbq_next);

		    if ( handle_lxb != lxb )
			(VOID)CSp_semaphore(SEM_EXCL, &handle_lxb->lxb_mutex);

		    /*
		    ** If the handle's transaction belongs to a process other than 
		    ** the one being run down, manually abort it.
		    */
		    if ( !abort_all &&
		         (handle_lxb->lxb_status & LXB_ABORT) == 0 &&
			 handle_lpb != lpb &&
			 handle_lpb->lpb_force_abort )
		    {
			handle_lxb->lxb_status |= LXB_MAN_ABORT;

			/* LK_rundown has already released the associated lock list */
			handle_lxb->lxb_lock_id = 0;

#ifdef LG_SHARED_DEBUG
			TRdisplay("%@ cleanup_process %d ABORT PID %d on handle LXB %x sLXB %x MAN_ABORT\n",
			    LG_my_pid, handle_lpb->lpb_pid,
			    *(u_i4*)&handle_lxb->lxb_id, *(u_i4*)&slxb->lxb_id);
#endif /* LG_SHARED_DEBUG */

			/* Add this LXB to the end of the list of those to be aborted */
			(VOID)CSp_semaphore(SEM_EXCL, &lgd->lgd_mutex);
			handle_lxb->lxb_lgd_abort.lxbq_prev = 
			    lgd->lgd_lxb_abort.lxbq_prev;
			handle_lxb->lxb_lgd_abort.lxbq_next = 
			    LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_abort);
			prev_lxbq = (LXBQ *)
			    LGK_PTR_FROM_OFFSET(lgd->lgd_lxb_abort.lxbq_prev);
			prev_lxbq->lxbq_next = lgd->lgd_lxb_abort.lxbq_prev = 
			    LGK_OFFSET_FROM_PTR(&handle_lxb->lxb_lgd_abort);
			lgd->lgd_lxb_aborts++;
			(VOID)CSv_semaphore(&lgd->lgd_mutex);

			LGcp_resume(&handle_lpb->lpb_force_abort_cpid);
		    }
		    else
		    {
#ifdef LG_SHARED_DEBUG
			TRdisplay("%@ cleanup_process %d ABORT PID %d on handle LXB %x sLXB %x removed from sLXB\n",
			    LG_my_pid, handle_lpb->lpb_pid,
			    *(u_i4*)&handle_lxb->lxb_id, *(u_i4*)&slxb->lxb_id);
#endif /* LG_SHARED_DEBUG */

			/*
			** Remove handle from SHARED transaction and convert it to
			** a "normal" non-shared LXB which, because it's NONPROTECT,
			** will be deallocated below.
			*/
			handle_lxb->lxb_status &= ~LXB_SHARED_HANDLE;
			handle_lxb->lxb_shared_lxb = 0;
			
			next_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_handles.lxbq_next);
			prev_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_handles.lxbq_prev);
			next_lxbq->lxbq_prev = handle_lxb->lxb_handles.lxbq_prev;
			prev_lxbq->lxbq_next = handle_lxb->lxb_handles.lxbq_next;

			/* If not the entry LXB, deallocate it */
			if ( handle_lxb != lxb )
			    cleanup_lxb(lgd, ldb, &handle_lxb, abort_all, &db_undo_recover);
			    /* now handle_lxb == NULL */

			/* If no handles remain, unSHARE the sLXB to make available for recovery */
			if ( --slxb->lxb_handle_count == 0 )
			{
			    LPD	*slpd = (LPD *)LGK_PTR_FROM_OFFSET(slxb->lxb_lpd);
			    LPB *slpb = (LPB *)LGK_PTR_FROM_OFFSET(slpd->lpd_lpb);

			    slxb->lxb_status &= ~LXB_SHARED;

#ifdef LG_SHARED_DEBUG
			    TRdisplay("%@ cleanup_process %d ABORT PID %d sLXB %x unSHARED\n",
				LG_my_pid, lpb->lpb_pid,
				*(u_i4*)&slxb->lxb_id);
#endif /* LG_SHARED_DEBUG */

			    /* Removed SHARED LXB from LGD list */
			    (VOID)CSp_semaphore(SEM_EXCL, &lgd->lgd_lxbb_mutex);
			    next_lxbq = 
				(LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_slxb.lxbq_next);
			    prev_lxbq = 
				(LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_slxb.lxbq_prev);
			    next_lxbq->lxbq_prev = slxb->lxb_slxb.lxbq_prev;
			    prev_lxbq->lxbq_next = slxb->lxb_slxb.lxbq_next;
			    (VOID)CSv_semaphore(&lgd->lgd_lxbb_mutex);

			    /* If SHARED LXB belongs to another process, clean it up */
			    if ( slpb != lpb )
				cleanup_lxb(lgd, ldb, &slxb, abort_all, &db_undo_recover);
			    /* Returns with sxlb = NULL */
			}
		    }
		    /* Unmutex the handle_lxb */
		    if ( handle_lxb && handle_lxb != lxb )
			(VOID)CSv_semaphore(&handle_lxb->lxb_mutex);
		}
		/* Unmutex the sLXB */
		if ( slxb && slxb != lxb )
		    (VOID)CSv_semaphore(&slxb->lxb_mutex);
	    }

	    /* Skip SHARED LXBs which still have handles attached */
	    if ( lxb->lxb_status & LXB_SHARED )
	    {
#ifdef LG_SHARED_DEBUG
		TRdisplay("%@ cleanup_process %d ABORT PID %d SHARED LXB %x still has %d handles, skipped\n",
		    LG_my_pid, lpb->lpb_pid,
		    *(u_i4*)&lxb->lxb_id, lxb->lxb_handle_count);
#endif /* LG_SHARED_DEBUG */

		(VOID)CSv_semaphore(&lxb->lxb_mutex);
		continue;
	    }

	    /* Either mark the LXB for recovery or deallocate it */
	    cleanup_lxb(lgd, ldb, &lxb, abort_all, &db_undo_recover);

	}   /* End of while for all the LXBs of the given LPD */

	if (abort_all)
	{
	    /*
	    ** Since the entire logging system is shutting down, clean up
	    ** this process-database connection, and clean up the database
	    ** block if there are no more connections left to it.
	    */
	    if (--ldb->ldb_lpd_count == 0)
	    {
		/*  Deallocate the LDB. */

		(VOID)CSp_semaphore(SEM_EXCL, &lgd->lgd_ldb_q_mutex);
		next_ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb->ldb_next);
		prev_ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb->ldb_prev);
		next_ldb->ldb_prev = ldb->ldb_prev;
		prev_ldb->ldb_next = ldb->ldb_next;
		(VOID)CSv_semaphore(&lgd->lgd_ldb_q_mutex);

		/*
		** Deallocating the LDB decrements the inuse count
		** and frees the ldb_mutex.
		*/
		LG_deallocate_cb(LDB_TYPE, (PTR)ldb);
		/* 
		** Note that the LDB is now gone.
		*/
		ldb = (LDB *)NULL;
	    }

	    /*  Deallocate the LPD. */

	    lpd_offset = lpd->lpd_next;

	    next_lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd->lpd_next);
	    prev_lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd->lpd_prev);
	    next_lpd->lpd_prev = lpd->lpd_prev;
	    prev_lpd->lpd_next = lpd->lpd_next;

	    /*
	    ** Deallocating the LPD decrements the inuse count.
	    */
	    LG_deallocate_cb(LPD_TYPE, (PTR)lpd);
	    lpb->lpb_lpd_count--;
	}
	else
	{
	    /*
	    ** If UNDO processing is required on this database, then
	    ** the process LPB and LDB structures as well as the LDB
	    ** are kept around.  The LDB and LPB will be cleaned up
	    ** in LG_end when the last transaction is recovered during
	    ** RCP recovery.  The database will be marked closed by
	    ** the RCP at the end of recovery processing.
	    **
	    ** If REDO processing is required, the database has been
	    ** marked for recovery by turning on the LDB_RECOVER status.
	    ** This will prevent new servers from adding the database
	    ** until recovery is complete.
	    **
	    ** If no REDO or UNDO processing is needed, then the RCP
	    ** can be signaled to close the database.
	    */
	    if (db_undo_recover == FALSE)
	    {
		if (db_redo_recover == FALSE)
		{
		    lpd_offset = lpd->lpd_next;

		    if (--ldb->ldb_lpd_count == 0)
		    {
			if ((ldb->ldb_status & LDB_NOTDB) == 0)
			{
			    if (ldb->ldb_status & LDB_JOURNAL &&
				ldb->ldb_j_first_la.la_block)
			    {
				ldb->ldb_status |= LDB_PURGE;
				LG_signal_check();
			    }
			    else
			    {
				ldb->ldb_status &= ~LDB_OPENDB_PEND;
				ldb->ldb_status &= ~LDB_OPN_WAIT;
				ldb->ldb_status |= LDB_CLOSEDB_PEND;
				LG_signal_event(LGD_CLOSEDB, 0, FALSE);
			    }
			}
		    }

		    /*  Deallocate the LPD. */
		    next_lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd->lpd_next);
		    prev_lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd->lpd_prev);
		    next_lpd->lpd_prev = lpd->lpd_prev;
		    prev_lpd->lpd_next = lpd->lpd_next;

		    /*
		    ** Deallocating the LPD decrements the inuse count.
		    */
		    LG_deallocate_cb(LPD_TYPE, (PTR)lpd);

		    lpb->lpb_lpd_count--;
		}
		else
		{
		    lpd_offset = lpd->lpd_next;
		}
	    }
	    else
	    {
		proc_undo_recover = TRUE;
		lpd_offset = lpd->lpd_next;
	    }

	    if (db_redo_recover)
		proc_redo_recover = TRUE;
	}
	if (ldb)
	    (VOID)CSv_semaphore(&ldb->ldb_mutex);
    }

    if (abort_all)
    {
	/*	Force slave processes to exit. */

	if ((lpb->lpb_status & LPB_MASTER) == 0)
	{
	    /* Process may already be gone, ignore status */
	    (VOID)PCforce_exit(lpb->lpb_pid, &sys_err);
	}
	/*
	** Indicate that the LPB can now be cleaned up:
	*/
	proc_undo_recover = proc_redo_recover = FALSE;
    }
    else
    {
	/*
	** If recovery needed, signal RCP
	*/
	if (proc_undo_recover || proc_redo_recover)
	    LG_signal_event(LGD_RECOVER, 0, FALSE);
    }

    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lpb->lpb_lfb_offset);

    if (LG_my_pid != lpb->lpb_pid)
    {
	if (status = LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex))
	    return;

	if (lfb->lfb_wq_count)
	{
	    SIZE_TYPE	lbb_offset;

	    lbb_offset = lfb->lfb_wq_next;
	    while ( lbb_offset != LGK_OFFSET_FROM_PTR(&lfb->lfb_wq_next) )
	    {
		lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb_offset);
		lxb = (LXB *)LGK_PTR_FROM_OFFSET(lbb->lbb_owning_lxb);
		if (lxb && lxb->lxb_cpid.pid == lpb->lpb_pid)
		{
		    LG_unmutex(&lfb->lfb_wq_mutex);
#ifdef xDEBUG
		    TRdisplay("%@ RCP: Reassign buffer %d state %x\n", 
		    		lbb_offset, lbb->lbb_state);
#endif

		    if ( LG_mutex(SEM_EXCL, &lbb->lbb_mutex) )
			return;

		    /*
		    ** So write_complete() won't skip this buffer
		    ** if there are more buffers on write queue than
		    ** there are logwriter threads
		    */
		    lbb->lbb_state &= ~(LBB_PRIMARY_LXB_ASSIGNED |
		    			LBB_DUAL_LXB_ASSIGNED);

		    /* So we don't see it again */
		    lbb->lbb_owning_lxb = 0;
		    lbb->lbb_writers = 0;
		    
		    LG_queue_write(lbb, lpb, (LXB*)NULL, 0);
		    LG_unmutex(&lbb->lbb_mutex);
		    LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex);

		    /* Start at beginning again */
		    lbb_offset = lfb->lfb_wq_next;
		}
		else
		    lbb_offset = lbb->lbb_next;
	    }
	}

	(VOID)LG_unmutex(&lfb->lfb_wq_mutex);
    }

    if ((lfb->lfb_status & LFB_USE_DIIO) == 0 &&
	abort_all == 0 && lpb->lpb_gcmt_lxb != 0)
    {
	/*
	** If the process which exited had a group commit thread, and if
	** there was a buffer currently undergoing group commit, then this
	** process's group commit thread might have been responsible for
	** forcing the buffer to disk. Now that the process is dead, the
	** thread can no longer monitor the buffer, so cancel the group
	** commit processing and initiate the buffer write. Then tell the
	** world that we no longer have a group commit thread, so that
	** during recovery of this process other processes don't try to
	** request group commit services of us.
	*/
	while (lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb))
        {
	   (VOID)CSp_semaphore(SEM_EXCL, &lbb->lbb_mutex);

           if ( lbb == (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb) )
           {
	      /* current_lbb is still the "CURRENT" lbb. */
              break;
           }

           (VOID)LG_unmutex(&lbb->lbb_mutex);

#ifdef xDEBUG
           TRdisplay( "LGinit(1):: Scenario for INGSRV2838 handled. EOF=<%d, %d, %d>\n",
                      lfb->lfb_header.lgh_end.la_sequence,
                      lfb->lfb_header.lgh_end.la_block,
                      lfb->lfb_header.lgh_end.la_offset);
#endif
        }

	if (lbb->lbb_state & LBB_FORCE)
	{
	    /* LG_queue_write() will free lbb_mutex */
	    LG_queue_write(lbb, lpb, (LXB*)NULL, 0);
	}
	else
	    (VOID)LG_unmutex(&lbb->lbb_mutex);
    }
    lpb->lpb_gcmt_lxb = 0;

    /*
    ** If UNDO processing not necessary, clean up the process
    ** LPB.  If fast commit consistency point protocol is waiting
    ** for this server to respond then continue with the protocol.
    **
    ** NOTE : If REDO processing is required on this database, it
    ** is important that the ECP for a consistency point in progress
    ** not be written until redo recovery is complete.  Since we
    ** are deleting the LPB for this structure and going forward with
    ** consistency point protocol, we are depending on the fact that
    ** the RCP will not write the ECP until it has finished with REDO
    ** recovery.
    */

    if (proc_undo_recover == FALSE && proc_redo_recover == FALSE)
    {
	/*
	** No transactions in the ran away process needed to be
	** recovered. If the process was using a non-local LFB, clean
	** up the LFB block. Then extinguish the process block.
	*/

	if (lfb->lfb_status & LFB_USE_DIIO)
	{
	    /*
	    ** Note that we de-allocate the lfb's buffers, before
	    ** we de-allocate the LFB itself.
	    */

	    (VOID)CSp_semaphore(SEM_EXCL, &lfb->lfb_mutex);

	    LG_dealloc_lfb_buffers( lfb );
	    /*
	    ** Deallocating the LFB frees its mutex.
	    */
	    LG_deallocate_cb(LFB_TYPE, (PTR)lfb);
	}

#if defined(conf_CLUSTER_BUILD)
	/* If abort_all, no recovery has been done */
	if ( !abort_all && CXcluster_enabled() && LG_my_pid != lpb->lpb_pid)
	{
	    STATUS		msg_status;

	    msg_status = LG_rcvry_cmpl(
	    			lpb->lpb_pid, lpb->lpb_id.id_id);
	}
#endif

	/*  Deallocate the LPB. */
	(VOID)CSp_semaphore(SEM_EXCL, &lgd->lgd_lpb_q_mutex);
	next_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb->lpb_next);
	prev_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb->lpb_prev);
	next_lpb->lpb_prev = lpb->lpb_prev;
	prev_lpb->lpb_next = lpb->lpb_next;
	(VOID)CSv_semaphore(&lgd->lgd_lpb_q_mutex);

	/*
	** Deallocating the LPB decrements lgd_lpb_inuse
	** and frees the lpb_mutex.
	*/
	LG_deallocate_cb(LPB_TYPE, (PTR)lpb);

	if (!abort_all)
	{
	    /*
	    ** If shutdown requested and all that is left is the recovery
	    ** process, then signal the shutdown.
	    */
	    if ((lgd->lgd_lpb_inuse == 1) &&
		(lgd->lgd_status & LGD_START_SHUTDOWN))
	    {
		LG_signal_event(LGD_IMM_SHUTDOWN, 0, FALSE);
	    }
	    else
	    {
		/*
		** Check if removing this server allows us to proceed with
		** consistency point protocol.
		*/
		LG_last_cp_server();
	    }
	}
    }
    else
    {
	(VOID)CSv_semaphore(&lpb->lpb_mutex);
    }

    return;
}

/*
** Name: cleanup_lxb		Recover or remove an LXB.
**
** Descrption:
**	This function marks an LXB for recovery or removes
**	it from the logging system.
**	
**	It must be called with the LXB's mutex held and
**	the transaction's LDB mutex held.
**
**	Returns with LXB's mutex released, LDB mutex held.
**
** History:
**	15-Dec-1999 (jenjo02)
**	    Extracted from cleanup_process().
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Also remove LXB from
**	    LDB's active queue if on it.
*/
static void
cleanup_lxb( LGD *lgd, LDB *ldb, LXB **lxbp, i4 abort_all, bool *db_undo_recover )
{
    LXB		*lxb = *lxbp, *next_lxb, *prev_lxb;
    LPD		*lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    LPB		*lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
    LXBQ	*next_lxbq, *prev_lxbq;
    LXH		*next_lxh, *prev_lxh;

    /*
    ** Protected transactions need RCP recovery, even if
    ** just to inherit and release their lock lists.
    */
    if (!abort_all && lxb->lxb_status & LXB_PROTECT)
    {
#ifdef LG_SHARED_DEBUG
	TRdisplay("%@ cleanup_lxb %d ABORT PID %d LXB %x marked LXB_RECOVER\n",
	    LG_my_pid, lpb->lpb_pid,
	    *(u_i4*)&lxb->lxb_id);
#endif /* LG_SHARED_DEBUG */

	/* Protected transactions in this server need RCP recovery. */

	lxb->lxb_status |= LXB_RECOVER;
	*db_undo_recover = TRUE;
	(VOID)CSv_semaphore(&lxb->lxb_mutex);
    }
    else
    {
	/* Remove LXB from LPD queue. */
	/* protected by ldb_mutex */

	next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_owner.lxbq_next);
	prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_owner.lxbq_prev);
	next_lxbq->lxbq_prev = lxb->lxb_owner.lxbq_prev;
	prev_lxbq->lxbq_next = lxb->lxb_owner.lxbq_next;

	lpd->lpd_lxb_count--;
	ldb->ldb_lxb_count--;

	/*  Remove LXB from active transaction queues. */
	if (lxb->lxb_status & LXB_ACTIVE)
	{
	    (VOID)CSp_semaphore(SEM_EXCL, &lgd->lgd_lxb_q_mutex);
	    lgd->lgd_protect_count--;
	    /* count one less active, protected txn in this database */
	    ldb->ldb_lxbo_count--;

	    /* Remove LXB from active txn hash queue */
	    next_lxh = 
		(LXH *)LGK_PTR_FROM_OFFSET(lxb->lxb_lxh.lxh_lxbq.lxbq_next);
	    prev_lxh = 
		(LXH *)LGK_PTR_FROM_OFFSET(lxb->lxb_lxh.lxh_lxbq.lxbq_prev);

	    next_lxh->lxh_lxbq.lxbq_prev = 
		lxb->lxb_lxh.lxh_lxbq.lxbq_prev;
	    prev_lxh->lxh_lxbq.lxbq_next = 
		lxb->lxb_lxh.lxh_lxbq.lxbq_next;

	    /* Remove LXB from active txn queue */
	    next_lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_next);
	    prev_lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_prev);
	    next_lxb->lxb_prev = lxb->lxb_prev;
	    prev_lxb->lxb_next = lxb->lxb_next;

	    (VOID)CSv_semaphore(&lgd->lgd_lxb_q_mutex);
	}

	if ( lxb->lxb_active_lxbq.lxbq_next != 0 )
	{
	    /* Remove LXB from LDB's active queue */
	    next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_active_lxbq.lxbq_next);
	    prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_active_lxbq.lxbq_prev);
	    next_lxbq->lxbq_prev = lxb->lxb_active_lxbq.lxbq_prev;
	    prev_lxbq->lxbq_next = lxb->lxb_active_lxbq.lxbq_next;
	}

	/* Set xid slot "inactive" (0) in xid array */
	SET_XID_INACTIVE(lxb);

#ifdef LG_SHARED_DEBUG
	TRdisplay("%@ cleanup_lxb %d ABORT PID %d LXB %x being deallocated\n",
	    LG_my_pid, lpb->lpb_pid,
	    *(u_i4*)&lxb->lxb_id);
#endif /* LG_SHARED_DEBUG */

	/*
	** Deallocating the LXB decrements the inuse count
	** and frees the lxb_mutex.
	*/
	LG_deallocate_cb(LXB_TYPE, (PTR)lxb);
    }

    *lxbp = (LXB *)NULL;

    return;
}

/*{
** Name: LG_rcvry_cmpl - Broadcast dbms recovery completed message
**
** Description:
**      Called by RCP after completing recovery for dead dbms
**      Broadcast CSP_MSG_12_DBMS_RCVR_CMPL (including dbms pid)
**
** Inputs:
**      node				local node number
**      pid				(dead) dbms pid
**      lg_id_id			id_id from LG LPB for dead dbms
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**          none
**
** History:
**       11-may-2005 (stial01)
**          created.
**	 28-may-2005 (devjo01)
**	    moved here to resolve linkage issues.
*/
STATUS
LG_rcvry_cmpl(PID pid, i2 lg_id_id)
{
#if defined(conf_CLUSTER_BUILD)
    CSP_CX_MSG class_msg;
    i1		node;

    node = CXnode_number(NULL);
    class_msg.csp_msg_action = CSP_MSG_12_DBMS_RCVR_CMPL;
    class_msg.csp_msg_node = node;
    class_msg.class_event.pid = pid;
    class_msg.class_event.lg_id_id = lg_id_id;
    TRdisplay(ERx("%@ RCP: DBMS_RCVRD(%d) node %d pid %d lg_id %d\n"),
		  CSP_MSG_12_DBMS_RCVR_CMPL, node, pid, lg_id_id);
    return CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&class_msg );
#else
    return OK;
#endif
}
