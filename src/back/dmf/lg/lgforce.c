/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/*
NO_OPTIM = r64_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dm.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGFORCE.C - Implements the LGforce function of the logging system
**
**  Description:
**	This module contains the code which implements LGforce.
**	
**	    LGforce -- force logfile information to be flushed to disk.
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-oct-1992 (bryanp)
**	    Make LG_write_headers publically visible.
**	19-oct-1992 (bryanp)
**	    Disable group commit for now until the VMS CL can handle it.
**	20-oct-1992 (bryanp)
**	    Pick up group commit threshold from the lgd_gcmt_threshold value.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (bryanp/andys)
**	    Cluster 6.5 Project I
**	    In the new Recovery System, we track things mostly by LSN, not by
**		LG_LA. LGforce now operates using LSN's, not LGLA's.
**	    Renamed stucture members of LG_LA to new names. This means
**		replacing lga_high with la_sequence, and lga_low with la_offset.
**	    Include <tm.h> to pick up TM declarations.
**	    Re-instate group commit, and fix some group commit bugs. We were
**		failing to properly initialize the lpb_gcmt_sid field, so
**		sometimes we would try to awaken a bogus sid.
**	    In multi-node log situations, the group commit threads will only
**		process the local lfb, so we must ensure that we only activate
**		group commit for log page forces on the local LFB, and then
**		only when there exists at least one group commit thread in the
**		system.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	20-sep-1993 (bryanp)
**	    rcpconfig -disable_dual can call LG_write_headers with a zero lxid.
**	    In this circumstance, we need to find an LPB to pass to
**		LG_queue_write, so pass the master lpb address.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	31-jan-1994 (jnash)
**	    Lint directed changes.  In LG_force(), rename err_code to sys_err, 
**	    make a loc_err_code, initialize sys_err.
**      13-jun-1995 (chech02)
**          Added LGK_misc_sem semaphore to protect critical sections.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly
**	    named when calling LGMUTEX functions.
**	16-Jan-1996 (jenjo02)
**	    Explicitly identify CS_LOG_EVENT in CSsuspend() calls.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	08-Mar-1996 (jenjo02)
**	    When selecting a group commit thread, give master (RCP)
**	    precedence. This in an effort to offload some of the
**	    work on the dbms, useful in a multi-cpu environment.
**	03-jun-1996 (canor01)
**	    Removed LGK_misc_sem semaphore, since code section protected by LG mutex.
**	06-Dec-1996 (jenjo02)
**	    Allow LG_LAST requests to be handled by gcmt.
**	09-Jan-1997 (jenjo02)
**	    Before locking (and blocking) the current buffer and write queue,
**	    take a dirty peek at lfb_forced_lsn. If it meets the caller's  
**	    criteria, return LG_SYNCH post haste.
**	16-Jan-1997 (jenjo02)
**	    Fix a bug introduced with last change. Can't use LSN_LTE in place
**	    of LSN_LT when comparing target_lsn. If a log record is split
**	    across multiple buffers, the LSN will in the first buffer, which
**	    may have already been forced, but the remaining pieces of the
**	    log record may not yet have been forced.
**	11-Mar-1997 (jenjo02)
**	    Force write of buffer if all potential writers are waiting
**	    or if stalled on BCP.
**      08-Dec-1997 (hanch04)
**          Initialize new block field in LG_LA
**	    for support of logs > 2 gig
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Replace DIread/write with LG_READ/WRITE macros, DI_IO structures
**	    with LG_IO structures.
**	16-Nov-1998 (jenjo02)
**	    Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	19-Feb-1999 (jenjo02)
**	    A variety of repairs to ensure that we attach to the correct
**	    buffer, but never to a log header.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    lfb_current_lbb_mutex replaced by individual LBB's lbb_mutex.
**	26-Oct-1999 (jenjo02)
**	    If "target_lbb" is no longer on the write queue after waiting
**	    for its mutex, research the write queue for another candidate
**	    "target_lbb".
**	15-Dec-1999 (jenjo02)
**	    Add support for shared log transactions (LXB_SHARED/LXB_SHARED_HANDLE).
**	17-Mar-2000 (jenjo02)
**	    Release lxb_mutex early in LGforce (also LGwrite) to allow the 
**	    possibility of async write_complete without a resulting 
**	    lxb_mutex block.
**      29-Jun-2000 (horda03) X-Int of fix for bug 48904
**         30-mar-94 (rachael)
**            Added call to CSnoresnow to provide a temporary fix/workaround
**            for sites experiencing bug 48904. CSnoresnow checks the current
**            thread for being CSresumed too early.  If it has been, that is if 
**            scb->cs_mask has the EDONE bit set, CScancelled is called.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Replaced CScp_resume() with LGcp_resume macro.
**      12-Feb-2003 (hweho01)
**          Turned off optimizer for AIX hybrid build by using 
**          the 64-bit configuration string (r64_us5), because the    
**          error happens only with the 64-bit compiler option.  
**      11-Jun-2004 (horda03) Bug 112382/INGSRV2838
**          Ensure that the current LBB is stil "CURRENT" after
**          the lbb_mutex is acquired. If not, re-acquire.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	6-Apr-2007 (kschendel) SIR 122890
**	    Use an intermediate for aliasing external i4 id's and LG_ID.
**	    Originally this was to fix gcc -O2's strict aliasing, but
**	    Ingres has so many other strict-aliasing violations that
**	    -fno-strict-aliasing is needed.  The id/LG_ID thing was so
**	    egregious, though, that I've kept this change.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/

/*
** Forward declarations of static functions:
*/
static STATUS	LG_force(i4 flag, LG_ID *lx_id, LG_LSN *lsn,
			LFB **lfb, 
			STATUS *async_status,
			CL_ERR_DESC *sys_err);

