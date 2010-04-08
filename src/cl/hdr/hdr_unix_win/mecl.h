/*
** Copyright (c) 2009 Ingres Corporation
**
*/

/*
** ME.h
**	defines global symbols needed by users of ME routines
**
**	History:
**	feb-83	-- (greg) written
**	17-Oct-1988 (daveb)
**		Take out sun special ifdef.   Should not be needed.
**	02-May-89 (GordonW)
**		added include <mecopy.h> for MECOPY macro calls
**	8-may-89 (daveb)
**		mecopy.h shouldn't exist.  define MECOPY macros
**		in line here.  Also remove redundant inclusion of
**		compat.h here.  It is an error for someone to include
**		me.h without having included compat.h first.
**
**		Note that NOT_ALIGNED macro is probably "illegal" in
**		INGRES Release 6, having not been speced or approved.
**	10-may-89 (daveb)
**		Two bugs in non-BYTE_ALIGN variants of the MECOPY macros.
**		VAR copied wrong bytes, CONST copied a few in the wrong
**		direction, trashing the source buffer.  oops.
**	10-may-89 (daveb)
**		Sometimes people don't hand MEcopy things that are really
**		char *'s.  This is officially illegal (You must correctly
**		cast args to a function), but sufficiently painful to
**		correct that the MECOPY macros need to coerce all args
**		to char * to be sure they will work correctly.  Bad things
**		happen when the macro does "s+4" when s _isn't_ a char *!
**
**		Also, don't use macros of the form _X, since that is 
**		perilously close to the symbols defined in the UNIX 
**		<ctype.h> and the INGRES <cm.h>:  rename _M to M1 and
**		_M8 to M8.
**
**	06-feb-89 (jeff/daveb)
**		Changed names of page protection flags to conform with 
**		coding standard (namespace conflict).
**      28-feb-1989 (rogerk)
**		Added ME_NOTPERM_MASK flag for shared memory allocations.
**      15-aug-1989 (fls-sequent)
**		Added ME_INGRES_SHMEM_ALLOC Meadvise flag for MCT.
**	16-feb-1990 (mikem)
**		Added ME_NOT_ALIGNED_MACRO() to be used to determine if a ptr
**	        is aligned to the worst case boundary for the machine.
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Defined shortened versions of MECOPY_CONST_MACRO and
**		MECOPY_VAR_MACRO for sco_us5;
**		Comment out strings following #endif;
**		Change incorrect symbols ME_READ_PROT, ME_WRITE_PROT and
**		ME_EXECUTE_PROT to the correct names: ME_PROT_READ, etc;
**		Add perf macros for sun_u42.
**	4-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		bug #9185 - added new cl internal error return for getting
**      	system specific error into the error log through the
**		CL_ERR_DESC. The new error added is
**		"E_CL1226_ME_SHARED_ALLOC_FAILURE".
**      21-aug-1990 (jkb)
**          Change ME_MAX_ALLOC from 1 <<12 to 1 << 15 to allow memory
**          allocation of up to 256mb rather than just 32mb
**      10-dec-1990 (mikem)
**          Change ME_MAX_ALLOC from 1 <<15 to 1 << 16 to allow memory
**          allocation of up to 512mb rather than just 256mb.  Some server
**	    class sun machines can handle more than 512mb of real memory now.
**	10-Jan-91 (anton)
**	    Renamed ME_INGRES_SHMEM_ALLOC to ME_INGRES_THREAD_ALLOC
**      25-sep-1991 (mikem) integrated following change: 12-aug-1991 (mikem)
**	    sir #39263
**          Change ME_MAX_ALLOC from 1 <<16 to 1 << 17 to allow memory
**          allocation of up to 1 gig rather than 512mb.  Some new machines
**	    now being delivered and to be delivered in 6.4's lifetime will be
**	    able to support 1 gigabyte of address space.
**	25-sep-1991 (mikem) integrated following change: 11-sept-91 (vijay)
**	    Performance changes. Using library routines for ME_fill
**	    etc. for hp8_us5, ris_us5 and ds3_ulx.
**	30-oct-1991 (bryanp)
**	    Added ME_SLAVEMAPPED_MASK so that DI can keep track of which
**	    segments have been mapped into the slaves.
**	14-apr-1992 (jnash) 
**	    Add new MEget_pages error messages: ME_NO_SHMEM_LOCK, ME_NO_MLOCK   
**	    ME_LOCK_FAIL and ME_LCKPAG_FAIL.  The first three are for unix, the
**	    last is vms.
**      28-nov-1992 (terjeb)
**	    Include <memory.h> for hp8_us5 to obtain prototype definitions for
**	    bcopy(), memset() and memcmp().
**	30-nov-92 (pearl)
**	    Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**	    declarations.
**	21-jan-93 (pearl)
**	    Add non-prototyped declarations for MEfree() and MEsize().
**	08-feb-93 (pearl)
**	    Add a few declarations that were missing.
**	24-mar-93 (smc)
**          Piggy backed hp8_us5 change for axp_osf to use library routines
**          for ME_fill etc.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	21-jun-93 (mikem)
**	    Added su4_us5 change to use library routines instead of ME routines
**	    where appropriate (included "metune" output from tests run on a 
**	    sparc2000 machine).
**	6-Jul-1993 (daveb)
**	    Move hp8_us5 include inside the gate, and also include <strings.h>
**	    so bcopy() would get defined.  Also ifdef out bogus FUNC_EXTERNs
**	    for system routines at the end.  You should include the system
**	    header to get those defines, or at the least include them in 
**	9-aug-93 (robf)
**          Add su4_cmw
**	    very system-specific ifdefs if they aren't in a system header.
**	11-aug-93 (ed)
**	    removed MEreqmem extern since it is in me.h
**	23-aug-1993 (bryanp)
**	    Moved ME_ALIGN into this file from <meprivate.h>, in the process
**		fixing ME_ALIGN to have its own unique value, rather than
**		(accidentally) having the same value as ME_NOTPERM_MASK.
**      23-aug-1993 (mikem)
**	    Change ME_MAX_ALLOC from 1 <<17 to 1 << 18 to allow memory
**	    allocation of up to 2 gig rather than 1 gig.  Some new machines
**	    now being delivered and to be delivered in 6.5's lifetime will be
**	    able to support 2 gigabyte of address space.
**	  
**	    Also included <memory.h> on su4_us5 to pick up system prototypes
**	    for mem*() routines.
**	16-sep-93 (vijay)
**		Include string.h for AIX to get inline code for memcpy etc.
**		Use memcpye etc which are cheaper on AIX. Include systypes.h.
**      11-feb-94 (ajc)
**              Added hp8_bls specific entries based on hp8*
**      10-feb-95 (chech02)
**              Added rs4_us5 for AIX 4.1.
**	23-may-95 (fanra01)
**		Desktop porting changes.
**	11-aug-95 (rambe99)
**              Moved MEcmp, MEcopy and MEfill defines for sqs_ptx here to
**		conform to the same method used by other platforms.  
**	08-nov-95 (sweeney)
**		re-integrate morayf's change inadvertently backed
**		out by the previous (rambe99) sequent submission.
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with su4_us5.
**              Reason, to port Solaris for Intel.
**      17-jul-95 (morayf)
**		Added sos_us5 where odt_us5 used.
**	17-oct-95 (emmag)
**		Added int_wnt and axp_wnt to save on a function call.
**	08-nov-1995 (canor01)
**		Added ME_FASTPATH_ALLOC for fastpath API.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT and
**          using data from a DLL in code built for static libraries.
**      16-Apr-96 (fanra01)
**          Changed definition of ME_MAX_ALLOC from 1 << 15 to 1 << 18.  The
**          definition of ME_MPAGESIZE remains at 4096.  This gives a possible
**          1Gbyte memory area.
**      30-May-96 (stial01)
**          Added ME_TUXEDO_ALLOC (B75073)
**	03-jun-1996 (canor01)
**	    Add definitions for thread-local storage functions.
**	22-nov-1996 (canor01)
**	    Move definitions of CL-specific thread-local storage functions
**	    to meprivate.h to avoid conflicts with similar functions in GL.
**	12-dec-1996 (donc)
**	    Cope with the VERY unfortunate choice of the INT and FLOAT names
**	    (they conflict mith Microsoft definitions) by ifeffing them out
**	    when OpenROAD is being compiled.
**      21-may-1997 (musro02)
**          Moved MEcmp, MEcopy and MEfill defines here for sqs_ptx to conform
**          to the same method used by other platforms.
**      01-Oct-97 (fanra01)
**          Add MEcopylng macro for NT.
**	02-dec-1997 (canor01)
**	    Update MEcopylng macros for Unix.
**	04-dec-1997 (canor01)
**	    Changed ME_MAX_ALLOC and ME_MPAGESIZE for Alpha, which has
**	    8K memory pages.
**	10-may-1999 (walro03)
**	    Remove obsolete version string odt_us5.
**	07-Sep-1999 (allmi01/bonro01)
**	    Added dgi_us5 to list of platforms that use memcpy.
**	26-Jan-2000 (hanch04)
**	    Redefined CC to CCHAR
**	28-Jan-2000 (hweho01)
**	    Added support for AIX 64-bit platform (ris_u64). 
**      14-Jul-2000 (hanje04)
**          The shared library suffix for int_lnx is ".so.2.0".
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-jan-2001 (somsa01)
**	    Added ibm_lnx defines fro S/390 Linux.
**	24-May-2001 (musro02)
**	    Added nc4_us5 defines to platforms defining MEcopy, etc.
**      06-june-2001 (legru01)
**          Added sos_us5 to platforms defining MEcopy. 
**	5-Jun-2001 (bonro01)
**	    Added rmx_us5 to platforms that use system library
**	    calls for MEcopy, MEcopylng, MEfill, MEfilllng & MEcmp
**          This was prompted by a bug found during ICE testing
**	    where buffers greater than 32k were not transferred back
**	    to the browser correctly. 
**      24-May-2001 (linke01)
**          Added usl_us5 defines to platforms defining MECOPY_WRAPPED_MEMCPY
**          to fix the truncated HTML page problem in ICE demo, (this is part
**          of change#446906. 
**      26-Jun-2001 (linke01)
**          when submitted change to piccolo, it was propagated incorrectly, 
**          re-submit the change. (this is to modify change#451142).
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	06-dec-1999 (somsa01)
**	    NT does not have strings.h .
**	16-dec-1999 (somsa01)
**	    Include systypes.h before stdlib.h to prevent compile problems
**	    on platforms such as HP.
**	26-Jan-2000 (hanch04)
**	    Redefined CC to CCHAR
**      01-Feb-2000 (stial01)
**          Added back M1 macro, added MECOPY_CONST_#_MACROs for 2,4 byte copy
**      24-feb-1999 (hanch04)
**          Added stdlib.h for memalign
**	08-feb-2001 (somsa01)
**	    Porting changes for i64_win.
**      23-Oct-2002 (hanch04)
**          Make pagesize variables of type long (L)
**	19-mar-2003 (somsa01)
**	    Removed osolete define ME_MAX_TLS_KEYS.
**	25-mar-2003 (somsa01)
**	    It turns out that ME_MAX_TLS_KEYS is still needed on Windows.
**	    Re-added the define.
**	31-Mar-2003 (bonro01)
**	    Change MEcopy() to use memmove() instead of memcpy() on i64_hpu
**	    The CL spec specifies that overlapping buffers are allowed if the
**	    destination address is lower than the source address, but memcpy()
**	    on i64_hpu causes memory overlays if the buffers overlap in any
**	    direction.
**	    This problem was discovered on i64_hpu because of code in gcarw.c
**	    that uses MEcopy() for overlapping buffers.
**      03-Apr-2003 (hweho01)
**          Defined MEcopy to OS memmove for rs4_us5 (AIX) platform,     
**          because memmove performs the copying nondestructively if the
**          areas of the Source and Target parameters overlap.
**	24-sep-2004 (somsa01)
**	    To avoid unaligned access messages on Linux Itanium, use
**	    memmove() instead of memcpy().
**	19-Jan-2005 (schka24)
**	    Comment updates re page based allocation.  Given the recent
**	    changes (on some platforms) to avoid manual page allocation
**	    via sbrk(), it is no longer true that we track pages
**	    independently.  Rather, free's must match get's, just like
**	    the usual situation with malloc and free (which is what is
**	    being used under the covers.)
**	16-Mar-2005 (bonro01)
**	    To avoid unaligned access messages on Solaris a64_sol, use
**	    memmove() instead of memcpy().
**	04-May-2005 (hanje04)
**	    Use sysconf(_SC_PAGESIZE) to determine ME_MPAGESIZE on Mac
**	    OS X so we can use valloc() to request aligned memory.
**      09-mar-2006 (horda03)
**          Defining MEfilllng which is missing on windows.
**       5-Feb-2006 (hanal04) Bug 115468
**          Overlapping buffers should not be used on i64_hpu memcpy().
**          Use memmove() when defining MECOPY_VAR_MACRO and
**          MECOPY_CONST_MACRO.
**	04-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with OSX and add support for Intel OSX.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	23-Sep-2009 (wonst02) SIR 121201
** 	    Change ME_TLS_KEY from i4 to DWORD for NT_GENERIC,
*/


