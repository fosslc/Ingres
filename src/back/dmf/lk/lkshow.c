/*
**Copyright (c) 2004 Ingres Corporation
**
NO_OPTIM=dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <sxf.h>
#include    <dm.h>
#include    <lk.h>
#include    <cx.h>
#include    <lkdef.h>
#include    <lgkdef.h>
#include    <csp.h>

/**
**
**  Name: LKSHOW.C - Implements the LKshow function of the locking system
**
**  Description:
**	This module contains the code which implements LKshow.
**	
**	    LKshow -- retrieve information from the locking system
**	    LKkey_to_string -- format lock key value into a buffer.
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-may-1993 (bryanp)
**	    Return the lock list's creator pid when showing a lock list.
**	    Return new locking system statistics.
**	    RSB's are now in the rbk_table. only lkbs in the sbk_table.
**	30-jun-1993 (shailaja)
**	    Fixed compiler warnings on semantics change in ANSI C.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Cluster-specific changes to LKshow(LK_S_KEY_INFO) -- on the cluster,
**		this routine must call out to the VMS DLM to enquire about the
**		row lock.
**	    Add some casts to pacify the compiler regarding unsigned conversion.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <tr.h>.
**      31-jan-1994 (mikem)
**          Bug #58407.
**          Added support to LK_show to export the new CS_SID info added to the 
**	    LK_LLB_INFO structure: llb_sid.  This is
**          used by lockstat and internal lockstat type info logging to print
**          out session owning locklist.
**          Given a pid and session id (usually available from errlog.log
**          traces), one can now track down what locks are associated with
**          that session.
**	31-jan-1994 (jnash)
**	    Fix lint detected ule_format param error.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Changed LK_xmutex() calls to pass mutex field address
**		- Add lkd pointer definition where necessary
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	06-Dec-1995 (jenjo02)
**	    Added LK_S_MUTEX function to LKshow() to return locking
**	    system semaphore stats.
**	03-jan-1996 (duursma) bug #71334
**	    unsent_gdlck_search no longer used; set corresponding field in
**	    stat to 0.
**	15-May-1996 (prida01)
**	    Fix for bug 76515. Check offset pointers are set before 
**	    accessing the structures they point to.
**      21-Feb-1996 (jenjo02)
**          MEfill 2*lstat when extracting RSB semaphore stats.
**	14-May-1996 (jenjo02)
**	    Added lkd_dlock_mutex stat.
**	20-May-1996 (jenjo02)
**	    Recheck rsb_type, llb_type after acquiring SHARED mutex;
**	    those cb's may have been in the process of being
**	    deallocated while we waited for the mutex.
**      01-aug-1996 (nanpr01 for ICL keving)                     
**          Store cback_count in rsb info blocks.
**	15-Nov-1996 (jenjo02)
**	    Removed references to rsb_dlock_mutex.
**	    Added 2 new stats, dlock_wakeups, dlock_max_q_len.
**	    Added sem stats for lkd_dw_q_mutex.
**      28-feb-1996 (stial01)
**          LK_show() Added LK_S_XSVDB
**	07-Apr-1997 (jenjo02)
**	    Added LK_S_SLOCKQ to quiet expected errors when running DMCM.
**	    Added new stats for callback threads.
**	24-Apr-1997 (jenjo02)
**	  - Added LK_S_DIRTY flag to augment LKshow() option. This notifies
**	    LKshow() that locking structures are NOT to be semaphore-protected,
**	    used as a debugging aid.
**      02-jul-97 (dilma04)
**          Return llb_status from  LK_show() if the routine is called with 
**          LK_S_LLB_INFO flag.
**	22-Jan-1998 (jenjo02)
**	    Added pid and sid to LK_LKB_INFO so that the true waiting session
**	    can be identified. The llb's pid and sid identify the lock list
**	    creator, not necessarily the lock waiter.
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure.
**	08-Sep-1998 (jenjo02)
**	    Added new statistic, max locks allocated per transaction.
**	16-Nov-1998 (jenjo02)
**	  o Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	  o Installed macros to use ReaderWriter locks when CL-available
**	    for blocking the deadlock matrix.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	08-Feb-1999 (jenjo02)
**	    Extract waiting LKB from llb_lkb_wait instead of llb_wait.
**	15-Mar-1999 (jenjo02)
**	    Added LK_S_LIST_NAME function to retrieve lock list id by
**	    "llb_name" (== LG transaction id).
**      28-May-1999 (jenjo02,nanpr01)
**          Rewrote LK_S_OWNER_GRANT to avoid SIGBUS
**	02-Aug-1999 (jenjo02)
**	    Count RSB-embedded LKBs in total of lkbs_inuse.
**	06-Oct-1999 (jenjo02)
**	    LLB's now identified with LLB_ID structure instead
**	    of LK_ID.
**	    LK_ID (rsb_id, lkb_id) changed to u_i4 instead of stucture
**	    containing only a u_i4.
**	22-Oct-1999 (jenjo02)
**	    Deleted lkd_dlock_lock.
**	30-Nov-1999 (jenjo02)
**	    Made "related" lock list an attribute of the PARENT_SHARED
**	    (referencing) list rather than the SHARED (referenced) list.
**	    Added llb_dis_id, llb_s_llb_id, llb_s_cnt to LK_LLB_INFO.
**	    Modified LK_S_TRAN_LOCKS, LK_S_REL_TRAN_LOCKS to cull out
**	    only "write" locks, which is what dm0l_secure() (the only 
**	    caller of these options) was doing anyway.
**	15-Dec-1999 (jenjo02)
**	    Removed llb_dis_tran_id, no longer needed.
**	04-Dec-2001 (devjo01)
**	    Use structures LK_S_LOCKS_HEADER and LK_S_RESOURCE_HEADER to
**	    calculate start of LKB_INFO portion of buffer for LK_S_LOCKS
**	    and LK_S_RESOURCE calls to LKshow.  Clear *sys_err in LK_show.
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	28-Feb-2002 (jenjo02)
**	    LLB_WAITING is now contrived based on the value of
**	    llb_waiters and is never "set" in LLB:llb_status
**	11-Sep-2003 (devjo01)
**	    Add LK_SEQUENCE and LK_XA_CONTROL to LKkey_to_string.
**	17-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**      13-May-2005(horda03) Bug 114508/INGSRV3301
**          LK_S_OWNER_GRANT now returns lkb_attribute details.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**      9-jun-2005 (mutma03)
**          Added check for rsb and llb to prevent SEGV.
**     23-sep-2005 (stial01)
**          LK_S_OWNER_GRANT: also return lkb_flags and lkb_phys_cnt (b115299)
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**	09-Jan-2009 (jonj)
**	    LKREQ st ucture deleted. lkb_lkreq.cpid is now lkb_cpid.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*
** Forward declarations for static functions:
*/
static STATUS	fill_in_stat_block(
		    LK_STAT	    *stat,
		    i4		    info_size,
		    u_i4	    *info_result,
		    LKD		    *lkd );
static STATUS	LK_show(
		    i4		    flag,
		    LK_LLID	    listid,
		    LK_LKID	    *lockid,
		    LK_LOCK_KEY	    *lock_key,
		    u_i4	    info_size,
		    PTR		    info_buffer,
		    u_i4	    *info_result,
		    u_i4	    *context,
		    CL_ERR_DESC	    *sys_err);

/*{
** Name: LKshow	- Show information about locks.
**
** Description:
**      This routine can be asked to return the following information:
**	  o  Information about a specific lock for a lock list.
**        o  Information about the deadlock that last occured for this list.
**	  o  Information about a specific resource.
**	  o  Information about all locks held by a lock list.
**
**	The last two pieces of information are variable length.  They can
**	return multiple entries for each successive call until no others
**	exist.  The consistency of these lists with respect to concurrent
**	changes is not defined except if the object is known not to be changing
**	at the moment.
**
** Inputs:
**      flag                            On of the following flags: 
**					    LK_S_RESOURCE 
**					    LK_S_LOCK 
**					    LK_S_LIST
**					    LK_S_LIST_LOCKS
**					    LK_S_DEADLOCK
**					    LK_S_MUTEX
**					Any of the above with the LK_S_DIRTY
**					flag added instructs LKshow() to 
**					skip semaphore protection of locking
**					structures. This is used as a debugging
**					aid so that one may be holding a lock
**					resource mutex and still be able to
**					produce a lockstat report.
**
**	lli				The list to use for a LK_S_LIST or
**					LK_LIST_LOCKS operation.
**	lockid				The lockid for a RESOURCE of LOCK 
**					request.
**	lock_key			The lock_key for a RESOURCE or LOCK 
**					request.
**	info_buffer			The address of the buffer to return the 
**                                      information.
**	info_size			The size of the buffer.
**	context				Set to zero on first call, LK uses this
**					to continue the LK service.
** Outputs:
**	info_result			The resulting length of the buffer.
**	context				Updated context information.
**	info_buffer			The information to be returned.
**
**	Returns:
**	    OK				Successful completion
**	    LK_BADPARARM		Bad parameter.
**
**	Exceptions:
**	   None.
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
** 	09-ocj-1992 (jnash)
**	    Reduced loggin project.  Add support for showing row locks 
**	    via LKshow(LK_S_KEY_INFO).
**	03-nov-1992 (jnash)
**	    Fix bug in LK_S_KEY_INFO and similar one in LK_S_OWNER.
**	16-feb-1993 (andys)
**	    Change third argument of LKshow to be a LK_LKID not a LK_ID
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Changed LK_xmutex() calls to pass mutex field address
**		- Added lkd pointer definition
**	06-Dec-1995 (jenjo02)
**	    Added LK_S_MUTEX function to LKshow() to return locking
**	    system semaphore stats. Removed latch of lkd_mutex.
**      21-Feb-1996 (jenjo02)
**          MEfill 2*lstat when extracting RSB semaphore stats.
**	24-Apr-1997 (jenjo02)
**	  - Added LK_S_DIRTY flag to augment LKshow() option. This notifies
**	    LKshow() that locking structures are NOT to be semaphore-protected,
**	    used as a debugging aid.
**	06-Oct-1999 (jenjo02)
**	    Correctly type second parm as LK_LLID.
*/
STATUS
LKshow(
i4                 flag,
LK_LLID		    listid,
LK_LKID		    *lockid,
LK_LOCK_KEY	    *lock_key,
u_i4	    	    info_size,
PTR		    info_buffer,
u_i4		    *info_result,
u_i4               *context,
CL_ERR_DESC         *sys_err)
{
    STATUS	status;
    LKD         *lkd = (LKD *)LGK_base.lgk_lkd_ptr;

    LK_WHERE("LKshow")

    return( LK_show(flag, listid, lockid, lock_key, info_size, 
			 info_buffer, info_result, context, sys_err));
}