static STATUS	queue_header_write( LFB *lfb );

/*{
** Name: LGforce	- Force log file.
**
** Description:
**      This routine guarantee's that a specific log record has been
**	safely forced to disk.  The flag arguments interpreted as
**	follows:
**	    LG_HDR -  Force the log file header to disk.
**	    LG_LAST - Force the last log page of the logging system to disk.
**	    0	    - Force the record denoted by the value in lsn.
**
**      This assumes an immediate write.  That is the call should
**      not return until the data is on disk.
**
** Inputs:
**	flag				Optional LG_HDR, LG_LAST or 0.
**	lx_id				Transaction identifier.
**	lsn				Optional pointer to specific log
**					record to guarantee is on disk.
**
** Outputs:
**	nlsn				Pointer to location to receive an LSN
**					    which has the property that ALL
**					    records with lower LSN's are known
**					    to be safely on disk already.
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**	    LG_BADPARAM		    Parameter(s) in error.
**	    LG_WRITEERROR	    Error writing log.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-feb-1993 (bryanp)
**	    Force by lsn.
**	16-Jan-1996 (jenjo02)
**	    Explicitly identify CS_LOG_EVENT in CSsuspend() calls.
**	25-Jan-1996 (jenjo02
**	    Mutex granularity project. Removed acquisition and
**	    release of singular lgd_mutex.
**	24-Aug-1999 (jenjo02)
**	    Set caller's "nlsn" after possibly waiting for it to
**	    be updated, rather than before.
*/
STATUS
LGforce(
i4		    flag,
LG_LXID		    lx_id,
LG_LSN		    *lsn,
LG_LSN		    *nlsn,
CL_ERR_DESC	    *sys_err)
{
    STATUS	status;
    STATUS	async_status = OK;
    i4		sleep_flags;
    LFB		*lfb;
    LG_I4ID_TO_ID xid;

#ifdef VMS
    /* Cancel any Rogue resumes. */
    CSnoresnow("LG!LGFORCE.C::LGforce_1", 1);
#endif

    xid.id_i4id = lx_id;
    do 
    {
	status = LG_force(flag, &xid.id_lgid, lsn, &lfb,
		    &async_status, sys_err);

	/* you can get 3 returns:
	**	LG_SYNCH	- completed without having to wait.
	**	LG_MUST_SLEEP	- could not get some resource so wait until it
	**			  is around and then redo the call.
	**	OK	- finished work but need to wait for an event.
	**			  when event arives then work is done.
	*/

	if (status == LG_SYNCH)
	{
	    status = OK;
	}
	else if (status == OK)
	{
	    CSsuspend(CS_LOG_MASK, 0, 0);

	    if (async_status)
		status = LG_WRITEERROR;
	    else
		status = OK;
	}
	else if (status == LG_MUST_SLEEP)
	{
	    CSsuspend(CS_LOG_MASK, 0, 0);
	}
    } while (status == LG_MUST_SLEEP);

    /* Return last forced LSN from log header, if wanted */ 
    if ( status == OK && nlsn )
	*nlsn = lfb->lfb_forced_lsn;

    return(status);
}

