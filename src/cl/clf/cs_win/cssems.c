/*
** Copyright (c) 1998, 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <sp.h>
#include    <st.h>
#include    <cs.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <tr.h>
#include    <csinternal.h>
#include    <mo.h>
#include    <di.h>
#include    "csmgmt.h"
#include    "cslocal.h"

/*
**  Name: CSSEM.C - Control System Semaphore Routines
**
**  Description:
**      This module contains those routines which provide the user interface to
**  system dispatching services' semaphoring routines:
**
**  CSi_semaphore() - Initialize a semaphore
**  CSp_semaphore() - Request and wait (if necessary) for a semaphore
**  CSv_semaphore() - Release a semaphore owned by a thread.
**  CSr_semaphore() - Remove a semaphore
**
**  History:
**	22-aug-1995 (canor01)
**	    remove references to ME_stream_sem which is not needed in NT
**	    and move the SemaphoreInitCriticalSection around the walking
**	    of the list, not around GetSemHandle()
**	12-sep-1995 (shero03)
**	    Added sync_obj
**	25-sep-1995 (shero03)
**	    Make performance improvements(e.g. reorder the multi-list, etc)
**	17-oct-95 (emmag)
**	    Add include for me.h
**	21-feb-1996 (canor01)
**	    Add system-wide shared memory mutex for shared semaphore access.
**	26-feb-1996 (canor01)
**	    Clean up cross-process shared semaphores.  Protect the slots
**	    in shared memory with the shared memory mutex, but protect
**	    access to the overflow chains with a CriticalSection.
**	23-apr-1996 (canor01)
**	    Store thread id in cs_sid member of semaphore
**	    structure.
**	24-apr-1996 (canor01)
**	    A semaphore request should be allowed to fail if the thread
**	    has been removed (CS_ABORTED_MASK set).
**	23-sep-1996 (canor01)
**	    Keep in sync with Unix version of statistics.
**	11-nov-1996 (canor01)
**	    Update shared semaphore algorithms to match those used in the
**	    Unix CL.
**      11-Dec-96 (fanra01)
**          Add CloseHandle call for the cs_cond event handle.
**	24-feb-1997 (cohmi01)
**	    Ch. FAIL retcodes to E_CSxxxx codes, else callers issue
**	    E_US0001_NUM_USERS message as that is the numerical equiv. of
**	    FAIL.  Bug 80050, 80242
**	19-Feb-1997 (jenjo02)
**          Do not register semaphores with MO; delete calls to
**          CS_sem_attach() and CS_detach_sem().
**      04-mar-1997 (canor01)
**          cs_thread_ordinal member of semaphore is now cs_sid.
**	12-may-1997 (canor01)
**	    Clean up potential leak in walking the handle list, plus
**	    consolidate a couple of the mutex-creation routines into a
**	    single function.
**	14-may-97 (mcgem01)
**	    Cleaned up compiler warnings.
**	27-jun-97 (mcgem01)
**	    Force semaphores to be exclusive.
**      08-jul-1997 (canor01)
**          Maintain semaphores on known semaphore list.
**      04-aug-97 (mcgem01)
**          During shutdown we free memory where a semaphore is allocated
**          before removing the semaphore.  This can play havoc with the known
**          semaphore list and cause an access violation during shutdown.
**	    Back out this change for now.
**	03-nov-1997 (canor01)
**	    Remove the event member of the semaphore structure to cut down
**	    on number of system resources being consumed.
**	08-dec-1997 (canor01)
**	    When resources are so low that we can't create a mutex, try
**	    freeing up file resources through DIlru.  If we can't free them,
**	    we're in deep trouble.
**	03-mar-1998 (canor01)
**	    Added CS_cp_sem_cleanup().
**      04-mar-1998 (canor01)
**          Parameters to DIlru_flush were changed on Unix side.  Change
**          them here too.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always
**	    a pointer to the thread's SCB, never OS thread_id, which is
**	    contained in cs_thread_id.
**	    Don't skew stats by counting share vs share temporary collisions;
**	    count only share vs excl.
**	10-jan-2000 (somsa01)
**	    In CSv_semaphore(), set CS_MUTEX when waiting for the semaphore.
**	24-jul-2000 (somsa01 for jenjo02)
**	    Removed dubious cs_sem_init_pid, cs_sem_init_addr from
**	    CS_SEMAPHORE, replaced cs_sid with cs_thread_id.
**	28-jun-2001 (somsa01)
**	    Since we may be dealing with Terminal Services, prepend
**	    "Global\" to the shared memory name, to specify that this
**	    shared memory segment is to be opened from the global name
**	    space.
**	29-mar-2002 (somsa01)
**	    In CSn_semaphore() and CSw_semaphore(), to allow for multiple
**	    installations, tack on the installation identifier in semaphore
**	    names.
**	11-sep-2003 (somsa01)
**	    Only get the instance identifier for semaphore names once for
**	    the process.
**	30-dec-2003 (wanfr01)
**	    Bug 111527, INGSRV2648
**	    All named mutex have the same name after change 465399.
**	    Now store only the Version name in a global 'VersionString'
**	23-jan-2004 (somsa01 for jenjo02)
**	    CSMTget_scb() only when we must, like when the underlying
**	    mutex is blocked and we must change scb->cs_state. Drilling
**	    down to find the thread's SCB turns out to be fairly
**	    expensive when one considers the number of CSp/v calls.
**	    Instead of the semaphore owner (sem->cs_sid) being
**	    scb->cs_self (which requires finding the SCB), the
**	    thread_id is used instead, which can be had by a quick
**	    CS_get_thread_id() call.
**	22-jul-2004 (somsa01)
**	    Removed unnecessary include of erglf.h.
**  20-Jul-2004 (lakvi01)
**      SIR#112703, cleaned-up warnings.
**  26-Jul-2004 (lakvi01)
**      Backed-out the above change to keep the open-source stable.
**      Will be revisited and submitted at a later date. 
**  27-Sep-2005 (fanra01)
**      Bug 115298
**      A service started as LocalSystem user will inherit special security
**      and environment settings from the service control manager as default.
**      If the service processes do not specify security attributes the
**      defaults are used preventing access between a service and a user
**      application.
**      Add security attributes for the creation of cross process objects.
**	06-Dec-2006 (drivi01)
**	    Adding support for Vista, Vista requires "Global\" prefix for
**	    shared objects as well.  Replacing calls to GVosvers with 
**	    GVshobj which returns the prefix to shared objects.
**  06-May-2008 (whiro01)
**     Cleanup CS_init_sem being left owned on errors, buffer overflows
**     with long semaphore names, consolidate duplicated code.
**  16-Feb-2009 (fanra01)
**     BUG 122443
**     Change type of synchronization object used for single semaphores.
*/

typedef struct _CS_SEM_LIST CS_SEM_LIST;

typedef struct _CS_SEM_LIST {
	CS_SEM_LIST  *pNext;
	CS_SEMAPHORE *pSem;
	HANDLE       hMutex;
};

/******************************************************************************
**
**  Forward and/or External function references.
**
******************************************************************************/

