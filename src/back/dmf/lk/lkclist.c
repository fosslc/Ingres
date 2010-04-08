/*
**Copyright (c) 1992, 2007 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <lk.h>
#include    <cx.h>
#include    <lkdef.h>
#include    <lgkdef.h>
#include    <tr.h>

/*
NO_OPTIM = rs4_us5 ris_u64 i64_aix r64_us5
*/

/**
**
**  Name: LKCLIST.C - Implements LKcreate_list in the locking system
**
**  Description:
**	This module contains the code which implements:
**	
**	    LKcreate_list -- create a new lock list.
**	    LKconnect     -- Connect to an existing lock list.
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	1-Apr-1993 (daveb)
**	    When creating a list, Init the llb_sid field so it's
**	    always valid.
**	24-may-1993 (bryanp)
**	    Clear the CL_ERR_DESC upon entry, so that garbage system errors
**		aren't reported in case of an LK error.
**	    Optimize away PCpid calls by using LK_my_pid global variable.
**	    Maintain additional statistics on lock list highwater marks, etc.
**	30-jun-1993 (shailaja)
**	    Fixed compiler warnings on ANSI C semantics change.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    All lock lists created by the CSP get marked LLB_MASTER_CSP, even
**		if the caller forgot to pass in the LK_MASTER_CSP flag.
**	    Added some casts to pacify compilers.
**	23-aug-1993 (bryanp)
**	    When adopting ownership of a lock list using the LK_RECOVER flag,
**		we must free the new LLB we allocated so that we don't leak.
**	31-jan-1994 (rogerk)
**	    Fixed problem setting the llb_sid field to the caller's session
**	    id in LK_RECOVER calls.
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Changed LK_xmutex() calls to pass the mutex address.
**		- Removed LKD semaphore acquisition in LKcreate_list().
**		- Acquire LKD semaphore in LK_create_list() before
**		  searching active transaction chain for insertion point.
**		- Acquire LKD and LLB semaphores in LK_create_list()
**		  when a process-specific connection to an existing shared
**		  LLB is made.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	 4-sep-1995 (dougb)
**	    Init new llb_gsearch_count field.
**	12-Jan-1996 (jenjo02)
**	    Moved assignment of llb_name to LK_allocate_cb() where
**	    it's done under the protection of the lkb_mutex.
**	30-may-1996 (pchang)
**	    Added LLB as a parameter to the call to LK_allocate_cb() to enable
**	    the reservation of SBK table resource for use by the recovery
**	    process. (B76879)
**	11-Oct-1996 (jenjo02)
**	    When inheriting a lock list using LK_RECOVER, also update the
**	    pid and sid of its related lock list, if any.
**      27-feb-1997 (stial01)
**          Added LK_RECOVER_LLID code.
**	16-Nov-1998 (jenjo02)
**	    Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	08-Feb-1999 (jenjo02)
**	    Initialize (new) llb_lkb_wait queue.
**	06-May-1999 (matbe01)
**	    Added NO_OPTIM for rs4_us5 to eliminate error when trying to
**	    bring up recovery server.
**	21-May-1999 (hweho01)
**	    Added NO_OPTIM for ris_u64 to eliminate error when trying to
**	    bring up recovery server.
**	02-Aug-1999 (jenjo02)
**	    Added LLB parm to LK_deallocate_cb() prototype.
**	06-Oct-1999 (jenjo02)
**	    LLBs identifiers now defined by LLB_ID structure, not LK_ID.
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID* parm to prototype, support for intrinsically
**	    shared XA lock lists.
**	    Associate "related" lock lists to the PSHARED (referencing)
**	    locklist instead of the SHARED (referenced) lock list.
**	15-Dec-1999 (jenjo02)
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      25-Jul-2003 (hweho01)
**          Added r64_us5 to NO_OPTIM list, fixed E_DM901D_BAD_LOCK_SHOW
**          error during HandoffQA replicator backup test.
**           Compiler : VisualAge 5.023.
**	17-Dec-2003 (jenjo02)
**	    Removed unneeded LK_create_list() static function,
**	    added LKconnect() function for Partitioned/Parallel Table
**	    project.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**	09-Jan-2009 (jonj)
**	    Make LLB_MULTITHREAD an attribute of the LLB holding the locks,
**	    telling all that there may be multiple concurrent threads
**	    accessing the LLB.
*/

/*
** Forward declarations for static functions:
*/

