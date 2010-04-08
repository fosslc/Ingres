/*
** Copyright (c) 1988, 2004 Ingres Corporation
**
*/
# include <compat.h>
# include <gl.h>
# include <me.h>
# include <er.h>
# include <lo.h>
# include <tr.h>
# include <sp.h>
# include <mo.h>
# include <cs.h>

# include <clconfig.h>
# include <meprivate.h>
# include <errno.h>

# ifdef axp_osf
# include <sys/mman.h>
# endif /* axp_osf */

/*
NO_OPTIM = nc4_us5
*/

/**
**
**  Name: MEPAGES.C - A Page based memory allocator
**
**  Description:
**	MEPAGES grew out of the need for a page based allocator which would
**	allow the exploitation of shared memory and other "special" memory.
**	This abstraction also supports returning pages to the operating
**	system (if conditions allow.)
**
**	NOTICE: These routines are not necessarily re-entrant.  Semaphores
**	must be added to make that so.
**
**	Shared Memory Issues:
**	This file provides definitions for shared memory operations.  Since
**	Shared memory operations will only be available on a subset of machines
**	on which ingres runs, one must only use these routines to improve
**	performance rather than add functionality.
**
**	Mainline clients of these routines must provide the same functionality
**	if these features are not available.  An example of this is the global
**	cache.  On machines with shared memory the global cache will be 
**	implemented.  On machines which lack this feature the global cache will
**	not be implemented but the interface to the i/o system will be the
**	same except that it may run slower.
**
**	OUTSTANDING ISSUES:
**	
**	1) Permissions on the shared memory segment.  (UNIX Example)
**
**	   On unix all shared memory interfaces provided, require permission
**	   on the segment to map to normal unix file permissions.  This is not
**	   a current problem with shared memory usage in the dbms server, since
**	   permission on these segments will be made 600 (only readable and
**	   writable by the creator).  The creator in this case will be programs
**	   which have been made setuid to ingres.
**
**	   The problem will show up if shared memory is used to communicate
**	   between front end and dbms server.  The (unpleasant) options
**	   include:  (1) Set permissions to be world readable and writable
**	   so that both a random user and ingres can read and write to the
**	   segment. (2) Kludge the CL by making the server setuid root
**	   (which has always been an unwelcome option to our users).
**	   The security problem can be handled by a user enabled option in the 
**	   main-line to encode the data.
**
**	2) ME_USER_ALLOC and shared memory
**
**	   Because we allow user applications to use their system provided
**	   stream based memory allocator, some difficulties will appear if
**	   we wish to map shared memory into the user's address space.  This
**	   problem is extremely environment dependent; on UNIX, this is
**	   probably the most non-portable issue possible.  Fundamentally,
**	   the problem is how to prevent the user memory allocator from
**	   interference with foreign shared memory being added to a process'
**	   address space.  Neither the current INGRES product set nor this
**	   proposal require the configuration of shared memory into a user
**	   process' address space, hence we simply disallow MEget_pages,
**	   MEfree_pages, MEprot_pages, and MEsmdestroy during
**	   MEadvise(ME_USER_ALLOC).  The other ME allocation routines
**	   can be built on the user allocator without using MEget_pages.
**
**	   (ME_FASTPATH_ALLOC partially answers this question by using
**	   the standard stream based memory allocator, but allowing
**	   the client to attach to shared memory that has been created
**	   by the server.)
**
**	This file defines:
**		MEget_pages()	- Allocate shared or unshared memory pages.
**		MEfree_pages()	- return allocated pages to the free page pool.
**		MEprot_pages()	- Set protection attributes on a page range.
**		MEsmdestroy()	- Destroy an existing shared memory segment.
**	    internal routines:
**		MEfindpages() 	- find free pages
**		MEsetpg()	- mark pages as allocated in a bitmap
**		MEclearpg()	- mark pages as free in a bitmap
**		MEalloctst()	- check a region for allocation status
**
**  History:
**	15-aug-89 (fls-sequent)
**	    Added ME_INGRES_SHMEM_ALLOC MEadvice support for MCT.
**
**	12-jun-89 (rogerk)
**	    Integrated changes made for Terminator I - shared buffer manager.
**	    Added allocated_pages parameter to MEget_pages.  Changed shared
**	    memory key from being a LOCATION to being a character string.
**      21-may-90 (blaise)
**          Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**      24-may-90 (blaise)
**          Integrated changes from ingresug:
**            Add "include <compat.h>";
**            Add TRdisplay error messages.
**      31-may-90 (blaise)
**          Added include <me.h> to pick up redefinition of MEfill().
**      21-Jan-91 (jkb)
**          Change "if( !(MEadvice & ME_INGRES_ALLOC) )" to
**          "if( MEadvice == ME_USER_ALLOC )" so it works with
**          ME_INGRES_THREAD_ALLOC as well as ME_INGRES_ALLOC
**	    Change ME_INGRES_SHMEM_ALLOC to ME_INGRES_THREAD_ALLOC
**	21-mar-91 (fredv, seng)
**	    Added IBM RS/6000 specific code to allow the INGRES memory
**	    allocator to co-exist with the system malloc call.  Integrated
**	    from 6.3/02p port.
**
**	 2-dec-88 (anton)
**	    Massive portability and readability changes
**
**	13-jul-88 (anton)
**	    Integrated into ME
**
**	03-May-88 (anton)
**	    Clarifications.
**
**      23-feb-88 (anton)
**	    Created.
**	11-mar-92 (jnash)
**	    Implement ME_LOCKED_MASK flag via mlock()for Sun4.
**      21-nov-92 (terjeb)
**	    Use ifdef to not include the mlock() code in this file for
**	    those systems not supporting this call.
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.  Delete forward and external function references;
**              these are now all in either me.h or meprivate.h.
**      05-feb-92 (smc)
**          	Cast return of sbrk(0) to same type as MEalloctab and
**		included mman.h for axp_osf to provide define for mlock.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jul-1993 (jnash)
**	    xDEBUG TRdisplay of mlock() success.
**	11-aug-93 (ed)
**	    unconditional prototypes
**	23-aug-1993 (bryanp)
**	    On HP-UX 9.0, and perhaps on other machines, MEget_pages doesn't
**		return memory aligned to any special boundary. In particular,
**		it doesn't return memory aligned to ME_MPAGESIZE boundaries.
**		Therefore, I've removed the alignment check in MEfree_pages,
**		which was causing MEfree_pages to reject perfectly valid
**		memory addresses with ME_BAD_PARAM.
**      25-aug-93 (smc)
**          Used ME_ALIGN_MACRO to set MElimit so that the size of the ptr
**          can be taken care of in the declaration of the macro.
**          Commented lint pointer truncation warning.
**          Made the restrictive alignment casts SCALARP to be portable to
**          axp_osf.         
**	20-sep-93 (kwatts)
**	    Integrated 6.4 fix from gordonw in MEfree_pages().
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in MEget_pages().
**      23-Mar-1994 (daveb) 60410
**          Add MO driven code to force "out of memory" errors for
**          testing, and other MO objects to show what's going on.
**	23-may-1994 (mikem)
**	    Bug #63113
**	    Added support for locking shared memory allocated with shmget().
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	19-apr-1995 (canor01)
**	    disabled the MCT shared memory functions
**	08-nov-1995 (canor01)
**	    Add ME_FASTPATH_ALLOC for fastpath API.
**      03-jun-1996 (canor01)
**          Add synchronization and make MEstatus local to make ME
**          stream allocator thread-safe when used with operating-system
**          threads.
**	19-Jul-96 (gordy)
**	    Change #ifdef xCL_MLOCK_EXISTS to include xCL_SETEUID_EXISTS.
**	    Preceding code handles both cases individually, but then code
**	    ifdef'd only for MLOCK referrences both functions.  It worked 
**	    as long as both defined or neither defined, but fails if 
**	    xCL_MLOCK_EXISTS is defined and xCL_SETEUID_EXISTS is not.
**	10-sep-1996 (canor01)
**	    Add multithreaded recursion checks in ifdef's for operating-system
**	    threads only.  Register MO objects only after memory in the
**	    pool has been initialized, since MO will need to allocate some.
**	11-sep-1996 (canor01)
**	    Also do not attempt to register MO objects when recursing in
**	    MEget_pages() with os threads.
**	10-oct-1996 (canor01)
**	    MEpage_recurse flag could be reset at wrong time, causing
**	    deadlock.
**	25-mar-1997 (canor01)
**	    Do not call CS_get_thread_id() before the first call to
**	    MEadvise().  During C++ setup, malloc() can be called before
**	    threading is enabled.
**      29-may-1997 (canor01)
**          Clean up some compiler warnings.
**      29-may-1997 (muhpa01)
**          For POSIX_DRAFT_4, added call to MEinitlists to initialize
**          mutexes, added initializer for MEpage_tid and used
**          CS macros for setting/testing MEpage_tid
**	20-Apr-1998 (merja01)
**          Change definition of MEpage_tid from i4  to CS_THREAD_ID to
**          match definition in me.c...this change appears to have not
**          been cross intgrated properly.
**      17-sep-1998 (matbe01)
**          Added NO_OPTIM for nc4_us5 to file.
**	02-Dec-1998 (muhpa01)
**	    Removed POSIX_DRAFT_4 code for HP - obsolete.
**    17-Dec-1998 (hanch04)
**	Use xCL_SYS_PAGE_MAP instead of JASMINE so that platforms that
**	perform better with system memory maps use it.
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	26-jan-1999 (muhpa01)
**	    In ME_alloc_brk(), when pageno (from MEfindpages) is < MEendtab,
**	    and pageno + pages is > MEendtab, and we detect that there is
**	    also pages_by_malloc, we were incorrectly mapping the system
**	    malloc usage & including it in the memory returned to the
**	    caller.  This resulted in various forms of SIGBUS/SIGSEGV errors
**	    on HP 11.00, and could be afflicting other platforms as well.
**	    Fixed the system-malloc mapping & the memory usage overlap.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**
**      08-Dec-1999 (podni01)  
**          Put back all POSIX_DRAFT_4 related changes; replaced POSIX_DRAFT_4
**          with DCE_THREADS to reflect the fact that Siemens (rux_us5) needs 
**          this code for some reason removed all over the place.
**      24-May-2000 (hanal04) Bug 98820 INGSRV 1094
**          Introduced the MEalloctab_mutex to protect the MEalloctab.
**          Callers should hold the mutex prior to referencing the
**          MEalloctab. If a dependant update is to be made the mutex
**          should not be released between reference and update of
**          the MEalloctab.
**      25-Sep-2000 (hanal04) Bug 102725 INGSRV 1275.
**          Acquire the MEbrk_mutex to protect the sbrk() value
**          used in ME_alloc_brk().
**	21-jan-1999 (hanch04) 
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**      08-Oct-2002 (hanch04)
**          Replace pagesize and pageno with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**	18-mar-2003 (devjo01)
**	    Change <mman.h> to <sys/mman.h> (Header used by Tru64 only)
**	    This is the correct location, and is consistent with meshare.c
**      27-jan-2004 (horda03) Bug 111658/INGSRV2680
**          Allow greater than 2GB of memory to be allocated, if the system
**          will allow (i.e. a 64 bit OS). Create a chain of memory bit
**          maps for the expanding memory being used. Also, as SIZE_TYPE is
**          an unsigned quantity, no concept of <0.
**      26-may-2004 (horda03) Bug 112390/INGSRV2842
**          The memory acquired for the process is divided into pages of
**          ME_PAGESIZE bytes. MElimit is the address where the next memory
**          allocated to the process is expected to be. This may not be the
**          case because memory is also added to the process by OS function
**          calls. sbrk(0) is used to determine the current end of memory
**          and if it doesn't equal MElimit, we know the OS has extended
**          the memory, so we take account of the memory allocated by the
**          OS by rounding it up the the next Ingres page.
**          The problem comes when an OS function extends the memory of the
**          following the sbrk(0) call, and before the brk(MElimt) call, if
**          this should happen, then OS functions and Ingres may both be
**          using the same piece of memory for conflicting purposes, which
**          will (probably) lead to a SIGSEGV.
**          There is no way that Ingres can prevent OS functions extending
**          memory between an sbrk(0) call and the brk() call. Rather
**          the OS supported method is to use malloc - but for us to do this
**          requires a re-write of the ME package.
**	18-Nov-2004 (hanje04)
**	   BUG 113487/ISS 13680685
**	   For reasons that are not entirely clear, sbrk() will sometimes
**	   fail when running under RHEL 3.0 Kernel 2.4.21-20EL. Calling sbrk(0)
**	   just before the point of failure returns an address which inside the
**	   current address space for the server. sbrk(0) should return the end 
**	   of the current address space which is implicitly "out of bounds".
**	   The 2.4.21-20EL kernel employs an differnent memory map that ALL
**	   other 2.4 kernels (and all but the very latest 2.6 kernels) and
**	   it is strongly suspected that this is what is causing the problem.
**	   (See http://lwn.net/Articles/91829/ for much more info)
**	   To address this ME_alloc_memalign has been added to obtain page
**	   aligned memory directly. This was previously done in
**	   ME_alloc_brk() using memalign() when xCL_SYS_PAGE_MAP is defined. 
**	   ME_alloc_memalign() uses posix_memalign() on Linux (similar to 
**	   memalign() which is used on Solaris) instead of sbrk() and brk()
**	   to requests aligned memory directly from the OS. It also removes the 
**	   problems described above in the fix for bug Bug 112390/INGSRV2842.
**	   ME_alloc_memalign()is currently only called when xCL_SYS_PAGE_MAP
**	   is defined but is now always available.
**	02-Dev-2004 (hanje04)
**	   In ME_alloc_memalign, pages and *allocated_pages should be of type
**	   SIZE_TYPE and not i4 to conform to changes made for bug 111847.
**	28-Dec-2004 (hweho01)
**         OS routine memalign() is not available on AIX; avoid linker error, 
**         excludes ME_alloc_memalign() for rs4_us5 platform.
**	19-Jan-2005 (schka24)
**	    MEget_pages is now passed a size-type pointer for the returned
**	    size.  Fix (i4) casting that was throwing away the size on
**	    Solaris 64-bit, causing the caller (eg CSsetup) to think that
**	    nothing had been allocated.  (Another bug in the caller led to
**	    a double free and eventual segv.)
**	    Remove the intermediate alloc-brk step for SYS_PAGE_MAP
**	    platforms.  Fix comments that imply that MEget_page is actually
**	    a page allocator;  on some platforms it's a malloc wrapper,
**	    and must be treated as such.
**	08-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
[@history_template@]...
**/