/*{
** Name: LK_show	- Show information about locks.
**
** Description:
**(    This routine can be asked to return the following information:
**	  o  Information about a specific lock for a lock list.
**        o  Information about the deadlock that last occured for this list.
**	  o  Information about a specific resource.
**	  o  Information about all locks held by a lock list.
**
**	The last two pieces of information are variable length.  They can
**	return multiple entries for each successive call until no others
**	exist.  The consistency of these lists with respect to concurrent
**	changes is not defined except if the object is known not to be changing
**	at the moment.
**
** Inputs:
**      flag                            One of the following flags: 
**					    LK_S_RESOURCE : show all locks -
**						group by resource.
**					    LK_S_SRESOURCE: show specified lock
**						resource.
**					    LK_S_LOCKS    : show all locks -
**						group by lock list.
**					    LK_S_SLOCK    : show specified lock.
**					    LK_S_SLOCKQ   : show specified lock,
**							    ignore error (DMCM).
**					    LK_S_OWNER    : show owner of 
**						specified lock.
**					    LK_S_ORPHANED : show orphaned locks.
**					    LK_S_XSVDB    : show lock list 
**						with EXCL SV_DATABASE lock.
**					    LK_S_STAT	  : show locking stats.
**					    LK_S_LIST     : show lock list
**					    LK_S_LLB_INFO : return id of related
**						lock list.  info_result set to 
**						zero if none exits.
**					    LK_S_TRAN_LOCKS: Return brief info
**						about locks on specified 
**						lock list.
**					    LK_S_REL_TRAN_LOCKS: Return brief
**						info about locks on related lock
**						list of specified list.
**					    LK_S_KEY_INFO  : show if key exists
**					    LK_S_LIST_LOCKS: show all locks
**						owned by a specified lock list.
**					    LK_S_MUTEX     : show sem stats
**					    LK_S_ULOCKS    : Return OK if
**						LK_U locks supported.
**					Any of the above with the LK_S_DIRTY
**					flag added instructs LKshow() to 
**					skip semaphore protection of locking
**					structures. This is used as a debugging
**					aid so that one may be holding a lock
**					resource mutex and still be able to
**					produce a lockstat report.
**	listid				The Lock list to use. Reqired for:
**					    LK_S_LIST - list to show.
**					    LK_S_OWNER - list lock is held on.
**					    LK_S_TRAN_LOCKS - list holding locks
**						to return info on.
**					    LK_S_REL_TRAN_LOCKS - list related
**						to llb holding locks to return
**						info on.
**					    LK_S_LLB_INFO - list to show.
**					    LK_S_LIST_LOCKS - list to show.
**	lockid				The lockid of lock to use. Required for:
**					    LK_S_SLOCK - lock to show.
**					    LK_S_SRESOURCE - resource to show.
**	lock_key			The lock_key to use. Required for:
**					    LK_S_OWNER - key of lock.
**					    LK_S_KEY_INFO - key of lock.
**	info_buffer			The address of the buffer to return the 
**                                      information.
**	info_size			The size of the buffer.
**	context				Set to zero on first call, LK uses
**					this to continue the LK show service.
** Outputs:
**	info_result			The resulting length of the buffer.
**					    LK_S_KEY_INFO:
**						zero if nothing found
** 						nonzero if "row lock held"
**	context				Updated context information.
**	info_buffer			The information to be returned.
**
**	Returns:
**	    OK			Successful completion
**	    LK_BADPARARM		Bad parameter.
**
**	Exceptions:
**	   None.
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	09-oct-1992 (jnash)
**	    Reduced logging project.
**	     -  Add support for LK_show(LK_S_KEY_INFO), used in system
**		catalog file extend operations.  This action accepts a lock
**	  	key and searches for it on the rsb queue.  If found, selected
**		information about the resource is returned in info_buffer
**		and "info_result" is nonzero.  If not found, "info_result"
**		is zero.  The cluster version of this routine also checks
**		for locks held within the VMS distributed lock manager.
**	03-nov-1992 (jnash)
**	    Fix bug in LK_S_KEY_INFO and similar one in LK_S_OWNER.
**	24-may-1993 (bryanp)
**	    Return the lock list's creator pid when showing a lock list.
**	    Return new locking system statistics.
**	    RSB's are now in the rbk_table. only lkbs in the sbk_table.
**	18-june-1993 (shailaja)
**         id_id cast to i4
**	26-jul-1993 (bryanp)
**	    Broke LK_S_KEY_INFO code out into a separate subroutine and added
**		some first attempts at cluster support.
**	31-jan-1994 (rogerk)
**	    Added LK_S_LIST_LOCKS show option to return all locks held by
**	    a particular lock list.  This option is used by the recovery
**	    system to search for locks to release prior to starting recovery 
**	    processing.
**      31-jan-1994 (mikem)
**          Bug #58407.
**          Added support to LK_show to export the new CS_SID info added to the
**          LK_LLB_INFO structure: llb_sid.  This is
**          used by lockstat and internal lockstat type info logging to print
**          out session owning locklist.
**          Given a pid and session id (usually available from errlog.log
**          traces), one can now track down what locks are associated with
**          that session.
**	31-jan-1994 (jnash)
**	    Fix lint detected ule_format param error.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	06-Dec-1995 (jenjo02)
**	    Added LK_S_MUTEX function to LKshow() to return locking
**	    system semaphore stats.
**	15-May-1996 (prida01)
**	    Need to test that the offsets for rsb's and llb's are setup or
**	    we could be in a race condition when they are created and get
**	    a SIGBUS or SEGV when using IPM.
**	14-May-1996 (jenjo02)
**	    Added lkd_dlock_mutex stat.
**	20-May-1996 (jenjo02)
**	    Recheck rsb_type, llb_type after acquiring SHARED mutex;
**	    those cb's may have been in the process of being
**	    deallocated while we waited for the mutex.
**      01-aug-1996 (nanpr01 for ICL keving)                     
**          Store cback_count in rsb info blocks.
**	15-Nov-1996 (jenjo02)
**	    Removed references to rsb_dlock_mutex.
**      28-feb-1996 (stial01)
**          Added LK_S_XSVDB
**	07-Apr-1997 (jenjo02)
**	    Added LK_S_SLOCKQ to quiet expected errors when running DMCM.
**	    Added new stats for callback threads.
**	24-Apr-1997 (jenjo02)
**	  - Added LK_S_DIRTY flag to augment LKshow() option. This notifies
**	    LKshow() that locking structures are NOT to be semaphore-protected,
**	    used as a debugging aid.
**      02-jul-97 (dilma04)
**          Return llb_status if called with LK_S_LLB_INFO flag.
**	22-Jan-1998 (jenjo02)
**	    Added pid and sid to LK_LKB_INFO so that the true waiting session
**	    can be identified. The llb's pid and sid identify the lock list
**	    creator, not necessarily the lock waiter.
**      28-dec-1998 (stial01)
**          Added LK_S_OWNER_GRANT
**	15-Mar-1999 (jenjo02)
**	    Added LK_S_LIST_NAME function to retrieve lock list id by
**	    "llb_name" (== LG transaction id).
**	02-Aug-1999 (jenjo02)
**	    Count RSB-embedded LKBs in total of lkbs_inuse.
**	06-Oct-1999 (jenjo02)
**	    LLB's now identified with LLB_ID structure instead
**	    of LK_ID.
**	    LK_ID (rsb_id, lkb_id) changed to u_i4 instead of stucture
**	    containing only a u_i4.
**	04-Dec-2001 (devjo01)
**	    - Use structures LK_S_LOCKS_HEADER and LK_S_RESOURCE_HEADER to
**	    calculate start of LKB_INFO portion of buffer to avoid calculating
**          pointer to lkb portion using logic like rsb = (LK_RSB_INFO*)buffer;
**          lkb = (LK_LKB_INFO*)&rsb[1].  Under Tru64, this causes alignment
**          problems because alignment requirements for LK_RSB_INFO and
**	    LK_LKB_INFO differ.
**	    - Also use CL_CLEAR_ERR to clear sys_err.  dm0p.c and possibly
**	    others, declare an uninitialized CL_ERR_DESC structure,
**	    then call LKshow then later pass the CL_ERR_DESC structure to
**	    ule_format which kicks out an E_CL0902_ER_NOT_FOUND
**	    when it finds non-zero garbage here.
**      13-May-2005(horda03) Bug 114508/INGSRV3301
**          LK_S_OWNER_GRANT now returns lkb_attribute details.
**	08-may-2001 (devjo01)
**	    Removed unused LK_S_KEY_INFO kludge. 
**	    Use lkb_dlm_lkid, instead of lkb_vms_lkid, etc.
**	08-jan-2004 (devjo01)
**	    Use macros to get lkb_dlm_lkid from cx_lock_id to allow for
**	    different DLM lock id formats.
**      07-dec-2004 (stial01)
**          LK_CKP_TXN: key change to include db_id
**          LK_CKP_TXN: key change remove node id, LKrequest LK_LOCAL instead
**	04-Jun-2007 (jonj)
**	    LK_S_SLOCK: return LK_BADPARAM also if rsb or llb is null.
**	    Cross-check lockid->lk_common vs rsb_id.
** 	01-Oct-2007 (jonj)
**	    CBACK is now embedded in LKB.
**	04-Apr-2008 (jonj)
**	    Add LK_S_ULOCKS function.
**	17-Apr-2008 (kibro01) b120277
**	    Removed erroneous ,0 from ule_format
**	09-Jan-2008 (jonj)
**	    For LK_S_MUTEX, cast LK_SEMAPHORE to CS_SEMAPHORE.
*/
/* ARGSUSED */
static STATUS
LK_show(
i4                  flag,
LK_LLID		    listid,
LK_LKID		    *lockid,
LK_LOCK_KEY	    *lock_key,
u_i4		    info_size,
PTR		    info_buffer,
u_i4		    *info_result,
u_i4		    *context,
CL_ERR_DESC	    *sys_err)
{
    LKD			*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    i4		i;
    i4		pid;
    STATUS		status;
    LLB			*llb;
    LLB			*related_llb;
    LLB			*my_llb;
    RSB			*rsb;
    RSB			*wait_rsb;
    LKB			*lkb;
    LKB			*wait_lkb;
    LKB			*next_lkb;
    SIZE_TYPE		lkb_offset;
    SIZE_TYPE		end_offset;
    LLBQ		*llbq;
    SIZE_TYPE		llbq_offset;
    RSH			*rsb_hash_bucket;
    u_i4		rsb_hash_value;
    LK_RSB_INFO		*rsbi;
    LK_LKB_INFO		*lkbi;
    LK_LLB_INFO		*llbi;
    LK_BLKB_INFO	*blkbi;
    LK_STAT		*stat;
    LK_SUMM		*summ;
    LLB_ID		*llb_id;
    LK_MUTEX		*mstat;
    CS_SEM_STATS	*lstat;
    LKH			*lkh;
    RSH			*rsh;
    i4		count;
    i4		count_max;
    i4		prev;
    SIZE_TYPE		*lbk_table;
    SIZE_TYPE		*sbk_table;
    SIZE_TYPE		*rbk_table;
    RSH			*rsh_table;
    LKH			*lkh_table;
    i4		matching_rsb;
    i4		starting_lkb;
    i4		lkb_idx;
    i4		err_code;
    i4             show_status;
    i4			dirty_read = (flag & LK_S_DIRTY);


    LK_WHERE("LK_show")

    /* Off the "dirty_read" flag */
    flag &= ~LK_S_DIRTY;

    CL_CLEAR_ERR(sys_err);

    switch (flag)
    {
    case LK_S_ORPHANED:
    case LK_S_XSVDB:
	if (flag == LK_S_ORPHANED)
	    show_status = LLB_ORPHANED;
	else 
	    show_status = LLB_XSVDB;

	if (info_size < sizeof(LK_ID) || lock_key == NULL)
	{
	    uleFormat(NULL, E_CL1043_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, info_size, sizeof(LK_ID), lock_key);
	    return(LK_BADPARAM);
	}

	/* *context denotes the lkd_lbk_table index to get the next LLB */

	i = *context + 1;
	*context = 0;	    /* Assume no more next */
	*info_result = 0;
	MECOPY_CONST_MACRO((char *)lock_key, sizeof(pid), (char *) &pid);

	lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);

	for (; i <= lkd->lkd_lbk_count; i++)	
	{
	    LLB_ID	*llb_id = (LLB_ID *) info_buffer;

	    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[i]);

	    if (llb->llb_type == LLB_TYPE &&
		llb->llb_status & show_status &&
		llb->llb_cpid.pid == pid)
	    {
		*llb_id = llb->llb_id;
		*context = i;
		*info_result = sizeof(LLB_ID);
		break;
	    }
	}
	break;

    case LK_S_RESOURCE:
	{
	    LK_RSB_INFO     *rsbi = (LK_RSB_INFO *)info_buffer;
	    LK_LKB_INFO     *lkbi = ((LK_S_RESOURCE_HEADER *)rsbi)->lkbi;

	    count = info_size - ((PTR)lkbi - (PTR)rsbi);
	    if (count < 0 || (count /=  sizeof(LK_LKB_INFO)) < 0)
	    {
		uleFormat(NULL, E_CL1044_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
			0, count);
		return(LK_BADPARAM);
	    }

	    i = *context + 1;
	    *context = 0;
	    *info_result = 0;

	    rbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_rbk_table);

	    for (; i <= lkd->lkd_rbk_count; i++)
	    {
		rsb = (RSB *)LGK_PTR_FROM_OFFSET(rbk_table[i]);

		if (rsb->rsb_type == RSB_TYPE)
		{
		    if (!dirty_read)
		    {
			if (status = LK_mutex(SEM_SHARE, &rsb->rsb_mutex))
			    return (status);

			/* Recheck RSB after semaphore wait */
			if (rsb->rsb_type != RSB_TYPE)
			{
			    (void)LK_unmutex(&rsb->rsb_mutex);
			    continue;
			}
		    }

		    *context = i;
		    rsbi->rsb_id = rsb->rsb_id;
		    rsbi->rsb_grant = rsb->rsb_grant_mode;
		    rsbi->rsb_convert = rsb->rsb_convert_mode;
		    rsbi->rsb_key[0] = rsb->rsb_name.lk_type;
		    rsbi->rsb_key[1] = rsb->rsb_name.lk_key1;
		    rsbi->rsb_key[2] = rsb->rsb_name.lk_key2;
		    rsbi->rsb_key[3] = rsb->rsb_name.lk_key3;
		    rsbi->rsb_key[4] = rsb->rsb_name.lk_key4;
		    rsbi->rsb_key[5] = rsb->rsb_name.lk_key5;
		    rsbi->rsb_key[6] = rsb->rsb_name.lk_key6;
		    rsbi->rsb_value[0] = rsb->rsb_value[0];
		    rsbi->rsb_value[1] = rsb->rsb_value[1];
		    rsbi->rsb_invalid = rsb->rsb_invalid;

                    rsbi->rsb_cback_count = rsb->rsb_cback_count;

		    /*
		    ** Count the total number of grants on this lock.
		    */
		    rsbi->rsb_count = 0;

		    /* Check this offset has been set up */
		    if (rsb->rsb_grant_q_next)
		    {
		    	end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
		    	for (lkb_offset = rsb->rsb_grant_q_next;
			     lkb_offset != end_offset;
			     lkb_offset = lkb->lkb_q_next)
		    	{
			    lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
			    rsbi->rsb_count += lkb->lkb_count;
		    	}

		    	end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
		    	for (lkb_offset = rsb->rsb_grant_q_next;
			     lkb_offset != end_offset;
			     lkb_offset = lkb->lkb_q_next)
		    	{
			    lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
			    llb = (LLB *)
				LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);
			    if (count == 0)
			    break;

			    lkbi->lkb_id = lkb->lkb_id;
			    lkbi->lkb_rsb_id = rsb->rsb_id;
			    lkbi->lkb_llb_id = *(i4 *)&llb->llb_id;
			    lkbi->lkb_phys_cnt = lkb->lkb_count;
			    lkbi->lkb_grant_mode = lkb->lkb_grant_mode;
			    lkbi->lkb_request_mode = lkb->lkb_request_mode;
			    lkbi->lkb_state = lkb->lkb_state;
			    lkbi->lkb_flags = lkb->lkb_attribute;
			    lkbi->lkb_pid = lkb->lkb_cpid.pid;
			    lkbi->lkb_sid = lkb->lkb_cpid.sid;

			    count--;
			    lkbi++;
		    	}
		    }

		    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_wait_q_next);
		    if (rsb->rsb_wait_q_next)
		    {
		    	for (lkb_offset = rsb->rsb_wait_q_next;
			     lkb_offset != end_offset;
			     lkb_offset = lkb->lkb_q_next)
		    	{
			    lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
			    llb = (LLB *)
				LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);
			    if (count == 0)
			    	break;

			    lkbi->lkb_id = lkb->lkb_id;
			    lkbi->lkb_rsb_id = rsb->rsb_id;
			    lkbi->lkb_llb_id = *(i4 *)&llb->llb_id;
			    lkbi->lkb_phys_cnt = lkb->lkb_count;
			    lkbi->lkb_grant_mode = lkb->lkb_grant_mode;
			    lkbi->lkb_request_mode = lkb->lkb_request_mode;
			    lkbi->lkb_state = lkb->lkb_state;
			    lkbi->lkb_flags = lkb->lkb_attribute;
			    lkbi->lkb_pid = lkb->lkb_cpid.pid;
			    lkbi->lkb_sid = lkb->lkb_cpid.sid;

			    count--;
			    lkbi++;
		    	}
		    }

		    if (!dirty_read)
			(void) LK_unmutex(&rsb->rsb_mutex);

		    *info_result = info_size - count * sizeof(LK_LKB_INFO);
		    break;
		}
	    }
	}
	break;

    case LK_S_LOCKS:
	{
	    LK_LLB_INFO	*llbi = (LK_LLB_INFO *)info_buffer;
	    LK_LKB_INFO *lkbi = ((LK_S_LOCKS_HEADER *)llbi)->lkbi;

	    count = info_size - ((PTR)lkbi - (PTR)llbi);
	    if (count < 0 || (count /=  sizeof(LK_LKB_INFO)) < 0)
	    {
		uleFormat(NULL, E_CL1045_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
			0, count);
		return (LK_BADPARAM);
	    }

	    i = *context + 1;
	    *context = 0;
	    *info_result = 0;

	    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);

	    for (; i <= lkd->lkd_lbk_count; i++)
	    {
		llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[i]);
		if (llb->llb_type == LLB_TYPE)
		{
		    if (!dirty_read)
		    {
			if (status = LK_mutex(SEM_SHARE, &llb->llb_mutex))
			    return(status);
			
			/* Recheck LLB again after semaphore wait */
			if (llb->llb_type != LLB_TYPE)
			{
			    (void)LK_unmutex(&llb->llb_mutex);
			    continue;
			}
		    }

		    *context = i;
		    llbi->llb_id = *(i4 *)&llb->llb_id;
		    llbi->llb_pid = llb->llb_cpid.pid;
		    llbi->llb_sid = llb->llb_cpid.sid;
		    llbi->llb_status = llb->llb_status;
		    llbi->llb_key[0] = llb->llb_name[0];
		    llbi->llb_key[1] = llb->llb_name[1];
		    llbi->llb_r_cnt = llb->llb_related_count;
		    llbi->llb_lkb_count = llb->llb_lkb_count;
		    llbi->llb_llkb_count = llb->llb_llkb_count;
		    llbi->llb_max_lkb = llb->llb_max_lkb;
		    llbi->llb_wait_rsb_id = 0;
		    llbi->llb_r_llb_id = 0;
		    llbi->llb_s_llb_id = 0;
		    llbi->llb_s_cnt = llb->llb_connect_count;
		    llbi->llb_tick = llb->llb_tick;
		    STRUCT_ASSIGN_MACRO(llb->llb_event, llbi->llb_event);
		    llbi->llb_evflags = llb->llb_evflags;
		    if (llb->llb_related_llb)
		    {
			related_llb =
			    (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_related_llb);
			llbi->llb_r_llb_id = *(i4 *)&related_llb->llb_id;
		    }
		    if ( llb->llb_shared_llb )
		    {
			related_llb =
			    (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_shared_llb);
			llbi->llb_s_llb_id = *(i4 *)&related_llb->llb_id;
		    }
		    if ( llb->llb_waiters )
		    {
			llbi->llb_status |= LLB_WAITING;
			/* Take the first waiting LKB from the list */
			llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llb->llb_lkb_wait.llbq_next);
			wait_lkb = (LKB *)((char *)llbq - CL_OFFSETOF(LKB,lkb_llb_wait));
			wait_rsb = (RSB *)
			    LGK_PTR_FROM_OFFSET(wait_lkb->lkb_lkbq.lkbq_rsb);
			llbi->llb_wait_rsb_id =  wait_rsb->rsb_id;
		    }

		    /* Check this pointer exists */
		    if (llb->llb_llbq.llbq_next)
		    {
		    	end_offset = LGK_OFFSET_FROM_PTR(&llb->llb_llbq.llbq_next);
		    	for (llbq_offset = llb->llb_llbq.llbq_next;
			    llbq_offset != end_offset;
			    llbq_offset = llbq->llbq_next)
		    	{
			    llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq_offset);
			    lkb = (LKB *)((char *)llbq - CL_OFFSETOF(LKB,lkb_llbq));
			    rsb = (RSB *)
				LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);
			    my_llb = (LLB *)
				LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);

			    if (count == 0)
			   	 break;

			    lkbi->lkb_id = lkb->lkb_id;
			    lkbi->lkb_rsb_id = rsb->rsb_id;
			    lkbi->lkb_llb_id = *(i4 *)&my_llb->llb_id;
			    lkbi->lkb_phys_cnt = lkb->lkb_count;
			    lkbi->lkb_grant_mode = lkb->lkb_grant_mode;
			    lkbi->lkb_request_mode = lkb->lkb_request_mode;
			    lkbi->lkb_state = lkb->lkb_state;
			    lkbi->lkb_flags = lkb->lkb_attribute;
			    lkbi->lkb_key[0] = rsb->rsb_name.lk_type;
			    lkbi->lkb_key[1] = rsb->rsb_name.lk_key1;
			    lkbi->lkb_key[2] = rsb->rsb_name.lk_key2;
			    lkbi->lkb_key[3] = rsb->rsb_name.lk_key3;
			    lkbi->lkb_key[4] = rsb->rsb_name.lk_key4;
			    lkbi->lkb_key[5] = rsb->rsb_name.lk_key5;
			    lkbi->lkb_key[6] = rsb->rsb_name.lk_key6;
			    CX_EXTRACT_LOCK_ID( &lkbi->lkb_dlm_lkid, \
                                                &lkb->lkb_cx_req.cx_lock_id );
			    lkbi->lkb_dlm_lkvalue[0] =
			     lkb->lkb_cx_req.cx_value[0];
			    lkbi->lkb_dlm_lkvalue[1] =
			     lkb->lkb_cx_req.cx_value[1];
			    lkbi->lkb_dlm_lkvalue[2] =
			     lkb->lkb_cx_req.cx_value[2];
			    lkbi->lkb_dlm_lkvalue[3] =
			     lkb->lkb_cx_req.cx_value[3];
			    lkbi->lkb_pid = lkb->lkb_cpid.pid;
			    lkbi->lkb_sid = lkb->lkb_cpid.sid;
    
			    count--;
			    lkbi++;
		        }
		    }

		    if (!dirty_read)
			(void) LK_unmutex(&llb->llb_mutex);

		    *info_result = info_size - count * sizeof(LK_LKB_INFO);
		    break;
		}
	    }
	}
	break;

    case LK_S_LLB_INFO:
	{
	    llbi = (LK_LLB_INFO *)info_buffer;
	    llb_id = (LLB_ID *) &listid;

	    /* Validate the lock list id. */

	    if (llb_id->id_id == 0 || 
		 (i4) llb_id->id_id > lkd->lkd_lbk_count)
	    {
		uleFormat(NULL, E_DMA030_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, llb_id->id_id, 0, lkd->lkd_lbk_count,
			0, "LK_S_LLB_INFO");
		return (LK_BADPARAM);
	    }

	    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[llb_id->id_id]);

	    if (llb->llb_type != LLB_TYPE ||
		llb->llb_id.id_instance != llb_id->id_instance)
	    {
		uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 5,
			0, llb->llb_type, 0, LLB_TYPE,
			0, *(u_i4*)&llb->llb_id, 0, *(u_i4*)&llb_id,
			0, "LK_S_LLB_INFO");
		return (LK_BADPARAM);
	    }
	    if (info_size < sizeof(LK_LLB_INFO))
	    {
		uleFormat(NULL, E_DMA032_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, info_size, 0, sizeof(LK_LLB_INFO),
			0, "LK_S_LLB_INFO");
		return (LK_BADPARAM);
	    }

	    if (llb->llb_related_llb)
	    {
		related_llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_related_llb);
	    	llbi->llb_r_llb_id = *(i4 *)&related_llb->llb_id;
                llbi->llb_status = *(i4 *)&llb->llb_status;
		*info_result = sizeof(LK_LLB_INFO);
	    }
	    else
		*info_result = 0;	    
	}
	break;

    case LK_S_TRAN_LOCKS:
    case LK_S_REL_TRAN_LOCKS:
	{
	    blkbi = (LK_BLKB_INFO *)info_buffer;
	    llb_id = (LLB_ID *) &listid;

	    count = info_size;

	    if (count < 0 || (count /=  sizeof(LK_BLKB_INFO)) < 0)
	    {
		uleFormat(NULL, E_DMA032_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, count, 0, sizeof(LK_BLKB_INFO),
			0, "LK_S_TRAN_LOCKS/LK_S_REL_TRAN_LOCKS");
		return (LK_BADPARAM);
	    }

	    /* Validate the lock list id. */

	    if (llb_id->id_id == 0 || 
		(i4)llb_id->id_id > lkd->lkd_lbk_count)
	    {
		uleFormat(NULL, E_DMA030_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, llb_id->id_id, 0, lkd->lkd_lbk_count,
			0, "LK_S_TRAN_LOCKS/LK_S_REL_TRAN_LOCKS");
		return (LK_BADPARAM);
	    }

	    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[llb_id->id_id]);

	    if (llb->llb_type != LLB_TYPE ||
		llb->llb_id.id_instance != llb_id->id_instance)
	    {
		uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 5,
			0, llb->llb_type, 0, LLB_TYPE,
			0, *(u_i4*)&llb->llb_id, 0, *(u_i4*)&llb_id,
			0, "LK_S_TRAN_LOCKS/LK_S_REL_TRAN_LOCKS");
		return (LK_BADPARAM);
	    }

	    /* Related LLB wanted? */
	    if (flag == LK_S_REL_TRAN_LOCKS)
	    {
		if (llb->llb_related_llb == 0)
		{
		    uleFormat(NULL, E_DMA033_LK_SHOW_NO_REL_LIST, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 2,
			0, llb->llb_id.id_id,
			0, "LK_S_REL_TRAN_LOCKS");
		    return (LK_BADPARAM);
		}

		llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_related_llb);

		if (llb->llb_type != LLB_TYPE)
		{
		    uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 5,
			0, llb->llb_type, 0, LLB_TYPE,
			0, llb->llb_id.id_id, 0, llb_id->id_id,
			0, "LK_S_REL_TRAN_LOCKS(2)");
		    return (LK_BADPARAM);
		}
	    }
	    /*
	    ** If lock list is a handle to a shared list, then use the actual shared
	    ** list llb, as it's the one actually holding the locks.
	    */
	    else if (llb->llb_status & LLB_PARENT_SHARED)
	    {
		if (llb->llb_shared_llb == 0)
		{
		    uleFormat(NULL, E_DMA034_LK_SHOW_NO_SHR_LIST, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 2,
			0, llb->llb_id.id_id,
			0, "LK_S_TRAN_LOCKS/LK_S_REL_TRAN_LOCKS");
		    return (LK_BADPARAM);
		}

		llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_shared_llb);

		if ((llb->llb_type != LLB_TYPE) ||
		    ((llb->llb_status & LLB_SHARED) == 0))
		{
		    uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 5,
			0, llb->llb_type, 0, LLB_TYPE,
			0, llb->llb_id.id_id, 0, llb_id->id_id,
			0, "LK_S_TRAN_LOCKS/LK_S_REL_TRAN_LOCKS(shared)");
		    return (LK_BADPARAM);
		}
	    }

	    if (!dirty_read)
	    {
		if (status = LK_mutex(SEM_SHARE, &llb->llb_mutex))
		    return(status);
		/* Recheck LLB after semaphore wait */
		if (llb->llb_type != LLB_TYPE)
		{
		    (void)LK_unmutex(&llb->llb_mutex);
		    uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			    &err_code, 5,
			    0, llb->llb_type, 0, LLB_TYPE,
			    0, llb->llb_id.id_id, 0, llb_id->id_id,
			    0, "LK_S_TRAN_LOCKS/LK_S_REL_TRAN_LOCKS");
		    return (LK_BADPARAM);
		}
	    }

	    prev = *context + 1;
	    *context = 0;
	    *info_result = 0;
	    i = 0;

	    end_offset = LGK_OFFSET_FROM_PTR(&llb->llb_llbq.llbq_next);

	    for (llbq_offset = llb->llb_llbq.llbq_next;
		    llbq_offset != end_offset;
		    llbq_offset = llbq->llbq_next)
	    {
		llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq_offset);
		lkb = (LKB *)((char *)llbq - CL_OFFSETOF(LKB,lkb_llbq));
		rsb = (RSB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);
		
		if (++i < prev)
		    continue;

		if (count == 0)
		    break;


		/* Only need to return selective information about LKB. */

		/*
		** dm0l_secure() is the only caller of this option and
		** it culls out all but "write" locks, so we can speed 
		** the process by returning only "write" locks.
		*/

		if ( lkb->lkb_grant_mode == LK_X ||
		     lkb->lkb_grant_mode == LK_U ||
		     lkb->lkb_grant_mode == LK_IX ||
		     lkb->lkb_grant_mode == LK_SIX )
		{
		    *context = i;
		    blkbi->lkb_grant_mode = lkb->lkb_grant_mode;
		    blkbi->lkb_attribute = 
			(lkb->lkb_attribute & LKB_PHYSICAL) ? LOCK_PHYSICAL : 0;
		    blkbi->lkb_key[0] = rsb->rsb_name.lk_type;
		    blkbi->lkb_key[1] = rsb->rsb_name.lk_key1;
		    blkbi->lkb_key[2] = rsb->rsb_name.lk_key2;
		    blkbi->lkb_key[3] = rsb->rsb_name.lk_key3;
		    blkbi->lkb_key[4] = rsb->rsb_name.lk_key4;
		    blkbi->lkb_key[5] = rsb->rsb_name.lk_key5;
		    blkbi->lkb_key[6] = rsb->rsb_name.lk_key6;

		    count--;
		    blkbi++;
		}
	    }

	    if (!dirty_read)
		(void) LK_unmutex(&llb->llb_mutex);

	    *info_result = info_size - count * sizeof(LK_BLKB_INFO);
	}
	break;

    case LK_S_STAT:
	return (fill_in_stat_block( (LK_STAT *)info_buffer, info_size,
				    info_result, lkd ));

    case LK_S_ULOCKS:
	return( (lkd->lkd_status & LKD_NO_ULOCKS) ? FAIL : OK );
    
    case LK_S_SUMM:
	{
	    summ = (LK_SUMM *) info_buffer;

	    if (info_size < sizeof(LK_SUMM))
	    {
		uleFormat(NULL, E_DMA032_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, info_size, 0, sizeof(LK_SUMM),
			0, "LK_S_SUMM");
		return (LK_BADPARAM);
	    }
	    summ->lkb_size	= lkd->lkd_sbk_size + 1;
	    summ->lkbs_per_xact = lkd->lkd_max_lkb;
	    summ->lkb_hash_size	= lkd->lkd_lkh_size;
	    summ->lkbs_inuse	= lkd->lkd_lkb_inuse + lkd->lkd_rsb_inuse;
	    summ->rsb_hash_size	= lkd->lkd_rsh_size;
	    summ->rsbs_inuse	= lkd->lkd_rsb_inuse;
	    summ->llb_size	= lkd->lkd_lbk_size + 1;
	    summ->llbs_inuse	= lkd->lkd_llb_inuse;
	    summ->rsb_size	= lkd->lkd_rbk_size + 1;
	    *info_result = sizeof(LK_SUMM);
	    return (OK);
	}

    case LK_S_OWNER:
    case LK_S_OWNER_GRANT:
    {
	LLB_ID			*llb_id = (LLB_ID *) &listid;
	i4			rsh_offset;
	i4			rsb_offset;

	if (llb_id->id_id == 0 || lock_key == NULL ||
	    (flag == LK_S_OWNER_GRANT && info_size != sizeof(LK_LKB_INFO)))
	{
	    uleFormat(NULL, E_CL1046_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, llb_id->id_id, 0, lock_key);
	    return(LK_BADPARAM);
	}

	if (flag == LK_S_OWNER_GRANT)
	{
	    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[llb_id->id_id]);
	    if (llb->llb_status & LLB_PARENT_SHARED)
	    {
		if (llb->llb_shared_llb == 0)
		{
		    uleFormat(NULL, E_CL1055_LK_REQUEST_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
			0, llb_id->id_id);
		    return (LK_BADPARAM);
		}

		llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_shared_llb);
		if ( llb->llb_type != LLB_TYPE ||
		    (llb->llb_status & LLB_SHARED) == 0 )
		{
		    uleFormat(NULL, E_CL1055_LK_REQUEST_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
				0, llb_id->id_id, 0, llb->llb_type, 
				0, llb->llb_status);
		    return (LK_BADPARAM);
		}

		/* Look for PARENT lock list id */
		llb_id->id_id = llb->llb_id.id_id;
	    }
	}

	/* Get the RSB of the lock. */

	rsb_hash_value = (lock_key->lk_type + lock_key->lk_key1 +
	    lock_key->lk_key2 + lock_key->lk_key3 + lock_key->lk_key4 +
	    lock_key->lk_key5 + lock_key->lk_key6);

	rsh_table = (RSH *)LGK_PTR_FROM_OFFSET(lkd->lkd_rsh_table);
	rsb_hash_bucket = (RSH *)&rsh_table
			[(unsigned)rsb_hash_value % lkd->lkd_rsh_size];

	/*
	**	Search the RSB hash bucket for a matching key.
	*/
	rsh_offset = LGK_OFFSET_FROM_PTR(rsb_hash_bucket);
	rsb_offset = rsb_hash_bucket->rsh_rsb_next;
	matching_rsb = 0;
	while (rsb_offset)
	{
	    rsb = (RSB *)LGK_PTR_FROM_OFFSET(rsb_offset);
	    rsb_offset = rsb->rsb_q_next;

	    if (rsb->rsb_name.lk_type == lock_key->lk_type &&
		rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
		rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
		rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
		rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
		rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
		rsb->rsb_name.lk_key6 == lock_key->lk_key6)
	    {
		if (LK_mutex(SEM_SHARE, &rsb->rsb_mutex))
                    return(FAIL);

		if (rsb->rsb_type == RSB_TYPE &&
		    rsb->rsb_rsh_offset == rsh_offset &&
		    rsb->rsb_name.lk_type == lock_key->lk_type &&
		    rsb->rsb_name.lk_key1 == lock_key->lk_key1 &&
		    rsb->rsb_name.lk_key2 == lock_key->lk_key2 &&
		    rsb->rsb_name.lk_key3 == lock_key->lk_key3 &&
		    rsb->rsb_name.lk_key4 == lock_key->lk_key4 &&
		    rsb->rsb_name.lk_key5 == lock_key->lk_key5 &&
		    rsb->rsb_name.lk_key6 == lock_key->lk_key6)
		{
		    matching_rsb = 1;
		    break;
		}
		/*
            	** RSB changed while we waited for its mutex.
            	** Unlock it and restart the RSH search.
            	*/
		(void)LK_unmutex(&rsb->rsb_mutex);
	    }
	    if (rsb->rsb_type != RSB_TYPE ||
		rsb->rsb_rsh_offset != rsh_offset ||
		rsb->rsb_q_next != rsb_offset)
	    {
		rsb_offset = rsb_hash_bucket->rsh_rsb_next;
	    }
	}

	if (matching_rsb == 0)
	{
	    /* Resource not found. */
	    return(LK_RETRY);
	}
	else
	{
	    /* 
	    ** See which transaction owns the resource. 
	    ** If the transaction that owns the resource is related to
	    ** the input lock list, the return LK_GRANTED so that the caller
	    ** don't need to request the resource.
	    */
	    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	    lkb_offset = rsb->rsb_grant_q_next;

    	    while (lkb_offset != end_offset)
	    {
		next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
		llb = (LLB *)LGK_PTR_FROM_OFFSET(next_lkb->lkb_lkbq.lkbq_llb);

		if (llb->llb_related_llb)
		    related_llb = (LLB *)
			    LGK_PTR_FROM_OFFSET(llb->llb_related_llb);
		else
		    related_llb = (LLB *)NULL;

		if ( (llb->llb_id.id_id == llb_id->id_id) ||
		     (related_llb && related_llb->llb_id.id_id == llb_id->id_id))
		{
		    if (flag == LK_S_OWNER_GRANT)
		    {
			/* Return GRANT mode in LKB */
			lkbi = (LK_LKB_INFO *)info_buffer;
			lkbi->lkb_grant_mode = next_lkb->lkb_grant_mode;
                        lkbi->lkb_flags = next_lkb->lkb_attribute;

			/* Return flags too so caller can tell if it is PHYS */
			lkbi->lkb_flags = next_lkb->lkb_attribute;
			lkbi->lkb_phys_cnt = next_lkb->lkb_count;
			
			*info_result = sizeof(LK_LKB_INFO);
		    }

		    (void) LK_unmutex(&rsb->rsb_mutex);
		    return(OK);
		}
		lkb_offset = next_lkb->lkb_q_next;
	    }
	    (void) LK_unmutex(&rsb->rsb_mutex);

	}
	return(LK_RETRY);
    }

    case LK_S_SLOCK:
    case LK_S_SLOCKQ:
    {
	lkbi = (LK_LKB_INFO *)info_buffer;

	if ((lockid == 0) || (info_size < sizeof(LK_LKB_INFO)))
	{
	    uleFormat(NULL, E_DMA032_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, info_size, 0, sizeof(LK_LKB_INFO),
			0, "LK_S_SLOCK");
	    return (LK_BADPARAM);
	}

	if ( lockid->lk_unique == 0 || 
	     lockid->lk_unique > lkd->lkd_sbk_count )
	{
	    uleFormat(NULL, E_DMA030_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, lockid->lk_unique, 0, lkd->lkd_sbk_count,
			0, "LK_S_SLOCK");
	    return (LK_BADPARAM);
	}

	sbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_sbk_table);
	lkb = (LKB *)LGK_PTR_FROM_OFFSET(sbk_table[lockid->lk_unique]);
	rsb = (RSB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);
	llb = (LLB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);

	/* In SMP machines, when we get here while the lock is being released 
	   the rsb,llb would be initialized to NULL and cause SEGV.
	    Added a check to see if the rsb and llb pointers are not NULL */

	/* Also check that the RSB in the passed lockid->lk_common is this RSB */
	if ( lkb->lkb_type != LKB_TYPE || !llb || !rsb ||
		rsb->rsb_id != lockid->lk_common )
	{
	    /*
	    ** It's a "normal" condition for the DMCM callback thread
	    ** to be using a stale lock id. DMCM uses the LK_S_SLOCKQ
	    ** (Quiet) flag to indicate it wants to know about the error, 
	    ** but not clutter the log with misleading non-error messages.
	    */
	    if (flag != LK_S_SLOCKQ)
	    {
		uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			    &err_code, 5,
			    0, lkb->lkb_type, 0, LKB_TYPE,
			    0, lkb->lkb_id, 0, lockid->lk_unique,
			    0, "LK_S_SLOCK");
	    }
	    return (LK_BADPARAM);
	}

	lkbi->lkb_id = lkb->lkb_id;
	lkbi->lkb_rsb_id = rsb->rsb_id;
	lkbi->lkb_llb_id = *(i4 *)&llb->llb_id;
	lkbi->lkb_phys_cnt = lkb->lkb_count;
	lkbi->lkb_grant_mode = lkb->lkb_grant_mode;
	lkbi->lkb_request_mode = lkb->lkb_request_mode;
	lkbi->lkb_state = lkb->lkb_state;
	lkbi->lkb_flags = lkb->lkb_attribute;
	lkbi->lkb_key[0] = rsb->rsb_name.lk_type;
	lkbi->lkb_key[1] = rsb->rsb_name.lk_key1;
	lkbi->lkb_key[2] = rsb->rsb_name.lk_key2;
	lkbi->lkb_key[3] = rsb->rsb_name.lk_key3;
	lkbi->lkb_key[4] = rsb->rsb_name.lk_key4;
	lkbi->lkb_key[5] = rsb->rsb_name.lk_key5;
	lkbi->lkb_key[6] = rsb->rsb_name.lk_key6;
	lkbi->lkb_dlm_lkvalue[0] = (i4)(SCALARP) lkb->lkb_cback.cback_arg;
	lkbi->lkb_pid = lkb->lkb_cpid.pid;
	lkbi->lkb_sid = lkb->lkb_cpid.sid;

	return (OK);
    }

    case LK_S_SRESOURCE:
    {
	rsbi = (LK_RSB_INFO *)info_buffer;

	if ((lockid == 0) || (info_size < sizeof(LK_RSB_INFO)))
	{
	    uleFormat(NULL, E_DMA032_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, info_size, 0, sizeof(LK_RSB_INFO),
			0, "LK_S_SRESOURCE");
	    return (LK_BADPARAM);
	}

	if ( lockid->lk_common == 0 ||
	     lockid->lk_common > lkd->lkd_rbk_count )
	{
	    uleFormat(NULL, E_DMA030_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, lockid->lk_common, 0, lkd->lkd_rbk_count,
			0, "LK_S_SRESOURCE");
	    return (LK_BADPARAM);
	}

	rbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_rbk_table);
    	rsb = (RSB *)LGK_PTR_FROM_OFFSET(rbk_table[lockid->lk_common]);

	if (!dirty_read)
	{
	    if (status = LK_mutex(SEM_SHARE, &rsb->rsb_mutex))
		return(status);

	    if (rsb->rsb_type != RSB_TYPE)
	    {
		(void)LK_unmutex(&rsb->rsb_mutex);
		uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			    &err_code, 5,
			    0, rsb->rsb_type, 0, RSB_TYPE,
			    0, rsb->rsb_id, 0, lockid->lk_common,
			    0, "LK_S_SRESOURCE");
		return (LK_BADPARAM);
	    }
	}

	rsbi->rsb_id = rsb->rsb_id;
	rsbi->rsb_grant = rsb->rsb_grant_mode;
	rsbi->rsb_convert = rsb->rsb_convert_mode;
	rsbi->rsb_key[0] = rsb->rsb_name.lk_type;
	rsbi->rsb_key[1] = rsb->rsb_name.lk_key1;
	rsbi->rsb_key[2] = rsb->rsb_name.lk_key2;
	rsbi->rsb_key[3] = rsb->rsb_name.lk_key3;
	rsbi->rsb_key[4] = rsb->rsb_name.lk_key4;
	rsbi->rsb_key[5] = rsb->rsb_name.lk_key5;
	rsbi->rsb_key[6] = rsb->rsb_name.lk_key6;
	rsbi->rsb_value[0] = rsb->rsb_value[0];
	rsbi->rsb_value[1] = rsb->rsb_value[1];
	rsbi->rsb_invalid = rsb->rsb_invalid;

        rsbi->rsb_cback_count = rsb->rsb_cback_count;

	/*
	** Count the total number of grants on this lock.
	*/
	rsbi->rsb_count = 0;
	end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	for (lkb_offset = rsb->rsb_grant_q_next;
	     lkb_offset != end_offset;
	     lkb_offset = lkb->lkb_q_next)
	{
	    lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
	    rsbi->rsb_count += lkb->lkb_count;
	}

	if (!dirty_read)
	    (void) LK_unmutex(&rsb->rsb_mutex);

	return (OK);
    }

    case LK_S_LIST:
    {
	llbi = (LK_LLB_INFO *)info_buffer;
	llb_id = (LLB_ID *) &listid;

	if (info_size < sizeof(LK_LLB_INFO))
	{
	    uleFormat(NULL, E_DMA032_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, info_size, 0, sizeof(LK_LLB_INFO),
			0, "LK_S_LIST");
	    return (LK_BADPARAM);
	}

	if (llb_id->id_id == 0 || (i4)llb_id->id_id > lkd->lkd_lbk_count)
	{
	    uleFormat(NULL, E_DMA030_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, llb_id->id_id, 0, lkd->lkd_lbk_count,
			0, "LK_S_LIST");
	   return (LK_BADPARAM);
	}

	lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[llb_id->id_id]);

	if (llb->llb_type != LLB_TYPE ||
	    llb->llb_id.id_instance != llb_id->id_instance)
	{
	    uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 5,
			0, llb->llb_type, 0, LLB_TYPE,
			0, *(u_i4*)&llb->llb_id, 0, *(u_i4*)&llb_id,
			0, "LK_S_LIST");
	    return (LK_BADPARAM);
	}

	/* Extract the related LLB from the "handle" LLB */
	if (llb->llb_related_llb)
	{
	    related_llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_related_llb);
	    llbi->llb_r_llb_id = *(i4 *)&related_llb->llb_id;
	}
	else
	    llbi->llb_r_llb_id = 0;

	/*
	** if this lock list is a handle to a shared list, then return the
	** information about the shared list.
	*/
	if (llb->llb_status & LLB_PARENT_SHARED)
	{
	    if (llb->llb_shared_llb == 0)
	    {
		uleFormat(NULL, E_DMA034_LK_SHOW_NO_SHR_LIST, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    &err_code, 2,
		    0, llb->llb_id.id_id,
		    0, "LK_S_LIST");
		return (LK_BADPARAM);
	    }

	    llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_shared_llb);

	    if ((llb->llb_type != LLB_TYPE) ||
		((llb->llb_status & LLB_SHARED) == 0))
	    {
		uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 5,
			0, llb->llb_type, 0, LLB_TYPE,
			0, llb->llb_id.id_id, 0, llb_id->id_id,
			0, "LK_S_LIST(shared)");
		return (LK_BADPARAM);
	    }
	}

	if (!dirty_read)
	{
	    /* Lock the LLB, then recheck it */
	    if (status = LK_mutex(SEM_SHARE, &llb->llb_mutex))
		return(status);
	    if (llb->llb_type != LLB_TYPE)
	    {
		(void)LK_unmutex(&llb->llb_mutex);
		uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			    &err_code, 5,
			    0, llb->llb_type, 0, LLB_TYPE,
			    0, llb->llb_id.id_id, 0, llb_id->id_id,
			    0, "LK_S_LIST");
		return (LK_BADPARAM);
	    }
	}

	llbi->llb_id = *(i4 *)&llb->llb_id;
	llbi->llb_pid = llb->llb_cpid.pid;
	llbi->llb_sid = llb->llb_cpid.sid;
	llbi->llb_status = llb->llb_status;
	llbi->llb_key[0] = llb->llb_name[0];
	llbi->llb_key[1] = llb->llb_name[1];
	llbi->llb_r_cnt = llb->llb_related_count;
	llbi->llb_s_llb_id = 0;
	llbi->llb_s_cnt = llb->llb_connect_count;
	llbi->llb_lkb_count = llb->llb_lkb_count;
	llbi->llb_llkb_count = llb->llb_llkb_count;
	llbi->llb_max_lkb = llb->llb_max_lkb;
	llbi->llb_wait_rsb_id = 0;
	STRUCT_ASSIGN_MACRO(llb->llb_event, llbi->llb_event);
	llbi->llb_evflags = llb->llb_evflags;
	if ( llb->llb_waiters )
	{
	    llbi->llb_status |= LLB_WAITING;
	    /* Take the first waiting LKB from the list */
	    llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llb->llb_lkb_wait.llbq_next);
	    wait_lkb = (LKB *)((char *)llbq - CL_OFFSETOF(LKB,lkb_llb_wait));
	    wait_rsb = (RSB *)LGK_PTR_FROM_OFFSET(wait_lkb->lkb_lkbq.lkbq_rsb);
	    llbi->llb_wait_rsb_id = wait_rsb->rsb_id;
	}

	if (!dirty_read)
	    (void)LK_unmutex(&llb->llb_mutex);
	return (OK);
    }

    case LK_S_LIST_NAME:
    {
	/*
	** Find LLB matching on "llb_name", return its lock list id.
	**
	** The input parm "lockid" holds the 2-part "list name".
	*/
	
	i4	llb_offset;
	LLB_ID  *lock_list_id = (LLB_ID *)info_buffer;

	if (info_size < sizeof(LLB_ID))
	{
	    uleFormat(NULL, E_DMA032_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, info_size, 0, sizeof(LLB_ID),
			0, "LK_S_LIST_NAME");
	    return (LK_BADPARAM);
	}

	if (lockid->lk_unique == 0 || lockid->lk_common == 0)
	{
	    uleFormat(NULL, E_DMA030_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, lockid->lk_unique, 0, lockid->lk_common,
			0, "LK_S_LIST_NAME");
	   return (LK_BADPARAM);
	}

	for (llb_offset = lkd->lkd_llb_prev;
	     llb_offset != LGK_OFFSET_FROM_PTR(&lkd->lkd_llb_next);
	     llb_offset = llb->llb_q_prev)
	{
	    llb = (LLB *)LGK_PTR_FROM_OFFSET(llb_offset);

	    if (llb->llb_name[0] == lockid->lk_unique &&
		llb->llb_name[1] == lockid->lk_common)
	    {
		*lock_list_id = llb->llb_id;
		return(OK);
	    }
	}
	lock_list_id->id_instance = 0;
	lock_list_id->id_id = 0;
	return(LK_BADPARAM);
    }

    case LK_S_LIST_LOCKS:
    {
	/*
	** LK_S_LIST_LOCKS - Return locks held by a specified lock list.
	**
	** Returns an array of LK_LKB_INFO structures describing the locks
	** held by the specified lock list.
	**
	** The number of lock information blocks returned is calculated by
	** info_result / sizeof(LK_LKB_INFO).
	**
	** The caller may need to call this routine multiple times in order
	** to return information on all the locks held by the lock list.
	** When LKshow returns zero bytes in info_result or a context value
	** of zero then all information has been returned.
	**
	** NOTE that the Locking System semaphore is not held between calls to
	** LKshow, so when multiple calls are needed it is possible that the
	** locks held by the lock list will change.  Unless the caller knows
	** that no lock calls can be made on the lock list between LKshow calls
	** then it is possible that locks held on the lock list might either be
	** skipped or reported twice.
	**
	** inputs:
	**	listid		- lock list to show
	**	info_size	- size of info_buffer in bytes
	**	info_buffer	- buffer in which to return information
	**	context		- used internally to keep track of information
	**			  returned when LKshow must be called multiple
	**			  times to return all the information.  Should
	**			  be initialized to zero on the first call.
	**
	** outputs:
	**	info_buffer	- filled in with an array of LK_LKB_INFO structs
	**	info_result	- number of bytes written into info_buffer
	**	context		- set with marker giving place to continue on
	**			  next call to LKshow if more information is
	**			  needed.
	*/

	llb_id = (LLB_ID *) &listid;

	if (llb_id->id_id == 0 || (i4)llb_id->id_id > lkd->lkd_lbk_count)
	{
	    uleFormat(NULL, E_DMA030_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 3,
		0, llb_id->id_id, 0, lkd->lkd_lbk_count, 0, "LK_S_LIST_LOCKS");
	   return (LK_BADPARAM);
	}

	lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[llb_id->id_id]);

	if (llb->llb_type != LLB_TYPE ||
	    llb->llb_id.id_instance != llb_id->id_instance)
	{
	    uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 5,
		0, llb->llb_type, 0, LLB_TYPE,
		0, *(u_i4*)&llb->llb_id, 0, *(u_i4*)&llb_id,
		0, "LK_S_LIST");
	    return (LK_BADPARAM);
	}

	/*
	** if this lock list is a handle to a shared list, then return the
	** information about the shared list.
	*/
	if (llb->llb_status & LLB_PARENT_SHARED)
	{
	    if (llb->llb_shared_llb == 0)
	    {
		uleFormat(NULL, E_DMA034_LK_SHOW_NO_SHR_LIST, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &err_code, 2, 0, llb->llb_id.id_id, 
		    0, "LK_S_LIST");
		return (LK_BADPARAM);
	    }

	    llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_shared_llb);

	    if ((llb->llb_type != LLB_TYPE) ||
		((llb->llb_status & LLB_SHARED) == 0))
	    {
		uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &err_code, 5, 0, llb->llb_type, 0, LLB_TYPE,
		    0, llb->llb_id.id_id, 0, llb_id->id_id,
		    0, "LK_S_LIST(shared)");
		return (LK_BADPARAM);
	    }
	}

	if (info_size < sizeof(LK_LLB_INFO))
	{
	    uleFormat(NULL, E_CL1045_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 1, 
		0, info_size);
	    return (LK_BADPARAM);
	}

	/*
	** Get starting point so we can continue where we left off on the
	** last call.  We will cycle through below until we get to the
	** starting lkb and then begin returning info.  On the first call
	** to LKshow, context will be zero so we will start with the first lkb.
	*/
	starting_lkb = *context;

	if (!dirty_read)
	{
	    if (status = LK_mutex(SEM_SHARE, &llb->llb_mutex))
		return(status);
	    /* Recheck LLB after mutex wait */
	    if (llb->llb_type != LLB_TYPE)
	    {
		(void)LK_unmutex(&llb->llb_mutex);
		uleFormat(NULL, E_DMA031_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 5,
		    0, llb->llb_type, 0, LLB_TYPE, 0, llb->llb_id.id_id,
		    0, llb_id->id_id, 0, "LK_S_LIST");
		return (LK_BADPARAM);
	    }
	}

	count = 0;
	lkb_idx = 0;
	lkbi = (LK_LKB_INFO *) info_buffer;
	end_offset = LGK_OFFSET_FROM_PTR(&llb->llb_llbq.llbq_next);
	count_max = info_size / sizeof(LK_LKB_INFO);

	/* If context is greater than the current number of LKBs, skip the scan */
	if (starting_lkb < llb->llb_lkb_count)
	{
	    for (llbq_offset = llb->llb_llbq.llbq_next;
		llbq_offset != end_offset;
		llbq_offset = llbq->llbq_next)
	    {
		llbq   = (LLBQ *) LGK_PTR_FROM_OFFSET(llbq_offset);
		lkb    = (LKB *) ((char *)llbq - CL_OFFSETOF(LKB,lkb_llbq));
		rsb    = (RSB *) LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);
		my_llb = (LLB *) LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);

		/*
		** If this is a continuing call to return more locks for this
		** lock list, then cycle through the list until we get to the
		** lkb that we left off at.
		*/
		if (lkb_idx++ < starting_lkb)
		    continue;

		lkbi->lkb_id = lkb->lkb_id;
		lkbi->lkb_rsb_id = rsb->rsb_id;
		lkbi->lkb_llb_id = *(i4 *)&my_llb->llb_id;
		lkbi->lkb_phys_cnt = lkb->lkb_count;
		lkbi->lkb_grant_mode = lkb->lkb_grant_mode;
		lkbi->lkb_request_mode = lkb->lkb_request_mode;
		lkbi->lkb_state = lkb->lkb_state;
		lkbi->lkb_flags = lkb->lkb_attribute;
		lkbi->lkb_key[0] = rsb->rsb_name.lk_type;
		lkbi->lkb_key[1] = rsb->rsb_name.lk_key1;
		lkbi->lkb_key[2] = rsb->rsb_name.lk_key2;
		lkbi->lkb_key[3] = rsb->rsb_name.lk_key3;
		lkbi->lkb_key[4] = rsb->rsb_name.lk_key4;
		lkbi->lkb_key[5] = rsb->rsb_name.lk_key5;
		lkbi->lkb_key[6] = rsb->rsb_name.lk_key6;
		CX_EXTRACT_LOCK_ID( &lkbi->lkb_dlm_lkid, \
				    &lkb->lkb_cx_req.cx_lock_id );
		lkbi->lkb_dlm_lkvalue[0] = lkb->lkb_cx_req.cx_value[0];
		lkbi->lkb_dlm_lkvalue[1] = lkb->lkb_cx_req.cx_value[1];
		lkbi->lkb_dlm_lkvalue[2] = lkb->lkb_cx_req.cx_value[2];
		lkbi->lkb_dlm_lkvalue[3] = lkb->lkb_cx_req.cx_value[3];
		lkbi->lkb_pid = lkb->lkb_cpid.pid;
		lkbi->lkb_sid = lkb->lkb_cpid.sid;
		lkbi++;

		/*
		** If we have completely filled the user buffer then we can't
		** return any more locks.  The user will have to call LKshow
		** again to get more information.
		*/
		if (++count >= count_max)
		    break;
	    }
	}

	if (!dirty_read)
	    (void) LK_unmutex(&llb->llb_mutex);

	/*
	** Return the spot at which we left off.  If LKshow is called again
	** this context value will tell us where to continue returning locks.
	** If we have reported all locks on the lock list (the above loop
	** terminated because we reached the end of the list, not because
	** we ran out of room in the caller's buffer) then set context back
	** to zero so the caller knows we are finished.
	*/
	*context = lkb_idx;
	if (count < count_max)
	    *context = 0;

	*info_result = count * sizeof(LK_LKB_INFO);

	return (OK);
    }


    case LK_S_MUTEX:
    {
	mstat = (LK_MUTEX *) info_buffer;

	mstat->count = 0;

	if (info_size < sizeof(LK_MUTEX))
	{
	    uleFormat(NULL, E_DMA032_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    &err_code, 3,
		    0, info_size, 0, sizeof(LK_MUTEX),
		    0, "LK_S_MUTEX");
	    return (LK_BADPARAM);
	}

	lstat = &mstat->stats[mstat->count];

	if ((CSs_semaphore(CS_INIT_SEM_STATS, 
				(CS_SEMAPHORE*)&lkd->lkd_ew_q_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, 
				(CS_SEMAPHORE*)&lkd->lkd_dw_q_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, 
				(CS_SEMAPHORE*)&lkd->lkd_lbk_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, 
				(CS_SEMAPHORE*)&lkd->lkd_sbk_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, 
				(CS_SEMAPHORE*)&lkd->lkd_rbk_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, 
				(CS_SEMAPHORE*)&lkd->lkd_llb_q_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, 
				(CS_SEMAPHORE*)&lkd->lkd_stall_q_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	status = FAIL;
	MEfill(sizeof(*lstat), 0, (char *)lstat);
	lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	for (i = 1; i <= lkd->lkd_lbk_count; i++)
	{
	    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[i]);
	    if (status = CSs_semaphore(CS_ROLL_SEM_STATS, 
	    				(CS_SEMAPHORE*)&llb->llb_mutex,
					lstat, sizeof(*lstat))) 
		break;
	}
	if (status == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	status = FAIL;
	MEfill((sizeof(*lstat) * 2), 0, (char *)lstat);
	rbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_rbk_table);
	for (i = 1; i <= lkd->lkd_rbk_count; i++)
	{
	    rsb = (RSB *)LGK_PTR_FROM_OFFSET(rbk_table[i]);
	    if (status = CSs_semaphore(CS_ROLL_SEM_STATS, 
	    				(CS_SEMAPHORE*)&rsb->rsb_mutex,
					lstat, sizeof(*lstat)))
		break;
	}
	if (status == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	status = FAIL;
	MEfill(sizeof(*lstat), 0, (char *)lstat);
	lkh_table = (LKH *)LGK_PTR_FROM_OFFSET(lkd->lkd_lkh_table);
	for (i = 0; i < lkd->lkd_lkh_size; i++)
	{
	    lkh = (LKH *) &lkh_table[i];
	    if (status = CSs_semaphore(CS_ROLL_SEM_STATS, 
	    				(CS_SEMAPHORE*)&lkh->lkh_mutex, 
					lstat, sizeof(*lstat)))
		break;
	}
	if (status == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	status = FAIL;
	MEfill(sizeof(*lstat), 0, (char *)lstat);
	rsh_table = (RSH *)LGK_PTR_FROM_OFFSET(lkd->lkd_rsh_table);
	for (i = 0; i < lkd->lkd_rsh_size; i++)
	{
	    rsh = (RSH *) &rsh_table[i];
	    if (status = CSs_semaphore(CS_ROLL_SEM_STATS, 
	    				(CS_SEMAPHORE*)&rsh->rsh_mutex, 
					lstat, sizeof(*lstat)))
		break;
	}
	if (status == OK)
	{
	    mstat->count++;
	}
	break;
    }


    default:
	
	uleFormat(NULL, E_CL1047_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
			0, flag);

	return(LK_BADPARAM);
    }
    return (OK);
}

/*
** History:
**	24-may-1993 (bryanp)
**	    Pulled out of the main body of LK_show to routine function size.
**	03-jan-1996 (duursma) bug #71334
**	    unsent_gdlck_search no longer used; set corresponding field in
**	    stat to 0.
**	11-Apr-1997 (jenjo02)
**	    Added new stats for callback threads.
**	08-Sep-1998 (jenjo02)
**	    Added new statistic, max locks allocated per transaction.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: add LK_TSTAT, stats by lock type.
**
*/
static STATUS
fill_in_stat_block( LK_STAT *stat, i4  info_size, u_i4 *info_result, LKD *lkd )
{
    i4	    err_code;

    LK_WHERE("fill_in_stat_block")

    if (info_size < sizeof(LK_STAT))
    {
	uleFormat(NULL, E_DMA032_LK_SHOW_BAD_PARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			&err_code, 3,
			0, info_size, 0, sizeof(LK_STAT),
			0, "LK_S_STATISTICS");
	return (LK_BADPARAM);
    }

    stat->create = lkd->lkd_stat.create_list;
    stat->request = lkd->lkd_tstat[0].request_new;
    stat->re_request = lkd->lkd_tstat[0].request_convert;
    stat->convert = lkd->lkd_tstat[0].convert;
    stat->release = lkd->lkd_tstat[0].release;
    stat->escalate = lkd->lkd_stat.release_partial;
    stat->destroy = lkd->lkd_stat.release_all;
    stat->wait = lkd->lkd_tstat[0].wait;
    stat->con_wait = lkd->lkd_tstat[0].convert_wait;
    stat->con_search = lkd->lkd_stat.convert_search;
    stat->con_dead = lkd->lkd_tstat[0].convert_deadlock;
    stat->dead_search = lkd->lkd_stat.deadlock_search;
    stat->deadlock = lkd->lkd_tstat[0].deadlock;
    stat->cancel = lkd->lkd_stat.cancel;	    
    stat->allocate_cb = lkd->lkd_stat.allocate_cb;
    stat->deallocate_cb = lkd->lkd_stat.deallocate_cb;
    stat->sbk_highwater = lkd->lkd_stat.sbk_highwater;
    stat->lbk_highwater = lkd->lkd_stat.lbk_highwater;
    stat->rbk_highwater = lkd->lkd_stat.rbk_highwater;
    stat->rsb_highwater = lkd->lkd_stat.rsb_highwater;
    stat->lkb_highwater = lkd->lkd_stat.lkb_highwater;
    stat->llb_highwater = lkd->lkd_stat.llb_highwater;
    stat->max_lcl_dlk_srch = lkd->lkd_stat.max_lcl_dlk_srch;
    stat->dlock_locks_examined = lkd->lkd_stat.dlock_locks_examined;
    stat->max_rsrc_chain_len = lkd->lkd_stat.max_rsrc_chain_len;
    stat->max_lock_chain_len = lkd->lkd_stat.max_lock_chain_len;
    stat->dlock_wakeups = lkd->lkd_stat.dlock_wakeups;
    stat->dlock_max_q_len = lkd->lkd_stat.dlock_max_q_len;
    stat->cback_wakeups = lkd->lkd_stat.cback_wakeups;
    stat->cback_cbacks = lkd->lkd_stat.cback_cbacks;
    stat->cback_stale = lkd->lkd_stat.cback_stale;
    stat->max_lkb_per_txn = lkd->lkd_stat.max_lkb_per_txn;

    /*
    ** cluster statistics are always returned, but only contain
    ** useful information in a cluster installation
    */
    stat->enq = lkd->lkd_stat.enq;
    stat->deq = lkd->lkd_stat.deq;
    stat->gdlck_search = lkd->lkd_stat.gdlck_search;
    stat->gdeadlock = lkd->lkd_stat.gdeadlock;
    stat->gdlck_grant = lkd->lkd_stat.gdlck_grant;
    stat->totl_gdlck_search = lkd->lkd_stat.totl_gdlck_search;
    stat->gdlck_call = lkd->lkd_stat.gdlck_call;
    stat->gdlck_sent = lkd->lkd_stat.gdlck_sent;
    stat->cnt_gdlck_call = lkd->lkd_stat.cnt_gdlck_call;
    stat->cnt_gdlck_sent = lkd->lkd_stat.cnt_gdlck_sent;
    stat->unsent_gdlck_search = 0;
    stat->sent_all = lkd->lkd_stat.sent_all;
    stat->synch_complete = lkd->lkd_stat.synch_complete;
    stat->asynch_complete = lkd->lkd_stat.asynch_complete;
    stat->csp_msgs_rcvd = lkd->lkd_stat.csp_msgs_rcvd;
    stat->csp_wakeups_sent = lkd->lkd_stat.csp_wakeups_sent;

    /* Bulk copy stats by lock type */
    MEcopy((PTR)&lkd->lkd_tstat, sizeof(LK_TSTAT) * (LK_MAX_TYPE+1),
    		(PTR)&stat->tstat);

    *info_result = sizeof(LK_STAT);
    return (OK);
}


/*{
** Name: LKkey_to_string - standard lock key to string formatter.
**
** Description:
**      This routine is used to format the value of a lock key into
**	a displayable form for use with IPM, lockstat, debug tracing,
**	etc.  It replaces a number of separate similar routines
**	which have been cloned from each other over the years.
**
** Inputs:
**      key	- pointer to LK_LOCK_KEY 
**	buffer	- buffer to receive ascii version of key.  Buffer
**	          should be at least 128 bytes in size to be safe.
**
** Outputs:
**	buffer	- filled in with '\0' terminator.
**
**	Returns:
**	    Address of buffer, so LKkey_to_string can be used to
**	    directly satisfy a "%s" format spec.
**
**	Exceptions:
**	   None.
**
** Side Effects:
**	    none
**
** History:
** 	04-sep-2002 (devjo01)
**	    "Created" by converting portion of dmd_lock_info() used to
**	    display lock keys for lockstat into a simple formatter.
**	11-Sep-2003 (devjo01)
**	    Add LK_SEQUENCE and LK_XA_CONTROL to LKkey_to_string.
**	26-Jan-2004 (jenjo02)
**	    Remove DB_TAB_PARTITION_MASK bit from lk_key3
**	    for PAGE/ROW locks
**	    unless temp table (lk_key2 < 0).
**	6-Feb-2004 (schka240
**	    Printf isn't trdisplay, use %s not %t.
**	23-Feb-2004 (thaju02)
**	    Add LK_ONLN_MDFY.
**	10-Dec-2007 (jonj)
**	    Repair LK_CKP_CLUSTER display.
**	07-feb-2008 (joea)
**	    Add LK_CSP, LK_CMO and LK_MSG.
**	04-Apr-2008 (jonj)
**	    Show partition table id's in a more readable form,
**	    "-lk_key3" instead of a large negative number.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add LK_CROW, LK_TBL_MVCC lock types.
**	11-Jun-2010 (jonj) Bug 123896
**	    Add distinct display of LK_SEQUENCE.
*/
char *
LKkey_to_string( LK_LOCK_KEY *key, char *buffer )
{
    char	*bp, *vp;
    char	tbuf1[64];
    char	tbuf2[20];


    switch(key->lk_type)
    {
    case LK_DATABASE:
	vp = "DATABASE";
	break;
    case LK_TABLE:
	vp = "TABLE";
	break;
    case LK_PAGE:
	vp = "PAGE";
	break;
    case LK_EXTEND_FILE:
	vp = "EXTEND";
	break;
    case LK_BM_PAGE:
	vp = "SV_PAGE";
	break;
    case LK_CREATE_TABLE:
	vp = "CREATE";
	break;
    case LK_OWNER_ID:
	vp = "NAME";
	break;
    case LK_CONFIG:
	vp = "CONFIG";
	break;
    case LK_DB_TEMP_ID:
	vp = "DB_TBL_ID";
	break;
    case LK_SV_DATABASE:
	vp = "SV_DATABASE";
	break;
    case LK_SV_TABLE:
	vp = "SV_TABLE";
	break;
    case LK_SS_EVENT:
	vp = "EVENT";
	break;
    case LK_TBL_CONTROL:
	vp = "CONTROL";
	break;
    case LK_JOURNAL:
	vp = "JOURNAL";
	break;
    case LK_OPEN_DB:
	vp = "OPEN_DB";
	break;
    case LK_CKP_DB:
	vp = "CKP_DB";
	break;
    case LK_CKP_CLUSTER:
	vp = "CKP_CLUSTER";
	break;
    case LK_BM_LOCK:
	vp = "BM_LOCK";
	break;
    case LK_BM_DATABASE:
	vp = "BM_DATABASE";
	break;
    case LK_BM_TABLE:
	vp = "BM_TABLE";
	break;
    case LK_CONTROL:
	vp = "SYS_CONTROL";
	break;
    case LK_EVCONNECT:
	vp = "EV_CONNECT";
	break;
    case LK_AUDIT:
	vp = "AUDIT";
	break;
    case LK_ROW:
	vp = "ROW";
	break;
    case LK_CROW:
	vp = "CROW";
	break;
    case LK_CKP_TXN:
	vp = "CKP_TXN";
	break;
    case LK_PH_PAGE:
	vp = "PH_PAGE";
	break;
    case LK_VAL_LOCK:
	vp = "VALUE";
	break;
    case LK_SEQUENCE:
	vp = "SEQUENCE";
	break;
    case LK_XA_CONTROL:
	vp = "XA_CONTROL";
	break;
    case LK_ONLN_MDFY:
	vp = "ONLINE_MODIFY";
	break;
    case LK_CMO:
	vp = "LK_CMO";
	break;
    case LK_CSP:
	vp = "LK_CSP";
	break;
    case LK_MSG:
	vp = "LK_MSG";
	break;
    case LK_TBL_MVCC:
	vp = "MVCC";
	break;
    default:
	vp = tbuf1;
	STprintf(vp, "%d", key->lk_type );
	break;
    }

    STcopy( vp, buffer );
    bp = buffer + STlength(buffer);
    *bp++ = ',';

    switch(key->lk_type)
    {
    case LK_DATABASE:
    case LK_SV_DATABASE:
    case LK_OWNER_ID:
    case LK_JOURNAL:
    case LK_DB_TEMP_ID:
    case LK_CONFIG:
    case LK_OPEN_DB:
    case LK_CONTROL:
	MEcopy(&key->lk_key1, 24, tbuf1);
	tbuf1[24] = '\0';
	STcopy(tbuf1, bp);
	STtrmwhite(bp);
	break;

    case LK_CKP_CLUSTER:
	if ( key->lk_key3 == CKP_CSP_INIT )
	{
	    TRformat(NULL, 0, bp, 64,
		"NODE=%d,DB=%x,PHASE=%w,NODE=%d\0",
		key->lk_key1, 
		key->lk_key2, 
		CKP_PHASE_NAME, key->lk_key3,
		key->lk_key4);
	}
	else
	{
	    TRformat(NULL, 0, bp, 64,
		"NODE=%d,DB=%x,PHASE=%w\0",
		key->lk_key1, 
		key->lk_key2, 
		CKP_PHASE_NAME, key->lk_key3);
	}
	break;

    case LK_CKP_TXN:
	MEcopy(&key->lk_key1, 20, tbuf1);
	tbuf1[20] = '\0';
	STtrmwhite(tbuf1);
	/* key1-2-3-4-5 1st 20 bytes of dbname, key6 dbid */
	STprintf(bp, "NAME='%s %d'",tbuf1, key->lk_key6);
	break;

    case LK_TABLE:
    case LK_SV_TABLE:
    case LK_TBL_CONTROL:
    case LK_EXTEND_FILE:
    case LK_BM_TABLE:
    case LK_ONLN_MDFY:
    case LK_TBL_MVCC:
	STprintf(bp,"DB=%x,TABLE=[%d,%d]", 
	key->lk_key1, key->lk_key2, key->lk_key3);
	break;

    case LK_PAGE:
    case LK_BM_PAGE:
    case LK_PH_PAGE:
	STprintf(bp,"DB=%x,TABLE=[%d%s%d],PAGE=%d",
	key->lk_key1, key->lk_key2, 
	(key->lk_key3 & DB_TAB_PARTITION_MASK && key->lk_key2 >= 0)
	    ? ",-"
	    : ",",
	(key->lk_key2 < 0)
	    ? key->lk_key3
	    : key->lk_key3 & ~DB_TAB_PARTITION_MASK,
	key->lk_key4);
	break;

    case LK_SS_EVENT:
	STprintf(bp,"SERVER=%x,[%x,%x]", key->lk_key1,
	    key->lk_key2, key->lk_key3);
	break;

    case LK_CREATE_TABLE:
    case LK_CKP_DB:
	MEcopy(&key->lk_key4, 12, tbuf1);
	tbuf1[12] = '\0';
	STtrmwhite(tbuf1);
	MEcopy(&key->lk_key2, 8, tbuf2);
	tbuf2[8] = '\0';
	STtrmwhite(tbuf2);
	STprintf(bp,"DB=%x,NAME='%s-%s'", 
		key->lk_key1,tbuf1,tbuf2);
	break;

    case LK_BM_LOCK:
	MEcopy(&key->lk_key1, 8, tbuf1);
	tbuf1[8] = '\0';
	STtrmwhite(tbuf1);
	STprintf(bp,"BUFMGR='%s'", tbuf1);
	break;

    case LK_BM_DATABASE:
	STprintf(bp,"DB=%x", key->lk_key1);
	break;

    case LK_EVCONNECT:
	STprintf(bp,"SERVER=%x", key->lk_key1);
	break;

    case LK_AUDIT:
	/*
	** Note: The interpretation of the AUDIT locks depend
	** on the SXF implementation. If SXF changes, then this
	** section should be updated as well
	*/
	switch (key->lk_key1)
	{
	case SXAS_LOCK:
	    switch ( key->lk_key2)
	    {
	    case SXAS_STATE_LOCK:
		vp = "PRIMARY";
		break;
	    case SXAS_SAVE_LOCK:
		vp = "SAVE";
		break;
	    default:
		vp = tbuf1;
		STprintf( vp, "OPER=%d", key->lk_key2 );
	    }
	    STprintf(bp,"STATE,%s",vp);
	    break;
	
	case SXAP_LOCK:
	    vp = tbuf1;

	    switch (key->lk_key2)
	    {
	    case SXAP_SHMCTL:
		STprintf(vp,"SHMCTL,NODE=%x", key->lk_key3);
		break;
	    case SXAP_ACCESS:
		vp = "ACCESS";
		break;
	    case SXAP_QUEUE:
		STprintf(vp,"QUEUE,NODE=%x", key->lk_key3);
		break;
	    default:
		STprintf(vp,"OPER=%d", key->lk_key2);
	    }
	    STprintf(bp, "PHYS_LAYER,%s", vp);
	    break;
	
	default:
	    STprintf(bp,"TYPE=%d,OPER=%d", key->lk_key1, key->lk_key2);
	}
	break;

    case LK_ROW:
    case LK_CROW:
	STprintf(bp,"DB=%x,TABLE=[%d%s%d],PAGE=%d,LINE=%d",
	key->lk_key1, key->lk_key2, 
	(key->lk_key3 & DB_TAB_PARTITION_MASK && key->lk_key2 >= 0)
	    ? ",-"
	    : ",",
	(key->lk_key2 < 0)
	    ? key->lk_key3
	    : key->lk_key3 & ~DB_TAB_PARTITION_MASK,
	key->lk_key4, key->lk_key5);
	break;

    case LK_VAL_LOCK:
	STprintf(bp,"DB=%x,TABLE=[%d,%d],VALUE=<%d,%d,%d>",
	    key->lk_key1, key->lk_key2,key->lk_key3,
	    key->lk_key4, key->lk_key5, key->lk_key6);
	break;

    case LK_CSP:
	STprintf(bp, "NODE=%d,PID=%x", key->lk_key1, key->lk_key2);
	break;

    case LK_CMO:
	STprintf(bp, "CMO=%d", key->lk_key1);
	break;

    case LK_MSG:
	STprintf(bp, "CHAN=%d,%s", key->lk_key1 >> 2,
                 ((key->lk_key1 & 3) == 0) ? "HELLO"
                 : ((key->lk_key1 & 3) == 1) ? "GOODBYE" : "SEND");
	break;

    case LK_SEQUENCE:
	STprintf(bp,"DB=%x,SEQUENCE=%d", 
	key->lk_key1, key->lk_key2);
	break;

    default:
	STprintf(bp,"(%x,%x,%x,%x,%x,%x)",
	    key->lk_key1,key->lk_key2,key->lk_key3,
	    key->lk_key4,key->lk_key5,key->lk_key6); 
    }
    return (buffer);
}