# ifndef	ME_included		/* don't include twice */
# define	ME_included

#include <systypes.h>
#include <stdlib.h>
#include <memory.h>		/* memcpy etc. */
#ifndef NT_GENERIC
#include <strings.h>		/* bcopy etc. */
#endif /* NT_GENERIC */
#include <string.h>

/*
** Arguments flags to MEadvise().
*/
# define ME_USER_ALLOC		0
# define ME_INGRES_ALLOC	1
# define ME_INGRES_THREAD_ALLOC	2
# define ME_FASTPATH_ALLOC	3
# define ME_TUXEDO_ALLOC        4

/*
**	used by MEdump routine
*/

# define	ME_D_ALLOC		1
# define	ME_D_FREE		2
# define	ME_D_BOTH		3

/*
** NOT_ALIGNED_MACRO returns NON-ZERO if 'ptr' is not aligned according to
** the requirements of the machine involved.  ME_align_table is defined in
** the file mealign.c and takes it's values from the defines in targetsys.h
** for [I2,I4,F4,F8]_ALIGN.
**
** We are depending on the values for INT and FLOAT to remain unchanged.
*/
# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF i4	ME_align_table[];
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF i4	ME_align_table[];
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

#ifndef wgl_wnt
# ifndef INT
# define	INT	30
# define	FLOAT	31
# endif	 /* INT */
#endif