/*{
** Name: LG_force	- Force log file.
**
** Description:
**      Force any buffers containing the record described by the transaction
**	or log address to disk.
**
** Inputs:
**	flag				Optional LG_HDR, LG_LAST or 0.
**      lx_id                           Transaction identifier.
**	clfb				Place to return LFB pointer.
** Outputs:
**	nlsn				Current forced log sequence number
**	sys_err				Reason for error
**	Returns:
**	    OK					Successful.
**	    LG_BADPARAM				Bad parameters.
**	    LG_DB_INCONSISTENT			Inconsistent database.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	20-oct-1992 (bryanp)
**	    Pick up group commit threshold from the lgd_gcmt_threshold value.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**	    Force-by-lsn.
**	    Substantial changes for non-local logfile processing. Isolate log
**		information into new LFB control block.
**	    In multi-node log situations, the group commit threads will only
**		process the local lfb, so we must ensure that we only activate
**		group commit for log page forces on the local LFB, and then
**		only when there exists at least one group commit thread in the
**		system.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	31-jan-1994 (jnash)
**	    Rename err_code to sys_err, make a loc_err_code, initialize 
**	    sys_err.
**	25-Jan-1996 (jenjo02)
**	    Mutex granualrity project. Many new semaphores take the
**	    place of singular lgd_mutex.
**	09-Jan-1997 (jenjo02)
**	    Before locking (and blocking) the current buffer and write queue,
**	    take a dirty peek at lfb_forced_lsn. If it meets the caller's  
**	    criteria, return LG_SYNCH post haste.
**	16-Jan-1997 (jenjo02)
**	    Fix a bug introduced with last change. Can't use LSN_LTE in place
**	    of LSN_LT when comparing target_lsn. If a log record is split
**	    across multiple buffers, the LSN will in the first buffer, which
**	    may have already been forced, but the remaining pieces of the
**	    log record may not yet have been forced.
**	11-Mar-1997 (jenjo02)
**	    Force write of buffer if all potential writers are waiting
**	    or if stalled on BCP.
**	19-Feb-1999 (jenjo02)
**	    A variety of repairs to ensure that we attach to the correct
**	    buffer, but never to a log header.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    lfb_current_lbb_mutex replaced by individual LBB's lbb_mutex.
**	    Return pointer to LFB, from which lfb_forced_lsn can be
**	    extracted after possibly waiting.
**	    If forcing to a specific LSN, we need only to wait until that
**	    LSN and prior LSNs have hit the disk.
**	19-Sep-1999 (jenjo02)
**	    The "target_lbb" must be mutexed before adding this LXB
**	    to its wait queue.
**	    lfb_current_lbb_mutex replaced by individual LBB's lbb_mutex.
**	26-Oct-1999 (jenjo02)
**	    If "target_lbb" is no longer on the write queue after waiting
**	    for its mutex, research the write queue for another candidate
**	06-May-2003 (jenjo02)
**	    Check that current_lbb has something in it before assuming
**	    its LSN is valid lest we write an empty log record and
**	    corrupt the log; B110192.
**	15-Mar-2006 (jenjo02)
**	    o Exclude lbb_commit_count from lgd_protect_count when
**	      deciding whether to write the buffer -now- or later.
**	      Those txn's which have committed in this buffer
**	      won't be doing any more writes, even though they're
**	      still counted in lgd_protect_count.
**	    o If local gcmt thread is busy, try the one in the RCP.
**	    o lxb_wait_reason defines moved to lg.h
**	20-Nov-2006 (jonj)
**	    "target_lbb" must include those which include LSNs which are
**	    greater than -or equal- to the target LSN as large log records
**	    may span more than just one page and we must wait for the entire
**	    LSN to hit the platter.
**       8-Jan-2006 (hanal04) Bug 117442
**          Correct equals assignment that should have been a test for 
**          equality. This lead to a performance hit as we spun round the
**          gcmt_offset loop taking and releasing the LG LXB mutex.
**	07-Sep-2007 (jonj)
**	    BUG 119091 SD 121503
**	    When picking a group commit thread, note that in the RCP
**	    master_lpb's gcmt_lxb will be the same as lpb's gcmt_lxb.
**	    Correct type of gcmt_offset is SIZE_TYPE, not i4.
**	06-Nov-2007 (jonj)
**	    When bailing out on a mutex error, be sure to unwind other
**	    held mutexes to alleviate mutex lockups in LG_rundown and
**	    transaction recovery.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: relocate "force" stat counts, keep going
**	    until target_lsn is completely forced.
**
*/
static STATUS
LG_force(
i4		    flag,
LG_ID		    *lx_id,
LG_LSN		    *lsn,
LFB		    **clfb,
STATUS		    *async_status,
CL_ERR_DESC	    *sys_err)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXB    *lxb, *handle_lxb;
    LPD		    *lpd;
    LDB		    *ldb;
    LPB		    *lpb;
    register LFB    *lfb;
    LPB		    *master_lpb;
    SIZE_TYPE	    end_offset;
    i4	    lbb_offset;
    LG_LSN	    target_lsn;
    LBB    	    *target_lbb;
    LBB    	    *lbb;
    LBB    	    *current_lbb;
    SIZE_TYPE	    *lxbb_table;
    i4	    loc_err_code;
    STATUS	    status, return_status;

    LG_WHERE("LG_force")

    CL_CLEAR_ERR(sys_err);

    /*	Check the lx_id. A zero transaction ID is no longer supported. */

    if ((i4)lx_id->id_id > lgd->lgd_lxbb_count || lx_id->id_id == 0)
    {
	uleFormat(NULL, E_DMA418_LGFORCE_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &loc_err_code, 2,
		    0, lx_id->id_id, 0, lgd->lgd_lxbb_count);
	return (LG_BADPARAM);
    }

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    handle_lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id->id_id]);

    if (handle_lxb->lxb_type != LXB_TYPE ||
	handle_lxb->lxb_id.id_instance != lx_id->id_instance)
    {
	uleFormat(NULL, E_DMA419_LGFORCE_BAD_XACT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &loc_err_code, 3,
		    0, handle_lxb->lxb_type, 
		    0, handle_lxb->lxb_id.id_instance,
		    0, lx_id->id_instance);
	return (LG_BADPARAM);
    }

    if (status = LG_mutex(SEM_EXCL, &handle_lxb->lxb_mutex))
	return(status);

    /* Check if the database the transaction running against is inconsistent. */

    lpd = (LPD *)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_lpd);
    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
    *clfb = lfb = (LFB *)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_lfb_offset);

    if (handle_lxb->lxb_status & LXB_PROTECT &&
	ldb->ldb_status & LDB_INVALID )
    {
	handle_lxb->lxb_status |= LXB_DBINCONST;
	(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	return(LG_DB_INCONSISTENT);
    }

    /*	Check for special operation on log file header. */

    if (flag & LG_HDR)
    {
	return_status = LG_write_headers( lx_id, lfb, async_status );
	(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	return(return_status);
    }

    /*
    ** If LXB is a handle to a SHARED LXB, then use the actual shared
    ** LXB for the force request. Remember the handle LXB as it will be
    ** the one suspended if need arises.
    **
    ** The HANDLE LXBs:
    **		o are flagged NON-PROTECT, are not recoverable
    **		o have affinity to a thread within a process
    **		o may be used to write log buffers
    **	 	o may be suspended/resumed
    **		o never appear on the active txn / hash queues
    **		o contains no log space reservation info
    **
    ** The SHARED LXB:
    **		o is flagged PROTECT and is recoverable
    **		o has no thread or process affinity
    **	  	o is never used for I/O
    **		o contains all the LSN/LGA info about the
    **		  transaction.
    **		o is never suspended
    **		o appears on the active txn and hash queues
    **		o contains log space reservation info
    */
    lxb = handle_lxb;
    if ( handle_lxb->lxb_status & LXB_SHARED_HANDLE )
    {
	if ( handle_lxb->lxb_shared_lxb == 0 )
	{
	    LG_I4ID_TO_ID xid;
	    xid.id_lgid = handle_lxb->lxb_id;
	    uleFormat(NULL, E_DMA493_LG_NO_SHR_TXN, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &loc_err_code, 3,
			0, xid.id_i4id,
			0, handle_lxb->lxb_status,
			0, "LG_force");
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return (LG_BADPARAM);
	}

	lxb = (LXB *)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_shared_lxb);

	if ( lxb->lxb_type != LXB_TYPE ||
	    (lxb->lxb_status & LXB_SHARED) == 0 )
	{
	    LG_I4ID_TO_ID xid1, xid2;
	    xid1.id_lgid = lxb->lxb_id;
	    xid2.id_lgid = handle_lxb->lxb_id;
	    uleFormat(NULL, E_DMA494_LG_INVALID_SHR_TXN, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &loc_err_code, 6,
			0, lxb->lxb_type,
			0, LXB_TYPE,
			0, xid1.id_i4id,
			0, lxb->lxb_status,
			0, xid2.id_i4id,
			0, "LG_force");
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return (LG_BADPARAM);
	}
	if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
	{
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return(status);
	}
	/* Both the handle and shared LXBs are now mutexed */
    }

    if ((flag & LG_LAST) == 0)
    {
	/*
	** If not forcing the last log page of the logging system,
	** target_lsn will be the value specified by the caller
	** or the last LSN of this transaction.
	*/
	if (lsn)
	{
	    target_lsn = *lsn;
	    handle_lxb->lxb_wait_lsn = *lsn;
	}
	else
	{
	    target_lsn = lxb->lxb_last_lsn;
	    handle_lxb->lxb_wait_lsn = lxb->lxb_first_lsn;
	}

	/*
	** Make a quick and dirty check that the requested LSN
	** has already been forced.
	*/
	if (LSN_LT(&target_lsn, &lfb->lfb_forced_lsn))
	{
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return (LG_SYNCH);
	}
    }

    /*
    ** Lock the write queue and current buffer,
    ** assuring that they'll stay in a constant state until
    ** we're done with the force.
    */

    while( current_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb))
    {
        if (status = LG_mutex(SEM_EXCL, &current_lbb->lbb_mutex))
        {
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return(status);
	}

        if ( current_lbb == (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb))
        {
	    /* current_lbb is still the "CURRENT" lbb. */
            break;
        }

        (VOID)LG_unmutex(&current_lbb->lbb_mutex);

#ifdef xDEBUG
        TRdisplay( "LG_force(1):: Scenario for INGSRV2838 handled. EOF=<%d, %d, %d>\n",
                  lfb->lfb_header.lgh_end.la_sequence,
                  lfb->lfb_header.lgh_end.la_block,
                  lfb->lfb_header.lgh_end.la_offset);
#endif
    }

    if (status = LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex))
    {
        (VOID)LG_unmutex(&current_lbb->lbb_mutex);
	(VOID)LG_unmutex(&lxb->lxb_mutex);
	if ( handle_lxb != lxb )
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	return(status);
    }

    if (flag & LG_LAST)
    {
	target_lsn.lsn_high = 0;
	target_lsn.lsn_low = 0;

	if ( current_lbb->lbb_state & LBB_CURRENT &&
	     current_lbb->lbb_last_lsn.lsn_high )
	{
	    target_lsn = current_lbb->lbb_last_lsn;
	}
	else if (lfb->lfb_wq_count)
	{
	    /*
	    ** Find the LBB waiting to be written that has the highest log 
	    ** sequence number. Skip the log header if encountered.
	    */
	    end_offset = LGK_OFFSET_FROM_PTR(&lfb->lfb_wq_next);
	    for (lbb_offset = lfb->lfb_wq_next;
		lbb_offset != end_offset;
		lbb_offset = lbb->lbb_next)
	    {
		lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb_offset);

		if (lbb->lbb_lga.la_block &&
		    LSN_GT(&lbb->lbb_forced_lsn, &target_lsn))
		{
		    target_lsn = lbb->lbb_forced_lsn;
		}
	    }
	}

	if (target_lsn.lsn_high == 0)
	    target_lsn = lxb->lxb_last_lsn;

	/*
	** Force all pages from lxb_first_lsn thru target_lsn.
	**
	** For an active transaction, lxb_first_lsn will be the
	** LSN of the first log record written by that transaction. 
	**
	** For an inactive transaction, lxb_first_lsn will be
	** zero, inducing a flush of all "gprev" log buffers,
	** just the behavior we want.
	*/

	handle_lxb->lxb_wait_lsn.lsn_high = 0;
	handle_lxb->lxb_wait_lsn.lsn_low = 0;
    }

    /* Release the SHARED LXB's mutex, retain the handle */
    if ( handle_lxb != lxb )
	(VOID)LG_unmutex(&lxb->lxb_mutex);

    /* Note that lfb_forced_lsn is protected by lfb_wq_mutex. */

    /*	Check if target LSN is less than the last forced LSN */

    if (LSN_LT(&target_lsn, &lfb->lfb_forced_lsn))
    {
	/* ... then we're good to go */
	(VOID)LG_unmutex(&lfb->lfb_wq_mutex);
	(VOID)LG_unmutex(&current_lbb->lbb_mutex);
	(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	return (LG_SYNCH);
    }

    /*  Change various counters. */

    handle_lxb->lxb_stat.force++;
    lgd->lgd_stat.force++;
    ldb->ldb_stat.force++;
    lpb->lpb_stat.force++;
    lfb->lfb_stat.force++;

    /*
    **	Our requirement is that we need to suspend the caller until all log
    **	records up to and including the log record indicated by "target_lsn"
    **	are safely on disk.
    **
    **	There may be several buffers of log data with
    **	lower LSNs on the write queue, as well as potentially a current buffer
    **	with LSNs <= our target LSN. If there are any unwritten buffers
    **	containing log records which need forcing, we wish to find the specific
    **	buffer among those buffers which has the greatest LSN. We then attach
    **	ourselves to this buffer.
    **
    **	Eventually, this buffer will hit disk, and the "greatest prior wait
    **	queue adoption" logic in LGwrite will transfer us to the next prior
    **	buffer, and then to the next prior, until all prior buffers have
    **	successfully hit disk, at which point our caller will be resumed and
    **	can proceed with his work.
    */

    /*  
    ** Note that we still hold EXCL mutexes on current_lbb's lbb_mutex
    ** and the write queue.
    */

    if ( current_lbb->lbb_state & LBB_CURRENT &&
	 current_lbb->lbb_first_lsn.lsn_high &&
	 LSN_LTE(&current_lbb->lbb_first_lsn, &target_lsn) )
    {
	/* We won't be needing the wq, so unlock it */
	(VOID)LG_unmutex(&lfb->lfb_wq_mutex);

	/*
	** The current buffer contains at least one log record with an LSN
	** less than or equal to the target LSN.
	**
	** Mark the buffer with force write request (unless already
	** done by a previous force call).  The buffer will
	** be written out when it becomes full or when the timer
	** has expired.
	**
	** If there is only one active (protected) transaction
	** in the logging system, then queue the log write immediately
	** rather waiting for piggyback commit.  No use in doing
	** group commit if there is only one write transaction active.
	**
	** The group commit threads in the system are all bound to the
	** local log, so we only initiate group commit if this page is
	** a local log file page. Non-local logfile access (cluster recovery
	** access) never uses group commit.
	**
	** When selecting a group commit thread to oversee the buffer during
	** group commit, we pick the thread in this process if this process
	** has a group commit thread; otherwise we pick the RCP's group
	** commit thread. The primitive nature of this selection algorithm
	** means that there may be times when we decide not to initiate
	** group commit for a particular buffer even though group commit
	** threads exist in the logging system (just neither in this
	** process nor in the RCP). To ensure that group commit is always
	** used, the RCP should be started with a group commit thread.
	*/

	/* We must wait for this page to hit the disk */
	handle_lxb->lxb_wait_reason = LG_WAIT_FORCE;
	LG_suspend_lbb(handle_lxb, current_lbb, async_status);

	/*
	** Release the LXB mutex now so that async completion
	** has a chance to work without blocking on the mutex.
	*/
	(VOID)LG_unmutex(&handle_lxb->lxb_mutex);

	/* Assume we will have to wait for I/O completion */
	return_status = OK;

	/* 
	** Whether already marked for FORCE or not,
	** if all potential writers to the buffer (active, protected
	** transactions) are now waiting for the buffer to be written, 
	** write it now rather than waiting for
	** the group commit thread to wake up and notice.
	*/

	if ( current_lbb->lbb_wait_count >= 
		lgd->lgd_protect_count - current_lbb->lbb_commit_count )
	{
	    /* LG_queue_write() will free current LBB's mutex */
	    return_status = LG_queue_write(current_lbb, lpb, handle_lxb, 0);
	}
	else if ((current_lbb->lbb_state & LBB_FORCE) == 0)
	{
	    master_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb);

	    /*
	    ** If this is the local log file, and if there exists a group
	    ** commit thread able to process this buffer, and if there are
	    ** at least lgd_gcmt_threshold number of protected transactions,
	    ** then initiate group commit processing for this buffer.
	    */
	    if (lgd->lgd_protect_count > lgd->lgd_gcmt_threshold &&
		lfb == &lgd->lgd_local_lfb &&
		(lpb->lpb_gcmt_lxb != 0 || master_lpb->lpb_gcmt_lxb != 0))
	    {
		SIZE_TYPE	gcmt_offset;

		current_lbb->lbb_state |= LBB_FORCE;

		(VOID)LG_unmutex(&current_lbb->lbb_mutex);

		if ((gcmt_offset = lpb->lpb_gcmt_lxb) == 0)
		     gcmt_offset = master_lpb->lpb_gcmt_lxb;

		/*
		** Awaken the GCMT thread if it's asleep, 
		** otherwise, try the other.
		*/
		while (gcmt_offset)
		{
		    LXB	*gcmt_lxb = (LXB*)LGK_PTR_FROM_OFFSET(gcmt_offset);
		    if (status = LG_mutex(SEM_EXCL, &gcmt_lxb->lxb_mutex))
		    {
			(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
			return(status);
		    }
		    if (gcmt_lxb->lxb_status & LXB_WAIT)
		    {
			gcmt_lxb->lxb_status &= ~LXB_WAIT;

			LGcp_resume(&gcmt_lxb->lxb_cpid);
			gcmt_offset = 0;
		    }
		    else
		    {
			/* In the RCP, these will be the same */
			if ( gcmt_offset == lpb->lpb_gcmt_lxb &&
			     gcmt_offset != master_lpb->lpb_gcmt_lxb )
			{
			    gcmt_offset = master_lpb->lpb_gcmt_lxb;
			}
			else
			    gcmt_offset = 0;
		    }
		    (VOID)LG_unmutex(&gcmt_lxb->lxb_mutex);
		}
	    }
	    else
		/* LG_queue_write() will free lbb_mutex */
		return_status = LG_queue_write(current_lbb, lpb, handle_lxb, 0);
	}
	else
	{
	    /* 
	    ** Current buffer already marked for force.
	    ** Return to let txn wait for its write to complete.
	    */
	    (VOID)LG_unmutex(&current_lbb->lbb_mutex);
	}
	return(return_status);
    }

    /* Unlock current LBB */
    (VOID)LG_unmutex(&current_lbb->lbb_mutex);

    /*
    ** Either there was no current buffer or the current buffer did not
    ** contain the desired log sequence number.
    ** 
    ** Check each of the LBB's already on the write queue, which is still 
    ** locked. If there are any LBB's which have LSN's that are
    ** less than or equal to our target LSN, then we wish to remember the
    ** address of the buffer with the highest such lsn.
    **
    ** Ignore the log header if encountered!
    **
    ** If we find such a buffer, we'll have our caller
    ** wait for it to hit disk. Due to the way that buffer wait queues are
    ** managed in LGwrite, this will also wait for all prior buffers to
    ** hit disk, which is exactly the semantics we desire.
    */

    /* We still hold a lock on the write queue */

    /* Keep going until our target is completely on disk */

    while ( lfb->lfb_wq_count && LSN_GTE(&target_lsn, &lfb->lfb_forced_lsn) )
    {
	target_lbb = (LBB *)NULL;

	end_offset = LGK_OFFSET_FROM_PTR(&lfb->lfb_wq_next);

	for (lbb_offset = lfb->lfb_wq_next;
	    lbb_offset != end_offset;
	    lbb_offset = lbb->lbb_next)
	{
	    lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb_offset);

	    if (lbb->lbb_lga.la_block &&
		LSN_LTE(&lbb->lbb_first_lsn, &target_lsn))
	    {
		/*
		** this buffer contains log records which we need to wait 
		** for. If this is the first such buffer we've seen in 
		** this search, or if contains HIGHER or EQUAL log sequence numbers 
		** than any previous such log buffer we've seen before,
		** then designate it the target lbb.
		**
		** At the end of this loop, target_lbb, if non zero, is 
		** the lbb with the highest log sequence numbers which are 
		** less than the log sequence numbers we're interested in.
		*/
		if (target_lbb == (LBB *)NULL ||
		    LSN_GTE(&lbb->lbb_first_lsn, &target_lbb->lbb_first_lsn))
		{
		    target_lbb = lbb;
		}
	    }
	}

	/*
	** If a buffer was found then queue the LXB to this buffer.
	** The buffer is already on the write queue, so we don't need to queue it.
	** The writer will restart the LXB after the page (and all prior pages)
	** have made it to disk.
	*/
	
	if (target_lbb)
	{
	    /* 
	    ** Unlock the write queue,
	    ** lock the target_lbb's lbb_mutex
	    ** and recheck the LSN info.
	    ** By the time we get the lbb_mutex, the buffer
	    ** may have been written, in which case we must attempt to
	    ** find a new target_lbb.
	    */
	    (VOID)LG_unmutex(&lfb->lfb_wq_mutex);
	    LG_mutex(SEM_EXCL, &target_lbb->lbb_mutex);

	    if ( target_lbb->lbb_state & LBB_ON_WRITE_QUEUE &&
		 target_lbb->lbb_lga.la_block &&
		    LSN_LTE(&target_lbb->lbb_first_lsn, &target_lsn) )
	    {
		handle_lxb->lxb_wait_reason = LG_WAIT_FORCE;
		LG_suspend_lbb(handle_lxb, target_lbb, async_status);
		(VOID)LG_unmutex(&target_lbb->lbb_mutex);
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		return(OK);
	    }
	    /* target_lbb's been written, rescan write queue */
	    (VOID)LG_unmutex(&target_lbb->lbb_mutex);
	    LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex);
	}
	else
	    break;
    }

    (VOID)LG_unmutex(&lfb->lfb_wq_mutex);
    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
    return (LG_SYNCH);
}