/*{
** Name: LKcreate_list	- Create a lock list.
**
** Description:
**      This function associates a unique identifier with a list that can 
**      contain a list of locks.  Deadlock detection is performed between 
**      lists, as this is the way of identifying different threads of 
**      execution.  A lock list can be related to another list by passing 
**      the lock list identifier of the related list.  The related list
**      cannot be released until all dependent lists have been released. 
**      This mechanism can be used to associate locks in different lists
**      with the same thread.  This works if the same lock does not appear 
**      in the related lock list and the created lock list.  Only one
**      level of lock lists are supported currently, this means that 
**      a related lock list cannot be related to another list.
**
**	Using the LK_RECOVER flag an existing lock list can be searched for
**	and it's identifer returned.  Using the LK_ASSIGN flag a unique number
**	will be supplied for you and a lock identifer will be returned.
**
**	If the LK_SHARED flag is specified, then the lock list will be
**	shareable by other processes.  This means that other LK clients may
**	connect to the same lock list and make lock calls on it (release locks
**	already held on it, convert them, or request new locks).
**
**	If the caller specifies LK_CONNECT, then the request is to connect
**	to an already existing shared lock list.  The caller should not specify
**	the LK_ASSIGN flag, but should pass in the key of the shared list
**	in the 'unique_id' argument.  A lock list id will be returned to use
**	to reference the shared lock list.  The lock list attributes (maxlocks,
**	related list, interruptable ..) are ignored if LK_CONNECT is specified.
**	The lock list attributes are set when the list is actually created.
**
**	Note that two processes using a common shared lock list may use
**	different lock list id's to reference the same lock list.  Lock list
**	id's are unique to each connection to a shared list.
**
** Inputs:
**      flags                           Create list options:
**					LK_RECOVER - assume ownership of an
**					    already existing lock list.  This
**					    is used only by the recovery system.
**					LK_NONPROTECT - list holds locks that
**					    can be released without recovery.
**					LK_ASSIGN - assign a unique lock list
**					    key to the created list.  If not
**					    specified, then the caller supplies
**					    the key (unique_id argument).
**					LK_NOINTERRUPT - lock requests on this
**					    list are not interruptable.
**					LK_MASTER - lock list owned by recovery
**					    process.
**					LK_SHARED - create shared lock list.
**					LK_CONNECT - connect to existing shared
**					    lock list.
**      unique_id                       A unique value used to identify a 
**                                      lock domain.
**      related_lli                     The lock list identifier of the list 
**                                      that should be related to this list.
**	count				The maximum number of locks on the
**					lock list.
** Outputs:
**      lock_list_id                    The lock list identifier assigned to 
**                                      this list.
**	Returns:
**	    OK			Successful completion.
**	    LK_BADPARAM			Something wrong with a parameter.
**	    LK_NOLOCKS			No more lock resources available.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** Implementation Notes:
**
**    Shared Lock Lists : Shared Lock Lists are lists that can be referenced
**	concurrently by multiple processes.  They are identified by a
**	llb_status of LLB_SHARED.
**
**	When a client creates a Shared Lock List, two lists are actually
**	created:
**
**	    - The actual shared list is created - this is the llb that
**	      all locks will be held on.  All of the information regarding
**	      the number of locks held, max number of locks, related lists,
**	      etc - are kept on this lock list.
**
**	    - A second lock list is created that is used as the creating
**	      process's handle to the shared lock list.  Its status type is
**	      LLB_PARENT_SHARED and its purpose is mainly to point to the
**	      shared lock list.  No locks are actually held on this list. The
**	      llb_pid, llb_ast, and llb_astp fields are set to the values that
**	      are specific to the creating process (the shared list cannot hold
**	      these values since they may be different for each process).  The
**	      llb_shared_llb field holds a pointer to the shared list.
**
**	The lock list id of the LLB_PARENT_SHARED list is returned to the
**	caller as the lock list id.  All lock requests should be made on
**	this lock list (the actual id of the shared list is unknown to the
**	calling process).
**
**	When a client connects to an already existing shared list, an
**	LLB_PARENT_SHARED handle is created and pointed at the shared list.
**	As in the create shared call, the id returned to the caller is the
**	id of the handle lock list.
**
**	All routines which recieve requests on a lock
**	list of the type LLB_PARENT_SHARED will handle the request as though
**	it were made on the shared list.  If the caller must be suspended then
**	the pid, ast, and ast parm are used from the PARENT_SHARED list.
**
**	The llb_connect_count of the shared lock list indicates the number of
**	handles that exist to the shared list.  Each time an LLB_PARENT_SHARED
**	list is deallocated, the reference count of the shared list must
**	be decremented.  When the last handle is released, the shared list
**	is released (and all locks released).
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	1-Apr-1993 (daveb)
**	    Init the llb_sid field so it's always valid.
**	24-may-1993 (bryanp)
**	    Optimize away PCpid calls by using LK_my_pid global variable.
**	26-jul-1993 (bryanp)
**	    All lock lists created by the CSP get marked LLB_MASTER_CSP, even
**		if the caller forgot to pass in the LK_MASTER_CSP flag.
**	    Added some casts to pacify compilers.
**	23-aug-1993 (bryanp)
**	    When adopting ownership of a lock list using the LK_RECOVER flag,
**		we must free the new LLB we allocated so that we don't leak.
**	31-jan-1994 (rogerk)
**	    The RCP adopts ownership of transactions it needs to recover
**	    by calling this routine with the LK_RECOVER flag.  The routine
**	    always allocates a new llb, but in the LK_RECOVER mode, it ends
**	    up looking up the orphaned llb and updating it.  The newly
**	    allocated llb is discarded.  Fixed bug in this code which was
**	    storing the caller's session id into the llb_sid field of the
**	    about-to-be-discarded llb rather than the target orphan llb.
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Changed LK_xmutex() calls to pass the mutex address.
**		- Acquire LKD semaphore in LK_create_list() before
**		  searching active transaction chain for insertion point.
**		- Acquire LKD and LLB semaphores in LK_create_list()
**		  when a process-specific connection to an existing shared
**		  LLB is made.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	19-jul-1995 (canor01)
**	    Mutex Granularity Project
**		- Acquire LKD semaphore before calling LK_deallocate_cb()
**	 4-sep-1995 (dougb)
**	    Init new llb_gsearch_count field.
**	12-Dec-1995 (jenjo02)
**	    Don't acquire LKD semaphore before calling LK_deallocate_cb();
**	    new mutex protection added to that function.
**	    Maintenance of lkd_llb_inuse moved to LK_allocate|deallocate_cb().
**	    A host of new queue/list mutexes (mutexi?) to reduce overabuse
**	    of lkd_mutex. Tried to be more sensible with mutex latching/
**	    unlatching, keeping the lock for as short a time as possible.
**	    Protect lkd_stat.next_id with lkd_mutex to prevent feeble
**	    but possible concurrent update by multiple processes.
**      10-jan-1996 (canor01)
**          Mutex granularization:
**              - acquire LKD semaphore before calling LK_allocate_cb()
**	12-Jan-1996 (jenjo02)
**	    Moved assignment of llb_name to LK_allocate_cb() where
**	    it's done under the protection of the lkb_mutex.
**	30-may-1996 (pchang)
**	    Added LLB as a parameter to the call to LK_allocate_cb() to enable
**	    the reservation of SBK table resource for use by the recovery
**	    process. (B76879)
**	11-Oct-1996 (jenjo02)
**	    When inheriting a lock list using LK_RECOVER, also update the
**	    pid and sid of its related lock list, if any.
**	28-Feb-2002 (jenjo02)
**	    LK_MULTITHREAD is now an attribute of a lock list, not just
**	    a run-time flag. It means a lock list is subject to being
**	    concurrently used by more than one thread, hence special
**	    care must be taken to prevent corruption of the list.
**	27-Apr-2007 (jonj)
**	    Mark shared LLB as MULTITHREAD as well as the sharers.
*/
STATUS
LKcreate_list(
i4			flags,
LK_LLID			related_lli,
LK_UNIQUE		*unique_id,
LK_LLID			*lock_list_id,
i4			count,
CL_ERR_DESC		*sys_err)
{
    LKD			*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB                 *related_llb = NULL;
    LLB			*shared_llb = NULL;
    LLB                 *llb;
    LLB			*end;
    LLB			*next_llb;
    LLB			*prev_llb;
    LLB			*proc_next_llb;
    LLB_ID              *recov_list_id;
    LLB                 *recov_llb;
    LLB_ID		*input_related_lli = (LLB_ID *) &related_lli;
    STATUS		status;
    STATUS              return_status;
    SIZE_TYPE		*lbk_table;
    SIZE_TYPE		llb_offset;
    SIZE_TYPE		end_offset;
    i4		err_code;
    STATUS      	local_status;
    CL_ERR_DESC	        local_sys_err;
    i4		assigned_llbname[2];

    LK_WHERE("LK_create_list")

    CL_CLEAR_ERR(sys_err);

    /* Must be within range, and not be related to another list. */

    lkd->lkd_stat.create_list++;
    if (input_related_lli->id_id)
    {
	if (input_related_lli->id_id == 0 ||
	    (i4)input_related_lli->id_id > lkd->lkd_lbk_count)
	{
	    uleFormat(NULL, E_CL102A_LK_CREATE_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, input_related_lli->id_id, 0, lkd->lkd_lbk_count);
	    return (LK_BADPARAM);
	}

	lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	related_llb = (LLB *)
		LGK_PTR_FROM_OFFSET(lbk_table[input_related_lli->id_id]);

	if (related_llb->llb_type != LLB_TYPE ||
	    related_llb->llb_related_llb)
	{
	    uleFormat(NULL, E_CL102B_LK_CREATE_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			    0, input_related_lli->id_id,
	    		    0, related_llb->llb_type,
			    0, related_llb->llb_id.id_id);
	    return (LK_BADPARAM);
	}
	LGK_VERIFY_ADDR( related_llb, sizeof(LLB) );
    }

    if (lock_list_id == (LK_LLID *) NULL)
    {
	uleFormat(NULL, E_CL102C_LK_CREATE_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return(LK_BADPARAM);
    }

    /*
    ** LK_RECOVER_LLID:
    ** 
    **    The RCP may try to assume ownership of a lock list for which
    **    it has the lock list id only.
    */
    if ( flags & LK_RECOVER_LLID)
    {
	recov_list_id = (LLB_ID *)lock_list_id;
	if ((i4) recov_list_id->id_id > lkd->lkd_lbk_count)
	{   
	    uleFormat(NULL, E_CL103D_LK_REQUEST_BAD_PARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			    0, recov_list_id->id_id, 0, lkd->lkd_lbk_count);
	   return (LK_BADPARAM);
	}

	/*
	**  Change owner of lock list to new process. This is typically
	**  performed by the recovery process when it requests
	**  ownership of an orphaned lock list.
	**  Set the lock list block fields such as PID and SID to the
	**  new owner.
	*/
	
	lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	recov_llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[recov_list_id->id_id]);
	LGK_VERIFY_ADDR( recov_llb, sizeof(LLB) );
	recov_llb->llb_status |= LLB_RECOVER;
	recov_llb->llb_status &= ~LLB_NOINTERRUPT;
	CSget_cpid(&recov_llb->llb_cpid);

	/*
	** If there's a related llb, update its pid and sid as well.
	** dmxe_resume()'s about to inherit the related lock list
	** and it must reflect the inheriting session's process information.
	*/
	if (recov_llb->llb_related_llb)
	{
	    related_llb = 
			(LLB *)LGK_PTR_FROM_OFFSET(recov_llb->llb_related_llb);
	    STRUCT_ASSIGN_MACRO(recov_llb->llb_cpid, 
				related_llb->llb_cpid);
	    LGK_VERIFY_ADDR( related_llb, sizeof(LLB) );
	}

	return (OK);
    }

    /*
    ** Allocate a lock list block.
    ** 
    ** This also results in the assignment of a unique llb_name
    ** and llb_cpid and the initialization of the LLB.
    */

    if ((llb = (LLB *)LK_allocate_cb(LLB_TYPE, (LLB *)NULL)) == 0)
    {
	uleFormat(NULL, E_DMA011_LK_NO_LLBS, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return (LK_NOLOCKS);
    }

    /*
    ** If not LK_ASSIGN, then unique id must be specified.  Normally
    ** the id cannot be zero, but if LK_CONNECT then let it be anything,
    ** the request will fail below if the specified id does not exist.
    */
    if ((flags & LK_ASSIGN) == 0)
    {
        if ((unique_id == 0) ||
	    ((unique_id->lk_uhigh == 0) && ((flags & LK_CONNECT) == 0)))
	{
	    (VOID) LK_unmutex(&lkd->lkd_llb_q_mutex);
	    uleFormat(NULL, E_CL1031_LK_BAD_UNIQUE_ID, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, unique_id,
			0, (unique_id ? unique_id->lk_uhigh : -1));
	    LK_deallocate_cb( llb->llb_type, (PTR)llb, llb );
	    return (LK_BADPARAM);
	}

	/*
	** Save the unique id assigned to the allocated LLB -
	** we may need it later.
	*/
	assigned_llbname[0] = llb->llb_name[0];
	assigned_llbname[1] = llb->llb_name[1];

        llb->llb_name[0] = unique_id->lk_uhigh;
        llb->llb_name[1] = unique_id->lk_ulow;
    }

    /*
    **  Find the position in the sorted lock list that this
    **  lock list block should be placed.  The list is sorted
    **  by unique identifier in descending order.  Since unique
    **  identifiers are normally every increasing numbers, this
    **  search should terminate on the first comparison.
    */

    if (local_status = LK_mutex(SEM_EXCL, &lkd->lkd_llb_q_mutex))
    {
	uleFormat(NULL, local_status, &local_sys_err,
		    ULE_LOG, NULL, 0, 0, 0, &err_code, 0);
	status = E_DMA02D_LKCLIST_SYNC_ERROR;
	STRUCT_ASSIGN_MACRO(local_sys_err, *sys_err);
	return (LK_NOLOCKS);
    }

    end_offset = LGK_OFFSET_FROM_PTR(&lkd->lkd_llb_next);
    next_llb = (LLB *)LGK_PTR_FROM_OFFSET(lkd->lkd_llb_next);

    for (llb_offset = lkd->lkd_llb_next;
	llb_offset != end_offset;
	llb_offset = next_llb->llb_q_next)
    {
	next_llb = (LLB *)LGK_PTR_FROM_OFFSET(llb_offset);

        if (llb->llb_name[0] > next_llb->llb_name[0] ||
	    (llb->llb_name[0] == next_llb->llb_name[0] &&
	    llb->llb_name[1] > next_llb->llb_name[1]))
	{
		break;
	}

	if (llb->llb_name[0] == next_llb->llb_name[0] && 
	    llb->llb_name[1] == next_llb->llb_name[1])
        {
	    /* Mutex the apparently matching list */
	    (VOID)LK_mutex(SEM_EXCL, &next_llb->llb_mutex);

	    /*
	    ** Unique id already exists.  This is an error unless the 'flag'
	    ** argument is LK_CONNECT or LK_RECOVER.
	    **
	    ** If the request flag is LK_RECOVER, then this is the recovery
	    ** process requesting an orphaned lock list.
	    **
	    ** If the request flag is LK_CONNECT then the caller is requesting
	    ** to create a connection to a shared lock list.  In this case
	    ** the lock list must be a shared lock list.
	    */
	    if ((flags & LK_CONNECT) &&	(next_llb->llb_status & LLB_SHARED))
	    {
		/*
		** Save pointer to the shared list.  The llb will be formatted
		** below to serve as a reference to this lock list.
		*/
		shared_llb = next_llb;

		LGK_VERIFY_ADDR( shared_llb, sizeof(LLB) );

		/*
		** The llb_name field was assigned above to the key of the
		** shared lock list in order to find the list to connect to.
		** Now that it is found, reassign the unique llb_name
		** given when the LLB was allocated, then
		** research the list for the spot for this key.
		*/
		llb->llb_name[0] = assigned_llbname[0];
		llb->llb_name[1] = assigned_llbname[1];

		llb_offset = lkd->lkd_llb_next;
		next_llb = (LLB *)LGK_PTR_FROM_OFFSET(llb_offset);
		next_llb = (LLB *)LGK_PTR_FROM_OFFSET(next_llb->llb_q_prev);

		continue;
	    }
	    else
	    {
		(VOID) LK_unmutex(&lkd->lkd_llb_q_mutex);
		LK_deallocate_cb( llb->llb_type, (PTR)llb, llb );
	    }

	    if (flags & LK_RECOVER)
	    {
		/*
		**  Change owner of lock list to new process. This is typically
		**  performed by the recovery process when it requests
		**  ownership of an orphaned lock list. We don't need the new
		**  lock list block we just allocated, so deallocate it.
		**  Set the lock list block fields such as PID and SID to the
		**  new owner. It might be nice to maintain both new and old,
		**  so you know who the old owner was, but we don't have the
		**  fields for this in the LLB.
		*/
		
		if ( CXconfig_settings( CX_HAS_MASTER_CSP_ROLE ) )
		    next_llb->llb_status |= LLB_MASTER_CSP;
    
		next_llb->llb_status |= LLB_RECOVER;
		next_llb->llb_status &= ~LLB_NOINTERRUPT;
		CSget_cpid(&next_llb->llb_cpid);
 
		/*
		** If there's a related llb, update its session info as well.
		** dmxe_resume()'s about to inherit the related lock list
		** and it must reflect the inheriting session's process information.
		*/
		if (next_llb->llb_related_llb)
		{
		    related_llb = (LLB *)LGK_PTR_FROM_OFFSET(next_llb->llb_related_llb);
		    STRUCT_ASSIGN_MACRO(next_llb->llb_cpid,
					related_llb->llb_cpid);
		 }

		*lock_list_id = *(LK_LLID *)&next_llb->llb_id;
		(VOID)LK_unmutex(&next_llb->llb_mutex);
		return (OK);
	    }

	    uleFormat(NULL, E_CL1032_LK_DUPLICATE_LOCK_ID, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, next_llb->llb_name[0], 0, next_llb->llb_name[1]);
	    (VOID)LK_unmutex(&next_llb->llb_mutex);
	    return (LK_BADPARAM);
        }
    }

    /*
    ** Lock list not found.
    ** If this is a request to connect to an already existing list, then    
    ** return an error since the lock list did not exist.
    */
    if ((flags & LK_RECOVER) ||
	((flags & LK_CONNECT) && (shared_llb == NULL)))
    {
	(VOID) LK_unmutex(&lkd->lkd_llb_q_mutex);
	uleFormat(NULL, E_CL1050_LK_LOCKID_NOTFOUND, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			0, llb->llb_name[0], 0, llb->llb_name[1],
			0, flags);
	LK_deallocate_cb( llb->llb_type, (PTR)llb, llb );
	return (LK_BADPARAM);
    }

    /*
    ** We are now creating a new lock list.
    ** LLB has been removed from the free list 
    ** and initialized by LK_allocate_cb().
    **
    ** Insert it in the active queue before the entry
    ** pointed to by "next".
    **
    ** llb_cpid was filled in by the LLB allocation code.
    */
    llb->llb_shared_llb = 0;
    llb->llb_related_llb = 0;

    llb->llb_max_lkb = lkd->lkd_max_lkb;
    if (count)
    {
	/* Rather than using the default maximum logical locks. */

	llb->llb_max_lkb = count;
    }

    /* Update the llb_status. */

    if (flags & LK_NONPROTECT)
    {
        llb->llb_status |= LLB_NONPROTECT;
	llb->llb_max_lkb = lkd->lkd_max_lkb / 2;
    }
    if (flags & LK_MASTER)
	llb->llb_status |= LLB_MASTER;
    if (flags & LK_MASTER_CSP)
	llb->llb_status |= LLB_MASTER_CSP;
    if (flags & LK_NOINTERRUPT)
	llb->llb_status |= LLB_NOINTERRUPT;
    if (flags & LK_MULTITHREAD)
	llb->llb_status |= LLB_MULTITHREAD;

    /*
    ** LLB_MASTER_CSP attribute is now only used by lock lists created
    ** by the master CSP during recovery.
    */
    if ( CXconfig_settings( CX_HAS_MASTER_CSP_ROLE ) )
	llb->llb_status |= LLB_MASTER_CSP;
    
    /*
    ** Now insert LLB into the active queue before the
    ** entry pointed to by "next_llb":
    */
    llb->llb_q_next = LGK_OFFSET_FROM_PTR(next_llb);
    llb->llb_q_prev = next_llb->llb_q_prev;
    prev_llb = (LLB *)LGK_PTR_FROM_OFFSET(next_llb->llb_q_prev);
    prev_llb->llb_q_next = next_llb->llb_q_prev = LGK_OFFSET_FROM_PTR(llb);

    if (flags & LK_SHARED)
    {
	/*
	** This is a request to create a shared lock list.  We have just
	** allocated the shared lock list, now create a list to use as the
	** caller's connection to the shared lock list.
	**
	** The function of allocating an LLB also assigned a unique
	** llb_name, llb_cpid and initialized the remainder.
	*/
	return_status = OK;
	shared_llb = llb;

	if ((llb = (LLB *)LK_allocate_cb(LLB_TYPE, (LLB *)NULL)) == 0)
	    return_status = LK_NOLOCKS;
	else
	{
	    /* Look for spot of new lock list in sorted lock list. */
	    end_offset = LGK_OFFSET_FROM_PTR(&lkd->lkd_llb_next);

	    for (llb_offset = lkd->lkd_llb_next;
		llb_offset != end_offset;
		llb_offset = proc_next_llb->llb_q_next)
	    {
		proc_next_llb = (LLB *)LGK_PTR_FROM_OFFSET(llb_offset);

		if (llb->llb_name[0] > proc_next_llb->llb_name[0] ||
		    (llb->llb_name[0] == proc_next_llb->llb_name[0] &&
		     llb->llb_name[1] > proc_next_llb->llb_name[1]))
		    break;
		if (llb->llb_name[0] == proc_next_llb->llb_name[0] && 
		    llb->llb_name[1] == proc_next_llb->llb_name[1])
		{
		    uleFormat(NULL, E_CL1051_LK_DUPLICATE_LOCK_ID,
			(CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, llb->llb_name[0], 0, llb->llb_name[1]);
		    return_status = LK_BADPARAM;
		    break;
		}
	    }
	}

	/*
	** If an error was encountered, then we have to free the lock list
	** already allocated for the shared lock list.
	*/
	if (return_status != OK)
	{
	    next_llb = (LLB *)LGK_PTR_FROM_OFFSET(shared_llb->llb_q_next);
	    prev_llb = (LLB *)LGK_PTR_FROM_OFFSET(shared_llb->llb_q_prev);
	    next_llb->llb_q_prev = shared_llb->llb_q_prev;
	    prev_llb->llb_q_next = shared_llb->llb_q_next;

	    (VOID) LK_unmutex(&lkd->lkd_llb_q_mutex);
	    LK_deallocate_cb( shared_llb->llb_type, (PTR)shared_llb, shared_llb );
	    return (return_status);
	}

	/*
	** We are creating a shared lock list.  The list in 'shared_llb' which was
	** formatted above is the actual shared list.  The callers unique
	** handle to the list is llb.  This list is formatted with
	** the information from 'shared_llb' and assigned LLB_PARENT_SHARED status.
	*/
	/* Mark LLB as SHARED, leave NONPROTECT up to the caller */
	shared_llb->llb_status |= (LLB_SHARED | LLB_MULTITHREAD);
	shared_llb->llb_connect_count = 0;

	/* Insert PARENT_SHARED list into the active queue */
	llb->llb_q_next = LGK_OFFSET_FROM_PTR(proc_next_llb);
	llb->llb_q_prev = proc_next_llb->llb_q_prev;
	prev_llb = (LLB *)LGK_PTR_FROM_OFFSET(proc_next_llb->llb_q_prev);
	prev_llb->llb_q_next =
	proc_next_llb->llb_q_prev = LGK_OFFSET_FROM_PTR(llb);

	/* Fall thru to update the SHARED list */
    }

    /* Release the active LLB queue */
    (VOID) LK_unmutex(&lkd->lkd_llb_q_mutex);
    
    if ( shared_llb )
    {
	/*
	** We are allocating a process-specific connection to a shared lock
	** list.  Mark the proc llb as a shared reference and point it to the
	** real shared lock list.  Also increment the reference count.
	**
	** The shared lock list has been mutexed.
	*/
	/* Mark LLB as PARENT_SHARED, NONPROTECT as it will contain no locks */
	llb->llb_status |= (LLB_PARENT_SHARED | LLB_NONPROTECT);
	llb->llb_status &= ~LLB_MULTITHREAD;
	llb->llb_shared_llb = LGK_OFFSET_FROM_PTR(shared_llb);

	/* Increment reference count to shared list */
	shared_llb->llb_connect_count++;

	(VOID) LK_unmutex(&shared_llb->llb_mutex);
    }
    
    if (related_llb)
    {
	/*	Bump related count. */
	llb->llb_related_llb = LGK_OFFSET_FROM_PTR(related_llb);
	(VOID) LK_unmutex(&llb->llb_mutex);
	(VOID) LK_mutex(SEM_EXCL, &related_llb->llb_mutex);
	related_llb->llb_related_count++;
	(VOID) LK_unmutex(&related_llb->llb_mutex);
    }
    else
	(VOID) LK_unmutex(&llb->llb_mutex);

    /*  Return the lock_list_id. */

    *lock_list_id = *(LK_LLID *)&llb->llb_id;
    return (OK);
}

/*{
** Name: LKconnect	- Connect to an existing lock list.
**
** Description:
**	This function connects to an existing lock list for the
**	purpose of sharing a transaction's lock context amongst
**	several threads so that the threads do not conflict with
**	each other for lock resources, yet may each independently
**	wait for lock resources.
**
**	On the first connect to the lock list, the list is
**	converted to a SHARED LLB which inherits all the
**	attributes and locks of the original list, and the
**	original LLB is transfomed into a PSHARED LLB, thenceforth
**	acting as a "handle" to the shared LLB.
**
**	On each call, a new PSHARED LLB is allocated on behalf
**	of the caller and linked to the SHARED list.
**
** Inputs:
**	clock_list_id		The lock list id to connect to
**
** Outputs:
**      lock_list_id            The lock list identifier assigned to 
**                              the PSHARED list.
**	Returns:
**	    OK			Successful completion.
**	    LK_BADPARAM		Something wrong with a parameter.
**	    LK_NOLOCKS		No more lock resources available.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-Dec-2003 (jenjo02)
**	    Invented for the Partitioned Table, Parallel query
**	    project.
**	12-Aug-2004 (jenjo02)
**	    Move LKB/RSB stash to SHARED list.
*/
STATUS
LKconnect(
LK_LLID			clock_list_id,
LK_LLID			*lock_list_id,
CL_ERR_DESC		*sys_err)
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB		*cllb, *sllb, *next_llb, *prev_llb;
    STATUS	status;
    SIZE_TYPE	*lbk_table;
    SIZE_TYPE	llb_offset;
    SIZE_TYPE	llbq_offset;
    SIZE_TYPE	end_offset;
    i4		err_code;
    i4		assigned_llbname[2];
    LK_UNIQUE	connect_id;
    LLB_ID	*connect_to_id = (LLB_ID*)&clock_list_id;
    LLBQ	*next_llbq, *prev_llbq;
    i4		flags, count;
    LKH		*lkh_table, *lkb_hash_bucket, *old_hash_bucket;
    LKB		*lkb;
    RSH		*rsh_table, *rsb_hash_bucket;
    RSB		*rsb;
    LKBQ	*lkbq, *next_lkbq, *prev_lkbq;
    u_i4	rsb_hash_value;

    LK_WHERE("LKconnect")

    if ( connect_to_id->id_id > lkd->lkd_lbk_count )
    {   
	uleFormat(NULL, E_CL106D_LK_CONNECT_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, connect_to_id->id_id, 0, lkd->lkd_lbk_count);
       return (LK_BADPARAM);
    }

    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
    cllb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[connect_to_id->id_id]);

    if ( cllb->llb_type != LLB_TYPE ||
	cllb->llb_id.id_instance != connect_to_id->id_instance )
    {
	uleFormat(NULL, E_CL106E_LK_CONNECT_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			0, *(u_i4*)&connect_to_id, 
			0, cllb->llb_type,
			0, *(u_i4*)&cllb->llb_id);
	return (LK_BADPARAM);
    }

    (VOID)LK_mutex(SEM_EXCL, &cllb->llb_mutex);

    /* If not already converted to a SHARED list, do it now */
    if ( (cllb->llb_status & LLB_PARENT_SHARED) == 0 )
    {
	/* Allocate an LLB for the SHARED list */
	if ( (sllb = (LLB*)LK_allocate_cb(LLB_TYPE, (LLB*)NULL)) == 0 )
	{
	    uleFormat(NULL, E_DMA011_LK_NO_LLBS, (CL_ERR_DESC*)NULL, ULE_LOG,
			NULL, (char*)NULL, 0L, (i4*)NULL, &err_code, 0);
	    (VOID)LK_unmutex(&cllb->llb_mutex);
	    return(LK_NOLOCKS);
	}

	/* Saved the "assigned" llb name for later */
	assigned_llbname[0] = sllb->llb_name[0];
	assigned_llbname[1] = sllb->llb_name[1];

	/*
	** "sllb" becomes the SHARED lock list and
	** inherits its characteristics and locks 
	** from the "connect to" list (cllb).
	*/

	sllb->llb_cpid = cllb->llb_cpid;

	/* SHARED LLB will start with one connected handle */
	sllb->llb_connect_count = 1;
	sllb->llb_shared_llb = 0;
	/* Move the related list, if any, to the sLLB */
	sllb->llb_related_llb = cllb->llb_related_llb;
	cllb->llb_related_llb = 0;
	sllb->llb_max_lkb = cllb->llb_max_lkb;
	/* SHARED list is multithreaded, handles are not */
	sllb->llb_status |= (LLB_SHARED | LLB_MULTITHREAD);
	sllb->llb_status &= 
	    ~(LLB_WAITING | LLB_EWAIT | LLB_ESET | LLB_EDONE);
	cllb->llb_status |= (LLB_PARENT_SHARED | LLB_NONPROTECT);
	cllb->llb_status &= ~LLB_MULTITHREAD;
	cllb->llb_shared_llb = LGK_OFFSET_FROM_PTR(sllb);

	/*
	** Move the LKB/RSB stash to the SHARED list.
	** Subsequent lock requests will be filled 
	** from the SHARED LLB, not the handles.
	*/
	sllb->llb_lkb_stash = cllb->llb_lkb_stash;
	sllb->llb_rsb_stash = cllb->llb_rsb_stash;
	sllb->llb_lkb_alloc = cllb->llb_lkb_alloc;
	sllb->llb_rsb_alloc = cllb->llb_rsb_alloc;
	cllb->llb_lkb_stash = 0;
	cllb->llb_rsb_stash = 0;
	cllb->llb_lkb_alloc = 0;
	cllb->llb_rsb_alloc = 0;

	/*
	** Replace the "connect to" LLB on the active list
	** with the SHARED LLB, insert the "connect to"
	** LLB in its collated spot to maintain the
	** descending sequence (by llb_name) of the list.
	**
	** For this we need to hold the llb_q_mutex:
	*/
	(VOID)LK_mutex(SEM_EXCL, &lkd->lkd_llb_q_mutex);

	/*
	** The SHARED LLB's "name" (tran id) becomes that
	** of the "connect" LLB.
	**
	** The "connect" LLB's "name" is replaced with
	** the assigned name.
	*/
	sllb->llb_name[0] = cllb->llb_name[0];
	sllb->llb_name[1] = cllb->llb_name[1];
	cllb->llb_name[0] = assigned_llbname[0];
	cllb->llb_name[1] = assigned_llbname[1];

	/*
	** The SHARED LLB replaces the "connect" LLB
	** on the LLB queue, in the same position.
	*/
	sllb->llb_q_next = cllb->llb_q_next;
	sllb->llb_q_prev = cllb->llb_q_prev;

	next_llb = (LLB*)LGK_PTR_FROM_OFFSET(sllb->llb_q_next);
	prev_llb = (LLB*)LGK_PTR_FROM_OFFSET(sllb->llb_q_prev);

	next_llb->llb_q_prev = prev_llb->llb_q_next =
	    LGK_OFFSET_FROM_PTR(sllb);

	/*
	** Find the insertion point in the active LLB list
	** for the "connect to" LLB.
	*/
	end_offset = LGK_OFFSET_FROM_PTR(&lkd->lkd_llb_next);
	next_llb = (LLB*)LGK_PTR_FROM_OFFSET(lkd->lkd_llb_next);

	for ( llb_offset = lkd->lkd_llb_next;
	      llb_offset != end_offset;
	      llb_offset = next_llb->llb_q_next )
	{
	    next_llb = (LLB*)LGK_PTR_FROM_OFFSET(llb_offset);

	    if ( cllb->llb_name[0] > next_llb->llb_name[0] ||
		(cllb->llb_name[0] == next_llb->llb_name[0] &&
		 cllb->llb_name[1] > next_llb->llb_name[1]) )
	    {
		break;
	    }
	}
	/* Connect "connect to" LLB into its new position */
	cllb->llb_q_next = LGK_OFFSET_FROM_PTR(next_llb);
	cllb->llb_q_prev = next_llb->llb_q_prev;
	prev_llb = (LLB*)LGK_PTR_FROM_OFFSET(next_llb->llb_q_prev);
	prev_llb->llb_q_next = next_llb->llb_q_prev =
	    LGK_OFFSET_FROM_PTR(cllb);

	(VOID)LK_unmutex(&lkd->lkd_llb_q_mutex);

	/* Relocate the cllb's lock list to the SHARED LLB: */

	sllb->llb_lkb_count = cllb->llb_lkb_count;
	sllb->llb_llkb_count = cllb->llb_llkb_count;

	if ( cllb->llb_llbq.llbq_next == 
		LGK_OFFSET_FROM_PTR(&cllb->llb_llbq) )
	{
	    /* No locks held */
	    sllb->llb_llbq.llbq_next = sllb->llb_llbq.llbq_prev
		= LGK_OFFSET_FROM_PTR(&sllb->llb_llbq);
	}
	else
	{
	    sllb->llb_llbq.llbq_next = cllb->llb_llbq.llbq_next;
	    sllb->llb_llbq.llbq_prev = cllb->llb_llbq.llbq_prev;
	    next_llbq = (LLBQ*)LGK_PTR_FROM_OFFSET(sllb->llb_llbq.llbq_next);
	    prev_llbq = (LLBQ*)LGK_PTR_FROM_OFFSET(sllb->llb_llbq.llbq_prev);
	    next_llbq->llbq_prev = 
		LGK_OFFSET_FROM_PTR(&sllb->llb_llbq.llbq_next);
	    prev_llbq->llbq_next = 
		LGK_OFFSET_FROM_PTR(&sllb->llb_llbq.llbq_next);
	}

	/* The handles (cllb) never hold locks */
	cllb->llb_llbq.llbq_next = cllb->llb_llbq.llbq_prev =
	    LGK_OFFSET_FROM_PTR(&cllb->llb_llbq.llbq_next);
	cllb->llb_lkb_count = 0;
	cllb->llb_llkb_count = 0;

	/* 
	** For each lock on the list:
	**	o rehash LKH using sllb
	**	o if hash to new bucket
	**	  - remove LKB from old lkb_hash
	**	  - insert into new lkb_hash
	**	o change lkbq_llb "owner" to sllb
	*/

	rsh_table = (RSH*)LGK_PTR_FROM_OFFSET(lkd->lkd_rsh_table);
	lkh_table = (LKH*)LGK_PTR_FROM_OFFSET(lkd->lkd_lkh_table);

	end_offset = LGK_OFFSET_FROM_PTR(&sllb->llb_llbq);

	for( llbq_offset = sllb->llb_llbq.llbq_next;
	     llbq_offset != end_offset;
	     llbq_offset = next_llbq->llbq_next )
	{
	    next_llbq = (LLBQ*)LGK_PTR_FROM_OFFSET(llbq_offset);
	    lkb = (LKB*)((char*)next_llbq - CL_OFFSETOF(LKB,lkb_llbq));
	    lkbq = &lkb->lkb_lkbq;

	    /* Compute new LKH hash bucket using sllb */
	    rsb = (RSB*)LGK_PTR_FROM_OFFSET(lkbq->lkbq_rsb);
	    rsb_hash_value = (rsb->rsb_name.lk_type + 
		rsb->rsb_name.lk_key1 + rsb->rsb_name.lk_key2 + 
		rsb->rsb_name.lk_key3 + rsb->rsb_name.lk_key4 + 
		rsb->rsb_name.lk_key5 + rsb->rsb_name.lk_key6);

	    rsb_hash_bucket = (RSH *)&rsh_table
		[(unsigned)rsb_hash_value % lkd->lkd_rsh_size];
	    lkb_hash_bucket = (LKH *)&lkh_table
		[((unsigned)rsb_hash_value + (unsigned)LGK_OFFSET_FROM_PTR(sllb)) %
					    lkd->lkd_lkh_size];

	    /* If hash to another bucket, relocate LKB to new hash */
	    old_hash_bucket = (LKH*)LGK_PTR_FROM_OFFSET(lkbq->lkbq_lkh);
	    if ( lkb_hash_bucket != old_hash_bucket )
	    {
		/* Remove from old hash bucket */
		(VOID)LK_mutex(SEM_EXCL, &old_hash_bucket->lkh_mutex);
		if ( lkbq->lkbq_next )
		{
		    next_lkbq = (LKBQ*)LGK_PTR_FROM_OFFSET(lkbq->lkbq_next);
		    next_lkbq->lkbq_prev = lkbq->lkbq_prev;
		}
		prev_lkbq = (LKBQ*)LGK_PTR_FROM_OFFSET(lkbq->lkbq_prev);
		prev_lkbq->lkbq_next = lkbq->lkbq_next;
		(VOID)LK_unmutex(&old_hash_bucket->lkh_mutex);

		/* Insert into new hash bucket */
		(VOID)LK_mutex(SEM_EXCL, &lkb_hash_bucket->lkh_mutex);
		lkbq->lkbq_lkh  = LGK_OFFSET_FROM_PTR(lkb_hash_bucket);
		lkbq->lkbq_next = lkb_hash_bucket->lkh_lkbq_next;
		lkbq->lkbq_prev = LGK_OFFSET_FROM_PTR(lkb_hash_bucket);
		if ( lkb_hash_bucket->lkh_lkbq_next )
		{
		    next_lkbq = (LKBQ*)LGK_PTR_FROM_OFFSET(lkb_hash_bucket->lkh_lkbq_next);
		    next_lkbq->lkbq_prev = LGK_OFFSET_FROM_PTR(lkbq);
		}
		lkb_hash_bucket->lkh_lkbq_next = LGK_OFFSET_FROM_PTR(lkbq);
		(VOID)LK_unmutex(&lkb_hash_bucket->lkh_mutex);
	    }

	    /* Change ownership of LKB to sllb */
	    lkbq->lkbq_llb = LGK_OFFSET_FROM_PTR(sllb);
	}
	/* Conversion of cllb is complete */
	(VOID)LK_unmutex(&cllb->llb_mutex);

	/* Keep the sllb mutex */
    }
    else
    {
	/* Already converted, sanity check sllb */
	if (cllb->llb_shared_llb == 0)
	{
	    uleFormat(NULL, E_CL106F_LK_CONNECT_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
			0, connect_to_id->id_id);
	    (VOID)LK_unmutex(&cllb->llb_mutex);
	    return (LK_BADPARAM);
	}

	sllb = (LLB *)LGK_PTR_FROM_OFFSET(cllb->llb_shared_llb);

	if ( sllb->llb_type != LLB_TYPE ||
	    (sllb->llb_status & LLB_SHARED) == 0 )
	{
	    uleFormat(NULL, E_CL1070_LK_CONNECT_BADPARAM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			0, connect_to_id->id_id, 
			0, sllb->llb_type, 
			0, sllb->llb_status);
	    (VOID)LK_unmutex(&cllb->llb_mutex);
	    return (LK_BADPARAM);
	}

	(VOID)LK_mutex(SEM_EXCL, &sllb->llb_mutex);
	(VOID)LK_unmutex(&cllb->llb_mutex);
    }

    /*
    ** This is the "name" (tran id) of the lock list
    ** we want to connect to.
    */
    connect_id.lk_uhigh = sllb->llb_name[0];
    connect_id.lk_ulow  = sllb->llb_name[1];

    /* Reverse engineer the flags */
    flags = LK_CONNECT;
    if ( sllb->llb_status & LLB_NOINTERRUPT )
	flags |= LK_NOINTERRUPT;
    if ( sllb->llb_status & LLB_MASTER )
	flags |= LK_MASTER;
    if ( sllb->llb_status & LLB_NONPROTECT )
	flags |= LK_NONPROTECT;
    if ( sllb->llb_status & LLB_MASTER_CSP )
	flags |= LK_MASTER_CSP;

    /* "maxlocks" for the handle will be the same as the sllb */
    count = sllb->llb_max_lkb;

    (VOID)LK_unmutex(&sllb->llb_mutex);

    /* Create a handle to this SHARED list */
    return(LKcreate_list(flags,
			  (LK_LLID)0,
			  &connect_id,
			  lock_list_id,
			  count,
			  sys_err));
}
