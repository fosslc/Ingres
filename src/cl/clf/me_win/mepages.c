/*
** Copyright (c) 1988, 2004 Ingres Corporation
**
*/

#include <compat.h>
#include <clconfig.h>
#include <me.h>
#include <meprivate.h>
#include <errno.h>
#include <cs.h>
#include <er.h>
#include <lo.h>
#include <mo.h>

#include <winnt.h>
#include <winbase.h>

/******************************************************************************
**
**  Name: MEPAGES.C - A Page based memory allocator
**
**  This file defines:
**      MEget_pages()   - Allocate shared or unshared memory pages.
**      MEfree_pages()  - return allocated pages to the free page pool.
**      MEprot_pages()  - Set protection attributes on a page range.
**      MEsmdestroy()   - Destroy an existing shared memory segment.
**      internal routines:
**      MEfindpages()   - find free pages
**      MEalloctst()    - check a region for allocation status
**  History:
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      15-Apr-96 (fanra01)
**          Added setup of cl error in ME_alloc_brk.
**	17-apr-1996 (canor01)
**	    Replace SETCLOS2ERR with SETWIN32ERR macro.
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	07-dec-2000 (somsa01)
**	    Use MEmax_alloc instead of ME_MAX_ALLOC. Also, in ME_init(),
**	    halve MEmax_alloc until VirtualAlloc() succeeds, but let it
**	    not exceed 1 << 15.
**	06-aug-2001 (somsa01)
**	    Fixed up 64-bit compiler warnings.
**	12-sep-2001 (somsa01 for fanra01)
**	    In MEget_pages(), added missing break statement in switch
**	    block for status.
**	07-jun-2002 (somsa01)
**	    Synch'ed up with UNIX.
**	22-oct-2002 (somsa01)
**	    Changes to allow 64-bit memory allocation.
**      11-Nov-2004 (horda03) Bug 111658/INGSRV2680
**          CHanged prototype for MEfindpages.
**	22-jul-2004 (somsa01)
**	    Removed unnecessary include of erglf.h.
**	30-Nov-2004 (drivi01)
**	  Updated i4 to SIZE_TYPE to port change #473049 to windows.
**	05-Aug-2005 (drivi01)
**	  Modified ME_init, MEget_pages, and MEfree_pages functions
**    to use calloc instead of VirtualAlloc and allocate
**    memory as needed instead of reserving a big chunk in
**    advance and then commiting parts of it as needed.
**      23-Aug-2005 (drivi01)
**          Although we don't allocate 1 Gig private cache upfront anymore
**          instead we allocate memory chunks as needed, VirtualAlloc
**          appears to be a better function for allocation of those smaller
**          chunks than calloc. 
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**	04-Apr-2010 (drivi01)
**	    MEget_pages and ME in general has inconsistently replaced i4
**	    with SIZE_TYPE, SIZE_TYPE is 8 bytes on 64-bit OS which 
**	    inconsistently assigns datatypes of SIZE_TYPE to DWORD or i4.
**	    PCexec_suid also makes call to MEget_pages where allocated_pages
**	    is being passed in as i4.
**	    Fix allocated_pages to be declared as SIZE_TYPE to stay consistent
**	    with the definitions within ME. 
**          Clean up warnings by removing unused variables.
**
******************************************************************************/

/******************************************************************************
**
**  Forward and/or External typedef/struct references.
**
******************************************************************************/

/******************************************************************************
**
** Definition of all global variables used by this file.
**
******************************************************************************/

GLOBALREF QUEUE			ME_segpool;
GLOBALREF CS_SEMAPHORE	*ME_page_sem;
GLOBALREF HANDLE ME_init_mutex;
/******************************************************************************
**
** Definition of all global variables owned by this file.
**
******************************************************************************/
GLOBALREF u_char *MEalloctab;
GLOBALREF char   *MEbase;
GLOBALREF i4     MEmax_alloc;

/******************************************************************************
**
**  Definition of static variables and forward static functions.
**
******************************************************************************/

/* call counters for MO */

static SIZE_TYPE	ME_getcount	ZERO_FILL;
static SIZE_TYPE	ME_freecount	ZERO_FILL;
static SIZE_TYPE	ME_allocd_pages ZERO_FILL;

/* 
**  Debug stuff
**  
**  In MEget_pages, if MEtestcount > 0, and MEtestcount < MEgetcount,
**  then return ME_OUT_OF_MEMORY whe !(MEgetcount % MEtestmod).
**
**  These fields are settable by MO.
*/