/*
** Definition of all global variables used by this file.
*/

GLOBALREF	QUEUE		ME_segpool;
# ifdef OS_THREADS_USED
GLOBALREF CS_SYNCH      MEalloctab_mutex;
GLOBALREF CS_SYNCH      MEfreelist_mutex;
GLOBALREF CS_SYNCH      MElist_mutex;
GLOBALREF CS_SYNCH      MEtaglist_mutex;
GLOBALREF CS_SYNCH      MEpage_mutex;
GLOBALREF CS_THREAD_ID  MEpage_tid;
GLOBALREF i4            MEpage_recurse;
# endif /* OS_THREADS_USED */

# if defined(DCE_THREADS)
static CS_THREAD_ID ME_thread_id_init ZERO_FILL;
# endif /* DCE_THREADS */
/*
** Definition of all global variables owned by this file.
*/

GLOBALDEF	MEALLOCTAB	MEalloctab;
GLOBALDEF	SIZE_TYPE	MEendtab;
GLOBALDEF	char	*MEbase = NULL;
GLOBALDEF	char	*MElimit;

/* call counters, for MO */

static i4	ME_getcount ZERO_FILL;
static i4	ME_freecount ZERO_FILL;

/*
** Debug stuff 
**
**  In MEget_pages, if MEtestcount > 0, and MEtestcount < ME_getcount,
**  	then return ME_OUT_OF_MEM when !(MEgetgount % MEtestmod)).
**  	
**  These are settable via MO.
*/