# define NOT_ALIGNED_MACRO(typ, len, ptr) ((i4)ptr &	\
					   ME_align_table[(typ - INT) +	\
							  (len >> 2)])


/*}
** Name: ME_NOT_ALIGNED_MACRO - is a ptr aligned to worst cast boundary.
**
** Description:
**	ME_NOT_ALIGNED_MACRO() is to be used to determine if a ptr
**      is aligned to the worst case boundary for the machine.  This is the
**	standard test used throughout the backend when the datatype of the
**	item in question is unknown.  More work could be done in this area
**	to save unecessary alignment work with support from adf since all most
**	of the code usually knows is the datatype id of the item in question.
**
**	This macro will always evaluate to FALSE if the machine is not
**	a BYTE_ALIGN architecture.  On some non-BYTE_ALIGN machines 
**	ALIGN_RESTRICT has been defined to be long rather than char.  What
**	is really desired is some version of "ALIGN_RESTRICT_DESIRED" and
**	"ALIGN_RESTRICT_REQUIRED" values so that the code can align data
**	for performance if it has to move the data anyway, but can decide
**	not do some work on non-BYTE_ALIGN machines.
**
** History:
**     16-feb-1990 (mikem)
**          Created.
**     10-sep-1993 (smc)
**          Defined out ME_NOT_ALIGNED_MACRO for linting, to save getting
**          a bzillion pointer truncation messages on machines where
**          ptr > i4. 
**	28-Dec-2009 (kschendel) SIR 122974
**	    Use SCALARP to suppress warnings, a la me-align-macro.
*/
# if defined(BYTE_ALIGN) && !defined(LINT)
# define  ME_NOT_ALIGNED_MACRO(ptr) (((SCALARP) ptr) & (sizeof(ALIGN_RESTRICT) - 1))
# else
# define  ME_NOT_ALIGNED_MACRO(ptr) (0)
# endif

