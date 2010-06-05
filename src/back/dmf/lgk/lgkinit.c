/*
** Copyright (c) 1992, 2008 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <id.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <ex.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <tm.h>
#include    <st.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <lk.h>
#include    <cx.h>
#include    <lgkdef.h>
#include    <lgkparms.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lkdef.h>
#ifdef	VMS
#include    <ssdef.h>
#include    <starlet.h>
#include    <lib$routines.h>
#include    <exhdef.h>
#endif

/**
**
**  Name: LGKINIT.C - LGK_initialize implementation
**
**  Description:
**	This module contains the code which implements LGK_initialize
**
**	    LGK_initialize() - initialize lgk control block
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	19-oct-1992 (bryanp)
**	    Check memory version number when attaching.
**	22-oct-1992 (bryanp)
**	    Change LGLKDATA.MEM to lglkdata.mem.
**	23-Oct-1992 (daveb)
**	    name the semaphore too.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (keving)
**	    Remove support for II_LGK_MEMORY_SIZE. If II_LG_MEMSIZE
**	    is not set then calculate memory size from PM values. 
**	24-may-1993 (bryanp)
**	    If the shared memory is the wrong version, don't install the
**	    at_exit handlers (the rundown routines won't be able to interpret
**	    the memory properly).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (andys)
**	    Correct some calls to ule_format.
**	26-jul-1993 (jnash)
**	    Add 'flag' param to LGK_initialize() lock the LGK data segment.
**	20-sep-1993 (bryanp)
**	    In addition to calling PCatexit, call (on VMS) sys$dclexh, since
**		there are some situations (image death and image rundown without
**		process rundown) which are caught neither by PCatexit (since
**		PCexit isn't run) nor by check-dead threads (since process
**		rundown never happened). This fixes a hole where an access-
**		violating ckpdb or auditdb command never got cleaned up.
**	31-jan-1994 (bryanp)
**	    Back out a few "features" which are proving countereffective:
**	    1) Don't bother checking mem_creator_pid to see if the previous
**		creator of the shared memory has died. This was an attempt to
**		gracefully re-use sticky shared memory following a system crash,
**		but it is suspected as being the culprit in a series of system
**		failures by re-initializing the shared memory at inopportune
**		times.
**	    2) Don't complain if the shared memory already exists but is of a
**		different size than you expected. Just go ahead and try to use
**		it anyway.
**	21-feb-1994 (bryanp)
**	    Reverse item (1) of the above 31-jan-1994 change and re-enable the
**		graceful re-use of shared memory. People weren't happy with
**		having to run ipcclean and csinstall all the time.
**	23-may-1994 (bryanp)
**	    On VMS, disable ^Y for LG/LK-aware processes. We don't want to allow
**		^Y because you might interrupt the process right in the middle
**		of an LG or LK operation, while holding the shared memory
**		semaphore, and this would then wedge the whole installation.
**      17-May-1994 (daveb) 59127
**          Attach lgk_mem semaphore if we're attaching to the segment.
**	30-jan-1995 (lawst01)
**	    Fix bug 61984
**	26-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**      12-jun-1995 (canor01)
**          semaphore protect memory allocation (MCT)
**      13-jun-1995 (chech02)
**          Added LGK_misc_sem semaphore to protect LGK_base.    
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed LGK_misc_sem which protected resources
**          updated only in single-threaded startup.
**	25-jun-96 (nick)
**	    Removed commented and ifdef'd out code added in change by lawst01
**	    above ( for which I'm adding a history entry as well !).
**	24-apr-1997 (nanpr01)
**	    Reinstate Bryanp's change. In the process of fixing bug 61984
**	    by Steve Lawrence and subsequent undo of Steve's fix by Nick
**	    Ireland on 25-jun-96 (nick) caused the if 0 code removed.
**	    Don't complain if the shared memory already exists but is of a
**	    different size than you expected. Just go ahead and try to use
**      05-aug-1997 (teresak)
**          Remove reference for VMS to exhdef.h being located in sys$library
**          as this is not a system header file but an Ingres header file
**          (located in jpt_clf_hdr).
**  12-Dec-97 (bonro01)
**      Added cleanup code for cross-process semaphores during
**      MEsmdestroy().
**	27-Mar-2000 (jenjo02)
**	    Added test for crossed thread types, refuse connection
**	    to LGK memory with E_DMA811_LGK_MT_MISMATCH.
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**      19-sep-2002 (devjo01)
**          If running NUMA clustered allocate memory out of local RAD.
**      23-Oct-2002 (hanch04)
**          Changed memory variables to use a SIZE_TYPE to allow
**          memory > 2 gig
**      14-Nov-2002 (hanch04)
**          When reading in II_LG_MEMSIZE in a LP64 server,  the conversion
**	    should be to an 8 byte long or SIZE_TYPE.
**	17-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	05-oct-2005 (devjo01)
**	    Copy II_INSTALLATION value returned by NMgtAt, since
**	    VMS implementation returns a pointer to a shared
**	    static buffer which will be reused by the next caller
**	    to NMgtAt for an existing logical.  Since CXinitialize
**	    queries logicals, the installation code used by CX may
**	    be overwritten unless it is copied to a safe area.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	21-feb-2008 (joea)
**	    Correct system-level installation value of II_INSTALLATION.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**      20-Nov-2009 (maspa05) bug 122642
**          In order to synchronize creation of UUIDs across servers added
**          a semaphore and a 'last time' variable into LGK memory. 
**      14-Dec-2009 (maspa05) bug 122642
**          #ifdef out the above change for Windows. The rest of the change
**          does not apply to Windows so the variables aren't defined.
**      19-apr-2010 (maspa05) bug 123595
**          Changed ID_UUID_SEM_INIT for unix to use cross-process routine
**          CS_cp_synch_init, which requires a status parameter.
**          
*/