static   	i4	ME_testcount ZERO_FILL;
static   	i4	ME_testmod ZERO_FILL; 
static	    	i4	ME_testerrs ZERO_FILL;

static MO_GET_METHOD MO_get_used_pages;
static MO_GET_METHOD MO_get_used_bytes;

GLOBALDEF MO_CLASS_DEF ME_page_classes[] =
{
  { 0, "exp.clf.unix.me.num.get_pages",
	sizeof(ME_getcount), MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&ME_getcount, MOcdata_index },

  { 0, "exp.clf.unix.me.num.free_pages",
	sizeof(ME_freecount), MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&ME_freecount, MOcdata_index },

  { 0, "exp.clf.unix.me.num.test_count",
	sizeof(ME_testcount),
	MO_READ|MO_SERVER_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&ME_testcount, MOcdata_index },

  { 0, "exp.clf.unix.me.num.test_errs",
	sizeof(ME_testcount),
	MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&ME_testerrs, MOcdata_index },

  { 0, "exp.clf.unix.me.num.test_mod",
	sizeof(ME_testmod),
	MO_READ|MO_SERVER_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&ME_testmod, MOcdata_index },

  { 0, "exp.clf.unix.me.num.pages_used",
	0, MO_READ, 0, 0, MO_get_used_pages, MOnoset, 0, MOcdata_index },

  { 0, "exp.clf.unix.me.num.bytes_used",
    	0, MO_READ, 0, 0, MO_get_used_bytes, MOnoset, 0, MOcdata_index },

  { 0 }
};

/*
**  Definition of static variables and forward static functions.
*/

 
/*{
** Name: ME_init()	- Initialize the ME system
**
** Description:
**	Initialize pointers and tables to manage all memory in the
**	process.
**
** Outputs:
**      err_code                        system dependent error code.
**
**	Returns:
**	    OK
**	    ME_OUT_OF_MEM
**
**	Exceptions:
**	    none
**
** History:
**	23-feb-88 (anton)
**          Created.
**	24-jan-89 (roger)
**	    Added CL_ERR_DESC* parameter, so we can report system call failures.
**	20-mar-89 (russ)
**	    Changed invokation of bzero to MEfill.
**	15-aug-89 (fls-sequent)
**	    Added ME_INGRES_SHMEM_ALLOC MEadvice support for MCT.
**      21-Jan-91 (jkb)
**          Change "if( !(MEadvice & ME_INGRES_ALLOC) )" to
**          "if( MEadvice == ME_USER_ALLOC )" so it works with
**          ME_INGRES_THREAD_ALLOC as well as ME_INGRES_ALLOC
**	    Change ME_INGRES_SHMEM_ALLOC to ME_INGRES_THREAD_ALLOC
**      24-May-2000 (hanal04) Bug 98820 INGSRV 1094
**          Mutex protect the MEalloctab with the MEalloctab_mutex.
*/
static STATUS
ME_init(
	CL_ERR_DESC	*err_code)
{
    register SIZE_TYPE tabsize;
    
    if ( MEadvice == ME_USER_ALLOC)
    {
	CL_CLEAR_ERR( err_code );
	return(ME_BAD_ADVICE);
    }

    QUinit(&ME_segpool);
# ifdef xCL_SYS_PAGE_MAP
    MEbase = (PTR)TRUE; /* mark initialized */
# else /* xCL_SYS_PAGE_MAP */
    tabsize = ME_MAX_ALLOC / ME_BITS_PER_BYTE;
# ifdef OS_THREADS_USED
    CS_synch_lock(&MEalloctab_mutex);
# endif /* OS_THREADS_USED */
    MEalloctab.next = (MEALLOCTAB *) 0;
    MEalloctab.alloctab = (char *)sbrk(tabsize);
    if (MEalloctab.alloctab == SBRK_FAIL)
    {
	SETCLERR(err_code, 0, ER_sbrk);
# ifdef OS_THREADS_USED
        CS_synch_unlock(&MEalloctab_mutex);
# endif /* OS_THREADS_USED */
	return(ME_OUT_OF_MEM);
    }
    MElimit = MEalloctab.alloctab + tabsize;
    MElimit = ME_ALIGN_MACRO(MElimit, ME_MPAGESIZE);
 
    if (brk(MElimit))
    {
	MElimit = 0;
	SETCLERR(err_code, 0, ER_brk);
# ifdef OS_THREADS_USED
        CS_synch_unlock(&MEalloctab_mutex);
# endif /* OS_THREADS_USED */
	return(ME_OUT_OF_MEM);
    }
    MEfill(tabsize,'\0',MEalloctab.alloctab);
# ifdef OS_THREADS_USED
    CS_synch_unlock(&MEalloctab_mutex);
# endif /* OS_THREADS_USED */
    MEbase = MElimit;
# endif /* xCL_SYS_PAGE_MAP */

# ifndef OS_THREADS_USED
    ME_mo_reg();
# endif /* OS_THREADS_USED */
    return(OK);
}
 