/*
** Name: LG_write_headers	    - write the log file headers
**
** Description:
**	This routine writes the log file headers. If a 'normal' LBB is
**	available, we fill the normal LBB with the log file headers, checksum
**	them, and call queue_write() to submit the LBB to be written. If
**	all the LBB's are in use (or if none have yet been allocated), we
**	use a special LBB which is reserved for this purpose -- since it is
**	not allocated and freed like normal LBB's, we use the special AST
**	routine 'LG_inline_hdr_write' to write this LBB.
**
** Inputs:
**	lx_id		    logging system identifier
**	async_status	    AST parameter for suspending.
**
** Outputs:
**	None
**
** Returns:
**	STATUS
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	20-sep-1993 (bryanp)
**	    rcpconfig -disable_dual can call LG_write_headers with a zero lxid.
**	    In this circumstance, we need to find an LPB to pass to
**		LG_queue_write, so pass the master lpb address.
**	25-Jan-1996 (jenjo02)
**	    Mutex granularity project. lxb_mutex (if lx_id
**	    passed) must be held on entry to function.
**	13-Aug-2001 (jenjo02)
**	    Set lbb_state to LBB_MODIFIED before initiating write
**	    of header.
**	21-Mar-2006 (jenjo02)
**	    Maintain (new) lfb_fq_leof.
**	20-Sep-2006 (kschendel)
**	    Buffer wait queue moved to LFB, update here.
*/
STATUS
LG_write_headers( LG_ID *lx_id, register LFB *lfb, STATUS *async_status )
{
    register LGD   *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LBB   	   *lbb;
    register LXB   *lxb;
    SIZE_TYPE 	   *lxbb_table;
    LBB	    	   *next_lbb;
    LBB	    	   *prev_lbb;
    LPB	    	   *lpb;
    LPD	    	   *lpd;
    char    	   *buf_ptr;
    STATUS   	   status;
    LBB		   *current_lbb;

    if (lx_id->id_id != 0 || lx_id->id_instance != 0)
    {
	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id->id_id]);

	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id->id_instance)
	{
	    return (LG_BADPARAM);
	}

	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
    }
    else
    {
	lxb = (LXB *)NULL;
	lpd = (LPD *)NULL;
	/*
	** It's important to have some lpb, even if it's not our lpb, to pass
	** to LG_queue_write, so if we don't have a local LPB handy, pass the
	** master LPB in.
	*/
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb);
    }

    /*  Wait for a free buffer if none available. */

    if (status = LG_mutex(SEM_EXCL, &lfb->lfb_fq_mutex))
	return(status);
    if (lfb->lfb_fq_count == 0)
    {
	if (lxb)
	{
	    lxb->lxb_wait_reason = LG_WAIT_FREEBUF;
	    LG_suspend(lxb, &lfb->lfb_wait_buffer, async_status);
	    (VOID)LG_unmutex(&lfb->lfb_fq_mutex);
	    return (LG_MUST_SLEEP);
	}
	else
	{
	    (VOID)LG_unmutex(&lfb->lfb_fq_mutex);
	    return ( queue_header_write( lfb ) );
	}
    }

    lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_fq_next);

    next_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb->lbb_next);
    prev_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb->lbb_prev);
    next_lbb->lbb_prev = lbb->lbb_prev;
    prev_lbb->lbb_next = lbb->lbb_next;

    /* If this LBB is logical EOF, reset leof to last on fq */
    if ( lfb->lfb_fq_leof == LGK_OFFSET_FROM_PTR(lbb) )
	lfb->lfb_fq_leof = lfb->lfb_fq_prev;

    lfb->lfb_fq_count--;

    (VOID)LG_unmutex(&lfb->lfb_fq_mutex);

    /* Lock the current_lbb to block updates to the header */
    while (current_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb))
    {
       if (status = LG_mutex(SEM_EXCL, &current_lbb->lbb_mutex))
	   return(status);

       if ( current_lbb == (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb) )
       {
	  /* current_lbb is still the "CURRENT" lbb. */
          break;
       }

       (VOID)LG_unmutex(&current_lbb->lbb_mutex);

#ifdef xDEBUG
       TRdisplay( "LG_write_headers():: Scenario for INGSRV2838 handled. EOF=<%d, %d, %d>\n",
                  lfb->lfb_header.lgh_end.la_sequence,
                  lfb->lfb_header.lgh_end.la_block,
                  lfb->lfb_header.lgh_end.la_offset);
#endif
    }

    TMget_stamp((TM_STAMP *)lfb->lfb_header.lgh_timestamp);
    lfb->lfb_header.lgh_active_logs = lfb->lfb_active_log;
    lfb->lfb_header.lgh_checksum = 
	    LGchk_sum((PTR)&lfb->lfb_header, sizeof(LG_HEADER));

    /* 
    ** Write the log header on every 1K page in the first log 
    ** block for 2 times.
    */

    buf_ptr = (char *)LGK_PTR_FROM_OFFSET(lbb->lbb_buffer);

    MEmove(sizeof(LG_HEADER), (PTR)&lfb->lfb_header, '\0', 1024, buf_ptr);
    LG_unmutex(&current_lbb->lbb_mutex);
    MEmove(sizeof(LG_HEADER), (PTR)buf_ptr, '\0', 1024,
	    &buf_ptr[1024] );

    lbb->lbb_lga.la_sequence = 0;
    lbb->lbb_lga.la_block    = 0;
    lbb->lbb_lga.la_offset   = 0;
    lbb->lbb_state = LBB_MODIFIED;

    /* Lock the header LBB's mutex */
    if (status = LG_mutex(SEM_EXCL, &lbb->lbb_mutex))
	return(status);

    /* Must wait for header to hit the disk */
    if (lxb)
    {
	lxb->lxb_wait_reason = LG_WAIT_HDRIO;
	LG_suspend_lbb(lxb, lbb, async_status);
    }

    /* LG_queue_write() will release the mutex */
    return( LG_queue_write(lbb, lpb, lxb, 0) );
}