static	i4	ME_testcount	ZERO_FILL;
static	i4	ME_testmod	ZERO_FILL;
static	i4	ME_testerrs	ZERO_FILL;

static MO_GET_METHOD MO_get_used_bytes;

FACILITYDEF MO_CLASS_DEF ME_page_classes[] =
{
  { 0, "exp.clf.nt.me.num.get_pages",
	sizeof(ME_getcount), MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&ME_getcount, MOcdata_index },

  { 0, "exp.clf.nt.me.num.free_pages",
	sizeof(ME_freecount), MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&ME_freecount, MOcdata_index },

  { 0, "exp.clf.nt.me.num.test_count",
	sizeof(ME_testcount),
	MO_READ|MO_SERVER_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&ME_testcount, MOcdata_index },

  { 0, "exp.clf.nt.me.num.test_errs",
	sizeof(ME_testcount),
	MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&ME_testerrs, MOcdata_index },

  { 0, "exp.clf.nt.me.num.test_mod",
	sizeof(ME_testmod),
	MO_READ|MO_SERVER_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&ME_testmod, MOcdata_index },

  { 0, "exp.clf.nt.me.num.pages_used",
	sizeof(ME_allocd_pages), MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&ME_allocd_pages, MOcdata_index },

  { 0, "exp.clf.nt.me.num.bytes_used",
    	0, MO_READ, 0, 0, MO_get_used_bytes, MOnoset, 0, MOcdata_index },

  { 0 }
};

# if defined(ME_DEBUG)
/*
** List of allocated blocks
** This exists for debugging purposes only
*/
typedef struct _ME_ALLOC_BLK {
    struct _ME_ALLOC_BLK *next;
    PTR addr;
    i4  npages;
} ME_ALLOC_BLK;

static ME_ALLOC_BLK *MEalloc_list = NULL;
static CRITICAL_SECTION MEalloc_csec;

STATUS MEverify_conttab();
# endif /* ME_DEBUG */


/******************************************************************************
** Name: ME_init()  - Initialize the ME system
**
** Description:
**  Initialize pointers and tables to manage all memory in the
**  process.
**
** Outputs:
**      err_code                        system dependent error code.
**
**  Returns:
**      OK
**      ME_OUT_OF_MEM
**
**  Exceptions:
**      none
**
**      20-jul-95 (reijo01)
**          Fix SETCLOS32ERR macro
**	07-dec-2000 (somsa01)
**	    Halve MEmax_alloc until VirtualAlloc() succeeds, but let it
**	    not exceed 1 << 15.
**	07-jun-2002 (somsa01)
**	    Synch'ed up with UNIX.
******************************************************************************/
static          STATUS
ME_init(CL_ERR_DESC *err_code)
{
    DWORD	rval = 0;
    i4		allocsize;
    char	*locator;
    bool	KeepGoing = TRUE;

    QUinit(&ME_segpool);

    allocsize = MEmax_alloc / ME_BITS_PER_BYTE;

	 /*
     ** Now, allocate a bitmap with 1 bit per page.
     */
	locator = VirtualAlloc((MEalloctab ? MEalloctab : NULL),
				allocsize,
				MEM_COMMIT, 
				PAGE_READWRITE);
	if (locator == NULL)
	{
	    rval = GetLastError();
	    SETWIN32ERR(err_code, rval, ER_alloc);
	    rval = ME_OUT_OF_MEM;
	}

    MEalloctab = locator;
    if (rval == 0)
	ME_allocd_pages += (allocsize + ME_MPAGESIZE -1) / ME_MPAGESIZE;

# if defined(ME_DEBUG)
    InitializeCriticalSection(&MEalloc_csec);
    rval = MEverify_conttab();
# endif /* ME_DEBUG */

    return rval;
}