/*{
** Name: MEget_pages  - add (possibly shared) memory to the client
**
** Description:
**
**      This routine adds pages to the virtual address space of the requesting
**	program. This routine is intended to be used by the dbms server for all
**	dynamic memory allocation.  It adds pages which are ME_MPAGESIZE bytes 
**	in length, and are guaranteed to be aligned on ME_MPAGESIZE byte 
**	boundaries.
**
**		(NOTE: On at least one machine, MEget_pages does NOT return
**		memory which is aligned on ME_MPAGESIZE boundaries!! This
**		machine is HP-UX 9.0, which appears to be returning memory
**		on 4096 byte boundaries, although "man shmat" makes no
**		guarantees whatsoever.)
**
**	Implementor's of the ME module can assume that MEget_pages will be
**	the exclusive allocator of memory when MEadvise(ME_INGRES_ALLOC).
**
**      This routine allocates some number of pages.  The memory 
**      allocated here is considered to be long-term memory (on the order
**      of session or server length of time).  All memory returned by this 
**      service is page (ME_MPAGESIZE bytes) aligned, and the unit of measuring
**      of this memory is pages (i.e. requests come in for number of pages, 
**      not number of bytes).  Note that this page size must be variable
**	from environment to environment.  This is necessary to adjust for
**	environmental performance differences in paging activity.
**
**		(Please note that alignment appears to vary from machine to
**		machine.)
**
**	When allocating shared memory, even more restrictive conditions
**	may apply, e.g.
**	  1) the allocation size being greater
**	  2) the alignment boundaries are further apart
**	  3) only specific regions of the address space	can support shared
**	     segments.
**	The various restrictions on shared memory are derived from a machine's
**	hardware architecture and the operating system's allocation primitives.
**	Because these restrictions are so varied, MEget_pages is not allowed
**	during MEadvise(ME_USER_ALLOC).  See 'OUTSTANDING ISSUES' above.
**
**	Attaching to a shared segment is performed by specifying either
**	ME_MSHARED_MASK or ME_SSHARED_MASK in the flags argument and by not
**	specifying ME_CREATE_MASK.  When attaching to an already existing
**	shared segment the user supplied 'pages' argument is ignored, the
**	user recieves all the pages that belong to the shared segment. This
**	number is returned in the 'allocated_pages' parameter.
**
**	When creating or attaching to shared memory segments, a key parameter
**	must be specified which uniquely identifies this shared segment within
**	the current Ingres Installation.  The key parameter is a character
**	string that must follow the Ingres file naming conventions: 
**	"prefix.suffix" where prefix is at most 8 characters and suffix is
**	at most 3.  All characters must be printable.  ME may use this key
**	to create a file to identify the shared memory segment, although this
**	is up to the particular implementation of ME.
**
** Inputs:
**      flag                            flag specifying special operations on
**					allocated memory. (advisorys)
**					Can be or'ed together.
**		    ME_NO_MASK		None of the other flags apply.
**                  ME_MZERO_MASK       zero the memory before returning
**		    ME_MERASE_MASK	"erase" the memory (DoD) 
**					 ...on systems where applicable
**		    ME_IO_MASK		Memory used for I/O
**		    ME_SPARSE_MASK	Memory used `sparsely' so should
**					be paged in small chunks if applicable
**		    ME_LOCKED_MASK	Memory should be locked down to inhibit
**					paging -- i.e. buffer caches.
**		    ME_SSHARED_MASK	Memory shared across processes on a
**					single machine
**		    ME_MSHARED_MASK	Memory shared with other processors.
**		    ME_CREATE_MASK	Create a new shared memory segment.
**					(Usually specified with ME_MZERO_MASK)
**					It is an error if the specified segment
**					already exists.
**		    ME_NOTPERM_MASK	Create temporary shared memory segment
**					rather than a permanent one (on systems
**					where this is supported).
**
**		    The flags ME_SSHARED_MASK, ME_MSHARED_MASK cannot both be
**		    specified in an MEget_pages call.  One of these flags must
**		    be specified in order for ME_CREATE_MASK or ME_NOTPERM_MASK
**		    flags to be used.
**
**      pages                           number of pages requested
**					note:
**					    if the memory is a shared segment
**					    that has already been created,
**					    flag value of ME_MSHARED_MASK|
**						ME_SSHARED_MASK&~ME_CREATE_MASK,
**					    the pages argument is ignored.
**
**	key				A character string identifier for the
**					a shared memory segment.  This argument
**					is only requried with shared memory
**					operations - when not needed, use NULL.
**
**					The key identifier should be a character
**					string that follows the Ingres file
**					naming conventions: 8 characters max
**					prefix name and 3 characters max suffix.
** Outputs:
**
**	memory				Upon success the address of the
**					allocated memory.
**	allocated_pages			Number pages of shared memory returned.
**					Normally, this will always match the
**					'pages' input parameter.  If attaching
**					to an existing shared memory segment,
**					this will give the actual size of the
**					attached segment.
**	err_code			System specific error information.
**
**	Returns:
**		OK			If all went well.
**					Otherwise, set to: 
**		ME_OUT_OF_MEM		Could not allocate required amount of 
**					memory.
**
**		ME_BAD_PARAM		Illegal parameter passed to routine
**					(pages <= 0, ...)
**
**		ME_ILLEGAL_USAGE	Combination of attributes is invalid
**
**		ME_BAD_ADVISE		call was made during ME_USER_ALLOC.
**
**		ME_ALREADY_EXISTS	shared memory identifier already exists
**
**		ME_NO_SUCH_SEGMENT	specified shared memory segment does
**					not exist
**		ME_NO_SHARED		system can not support shared memory
**		ME_NO_MLOCK		no mlock() function on this machine
**		ME_LOCK_FAIL 		mlock() function call failed
**		ME_NO_SHMEM_LOCK  	can't lock older sys V shared mem
**					(we require use of mmap())
**
**	Exceptions:
**	    none
**
** Side Effects:
**	File may be created in ME_SHMEM_DIR directory to identify shared segment
**
** Implementation Notes:
**
**	Shared Memory Segment id's on UNIX are derived using the ftok()
**	function utilizing a file in the Installation's memory directory.
**
**	Each time a shared segment is created, a file will be created in
**	a subdirectory of II_CONFIG (II_CONFIG/memory) that identifies that
**	shared segment.  The filename will be the user supplied key name.
**
**	These files are also used by MEshow_pages to list existing shared
**	segments.
**
**	While implementing ME routines, note that the existence of such a file
**	does not guarantee the existence of a corresponding shared segment
**	(for example, following a system crash), but the reverse is guaranteed
**	to be true.
**
**	The ME_NOTPERM_MASK flag is not implemented on UNIX.  Clients of
**	shared memory must implement other ways to clean up segments left
**	around unintentionally.
**
** History:
**	 2-dec-88 (anton)
**	    massive portability and readability changes
**	11-jul-88 (anton)
**	    integrate as part of ME
**	23-feb-88 (anton)
**	    combined shared memory spec and first cut at implementing.
**	21-Jan-88 (mmm)
**          Proposed.
**	24-jan-1989 (roger)
**          Don't set err_code on ME_init failure; let ME_init do it!
**	20-mar-89 (russ)
**	    Changed invokation of bzero to MEfill.
**	12-jun-89 (rogerk)
**	    Integrated changes for Terminator I - shared buffer manager.
**	    Added allocated_pages argument.  Changed shared memory key
**	    to be character string rather than location.
**	11-mar-92 (jnash)
**	    Implement ME_LOCKED_MASK flag via mlock()for Sun4.
**      21-nov-92 (terjeb)
**	    Use ifdef to not include the mlock() code in this file for
**	    those systems not supporting this call.
**	26-jul-1993 (jnash)
**	    xDEBUG TRdisplay of mlock() success.
**	23-aug-1993 (bryanp)
**	    Added comments to function header indicating that, at least under
**		HP-UX 9.0, the memory returned by MEget_pages is NOT aligned
**		to ME_MPAGESIZE boundaries.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	23-may-1994 (mikem)
**	    Bug #63113
**	    Added support for locking shared memory allocated with shmget().
**	19-Jul-96 (gordy)
**	    Change #ifdef xCL_MLOCK_EXISTS to include xCL_SETEUID_EXISTS.
**	    Preceding code handles both cases individually, but then code
**	    ifdef'd only for MLOCK referrences both functions.  It worked 
**	    as long as both defined or neither defined, but fails if 
**	    xCL_MLOCK_EXISTS is defined and xCL_SETEUID_EXISTS is not.
**	10-sep-1996 (canor01)
**	    Add multithreaded recursion checks in ifdef's for operating-system
**	    threads only.  Register MO objects only after memory in the
**	    pool has been initialized, since MO will need to allocate some.
**	11-sep-1996 (canor01)
**	    Also do not attempt to register MO objects when recursing in
**	    MEget_pages() with os threads.
**	10-oct-1996 (canor01)
**	    MEpage_recurse flag could be reset at wrong time, causing
**	    deadlock.
**	25-mar-1997 (canor01)
**	    Do not call CS_get_thread_id() before the first call to
**	    MEadvise().  During C++ setup, malloc() can be called before
**	    threading is enabled.
**	19-Jan-2005 (schka24)
**	    allocated-pages param is now SIZE_TYPE, remove intermediary
**	    (which actually caused difficulties on Solaris 64-bit).
*/
STATUS
MEget_pages(
	i4		flag,
	SIZE_TYPE	pages,
	char		*key,
	PTR		*memory,
	SIZE_TYPE	*allocated_pages,
	CL_ERR_DESC	*err_code)
{
    STATUS	status;
    uid_t	saveuid;
# ifdef OS_THREADS_USED
    static bool	MO_reg_needed = TRUE;
# endif /* OS_THREADS_USED */

    if (MEbase == NULL)
    {
	if ((status = ME_init(err_code)) != OK)
	    return(status);
    }
# ifdef OS_THREADS_USED
    else if (MO_reg_needed && MEgotadvice && !MEpage_recurse)
    {
	MO_reg_needed = FALSE;
	ME_mo_reg();
    }

    /*
    ** if this thread is the one holding the lock, it is recursing,
    ** so don't try to re-acquire the lock.
    */
    if ( MEgotadvice )
    {
# if defined(DCE_THREADS)
        /*  For Posix draft 4 on rux_us5, there is no static initializer
        **  for pthread mutexes, so we need the call to MEinitLists()
        **  which does the job
        */
            if ( MEsetup == FALSE )
                        if ( ( status = MEinitLists() ) != OK )
                                return(status);

        if ( CS_synch_trylock( &MEpage_mutex ) == 0 )
# else
        if ( CS_synch_trylock( &MEpage_mutex ) )
# endif /* POSIX_DRAFT_4 */
        {
            if ( !CS_thread_id_equal( MEpage_tid, CS_get_thread_id() ) )
	    {
	        CS_synch_lock( &MEpage_mutex );
                MEpage_recurse = 0;
            }
        }
        CS_thread_id_assign( MEpage_tid, CS_get_thread_id() );
        MEpage_recurse++;
    }
# endif /* OS_THREADS_USED */

    if (flag & (ME_SSHARED_MASK|ME_MSHARED_MASK))
    {
	status = ME_alloc_shared(flag, pages, key, memory,
	    allocated_pages, err_code);
    }
    else
    {
	status = ME_alloc_brk(flag, (SIZE_TYPE) pages, memory, allocated_pages, err_code);
    }

# ifdef OS_THREADS_USED
    if ( MEpage_recurse )
    {
        MEpage_recurse--;
        if ( MEpage_recurse == 0 )
        {
# if defined(DCE_THREADS)
                CS_thread_id_assign( MEpage_tid, ME_thread_id_init );
# else
            MEpage_tid = 0;
# endif /* DCE_THREADS */
	    CS_synch_unlock( &MEpage_mutex );
        }
    }
# endif /* OS_THREADS_USED */

    if (status == OK && (flag & (ME_MZERO_MASK|ME_MERASE_MASK)))
    {
	MEfill(pages * ME_MPAGESIZE,'\0',(char *)*memory);
    }

# ifndef xCL_MLOCK_EXISTS

    if (status == OK && (flag & ME_LOCKED_MASK))
    {
# if defined(SYS_V_SHMEM)
	{

	    /* Lock the memory, can only be done as root.
	    ** If the user requested locked memory and it fails, then we'll
	    ** fail the startup attempt. 
	    */
	    TRdisplay("calling ME_lock_sysv_mem\n");
	    status = ME_lock_sysv_mem(key, err_code);
	    return(status);
	}
# endif
    }
#endif

# ifndef xCL_SETEUID_EXISTS
    if (status == OK && (flag & ME_LOCKED_MASK))
    {
	TRdisplay("MEget_pages: Memory locking requires seteuid() fn call\n");
	CL_CLEAR_ERR( err_code );
        return(ME_NO_MLOCK);
    }
# endif 

# ifndef xCL_MLOCK_EXISTS
    if (status == OK && (flag & ME_LOCKED_MASK))
    {
	TRdisplay("MEget_pages: Memory locking requires mlock() fn call\n");
	CL_CLEAR_ERR( err_code );
        return(ME_NO_MLOCK);
    }
# endif   

#if defined(xCL_MLOCK_EXISTS) && defined(xCL_SETEUID_EXISTS)
    /* Check for memory locking, currently limited to DMF cache */
    if (status == OK && (flag & ME_LOCKED_MASK))
    {
	/* Lock the memory, can only be done as root.
	** If the user requested locked memory and it fails, then we'll
	** fail the startup attempt. 
	*/

	saveuid = geteuid();
        if ( seteuid((uid_t)0) == -1 )
	{
	    SETCLERR(err_code, 0, ER_setuid);
	    return(ME_LOCK_FAIL);
	}

	if (mlock(*memory, pages * ME_MPAGESIZE) == -1)
	{
            SETCLERR(err_code, 0, ER_mlock)
	    _VOID_ seteuid((uid_t)saveuid);   
            return(ME_LOCK_FAIL);
	}

#ifdef xDEBUG
	TRdisplay("MEget_pages: mlock() of %d pages succeeded, addr: %p\n", 
		pages, memory);
#endif

	if ( seteuid((uid_t)saveuid) == -1 )
	{
	    SETCLERR(err_code, 0, ER_setuid);
            return(ME_LOCK_FAIL);
	}
    }
#endif

    return(status); 
}