/*
** Name: queue_header_write	- queue the special header_lbb to the master
**
** Description:
**	Normally, a write to the log headers is performed just like a write
**	to the normal log data pages: an lbb is allocated, filled in with
**	information, and queued to the master with 'write_block' as the AST
**	function to call. However, there are two special circumstances when
**	this method cannot be used:
**	    1) a write error occurs and all the normal LBB's are in use.
**	    2) the headers need to be written, but the RCP is not up (and the
**		normal LBB memory does not exist). For example, when the RCP
**		is down, rcpconfig/nodual_logging can be invoked to disable
**		dual logging.
**
**	For these cases, write_headers calls this routine, which uses a 
**	special reserved LBB, called the lgd_header_lbb. There is only one of
**	these special LBB's. It is marked as 'in-use' once this routine runs
**	and is marked 'free' again once the headers get written.
**
**	Another critical difference between this routine and normal LBB usage
**	is that the header is copied into the buffer by the AST routine, not
**	by this routine.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	STATUS
**
** History:
**	30-aug-1990 (bryanp)
**	    Created.
**	25-Jan-1996 (jenjo02)
**	    Mutex granularity project. 
*/
static STATUS
queue_header_write( register LFB *lfb )
{
    register LGD	*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LBB	*header_lbb;
    STATUS   		status;

    /*
    ** Called before master has opened log?
    */
    if (lgd->lgd_header_lbb == 0 || lgd->lgd_master_lpb == 0)
	return (LG_BADPARAM);

    /*
    ** Use the header_lbb's lbb_mutex to single thread the use
    ** of the header_lbb.
    */

    header_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lgd->lgd_header_lbb);

    if (status = LG_mutex(SEM_EXCL, &header_lbb->lbb_mutex))
	return(status);

    /*
    ** If a header flush has already been queued, quietly ignore this one.
    ** THINK ABOUT THIS CAREFULLY!! FIX_ME!!
    */
    if (header_lbb->lbb_state != LBB_FREE)
    {
	(VOID)LG_unmutex(&header_lbb->lbb_mutex);
	return (OK);
    }

    /*
    ** Mark the header lbb as in use.
    */
    header_lbb->lbb_state = LBB_FORCE;
    header_lbb->lbb_lfb_offset = LGK_OFFSET_FROM_PTR(lfb);

    /*
    ** Need to queue an AST to the RCP to write out the headers, using the
    ** special reserved header LBB. Need to make this work for RCPCONFIG as
    ** well...
    */

    LG_inline_hdr_write((LBB *)header_lbb);

    (VOID)LG_unmutex(&header_lbb->lbb_mutex);

    return (OK);
}