/*
**	ME page allocation section
**
**	This section deals with page based memory allocation from the OS.
*/
 
/**CL_SPEC
** Name: ME.H - Page based dynamic memory allocator
**
** Description:
**      This file contains define's and datatype definitions for a
**	page based memory allocator.
**
** Specification:
**
**  Description:
**
**	The ME module provides dynamic memory allocation and deallocation to
**	a client.  The allocation unit is a ME_MPAGESIZE page, and all
**	allocation and deallocation requests are in units of pages.  
**	Deallocation of pages must exactly match allocation of pages,
**	in the traditional manner of malloc() and free().  (Indeed, on
**	some platforms -- those which define xCL_SYS_PAGE_MAP -- the
**	underlying memory allocation mechanism is indeed malloc(), or
**	an equivalent, and free().)
**
**	Shared memory regions are also available via ME when using
**	MEadvise(ME_INGRES_ALLOC).  Current clients of shared memory
**	consist of the UNIX logging and locking systems, and the UNIX
**	DBMS server slave processes.  Anticipated clients of this shared
**	memory interface include the DBMS buffer pool.
**
**  Intended Uses:
**
**	The dynamic memory usage of the dbms server requires a "page" based
**	allocation and deallocation scheme.  This "page" based system is best
**	implemented below the CL, rather than above based on a byte abstraction.
**      
**    Assumptions:
**
**	The normal usage of the ME page requests will be relatively infrequent 
**	requests for large numbers of pages.  Memory in general will be 
**	available back to the system at session termination time, and in 
**	normal operation, memory usage will be LIFO in nature (this allows 
**	optimization on UNIX where only sbrk() can be used to free memory
**	back to the OS).  Client code must not be dependent on this operation
**	but an implementation may be optimized for it.
**
**	Clients of the shared memory subsystem should assume that the shared
**	memory resource is limited, and is not available at all on some systems.
**	Mainline designs based on the use of shared memory must include
**	alternate methods of performing the same task; possibly at some cost
**	in performance (ie. the	DBMS can not assume that there is only one
**	buffer manager per installation--though it can take advantage of
**	this optimization).  Also, designs which utilize shared memory are
**	expected to fully specify how the segments are installed and
**	deinstalled (this should include what happens should the installing
**	process call MEsmdestroy(), or abnormally terminate while associated
**	processes are attached.)  Such installation and cleanup procedures
**	are likely to be OS specific much like the overall install scripts.
**
**	The number of different shared memory segments that can be attached
**	per process is system dependent.  For this reason users of the shared
**	memory subsystem should assume a very limited number of shared memory
**	segments can be created, and should optimize use based on this.  Clients
**	should allocate a single shared memory segment, and then split the
**	segment into smaller pieces for their use.
**
**	Product designs which utilize shared memory are expected to fully
**	specify how the segments are installed and deinstalled (this should
**	include what happens should the installing process call MEsmdestroy(),
**	or abnormally terminate, while associated processes are attached.)
**	Installation and cleanup procedures are likely to be OS specific,
**	much like the overall install scripts.
**
**  Definitions/Concepts:
**
**	ME is providing a memory page management abstraction.  On some
**	systems this is not simple.  On systems such as UNIX this can
**      be accomplished by maintaining either a map of what memory is
**      allocated and what memory is meerly present or keeping a list of free
**      but present pages.  Allocation attempts would first search for
**      sufficient pages being present but not yet allocated and if that fails
**      then the system would be called to extend the memory region.  Requests
**      to free pages just mark the pages as not allocated and then check
**      if pages can be returned to the system.  On systems where we
**	manually allocate pages using sbrk(), a bit map of allocated pages
**	is kept to avoid page faults that could occur if we linked pages
**	in a list.  For reasonable page sizes and 32-bit address spaces,
**	the bit map is small:  1K per 64M of address space for 8K pages.
**
**	On some platforms, rather than maintaining a by-hand bitmap of
**	pages, we call the system provided malloc() (or equivalent) and
**	free().  This eliminates the ability to return memory to the OS
**	via sbrk(), unless free() does so;  but it turns out that
**	releasing address space can hardly ever happen anyway, so this
**	is no great loss.  Plus, sbrk() has been unreliable on some
**	platforms (e.g. some late RHEL 2.4 linux kernels), or suffers
**	from race conditions situations where other threads might be
**	operating via malloc/free.  (Even in the back-end, "OS" calls
**	can be C-library calls, which can use malloc and free.)
**      
**	Identification of shared memory segments however is abstracted to
**	files in a file system.  (This may be what the OS recommends--Mapped
**	files.)  If the OS doesn't prefer a file name to identify shared
**	memory, these ME routines can be written to use a file to store
**	whatever identification might be needed to get to the shared memory
**	segment.  The fact that the file contains a shared memory id is local
**	to ME as these files are not intended for use by any other code.  On
**	UNIX, mmap(2) provides the right abstraction, but System V shared
**	memory may have to be used.  System V provides a useful function
**	named ftok(3) which takes an existing file and computes a number
**	to identify a shared memory segment so the file name abstraction
**	is not too bad.  Because of these issues we have chosen to use
**	the LOCATION CL abstraction to identify shared memory.
**      
**      Another shared memory issue is whether portions of mapped shared
**      memory can be unmapped.  Many systems have an all-or-nothing attitude
**      about shared memory (the page map abstraction mentioned above
**	handles this.)
**
**      Other shared memory issues are not so easily abstracted; especially
**	when contemplates configuring shared memory segments into "unrelated"
**	user processes.  This functionality is not addressed by this proposal.
**
** History:
**	02-May-88 (anton)
**	    Clarifications.
**
**      23-Feb-88 (anton)
**          Initial definition.
**	30-oct-1991 (bryanp)
**	    Added ME_SLAVEMAPPED_MASK so that DI can keep track of which
**	    segments have been mapped into the slaves.
**	12-sep-2002 (devjo01)
**	    Add ME_LOCAL_RAD to advise page allocator to inform underlaying
**	    OS to allocate the request from memory physically on the
**	    home Resource Affinity Domain (RAD) of the calling process.
**	    This is only of concern for Non-Uniform Memory Architecture
**	    (NUMA) machines, and will be ignored on non-NUMA boxes.
**	19-Jan-2005 (schka24)
**	    Comment update.  As long as we base some platforms on malloc
**	    and free, free-page requests must mirror the corresponding
**	    get-page result.
**	08-May-2007 (wanfr01)
**	    SIR 118278 - add ME_VERBOSE_MASK to allow csinstall to
**	    display detailed error messages.
**/


