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
#include    <pc.h>
#include    <er.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGRESERVE.C - Implements the LGreserve function of the logging system
**
**  Description:
**	This module contains the code which implements LGreserve.
**
**	External Routines:
**
**	    LGreserve -- Reserve space for a subsequent log write.
**
**	Internal Routines:
**
**	    LG_reserve
**
**  History:
**	07-jan-1993 (jnash)
**	    Created for the Reduced Logging project.
**	18-jan-1993 (jnash)
**	    Loosen parameter checking, add CL_CLEAR_ERR().
**	08-feb-1993 (jnash)
**	    ANSI-ize function defintion.
**	15-mar-1993 (jnash & rogerk)
**	    Add a "i4 factor" for each record reserved.
**	    Added LXB_NORESERVE flag.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>
**	20-sep-1993 (jnash)
**	    Add "force_flag" param to LGreserve().
**	18-oct-1993 (rogerk)
**	    Changed to pass flag parameter down to the LG_reserve call so
**	    the logspace reservation logic can be encapsulated into one place.
**	    Added LG_calc_reserved_space routine for this encapsulation.
**	20-jun-1994 (jnash)
** 	    Fix lint detected problems: remove unused variables i, rec_ohead,
**	    buf_ohead.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly
**	    named when calling LKMUTEX functions. Several new new
**	    semaphores added to keep lgd_mutex company.
** 	16-Jan-1996 (jenjo02)
**	    Explicitly state CS_LOG_MASK in CSsuspend() calls.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	06-mar-1996 (stial01 for bryanp)
**          Modify LGreserve edit checking to allow for reservation of space
**          for very long log records.
**	23-sep-96 (nick)
**	    Recast some of our stat counters to unsigned.
**	11-Mar-1997 (jenjo02)
**	    Removed Nick's log reservation changes which attempted to factor
**	    in "wasted" space per log record. Inaccurate at best, it abetted
**	    log space reservation creep, resulting in bogus DMA490/DMA491
**	    warnings.
**	15-Mar-1999 (jenjo02)
**	    Return LG_CKPDB_STALL error if txn is becoming active and needs
**	    to be stalled for CKPDB.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	25-Aug-1999 (jenjo02)
**	    Defeat LG_RS_FORCE which seems to serve no purpose other than
**	    to eat copious amounts of log space.
**	15-Dec-1999 (jenjo02)
**	    Add support for shared log transactions (LXB_SHARED/LXB_SHARED_HANDLE).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Feb-2003 (hweho01)
**          Turned off optimizer for AIX hybrid build by using 
**          the 64-bit configuration string (r64_us5), because the    
**          error happens only with the 64-bit compiler option.  
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
static STATUS	LG_reserve(
    LG_ID 		*lx_id,
    i4		flag,
    i4			num_records,
    i4 		res_space,
    STATUS 		*async_status);