/*
** Name: LG_inline_hdr_write	    - write the log headers
**
** Description:
**	This routine's function is to copy the logging system header from
**	shared memory into the provided log buffer, then to write it to the
**	log headers.
**
**	This routine is used instead of 'write_block' because the LBB in
**	question is the special reserved header LBB, which is not deallocated
**	to the standard LBB free list, and because the writes are done
**	synchronously -- noone waits for completion
**
** Inputs:
**	lbb	    - the special header LBB
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** History:
**	30-aug-1990 (bryanp)
**	    Created.
**	25-Jan-1996 (jenjo02)
**	    Mutex granularity project. lgd_header_lbb_mutex must be held on
**	    entry to this function.
**	03-Jan-2003 (jenjo02)
**	    During startup, current_lbb may not yet be assigned.
*/
VOID
LG_inline_hdr_write(register LBB *lbb)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LFB    *lfb = (LFB *)LGK_PTR_FROM_OFFSET(lbb->lbb_lfb_offset);
    i4	    status;
    CL_ERR_DESC	    sys_err;
    i4		    num_pages;
    i4	    err_code;
    char	    *write_buffer;
    LG_IO	    *primary_di_io;
    LG_IO	    *dual_di_io;
    LBB		    *current_lbb = (LBB*)NULL;

    if (lgd->lgd_master_lpb == 0)
	return;

    /* Lock the current_lbb, if any, to block updates to lfb_header */
    while ( lfb->lfb_current_lbb )
    {
	current_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb);
	if (status = LG_mutex(SEM_EXCL, &current_lbb->lbb_mutex))
	    return;

        if ( current_lbb == (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb) )
        {
	   /* current_lbb is still the "CURRENT" lbb. */
           break;
        }

        (VOID)LG_unmutex(&current_lbb->lbb_mutex);