#ifndef xCL_SYS_PAGE_MAP
/*
** Name: ME_alloc_brk() (for non-SYS_PAGE_MAP platforms)
**
**	This implementation allocates memory "by hand" by looking at sbrk()
**	directly, and maintaining an allocation bit-map of pages that
**	we've acquired from the OS.  
**
** History:
**	15-aug-89 (fls-sequent)
**	    Added ME_INGRES_SHMEM_ALLOC MEadvice support for MCT.
**      21-Jan-91 (jkb)
**	    Change ME_INGRES_SHMEM_ALLOC to ME_INGRES_THREAD_ALLOC
**	10-jan-94 (swm)
**	    Bug #58645
**	    Add axp_osf to the ris_us5 case which checks whether malloc()
**	    in libc affects the sbrk value. Also, cast return value of
**	    sbrk() to PTR, on axp_osf it returns void *.
**      15-Mar-1994 (daveb) 60410
**          Add test hook for out of memory errors.   Remove ifdef
**  	    around code that checks malloc interactions.  Do it
**  	    everywhere.
**      24-May-2000 (hanal04) Bug 98820 INGSRV 1094
**          We must mutex protect references and changes made to the
**          MEalloctab by holding the MEalloctab_mutex.
**      25-Sep-2000 (hanal04) Bug 102725 INGSRV 1275.
**          Acquire the MEbrk_mutex when checking to see if the sbrk()
**          value has changed. If we don't stop the sbrk() from changing
**          we'll trash pages that are already in use.
**	13-Mar-2003 (bonro01)
**	    Fix bad integration that caused CS_sync_unlock(MEbrk_mutex)
**	    to be placed outside the IF statement where the lock was
**	    performed.  This caused unlock to be performed without a 
**	    corresponding lock.
**      26-may-2004 (horda03) Bug 112390/INGSRV2842
**          Use sbrk to acquire the memory, thus preventing OS and
**          Ingres trying to use the same piece of memory for their
**          own usage.
**	18-Nov-2004 (hanje04)
**	    BUG 113487/ISS 13680685
**	    Move all of xCL_SYS_PAGE_MAP block to new function, 
**	    ME_alloc_memalign(), which completely replaces ME_alloc_brk().
**	    When xCL_SYS_PAGE_MAP is deinfed system calls are used to obtain
**	    page aligned memory directly instead of messing around with sbrk().
*/
STATUS
ME_alloc_brk(
	i4		flag,
	SIZE_TYPE	pages,
	PTR		*memory,
	SIZE_TYPE	*allocated_pages,
	CL_ERR_DESC	*err_code)
{
    STATUS	status;
    SIZE_TYPE	pgsize;
    SIZE_TYPE	pageno;
    SIZE_TYPE	bytes_by_malloc;
    SIZE_TYPE	pages_by_malloc;
    bool        found = TRUE;

    pgsize = ME_MPAGESIZE;
    status = OK;
    CL_CLEAR_ERR( err_code );

    ME_getcount++;
    if (pages == (SIZE_TYPE) 0)
    {
	status = ME_BAD_PARAM;
    }
    if (!status && (flag & ME_ALIGN))
    {
	status = ME_align(&pgsize);
    }

    if (pgsize)
    {
        pgsize /= ME_MPAGESIZE;
    }
 
    /* test hook to force mem errors (60410) */
    if( (ME_testcount > 0) && (ME_testcount > ME_getcount) &&
       !(ME_getcount % ME_testmod ) )
    {
        ME_testerrs++;
        status = ME_OUT_OF_MEM;
    }
	
# ifdef OS_THREADS_USED
    CS_synch_lock(&MEalloctab_mutex);
# endif /* OS_THREADS_USED */
    if (!status)
    {
        if (flag & ME_ADDR_SPEC)
        {
            if ((SIZE_TYPE)*memory & (pgsize-1))
            {
                status = ME_ILLEGAL_USAGE;
            }
#ifdef fixme
            else if ((char *)*memory < MEbase ||
                     (char *)*memory >= MEbase+ME_MPAGESIZE*(ME_MAX_ALLOC-pages))
            {
                status = ME_BAD_PARAM;
            }
#endif
            else
            {
                pageno = ((char *)*memory - MEbase) / ME_MPAGESIZE;
                if (MEisalloc(pageno, pages, FALSE))
                {
                    status = ME_ILLEGAL_USAGE;
                }
            }
        }
        else if (pgsize > (SIZE_TYPE) 1)
        {
            /* restrictive alignment */
            pageno = ((((SCALARP)MEbase
                        & ~(SCALARP)(pgsize * ME_MPAGESIZE - 1))
                       + pgsize * ME_MPAGESIZE) - (SCALARP)MEbase)
              / ME_MPAGESIZE;
 
            while (pageno < MEendtab &&
                   MEisalloc(pageno, pages, FALSE))
            {
                pageno += pgsize;
            }
            if (pageno + pages > (SIZE_TYPE) ME_MAX_ALLOC)
            {
		found = FALSE;
            }
        }
        else
        {
            /* normal allocation */
            found = MEfindpages(pages, &pageno);
        }
    }
    if (!status)
    {
        if (!found)
        {
            status = ME_OUT_OF_MEM;
        }
        else
        {
            /*
            ** Return # of pages allocated - currently, we only return
            ** successfully if allocated the number of pages requested.
            */
            *allocated_pages = pages;
 
            if (!(flag & ME_ADDR_SPEC))
            {
                *memory = MEbase + ME_MPAGESIZE * pageno;
            }
            if (pageno + pages > MEendtab)
            {
                /*
		** We need to get memory such that the memory we allocate
		** falls on our memory page boundary. Now, the OS may
		** also be allocating memory, and we need to take into
		** account the possibility that the initial memory we
		** acquire isn't where we expect it to be, so we need
		** to get the appropriate number of bytes to get the 
		** next page boundary - of course while we do this the
		** OS could get in first; so if the initial memory
		** request didn't return memory where we expected, we
		** do additional sbrk() calls, until we get two adjacent
		** memory blocks, then we know we have memory up to one
		** of our page boundaries.
                **
		** FIXME:
                ** Not very elegant I know, as it may waste (lots) of
		** memory; but what else can we do ? We can't prevent the
		** OS from getting memory (who knows what OS functions
		** get memory?) between us finding out the current
		** memory limit and then use a BRK() to get the memory we
		** need. Really we need to re-write the ME package.
		**
		** FIXED? - hanje04 18-Nov-2004
		** Added ME_alloc_aligned which should prevent the problem
		** described here. Define xCL_SYS_PAGE_MAP to try.
                */
		PTR sb;
                SIZE_TYPE bytes_to_alloc = pages * ME_MPAGESIZE;
                SIZE_TYPE delta;
                PTR       req;
                PTR       extra;

# ifdef OS_THREADS_USED
                CS_synch_lock(&MEbrk_mutex);
# endif /* OS_THREADS_USED */

                for (req = MElimit, delta = 0;
                     (sb = sbrk( bytes_to_alloc + delta)) != req;
                     delta = - ( (SIZE_TYPE)req % ME_MPAGESIZE))
                {
                   if (sb == SBRK_FAIL) break;

		   /* To get here mean that the OS has extended the
		   ** memory used by the process.
		   **
		   ** Calculate how much memory we need to take us to
		   ** the end of the current ME page.
		   */
                   delta = ME_MPAGESIZE - (( sb - req) % ME_MPAGESIZE);

                   if ( (extra = sbrk( delta ) ) == SBRK_FAIL )
                   {
                      sb = SBRK_FAIL;
                      break;
                   }

                   if (extra == sb + bytes_to_alloc)
		   {
                      /* Phew! the previous memory allocation, came directly
                      ** after the chunk of memory we got for the pages
                      ** required by the calling function, so now we have
                      ** memory upto a ME page boundary.
                      */
		      break;
		   } 

                   /* The OS has done it to us again, its sneaked in before
                   ** we could get the memory we needed to set us at a page
                   ** boundary. So calculate the address we hope sbrk()
                   ** returns.
                   */ 
                   req = extra + delta;

                   /* We now have a piece of memory which straddles a  page
                   ** so delata will become negative, as we don't need quite
                   ** so many bytes.
                   */
                }

                if (sb == SBRK_FAIL)
                {
                   SETCLERR(err_code, 0, ER_brk);
                   status = ME_OUT_OF_MEM;
                }
                else
                {
                   /* We finally got the memory we need. sb is the
                   ** address returned by the last sbrk call, so add
                   ** delta to it to position us at a ME page boundary.
                   */
                   sb += delta;

                   if ( (pages_by_malloc = (sb - MElimit) / ME_MPAGESIZE))
                   {
                      /* For all the pages we had to skip over because of OS
                      ** memory expansion, mark the pages as busy.
                      */
                      MEsetalloc(MEendtab, pages_by_malloc);
                   }

                   pageno = MEendtab + pages_by_malloc;

                   if (!(flag & ME_ADDR_SPEC))
                   {
                      *memory = MEbase + ME_MPAGESIZE * pageno;
                   }

                   MElimit = (pageno + pages) * ME_MPAGESIZE + MEbase;
             
                   MEendtab = pageno + pages;
                }

# ifdef OS_THREADS_USED
                CS_synch_unlock(&MEbrk_mutex);
# endif /* OS_THREADS_USED */
            }
            if (!status)
            {
                MEsetalloc(pageno, pages);
            }
        }
    }
# ifdef OS_THREADS_USED
    CS_synch_unlock(&MEalloctab_mutex);
# endif /* OS_THREADS_USED */

    return(status);
}
# endif /* not xCL_SYS_PAGE_MAP */
/*{
** Name: MEfree_pages()	- Free pages back to OS
**
** Description:
**	Routine will, if possible, return pages to OS.  Memory to be freed
**	must have been allocated using the MEget_pages() call.  Memory to
**	be freed must match the memory allocated by a corresponding
**	MEget_pages call, since on some platforms the ME routines ultimately
**	call malloc (or equivalent) and free.
**	(A possible exception exists for shared-memory pages, which are
**	handled separately.)
**
**	Note: The memory might not be freed immediately.  On Unix, memory
**	must be contiguous and freed in LIFO order.  Thus the memory will not
**	returned to the OS until all memory above it is also freed.
**	However, new memory allocations will prefer to use lower memory.
**
**	Shared Memory Note: Shared memory often must be released in an
**	all-or-nothing manner so again the actual release may be postponed.
**
** Inputs:
**      address                         address of the 1st allocated block of 
**					memory to be freed.
**      num_of_pages                    number of pages to be freed.
**
** Outputs:
**      err_code                        system dependent error code.
**
**	Returns:
**	    OK				Free operation succeeded.
**	    ME_BAD_PARAM		Parameter is invalid
**	    ME_NOT_ALLOCATED		Pages to be freed were not all
**						allocated (warning)
**	    ME_OUT_OF_MEM		should never happen (OS
**						doesn't want memory back)
**	    ME_BAD_ADVISE		call was made during ME_USER_ALLOC.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-jan-87 (mmm)
**          proposed
**	23-feb-88 (anton)
**	    combined shared memory spec and first cut at implementing.
**	15-aug-89 (fls-sequent)
**	    Treat MEadvice as a flag word instead of an integer.
**      21-Jan-91 (jkb)
**          Change "if( !(MEadvice & ME_INGRES_ALLOC) )" to
**          "if( MEadvice == ME_USER_ALLOC )" so it works with
**          ME_INGRES_THREAD_ALLOC as well as ME_INGRES_ALLOC
**	    Change mct specific ifdefs to #ifdef MCT rather than listing each
**	    platform
**	23-aug-1993 (bryanp)
**	    On HP-UX 9.0, and perhaps on other machines, MEget_pages doesn't
**		return memory aligned to any special boundary. In particular,
**		it doesn't return memory aligned to ME_MPAGESIZE boundaries.
**		Therefore, I've removed the alignment check in MEfree_pages,
**		which was causing MEfree_pages to reject perfectly valid
**		memory addresses with ME_BAD_PARAM.
**	20-sep-93 (kwatts)
**	    Integrated 6.4 fix from gordonw:
**         Don't assume that a memory block to be freed is below the MEbase.
**         On some systems, DEC/MIPS, RS/6000, the shared memory is not
**         allocated at high memory address space.
**         This was fixed in 6.3/02p but someone put it back in 6.4.
**      24-May-2000 (hanal04) Bug 98820 INGSRV 1094
**          We must mutex protect references and changes made to the
**          MEalloctab by holding the MEalloctab_mutex.
*/
STATUS
MEfree_pages(
	PTR		address,
	SIZE_TYPE	num_of_pages,
	CL_ERR_DESC	*err_code)
{
    STATUS		status = OK;
    SIZE_TYPE		pageno;
    char		*lower, *upper;
    
    CL_CLEAR_ERR( err_code );
    ME_freecount++;
    if ( MEadvice == ME_USER_ALLOC)
	return(ME_BAD_ADVICE);

# ifdef xCL_SYS_PAGE_MAP
    lower = (char *)address;
    upper = lower + ME_MPAGESIZE * num_of_pages;
    if (ME_find_seg(lower, upper, &ME_segpool))
    {
	/* it's a shared memory */
	if ((status = MEseg_free(&address, &num_of_pages, err_code) == FAIL)
	    || num_of_pages == 0)
	{
	    return(status);
	}
    }
    else
    {
        free(address);
    }
# else /* xCL_SYS_PAGE_MAP */
    /* give the shared memory system first shot at freeing memory */
    /* This may change the range of memory in the break region being freeed */
    /* In fact, no memory may be freeable in the break region so we
       may return right here with OK */
    if ((status = MEseg_free(&address, &num_of_pages, err_code) == FAIL)
        || num_of_pages == 0)
    {
        return(status);
    }
 
    pageno = ((char *)address - MEbase) / ME_MPAGESIZE;

# ifdef OS_THREADS_USED
    CS_synch_lock(&MEalloctab_mutex);
# endif /* OS_THREADS_USED */
    /* check that pages were allocated */
    if (MEisalloc(pageno, num_of_pages, TRUE))
        status = ME_NOT_ALLOCATED;
 
    MEclearalloc(pageno, num_of_pages);
# ifdef OS_THREADS_USED
    CS_synch_unlock(&MEalloctab_mutex);
# endif /* OS_THREADS_USED */

    /* try to reduce the break region */
# endif /* xCL_SYS_PAGE_MAP */

    return(status);
}