/******************************************************************************
**
** System-wide shared memory mutex.  Needed only because opening an unnamed
** mutex semaphore with the same address does not cause an error.  If this
** ever changes, *please* get rid of this ugliness. -seg
**
******************************************************************************/
GLOBALREF HANDLE CS_init_sem;
GLOBALREF i4     Cs_desched_usec_sleep;

/* Linked list of semaphores */
GLOBALREF CRITICAL_SECTION SemaphoreInitCriticalSection;
static CS_SEM_LIST	*pCPsem_list = NULL;
static i4 		cntSemList = 0;

static char		sem_name[CS_SEM_NAME_LEN] = "";
static char		VersionString[256] = "";
static char		*ObjectPrefix = NULL;

/*
** Function pointer to hold the address of the TryEnterCriticalSection
** function.
*/
static BOOL (WINAPI *pfnTryEnterCriticalSection)( 
                        LPCRITICAL_SECTION lpCriticalSection
					) = NULL;

GLOBALREF CS_SYSTEM	Cs_srv_block;

FUNC_EXTERN VOID	GVshobj(char **ObjectPrefix);
STATUS			iimksecdacl(SECURITY_ATTRIBUTES *sa);

/******************************************************************************
**
**  Get semaphore from linked list
**    If it can't be found, then create a new one and link in the list
**
**  History:
**	25-sep-1995 (shero03)
**	    If there are more than 4 members in the list, move the requested
**	    sem to the front of the list.
**
******************************************************************************/
static
HANDLE
GetMultiSem(CS_SEMAPHORE *sem, SECURITY_ATTRIBUTES *pSa, DWORD *error)
{
	CS_SEM_LIST *pCPsem;
	CS_SEM_LIST *pPrevSem = NULL;
	HANDLE hSem = NULL;

	/* Initialize the OS error code returned */
	*error = ERROR_SUCCESS;

	EnterCriticalSection(&SemaphoreInitCriticalSection);
	for (pCPsem = pCPsem_list; pCPsem != NULL; pCPsem = pCPsem->pNext)
	{
	    if (pCPsem->pSem == sem)
	    {
		if ((pPrevSem != NULL) &&	/* move the sem to head of list */
		    (cntSemList > 4) )
		{
		    pPrevSem->pNext = pCPsem->pNext;
		    pCPsem->pNext = pCPsem_list;
		    pCPsem_list = pCPsem;
		}
		/* We found a matching entry in the list */
		break;
	    }
	    pPrevSem = pCPsem;
	}
	if (pCPsem == NULL) 
	{
	    if ((pCPsem  = (CS_SEM_LIST *) 
		MEreqmem(0,sizeof(CS_SEM_LIST),FALSE,NULL)) != NULL)
	    {
		/* Add new entry to head of the list */
		pCPsem->pNext = pCPsem_list;
		pCPsem_list = pCPsem;
		cntSemList++;

		pCPsem->pSem = sem;
		pCPsem->hMutex  = CreateMutex(pSa,
					FALSE,
			                sem->cs_sem_name);
		if (pCPsem->hMutex == NULL)
		    *error = GetLastError();
	    }
	    else
		*error = ERROR_NOT_ENOUGH_MEMORY;
	}
	if (pCPsem != NULL)
	    hSem = pCPsem->hMutex;
	LeaveCriticalSection(&SemaphoreInitCriticalSection);
	return hSem;
}

static
STATUS
RemoveMultiSem( CS_SEMAPHORE *sem )
{
	CS_SEM_LIST *pCPsem;
	CS_SEM_LIST *pPrev;
	STATUS status = OK;
	
	EnterCriticalSection(&SemaphoreInitCriticalSection);
	for (pPrev = NULL, pCPsem = pCPsem_list;
		pCPsem && (pCPsem->pSem != sem);
		pPrev = pCPsem, pCPsem = pCPsem->pNext) 
	{
	    ;
	}
	if (pCPsem == NULL) 
	{
	    status = E_CS000A_NO_SEMAPHORE;
	}
	else
	{
	    if (pPrev == NULL)
		pCPsem_list = pCPsem->pNext;
	    else
		pPrev->pNext = pCPsem->pNext;
	}

	cntSemList--;
	LeaveCriticalSection(&SemaphoreInitCriticalSection);

	MEfree((PTR)pCPsem);

	return status;
}

static
HANDLE
CreateNewSemHandle( CS_SEMAPHORE *sem, i4  index, SECURITY_ATTRIBUTES *pSa, DWORD *error)
{
    HANDLE hSem;
    *error = ERROR_SUCCESS;

    sem->cs_multi_lst[index].hSem = hSem = CreateMutex(pSa,
                        FALSE,
                        sem->cs_sem_name);
    sem->cs_multi_lst[index].pid = Cs_srv_block.cs_pid;
    if (hSem == NULL)
    {
        *error = GetLastError();
    }
    return (hSem);
}