/*{
** Name: LGreserve	- Reserve logfile space for subsequent LGwrite.
**
** Description:
**	In order to manage logfull calculations, normal log writes must be 
**	preceded by calls to LGreserve to reserve space in the log file.
**	Log writes that have compensating actions (that log
**	a Compensation Log Record (CLR) during transaction undo) must 
**	also reserve space for the CLR.  Space may be reserved for more
**	than one log write in a single call.  Log writes performed 
**	during recovery do not generally need to reserve space --
**	is has already been reserved prior to the action which is
**	being compensated.
**
** 	This action is taken outside LGwrite because recovery algorithms 
**	require that page mutexes be held throughout the call to LGwrite, 
**	ensuring that no one modifies the page between the log write and 
**	the insertion of the LSN onto the page.  
**
**	A normal sequence of actions would be:
**	    LGreserve()
**	    dm0p_mutex(page) 
**	    LGwrite()
**	    page->page_lsn = lsn
**	    dm0p_unmutex() 
**
**	If no space is available in the log file, LGreserve will wait 
**	for it to become available.  This routine does not currently
**	include an option to not wait for reserved space to become 
**	available, but this ability may prove useful in the future.
**
**	The force flag is used to reserve an entire log buffer for
***	use in the recovery of that operation.  This is the worst case
**	situation, but represents the easiest way to deal with this
**	situation.
**	
** Inputs:
**	lx_id				Transaction identifier.
**	flag				Zero, no special action
**					  LG_RS_FORCE, reserve 1 log buffer
**	res_space			The amount of space to reserve
**	num_records			Number of log records
**
** Outputs:
**	sys_err				CL error
**	Returns:
**	    OK
**	    LG_BADPARAM                 Parameter(s) in error.
**	    LG_WRITEERROR               Error reserving space.
**	    LG_BADMUTEX			Bad LG mutex operation
**	Exceptions:
**	    none
**
** Side Effects:
**	Caller will be suspended if no log space is available.
**
** History:
**	07-jan-1993 (jnash)
**          Created for the Reduced logging project.
**	20-sep-1993 (jnash)
**	    Add flag param, currently used to indicate a log force 
**	    is required during recovery.  In response, reserve 
**	    an additional log buffer's worth.
**	18-oct-1993 (rogerk)
**	    Changed to pass flag parameter down to the LG_reserve call so
**	    the logspace reservation logic can be encapsulated into one place.
** 	16-Jan-1996 (jenjo02)
**	    Explicitly state CS_LOG_MASK in CSsuspend() calls.
**	25-Jan-1996 (jenjo02)
**	    Removed acquisition and freeing of lgd_mutex.
**	06-mar-1996 (stial01 for bryanp)
**	    Modify LGreserve edit checking to allow for reservation of space
**		for very long log records.
**	29-jul-96 (nick)
**	    Add some calculations to reflect the factor by which log file
**	    space is unused and then take that into account when determining
**	    amount to reserve. #77308
*/
STATUS
LGreserve(
i4		    flag,
LG_LXID		    lx_id,
i4		    num_records,
i4		    res_space,
CL_ERR_DESC	    *sys_err)
{
    STATUS	    status;
    STATUS	    async_status = 0;
    LG_I4ID_TO_ID   xid;

    CL_CLEAR_ERR(sys_err);

    if ((num_records <= 0)  	||
	(num_records > 128)  	||      	    	/* Arbitrary */
	(res_space > (LG_MAX_RSZ * num_records)))	/* Arbitrary */
    {
	TRdisplay(
	  "LGreserve() bad num_records (%d) or res_space (%d) parameter.\n",
		num_records, res_space);
        return(LG_BADPARAM);
    }

    xid.id_i4id = lx_id;
    do
    {
	status = LG_reserve(&xid.id_lgid, flag, num_records, res_space, 
		    &async_status);

	/* Returns include (others are errors to be returned to caller):
	**      LG_SYNCH   	- Synchronous completion ok
	**      LG_MUST_SLEEP   - could not get some resource so wait until it
	**                        is around and then redo the call.
	*/
	if (status == LG_MUST_SLEEP)
	{
	    CSsuspend(CS_LOG_MASK, 0, 0);
	}
	else
	{
	    break;
	}

    } while (status == LG_MUST_SLEEP);

    if (status == LG_SYNCH)
	status = OK;

    return(status);
}