/******************************************************************************
** Name: MEget_pages  - add (possibly shared) memory to the client
**
** Description:
**
**      This routine adds pages to the virtual address space of the requesting
**  program. This routine is intended to be used by the dbms server for all
**  dynamic memory allocation.  It adds pages which are ME_MPAGESIZE bytes
**  in length, and are guaranteed to be aligned on ME_MPAGESIZE byte
**  boundaries.
**
**  Implementor's of the ME module can assume that MEget_pages will be
**  the exclusive allocator of memory when MEadvise(ME_INGRES_ALLOC).
**
** This routine allocates some number of pages.  The memory
** allocated here is considered to be long-term memory (on the order
** of session or server length of time).  All memory returned by this
** service is page (ME_MPAGESIZE bytes) aligned, and the unit of measuring
** of this memory is pages (i.e. requests come in for number of pages,
** not number of bytes).  Note that this page size must be variable
**  from environment to environment.  This is necessary to adjust for
**  environmental performance differences in paging activity.
**
**  When allocating shared memory, even more restrictive conditions
**  may apply, e.g.
**    1) the allocation size being greater
**    2) the alignment boundaries are further apart
**    3) only specific regions of the address space can support shared
**       segments.
**  The various restrictions on shared memory are derived from a machine's
**  hardware architecture and the operating system's allocation primitives.
**  Because these restrictions are so varied, MEget_pages is not allowed
**  during MEadvise(ME_USER_ALLOC).  See 'OUTSTANDING ISSUES' above.
**
**  Attaching to a shared segment is performed by specifying either
**  ME_MSHARED_MASK or ME_SSHARED_MASK in the flags argument and by not
**  specifying ME_CREATE_MASK.  When attaching to an already existing
**  shared segment the user supplied 'pages' argument is ignored, the
**  user recieves all the pages that belong to the shared segment. This
**  number is returned in the 'allocated_pages' parameter.
**
**  When creating or attaching to shared memory segments, a key parameter
**  must be specified which uniquely identifies this shared segment within
**  the current Ingres Installation.  The key parameter is a character
**  string that must follow the Ingres file naming conventions:
**  "prefix.suffix" where prefix is at most 8 characters and suffix is
**  at most 3.  All characters must be printable.  ME may use this key
**  to create a file to identify the shared memory segment, although this
**  is up to the particular implementation of ME.
**
** Inputs:
**      flag      flag specifying special operations on
**                allocated memory. (advisorys) Can be or'ed together.
**          ME_NO_MASK        None of the other flags apply.
**          ME_MZERO_MASK     zero the memory before returning
**          ME_MERASE_MASK    "erase" the memory (DoD)
**                            ...on systems where applicable
**          ME_IO_MASK        Memory used for I/O
**          ME_SPARSE_MASK    Memory used `sparsely' so should
**                            be paged in small chunks if applicable
**          ME_LOCKED_MASK    Memory should be locked down to inhibit
**                            paging -- i.e. buffer caches.
**          ME_SSHARED_MASK   Memory shared across processes on a
**                            single machine
**          ME_MSHARED_MASK   Memory shared with other processors.
**          ME_CREATE_MASK    Create a new shared memory segment.
**                            (Usually specified with ME_MZERO_MASK)
**                            It is an error if the specified segment
**                            already exists.
**          ME_NOTPERM_MASK   Create temporary shared memory segment
**                            rather than a permanent one (on systems
**                            where this is supported).
**          ME_ADDR_SPEC      Map the shared memory to the address
**                            specified by 'memory'.
**
**          The flags ME_SSHARED_MASK, ME_MSHARED_MASK cannot both be
**          specified in an MEget_pages call.  One of these flags must
**          be specified in order for ME_CREATE_MASK or ME_NOTPERM_MASK
**          flags to be used.
**
**      pages           number of pages requested
**                  note:
**                      if the memory is a shared segment
**                      that has already been created,
**                      flag value of ME_MSHARED_MASK|
**                      ME_SSHARED_MASK&~ME_CREATE_MASK,
**                      the pages argument is ignored.
**
**      key         A character string identifier for the
**                  a shared memory segment.  This argument
**                  is only requried with shared memory
**                  operations - when not needed, use NULL.
**
**                  The key identifier should be a character
**                  string that follows the Ingres file
**                  naming conventions: 8 characters max
**                  prefix name and 3 characters max suffix.
** Outputs:
**
**  memory      Upon success the address of the
**                  allocated memory.
**  allocated_pages         Number pages of shared memory returned.
**                  Normally, this will always match the
**                  'pages' input parameter.  If attaching
**                  to an existing shared memory segment,
**                  this will give the actual size of the
**                  attached segment.
**  err_code            System specific error information.
**
**  Returns:
**      OK                  If all went well.
**      ME_OUT_OF_MEM       Could not allocate required amount of
**                          memory.
**      ME_BAD_PARAM        Illegal parameter passed to routine
**                          (pages <= 0, ...)
**      ME_ILLEGAL_USAGE    Combination of attributes is invalid
**      ME_BAD_ADVISE       call was made during ME_USER_ALLOC.
**      ME_ALREADY_EXISTS   shared memory identifier already exists
**      ME_NO_SUCH_SEGMENT  specified shared memory segment does
**                          not exist
**      ME_NO_SHARED        system can not support shared memory
**
**  Exceptions:
**      none
**
** Side Effects:
**  File may be created in ME_SHMEM_DIR directory to identify shared segment
**
** Implementation Notes:
**
**  Shared Memory Segment id's on UNIX are derived using the ftok()
**  function utilizing a file in the Installation's memory directory.
**
**  Each time a shared segment is created, a file will be created in
**  a subdirectory of II_CONFIG (II_CONFIG/memory) that identifies that
**  shared segment.  The filename will be the user supplied key name.
**
**  These files are also used by MEshow_pages to list existing shared
**  segments.
**
**  While implementing ME routines, note that the existence of such a file
**  does not guarantee the existence of a corresponding shared segment
**  (for example, following a system crash), but the reverse is guaranteed
**  to be true.
**
**  The ME_NOTPERM_MASK flag is not implemented on UNIX.  Clients of
**  shared memory must implement other ways to clean up segments left
**  around unintentionally.
**
** History:
**	21-feb-1996 (canor01)
**	    Add support for ME_LOCKED_MASK for Windows NT, though the
**	    current limitation will only lock a small portion in
**	    memory. Perhaps this limitation will be lifted (or at
**	    least fully explained) in a later release of MSVC or NT.
**	12-sep-2001 (somsa01 for fanra01)
**	    Added missing break statement in switch block for status.
**	07-jun-2002 (somsa01)
**	    Synch'ed up with UNIX.
**
******************************************************************************/
STATUS
MEget_pages(i4          flag,
            SIZE_TYPE          pages,
            char        *key,
            PTR         *memory,
            SIZE_TYPE          *allocated_pages,
            CL_ERR_DESC *err_code)
{
    DWORD	status = 0;
    static bool	MO_reg_needed = TRUE;

    if (ME_init_mutex == NULL)
	ME_init_mutex = CreateMutex(NULL, FALSE, NULL);

    if (MEbase == NULL)
    {
	status = WaitForSingleObject(ME_init_mutex, -1);
	switch (status)
	{
	    case 0:
		if (MEbase == NULL)
		    status = ME_init(err_code);
		break;
	    default:
		status = GetLastError();
		SETWIN32ERR(err_code, status, ER_alloc);
	}
	ReleaseMutex(ME_init_mutex);
    
	if (status)
	    return (status);
    }
    else if (MO_reg_needed && MEgotadvice)
    {
	MO_reg_needed = FALSE;
	ME_mo_reg();
    }

    if (flag & (ME_SSHARED_MASK | ME_MSHARED_MASK))
    {
	status = ME_alloc_shared(flag,
	                         pages,
	                         key,
	                         memory,
	                         allocated_pages,
	                         err_code);
    }
    else
    {
	status = ME_alloc_brk(flag,
	                      pages,
	                      memory,
	                      allocated_pages,
	                      err_code);
    }
	   
    if (status == OK)
    {
	ME_allocd_pages += pages;
	if ( flag & ME_LOCKED_MASK )
	{
	    /*
	    ** According to the MSVC 2.0 doc, "The limit on the number 
	    ** of pages that can be locked is 30 pages. The limit is 
	    ** intentionally small to avoid severe performance 
	    ** degradation."
	    ** However, experience has shown the limit to be related
	    ** in some way to the amount of system memory.
	    ** Size of native page can be retrieved with GetSystemInfo.
	    */

	    SYSTEM_INFO si;
	    SIZE_TYPE maxlockbytes;
	    SIZE_TYPE lockbytes;
		
	    GetSystemInfo(&si);
	    maxlockbytes = 30 * si.dwPageSize;
	    lockbytes = pages * ME_MPAGESIZE;
	    if (lockbytes > maxlockbytes)
	        lockbytes = maxlockbytes;
	    if (VirtualLock( *memory, lockbytes ) == FALSE)
	    {
		status = GetLastError();
		SETWIN32ERR(err_code, status, ER_alloc);
		status = ME_LOCK_FAIL;
	    }
	}
    }

# if defined(ME_DEBUG)
    /*
    ** Update the list of allocated blocks
    */
    EnterCriticalSection(&MEalloc_csec);
    {
	STATUS stat;
	ME_ALLOC_BLK *blk =
		(ME_ALLOC_BLK *)MEreqmem(0, sizeof(ME_ALLOC_BLK), TRUE, &stat);
	ME_ALLOC_BLK *que = MEalloc_list;

	blk->addr = *memory;
	blk->npages = *allocated_pages;

	/*
	** Go thru the queue to see if we already
	** have a block overlapping with this one
	*/
	while (que)
	{
	    if (que->addr <= blk->addr &&
		que->addr + que->npages * ME_MPAGESIZE > blk->addr ||
		que->addr >= blk->addr &&
		blk->addr + blk->npages * ME_MPAGESIZE > que->addr)
	    {
		status = ME_BAD_PARAM;
		LeaveCriticalSection(&MEalloc_csec);
		return (status);
	    }
	    que = que->next;
	}

	/* Add the new block to the queue */
	blk->next = MEalloc_list;
	MEalloc_list = blk;
    }
    LeaveCriticalSection(&MEalloc_csec);
# endif /* ME_DEBUG */

    return (status);
}