/*
**  Forward and/or External function references.
*/

/*
**  Defines of constants.
*/

/*
** These used to be ifdef-ed for sun.  I don't think they need to change
** for different machines.  If it does, beware the claim that these values
** must match those of BLOCKSIZE in cl/cl/hdr/meprivate.h (daveb).
*/

/* ME_PAGESIZE * ME_MAX_ALLOC == total allocatable (currently 1024 Mbytes) */

# ifdef DESKTOP
# if defined(NT_ALPHA) || defined(NT_IA64)
# define	ME_MPAGESIZE	8192L
# define        ME_MAX_ALLOC    (1 << 17)
# else /* NT_ALPHA || NT_IA64 */
# define	ME_MPAGESIZE	4096L
# define        ME_MAX_ALLOC    (1 << 18)
# endif /* NT_ALPHA || NT_IA64 */
# elif defined(OSX)
# define	ME_MPAGESIZE	SC_PAGESIZE /* defined in bzarch.h */
# define	ME_MAX_ALLOC	(1 << 18)
# else   /* DESKTOP */
# define	ME_MPAGESIZE	8192L
# define	ME_MAX_ALLOC	(1 << 18)
# endif  /* DESKTOP */

/*
**	The following symbols are for calling MEget_pages. Please note, not
**	all of these are legal for use by mainline code. Some of these are
**	private to the Unix CL and are only usable within the CL.
*/

