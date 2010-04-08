/*
** Copyright (c) 1993, 2008 Ingres Corporation
NO_OPTIM=dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <nm.h>
#include    <cs.h>
#include    <pc.h>
#include    <tr.h>
#include    <me.h>
#include    <cm.h>
#include    <er.h>
#include    <lo.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm0s.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmd.h>
#include    <lkdef.h>

/*
**
**  Name: DMDLOCK.C - Debug print routine for system locks.
**
**  Description:
**      This module contains the routine used to format locking information.
**
**          dmd_lock_info 	   - Format lock list information.
**	    format_event_wait_info - Format wait info
**
**  History:
**      22-nov-1993 (jnash)
**	    Extracted from lockstat.c for lock tracing.
**      31-jan-1994 (mikem)
**          Bug #58407.
**          Added support to display the newly added CS_SID info in the
**          LK_LLB_INFO structure: llb_sid.  This is used by lockstat and
**          internal lockstat type info logging to print out session owning
**          locklist.
**          Given a pid and session id (usually available from errlog.log
**          traces), one can now track down what locks are associated with
**          that session.
**	20-apr-1994 (mikem)
**	    bug #59490
**	    TRdisplay() CS_SID's as %p, not %x.
**      20-apr-95 (nick)
**          Added support for lock type LK_CKP_TXN. (BUG 67888)
**	 1-nov-95 (nick)
**	    Include <sxf.h> and change use of magic numbers to defined
**	    values.
**	12-Jan-1996 (jenjo02)
**	    Added TRdisplay of llb_highwater, which is NOT equal
**	    to lbk_highwater! Added DMTR_SMALL_HEADER option to print
**	    a header which excludes the date/time.
**      01-aug-1996 (nanpr01 for ICL keving)
**          Display the number of granted LKBs with callbacks for RSBs.
**	21-Nov-1996 (jenjo02)
**	    Added 2 new stats, dlock_wakeups, dlock_max_q_len.
**      22-nov-96 (dilma04)
**          Row Locking Project:
**          Add support for LK_PH_PAGE and LK_VAL_LOCK.
**	11-Apr-1997 (jenjo02)
**	    Added new stats for callback threads.
**	24-Apr-1997 (jenjo02)
**	  - Added DMTR_LOCK_DIRTY/LK_S_DIRTY flag to augment LKshow() option. 
**	    This notifies LKshow() that locking structures are NOT to be 
**	    semaphore protected, used as a debugging aid.
**	  - Fixed DMTR_LOCK_LISTS so that it'll now list all LKBs on a lock list.
**	    Previously, it'd quit if the LLB + LKBs filled the buffer but there
**	    were more LKBs on the lock list.
**	22-Jan-1998 (jenjo02)
**	    Added pid and sid to LK_LKB_INFO so that the true waiting session
**	    can be identified. The llb's pid and sid identify the lock list
**	    creator, not necessarily the lock waiter (LK_MULTITHREAD-type
**	    lock lists and requests).
**	11-Jun-1998 (jenjo02)
**	    Added ON_DLOCKQ, DEADLOCK, XSVDB, XROW states to display of
**	    llb_status.
**	    Added DM0P_FCWAIT_EVENT, DM0P_GWAIT_EVENT interpretations.
**	    Extract WriteBehind thread's cache and show it.
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure. Updated format_event_wait()
**	    to deal with new interpretations.
**      28-Jul-1998 (strpa05)
**        - Correct LOCKSTAT output heading (x-int from oping12)        
**	03-Sep-1998 (jenjo02)
**	    DMTR_LOCK_RESOURCES was decrementing the size remaining in the
**	    buffer by sizeof(LK_LKB_INFO) instead of sizeof(LK_RSB_INFO),
**	    causing it to lose track of (and not display) the last (or only)
**	    LKB tied to a RSB.
**	08-Sep-1998 (jenjo02)
**	    Added display of max locks allocated per transaction.
**	16-feb-1999 (nanpr01)
**	    Support for update mode lock.
**	30-Nov-1999 (jenjo02)
**	    Cleaned up LLB TRdisplays, added to it llb_shared, llb_connect_count,
**	    and, if present, lock list's distributed transaction id.
**	15-Dec-1999 (jenjo02)
**	    Removed lock list distributed txn id, no longer needed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-may-2001 (devjo01) s103715.
**	    Always display UNIX pids as base 10 values, even if
**	    using clusters.  Remove VMS conditional compiles for
**	    cluster output, and use renamed fields for DLM info.
**	15-Feb-2002 (rigka01) bug#107029
**	  in dmd_lock_info(),
**	  print unsigned longnat/i4 data using %u instead of %d with TRdisplay 
**      30-dec-2005 (stial01 for huazh01)
**          Align buffer passed to LKshow 
**	17-oct-2006 (abbjo03/jenjo02)
**	    Display all the flag values for lkb_attribute's, fix display of
**	    count of physical locks.
**	21-nov-2006 (abbjo03)
**	    Remove stray 'd' from display of cluster enqueue totals.
**	05-feb-2008 (joea)
**	    Call new CXdlm_lock_info() if running in a cluster.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
*/