static STATUS LGK_destroy(SIZE_TYPE	pages,
			  CL_ERR_DESC 	*sys_err );

#if defined(VMS) || defined(UNIX)
GLOBALREF ID_UUID_SEM_TYPE  *ID_uuid_sem_ptr;
GLOBALREF HRSYSTIME *ID_uuid_last_time_ptr;
GLOBALREF u_i2 *ID_uuid_last_cnt_ptr;
#endif

/*{
** Name: LGK_initialize()	-  initialize the lg/lk shared mem segment.
**
** Description:
**	This routine is called by the LGinitialize or LKinitialize routine.  IT
**	assumes that a previous caller has allocated the shared memory segment.
**
**	If it discovers that the shared memory segment has not yet been
**	initialized, it calls the LG and LK initialize-memory routines to do so.
**
** Inputs:
**	flag		- bit mask of:
**			  LOCK_LGK_MEMORY to lock the shared data segment
**			  LGK_IS_CSP if process is CSP process this node.
**
** Outputs:
**	sys_err		- place for system-specific error information.
**
**	Returns:
**	    OK	- success
**	    !OK - failure (CS*() routine failure, segment not mapped, ...)
**	
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	19-oct-1992 (bryanp)
**	    Check memory version number when attaching.
**	22-oct-1992 (bryanp)
**	    Change LGLKDATA.MEM to lglkdata.mem.
**	23-Oct-1992 (daveb)
**	    name the semaphore too.
**	13-feb-1993 (keving)
**	    Remove support for II_LGK_MEMORY_SIZE. If II_LG_MEMSIZE
**	    is not set then calculate memory size from PM values. 
**	24-may-1993 (bryanp)
**	    If the shared memory is the wrong version, don't install the
**	    at_exit handlers (the rundown routines won't be able to interpret
**	    the memory properly).
**	26-jul-1993 (jnash)
**	    Add 'flag' param lock the LGK data segment.
**	20-sep-1993 (bryanp)
**	    In addition to calling PCatexit, call (on VMS) sys$dclexh, since
**		there are some situations (image death and image rundown without
**		process rundown) which are caught neither by PCatexit (since
**		PCexit isn't run) nor by check-dead threads (since process
**		rundown never happened). This fixes a hole where an access-
**		violating ckpdb or auditdb command never got cleaned up.
**	31-jan-1994 (bryanp)
**	    Back out a few "features" which are proving countereffective:
**	    1) Don't bother checking mem_creator_pid to see if the previous
**		creator of the shared memory has died. This was an attempt to
**		gracefully re-use sticky shared memory following a system crash,
**		but it is suspected as being the culprit in a series of system
**		failures by re-initializing the shared memory at inopportune
**		times.
**	    2) Don't complain if the shared memory already exists but is of a
**		different size than you expected. Just go ahead and try to use
**		it anyway.
**	21-feb-1994 (bryanp)
**	    Reverse item (1) of the above 31-jan-1994 change and re-enable the
**		graceful re-use of shared memory. People weren't happy with
**		having to run ipcclean and csinstall all the time.
**	23-may-1994 (bryanp)
**	    On VMS, disable ^Y for LG/LK-aware processes. We don't want to allow
**		^Y because you might interrupt the process right in the middle
**		of an LG or LK operation, while holding the shared memory
**		semaphore, and this would then wedge the whole installation.
**          
**      17-May-1994 (daveb) 59127
**          Attach lgk_mem semaphore if we're attaching to the segment.
**      30-jan-1995 (lawst01) bug 61984
**          Use memory needed calculation from the 'lgk_calculate_size'
**          function to determine the size of the shared memory pool for
**          locking and locking. If the II_LG_MEMSIZE variable is specified
**          with a value larger than needed use the supplied value. If
**          lgk_calculate_size is unable to calculate a size then use the
**          magic number of 400000.  In addition issue a warning message
**          and continue executing in the event the number of pages
**          allocated is less than the number requested. 
**	24-apr-1997 (nanpr01)
**	    Reinstate Bryanp's change. In the process of fixing bug 61984
**	    by Steve Lawrence and subsequent undo of Steve's fix by Nick
**	    Ireland on 25-jun-96 (nick) caused the if 0 code removed.
**	    Part of the Steve's change was not reinstated such as not returning
**	    the status and exit and continue.
**	    1. Don't complain if the shared memory already exists but is of a
**	    different size than you expected. Just go ahead and try to use
**	    it.
**     18-aug-1998 (hweho01)
**          Reclaim the kernel resource if LG/LK shared memory segment is  
**          reinitialized. If the shared segment is re-used (the previous creator 
**          of the shared segment has died), the cross-process semaphores get 
**          initialized more than once at the same locations. That cause the
**          kernel resource leaks on DG/UX (OS release 4.11MU04). To fix the 
**          problem, CS_cp_sem_cleanup() is called to destroy all the 
**          semaphores before LG/LK shraed segment get recreated. 
**          CS_cp_sem_cleanup() is made dependent on xCL_NEED_SEM_CLEANUP and
**          OS_THREADS_USED, it returns immediately for most platforms.  
**	27-Mar-2000 (jenjo02)
**	    Added test for crossed thread types, refuse connection
**	    to LGK memory with E_DMA811_LGK_MT_MISMATCH.
**	18-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	    - Add CX mem requirement calculations.
**	    - Add LGK_IS_CSP flag to indicate that LGK memory is being
**	      initialized for a CSP process.
**	    - Add basic CX initialization.
**      19-sep-2002 (devjo01)
**          If running NUMA clustered allocate memory out of local RAD.
**	30-Apr-2003 (jenjo02)
**	    Rearchitected to silence long-tolerated race conditions.
**	    BUG 110121.
**	27-feb-2004 (devjo01)
**	    Rework allocation of CX shared memory to be compatible
**	    with race condition fix introduced for bug 110121.
**	29-Dec-2008 (jonj)
**	    If lgk_calculate_size() returns FAIL, the total memory
**	    needed exceeds MAX_SIZE_TYPE and we can't continue, but
**	    tell what we can about the needs of the various bits of
**	    memory before quitting.
**	06-Aug-2009 (wanfr01)
**	    Bug 122418 - Return E_DMA812 if LOCK_LGK_MUST_ATTACH is
**	    is passed in and memory segment does not exist
**      20-Nov-2009 (maspa05) bug 122642
**          In order to synchronize creation of UUIDs across servers added
**          a semaphore and a 'last time' variable into LGK memory. 
**      14-Dec-2009 (maspa05) bug 122642
**          #ifdef out the above change for Windows. The rest of the change
**          does not apply to Windows so the variables aren't defined.
*/
STATUS
LGK_initialize(
i4	  	flag,
CL_ERR_DESC	*sys_err,
char		*lgk_info)
{
    PTR		ptr;
    SIZE_TYPE	memleft;
    SIZE_TYPE	size;
    STATUS	ret_val;
    STATUS	mem_exists;
    char	mem_name[15];
    SIZE_TYPE	allocated_pages;
    i4		me_flags;
    i4		me_locked_flag;
    SIZE_TYPE	memory_needed;
    char	*nm_string;
    SIZE_TYPE	pages;
    LGK_MEM	*lgk_mem;
    i4		err_code;
    SIZE_TYPE   min_memory;
    i4		retries;
    i4		i;
    i4		attached;
    PID		*my_pid_slot;
    i4		clustered;
    u_i4	nodes;
    SIZE_TYPE	cxmemreq;
    PTR		pcxmem;
    LGLK_INFO	lgkcount;
    char	instid[4];

    CL_CLEAR_ERR(sys_err);

    /*
    ** if LGK_base is set then this routine has already been called.  It is
    ** set up so that both LGiniitalize and LKinitialize calls it, but only
    ** the first call does anything.
    */

    if (LGK_base.lgk_mem_ptr)
	return(OK);

    PCpid( &LGK_my_pid );

    memory_needed = 0;
    NMgtAt("II_LG_MEMSIZE", &nm_string);
    if (nm_string && *nm_string)
#if defined(LP64)
	if (CVal8(nm_string, (long*)&memory_needed))
#else
	if (CVal(nm_string, (i4 *)&memory_needed))
#endif /* LP64 */
	    memory_needed = 0;

    /* Always calculate memory needed from PM resource settings  */
    /* and compare with supplied value, if supplied value is less */
    /* than minimum then use minimum                             */

    min_memory = 0;
    if ( OK == lgk_get_counts(&lgkcount, FALSE))
    {
	if ( lgk_calculate_size(FALSE, &lgkcount, &min_memory) )
	{
	    /*
	    ** Memory exceeds MAX_SIZE_TYPE, can't continue.
	    ** 
	    ** Do calculation again, this time with "wordy"
	    ** so user can see allocation bits, then quit.
	    */
	    lgk_calculate_size(TRUE, &lgkcount, &min_memory);
	    return (E_DMA802_LGKINIT_ERROR); 
	}
    }
    if (min_memory)
       memory_needed = (memory_needed < min_memory) ? min_memory
                                                    : memory_needed;
    else
       memory_needed = (memory_needed < 400000 ) ? 400000 
                                                 : memory_needed;

    clustered = (i4)CXcluster_enabled();
    cxmemreq = 0;
    if ( clustered )
    {

	if ( OK != CXcluster_nodes( &nodes, NULL ) )
	    nodes = 0;
	cxmemreq = CXshm_required( 0, nodes, lgkcount.lgk_max_xacts,
		    lgkcount.lgk_max_locks, lgkcount.lgk_max_resources );
	if ( MAX_SIZE_TYPE - memory_needed < cxmemreq )
	{
	    /*
	    ** Memory exceeds MAX_SIZE_TYPE, can't continue.
	    ** 
	    ** Do calculation again, this time with "wordy"
	    ** so user can see allocation bits, then quit.
	    */
	    SIprintf("Total LG/LK/CX allocation exceeds max of %lu bytes by %lu\n"
	    	     "Adjust logging/locking configuration values and try again\n",
		         MAX_SIZE_TYPE, cxmemreq - (MAX_SIZE_TYPE - memory_needed));
	    lgk_calculate_size(TRUE, &lgkcount, &min_memory);
	    return (E_DMA802_LGKINIT_ERROR); 
	}
	memory_needed += cxmemreq;
    }

    if ( memory_needed < MAX_SIZE_TYPE - ME_MPAGESIZE )
	pages = (memory_needed + ME_MPAGESIZE - 1) / ME_MPAGESIZE;
    else
        pages = memory_needed / ME_MPAGESIZE;

    /*
    ** Lock the LGK segment if requested to do so
    */
    if (flag & LOCK_LGK_MEMORY)
	me_locked_flag = ME_LOCKED_MASK;
    else
	me_locked_flag = 0;

    me_flags = (me_locked_flag | ME_MSHARED_MASK | ME_IO_MASK | 
		ME_CREATE_MASK | ME_NOTPERM_MASK | ME_MZERO_MASK);
    if (CXnuma_user_rad())
        me_flags |= ME_LOCAL_RAD;

    STcopy("lglkdata.mem", mem_name);

    /*
    ** In general, we just want to attach to the shared memory and detect if
    ** we are the first process to do so. However, there are ugly race
    ** conditions to consider, as well as complications because the shared
    ** memory may be left around following a system crash.
    **
    ** First we attempt to create the shared memory. Usually it already exists,
    ** so we check for and handle the case of "already exists".
    */

    /*
    ** (jenjo02)
    **
    ** Restructured to better handle all those ugly race conditions
    ** which are easily reproduced by running two scripts, one that
    ** continuously executes "lockstat" while the other is starting
    ** and stopping Ingres.
    **
    ** For example,
    **
    **		lockstat A	acquires and init's the memory
    **		RCP		attaches to "A" memory
    **		lockstat A	terminates normally
    **		lockstat B	attaches to "A" memory, sees that
    **				"A"s pid is no longer alive, and
    **				reinitializes the memory, much to
    **				the RCP's chagrin.
    ** or (more commonly)
    **
    **		lockstat A	acquires and begins to init the mem
    **		RCP		attaches to "A" memory which is
    **				still being zero-filled by lockstat,
    **				checks the version number (zero),
    **				and fails with a E_DMA434 mismatch.
    **
    ** The fix utilizes the mem_ext_sem to synchronize multiple
    ** processes; if the semaphore hasn't been initialized or
    ** if mem_version_no is zero, we'll wait one second and retry,
    ** up to 60 seconds before giving up. This gives the creating
    ** process time to complete initialization of the memory.
    **
    ** Up to LGK_MAX_PIDS are allowed to attach to the shared
    ** memory. When a process attaches it sets its PID in the
    ** first vacant slot in lgk_mem->mem_pid[]; if there are
    ** no vacant slots, the attach is refused. When the process
    ** terminates normally by calling LGK_rundown(), it zeroes
    ** its PID slot.
    **
    ** When attaching to an existing segment, we check if  
    ** there are any live processes still using the memory;
    ** if so, we can't destroy it (no matter who created it).
    ** If there are no live processes attached to the memory,
    ** we destroy and reallocate it (based on current config.dat
    ** settings).
    */

    for ( retries = 0; ;retries++ )
    {
	LGK_base.lgk_mem_ptr = (PTR)NULL;
	
	/* Give up if unable to get memory in one minute */
	if ( retries )
	{
	    if ( retries < 60 )
		PCsleep(1000);
	    else
	    {
		/* Another process has it blocked way too long */
		uleFormat(NULL, E_DMA800_LGKINIT_GETMEM, (CL_ERR_DESC *)NULL,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		/* Unable to attach allocated shared memory segment. */
		return (E_DMA802_LGKINIT_ERROR); 
	    }
	}

	ret_val = MEget_pages(me_flags,
				pages, mem_name, (PTR*)&lgk_mem,
				&allocated_pages, sys_err);

	if ( mem_exists = ret_val )
	{
	    if (ret_val == ME_ALREADY_EXISTS)
	    {
		ret_val = MEget_pages((me_locked_flag | 
				       ME_MSHARED_MASK | ME_IO_MASK),
				      pages, mem_name, (PTR*)&lgk_mem,
				      &allocated_pages, sys_err);
	    }
	    if (ret_val)
	    {
		uleFormat(NULL, ret_val, sys_err,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		uleFormat(NULL, E_DMA800_LGKINIT_GETMEM, (CL_ERR_DESC *)NULL,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		/* Unable to attach allocated shared memory segment. */
		return (E_DMA802_LGKINIT_ERROR); 
	    }
	}
	else if (flag & LOCK_LGK_MUST_ATTACH)
	{	
	    /* Do not use the shared segment you just allocated */
	    MEfree_pages((PTR)lgk_mem, allocated_pages, sys_err);
	    return (E_DMA812_LGK_NO_SEGMENT); 
	}

	size = allocated_pages * ME_MPAGESIZE;

	/* Expose this process to the memory */
	LGK_base.lgk_mem_ptr = (PTR)lgk_mem;

	if ( mem_exists )
	{
	    /*
	    ** Memory exists.
	    **
	    ** Try to acquire the semaphore. If it's
	    ** uninitialzed, retry from the top.
	    **
	    ** If the version is zero, then another
	    ** process is initializing the memory;
	    ** keep retrying until the version is 
	    ** filled in.
	    **
	    */
	    if ( ret_val = CSp_semaphore(1, &lgk_mem->mem_ext_sem) )
	    {
		if ( ret_val != E_CS000A_NO_SEMAPHORE )
		{
		    uleFormat(NULL, ret_val, sys_err,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    ret_val = E_DMA802_LGKINIT_ERROR;
		    break;
		}
		continue;
	    }

	    /* Retry if still being init'd by another process */
	    if ( !lgk_mem->mem_version_no )
	    {
		CSv_semaphore(&lgk_mem->mem_ext_sem);
		continue;
	    }

	    /*
	    ** Check pids which appear to be attached to
	    ** the memory:
	    **
	    ** If any process is still alive, then we
	    ** assume the memory is consistent and use it.
	    **
	    ** If a process is now dead, it terminated
	    ** without going through LGK_rundown
	    ** to zero its PID slot, zero it now.
	    **
	    ** If there are no live PIDs attached to 
	    ** the memory, we destroy and recreate it.
	    */
	    my_pid_slot = (PID*)NULL;
	    attached = 0;

	    for ( i = 0; i < LGK_MAX_PIDS; i++ )
	    {
		if ( lgk_mem->mem_pid[i] && 
		     PCis_alive(lgk_mem->mem_pid[i]) )
		{
		    attached++;
		}
		else
		{
		    /* Vacate the slot */
		    if (lgk_mem->mem_pid[i])
		    {
			uleFormat(NULL, E_DMA499_DEAD_PROCESS_INFO, (CL_ERR_DESC *)NULL,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
				0, lgk_mem->mem_pid[i],
				0, lgk_mem->mem_info[i].info_txt);
		    }
		    lgk_mem->mem_pid[i] = (PID)0;
		    lgk_mem->mem_info[i].info_txt[0] = EOS;

		    /* Use first vacant slot for this process */
		    if ( !my_pid_slot )
		    {
			my_pid_slot = &lgk_mem->mem_pid[i];
			LGK_base.lgk_pid_slot = i;
		    }
		}
		/* Quit when both questions answered */
		if ( attached && my_pid_slot )
		    break;
	    }

	    /* If no living pids attached, destroy/reallocate */
	    if ( !attached )
	    {
		CSv_semaphore(&lgk_mem->mem_ext_sem);
		if ( LGK_destroy(allocated_pages, sys_err) )
		{
		    ret_val = E_DMA802_LGKINIT_ERROR;
		    break;
		}
		continue;
	    }

	    /* All attached pids alive? */
	    if ( !my_pid_slot )
	    {
		/* ... then there's no room for this process */
		uleFormat(NULL, E_DMA80A_LGK_ATTACH_LIMIT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
		    0, attached);
	        ret_val = E_DMA802_LGKINIT_ERROR;
	    }
	    else if (lgk_mem->mem_version_no != LGK_MEM_VERSION_CURRENT)
	    {
		uleFormat(NULL, E_DMA434_LGK_VERSION_MISMATCH, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, lgk_mem->mem_version_no, 0, LGK_MEM_VERSION_CURRENT);
		ret_val = E_DMA435_WRONG_LGKMEM_VERSION;
	    }
	    /*
	    ** Don't allow mixed connections of MT/non-MT processes.
	    ** Among other things, the mutexing mechanisms are 
	    ** incompatible!
	    */
	    else if ( (CS_is_mt() && (lgk_mem->mem_status & LGK_IS_MT) == 0) ||
		     (!CS_is_mt() &&  lgk_mem->mem_status & LGK_IS_MT) )
	    {
		uleFormat(NULL, E_DMA811_LGK_MT_MISMATCH, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, (lgk_mem->mem_status & LGK_IS_MT) ? "OS"
							 : "INTERNAL",
		    0, (CS_is_mt()) ? "OS"
				    : "INTERNAL");
		ret_val = E_DMA802_LGKINIT_ERROR;
	    }
	    else
	    {
		/*
		** CX memory (if any) will lie immediately past LGK header.
		*/
		pcxmem = (PTR)(lgk_mem + 1);
		pcxmem = (PTR)ME_ALIGN_MACRO(pcxmem, sizeof(ALIGN_RESTRICT));

		LGK_base.lgk_lkd_ptr = (char *)LGK_base.lgk_mem_ptr +
					lgk_mem->mem_lkd;
		LGK_base.lgk_lgd_ptr = (char *)LGK_base.lgk_mem_ptr +
					lgk_mem->mem_lgd;
		
		/* Stuff our pid in first vacant slot */
		*my_pid_slot = LGK_my_pid;
		STlcopy(lgk_info, lgk_mem->mem_info[i].info_txt, LGK_INFO_SIZE-1);
	    }

#if defined(VMS) || defined(UNIX)
	    /* set up pointers to reference the uuid mutex and last time
	     * variable */

	    if (!ID_uuid_sem_ptr)
           	ID_uuid_sem_ptr=&lgk_mem->id_uuid_sem;

	    if (!ID_uuid_last_time_ptr)
                ID_uuid_last_time_ptr=&lgk_mem->uuid_last_time;

	    if (!ID_uuid_last_cnt_ptr)
                ID_uuid_last_cnt_ptr=&lgk_mem->uuid_last_cnt;
#endif

	    CSv_semaphore(&lgk_mem->mem_ext_sem);
	}
	else
	{

	    /* Memory did not exist */
	    /* Zero the version to keep other processes out */
	    lgk_mem->mem_version_no = 0;

#if defined(VMS) || defined(UNIX)
	    /* set up the uuid mutex and last time pointers to
	     * reference the objects in shared memory */

	    {
	        STATUS id_stat;

	        ID_uuid_sem_ptr=&lgk_mem->id_uuid_sem;
                ID_uuid_last_time_ptr=&lgk_mem->uuid_last_time;
                ID_uuid_last_cnt_ptr=&lgk_mem->uuid_last_cnt;
	        *ID_uuid_last_cnt_ptr=0;
	        ID_UUID_SEM_INIT(ID_uuid_sem_ptr,CS_SEM_MULTI,"uuid sem",
				&id_stat);
	    }
#endif

	    /* ... then initialize the mutex */
	    CSw_semaphore(&lgk_mem->mem_ext_sem, CS_SEM_MULTI,
	    			    "LGK mem ext sem" );

	    /* Record if memory created for MT or not */
	    if ( CS_is_mt() )
		lgk_mem->mem_status = LGK_IS_MT;

	    /*
	    ** memory is as follows:
	    **
	    **	-----------------------------------------------------------|
	    **	| LGK_MEM struct (keep track of this mem)	           |
	    **	|							   |
	    **	-----------------------------------------------------------|
	    **	| If a clustered installation memory reserved for CX       |
	    **	|							   |
	    **	------------------------------------------------------------
	    **	| LKD - database of info for lk system			   |
	    **	|							   |
	    **	------------------------------------------------------------
	    **	| LGD - database of info for lg system			   |
	    **	|							   |
	    **	------------------------------------------------------------
	    **	| memory manipulated by LGKm_* routines for structures used |
	    **	| by both the lk and lg systems.			   |
	    **	|							   |
	    **	------------------------------------------------------------
	    */

	    /* put the LGK_MEM struct at head of segment leaving ptr pointing 
	    ** at next aligned piece of memory
	    */

	    /*
	    ** CX memory (if any) will lie immediately past LGK header.
	    */
	    pcxmem = (PTR)(lgk_mem + 1);
	    pcxmem = (PTR)ME_ALIGN_MACRO(pcxmem, sizeof(ALIGN_RESTRICT));

	    LGK_base.lgk_lkd_ptr = pcxmem + cxmemreq;
	    LGK_base.lgk_lkd_ptr = (PTR) ME_ALIGN_MACRO(LGK_base.lgk_lkd_ptr,
						sizeof(ALIGN_RESTRICT));
	    lgk_mem->mem_lkd = (i4)((char *)LGK_base.lgk_lkd_ptr -
					 (char *)LGK_base.lgk_mem_ptr);

	    LGK_base.lgk_lgd_ptr = (PTR) ((char *) LGK_base.lgk_lkd_ptr +
					    sizeof(LKD));
	    LGK_base.lgk_lgd_ptr = (PTR) ME_ALIGN_MACRO(LGK_base.lgk_lgd_ptr,
						sizeof(ALIGN_RESTRICT));
	    lgk_mem->mem_lgd = (i4)((char *)LGK_base.lgk_lgd_ptr -
					 (char *)LGK_base.lgk_mem_ptr);

	    /* now initialize the rest of memory for allocation */

	    /* how much memory is left? */

	    ptr = ((char *)LGK_base.lgk_lgd_ptr + sizeof(LGD));
	    memleft = size - (((char *) ptr) - ((char *) LGK_base.lgk_mem_ptr));

	    if ( (ret_val = lgkm_initialize_mem(memleft, ptr)) == OK &&
		 (ret_val = LG_meminit(sys_err)) == OK &&
		 (ret_val = LK_meminit(sys_err)) == OK )
	    {
		/* Clear array of attached pids and pid info */
		for ( i = 0; i < LGK_MAX_PIDS; i++ )
		{
		    lgk_mem->mem_pid[i] = (PID)0;
		    lgk_mem->mem_info[i].info_txt[0] = EOS;
		}

		/* Set the creator pid */
		LGK_base.lgk_pid_slot = 0;
		lgk_mem->mem_creator_pid = LGK_my_pid;

		/* Set the version, releasing other processes */
		lgk_mem->mem_version_no = LGK_MEM_VERSION_CURRENT;
	    }
	    else
	    {
		uleFormat(NULL, ret_val, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		ret_val = E_DMA802_LGKINIT_ERROR;

		/* Destroy the shared memory */
		LGK_destroy(allocated_pages, sys_err);
	    }
	}

	if ( ret_val == OK )
	{
	    PCatexit(LGK_rundown);

	    if ( clustered )
	    {
		/*
		** Perform preliminary cluster connection and CX memory init.
		*/

		/* Get installation code */
		NMgtAt("II_INSTALLATION", &nm_string);
		if ( nm_string )
		{
		    instid[0] = *(nm_string);
		    instid[1] = *(nm_string+1);
		}
		else
		{
		    instid[0] = 'A';
		    instid[1] = 'A';
		}
		instid[2] = '\0';
		ret_val = CXinitialize( instid, pcxmem, flag & LGK_IS_CSP );
		if ( ret_val )
		{
		    /* Report error returned from CX */
		    uleFormat(NULL, ret_val, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0 );
		    break;
		}
	    }

#ifdef VMS
	    {
	    static $EXHDEF	    exit_block;
	    i4			ctrl_y_mask = 0x02000000;

	    /*
	    ** On VMS, programs like the dmfjsp and logstat run as images in
	    ** the shell process. That is, the system doesn't start and stop
	    ** a process for each invocation of the program, it just starts
	    ** and stops an image in the same process. This means that if
	    ** the program should die, the image may be rundown but the process
	    ** will remain, which means that the check-dead threads of other
	    ** processes in the installation will not feel that they need to
	    ** rundown this process, since it's still alive.
	    **
	    ** By declaring an exit handler, which will get a chance to run
	    ** even if PCexit isn't called, we improve our chances of getting
	    ** to perform rundown processing if we should die unexpectedly.
	    **
	    ** Furthermore, we ask DCL to disable its ^Y processing, which
	    ** lessens the chance that the user will interrupt us while we
	    ** are holding the semaphore.
	    */
	    exit_block.exh$g_func = LGK_rundown;
	    exit_block.exh$l_argcount = 1;
	    exit_block.exh$gl_value = &exit_block.exh$l_status;

	    if (sys$dclexh(&exit_block) != SS$_NORMAL)
		ret_val = FAIL;

	    lib$disable_ctrl(&ctrl_y_mask, 0);
	    }
#endif
	}
	break;
    }

    if ( ret_val )
	LGK_base.lgk_mem_ptr = NULL;

    return(ret_val);
}

/*{
** Name: LGK_destroy()	- destroy shared memory segment used by lg and lk
**
** Description:
**	Destroy the shared memory segment.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    !OK		couldn't destroy it for some reason.
**	    OK		success.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	30-Apr-2003 (jenjo02)
**	    Destroy the mem_ext_sem. BUG 110121.
**	19-Jun-2003 (jenjo02)
**	    Prototyped function, which is a static. Before
**	    destroying the memory, free it as some OS's 
**	    (read "Linux") insist, returning EIDRM if the 
**	    process still has the memory "pinned", destroys it,
**	    then attempts to MEget_pages it again. This does
**	    not seem to trouble other OS's (Solaris, etal).
*/
static STATUS 
LGK_destroy(
SIZE_TYPE	pages,
CL_ERR_DESC *sys_err)
{
    STATUS	ret_val;
    i4		err_code;
    LGK_MEM	*lgk_mem = (LGK_MEM *)LGK_base.lgk_mem_ptr;

    /* Destroy the semaphore */
    CSr_semaphore(&lgk_mem->mem_ext_sem);
    CS_cp_sem_cleanup("lglkdata.mem", sys_err);
    MEfree_pages((PTR)lgk_mem, pages, sys_err);
    ret_val = MEsmdestroy("lglkdata.mem", sys_err);
    if (ret_val)
    {
	uleFormat(NULL, ret_val, (CL_ERR_DESC *)sys_err,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	ret_val = E_DMA803_LGKDEST_ERROR;
    }
    LGK_base.lgk_mem_ptr = NULL;

    return(ret_val);
}

/*{
** Name: LGK_rundown() - Called upon exit from a process.
**
** Description:
**	Upon normal exit from a process this routine will be called from
**	the PCexit() routine.  It calls the respective LK_rundown and
**	LG_rundown routines.   On unix there is no way to guarantee that
**	this routine will be called (uncaught signals, kill -9, ...).
**
**	If for some reason a process goes away with calling this routine
**	all their data will remain in the LG and LK tables.  The dmfrcp
**	on unix wakes up periodically and checks whether or not processes
**	are still there.  If they are not then it calls the rundown routines
**	on their behalf.  See the lg code for more info.
**
** Inputs:
**	status			unused - required by on_exit()
**	arg			unused - required by on_exit()
**
** Outputs:
**	none.
**
**	Returns:
**	    void
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	23-may-1994 (bryanp)
**	    On VMS, disable ^Y for LG/LK-aware processes. We don't want to allow
**		^Y because you might interrupt the process right in the middle
**		of an LG or LK operation, while holding the shared memory
**		semaphore, and this would then wedge the whole installation.
**	18-apr-2001 (devjo01)
**	    Call CXterminate() on exit if clustered.
**	30-Apr-2003 (jenjo02)
**          Lock mutex. Zero this process's PID slot. BUG 110121.
*/
/* ARGSUSED */
void
LGK_rundown(status, arg)
int	status;
int	arg;
{
    i4		ctrl_y_mask = 0x02000000;
    static i4	already_run = 0;
    LGK_MEM	*lgk_mem = (LGK_MEM *)LGK_base.lgk_mem_ptr;

    if (!already_run)
    {
	already_run = 1;

	if ( lgk_mem )
	{
	    LK_rundown((i4) LGK_my_pid);
	    LG_rundown((i4) LGK_my_pid);

	    /* Clean our pid slot */
	    CSp_semaphore(1, &lgk_mem->mem_ext_sem);
	    if ( lgk_mem->mem_pid[LGK_base.lgk_pid_slot] == LGK_my_pid )
	    {
		lgk_mem->mem_pid[LGK_base.lgk_pid_slot] = (PID)0;
		lgk_mem->mem_info[LGK_base.lgk_pid_slot].info_txt[0] = EOS;
	    }
	    CSv_semaphore(&lgk_mem->mem_ext_sem);

	    if ( CXcluster_enabled() )
		CXterminate(0);
#ifdef VMS
	    lib$enable_ctrl(&ctrl_y_mask, 0);
#endif
	}
    }

    return;
}

/*{
** Name: LGK_deadinfo() - Dead process write info and cleanup.
**
** Description:
**	Called from LG_dead_check to print info for a dead PID and to
**	clean up the shared memory slot, rather than waiting for
**	LGK_initialize to stumble on the slot.
**
** Inputs:
**	PID (noticed by check_dead)
**
** Outputs:
**	dead process error message E_DMA499_DEAD_PROCESS_INFO
**	cleanup of PID and info slot for dead process
**
** Returns:
**	void
**
** History:
**	15-May-2007 (toumi01)
**	    Written. For supportability add process info to shared memory.
*/
void
LGK_deadinfo(PID dead_pid)
{
    LGK_MEM	*lgk_mem = (LGK_MEM *)LGK_base.lgk_mem_ptr;
    i4		i;
    i4		err_code;

    CSp_semaphore(1, &lgk_mem->mem_ext_sem);

    for ( i = 0; i < LGK_MAX_PIDS; i++ )
    {
	if ( lgk_mem->mem_pid[i] && 			/* sanity check */
	     lgk_mem->mem_pid[i] == dead_pid &&		/* the dead pid */
	     !(PCis_alive(lgk_mem->mem_pid[i])) )	/* sanity check */
	{
	    /* Spit out some info and vacate the slot */
	    uleFormat(NULL, E_DMA499_DEAD_PROCESS_INFO, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		0, lgk_mem->mem_pid[i],
		0, lgk_mem->mem_info[i].info_txt);
	    lgk_mem->mem_pid[i] = (PID)0;
	    lgk_mem->mem_info[i].info_txt[0] = EOS;
	}
    }

    CSv_semaphore(&lgk_mem->mem_ext_sem);

    return;
}