/*{
** Name: MEprot_pages()	- Set protection attributes on a page range
**
** Description:
**	For host OS' which support it, modify access attributes on a page
**	range.  Possible uses of this routine include stack overflow warning,
**	facility and/or session verification traces of valid memory use, 
**	protection from dynamically linked user code in INGRES proprietary
**	processes, ...
**
**	Mainline (user accessible) functionality can not depend on this
**	interface; improved error handling, enhanced debugging capabilities,
**	etc. are the intended clients.  Clients are not expected to ifdef
**	use of this interface.
**
**	Setting attributes on a shared segment only affects the client
**	that calls MEprot_pages.
**
** Inputs:
**	addr				start address of page range
**	pages				number of pages in range
**	protection			protections requested:
**			ME_PROT_READ
**			ME_PROT_WRITE
**			ME_PROT_EXECUTE
**
** Outputs:
**	return status
**
**	Returns:
**	    OK				Protections could be set.
**	    ME_NOT_ALLOCATED		Pages are not allocated.
**	    ME_NOT_SUPPORTED		OS can not comply with request
**	    ME_BAD_ADVISE		call was made during ME_USER_ALLOC.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	Memory accesses which violate set permissions will generate
**	OS specific exceptions.
**
** History:
**      03-may-88 (anton)
**          Created.
**	15-aug-89 (fls-sequent)
**	    Treat MEadvice as a flag word instead of an integer.
**      21-Jan-91 (jkb)
**          Change "if( !(MEadvice & ME_INGRES_ALLOC) )" to
**          "if( MEadvice == ME_USER_ALLOC )" so it works with
**          ME_INGRES_THREAD_ALLOC as well as ME_INGRES_ALLOC
*/
STATUS
MEprot_pages(
	PTR		addr,
	SIZE_TYPE	pages,
	i4		protection)
{
    if ( MEadvice == ME_USER_ALLOC)
	return(ME_BAD_ADVICE);
    return(ME_NOT_SUPPORTED);
}