/*
**  Forward function references.
*/
static  VOID    format_event_wait_info(
				LK_EVENT *event,
				i4 event_flags);



/*{
** Name: dmd_lock_info	- Format locking system information.
**
** Description:
**      This routine formats locking system information.  It is used
**	by LOCKSTAT as well as error handling code triggered by certain
**	locking errors.
**
** Inputs:
**      options                         Display options
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Inputs:
**	options		- which statistics to report:
**			    DMTR_LOCK_SUMMARY		-- just summary info
**			    DMTR_LOCK_STATISTICS 	-- all lock statistics
**			    DMTR_LOCK_LISTS		-- locks by lock list
**			    DMTR_LOCK_USER_LISTS	-- locks by lock list
**			    DMTR_LOCK_SPECIAL_LISTS 	-- locks by lock list
**			    DMTR_LOCK_RESOURCES  	-- locks by resource
**			    DMTR_LOCK_ALL		-- all of the above
**			    DMTR_SMALL_HEADER	with any of the above
**	   					to display a shorter header.
**			    DMTR_LOCK_DIRTY		-- dirty-read lock 
**							   lock structures.
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
**      16-jun-1987 (Derek)
**	    Created for Jupiter.
**      28-Jul-1987 (ac)
**	    Added cluster support.
**	27-mar-1989 (rogerk)
**	    Added new lock list status values for Shared Buffer Managaer.
**	    Added new lock types for shared buffer manager.
**	19-jun-1989 (rogerk)
**	    When formatting locks by resource, use RSB, not LKB for buffer
**	    manager locks.
**	21-feb-1990 (sandyh)
**	    Added LK_SUMM information call to gather quota statistics.
**      25-sep-1991 (mikem) integrated following change: 19-aug-1991 (ralph)
**	    Add support for LK_EVCONNECT
**	11-sep-1992 (jnash)
**	    Redued logging project.
**	     -  Add LK_ROW lock type.
**	14-dec-1992 (jnash)
**	    Add AUDIT lock type in TRdisplay.
**	10-dec-1992 (robf)
**	    Handle AUDIT locks, also ROW locks now correct. 
**	24-may-1993 (bryanp)
**	    Added options field to enable selective lockstat.
**	    Fixed display of llb's because cluster and non-cluster LLB's now
**		both have the same llb_status definitions.
**	    Added llb_pid field to the lock list information structure so that
**		the lock list's creator PID could be displayed. Displayed the
**		PID in hex for VMS, decimal for all other systems.
**	    Added lots of new locking system statistics.
**	21-jun-1993 (bryanp)
**	    Display ALL the lkb_attribute values.
**	    Added "-user_lists" and "-special_lists"
**	26-jul-1993 (bryanp)
**	    If a lock list is in LLB_INFO_EWAIT state, display event info.
**	22-nov-1993 (jnash)
**	    Moved from lockstat.c to here for use in deadlock diagnostic  
**	    output.  Rename to dmd_lock_info().  Rename options to 
**	    include DMTR prefix.
**	7-jan-94 (robf)
**          Add back changes made to lockstat.c:
**          12-jul-93 (robf)
**             Improve formatting for AUDIT locks, now  shows  what they are
**      31-jan-1994 (mikem)
**          Bug #58407.
**          Added support to display the newly added CS_SID info in the 
**	    LK_LLB_INFO structure: llb_sid.  This is used by lockstat and 
**	    internal lockstat type info logging to print out session owning 
**	    locklist.
**          Given a pid and session id (usually available from errlog.log
**          traces), one can now track down what locks are associated with
**          that session.
**	31-mar-94 (robf)
**	     Added AUDIT QUEUE locks to trace
**	20-apr-1994 (mikem)
**	    bug #59490
**	    TRdisplay() CS_SID's as %p, not %x.
**      20-apr-1995 (nick)
**          Add support for LK_CKP_TXN
**	12-Jan-1996 (jenjo02)
**	    Added TRdisplay of llb_highwater, which is NOT equal
**	    to lbk_highwater! Added DMTR_SMALL_HEADER option to print
**	    a header which excludes the date/time.
**      01-aug-1996 (nanpr01 for ICL keving)
**          Display the number of granted LKBs with callbacks for RSBs.
**      22-nov-96 (dilma04)
**          Row Locking Project:
**          Add support for LK_PH_PAGE and LK_VAL_LOCK.
**	11-Apr-1997 (jenjo02)
**	    Added new stats for callback threads.
**	24-Apr-1997 (jenjo02)
**	  - Added DMTR_LOCK_DIRTY/LK_S_DIRTY flag to augment LKshow() option. 
**	    This notifies LKshow() that locking structures are NOT to be 
**	    semaphore protected, used as a debugging aid.
**	  - Fixed DMTR_LOCK_LISTS so that it'll now list all LKBs on a lock list.
**	    Previously, it'd quit if the LLB + LKBs filled the buffer but there
**	    were more LKBs on the lock list.
**	22-Jan-1998 (jenjo02)
**	    Added pid and sid to LK_LKB_INFO so that the true waiting session
**	    can be identified. The llb's pid and sid identify the lock list
**	    creator, not necessarily the lock waiter (LK_MULTITHREAD-type
**	    lock lists and requests).
**	11-Jun-1998 (jenjo02)
**	    Added ON_DLOCKQ, DEADLOCK, XSVDB, XROW states to display of
**	    llb_status.
**	03-Sep-1998 (jenjo02)
**	    DMTR_LOCK_RESOURCES was decrementing the size remaining in the
**	    buffer by sizeof(LK_LKB_INFO) instead of sizeof(LK_RSB_INFO),
**	    causing it to lose track of (and not display) the last (or only)
**	    LKB tied to a RSB.
**      28-Jul-1998 (strpa05) 
**        - Correct LOCKSTAT output heading (x-int from oping12)	
**	22-may-2001 (devjo01)
**	    s103715 generic cluster stuff.
**	04-dec-2001 (devjo01)
**	    Use structures LK_S_LOCKS_HEADER and LK_S_RESOURCE_HEADER to
**	    calculate start of LKB_INFO portion of buffer.  This technique
**	    assures that compilers on all platforms (even Tru64) will
**	    correctly align structures to prevent unaligned memory accesses
**	    regardless of platforms structure padding policy.
**	15-Feb-2002 (rigka01) bug#107029
**	  print unsigned longnat/i4 data using %u instead of %d with TRdisplay 
**	28-Feb-2002 (jenjo02)
**	    Added MULTITHREAD attribute to llb_status. LLB_WAITING is 
**	    contrived based on llb_waiters.
**	05-Mar-2002 (jenjo02)
**	    Added LK_SEQUENCE lock type.
**      04-sep-2002 (devjo01)
**          Take advantage of LKkey_to_string.
**	26-Oct-2007 (jonj)
**	    Update llb,lkb status bits, display
**	    lock requestor pid/sid if multithreaded list,
**	    all VMS pids in hex.
**	19-Nov-2007 (jonj)
**	    Include lkdef.h to pick up (new) status bits
**	    string defines.
**	04-Apr-2008 (jonj)
**	    Save local copies of llb_status, llb_pid, llb_sid
**	    as LLB gets overwritten with LKB if lots of locks.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: add display of stats by lock type.
*/
VOID
dmd_lock_info(i4 options)
{
    CL_ERR_DESC	    sys_err;
    LK_STAT	    stat;
    LK_SUMM	    summ;
    i4	    status;
    i4	    length;
    i4	    count;
    i4	    context;
    i4	    lkb_context;
    i4	    llb_id;
    i4	    llb_lkb_count;
    u_i4	    llb_status;
    PID		    llb_pid;
    CS_SID	    llb_sid;
    char            keybuf[128];
    char	    info_buffer[16384]; /* (wow) */
    char	    *buffer;
    i4		    buf_size;
    bool	    cluster;
    char	    *format_string;
    i4		    sem_option = options & DMTR_LOCK_DIRTY;
    i4		    i;
    
    cluster = CXcluster_enabled();
    buffer = (PTR) ME_ALIGN_MACRO(info_buffer, sizeof(ALIGN_RESTRICT));
    buf_size = sizeof(info_buffer) - (buffer - info_buffer);

    if (options & (DMTR_LOCK_SUMMARY|DMTR_LOCK_STATISTICS))
    {
	status = LKshow(LK_S_SUMM | sem_option, 0, 0, 0, sizeof(summ), 
                        (PTR)&summ, (u_i4 *)&length, (u_i4 *)&context,
			&sys_err); 
	if (status)
	{
	    TRdisplay("Can't show locking configuration summary.\n");
	    return;
	}
	if (options & DMTR_SMALL_HEADER)
	    TRdisplay("%29*- Locking System Summary %27*-\n\n");
	else
	    TRdisplay("%44*=%@ Locking System Summary%47*=\n\n");
	TRdisplay("    Total Locks          %8d", summ.lkb_size);
	TRdisplay("%8* Total Resources      %8d\n", summ.rsb_size);
	TRdisplay("%41* Locks per transaction%8d\n", summ.lkbs_per_xact);
	TRdisplay("    Lock hash table      %8d", summ.lkb_hash_size);
	TRdisplay("%8* Locks in use         %8d\n", summ.lkbs_inuse);
	TRdisplay("    Resource hash table  %8d", summ.rsb_hash_size);
	TRdisplay("%8* Resources in use     %8d\n", summ.rsbs_inuse);
	TRdisplay("    Total lock lists     %8d", summ.llb_size);
	TRdisplay("%8* Lock lists in use    %8d\n", summ.llbs_inuse);
    }

    if (options & DMTR_LOCK_STATISTICS)
    {
	status = LKshow(LK_S_STAT | sem_option, 0, 0, 0, sizeof(stat), 
                        (PTR)&stat, (u_i4 *)&length, (u_i4 *)&context,
			&sys_err); 
	if (status)
	{
	    TRdisplay("Can't show locking statistics.\n");
	    return;
	}

	if (options & DMTR_SMALL_HEADER)
	    TRdisplay("%29*- Locking System Statistics%24*-\n\n");
	else
	    TRdisplay("\n%44*=%@ Locking System Statistics%44*=\n\n"); 
	TRdisplay("    Create lock list     %10u", stat.create);
	TRdisplay("%8* Release lock list    %10u\n", stat.destroy);
	TRdisplay("    Request lock         %10u", stat.request);
	TRdisplay("%8* Re-request lock      %10u\n", stat.re_request);
	TRdisplay("    Convert lock         %10u", stat.convert);
	TRdisplay("%8* Release lock         %10u\n", stat.release);
	TRdisplay("    Escalate             %10u", stat.escalate);
	TRdisplay("%8* Lock wait            %10u\n", stat.wait);
	TRdisplay("    Convert wait         %10u", stat.con_wait);
	TRdisplay("%8* Convert Deadlock     %10u\n", stat.con_dead);
	TRdisplay("    Deadlock Wakeups     %10u", stat.dlock_wakeups);
	TRdisplay("%8* Max dlk queue len    %10u\n", stat.dlock_max_q_len);
	TRdisplay("    Deadlock Search      %10u", stat.dead_search);
	TRdisplay("%8* Deadlock             %10u\n", stat.deadlock);
	TRdisplay("    Cancel               %10u", stat.cancel);
	TRdisplay("%8* Convert Search       %10u\n", stat.con_search);
	TRdisplay("    Allocate CB          %10u", stat.allocate_cb);
	TRdisplay("%8* Deallocate CB        %10u\n", stat.deallocate_cb);
	TRdisplay("    LBK Highwater        %10u", stat.lbk_highwater);
	TRdisplay("%8* LLB Highwater        %10u\n", stat.llb_highwater);
	TRdisplay("    SBK Highwater        %10u", stat.sbk_highwater);
	TRdisplay("%8* LKB Highwater        %10u\n", stat.lkb_highwater);
	TRdisplay("    RBK Highwater        %10u", stat.rbk_highwater);
	TRdisplay("%8* RSB Highwater        %10u\n", stat.rsb_highwater);
	TRdisplay("    Max Local dlk srch   %10u", stat.max_lcl_dlk_srch);
	TRdisplay("%8* Dlk locks examined   %10u\n", stat.dlock_locks_examined);
	TRdisplay("    Max rsrc chain len   %10u", stat.max_rsrc_chain_len);
	TRdisplay("%8* Max lock chain len   %10u\n", stat.max_lock_chain_len);
	TRdisplay("    Max locks per txn    %10u", stat.max_lkb_per_txn);
	TRdisplay("%8* Callback Wakeups     %10u\n", stat.cback_wakeups);
	TRdisplay("    Callbacks Invoked    %10u", stat.cback_cbacks);
	TRdisplay("%8* Callbacks Ignored    %10u\n", stat.cback_stale);

	if (cluster)
	{
	    TRdisplay("    Enq                  %10u", stat.enq);
	    TRdisplay("%8* Deq                  %10u\n", stat.deq);
	    TRdisplay("    Sync Completions     %10u", stat.synch_complete);
	    TRdisplay("%8* Async Completions    %10u\n", stat.asynch_complete);
	    TRdisplay("    Gbl dlck srch pend   %10u", stat.gdlck_search);
	    TRdisplay("%8* Gbl deadlock         %10u\n", stat.gdeadlock);
	    TRdisplay("    Gbl grant pre-srch   %10u", stat.gdlck_grant);
	    TRdisplay("%8* Gbl dlck srch rqst   %10u\n", stat.totl_gdlck_search);
	    TRdisplay("    Gbl dlck srch calls  %10u", stat.gdlck_call);
	    TRdisplay("%8* Gbl dlck msgs sent   %10u\n", stat.gdlck_sent);
	    TRdisplay("    Con't gbl dlck calls %10u", stat.cnt_gdlck_call);
	    TRdisplay("%8* Con't gbl dlck sent  %10u\n", stat.cnt_gdlck_sent);
	    TRdisplay("    Unsent dlck srch rqst%10u", stat.unsent_gdlck_search);
	    TRdisplay("%8* Sent dlck srch rqst  %10u\n", stat.sent_all);
	    TRdisplay("    CSP IPC messages     %10u", stat.csp_msgs_rcvd);
	    TRdisplay("%8* CSP IPC wakeups      %10u\n", stat.csp_wakeups_sent);
	}

	TRdisplay("\n    Statistics by lock type:\n\n");

	/* NB: tstat[0] are the counts for all types */
	for ( i = 1; i <= LK_MAX_TYPE; i++ )
	{
	    LK_TSTAT	*tstat = &stat.tstat[i];

	    if ( tstat->request_new )
	    {
	        TRdisplay("    %12w Request lock        %10u", 
				LK_LOCK_TYPE_MEANING, i,
				tstat->request_new);
	        TRdisplay("%8* Re-request lock      %10u\n", tstat->request_convert);
		TRdisplay("    %13* Convert lock        %10u", tstat->convert);
	        TRdisplay("%8* Release lock         %10u\n", tstat->release);
		TRdisplay("    %13* Lock wait           %10u", tstat->wait);
	        TRdisplay("%8* Convert wait         %10u\n", tstat->convert_wait);
		TRdisplay("    %13* Deadlock            %10u", tstat->deadlock);
	        TRdisplay("%8* Convert deadlock     %10u\n", tstat->convert_deadlock);
	    }
	}
	TRdisplay("\n");
    }

    if (options & DMTR_LOCK_LISTS)
    {
	if (options & DMTR_SMALL_HEADER)
	    TRdisplay("%29*- Locks by lock list %30*-\n");
	else
	    TRdisplay("%55*-Locks by lock list%55*-\n");
	/*  Get information about lock lists and locks. */

	for (context = 0;;)
	{
	    LK_LLB_INFO		*llb = (LK_LLB_INFO*)buffer;
	    LK_LKB_INFO		*lkb = ((LK_S_LOCKS_HEADER*)llb)->lkbi;
    
	    status = LKshow(LK_S_LOCKS | sem_option, 0, 0, 0, buf_size, 
                            (PTR)buffer, (u_i4 *)&length, (u_i4 *)&context,
			    &sys_err); 

	    if (length == 0)
		break;

	    if ((options & DMTR_LOCK_USER_LISTS) == 0)
	    {
		if ((llb->llb_status & LLB_INFO_NONPROTECT) == 0 &&
		    llb->llb_key[0] != 0)
		    continue;	/* this is a user lock list */
	    }
	    if ((options & DMTR_LOCK_SPECIAL_LISTS) == 0)
	    {
		if ((llb->llb_status & LLB_INFO_NONPROTECT) != 0 ||
		    llb->llb_key[0] == 0)
		    continue;	/* this is a non-user lock list */
	    }

	    /*
	    **	Format the LLB. The llb_tick field is displayed only on a
	    **	VAXCluster.
	    */
 
	    TRdisplay(
"\nId: %x Tran_id: %x%x R_llb: %x R_cnt: %d S_llb: %x S_cnt: %d Wait: %x Locks: (%d,%d/%d)\n",
		llb->llb_id, llb->llb_key[0], llb->llb_key[1], 
                llb->llb_r_llb_id, llb->llb_r_cnt, 
		llb->llb_s_llb_id, llb->llb_s_cnt,
		llb->llb_wait_rsb_id, llb->llb_lkb_count,
		llb->llb_llkb_count, llb->llb_max_lkb);

#ifdef VMS
	    TRdisplay("    PID:%x SID:%p ", 
		    llb->llb_pid, llb->llb_sid);
#else
	    TRdisplay("    PID:%d SID:%p ", 
		    llb->llb_pid, llb->llb_sid);
#endif /* VMS */
	    if ( cluster )
		TRdisplay("Tick: %d ", llb->llb_tick);
		
	    TRdisplay("Status: %v ",
                LLB_STATUS, llb->llb_status);

	    TRdisplay("\n");
		
	    if (llb->llb_status & LLB_EWAIT)
		format_event_wait_info(&llb->llb_event, llb->llb_evflags);

	    TRdisplay("\n");

	    /*
	    ** If there are more LKBs on this list than will fit in our
	    ** buffer, we'll need to make multiple calls to LKshow() to
	    ** get them all.
	    **
	    ** Note that between calls, the number of LKBs on the list may
	    ** change and be more or less than that just reported in the LLB,
	    ** so don't expect perfection here.
	    */
	    llb_id = llb->llb_id;
	    llb_lkb_count = llb->llb_lkb_count;
	    llb_status = llb->llb_status;
	    llb_pid = llb->llb_pid;
	    llb_sid = llb->llb_sid;

	    /* count = number of LKBs just returned */
	    count = (length - ((PTR)lkb - (PTR)llb)) / sizeof(LK_LKB_INFO);
	    lkb_context = count;

	    while (count)
	    {
		/*  Format each LKB. */

		if (cluster)
		{
		    TRdisplay("    Id:%x Rsb:%x Gr:%3w Req:%3w State:%2w ", 
		    lkb->lkb_id, lkb->lkb_rsb_id,
		    LOCK_MODE, lkb->lkb_grant_mode,
		    LOCK_MODE, lkb->lkb_request_mode,
		    LKB_STATE, lkb->lkb_state);
		    if (lkb->lkb_flags & LKB_PHYSICAL)
			TRdisplay("PHYS(%d) ", lkb->lkb_phys_cnt);
		    else
			TRdisplay("%8* ");
		    TRdisplay("Lkid:%x.%x V:%x%x ",
			lkb->lkb_dlm_lkid.lk_uhigh,
			lkb->lkb_dlm_lkid.lk_ulow,
			lkb->lkb_dlm_lkvalue[0], lkb->lkb_dlm_lkvalue[1]);

		    TRdisplay("lk_attr: %v ",
			LKB_ATTRIBUTE, lkb->lkb_flags);
		}
		else
		{
		    TRdisplay("    Id: %x Rsb: %x Gr: %3w Req: %3w State: %2w ", 
		    lkb->lkb_id, lkb->lkb_rsb_id,
		    LOCK_MODE, lkb->lkb_grant_mode,
		    LOCK_MODE, lkb->lkb_request_mode,
		    LKB_STATE, lkb->lkb_state);
		    if (lkb->lkb_flags & LKB_PHYSICAL)
			TRdisplay("PHYS(%d) ", lkb->lkb_phys_cnt);
		    else
			TRdisplay("%8* ");
		}

		/* 
		** If not granted and the lock requestor is other than
		** the lock list creator, show the pid and sid of the
		** lock requestor.
		**
		** Also display pid/sid if multithreaded lock list.
		*/
		if ( (llb_status & LLB_MULTITHREAD ||
		       lkb->lkb_grant_mode != lkb->lkb_request_mode) &&
		    (lkb->lkb_pid != llb_pid || lkb->lkb_sid != llb_sid) )
		{
#ifdef VMS
		    TRdisplay("Pid:%x Sid:%p ", lkb->lkb_pid, lkb->lkb_sid);
#else
		    TRdisplay("Pid:%d Sid:%p ", lkb->lkb_pid, lkb->lkb_sid);
#endif /* VMS */
		}

                TRdisplay("KEY(%s)\n", LKkey_to_string(
		  (LK_LOCK_KEY *)&lkb->lkb_key[0], keybuf) );

		lkb++;
		llb_lkb_count--;
		/* Do next in this buffer */
		if (--count)
		    continue;
		/* Get next block of LKBs if there appear to be more */
		if (llb_lkb_count > 0)
		{
		    status = LKshow(LK_S_LIST_LOCKS | sem_option, llb_id, 0, 0,
				buf_size, buffer, (u_i4 *)&length, 
				(u_i4 *)&lkb_context, &sys_err);
		    if (length != 0)
		    {
			count = length / sizeof(LK_LKB_INFO);
			lkb = (LK_LKB_INFO *)buffer;
		    }
		}
	    } /* while (count) */
	} /* for (context = 0;;) */
    }

    if (options & DMTR_LOCK_RESOURCES)
    {
	if (options & DMTR_SMALL_HEADER)
	    TRdisplay("%29*- Locks by resource %31*-\n");
	else
	    TRdisplay("%55*-Locks by resource%56*-\n");

	/*  Get information about resources and lock requests. */

	for (context = 0;;)
	{
	    LK_RSB_INFO		*rsb = (LK_RSB_INFO*)buffer;
	    LK_LKB_INFO		*lkb = ((LK_S_RESOURCE_HEADER*)rsb)->lkbi;

	    status = LKshow(LK_S_RESOURCE | sem_option, 0, 0, 0, buf_size, 
                      buffer, (u_i4 *)&length, (u_i4 *)&context, &sys_err); 

	    if (length == 0)
		break;

	    /*	Format the RSB. */

            TRdisplay("\nId: %x Gr: %3w Conv: %3w Cbacks %d Value: <%x%x> %8w KEY(%s)\n",
		rsb->rsb_id,
		LOCK_MODE, rsb->rsb_grant,
		LOCK_MODE, rsb->rsb_convert,
                rsb->rsb_cback_count,
		rsb->rsb_value[0], rsb->rsb_value[1],
		",INVALID",  rsb->rsb_invalid,
		LKkey_to_string( (LK_LOCK_KEY *)&rsb->rsb_key[0], keybuf ) );

	    for (count = (length - ((PTR)lkb - (PTR)rsb))
                           / sizeof(LK_LKB_INFO); count; count--) 
	    {
		/*  Format each LKB. */

		TRdisplay("    Id: %x Llb: %x Gr: %3w Req: %3w State: %2w ", 
		    lkb->lkb_id, lkb->lkb_llb_id,
		    LOCK_MODE, lkb->lkb_grant_mode,
		    LOCK_MODE, lkb->lkb_request_mode,
		    LKB_STATE, lkb->lkb_state);
		if (lkb->lkb_flags & LKB_PHYSICAL)
		    TRdisplay("PHYS(%d) ", lkb->lkb_phys_cnt);
		/* Show the pid and sid of the lock requestor. */
		if (lkb->lkb_grant_mode != lkb->lkb_request_mode)
#ifdef VMS
		    TRdisplay("Pid:%x Sid:%p\n", lkb->lkb_pid, lkb->lkb_sid);
#else
		    TRdisplay("Pid:%d Sid:%p\n", lkb->lkb_pid, lkb->lkb_sid);
#endif /* VMS */
		else
		    TRdisplay("\n");
		lkb++;
	    }
	}
    }

    if (cluster && ((options & DMTR_LOCK_DLM) || (options & DMTR_LOCK_DLM_ALL)))
        dmd_dlm_lock_info((options & DMTR_LOCK_DLM_ALL) ? FALSE : TRUE);

    if (options & DMTR_SMALL_HEADER)
	TRdisplay("%80*-\n");
    else
	TRdisplay("%132*=\n");

    return;
}