/******************************************************************************
**
**
**
**  History:
**	25-sep-1995 (shero03)
**	    Move the Create Mutex outside the Critical Section
**	08-feb-1996 (canor01,jenjo02,cohmi01)
**	    Windows NT cross-process semaphores require unique handles that
**	    are valid only within the current process.  Cross-process
**	    semaphores are kept unique by name within the Windows NT kernel
**	    space, and each process accesses the semaphore by the handle.
**
**	    Since the CS_SEMAPHORE structure is in shared memory, a single
**	    handle cannot be associated with the structure.  Instead, there
**	    are CS_MULTI_LST_COUNT pid/handle pairs in the structure.  This
**	    allows fairly fast lookup of handles for the main servers (rcp,
**	    dbms, star), plus leaving room for pseudo-servers (rcpstat, 
**	    logstat, etc.).
**
**	    If the current process's pid is not found in one of the main
**	    slots, a critical section is entered, then 1) an unused slot
**	    is searched for, and if not found, 2) a slot with a pid for
**	    a process that is no longer running is reused, and if not
**	    found, 3) an entry in a linked list of overflow pid/handle
**	    pairs is created.
**
**	    This minimizes the time spent inside the critical section
**	    (which can only be entered by one thread per process at a
**	    time).
**	20-feb-1996 (canor01)
**	    If the semaphore handle is stale, i.e. it belonged to a different
**	    process with the same pid, force allocation of a new one.
**	26-mar-1996 (canor01)
**	    If we are in a CS_INITIALIZING state, treat it as if
**	    the shared memory slots are unavailable.
**  16-Feb-2009 (fanra01)
**      Modified the order for the conditional check for the more frequent
**      outcome.
**      Change the return type from HANDLE to void* to enable use of
**      CRITICAL_SECTION.
**
******************************************************************************/
static
void *
GetSemHandle(
	CS_SEMAPHORE *sem, 
	bool force, 
	DWORD *error )
{
	register int	    i;
	i4 	    pidslot = -1;
	HANDLE	hSem = NULL;

	SECURITY_ATTRIBUTES sa;

	iimksecdacl( &sa );

	/* Initialize "out" values */
	*error = ERROR_SUCCESS;

	if (sem->cs_type == CS_SEM_SINGLE) 
	{
        LPCRITICAL_SECTION csSem;
	    if (sem->cs_mutex == NULL) 
	    {
            if (CSi_semaphore(sem, CS_SEM_SINGLE) != OK)
                *error = sem->cs_thread_id;
	    }
	    csSem = sem->cs_mutex;
        return((void*)csSem);
	}
	else
	{
	    if (Cs_srv_block.cs_state != CS_INITIALIZING)
	    {
		for ( i = 0; i < CS_MULTI_LST_COUNT; i++ )
		{
		    if ( sem->cs_multi_lst[i].pid == Cs_srv_block.cs_pid )
		    {
			hSem = sem->cs_multi_lst[i].hSem;
			pidslot = i;
			break;
		    }
		}

		if ( hSem != NULL && !force )
		{
		    return hSem;
		}

		/*
		** No semaphore handle was found or we are being forced to 
		** create a new one to replace a stale handle from a dead process
		*/
		/* Serialize access to these shared-memory structures */
		WaitForSingleObject( CS_init_sem, INFINITE );

		if ( hSem != NULL )
		{
		    /* 
		    ** invalidate the semaphore handle currently
		    ** assigned to this PID
		    */
		    if ( pidslot >= 0 )
			sem->cs_multi_lst[pidslot].hSem = NULL;
		}
		else
		{
		    /*
		    ** Has another thread already done this
		    ** while we were waiting for the mutex?
		    */
		    if ( pidslot >= 0 && (hSem = sem->cs_multi_lst[pidslot].hSem) != NULL )
		    {
			ReleaseMutex( CS_init_sem );
			return hSem;
		    }
		}

		/* make sure another thread hasn't filled in the slot first */
		for ( i = 0; i < CS_MULTI_LST_COUNT; i++ )
		{
		    if ( sem->cs_multi_lst[i].pid == Cs_srv_block.cs_pid )
		    {
			if ((hSem = sem->cs_multi_lst[i].hSem) != NULL)
			{
			    ReleaseMutex( CS_init_sem );
			    return hSem;
			}
		    }
		}
		if ( pidslot != -1 )
		{
		    hSem = CreateNewSemHandle( sem, pidslot, &sa, error );
		    ReleaseMutex( CS_init_sem );
		    return hSem;
		}
		else
		{
		    /* pick an empty slot if there is one */
		    for ( i = 0; i < CS_MULTI_LST_COUNT; i++ )
		    {
			if ( sem->cs_multi_lst[i].pid == 0 )
			{
			    hSem = CreateNewSemHandle( sem, i, &sa, error );
			    ReleaseMutex( CS_init_sem );
			    return hSem;
			}
		    }
		}
		/* if no empty slots, re-use one if its process has gone away */
		for ( i = 0; i < CS_MULTI_LST_COUNT; i++ )
		{
		    if ( !PCis_alive( sem->cs_multi_lst[i].pid ) )
		    {
			hSem = CreateNewSemHandle( sem, i, &sa, error );
			ReleaseMutex( CS_init_sem );
			return hSem;
		    }
		}
		ReleaseMutex( CS_init_sem );
	    }

	    /* all slots are full from valid processes, so add to linked list */
	    if ( hSem == NULL )
	    {
		hSem = GetMultiSem( sem, &sa, error );
	    }
	}
	return ((void*)hSem);
}

/******************************************************************************
**
** Name: CSi_semaphore  - Initialize Semaphore.
**
** Description:
**  This routine is used to initialize a semaphore for use by the
**  CSp_semaphore and CSv_semaphore routines.  This should always
**  be called before the first use of a semaphore and needs only
**  be called once.  It is illegal to call CSi_semaphore on a
**  semaphore that is currently in use.
**
**  OS/2 note:  The last caveat ("It is illegal...") implies that we
**  can assume that any call on a non-zero mutex is invalid.  Also, it
**  implies that CSi_semaphore is not called on a shared semaphore that
**  has already been inited in another process.  This last observation
**  is true of current practice.
**
** Inputs:
**      sem             A pointer to the semaphore to initialize
**  type                Semaphore type:
**                      CS_SEM_SINGLE - normal semaphore
**                      CS_SEM_MULTI  - semaphore may be
**                      requested by threads in
**                      different processes.
**
** Outputs:
**      none
**  Returns:
**      OK
**      E_CS0004_BAD_PARAMETER      Illegal semaphore type specified.
**      E_CS0050_CS_SEMOP_GET       Could not create OS mutex object
**      CS_SEMAPHORE->cs_thread_id set to OS error on failure.
**  Exceptions:
**      none
**
** Side Effects:
**      If the mutex already exists, it will be connected to, and no
**      notice will be taken or given of the prior occurrence.
** History:
**      16-Feb-2009 (fanra01)
**          Replaced CreateMutex call with initialization of a critical
**          section.
**
******************************************************************************/
STATUS
CSi_semaphore(CS_SEMAPHORE *sem,
              i4           type)
{
	if ((type != CS_SEM_SINGLE) && (type != CS_SEM_MULTI))
		return E_CS0004_BAD_PARAMETER;

	MEfill(sizeof(CS_SEMAPHORE), 0, (char *) sem);

    if ( pfnTryEnterCriticalSection == NULL )
	{
        /*
        ** TryEnterCriticalSection is supported from Win2k.
        ** Assumes the kernel32 is already loaded into the address space.
        */
        pfnTryEnterCriticalSection = 
            (BOOL (WINAPI *)( LPCRITICAL_SECTION lpCriticalSection )) 
                GetProcAddress(GetModuleHandle("kernel32.dll"),
                    TEXT("TryEnterCriticalSection"));
	}

	/* Then do stuff that may be non-zero */
	sem->cs_type = (i2)type;
	sem->cs_sem_scribble_check = CS_SEM_LOOKS_GOOD;

	if (type == CS_SEM_SINGLE) 
	{
        sem->cs_mutex = &sem->cs_section;
        InitializeCriticalSection(sem->cs_mutex);
	}

	return OK;
}