void
ME_mo_reg()
{
    (void) MOclassdef( MAXI2, ME_page_classes );
}


static STATUS
MO_get_used_pages(i4 offset,
		  i4  objsize,
		  PTR object,
		  i4  luserbuf,
		  char *userbuf)
{
    return( MOlongout( MO_VALUE_TRUNCATED,
		      (i8)((MElimit - MEbase) / ME_MPAGESIZE),
		      luserbuf,
		      userbuf ));
}

static STATUS
MO_get_used_bytes(i4 offset,
		  i4  objsize,
		  PTR object,
		  i4  luserbuf,
		  char *userbuf)
{
    return( MOlongout( MO_VALUE_TRUNCATED,
		      (i8)(MElimit - MEbase),
		      luserbuf,
		      userbuf ));
}


#ifdef xCL_SYS_PAGE_MAP

/*{
** Name: ME_alloc_brk()  (for SYS_PAGE_MAP platforms)
**
** Description:
**	Obtain aligned memory pages directly from the OS. This function
**	is intended to elimitate the problems with using brk() and sbrk()
**	to try and extend the dataspace to a page aligned boundary. (See
**	comments in the other ME_alloc_brk() for details)
**
**	This implementation uses memalign (a malloc-class routine) to
**	allocate memory aligned on the ME-page address boundary.
**
**	Note that the caller has probably obtained the MEpage_mutex
**	lock, which is probably unnecessary for most if not all mallocs
**	which are usable here.  Probably ought to fix the double
**	locking someday.
** 
** Input:
**	flag		Currently ignored except for ME_ADDR_SPEC which
**			will return ME_ILLEGAL_USAGE.
**	pages		Number of pages required
**
** Output:
**	memory		Addres of allocated pages (PTR **)
**	allocated_pages Number of pages actually allocated
**	err_code	System depended error code
**
** Return:
**	OK
**	ME_BAD_PARAM
**	ME_ILLEGAL_USAGE
**	ME_OUT_OF_MEM
**
** History:
**	18-Nov-2004 (hanje04)
**	    Created.
**	28-Dec-2004 (hweho01)
**         OS routine memalign() is not available on AIX; avoid linker error, 
**         excludes ME_alloc_memalign() for rs4_us5 platform.
**	19-Jan-2005 (schka24)
**	    Rename, since only one of the two alloc_brk-class routines
**	    can be relevant based on the xCL_SYS_PAGE_MAP definition.
**	    Fix parameter types to match the other alloc_brk.
**	    Make sure status is returned properly on non-linux failure.
**	18-Jun-2005 (hanje04)
**	    Add support for Mac OSX. Use valloc() to allocate aligned memory
**	    as memalign() doesn't exist and use of brk()/sbrk() is strongly
**	    discouraged on Mac OSX. (see man page for brk())
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
}*/