/*
** Name: format_event_wait_info		    - format the lock event wait info
**
** Description:
**	When a lock list is waiting in EWAIT state, this routine attempts to
**	decipher and display information about what the lock list is waiting
**	on. I've made a first attempt at formatting something useful; we'll
**	probably need to add more good formatting info down the line...
**
** Inputs:
**	event				- Pointer to LK_EVENT describing the event
**	event_flags			- cross process event wait?
**
** Outputs:
**	none, but it TRdisplays stuff.
**
** Returns:
**	VOID
**
** History:
**	26-jul-1993 (bryanp)
**	    Created.
**	22-nov-1993 (jnash)
**	    Moved into dmdlock.c
**	11-Jun-1998 (jenjo02)
**	    Added DM0P_FCWAIT_EVENT, DM0P_GWAIT_EVENT interpretations.
**	    Extract WriteBehind thread's cache and show it.
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure. Updated format_event_wait()
**	    to deal with new interpretations. DM0P_._EVENT types have the
**	    cache index or'd into the lower portion of type_high and the 
**	    object being waited upon, if any, in type_low, and the buffer
**	    manager id in "value", i.e.,
**
**		type_high = DM0P_._EVENT | cache_ix
**		type_low  = buffer/group number/0
**		value	  = bmcb_id
**	30-Nov-1999 (jenjo02)
**	    Reduced number of tabs from 3 to 1 to make these displays
**	    easier on the eyes.
*/
static	VOID
format_event_wait_info(
LK_EVENT *event,
i4   event_flags)
{
    if (event->type_high >= DM0P_FWAIT_EVENT &&
	    event->type_high <= DM0P_FWAIT_EVENT + DM0P_EVENT_SHIFT - 1)
	TRdisplay("\t(Waiting for %wK free pages in cache %x)\n",
		    "2,4,8,16,32,64", event->type_high - DM0P_FWAIT_EVENT,
		    event->value);
    else if (event->type_high >= DM0P_WBFLUSH_EVENT &&
	    event->type_high <= DM0P_WBFLUSH_EVENT + DM0P_EVENT_SHIFT - 1)
	TRdisplay("\t(Waiting for %wK write-behind flush event in cache %x",
		    "2,4,8,16,32,64", event->type_high - DM0P_WBFLUSH_EVENT,
		    event->value);
    else if (event->type_high >= DM0P_BIO_EVENT &&
	    event->type_high <= DM0P_BIO_EVENT + DM0P_EVENT_SHIFT - 1)
	TRdisplay("\t(Waiting for I/O completion, %wK buffer %d in cache %x",
		    "2,4,8,16,32,64", event->type_high - DM0P_BIO_EVENT,
		    event->type_low, event->value);
    else if (event->type_high >= DM0P_PGMUTEX_EVENT &&
	    event->type_high <= DM0P_PGMUTEX_EVENT + DM0P_EVENT_SHIFT - 1)
	TRdisplay("\t(Waiting for page mutex, %wK buffer %d in cache %x",
		    "2,4,8,16,32,64", event->type_high - DM0P_PGMUTEX_EVENT,
		    event->type_low, event->value);
    else if (event->type_high >= DM0P_BMCWAIT_EVENT &&
	    event->type_high <= DM0P_BMCWAIT_EVENT + DM0P_EVENT_SHIFT - 1)
	TRdisplay("\t(Waiting for bufmgr cached object %d in cache %x",
		    event->type_low, event->value);
    else if (event->type_high >= DM0P_FCWAIT_EVENT &&
	    event->type_high <= DM0P_FCWAIT_EVENT + DM0P_EVENT_SHIFT - 1)
	TRdisplay("\t(Waiting for Consistency Point flush of %wK buffer %d in cache %x",
		    "2,4,8,16,32,64", event->type_high - DM0P_FCWAIT_EVENT,
		    event->type_low, event->value);
    else if (event->type_high >= DM0P_GWAIT_EVENT &&
	    event->type_high <= DM0P_GWAIT_EVENT + DM0P_EVENT_SHIFT - 1)
	TRdisplay("\t(Waiting for busy %wK group %d in cache %x",
		    "2,4,8,16,32,64", event->type_high - DM0P_GWAIT_EVENT,
		    event->type_low, event->value);
    else if (event->type_high == DM0S_TCBWAIT_EVENT)
	TRdisplay("\t(Waiting for TCB at physical address %x",
		    event->value);
    else if (event->type_high == 0)
#ifdef VMS
	TRdisplay("\t(Event Thread waiting in process %x",
#else
	TRdisplay("\t(Event Thread waiting in process %d",
#endif
		    event->value);
    else
	TRdisplay("\t(Waiting for event type %x.%x, event value %x",
		    event->type_high, event->type_low, event->value);

    if (event_flags == 1)
	TRdisplay(" -- CROSS_PROCESS)\n");
    else
	TRdisplay(")\n");

    return;
}