/*
**  Name: CSp_semaphore()    - perform a "P" operation on a semaphore
**
**  Description:
**	This routine is called to allow a thread to request a semaphore
**	and if it is not available, to queue up for the semaphore.  The
**	thread requesting the semaphore will be returned to the ready
**	queue when the semaphore is V'd.
**
**	At the time that the semaphore is granted, the fact that a semaphore
**	is held by the thread is noted in the thread control block, to prevent
**	the stopping of the thread (via the monitor) while a semaphore is held.
**
**	Shared semaphores refer to an extension of the semaphore concept to
**	include a more general software lock.  This mechanism is used to
**	facilitate a more efficient reference count mechanism.
**
**	If the requested semaphore has a type CS_SEM_MULTI then that semaphore
**	may be located in shared memory and the granting/blocking on this
**	semaphore must work between multiple processes.  CS_SEM_MULTI semaphores
**	are expected to model spin locks and should only be held for short
**	periods of time.  Clients should never do operations which may cause
**	their threads to be suspended while holding a CS_SEM_MULTI semaphore.
**
**	All semaphores should be initialized with a CSi_semaphore call before
**	being used by CSp_semaphore().
**
**  Inputs:
**	exclusive	Boolean value representing whether this
**			request is for an exclusive semaphore
**	sem		A pointer to a CS_SEMAPHORE for which
**			the thread wishes to wait.
**
**  Outputs:
**	none
**
**	Returns:
**	    OK				- semaphore granted.
**	    E_CS000F_REQUEST_ABORTED	- request interrupted.
**	    E_CS0017_SMPR_DEADLOCK	- semaphore already held by requestor.
**	    E_CS0029_SEM_OWNER_DEAD	- owner process of semaphore has exited.
**
**	Exceptions:
**	    none
**
**  Side Effects:
**	none
**
**  History:
**	10-Aug-1995 (shero03)
**	    increment counters for stats
**	09-feb-1996 (canor01)
**	    add debugging code
**	20-feb-1996 (canor01)
**	    If the semaphore handle is stale, i.e. it belonged to a different
**	    process with the same pid, force allocation of a new one.
**	30-apr-1996 (canor01)
**	    Since CSp_semaphore() can be called to protect control blocks
**	    in the middle of other states, save the scb->cs_state value,
**	    CS_COMPUTABLE may not be the correct state following
**	    semaphore acquisition.
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always
**	    a pointer to the thread's SCB, never OS thread_id, which is
**	    contained in cs_thread_id.
**	    Don't skew stats by counting share vs share temporary collisions;
**	    count only share vs excl.
**	24-jul-2000 (somsa01 for jenjo02)
**	    Removed dubious cs_sem_init_pid, cs_sem_init_addr from
**	    CS_SEMAPHORE, replaced cs_sid with cs_thread_id.
**	23-jan-2004 (somsa01 for jenjo02)
**	    CSget_scb() only when we must, like when the underlying
**	    mutex is blocked and we must change scb->cs_state. Drilling
**	    down to find the thread's SCB turns out to be fairly
**	    expensive when one considers the number of CSp/v calls.
**	    Instead of the semaphore owner (sem->cs_sid) being
**	    scb->cs_self (which requires finding the SCB), the
**	    thread_id is used instead, which can be had by a quick
**	    CS_get_thread_id() call.
**  16-Feb-2009 (fanra01)
**      Divide the initialization of the requested semaphore type in order
**      that both critical sections and mutex objects can be used
**      transparently.
*/
STATUS
CSp_semaphore(i4  exclusive, register CS_SEMAPHORE *sem)
{
    STATUS status;
    if (sem->cs_type == CS_SEM_SINGLE)
    {
        status = CSp_singlesem(exclusive, sem);
    }
    else
    {
        status = CSp_multisem(exclusive, sem);
    }
    return(status);
}

/*
** Name: CSp_singlesem  - request an in-process synchronization semaphore
**
** Description:
**  This function based on the original CSp_semaphore, replaces the use of
**  mutex objects with CRITICAL_SECTION.
**
** Inputs:
**      exclusive
**      sem
**
** Outputs:
**      Cs_srv_block
**          .cs_smstatistics
**
** Returns:
**      OK                      completed successfully
**      E_CS000A_NO_SEMAPHORE   uninitialized semaphore structure
**
** History:
**      16-Feb-2009 (fanra01)
**          Created based on the original CSp_semaphore.
*/
STATUS
CSp_singlesem( i4 exclusive, register CS_SEMAPHORE *sem )
{
    CS_SCB		    *scb;
    i4			    saved_state;
    DWORD		    retval, error;
    STATUS		    status = OK;
    bool		    force = FALSE;
    i4			    i;
    CS_THREAD_ID	tid;
    LPCRITICAL_SECTION csSem;
    struct _CS_SMSTAT *srv_stats = &Cs_srv_block.cs_smstatistics;
    struct  _CS_EXT_SMSTAT *sem_stats = &sem->cs_smstatistics;

    /* Get this thread's thread_id */
    CS_thread_id_assign(tid, CS_get_thread_id());

    if (sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD)
    {
        TRdisplay("%@ CSp_semaphore called with uninitialized semaphore, thread: %d\n",
            tid);
        return(E_CS000A_NO_SEMAPHORE);
    }

    exclusive = TRUE;
    
    if (exclusive)
    {
        sem_stats->cs_smx_count++; /* exclusive requests */
        if (sem->cs_type == CS_SEM_SINGLE)
            srv_stats->cs_smsx_count++;
        else
            srv_stats->cs_smmx_count++;
    }
    else
    {
        sem_stats->cs_sms_count++; /* shared requests */
        if (sem->cs_type == CS_SEM_SINGLE)
            srv_stats->cs_smss_count++;
        else
            srv_stats->cs_smms_count++;
    }
    csSem = (LPCRITICAL_SECTION)GetSemHandle( sem, force, &error );
    if (pfnTryEnterCriticalSection(csSem))
    {
        if(exclusive)
        {
            if (sem->cs_count == 0)
            {
                sem->cs_value = 1;
                CS_thread_id_assign(sem->cs_thread_id, tid);
                sem->cs_pid = Cs_srv_block.cs_pid;
            }
            else
            {
                /* Want EXCL, but held share, we must wait */
                sem_stats->cs_smsx_count++;
                if (sem->cs_type == CS_SEM_MULTI)
                    srv_stats->cs_smmsx_count++;
                else
                    srv_stats->cs_smssx_count++;
                LeaveCriticalSection(csSem);
            }
        }
        else
        {
            if (sem->cs_count++ == 0)
            {
                CS_thread_id_assign( sem->cs_thread_id, tid );
                sem->cs_pid = Cs_srv_block.cs_pid;
            }

            if (sem->cs_count > sem_stats->cs_sms_hwm)
                sem_stats->cs_sms_hwm = sem->cs_count;
            LeaveCriticalSection(csSem);
        }
    }
    else
    {
        CSget_scb(&scb);
        scb->cs_sync_obj = (PTR)sem;
        saved_state = scb->cs_state;
        scb->cs_state = CS_MUTEX;
        EnterCriticalSection(csSem);
        while (exclusive && sem->cs_count)
        {
            sem->cs_excl_waiters++;
            LeaveCriticalSection(csSem);
            Sleep(sem->cs_count << 2);
            EnterCriticalSection(csSem);
            sem->cs_excl_waiters--;
        }
        if (sem->cs_count == 0)
        {
            sem->cs_value = 1;
            CS_thread_id_assign(sem->cs_thread_id, tid);
            sem->cs_pid = Cs_srv_block.cs_pid;
        }
    }
    return(status);
}