#ifdef xDEBUG
        TRdisplay( "LG_inline_hdr_write():: Scenario for INGSRV2838 handled. EOF=<%d, %d, %d>\n",
                   lfb->lfb_header.lgh_end.la_sequence,
                   lfb->lfb_header.lgh_end.la_block,
                   lfb->lfb_header.lgh_end.la_offset);
#endif

        current_lbb = (LBB*)NULL;
    }

    TMget_stamp((TM_STAMP *)lfb->lfb_header.lgh_timestamp);
    lfb->lfb_header.lgh_active_logs = lfb->lfb_active_log;
    lfb->lfb_header.lgh_checksum = 
	    LGchk_sum((PTR)&lfb->lfb_header, sizeof(LG_HEADER));

    /* 
    ** Write the log header on every 1K page in the first log 
    ** block for 2 times.
    ** Copy the header into the buffer, release the mutex,
    ** then make the second copy from the first.
    */

    write_buffer = (char *)LGK_PTR_FROM_OFFSET(lbb->lbb_buffer);

    MEmove(sizeof(LG_HEADER), (PTR)&lfb->lfb_header, '\0',
	    1024, write_buffer);
    if ( current_lbb )
	(VOID)LG_unmutex(&current_lbb->lbb_mutex);
    MEmove(sizeof(LG_HEADER), (PTR)write_buffer, '\0',
	    1024, write_buffer + 1024); 

    /*
    ** We'll try the primary log first. We'll also try the dual log. If we
    ** didn't have both logs open, then the writes will fail, but as long as
    ** we successfully write to *some* log, we're happy enough. Note that we
    ** do this write "under the mutex". ON VMS, this means that we must have
    ** AST's enabled, and this is one of the reasons that we can't call
    ** EXinterrupt(EX_OFF) in LG_mutex(). Releasing the mutex is not a great
    ** solution, because we really want the world to be frozen until we can
    ** safely record any dual logging state change (i.e., the disabling of a
    ** log due to an error) on disk.
    */
    if (lfb->lfb_status & LFB_USE_DIIO)
    {
	primary_di_io = &lfb->lfb_di_cbs[LG_PRIMARY_DI_CB];
	dual_di_io = &lfb->lfb_di_cbs[LG_DUAL_DI_CB];
    }
    else
    {
	primary_di_io = &Lg_di_cbs[LG_PRIMARY_DI_CB];
	dual_di_io = &Lg_di_cbs[LG_DUAL_DI_CB];
    }

    num_pages = 1;
    status = LG_WRITE(primary_di_io, &num_pages,
			    (i4)0, write_buffer, &sys_err);
    if (status != OK && status != DI_BADFILE)
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);

    /*
    ** write the headers to the dual log file.
    */
    num_pages = 1;
    status = LG_WRITE(dual_di_io, &num_pages,
			    (i4)0, write_buffer, &sys_err);
    if (status != OK && status != DI_BADFILE)
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);

    /*
    ** Release the LBB for the next user.
    */
    lbb->lbb_state = LBB_FREE;

    /*
    ** This function is VOID...
    ** return (status);
    */
    return;
}