/*{
** Name: LG_reserve	- Reserve space in log file for subsequent LG_write
**
** Description:
**	Reserve space in log file for log record(s) about to be written.
**	If there is no space available and requested to wait, then
**	call suspend() here and return warning.  If caller does not wish
**	to wait for space to be freed, return warning.
**
**	Space reserved by this routine is freed by: 
**	   LG_write() will free whatever it writes (except CLR reservations),
**	   LG_end_tran() will free any space still reserved by the transaction.
**
**	Note that log writes performed by the RCP are not made within BT/ET
**	boundaries, so cannot rely on LG_end_tran() cleanup.
**
**	The routine does not perform detailed space available calculation
**	unless the log file is nearly full.
**	
** Inputs:
**	lx_id				The transaction id
**	res_space			The amount of space to reserve
**	num_records			Number of log records
**	astp				Ast parameter to pass.
**	async_status			sync status (not used)
**
** Outputs:
**	Returns:
**	    LG_BADPARAM			Bad parameters.
**	    LG_SYNCH			Success 
**	    LG_MUST_WAIT		Caller must wait
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-jan-1993 (jnash)
**	    Created for the Reduced logging project.
**	08-feb-1993 (jnash)
**	    ANSI-ize function defintion.
**	15-mar-1993 (jnash & rogerk)
**	    Include i4 space adjustment for all records.
**	    Added LXB_NORESERVE flag.  Transactions which specify this will
**		make updates without reserving space.  If one of these
**		transactions makes an LGreserve call, ignore the request
**		since the reservation will not be decremented on write requests.
**	18-oct-1993 (rogerk)
**	    Changed to pass flag parameter down to the LG_reserve call so
**	    the logspace reservation logic can be encapsulated into one place.
**	    Added LG_calc_reserved_space routine for this encapsulation.
**	    Moved checks for check_stall limit into the LG_check_logfull
**	    routine itself rather than checking before the call.
**	25-Jan-1996 (jenjo02)
**	    Several new mutexes added to augment lgd_mutex.
**	15-Mar-1999 (jenjo02)
**	    Return LG_CKPDB_STALL error if txn is becoming active and needs
**	    to be stalled for CKPDB.
**	26-Apr-2000 (thaju02)
**	    If txn is becoming active and needs to be stalled for ckpdb,
**	    put lxb on the lgd_wait_stall queue and return LG_MUST_SLEEP 
**	    to avoid race condition. (B101303)
**      18-Mar-2003 (horda03) Bug 109857
**          Remove change for Bug 101303. This causes another hang, which
**          is much easier to produce. If the DB is stalled, then the
**          session reserving space for a BT log recored should acquire
**          the LK_CKP_TXN lock
**	15-Mar-2006 (jenjo02)
**	    lgd_stat.wait cleanup, lxb_wait_reason defines moved to lg.h
*/
static STATUS
LG_reserve(
LG_ID		*lx_id,
i4		flag,
i4		num_records,
i4		res_space,
STATUS		*async_status)
{
    register LGD	*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXB	*lxb, *handle_lxb;
    register LDB	*ldb;
    register LPD	*lpd;
    register LPB	*lpb;
    register LFB	*lfb;
    SIZE_TYPE		*lxbb_table;
    i4			space_needed;
    STATUS		status;
    i4			err_code;

    LG_WHERE("LG_reserve");

    if (lx_id->id_id == 0 || (i4)lx_id->id_id > lgd->lgd_lxbb_count)
    {
	return (LG_BADPARAM);
    }

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    handle_lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id->id_id]);
    lfb = (LFB *)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_lfb_offset);
    lpd = (LPD *)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_lpd);
    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

    if (status = LG_mutex(SEM_EXCL, &handle_lxb->lxb_mutex))
	return(status);
    /*
    ** If LXB is a handle to a SHARED LXB, then use the actual shared
    ** LXB for the write request. Remember the handle LXB as it will be
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
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, xid.id_i4id,
			0, handle_lxb->lxb_status,
			0, "LG_reserve");
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
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			0, lxb->lxb_type,
			0, LXB_TYPE,
			0, xid1.id_i4id,
			0, lxb->lxb_status,
			0, xid2.id_i4id,
			0, "LG_reserve");
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

    /*
    ** Check if the database the transaction running against is inconsistent.
    ** In this case, just return without doing anything more and let LGwrite 
    ** flag the error.  This is done to avoid stalling below in logfull 
    ** conditions when we can't write log records anyway.
    */
    if ((ldb->ldb_status & LDB_INVALID) && (lxb->lxb_status & LXB_PROTECT))
    {
	lxb->lxb_status |= LXB_DBINCONST;
	(VOID)LG_unmutex(&lxb->lxb_mutex);
	if ( handle_lxb != lxb )
	{
	    handle_lxb->lxb_status |= LXB_DBINCONST;
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	}
	return(LG_SYNCH);
    }

    /*
    ** Setup logfull condition mask.  This call sets the LGD_STALL_MASK if
    ** appropriate for the checks below.
    */
    LG_check_logfull(lfb, lxb);   

    /*
    ** Check for Stall Conditions:
    **
    **   - if the RCP is writing a group of BCP records, we need to stall until
    **     the records are completed.
    **   - if we have reached the LOGFULL limit, we need to stall until more
    **     logspace is made available.
    **   - if we have reached the cpstall limit during a Consistency Point
    **     (generally 90% of the force_abort limit) then we need to stall until
    **     the CP is finished. This condition looks identical to normal LOGFULL
    **     to this routine.
    **
    ** The RCP is never stalled from the above conditions.
    ** The ACP and aborting transactions are allowed to continue during LOGFULL
    ** since they actually free up space rather then using it.
    */

    if (lgd->lgd_status & LGD_STALL_MASK)
    {
	/*
	** Lock the LGD, then check again.
	*/
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);

	if ((lgd->lgd_status & LGD_BCPSTALL) && 
		(lpb->lpb_status & LPB_MASTER) == 0)
	{
	    /* 
	    ** Everyone has to be stalled except the master during the
	    ** consistency point.
	    */
	    handle_lxb->lxb_wait_reason = LG_WAIT_BCP;
	    LG_suspend(handle_lxb, &lgd->lgd_wait_stall, async_status);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return (LG_MUST_SLEEP);
	}
	else if ((lgd->lgd_status & LGD_LOGFULL) &&
	    (lpb->lpb_status & LPB_MASTER) == 0 &&
	    (lpb->lpb_status & LPB_ARCHIVER) == 0 &&
	    (lxb->lxb_status & LXB_ABORT) == 0)
	{
	    /* 
	    ** Log file is full and we must wait
	    */
	    handle_lxb->lxb_wait_reason = LG_WAIT_LOGFULL;
	    LG_suspend(handle_lxb, &lgd->lgd_wait_stall, async_status);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
            return (LG_MUST_SLEEP);
	}
	else
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
    }

    /*
    ** If the transaction has set itself in LXB_NORESERVE state (it bypasses
    ** logspace reservation requirements) then just return.  Note that we
    ** were required to wait above on any logstall conditions.
    */
    if (lxb->lxb_status & LXB_NORESERVE)
    {
	(VOID)LG_unmutex(&lxb->lxb_mutex);
	if ( handle_lxb != lxb )
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	return(LG_SYNCH);
    }

    /* Active/protect txn reserving for first LGwrite? */
    if (lxb->lxb_status & LXB_INACTIVE &&
	lxb->lxb_status & LXB_PROTECT)
    {
	/*
	** Check for various stall conditions:
	**
	**   - Checkpoint pending: this indicates that an online checkpoint has
	**     been signaled and the logging system is preventing transactions
	**     from becoming active until all other active transactions have
	**     completed.
	**	
	**     Don't stall the checkpoint process from doing its online
	**     backup work (LPB_CKPDB).
	**
	**     Return LG_CKPDB_STALL status to the LGreserve() caller. Upon
	**     receiving that status, they will make a SHR lock request
	**     on LK_CKP_TXN for this database.
	**
	**     CKPDB has either been granted or has requested an EXCL lock
	**     on LK_CKP_TXN for this database. When it is ready to unstall
	**     it will release the EXCL lock, allowing these stalled log
	**     writers to run.
	**
	**     By utilizing locks instead of LGsuspend, all players become
	**     visible to locking's deadlock detection, and if a deadlock
	**     develops around CKPDB, it will be discovered and an 
	**     offending transaction aborted to free up the deadlock.
	*/
	if ( ldb->ldb_status & LDB_BACKUP &&
	     ldb->ldb_status & LDB_STALL &&
	    (lpb->lpb_status & LPB_CKPDB) == 0 )
	{
	    /* Lock the LDB, check again */
	    if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
		return(status);
	    if (ldb->ldb_status & LDB_BACKUP &&
		ldb->ldb_status & LDB_STALL)
	    {
		ldb->ldb_status |= LDB_CKPDB_PEND;
		lxb->lxb_status |= LXB_CKPDB_PEND;

		(VOID)LG_unmutex(&ldb->ldb_mutex);
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		if ( handle_lxb != lxb )
		    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		return LG_CKPDB_STALL;
	    }
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	}
	lxb->lxb_status &= ~LXB_CKPDB_PEND;
    }

    /*
    ** Calculate true space needed to fulfill the LGreserve request.
    ** This takes into account overhead for the log record header, log
    ** buffer header and required and possible log buffer forces.
    */
    space_needed = LG_calc_reserved_space(lxb, num_records, res_space, flag);

    lxb->lxb_reserved_space += space_needed;
    (VOID)LG_unmutex(&lxb->lxb_mutex);
    if ( handle_lxb != lxb )
	(VOID)LG_unmutex(&handle_lxb->lxb_mutex);

    /* lfb_mutex blocks concurrent update of lfb_reserved_space */
    if (status = LG_mutex(SEM_EXCL, &lfb->lfb_mutex))
	return(status);

    lfb->lfb_reserved_space += space_needed;
    
    (VOID)LG_unmutex(&lfb->lfb_mutex);

    return(LG_SYNCH);
}