/*
** Name: CSp_multisem  - request a cross-process synchronization semaphore
**
** Description:
**  This function based on the original CSp_semaphore retains the use of
**  mutex objects.
**
** Inputs:
**      exclusive
**      sem
**
** Outputs:
**      Cs_srv_block
**          .cs_smstatistics
**
** Returns:
**      OK                      completed successfully
**      E_CS000A_NO_SEMAPHORE   uninitialized semaphore structure
**      E_CS0004_BAD_PARAMETER  invalid mutex handle
**      E_CS0050_CS_SEMOP_GET   unable to return a handle, depleted resources
**      E_CS0017_SMPR_DEADLOCK  semaphore already owned
**
** History:
**      16-Feb-2009 (fanra01)
**          Created based on the original CSp_semaphore.
*/
STATUS
CSp_multisem(i4  exclusive, register CS_SEMAPHORE *sem)
{
    CS_SCB		*scb;
    i4			saved_state;
    DWORD		retval, error;
    STATUS		status;
    HANDLE		hSem = NULL;
    bool		force = FALSE;
    i4			i;
    CS_THREAD_ID	tid;

    /* Get this thread's thread_id */
    CS_thread_id_assign(tid, CS_get_thread_id());

    if (sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD)
    {
	TRdisplay("%@ CSp_semaphore called with uninitialized semaphore, thread: %d\n",
		  tid);
	return(E_CS000A_NO_SEMAPHORE);
    }

    exclusive = TRUE;

    if (exclusive)
    {
	sem->cs_smstatistics.cs_smx_count++;
	if (sem->cs_type == CS_SEM_MULTI)
	    Cs_srv_block.cs_smstatistics.cs_smmx_count++;
	else
	    Cs_srv_block.cs_smstatistics.cs_smsx_count++;
    }
    else
    {   
	sem->cs_smstatistics.cs_sms_count++;
	if (sem->cs_type == CS_SEM_MULTI)
	    Cs_srv_block.cs_smstatistics.cs_smms_count++;
	else
	    Cs_srv_block.cs_smstatistics.cs_smss_count++;
    }

    hSem = GetSemHandle(sem, force, &error);
    for (i = 0; (error == ERROR_NOT_ENOUGH_MEMORY || 
		 error == ERROR_NO_SYSTEM_RESOURCES) && i < 32; i++)
    {
	CL_ERR_DESC err_code;

	if ((status = DIlru_flush(&err_code)) != OK)
	    return(E_CS0050_CS_SEMOP_GET);
	hSem = GetSemHandle(sem, force, &error);
    }

    if (hSem == NULL)
    {
	CS_breakpoint();
	return (E_CS0004_BAD_PARAMETER);
    }

    /* Test the status of the semaphore handle without waiting */
    if ((retval = WaitForSingleObject(hSem, 0)) == WAIT_FAILED &&
	GetLastError() == ERROR_INVALID_HANDLE)
    {
	CS_breakpoint();
	if (sem->cs_type == CS_SEM_MULTI)
	{
	    /* force a new handle */
	    hSem = GetSemHandle(sem, TRUE, &error);
	    if (hSem != NULL)
		retval = WaitForSingleObject(hSem, 0);
	    else
		retval = WAIT_ABANDONED;
	}
    }

    /* The semaphore handle is good now */
    if (retval == WAIT_OBJECT_0)
    {
	if (exclusive)
	{
	    if (sem->cs_count == 0)
	    {
		sem->cs_value = 1;
		CS_thread_id_assign(sem->cs_thread_id, tid);
		sem->cs_pid = Cs_srv_block.cs_pid;
		sem->cs_mutex = hSem;
		return(OK);
	    }
	    else
	    {
		/* Want EXCL, but held share, we must wait */
		sem->cs_smstatistics.cs_smsx_count++;
		if (sem->cs_type == CS_SEM_MULTI)
		    Cs_srv_block.cs_smstatistics.cs_smmsx_count++;
		else
		    Cs_srv_block.cs_smstatistics.cs_smssx_count++;
		/* HOT: we probably don't need to check return value here -- status is not checked */
		if (ReleaseMutex(hSem) != 0)
		    status = OK;
		else
		    status = E_CS0050_CS_SEMOP_GET;
	    }
	}
	else
	{
	    if (sem->cs_count++ == 0)
	    {
		CS_thread_id_assign( sem->cs_thread_id, tid );
		sem->cs_pid = Cs_srv_block.cs_pid;
		sem->cs_mutex = hSem;
	    }

	    if (sem->cs_count > sem->cs_smstatistics.cs_sms_hwm)
		sem->cs_smstatistics.cs_sms_hwm = sem->cs_count;

	    if (ReleaseMutex(hSem) != 0)
		status = OK;
	    else
		status = E_CS0050_CS_SEMOP_GET;

	    return(status);
	}
    }
    else if (retval == WAIT_FAILED || retval == WAIT_TIMEOUT)
    {
	/* Semaphore is blocked */
	if (CS_thread_id_equal(sem->cs_thread_id, tid) &&
	    sem->cs_pid == Cs_srv_block.cs_pid)
	{
	    /*
	    ** Thread apparently already owns the semaphore.
	    ** Since we're looking at the semaphore dirty,
	    ** check it a few more times to make sure
	    ** we're not looking at a semaphore in a transitive 
	    ** state which appeares to match our 
	    ** pid and tid.
	    */
	    i4	spin;

	    for (spin = 10; spin; spin--)
	    {
		if (!CS_thread_id_equal(sem->cs_thread_id, tid) ||
		    sem->cs_pid != Cs_srv_block.cs_pid)
		{
		    break;
		}
	    }

	    if (spin == 0)
	    {
		/* This thread already owns the semaphore */
		CS_breakpoint();
		TRdisplay("%@ CSp_semaphore failed for %s, session: %x line: %d\n",
			  sem->cs_sem_name, tid, __LINE__);
		return(E_CS0017_SMPR_DEADLOCK);
	    }
	}

	if (exclusive)
	{
	    sem->cs_smstatistics.cs_smxx_count++;
	    if (sem->cs_type == CS_SEM_MULTI)
		Cs_srv_block.cs_smstatistics.cs_smmxx_count++;
	    else
		Cs_srv_block.cs_smstatistics.cs_smsxx_count++;
	}
	else
	{
	    /*
	    ** Don't count shared vs shared collisions, just share
	    ** vs excl.
	    */
	    if (sem->cs_value || sem->cs_excl_waiters)
	    {
		sem->cs_smstatistics.cs_smsx_count++;
		if (sem->cs_type == CS_SEM_MULTI)
		    Cs_srv_block.cs_smstatistics.cs_smmsx_count++;
		else
		    Cs_srv_block.cs_smstatistics.cs_smssx_count++;
	    }
	}

	status = OK;
    }
    else
    {
	/* Semaphore is bogus */
	CS_breakpoint();
	TRdisplay("%@ CSp_semaphore failed for %s, session: %x line: %d\n",
		  sem->cs_sem_name, tid, __LINE__);
	return(E_CS000A_NO_SEMAPHORE);
    }

    /* 
    ** Mutex is blocked, we must wait.
    */

    /* Only now do we get a pointer to the sessions's SCB */
    CSget_scb(&scb);

    scb->cs_sync_obj = (PTR)sem;
    saved_state = scb->cs_state;
    scb->cs_state = CS_MUTEX;

    retval = WaitForSingleObject(hSem, INFINITE);

    /*
    ** Loop until the semaphore becomes logically available.
    **
    **   1. want EXCL, but sem is held SHARE
    */
    while (retval == WAIT_OBJECT_0 && exclusive && sem->cs_count)
    {
	sem->cs_excl_waiters++;

	ReleaseMutex(hSem);
	Sleep(sem->cs_count << 2);
	retval = WaitForSingleObject(hSem, INFINITE);

	sem->cs_excl_waiters--;

	if (retval == WAIT_ABANDONED)
	    retval = WAIT_OBJECT_0;
    }

    scb->cs_state = saved_state;

    if (retval == WAIT_OBJECT_0)
    {
	if (sem->cs_count == 0)
	{
	    CS_thread_id_assign(sem->cs_thread_id, tid);
	    sem->cs_pid = Cs_srv_block.cs_pid;
	    sem->cs_mutex = hSem;
	}
	if (exclusive)
	{
	    sem->cs_value = 1;
	    return(OK);
	}
	else /* shared */
	{
	    if (++sem->cs_count > sem->cs_smstatistics.cs_sms_hwm)
		sem->cs_smstatistics.cs_sms_hwm = sem->cs_count;

	    if (ReleaseMutex(hSem) != 0)
		status = OK;
	    else
		status = E_CS0050_CS_SEMOP_GET;

	    return (status);
	}
    }
    else
    {
	error = GetLastError();
	CS_breakpoint();
	TRdisplay("%@ CSp_semaphore failed for %s, session: %x line: %d\n",
		  sem->cs_sem_name, tid, __LINE__);
	ReleaseMutex(hSem);
	return(E_CS0017_SMPR_DEADLOCK);
    }
}