/******************************************************************************
**
** Name: ME_alloc_brk()
**  History:
**      16-Apr-96 (fanra01)
**          Added CL_ERR_DESC setup.
**
**
******************************************************************************/
STATUS
ME_alloc_brk(i4          flag,
             SIZE_TYPE          pages,
             PTR         *memory,
             SIZE_TYPE          *allocated_pages,
             CL_ERR_DESC *err_code)
{
	DWORD          status;

	status = OK;
	CLEAR_ERR(err_code);

	ME_getcount++;

	if (pages <= 0)
        {
                SETCLERR(err_code, ME_BAD_PARAM, 0);
                return ME_BAD_PARAM;
        }


	/*
	 * Return # of pages allocated - currently, we only return 
	 * successfully if allocated the number of pages requested.
	 */

	*allocated_pages = pages;
	*memory = VirtualAlloc(NULL,
	                    ME_MPAGESIZE * pages,
	                    MEM_COMMIT,
	                    PAGE_READWRITE);
	if (ME_getcount == 1)
		MEbase = *memory;
	
	if (*memory == NULL) {
		status = GetLastError();
		SETWIN32ERR(err_code, status, ER_alloc);
		status = FAIL;
	}

	return (status);
}

/******************************************************************************
** Name: MEfree_pages() - Free pages back to OS
**
** Description:
**  Routine will, if possible, return pages to OS.  Memory to be freed
**  must have been allocated using the MEget_pages() call.  Memory to
**  be freed need not be exact extent allocated by a MEget_pages() call
**  (for example one MEfree_pages() call can be used to free memory
**  allocated by two MEget_pages() calls assuming the memory was
**  contiguous; or one could free half of the memory allocated by a single
**  MEfree_pages() call.)
**
**  Note: The memory might not be freed immediately.  On Unix, memory
**  must be contiguous and freed in LIFO order.  Thus the memory will not
**  returned to the OS until all memory above it is also freed.
**  However, new memory allocations will prefer to use lower memory.
**
**  Shared Memory Note: Shared memory often must be released in an
**  all-or-nothing manner so again the actual release may be postponed.
**
** Inputs:
**      address                         address of the 1st allocated block of
**                  memory to be freed.
**      num_of_pages                    number of pages to be freed.
**
** Outputs:
**      err_code                        system dependent error code.
**
**  Returns:
**      OK              Free operation succeeded.
**      ME_BAD_PARAM        Parameter is invalid
**      ME_NOT_ALLOCATED        Pages to be freed were not all
**                      allocated (warning)
**      ME_OUT_OF_MEM       should never happen (OS
**                      doesn't want memory back)
**      ME_BAD_ADVISE       call was made during ME_USER_ALLOC.
**
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
******************************************************************************/
STATUS
MEfree_pages(PTR         address,
             SIZE_TYPE          num_of_pages,
             CL_ERR_DESC *err_code)
{
	DWORD           status = OK;
	SIZE_TYPE       memsize;

	CLEAR_ERR(err_code);
	ME_freecount++;

	memsize = num_of_pages * ME_MPAGESIZE;

	/*
	 * Can't be sure the previous will work.  Instead, give the shared
	 * memory system a chance to free memory...
	 */

	if ((status = MEshared_free(&address,
	                            &num_of_pages,
	                            err_code)) != ME_NOT_ALLOCATED) {
		return (status);
	} else {		/* mem is local only */
		status = VirtualFree(address, 0, MEM_RELEASE);
		if (!status)
		{
			status = GetLastError();
			SETWIN32ERR(err_code, status, ER_system);
			status = ME_NO_FREE;
		} 
		else 
		{
			status = 0;
		}
	}

	if (status == OK)
	{
	    ME_allocd_pages -= num_of_pages;
	}

# if defined(ME_DEBUG)
	EnterCriticalSection(&MEalloc_csec);
	{
	    ME_ALLOC_BLK *blk = MEalloc_list, *prev = NULL;

	    while (blk)
	    {
		if (blk->addr == address &&
		    blk->npages == num_of_pages)
		{
		    break;
		}
		prev = blk;
		blk  = blk->next;
	    }

	    if (blk == NULL)
	    {
		/* the block was not found -- an error */
		status = ME_BAD_PARAM;
	    }
	    else
	    {
		if (prev)
		    prev->next = blk->next;
		else
		    MEalloc_list = blk->next;
		MEfree(blk);
	    }
	}
	LeaveCriticalSection(&MEalloc_csec);
# endif /* ME_DEBUG */

	return (status);
}