/*{
** Name: LG_calc_reserved_space	- Calculate required logspace reservation.
**
** Description:
**	This routine takes a request for a logspace reservation for a certain
**	number of bytes and log records and computes the amount of logfile 
**	space (in bytes) needed to satisfy the logspace reservation protocols.
**
**	It takes into account the overhead for record headers, log buffer
**	headers, consistency points and known and possible log buffer forces 
**	during recovery.
**
**	Some of the overhead calculations round up to make sure that at least
**	one byte is allocated for that overhead.  Without this we might never
**	reserve space in some of the calculations because the computation
**	would yield less than 1 byte per log record.  When we do round up
**	in these calculations, it must be by 'num_records' amount rather than
**	by 1 byte to make the logspace calculations more deterministic.
**
**	This routine should be deterministic in the following way:
**
**	    The sum of the results of the calls:
**
**		LG_calc_reserved_space(1 record, J bytes);
**		LG_calc_reserved_space(1 record, K bytes);
**
**	    Must be equivalent to the result of the call:
**
**		LG_calc_reserved_space(2 records, J + K bytes);
**
**	This is because LGreserve will be called once to reserve space for
**	both the normal log record and its CLR.  As the log records are written
**	individually the space will be given back in two sets.
**
**	As it turns out however, since this routine does integer arithmetic
**	it cannot be completely deterministic because of truncations when
**	division is done.  It is possible that the sum of the two calls above
**	will return a value less than the single call below it.  This is
**	the preferred direction of the flaw since it will cause our xacts to
**	accumulate small amounts of extra reserved log space rather than
**	creeping in the direction of losing reserved space.  (The note above
**	about always using 'num_records' when rounding up calculations ensures
**	that the creep is in the positive direction).
**
**	In addition, there are factors outside our control for which we need
**	to account ; in group_commit scenarios ( or rather lack of ) log
**	writes don't actually write out 100% full buffers.  The ultimate
**	effect of this is that the space we reserve is insufficient to permit
**	successful recovery in some situations ( especially when we are 
**	aborted due to logfile limits ).  This make the calculation even less
**	deterministic.
**
** Inputs:
**	lxb				Transaction context
**	num_records			Number of log records
**	log_bytes			The amount of log record needed
**	flag				Reserve options:
**					LG_RS_FORCE - caller indication that a
**					    logforce will be required to undo
**					    the operation for which the log
**					    record space reservation is being
**					    done.  This causes an additional
**					    buffer-size amount of bytes to be
**					    tacked onto the reservation.
**
** Outputs:
**	Returns:
**	    space_needed		Amount of bytes needed to reserve
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-oct-1993 (rogerk)
**	    Created.
**	21-jun-96 (nick)
**	    Add some calculations to increase the reserved space to 
**	    account for the 'dead' space contained in the log buffers.
**	23-sep-96 (nick)
**	    Cast some things I added in the above change to unsigned.  This
**	    is a short term change as I'm in the process of redesigning
**	    this to remove dependency on long-term stats. and make the
**	    calculation more responsive to short term changes.
**	11-Mar-1997 (jenjo02)
**	    Removed Nick's log reservation changes which attempted to factor
**	    in "wasted" space per log record. Inaccurate at best, it abetted
**	    log space reservation creep, resulting in bogus DMA490/DMA491
**	    warnings.
**	    The calls to LG_calc_reserved_space() from LGreserve() and 
**	    LGwrite() must come up with the same number. Introducing 
**	    a variable into the computation can cause the reserved space
**	    to creep and, for very long running transactions, lead to 
**	    artificial DMA490/DMA491 warnings in the log.
**	25-Aug-1999 (jenjo02)
**	    Defeat LG_RS_FORCE which seems to serve no purpose other than
**	    to eat copious amounts of log space.
**	30-Apr-2004 (jenjo02)
**	    Brought back LG_RS_FORCE. MODIFY of large number of 
**	    partitions writes a ton of forced FCREATE and FRENAME
**	    log records, causing recovery to run off the end of
**	    the log.
**	06-Jul-2010 (jonj) SIR 122696
**	    Include LG_BKEND_SIZE in page overhead.
*/
i4
LG_calc_reserved_space(
register LXB	*lxb,
i4		num_records,
i4		log_bytes,
i4		flag)
{
    i4	    space_needed;
    register LFB    *lfb;
    register LPD    *lpd;
    register LPB    *lpb;

    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lxb->lxb_lfb_offset);
    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

    space_needed = log_bytes;

    /*
    ** Add log record overhead: LRH + one i4.
    **
    ** Each log record written needs to have its length rounded up to the size
    ** of a i4.  Since we don't know in this routine how the log_bytes
    ** amount of space is broken up if multiple log records are written, we
    ** play it safe and add an addition 3 bytes per log record.
    */
    space_needed += num_records * 
			(sizeof(LRH) + sizeof(i4) + (sizeof(i4) - 1));

    /*
    ** Add log buffer overhead.
    **
    ** Each lgh_size number of bytes we will be required to allocate a new
    ** log buffer.  We include the overhead for the buffer header by
    ** adding a percentage of this amount to each space reservation.
    */
    space_needed += ((space_needed * sizeof(LBH)) / 
			(lfb->lfb_header.lgh_size-LG_BKEND_SIZE - sizeof(LBH)))
				+ num_records;

    /*
    ** If the reserve flag indicated that an LGforce would be required during
    ** recovery then we must make sure that we allocate space for it.  Since
    ** we don't know exactly how much space will be used up in the log force
    ** we reserve an entire log buffers worth.
    */
    if (flag & LG_RS_FORCE)
	space_needed += lfb->lfb_header.lgh_size;