/*
**  Name: CSv_semaphore  - Perform a "V" operation, releasing the semaphore
**
**  Description:
**	This routine is used by a running thread to release a semaphore which
**	it currently owns.  The act of doing this decrements the thread's
**	owned semaphore count.  It will allow the threads awaiting this
**	semaphore to try once again.
**
**  Inputs:
**	sem	A pointer to the semaphore in question.
**
**  Outputs:
**	none
**
**	Returns:
**	    OK
**	    E_CS000A_NO_SEMAPHORE	- If the task does not own a semaphore.
**
**	Exceptions:
**	    none
**
**  Side Effects:
**	none
**
**  History:
**	10-jan-2000 (somsa01)
**	    Set CS_MUTEX when waiting for the semaphore.
**	24-jul-2000 (somsa01 for jenjo02)
**	    Removed dubious cs_sem_init_pid, cs_sem_init_addr from
**	    CS_SEMAPHORE, replaced cs_sid with cs_thread_id.
**	23-jan-2004 (somsa01 for jenjo02)
**	    Avoid CSget_scb() calls unless we're blocked and have to
**	    change scb->cs_state. Use thread_id to identify sem owner
**	    instead of scb->cs_self.
**  16-Feb-2009 (fanra01)
**      Divide the release of the requested semaphore type so that
**      both critical sections and mutex objects are used
**      transparently.
*/
STATUS
CSv_semaphore( register CS_SEMAPHORE *sem )
{
    STATUS status;
    if (sem->cs_type == CS_SEM_SINGLE)
    {
        status = CSv_singlesem( sem );
    }
    else
    {
        status = CSv_multisem( sem );
    }
    return(status);
}
/*
** Name: CSv_singlesem  - release an in-process synchronization semaphore
**
** Description:
**  This function based on the original CSv_semaphore, replaces the use of
**  mutex objects with CRITICAL_SECTION.
**
** Inputs:
**      sem
**
** Outputs:
**      None
**
** Returns:
**      OK                      completed successfully
**      E_CS000A_NO_SEMAPHORE   uninitialized/invalid semaphore structure
**
** History:
**      16-Feb-2009 (fanra01)
**          Created based on the original CSp_semaphore.
*/STATUS
CSv_singlesem( register CS_SEMAPHORE *sem )
{
    bool		force = FALSE;
    DWORD		retval, error;
    CS_THREAD_ID	tid;
    HANDLE		hSem;

    /* Get this thread's thread_id */
    CS_thread_id_assign(tid, CS_get_thread_id());

    if (sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD)
    {
        TRdisplay("%@ CSv_semaphore called with uninitialized semaphore, thread: %d\n",
		  tid);
        return(E_CS000A_NO_SEMAPHORE);
    }

    if (sem->cs_value)
    {
        if (CS_thread_id_equal(sem->cs_thread_id, tid) &&
            sem->cs_pid == Cs_srv_block.cs_pid)
        {
            sem->cs_value = 0;
            CS_thread_id_assign(sem->cs_thread_id, 0);
            sem->cs_pid = 0;
        } 
        else
        {
            CS_breakpoint();
            return(E_CS000A_NO_SEMAPHORE);
        } 
    }
    else
    {
        i4	saved_state;
        CS_SCB	*scb;

        /* Now get the session's SCB */
        CSget_scb(&scb);

        saved_state = scb->cs_state;
        scb->cs_sync_obj = (PTR)sem;
        scb->cs_state = CS_MUTEX;
        EnterCriticalSection(sem->cs_mutex);
        scb->cs_state = saved_state;

        if (sem->cs_count-- == 0)
        {
            CS_breakpoint();
            sem->cs_count = 0;
            scb->cs_sem_count++;
            LeaveCriticalSection(sem->cs_mutex);
            return(E_CS000A_NO_SEMAPHORE);
        }

        if (CS_thread_id_equal( sem->cs_thread_id, tid ) &&
            sem->cs_pid == Cs_srv_block.cs_pid)
        {
            CS_thread_id_assign( sem->cs_thread_id, 0 );
            sem->cs_pid = 0;
        } 
    }
    LeaveCriticalSection(sem->cs_mutex);
    return(OK);
}
/*
** Name: CSv_multisem  - release a cross-process synchronization semaphore
**
** Description:
**  This function based on the original CSv_semaphore retains the use of
**  mutex objects.
**
** Inputs:
**      sem
**
** Outputs:
**      None
**
** Returns:
**      OK                      completed successfully
**      E_CS000A_NO_SEMAPHORE   uninitialized/invalid semaphore structure
**
** History:
**      16-Feb-2009 (fanra01)
**          Created based on the original CSv_semaphore.
*/
STATUS
CSv_multisem(register CS_SEMAPHORE *sem)
{
    HANDLE		hSem;
    bool		force = FALSE;
    DWORD		retval, error;
    CS_THREAD_ID	tid;

    /* Get this thread's thread_id */
    CS_thread_id_assign(tid, CS_get_thread_id());

    if (sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD)
    {
	TRdisplay("%@ CSp_semaphore called with uninitialized semaphore, thread: %d\n",
		  tid);
	return(E_CS000A_NO_SEMAPHORE);
    }

    if (sem->cs_value)
    {
	/*
	** Held exclusively. Make sure this thread holds the
	** semaphore.
	*/

        hSem = sem->cs_mutex;

	if (CS_thread_id_equal(sem->cs_thread_id, tid) &&
	    sem->cs_pid == Cs_srv_block.cs_pid)
	{
	    sem->cs_value = 0;
	    CS_thread_id_assign(sem->cs_thread_id, 0);
	    sem->cs_pid = 0;
	} 
	else
	{
	    CS_breakpoint();
	    return(E_CS000A_NO_SEMAPHORE);
	} 
    }
    else /* shared */
    {
	i4	saved_state;
	CS_SCB	*scb;

	/* Now get the session's SCB */
	CSget_scb(&scb);

	saved_state = scb->cs_state;
	scb->cs_sync_obj = (PTR)sem;
	scb->cs_state = CS_MUTEX;

        hSem = GetSemHandle(sem, force, &error);

	if ((retval = WaitForSingleObject(hSem, INFINITE)) != WAIT_OBJECT_0)
	{
	    CS_breakpoint();
	    return(E_CS000A_NO_SEMAPHORE);
	}

	scb->cs_state = saved_state;

	if (sem->cs_count-- == 0)
	{
	    CS_breakpoint();
	    sem->cs_count = 0;
	    scb->cs_sem_count++;
	    ReleaseMutex(hSem);
	    error = GetLastError();
	    if (error != ERROR_SUCCESS)
		CS_breakpoint();
	    return(E_CS000A_NO_SEMAPHORE);
	}

	if (CS_thread_id_equal( sem->cs_thread_id, tid ) &&
	    sem->cs_pid == Cs_srv_block.cs_pid)
	{
	    CS_thread_id_assign( sem->cs_thread_id, 0 );
	    sem->cs_pid = 0;
	} 

    }
     
    if (ReleaseMutex(hSem) != 0)
        return(OK);
    else
    {
	CS_breakpoint();
        return(E_CS000A_NO_SEMAPHORE);
    }
}