# define	ME_NO_MASK	0	/* None of the below flags apply */
# define	ME_MZERO_MASK	0x01	/* Zero the memory before returning */
# define	ME_MERASE_MASK	0x02	/* Erase the memory (DoD specific) */
# define	ME_IO_MASK	0x04	/* memory used for I/O */
# define	ME_SPARSE_MASK	0x08	/* memory used `sparsely' so it should
					   be pages in small chunks */
# define	ME_LOCKED_MASK	0x10	/* memory should be locked down to
					   inhibit paging -- i.e. caches */
# define	ME_SSHARED_MASK	0x20	/* memory shared across process on a
					   single machine */
# define	ME_MSHARED_MASK	0x40	/* memory shared with other machines */
# define	ME_CREATE_MASK	0x80	/* If shared memory doen't yet exist
					   it may be created.  (Usually
					   specified with ME_ZERO_MASK)
					   If the shared memory alread exists
					   it is an error to create it. */
# define	ME_ADDR_SPEC	0x0100	/* memory must be allocated at this
					   address (UNIX CL use only) */
# define        ME_NOTPERM_MASK 0x0200  /* memory segment should not be created
                                           permanent */
# define	ME_SLAVEMAPPED_MASK 0x0400
					/* this segment has been mapped
					   into the slave (UNIX CL use only) */
# define	ME_ALIGN	0x0800	/* Please align the memory returned */
					/* ME_ALIGN is for UNIX CL use only */
# define	ME_LOCAL_RAD	0x1000  /* Memory to be allocatted out of
					   physical memory local to the
					   processes Resource Affinity Domain.
					   (NUMA only, otherwise ignored)*/
# define	ME_VERBOSE_MASK	0x2000  /* Display Shared memory errors */

/*
**	The following symbols are for calling MEprot_pages.
*/
# define	ME_PROT_READ	0x4	/* pages should be readable */
# define	ME_PROT_WRITE	0x2	/* pages should be writable */
# define	ME_PROT_EXECUTE	0x1	/* pages should be executable */
# define	ME_ALL_PROT	0x7
# define	ME_NONE_PROT	0x0

/*
**	Thread-local storage
*/
# define	ME_MAX_TLS_KEYS	32	/* Maximum TLS unique keys */

# ifdef NT_GENERIC
# define	ME_TLS_KEY	DWORD	/* Thread-local storage key */
# else
# define	ME_TLS_KEY	i4	/* Thread-local storage key */
# endif

/* ME return status codes. */

# define	 ME_GOOD	(E_CL_MASK + E_ME_MASK + 0x01)
	/*  ME routine: The Status returned was good  */
# define	 ME_BD_CHAIN	(E_CL_MASK + E_ME_MASK + 0x02)
	/*  MEdump: correct parameter value must be one of ME_D_ALLOC, 
        **  ME_D_FREE, ME_D_BOTH  */
# define	 ME_BD_CMP	(E_CL_MASK + E_ME_MASK + 0x03)
	/*  MEcmp: number of bytes to compare must be > 0  */
# define	 ME_BD_COPY	(E_CL_MASK + E_ME_MASK + 0x04)
	/*  MEcopy: number of bytes to copy must be > 0  */
# define	 ME_BD_FILL	(E_CL_MASK + E_ME_MASK + 0x05)
	/*  MEfill: number of bytes to fill must be > 0  */
# define	 ME_BD_TAG	(E_CL_MASK + E_ME_MASK + 0x06)
	/*  MEt[alloc, free]: tags must be > 0  */
# define	 ME_ERR_PROGRAMMER	(E_CL_MASK + E_ME_MASK + 0x07)
	/*  MEfree: There is something wrong with the way this 
        **  routine was programmed.  Sorry.  */