/******************************************************************************
** Name: MEprot_pages() - Set protection attributes on a page range
**
** Description:
**  For host OS' which support it, modify access attributes on a page
**  range.  Possible uses of this routine include stack overflow warning,
**  facility and/or session verification traces of valid memory use,
**  protection from dynamically linked user code in INGRES proprietary
**  processes, ...
**
**  Mainline (user accessible) functionality can not depend on this
**  interface; improved error handling, enhanced debugging capabilities,
**  etc. are the intended clients.  Clients are not expected to ifdef
**  use of this interface.
**
**  Setting attributes on a shared segment only affects the client
**  that calls MEprot_pages.
**
** Inputs:
**  addr                start address of page range
**  pages               number of pages in range
**  protection          protections requested:
**          ME_PROT_READ
**          ME_PROT_WRITE
**          ME_PROT_EXECUTE
**
** Outputs:
**  return status
**
**  Returns:
**      OK              Protections could be set.
**      ME_NOT_ALLOCATED        Pages are not allocated.
**      ME_NOT_SUPPORTED        OS can not comply with request
**      ME_BAD_ADVISE       call was made during ME_USER_ALLOC.
**
**  Exceptions:
**      none
**
** Side Effects:
**  Memory accesses which violate set permissions will generate
**  OS specific exceptions.
**
******************************************************************************/
STATUS
MEprot_pages(PTR addr,
             SIZE_TYPE  pages,
             i4  protection)
{
	ULONG           flags;
	SIZE_TYPE       len;
	DWORD           status;
	ULONG           old_prot;

	len = pages * ME_MPAGESIZE;

	if (MEisalloc((i4)(((char *) addr - MEbase) / ME_MPAGESIZE),
	              pages,
	              TRUE))
		return ME_NOT_ALLOCATED;

	flags = PAGE_NOACCESS;

	if (protection & ME_PROT_READ)
		flags |= PAGE_READONLY;

	if (protection & ME_PROT_EXECUTE)
		flags |= PAGE_READONLY;	/* same as PAG_READ on 80386 */

	if (protection & ME_PROT_WRITE)
		flags |= PAGE_READWRITE;

	status = VirtualProtect(addr,
	                        len,
	                        flags,
	                        &old_prot);

	return status ? ME_NOT_SUPPORTED : OK;
}

/******************************************************************************
**
**
**
******************************************************************************/
STATUS
MEp_page_sem()
{
	return CSp_semaphore(TRUE, ME_page_sem);
}
/******************************************************************************
**
**
**
******************************************************************************/
STATUS
MEv_page_sem()
{
	return CSv_semaphore(ME_page_sem);
}

void
ME_mo_reg()
{
    (void) MOclassdef(MAXI2, ME_page_classes);
}

/******************************************************************************
**
**
**
******************************************************************************/
static STATUS
MO_get_used_bytes(i4  offset,
		  i4  objsize,
		  PTR object,
		  i4  luserbuf,
		  char *userbuf)
{
    return( MOlongout( MO_VALUE_TRUNCATED,
		      (i8)(ME_allocd_pages * ME_MPAGESIZE),
		      luserbuf,
		      userbuf ));
}