/******************************************************************************
**
** Name: CSr_semaphore  - Remove a semaphore (opposite of CSi_semaphore)
**
** Description:
**      This routine is used by a running thread to return any resources from
**      a semaphore that is no longer needed.  This also prevents other
**  threads from doing a "P" operation until the semaphore is re-inited.
**
** Inputs:
**      sem   A pointer to the semaphore in question.
**
** Outputs:
**      none
**  Returns:
**      OK
**      E_CS000A_NO_SEMAPHORE    if the semaphore is invalid.
**      E_CS0003_NOT_QUIESCENT   if the semaphore is currently held
**      E_CS0004_BAD_PARAMETER   on other errors
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**      11-Dec-96 (fanra01)
**          Add a CloseHandle for the cs_cond event.
**	11-apr-1997 (canor01)
**	    Make sure to close the correct handles for the semaphore
**  16-Feb-2009 (fanra01)
**      Replace removal of mutex handle for CS_SEM_SINGLE with a
**      DeleteCriticalSection.
**
******************************************************************************/
STATUS
CSr_semaphore(CS_SEMAPHORE *sem)
{
	HANDLE 	hSem = 0;
	STATUS	status = OK;
	i4  	i;
	DWORD	error;

	if (!sem)
	    return E_CS0004_BAD_PARAMETER;

	/* if multi-process, clean up the structures and linked list */
	if (sem->cs_type == CS_SEM_MULTI) 
	{
	    /* CS_SEM_MULTI */

            /* Get the handles valid for this process */
            hSem = GetSemHandle(sem, FALSE, &error);
    
            /* if the handle has been created, close it */
            if (hSem)
            {
                CloseHandle(hSem);
            }

	    WaitForSingleObject( CS_init_sem, INFINITE );

	    for ( i = 0; i < CS_MULTI_LST_COUNT; i++ )
	    {
		if ( sem->cs_multi_lst[i].pid == Cs_srv_block.cs_pid )
		{
		    sem->cs_multi_lst[i].hSem = NULL;
		    sem->cs_multi_lst[i].pid  = 0;
		    sem->cs_owner = NULL;
		    sem->cs_value = 0;
		    sem->cs_mutex = 0;
		    sem->cs_sem_scribble_check = CS_SEM_WAS_REMOVED;
		    ReleaseMutex( CS_init_sem );
		    CSd_semaphore(sem);
		    return(OK);
		}
	    }
	    
	    /* Not found in the shared memory list, remove from linked list */
	    sem->cs_owner = NULL;
	    sem->cs_value = 0;
	    sem->cs_sem_scribble_check = CS_SEM_WAS_REMOVED;
	    ReleaseMutex( CS_init_sem );

	    status = RemoveMultiSem(sem);

	    CSd_semaphore(sem);
	}
	else
	{
	    /* CS_SEM_SINGLE */
	    if (!sem->cs_mutex)
		return ( OK );

	    sem->cs_count = 0;
        DeleteCriticalSection(sem->cs_mutex);
	}

	sem->cs_mutex = 0;
	sem->cs_sem_scribble_check = CS_SEM_WAS_REMOVED;

	return ( status );
}

/******************************************************************************
** Name:	CSa_semaphore	\- attach to a CS_SEM_MULTI semaphore
**
** Description:
**	Allows the process to use a CS_SEM_MULTI semaphore that was not created
**	by the current process.  Silent no-op on CS_SEM_SINGLE semaphores.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		the sem to attach.
**
** Outputs:
**	sem		may be written.
**
** Returns:
**	OK		or reportable error.
**
** Side Effects:
**	May make the semaphore known to management facilities.
**
******************************************************************************/
STATUS
CSa_semaphore(CS_SEMAPHORE *sem )
{
    STATUS clstat = OK;

    return( clstat );
}

/******************************************************************************
** Name:	CSd_semaphore - detach semaphore.
**
** Description:
**	Detach from use of the specified CS_SEM_MULTI semphore, leaving
**	it for other processes to use.  Save no-op on CS_SEM_SINGLE sem.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		semaphore to detach.
**
** Outputs:
**	sem		will probably be written.
**
** Returns:
**	OK		or reportable error.
**
******************************************************************************/
STATUS
CSd_semaphore(CS_SEMAPHORE *sem )
{
    STATUS clstat = OK;

    if (sem->cs_type == CS_SEM_MULTI)
    {
#ifdef USE_MO_WITH_SEMS
        clstat = CS_detach_sem(sem);
#endif /* USE_MO_WITH_SEMS */
    }

    return( clstat );
}

/*
** Name:	CSn_semaphore - give a name to a semaphore
**
** Description:
**	Give the semaphore a name, up to CS_SEM_NAME_LEN characters.
**	If the name is too long, it will be silently truncated to fit.
**	The name will be copied, so the input may vanish after the call
**	returns.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		the semaphore to name.
**	name		the name to give it.
**
** Outputs:
**	sem		probably written.
**
** Returns:
**	none.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented, created for VMS.
**	28-mar-2002 (somsa01)
**	    Tack on the Installation identifier to keep it unique.
**      30-dec-2003 (wanfr01)
**          Bug 111527, INGSRV2648
**          Changed to store only Version in the global, not the 
**	    entire semaphore name.  
**  03-Mar-2007 (wonca01) BUG 117408
**      Add conditional to disable short semaphore name by 
**      not appending the string paramater to it.  This will to 
**      eliminate a performance issue with perfmon.exe.
**  02-Oct-2008 (whiro01)
**	Remove previous change during cross-integration of 491940
**	which fixes the problem a better way.
*/

VOID
CSn_semaphore(CS_SEMAPHORE *sem, char *string)
{
    if (VersionString[0] == '\0')
    {
	char *inst_id;
	GVshobj(&ObjectPrefix);
	NMgtAt("II_INSTALLATION", &inst_id);
	STprintf(VersionString, "%s%s", ObjectPrefix, inst_id);
    }
    /* Length-limit this name to max 47 + EOS characters */
    STlpolycat( 3, sizeof(sem_name)-1, VersionString, " ", string,
        sem_name );
    STmove(sem_name, EOS, sizeof(sem->cs_sem_name), sem->cs_sem_name);
}