# define	 ME_FREE_FIRST	(E_CL_MASK + E_ME_MASK + 0x08)
	/*  ME[t]free: can't free a block before any blocks have 
        **  been allocated  */
# define	 ME_GONE	(E_CL_MASK + E_ME_MASK + 0x09)
	/*  ME[t]alloc: system can't allocate any more memory for 
        **  this process  */
# define	 ME_NO_ALLOC	(E_CL_MASK + E_ME_MASK + 0x0A)
	/*  ME[t]alloc: request to allocate a block with zero (or less) 
        **  bytes was ignored  */
# define	 ME_NO_FREE	(E_CL_MASK + E_ME_MASK + 0x0B)
	/*  MEfree: can't find a block with this address in the free 
        **  list  */
# define	 ME_NO_TFREE	(E_CL_MASK + E_ME_MASK + 0x0C)
	/*  MEtfree: process hasn't allocated any memory with this tag  */
# define	 ME_00_PTR	(E_CL_MASK + E_ME_MASK + 0x0D)
	/*  ME[t]alloc: passed a null pointer  */
# define	 ME_00_CMP	(E_CL_MASK + E_ME_MASK + 0x0E)
	/*  MEcmp: passed a null pointer  */
# define	 ME_00_COPY	(E_CL_MASK + E_ME_MASK + 0x0F)
	/*  MEcopy: passed a null pointer  */
# define	 ME_00_DUMP	(E_CL_MASK + E_ME_MASK + 0x10)
	/*  MEdump: passed a null pointer  */
# define	 ME_00_FILL	(E_CL_MASK + E_ME_MASK + 0x11)
	/*  MEfill: passed a null pointer  */
# define	 ME_00_FREE	(E_CL_MASK + E_ME_MASK + 0x12)
	/*  MEfree: passed a null pointer  */
# define	 ME_TR_FREE	(E_CL_MASK + E_ME_MASK + 0x13)
	/*  MEfree: the memory has been corrupted  */
# define	 ME_TR_SIZE	(E_CL_MASK + E_ME_MASK + 0x14)
	/*  MEsize: the memory has been corrupted  */
# define	 ME_TR_TFREE	(E_CL_MASK + E_ME_MASK + 0x15)
	/*  MEtfree: the memory has been corrupted  */
# define	 ME_OUT_OF_RANGE	(E_CL_MASK + E_ME_MASK + 0x16)
	/*  ME routine: address is out of process's data space, 
        **  referencing will cause bus error  */
# define	 ME_BF_OUT	(E_CL_MASK + E_ME_MASK + 0x17)
	/* MEneed: 'buf' doesn't have 'nbytes' left to allocate */
# define	 ME_BF_ALIGN	(E_CL_MASK + E_ME_MASK + 0x18)
	/* MEinitbuf: 'buf' not aligned */
# define	 ME_BF_FALIGN	(E_CL_MASK + E_ME_MASK + 0x19)
	/* MEfbuf: 'buf' not aligned */
# define	 ME_BF_PARAM	(E_CL_MASK + E_ME_MASK + 0x1A)
	/* MEfbuf: 'bytes' argument must come from call to MEfbuf() */
# define	 ME_TOO_BIG	(E_CL_MASK + E_ME_MASK + 0x1B)
	/* MEreqmem/MEreqlng/ME[t][c]alloc: Object too big */
# define         ME_OUT_OF_MEM	(E_CL_MASK + E_ME_MASK + 0x1C)
	/* MEget_pages: Can not expand memory size */
# define         ME_BAD_PARAM	(E_CL_MASK + E_ME_MASK + 0x1D)
	/* Bad Paramater to ME routines */
# define         ME_ILLEGAL_USAGE (E_CL_MASK + E_ME_MASK + 0x1E)
	/* Combination of arguments to an ME routine is invalid */
# define         ME_BAD_ADVICE	(E_CL_MASK + E_ME_MASK + 0x1F)
	/* An ME routine will not function with the given MEadvice) */
# define         ME_ALREADY_EXISTS (E_CL_MASK + E_ME_MASK + 0x20)
	/* A shared memory segment to be created already exists */
# define         ME_NO_SUCH_SEGMENT (E_CL_MASK + E_ME_MASK + 0x21)
	/* A shared memory segment to be manipulated does not exist */
# define         ME_NO_SHARED	(E_CL_MASK + E_ME_MASK + 0x22)
	/* This CL does not support shared memory */
# define         ME_NOT_ALLOCATED (E_CL_MASK + E_ME_MASK + 0x23)
	/* The memory specified for some ME operation was not all allocated */
# define         ME_NOT_SUPPORTED (E_CL_MASK + E_ME_MASK + 0x24)
	/* The ME operation requested is not supported in this CL */