#ifdef xBMFORCE_RES
    /*
    ** Add overhead for background cache forces during recovery processing.
    ** During update processing we attempt to prereserve all resources that
    ** may be required in order to backout the update and restore the database
    ** to a consistent state.  This includes space needed in the logfile.  We
    ** wish to avoid at all costs reaching a logfile wrap-around condition
    ** during recovery.
    **
    ** Most resources needed during recovery are fairly deterministic - they
    ** are directly related and culculable from the particular update action.
    ** But Buffer Manager cache tosses are fairly non-deterministic.
    **
    ** The buffer manager caches Ingres pages with an LRU and page priority 
    ** scheme in which modified pages are given higher priorty than free pages
    ** and some pages tend to remain in the cache longer than others.
    ** During recovery operations, the buffer manager may be required to toss
    ** a modified page from the cache in order to make room for new read
    ** requests.  When this occurs, the buffer manager may need to force the
    ** log file (Write-Ahead Logging protocols).
    **
    ** These types of cache forces are extremely difficult to predict.
    ** We note that they are slightly more predictable in offline recovery
    ** situations when there are no other cache users complicating the issue,
    ** but even then it is not completely deterministic.
    **
    ** We attempt here to calculate some overhead to the space reservation
    ** request based upon the size of the callers cache to prepare for possible
    ** cache tosses needed during recovery.  We assume that one force request
    ** will be needed for every cache_factor log records - where cache_factor 
    ** is a function of:
    **
    **     - the server's cache size
    **     - the RCP's cache size (since recovery may fail over to the RCP)
    **     - the cache write-behind, mlimit and flimit values (since these
    **       cause cache tosses to occur before the cache becomes full).
    **
    ** We add a percentage of log force space requirement for each log record
    ** reservation.
    */
    if (lpb->lpb_cache_factor)
    {
	space_needed += 
	    ((num_records * lfb->lfb_header.lgh_size) / lpb->lpb_cache_factor);
    }
#endif

    /*
    ** We need to account for dead space in the logfile ; failure to
    ** do so means we potentially run out of space when writing CLRs 
    ** which then causes inconsistent databases of course.
    */

    /*
    ** Add Consistency Point overhead.
    **
    ** During future recovery operations we may need to execute Consistency
    ** Points as each cpinterval's worth of log records are written.
    ** We prereserve one log buffer of space for each cpinterval log bytes
    ** of reserved space allocating a portion to each reserve operation.
    ** The formula below looks non-intuitive because the log buffer size
    ** was factored out of the original equation:
    **
    **                                           buffer_size
    **  space_needed +=  space_needed *  -----------------------------
    **                                     (buffer_size * lgh_l_cp)
    */
    space_needed += (space_needed / lfb->lfb_header.lgh_l_cp) + num_records; 

    return (space_needed);
}