/*{
** Name: CSw_semaphore  - Initialize and Name Semaphore
**                        in one fell swoop.
**
** Description:
**      This routine is used to initialize a semaphore for use by the
**      CSp_semaphore and CSv_semaphore routines.  This should always
**      be called before the first use of a semaphore and needs only
**      be called once.  It is illegal to call CSw_semaphore on a
**      semaphore that is currently in use.
**
**      Give the semaphore a name, up to CS_SEM_NAME_LEN characters.
**      If the name is too long, it will be silently truncated to fit.
**      The name will be copied, so the input may vanish after the call
**      returns.
**
** Inputs:
**      sem                             A pointer to the semaphore to initialize
**      type                            Semaphore type:
**                                          CS_SEM_SINGLE - normal semaphore
**                                          CS_SEM_MULTI  - semaphore may be
**                                              requested by threads in
**                                              different processes.
**      name                            the name to give it
**
** Outputs:
**      none
**      Returns:
**          OK
**          E_CS0004_BAD_PARAMETER          Illegal semaphore type specified.
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      22-May-1995 (emmag)
**	   Created.
**	29-mar-2002 (somsa01)
**	   Tack on the Installation identifier to keep it unique. 
**      30-dec-2003 (wanfr01)
**          Bug 111527, INGSRV2648
**          Changed to store only Version in the global, not the
**          entire semaphore name.
**  03-Mar-2007 (wonca01) BUG 117408
**      Add conditional to disable short semaphore name by 
**      not appending the string paramater to it.  This will to 
**      eliminate a performance issue with perfmon.exe.
**  02-Oct-2008 (whiro01)
**	Remove previous change during cross-integration of 491940
**	which fixes the problem a better way.
*/
STATUS
CSw_semaphore(CS_SEMAPHORE *sem, i4  type, char *string)
{
    STATUS ret;

    /* Additional error check that isn't in CSi_semaphore() */
    if ( (type == CS_SEM_MULTI) && 
         ( (string == NULL) ||
	   (*string == EOS) ) )
        return (E_CS0004_BAD_PARAMETER);

    /* Initialize the semaphore */
    if ((ret = CSi_semaphore(sem, type)) != OK)
	return ret;

    /* Now name the semaphore (just as above) */
    CSn_semaphore(sem, string);

    return( OK );
}

/*{
** Name:        CSs_semaphore - return stats for a semaphore.
**
** Description:
**      Return semaphore statistics.
**
** Re-entrancy:
**      yes.
**
** Inputs:
**      options         CS_INIT_SEM_STATS to initialize return area
**                      CS_ROLL_SEM_STATS to accumulate in return area
**                      CS_CLEAR_SEM_STATS to additionally clear the
**                      accumulators from the semaphore.
**      sem             the semaphore for which stats are wanted.
**      length          the size of the output area.
**
** Outputs:
**      stats           filled in with the appropriate stats
**                      unless output area is too small or
**                      semaphore doesn't look like a semaphore.
**
** Returns:
**      nothing
**
** History:
**      07-Dec-1995 (jenjo02)
**          created
**	24-jul-2000 (somsa01 for jenjo02)
**	    Removed dubious cs_sem_init_pid, cs_sem_init_addr from
**	    CS_SEMAPHORE, replaced cs_sid with cs_thread_id.
*/
STATUS                                                                       
IICSs_semaphore(
    i4           options,
    CS_SEMAPHORE *sem,
    CS_SEM_STATS *stats,
    i4           length )
{
    if (length >= sizeof(CS_SEM_STATS))                                      
    {
        if (sem->cs_sem_scribble_check == CS_SEM_LOOKS_GOOD)
        {
            if (options & CS_ROLL_SEM_STATS && *stats->name != EOS)
            {
                stats->excl_requests    += sem->cs_smstatistics.cs_smx_count;
                stats->share_requests   += sem->cs_smstatistics.cs_sms_count;
                stats->excl_collisions  += sem->cs_smstatistics.cs_smxx_count;
                stats->share_collisions += sem->cs_smstatistics.cs_smsx_count;
            }
            else
            {
                STcopy( sem->cs_sem_name, stats->name);
                stats->type = sem->cs_type;
                stats->excl_requests    = sem->cs_smstatistics.cs_smx_count;
                stats->share_requests   = sem->cs_smstatistics.cs_sms_count;
                stats->excl_collisions  = sem->cs_smstatistics.cs_smxx_count;
                stats->share_collisions = sem->cs_smstatistics.cs_smsx_count;
            }
            if (options & CS_CLEAR_SEM_STATS)
            {
                MEfill(sizeof(sem->cs_smstatistics), 0,
                        (char *)&sem->cs_smstatistics);
            }
            return(OK);
        }
        else if ((options & CS_ROLL_SEM_STATS) == 0)
        {
            MEfill(sizeof(*stats), 0, (char *)stats);
        }

    }
    return(E_CS0004_BAD_PARAMETER);
}


/*{
** Name: CS_remove_all_sems - delete known semaphores at shutdown
**
** Description:
**      Walks the known semaphore list and removes semaphores from the
**      system.  This is necessary for operating systems that retain
**      a portion of the underlying synchronization object (mutex or
**      semaphore) in the kernel that is not automatically released
**      by releasing shared memory or exiting all processes using the
**      object.
**
**      NT will release mutex objects when all processes using them
**	exit, so this can be a noop for now.
**
** Inputs:
**      type            CS_SEM_MULTI - to release cross-process sems
**                      CS_SEM_SINGLE - to release private sems
**
** Outputs:
**      none.
**
** Returns:
**      OK              - operation succeeded
**      other system-specific return
**
** History:
**      08-jul-1997 (canor01)
**          Created.
*/
STATUS
CSremove_all_sems( i4  type )
{
# if 0 /* noop for now */

    CS_SEMAPHORE *sem;
    QUEUE        *semlist;

    if ( type == CS_SEM_MULTI )
        semlist = &Cs_srv_block.cs_multi_sem;
    else
        semlist = &Cs_srv_block.cs_single_sem;
 
    CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
 
    for ( sem = (CS_SEMAPHORE *) semlist->q_next;
          sem != (CS_SEMAPHORE *) semlist;
          sem = (CS_SEMAPHORE *) sem->cs_sem_list.q_next )
    {
        sem->cs_sem_scribble_check = CS_SEM_WAS_REMOVED;
 
        CS_synch_destroy( &sem->cs_mutex );
    }
 
    CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );

# endif /* noop for now */
 
    return( OK );
}
/*
** Name: CS_cp_sem_cleanup()
**
** Description:
**      Walks the known semaphore list and removes semaphores from the
**      system.  This is necessary for operating systems that retain
**      a portion of the underlying synchronization object (mutex or
**      semaphore) in the kernel that is not automatically released
**      by releasing shared memory or exiting all processes using the
**      object.
**  This routine differs from CSremove_all_sems() in that this is called
**  during shared memory deletion, while CSremove_all_sems() is called
**  during dmfrcp shutdown.
**
** Inputs:
**      key                             shared memory key of area to cleanup
**
** Outputs:
**      none.
**
**      Returns:
**
** History:
**  23-Feb-1998 (bonro01)
**      Created.
**	10-Nov-2010 (kschendel) SIR 124685
**	    Change dummy to be void.
*/
void
CS_cp_sem_cleanup(
    char            *key,
    CL_ERR_DESC     *err_code)
{

    CL_CLEAR_ERR(err_code);

    /* Not used on NT */
}