# define         ME_NO_PERM	(E_CL_MASK + E_ME_MASK + 0x25)
	/* Insufficient permission for some ME operation */
# define         E_CL1226_ME_SHARED_ALLOC_FAILURE (E_CL_MASK + E_ME_MASK + 0x26)

# define         ME_NO_SHMEM_LOCK  (E_CL_MASK + E_ME_MASK + 0x27)
	/* Cannot lock DMF cache on sys V shmem configuration */
# define         ME_NO_MLOCK       (E_CL_MASK + E_ME_MASK + 0x28)
	/* Cannot lock DMF cache if without mlock() function */
# define         ME_LOCK_FAIL      (E_CL_MASK + E_ME_MASK + 0x29)
	/* Cache Lock operation failed  */
# define	 ME_LCKPAG_FAIL	   (E_CL_MASK + E_ME_MASK + 0x2A)
	/* Cache lock operation failed (VMS) */


/**
** Generalized MEcopy macros to generate in-line copy code or
** a function call to MEcopy depending upon size of copy buffer.
**
** History:
**	05-Apr-89 (GordonW)
**		initial version
**	08-Apr-89 (GordonW)
**		modify for Xenix/Unix SYSv macro size limits
**	17-apr-89 (daveb)
**		Fix syntax errors in non-BYTE_ALIGN macros.  oops.
**	27-Apr-89 (GordonW)
**		moved from cl/clf/hdr to cl/hdr/hdr
**		added _MECOPY_INCLUDE test for re-enterancy
**	08-may-89 (daveb)
**		move directly into me.h now that it's approved.
**		Two bugs in non-BYTE_ALIGN variants of the MECOPY macros.
**		VAR copied wrong bytes, CONST copied a few in the wrong
**		direction, trashing the source buffer.  oops.
**	10-may-89 (daveb)
**		Sometimes people don't hand MEcopy things that are really
**		char *'s.  This is officially illegal (You must correctly
**		cast args to a function), but sufficiently painful to
**		correct that the MECOPY macros need to coerce all args
**		to char * to be sure they will work correctly.  Bad things
**		happen when the macro does "s+4" when s _isn't_ a char *!
**
**		Also, don't use macros of the form _X, since that is 
**		perilously close to the symbols defined in the UNIX 
**		<ctype.h> and the INGRES <cm.h>:  rename _M to M1 and
**		_M8 to M8.
**	23-Mar-1999 (kosma01)
**		Chose memcpy for sgi_us5
**	19-Nov-1999 (hanch04)
**		Wipe out all macros, use OS calls.
**      02-Feb-2001 (fanra01)
**              Bug 103864
**              Update MEcalloc macro to use MEreqmem.
**/

/*
** Unaligned one byte move with offset
** s = source, o = offset, d = destination
*/
# define M1(s,o,d) (((char*)(d))[o]=((char*)(s))[o])

/* 
** The following MECOPY_CONST_N_MACROs were adapted from an old 
** version of MECOPY_CONST_MACRO which was found to have no performance 
** improvements, probably due to the length tests being done in the
** original macro. These versions are coded for specific lengths.
*/
# define MECOPY_CONST_2_MACRO( s, d ) \
{ M1(s,0,d); M1(s,1,d); }

# define MECOPY_CONST_4_MACRO( s, d ) \
{ M1(s,0,d); M1(s,1,d); M1(s,2,d); M1(s,3,d); }

/* These all exists on su4_us5, su9_us5 */

#if defined(i64_hpu) || defined(any_aix) || defined(i64_lnx) || \
    defined(a64_sol)
#define MEcopy(source, n, dest)		memmove(dest, source, n)
#else
#define MEcopy(source, n, dest)		memcpy(dest, source, n)
#endif

#if defined(i64_lnx) || defined (i64_hpu)
#define MECOPY_CONST_MACRO(s,n,d)	memmove(d,s,n)
#define MECOPY_VAR_MACRO(s,n,d)		memmove(d,s,n)
#else
#define MECOPY_CONST_MACRO(s,n,d)	memcpy(d,s,n)
#define MECOPY_VAR_MACRO(s,n,d)		memcpy(d,s,n)
#endif

#define MEmemcpy			memcpy
#define MEmemccpy			memccpy
#define MEfill(n,v,s)			memset(s, v, n)
#define MEfilllng(n,v,s)		memset(s, v, n)
#define MEmemset			memset
#define MEcmp(s1,s2,n)			memcmp(s1, s2, n)
#define MEmemcmp			memcmp
#define MEmemmove			memmove

#define MEmalloc			malloc
#define MEsysfree			free
#define MEcalloc(size, status)          MEreqmem(0, (size), TRUE, (status))
#define MErealloc			realloc
#define MEmemalign			memalign

# endif				/* ME_included */