/* FIXME why are aix platforms SYS_PAGE_MAP then?  or are they? */
#if !defined(any_aix)

/* Can't have shared memory coming from malloc'ed memory */
#ifdef ME_SEG_IN_BREAK
whoops FIXME! ME_SEG_IN_BREAK and xCL_SYS_PAGE_MAP both defined!
#endif


STATUS
ME_alloc_brk(
	i4		flag,
	SIZE_TYPE	pages,
	PTR		*memory,
	SIZE_TYPE	*allocated_pages,
	CL_ERR_DESC	*err_code)
{
    STATUS	status;
    SIZE_TYPE	pgsize;
    SIZE_TYPE	pageno;

    pgsize = ME_MPAGESIZE;
    status = OK;
    CL_CLEAR_ERR( err_code );

    ME_getcount++;

    if ( pages == 0 )
    {
	status = ME_BAD_PARAM;
    }

    if ( flag & ME_ADDR_SPEC )
    {
	/* cannot allocate at the specified
	   address when not using brk() */
	status = ME_ILLEGAL_USAGE;
    }

    /* Request memory block from OS aligned on pgsize */
    if (!status)
    {
#ifdef LNX
	/* Use posix_memalign() on Linux to guarantee memory can be free()'d */
	status = posix_memalign( (void **) memory, pgsize,
					(SIZE_TYPE)(pages * pgsize));
# elif defined(OSX)
	*memory=(PTR)valloc(pages * pgsize);
	 if (*memory == NULL) status = ME_OUT_OF_MEM;
#else
	*memory = (PTR)memalign(pgsize, pages * pgsize);
	if (*memory == NULL) status = ME_OUT_OF_MEM;
#endif
	if (status)
	{
            SETCLERR(err_code, 0, ER_memalign);
	    status = ME_OUT_OF_MEM;
	    *allocated_pages = 0;
	}
	else
	{
	    *allocated_pages = pages;
	}
    }

    return(status);
}

# endif /* rs4_us5 */
#endif /* xCL_SYS_PAGE_MAP */
