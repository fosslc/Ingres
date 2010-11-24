/*
**Copyright (c) 2004 Ingres Corporation
*/

#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <systypes.h>
#include   <cv.h>
#include   <er.h>
#include   <di.h>
#include   <qu.h>
#include   <me.h>
#include   <nm.h>
#include   <tr.h>
#include   <cs.h>
#include   <st.h>
#include   <fdset.h>
#include   <csev.h>
#include   <errno.h>
#include   <dislave.h>
#include   <dicl.h>
#include   <cldio.h>
#include   "dilocal.h"
#include   "dilru.h"
#include   "dimo.h"

#ifdef	    xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */

#ifdef      xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif	    /* xCL_007_FILE_H_EXISTS */

#ifdef      xCL_ASYNC_IO
#ifdef dr6_us5
#include    <sys/iclaio.h>
#else
#include    <aio.h>
#endif /* dr6_us5 */
#endif      /* xCL_ASYNC_IO */

/**
**
**  Name: DILRU.C - This file contains the UNIX internal DI cache routines.
**
**  Description:
**      The DILRU.C file contains all the routines needed to perform LRU
**	caching of per session file descriptors.  Routines are called by
**	both other DI routines and other backend CL modules which may
**	need to free up a file descriptor.
**
**	  DIlru_close - close a given file, freeing a file descriptor.
**	  DIlru_free - pick victim file and close , freeing a file descriptor.
**	  DIlru_aio_free - as DIlru_free when asyncio in use.
**	  DIlru_init  - initialize hash control block
**	  DIlru_open - open file returning pointer to file descriptor.
**	  DIlru_uniq - Return uniq numeric id
**	  DIlru_release - release use of a file descriptor.
**	  DIlru_set_di_error - cause a DI error to be set.
** 
**	The DI module must support up to 256 open files (TCB's).  Since
**	most if not all UNIX boxes can not support this many open file
**	descriptors a cache of currently open files is supported by this
**	module.
**
**	At DIopen time a path and filename are passed into the function.
**	At this time an new numeric id (unique to the server) is assigned
**	to the file by DIlru_uniq.  Then DI attempts to open the file using
**	DIlru_open.  This function first attempts an open() on the file.
**	If it gets an error, then it closes a file using (DIlru_close) in
**	the descriptor cache, and retries the open until it either
**	succeeds in opening the file or runs out of files to close.
**
**	When it succeeds in opening the file it adds an entry to the
**	file descriptor cache.  The number of entries in the descriptor
**	cache is <= the number of files a single process can open.
**
**	On successive DI calls a pointer to a DI_IO struct is passed in.
**	In this structure is the unique id assigned by DIlru_uniq.  The
**	unique id is used to do a hash search of the open file descriptor
**	cache.  If the file is found then the current open file descriptor
**	is used, else the path and filename information stored in the DI_IO
**	by DIopen are used to open the file (using DIlru_close if necessary).
**
**	Jupiter Issues:
**	DI likes to do all I/O in slave processes via the CS event support.
**	However, many utilities need to use DI as well so DI will operate
**	in the main process if needed.  DIlru_open will fill in a DI_OP
**	control block if slave driven or a unix file descriptor if DI is in
**	the main process.  The DI_OP structure must be released with the
**	call DIlru_release.
**
**	Notes:
**	
**	18-dec-1992 (rmuth)
**
**	Whilst resructuring a large amount of this code i noticed the 
**	following design issues...thought i would note them here otherwise
**	they will be lost forever.
**
**	1. DIlru_file_open makes a request to a slave to open a file,
**	   if the request fails due to too many open files in that slave
**	   and the user has requested RETRY we inact the following 
**	   algorithm.
**		a. Search for a file which does not have an outstanding
**		   slave operation on it, ie fd_refcnt == 0.
**	        b. Once the above has been found see if that file is
**		   currently open in any slave. Note this search
**		   starts at slave one and moves to slave N.
**		c. If both the above have been satisfied then send a request
**		   to that slave requesting that it close the victim file
**		   and open the new file.
**
**		If no victim file can be found then return an error.
**
**	   The above algoritm unnecessarily closes files, as it does not take
**	   into account the fact that altough the slave we made the initial
**	   request to has no vacant FD's one of the other slaves may have
**	   vacant FD's.
**
**	   Hence in the future we may want to improve the above algorithm
**	   to first try the other slaves before it closes a file. Note this
**	   probably not an issues as by the time we have got to this state
**	   the system is probably pretyy sluggish hence a 'close' here or
**	   there would be no great deal.
**
**	   The reason for the above is that the code was orginally written
**	   when there we no slave processes it was all done inline.
**
**	2. The fd_refcnt is used to indicate that there is an outstanding
**	   slave operation on this file and hence we should not DIlru
**	   close the file. This flag need to be at the granularity of
**	   slave instead of all slaves, as we can close the file in 
**	   slave A if the operation is outstanding in Slave B.
**
**
** History:
**      05-mar-87 (mmm)    
**          Created new for jupiter.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	29-mar-88 (anton)
**	    DI in slave process using CS events
**	29-apr-88 (anton)
**	    more slave di bug fix.
**	08-jun-88 (anton)
**          report DI_FILENOTFOUND
**	25-aug-88 (anton)
**	    lru_close fix
**	09-sep-88 (mikem)
**	    added documentation for the use of CSreserve_event()
**	09-sep-88 (anton)
**	    allocate a DI event for each possible active session
**	30-sep-88 (anton)
**	    move the file to the top of the lru on each file operation.
**	17-oct-88 (anton)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	25-oct-88 (mikem)
**	    better error handling (still need to add roger's new er stuff)!
**	20-dec-88 (anton)
**	    use MEreqmem instead of MEalloc
**	    added logic to open the same DI file in many slave processes
**	    added II_NUM_SLAVES to allows setting the number of slave processes.
**	21-jan-89 (mikem)
**	    O_SYNC support.
**	06-feb-89 (mikem)
**	    Better support for DI CL_ERR_DESC, including initializing to zero,
**	    returning errors from DIlru_open() and DIlru_inproc_open().
**	    Added unix_open_flags member to the slave control block so we don't
**	    have to overload length anymore.
**	    Also added "#include <me.h>".
**	28-Feb-1989 (fredv)
**	    Replaced xDI_xxx by corresponding xCL_xxx defs in clconfig.h.
**	    Include <systypes.h>.
**	01-mar-89 (mikem)
**	    Print warning message to the log if you can't start up the DI
**	    slaves.
**     02-feb-90 (jkb)
**	    Use daveb's IIdio routines to encapsultate sequent/direct io magic.
**	20-Mar-90 (anton)
**	    Serveral DI fixes - LRU multiple open race condition fixed
**		- LRU queue management fixed.
**		- Check that file is not pinned when closed
**	23-Mar-90 (anton)
**	    Fix the last fix - arguments to CSi_semaphore were reversed
**	    Also, add still more xDEV_TST traceing
**	11-may-90 (blaise)
**	    Integrate changes from 61 &ug:
**		Interpret open_flags with DI_SYNC_MASK
**	31-may-90 (blaise)
**	    Integrated changes from termcl:
**		lru_close of a pinned file should not be fatal - bug #21793;
**		Be sure reference count is set to zero when allocating
**		DI_FILE_DESC's.
**      04-sep-90 (jkb)
**	    Call CSslave_init even if II_NUM_SLAVES = 0 so the control
**          block is properly initialized
**	13-Jun-90 (anton)
**	    New reentrancy support and use of CS_CONDITIONs
**      1--jan-91 (jkb)
**	    add check to see if Di_nslaves > DI_MAX_SLAVES
**      25-sep-1991 (mikem) integrated following change: 27-jul-91 (mikem)
**          bug #38872
**          A previous integration made changes to the DI_SYNC_MASK was handled
**          and the result of that change was that fsync() was enabled on all
**          DIforce() calls on platforms which define xCL_010_FSYNC_EXISTS.  The
**          changes to fix this bug affect diopen.c, dilru.c, dislave.c,
**          dislave.h, and diforce.c.  It reintegrates the old mechanism where
**          DI_SYNC_MASK is only used as a mechanism for users of DIopen() to
**          indicate that the files they have specified must be forced to disk
**          prior to return from a DIforce() call.  One of the constants
**          DI_O_OSYNC_MASK or DI_O_FSYNC_MASK is then stored in the DI_IO
**          structure member io_open_flags.  These constants are used by the
**          rest of the code to determine what kind of syncing, if any to
**          perform at open/force time.
**	30-oct-1991 (bryanp)
**	    Added code to map ME_IO_MASK segments in each slave. This allows
**	    the slaves to perform the I/O directly without requiring copies.
**	13-nov-1991 (bryanp)
**	    Don't attempt to initialize DI slaves in non-pseudo-servers.
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	12-dec-1991 (bryanp)
**	    It's not safe to depend on 'errno' when using IIdio* routines to
**	    access files. The value of errno is not dependable (for example,
**	    it may be zero even if the IIdio* call failed). Instead, the value
**	    of errno in the CL_ERR_DESC should be used, as that is the errno
**	    value which describes the reason why the call failed.
**	30-October-1992 (rmuth)
**	    - Prototype the code. 
**	    - Make DIlru_release return a status.
**	11-nov-1992 (rmuth)
**	    prototyping CS showed up error
**	30-nov-1992 (rmuth)
**	    Various
**	     - Include <cldio.h>
**	     - Add error checking.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	30-feb-1993 (rmuth)
**	    Use DI_FILE_MODE for file modes, was hardcoded to 0777 before.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jul-1993 (mikem)
**	    Added DImo_init_slv_mem() call to register the DI slave MO 
**	    interface to performance statistics.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	23-aug-1993 (bryanp)
**	    Isolated slave-mapping code into DI_lru_slmapmem for sharing.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in DIlru_free(),
**	    DIlru_open(), DIlru_dump(), DIlru_file_open(),
**	    DIlru_allocate_more_slots() and DIlru_file_close().
**	31-jan-1994 (bryanp) B56634
**	    Remove unused variables from DIlru_init; they were causing some
**		compilers to issue warning messages about "used before set".
**	17-May-1994 (daveb) 59127
**	    Fixed semaphore leaks, named sems.
**	24-May-1994 (fredv)
**	    Added missing declaration of Di_no_sticky_check.
**	01-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	19-Jun_1994 (amo ICL)
**	    Added async io amendments
**	21-sep-1995 (itb ICL)
**	    Read the gather write configuration and set Di_gather_io.
**	15-mar-96 (nick)
**	    Remove magic number 999 in DIlru_open() - we now no longer have
**	    a problem if the slave supports more than this number of open
**	    files.
**	29-oct-1996 (canor01)
**	    When running out of file descriptors, Solaris may possibly
**	    return 0 from open() instead of returning -1 and setting
**	    errno to EMFILE.  Treat the situations the same.
**	07-nov-1996 (canor01)
**	    If async_io is OFF, check II_NUM_SLAVES to see if slaves should
**	    be started.
**	20-nov-1996 (canor01)
**	    DIlru_aio_free() is dependent on xCL_ASYNC_IO.
**	19-feb-1997 (canor01)
**	    When compiled with OS_THREADS_USED, but running without MT
**	    support, default to 2 slaves.
**	21-Mar-1997 (wonst02)
**	    Support read-only access for CD-ROM installations.
** 	19-jun-1997 (wonst02)
** 	    Remove change of 21-Mar-1997...not needed for readonly database.
**	02-sep-1997 (wonst02)
**	    Added code to return DI_ACCESS for file open access error.
**	07-Oct-1997 (jenjo02)
**	  o Di_slave_open static (number of open fd's per slave) and Di_slave_cnt
**	    (number of outstanding ops per slave) were uninitialized,
**	    added ZERO_FILL.
**	  o Initialize DI_sc_sem.
**	  o DIlru_uniq() assignment of "unique" file handle now semaphore-protected
**	    using DImisc_sem to close a OS-threaded server window in which more
**	    than one file -could- be given the same handle.
**	  o DIlru_inproc_open() now puts di_file on the hash queue before releasing
**	    DImisc_sem and physically opening the file. A second thread could fail
**	    to find the file on the hash queue, assume it hadn't yet been opened,
**	    and create and open a second di_file for the same file.
**	  o DIlru_file_open() was called before fd_refcnt was incremented. If 0,
**	    another thread -could- flush that di_file from the lru cache while it
**	    was being opened (slaves only).
**	  o FD leaks fixed. DIlru_file_open() called with disl->dest_slave_no
**	    set to slave assigned to open the file (e.g., "A"). If out of FDs,
**	    an unreferenced file on the lru queue is found and then a slave which
**	    has that file opened is located, setting disl->dest_slave_no to that
**	    slave ("B") and reissuing the open. If the original file is already
**	    open on slave "B", its FD is leaked when the file is opened a second
**	    time. Code was changed to only close files opened by the original
**	    slave ("A"), thus avoiding the leak.
**	  o When awakened after waiting on DI_O_OPEN_IN_PROGRESS, the di_file
**	    -may- have been recycled and now belong to another file. Code was
**	    added to check for that possibility.
**	  o When OS threads are being used, use a shared DImisc_sem to search
**	    the di_file hash queue to improve concurrency.
**      03-Nov-1997 (hanch04)
**          Added LARGEFILE64 for files > 2gig
**	04-Nov-1997 (jenjo02)
**	    Added DI_lru_free() static function to distinguish external/internal
**	    calls to DIlru_free(). In the external case, DI_misc_sem is not
**	    held and will be acquired by DIlru_free().
**	10-Nov-1997 (jenjo02)
**	    Backed out use of SHARED semaphore to search cache. If the
**	    file is found in the cache, DIlru_search() moves it to the 
**	    top of the LRU queue. Two threads doing this at once will
**	    sometimes corrupt the LRU queue, whose stability depends on 
**	    an EXCL DI_misc_sem.
**	03-Dec-1997 (jenjo02)
**	    Backed out bullet 4 from 07-Oct-1997 change, above. The non-slave
**	    DI code is not thread-safe enough to support multiple threads sharing the
**	    same fd/di_file. In particular, the number of references to a di_file
**	    is not maintained, so one thread can easily close or LRU-out a di_file
**	    in the process of being used by a second thread, leading to bad fd number
**	    errors. 
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs to dilocal.h, GLOBALDEFs to dilru.c
**	    Replaced free, hash, and lru queues with a single queue of all
**	    allocated DI_FILE_DESC's. Opened descriptors are at the front and
**	    closed/free descriptors are at the tail. Added a pointer to the 
**	    DI_UNIQUE_ID structure so that the caller's DI_IO->io_uniq points to
**	    an DI_FILE_DESC and the DI_FILE_DESC points back to the caller's
**	    DI_IO. This is more useful than a hash queue.
**	    Substantially modified for new mutexing scheme. Instead of single
**	    threading all DI requests on DI_misc_sem, define a mutex per file
**	    descriptor, and add a mutex to the fd list.
**	    Non-slave DI_FILE_DESC are now pinned and cannot be LRU'd if they
**	    have a pin count, just like slaves.
**	04-mar-1998 (canor01)
**	    Reserve enough file descriptors to support socket connections for
**	    cs_max_sessions. This avoids a situation where all available
**	    file descriptors are in use and a new socket connection cannot
**	    be created.
**	30-mar-1998 (hanch04)
**	    Fix typo, p->prev should be p->q_prev.
**	01-sep-1998 (kinpa04)
**	    Corrected calculation for Di_lru_cache_size for the slave
**	    environment. integration of 2.0 change#436913
**	08-Oct-1998 (jenjo02)
**	    Even if there are no slaves, CSslave_init() must be called
**	    to properly initialize the server's shared memory, in
**	    particular csev must register CS_event_shutdown as a
**	    atexit so that the shared memory gets clean up when the
**	    server terminates, else the RCP's checkdead thread will think
**	    the server has died.
**	14-Oct-1998 (jenjo02)
**	    01-sep-1998 change to DI_free_remove caused a mutex deadlock
**	    because the LRU list mutex is already held and DIlru_flush()
**	    attempts to reacquire it.
**	16-Nov-1998 (jenjo02)
**	    Disallow slaves when running with OS threads. When running OS/MT
**	    threads, the IdleThread mechanism no longer exists, hence
**	    slaves won't work.
**	22-Dec-1998 (jenjo02)
**	    If DI_FD_PER_THREAD is defined (dilru.h), each thread 
**	    will open and use its own file descriptor instead of one file
**	    descriptor per file.
**	02-Feb-1999 (schte01)
**	    Modified code so that if OS_THREADS and USE_IDLE_THREAD we don't
**       automatically assume slaves. If II_THREAD_TYPE has not been set
**       to internal (CS_is_mt), do not set number of slaves. 
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	23-Jun-1999 (jenjo02)
**	    Support for writev()-based GatherWrite. This implementation
**	    has no async-io dependencies, but is supported only
**	    in OS-threaded servers.
**	24-Aug-1999 (jenjo02)
**	    Disallow GatherWrite if running Ingres threads.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-Oct-2000 hanal04 Bug 102756 INGSRV 1279
**          Prevent mutex deadlock between the DI FD %d mutex and the
**          DI LRU list mutexes by ensuring the FD mutex is not
**          requested whilst holding the LRU list mutex
**	15-Apr-2004 (fanch01)
**	    Make htb, htb_initialized global to accomodate DIrename fix
**	    to force a close on open files.
**	26-Jul-2005 (schka24)
**	    FD-per-thread fixes: safe reading of the list of FD's per
**	    DI_IO;  thread-safe updating of the open FD count (not essential
**	    but keeps the flusher in line).
**    18-Jun-2005 (hanje04)
**        Replace GLOBALDEF with GLOBALDEF for DI_sc_sem to stop
**        compiler error on MAC OSX.
**	03-Aug-2005 (toumi01)
**	    Don't issue TRdisplay re allocated fd slots if we are in the
**	    front end (e.g. cbf).
**	30-Sep-2005 (jenjo02)
**	    Change CS_SEMAPHORES to faster, less obtrusive CS_SYNCH
**	    objects.
**	    Repair false positive ALL_PINNED errors.
**	06-Oct-2005 (jenjo02)
**	    DI_FD_PER_THREAD now config param
**	    ii.$.$.$.fd_affinity:THREAD, Di_thread_affinity = TRUE,
**	    still dependent on OS_THREADS_USED.
**	28-Nov-2005 (kschendel)
**	    Make zero-buffer size a config param and init it here, or
**	    in slave init.
**	11-Feb-2009 (smeke01) b120852
**	    Remove redundant message about fd_affinity=thread now that
**	    open file descriptor limit for Linux can be increased from 
**	    1024 to 8192.
**	08-jul-2009 (smeke01) b120853
**	    Make sure that with DI_thread_affinity DI_FILE_DESCs get 
**	    closed tidily.
**	    
[@history_template@]...
**/


/*
**  Forward and/or External typedef/struct references.
*/
static STATUS DI_lru_check_init( STATUS	    *intern_status,
				CL_ERR_DESC *err_code);

static STATUS DI_lru_allocate_more_slots( 
				STATUS	*intern_status );

static STATUS DI_lru_file_open(
				i4			flags,
				register DI_SLAVE_CB	*disl,
				DI_FILE_DESC		*di_file,
				DI_OP			*op,
			        DI_IO			*f,
			        CL_ERR_DESC		*err_code,
				bool			*reserve_ev_flag,
				STATUS			*intern_status );

static STATUS DI_lru_inproc_open(
			        DI_IO			*f,
				i4			create_flag,
				DI_OP			*op,
			        CL_ERR_DESC		*err_code);

static STATUS DI_find_fd(
			        DI_IO			*f,
				i4			create,
				DI_FILE_DESC		**di_file,
				CL_ERR_DESC		*err_code);

static VOID DI_free_insert(
				DI_FILE_DESC		**di_file);

static STATUS DI_free_remove(
				DI_FILE_DESC		**di_file,
				CL_ERR_DESC		*err_code);

static STATUS DI_lru_file_close(
				DI_FILE_DESC		**di_file,
				CL_ERR_DESC		*err_code,
				STATUS			*intern_status );

static VOID DI_tidy_close( 
				DI_IO			*f);

/* externs */

# ifdef OS_THREADS_USED
GLOBALDEF i4		Di_nslaves = 0;	/* Number of DI slaves */
# else /* OS_THREADS_USED */
GLOBALDEF i4		Di_nslaves = 2;	/* Number of DI slaves */
# endif /* OS_THREADS_USED */

GLOBALDEF i4		Di_async_io =  FALSE;
GLOBALDEF i4		Di_gather_io = FALSE;
GLOBALDEF i4		Di_thread_affinity = FALSE;
GLOBALDEF i4		Di_backend = FALSE;

/* Buffer for writing zeros to disk.  By default the size is
** DI_ZERO_BUFFER_SIZE bytes, but this can be changed with the
** di_zero_bufsize config param.  Sites that work with very large
** table files and want contiguous preallocation might want a
** larger zeroing buffer.
** Di_zero_buffer will be aligned to the largest of the platform
** direct-IO requirement;  DI_RAWIO_ALIGN;  and ME_MPAGESIZE.
** It will always be at least DI_MAX_PAGESIZE in size.
*/

GLOBALDEF PTR Di_zero_buffer = NULL;
GLOBALDEF i4 Di_zero_bufsize;		/* Size of zeroing buffer */

FUNC_EXTERN     STATUS  gen_Psem();
FUNC_EXTERN     STATUS  gen_Vsem();

GLOBALDEF CSSL_CB	*Di_slave = NULL;	/* slave control block */

GLOBALDEF i4		Di_slave_cnt[DI_MAX_SLAVES] ZERO_FILL;

				/* measure of outstanding ops per slave */

GLOBALDEF CS_SEMAPHORE	DI_misc_sem;

GLOBALREF CS_SEMAPHORE	DI_sc_sem;

GLOBALDEF DI_HASH_TABLE_CB    *htb ZERO_FILL; /* hash table control block */
GLOBALDEF i4		    htb_initialized = FALSE;
					   /* Is htb properly initialized? */

#if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
GLOBALDEF bool     Di_no_sticky_check;  /* Do we check for O_SYNC when setting
                                        ** the sticky bit?
                                        */
#endif

GLOBALDEF i4		Di_fatal_err_occurred ZERO_FILL;
			/*
			** This is used to indicate that we got an error
			** in DI where we do not want to continue operation
			** as we may corrupt something as the DI internal
			** structures may be knackered hence we PCexit.
			** We use this flag instead of just calling PCexit
			** when we encounter the error to try and give
			** time for the upper layers to log some traceback
			** messages.
			*/


/*
**  Local Definitions.
*/
#define		SEM_SHARE	0
#define		SEM_EXCL	1



i4		   Di_lru_cache_size;
i4		   Di_fd_cache_size = 0;
bool		   Di_lru_clim_reached = FALSE;

i4		   Di_slave_open[DI_MAX_SLAVES] ZERO_FILL;
				/* count of open file descriptors per slave */

FUNC_EXTERN STATUS	   DI_complete();
					/* event completion routine */

/*
** Name: DIget_direct_align - Get platform direct I/O alignment requirement
**
** Description:
**	Some platforms require that buffers used for direct I/O be aligned
**	at a particular address boundary.  This routine returns that
**	boundary, which is guaranteed to be a power of two.  (If some
**	wacky platform has a non-power-of-2 alignment requirement, we'll
**	return the nearest power of 2, but such a thing seems unlikely.)
**
**	I suppose this could be a manifest constant, like DI_RAWIO_ALIGN,
**	but doing it as a routine offers a bit more flexibility.
**
**	At present (late 2009), no supported platforms have any known
**	filesystem dependencies on alignment, so there's no need to
**	pass in a file description of any sort.  If a platform were to
**	have such a dependency, we'll return the platform worst case.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns required power-of-2 alignment for direct I/O;
**	may be zero if no alignment needed, or direct I/O not supported.
**
** History:
**	6-Nov-2009 (kschendel) SIR 122757
**	    Written.
*/

u_i4
DIget_direct_align(void)
{
#ifdef LNX
    return (512);
#elif defined(any_aix)
    return (4096);
#elif defined(sparc_sol) || defined(a64_sol)
    /* Solaris doesn't need alignment */
    return (0);
#else
    /* Catch-all, assume 512 */
    return (512);
#endif
} /* DIget_direct_align */

/*{
** Name: DIlru_close - Close a given DI file.
**
** Description:
**	Will close a given DI file.  Called from DIclose when dbms actually
**	closes the file.  This will insure that a session that closes a file
**	which it has opened will actually close it in DIclose.  If file is
**	not in the open file cache then return OK since this means it was
**	LRU'd out (closed).
**
**	The slot in the open file cache is put on the free list.
**
**	In the multiple session per server case DIclose may not be called by the
**	session which originally opened the file.  This can take place in the
**	following scenario:
**
**		Session A opens the file.  Session B opens the file.  Now both
**		sessions have their own separate UNIX file descriptors for the
**		file.  DMF decides not to cache the open file any more so it
**		issues a DIclose() on the file - DMF has decided to do this
**		processing a request from Session B.  Session B will call 
**		DIlru_close() actually closing the UNIX file descriptor.  The
**		architecture makes it difficult if not impossible for Session B
**		to inform Session A to close the file; so the file remains open
**		in Session A.  Eventually as session A opens more files the file
**		in question will be closed by the lru algorithm. 
**
**	Also it is assumed that all open files for a session are closed when the
**	process who owns that session exits.
**
** Inputs:
**	f		    Pointer to the DI file context needed to do I/O.
**
** Outputs:
**	err_code	    Pointer to a variable used to return OS errors.
**
**    Returns:
**        OK		    Successfully closed file, freeing a file descriptor.
**	  DI_CANT_CLOSE	    No available file descriptors to close.
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    06-mar-87 (mmm)
**          Created new for jupiter.
**    17-Mar-90 (anton)
**	    Check that file is not pinned when closed
**      31-may-90 (blaise)
**          Integrated changes from termcl:
**              lru_close of a pinned file should not be fatal - bug #21793.
**    30-nov-1992 (rmuth)
**	    Add error checking.
**	1-Jul-2005 (schka24)
**	    If fd-per-thread, we need to close ALL of the fd's that this
**	    file was using in other threads.  Otherwise, fd's vanish and
**	    we eventually fail with "all fd's pinned".
**	08-jul-2009 (smeke01) b120853
**	    For DI_thread_affinity make sure that any DI_FILE_DESC objects 
**	    remaining on the DI_IO io_fd_q after a DIclose() no longer point 
**	    back to the DI_IO.
*/
STATUS
DIlru_close(
    DI_IO		*f,
    CL_ERR_DESC	*err_code )
{
    DI_FILE_DESC	*di_file;
    bool		found;
    STATUS		big_status =OK, ret_status = OK, intern_status =OK;

    /*
    ** If a fatal error occured in DI at some time then bring down 
    ** the server to prevent possible corruption.
    */
    if ( Di_fatal_err_occurred )
    {
	TRdisplay(
	   "DIlru_close: Failing server due to unrecoverable DI error\n");
	PCexit(FAIL);
    }

    /*
    ** If FDThreadAffinity, need to loop to close all FDs
    ** associated with this DI_IO.
    ** The FD's are on the DI_IO's io_fd_q.  If we don't free them here,
    ** we lose track of them!  Of course, all the FD's are expected to
    ** be idle at this point (else the caller is making a mistake in
    ** trying to close the file now).
    */
    do
    {
	/* Find fd for this file in the open file cache */
	DI_find_fd(f, FALSE, &di_file, err_code);
	found = (di_file != NULL);

	if (di_file && di_file->fd_racestat == OK)
	{
	    /* 
	    ** found entry in the open file cache, its fd_mutex is locked
	    */
# ifdef xCL_ASYNC_IO
	    if (Di_async_io)                                                     
	    {                                                                    
		/*                                                                 
		** DI_aio_inprog returns FAIL if EINPROGRESS set                   
		** for this file, otherwise OK.                                    
		*/                                                                 
								     
		if ( DI_aio_inprog(di_file->fd_unix_fd) == OK)                     
		{
		    ret_status = DI_lru_file_close( &di_file, err_code,              
					  &intern_status);
		    f->io_stat.lru_close++;
		}
		else                                                               
		    TRdisplay(                                                     
		      "DIwarning: DIlru_close on in-use file - close deferred\n");
	    }                                                                    
	    else                                                                 
# endif /* xCL_ASYNC_IO */

	    if (di_file->fd_refcnt > 0)
	    {
		/* 
		** Someone is still referencing the file so
		** we will not do the close immediately in this case.
		** This may leave the file open for a long time, but
		** it will be closed off if lru activity begins. 
		*/
		TRdisplay(
		    "DIwarning: DIlru_close on pinned file %12s %p %d (%d) - fd %d close deferred\n",
			    f->io_filename,
			    di_file,
			    di_file->fd_uniq.uniq_seqno,
			    di_file->fd_refcnt,
			    di_file->fd_unix_fd);
	    }
	    else
	    {
		ret_status = DI_lru_file_close( &di_file, err_code,
					       &intern_status);
		f->io_stat.lru_close++;
	    }
	}

	/*
	** If we still have the di_file then release its mutex
	*/
	if ( di_file )
	    CS_synch_unlock( &di_file->fd_mutex );

    } while ( Di_thread_affinity && found );	/* Loop for all FD's for this DI_IO */

    /* 
    ** invalidate the control block 
    */
    f->io_type = DI_IO_LRUCLOSE_INVALID;

    if ( Di_thread_affinity )
	DI_tidy_close(f);

    f->io_uniq.uniq_di_file = (PTR)NULL;
    f->io_uniq.uniq_seqno   = 0;

    if ( big_status != OK )
	ret_status = big_status;

    if (ret_status != OK )
	DIlru_set_di_error( &ret_status, err_code, intern_status, 
			    DILRU_CLOSE_ERR);

#ifdef	xDEV_TST
    /* Dump stats when the last log is closed */
    if (f->io_open_flags & DI_O_LOG_FILE_MASK &&
	htb->htb_stat.fds_open == 0)
    {
	TRdisplay("%@ DI stats: sizeof(DI_FILE_DESC) %d number of DI_FILE_DESC %d\n",
		sizeof(DI_FILE_DESC), htb->htb_fd_list_count);
	TRdisplay("\trequests: %d hits: %d ",
"io_hits: %d ",
"maxfds: %d fd opens: %d fds open: %d fds lru'd: %d\n",
		htb->htb_stat.requests, htb->htb_stat.hits, 
		htb->htb_stat.io_hits, 
		htb->htb_stat.max_fds, htb->htb_stat.opens, htb->htb_stat.fds_open, 
		htb->htb_stat.lru_closes);
    }
#endif /* xDEV_TST */

    return( ret_status );
}

/*{
** Name: DIlru_free - Close a DI file using lru algorithm.
**
** Description:
**	Will close a DI file using a lru algorithm.  Can be called from
**	anywhere in the CL.  This routine will be called by any CL routine
**	called by the dbms which fails while trying to open a file because
**	there are no available file descriptors.  Callers should loop
**	if they encounter the same out of file descriptor error until 
**	DIlru_free indicates that it can't close any more files.
**
**	The last entry in the lru queue is chosen as the victim.  That 
**	file descriptor is closed.  The entry is put on the free list.
**
**	NOTE: this is now only used when DI is operating in the main
**		process and not a slave async model.
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**    Returns:
**        OK		    Successfully closed file, freeing a file descriptor.
**	  DI_CANT_CLOSE	    No available file descriptors to close.
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    06-mar-87 (mmm)
**          Created new for jupiter.
**    30-nov-1992 (rmuth)
**	    - Do not put the Di_fatal_err_occurred check in this routine as
**	      it can be called by other DIlru routines and hence may stop
**	      the trace back message being issued.
**	    - Track close failure.
**	    - Prototype.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x into %p for pointer values.
**    12-may-1994 (rog)
**	    Reset the file's sticky bit when we close the file if it was
**	    turned off.
**	04-Nov-1997 (jenjo02)
**	    Acquire DI_misc_sem before calling DI_lru_free().
**	30-Sep-2005 (jenjo02)
**	    Squashed DI_lru_free back into this fcn - it was
**	    called nowhere else.
**	    While we were perhaps unable to close any files,
**	    perhaps some other threads were able to.
*/
STATUS
DIlru_free( 
    CL_ERR_DESC *err_code)
{
    DI_FILE_DESC	*di_file;
    QUEUE		*q, *p;
    STATUS		status = OK, intern_status = OK;
    u_i4		current_lru_count;

    /*
    ** Save the current number of files closed by LRU activity.
    ** If this number is different after we wait for the mutex,
    ** then concurrent LRU-ing has happened, so we'll just return
    ** with the hope that sufficient FDs have been freed up.
    */
    current_lru_count = htb->htb_stat.lru_closes;

    CS_synch_lock(&htb->htb_fd_list_mutex);

    /* If some file(s) have been LRU'd, return optimistically */
    if (current_lru_count != htb->htb_stat.lru_closes)
    {
	CS_synch_unlock(&htb->htb_fd_list_mutex);
	return(OK);
    }

    /* Find one open, unreferenced entry from the back of the LRU list */

    q = &htb->htb_fd_list;
    p = q->q_prev;

    while (p != q)
    {
	di_file = (DI_FILE_DESC *)p;

	/* Skip files that are not open or being referenced */
	if (di_file->fd_state == FD_FREE || di_file->fd_refcnt)
	{
	    p = p->q_prev;
	    continue;
	}

	/* Unlock the lru queue, lock the fd, check again */
	CS_synch_unlock(&htb->htb_fd_list_mutex);
	CS_synch_lock(&di_file->fd_mutex);
	/*
	** By the time we get the mutex, the di_file may
	** be closed or open and referenced.
	*/
	if (di_file->fd_state == FD_FREE || di_file->fd_refcnt)
	{
	    CS_synch_unlock(&di_file->fd_mutex);
	    CS_synch_lock(&htb->htb_fd_list_mutex);
	    p = q->q_prev;
	    continue;
	}

	/*
	** Set the reference count to -1 so that other threads in this
	** function will ignore this file.
	*/
	di_file->fd_refcnt--;

#if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
	/* Reset the sticky bit in case we turned it off. */
	if (!Di_no_sticky_check)
	{
	    struct stat sbuf;

	    if (!fstat(di_file->fd_unix_fd, &sbuf))
	    {
		int new_perms = (unsigned)sbuf.st_mode & ~S_IFMT;

		new_perms |= S_ISVTX;
		(VOID) fchmod(di_file->fd_unix_fd, new_perms);
	    }
	}
#endif

	/* close the file */
	if (close(di_file->fd_unix_fd) == (int) -1)
	{
	    SETCLERR( err_code, DI_BADCLOSE, ER_close);
	    status = DILRU_FREE_ERR;
	}

	CSadjust_counter(&htb->htb_stat.fds_open,-1);
	CSadjust_counter(&htb->htb_stat.lru_closes,1);

	/* now put on free list and release fd_mutex */
	DI_free_insert(&di_file);

	if (status != OK)
	    DIlru_set_di_error( &status, err_code, intern_status, DILRU_FREE_ERR );
	return(status);
    }
    
    /* No open unreferenced entries for us to LRU... */
    CS_synch_unlock(&htb->htb_fd_list_mutex);

    /* ...but maybe another thread freed one up */
    if (current_lru_count == htb->htb_stat.lru_closes)
    {
	status = DILRU_ALL_PINNED_ERR;
	DIlru_set_di_error( &status, err_code, intern_status, DILRU_FREE_ERR );
    }
    
    return(status);
}

# ifdef xCL_ASYNC_IO
/*{
** Name: DIlru_aio_free - Find a DI file to close when async_io in use.
**
** Description:
**	
**	Called when the fd cache limit (async io only) is reached.
**	
**	Searches the cache to find a file which has no incompleted io
**	and closes that one (starts with the oldest file in the list)
**	
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**    Returns:
**        OK		    	Successfully closed file, freeing a 
**				file descriptor.
**	  DILRU_ALL_PINNED_ERR	No available file descriptors to close.
**	  DILRU_FREE_ERR    	Failure when closing file or accessing queue.
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
** 	18oct95 (amoICL)
**		created.
*/
STATUS
DIlru_aio_free( 
    CL_ERR_DESC *err_code )
{
    DI_FILE_DESC	*di_file;
    QUEUE		*p, *q;
    STATUS		close_ok, status = OK;

    CS_synch_lock(&htb->htb_fd_list_mutex);
    q = &htb->htb_fd_list;
    p = q->q_prev;
    
    while (p != q)
    {
	di_file = (DI_FILE_DESC *)p;

	/* Skip files that are not opened */
	if (di_file->fd_state == FD_FREE)
	{
	    p = p->q_prev;
	    continue;
	}

	CS_synch_unlock(&htb->htb_fd_list_mutex);
	CS_synch_lock(&di_file->fd_mutex);
	if (di_file->fd_state == FD_FREE)
	{
	    CS_synch_unlock(&di_file->fd_mutex);
	    CS_synch_lock(&htb->htb_fd_list_mutex);
	    p = q->q_prev;
	    continue;
	}
	if ((close_ok = DI_aio_inprog(di_file->fd_unix_fd)) == OK)
		break;
	p = p->q_prev;
	CS_synch_unlock(&di_file->fd_mutex);
	CS_synch_lock(&htb->htb_fd_list_mutex);
    }

    /* Check for head of list */

    if (p == q)
    {
	/* No victim found - return failure */
	CS_synch_unlock(&htb->htb_fd_list_mutex);
	return(DILRU_ALL_PINNED_ERR);
    }

    /* Victim found! */

#if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
    /* Reset the sticky bit in case we turned it off. */
    if (!Di_no_sticky_check)
    {
#ifdef LARGEFILE64
	struct stat64 sbuf;

	if (!fstat64(di_file->fd_unix_fd, &sbuf))
#else /* LARGEFILE64 */
       struct stat sbuf;

       if (!fstat(di_file->fd_unix_fd, &sbuf))
#endif /* LARGEFILE64 */
       {
	    int new_perms = (unsigned)sbuf.st_mode & ~S_IFMT;

	    new_perms |= S_ISVTX;
	    (VOID) fchmod(di_file->fd_unix_fd, new_perms);
       }
    }
#endif

    /* close the file */
    if (close(di_file->fd_unix_fd) == (int) -1)
    {
       SETCLERR( err_code, DI_BADCLOSE, ER_close);
       status = DILRU_FREE_ERR;
    }

    /* now put on free list and release the fd_mutex */
    DI_free_insert(&di_file);

    return(status);
}
# endif /* xCL_ASYNC_IO */

/*{
** Name: DIlru_init - Initialize hash control block.
**
** Description:
**	Given a pointer to a HASH_CONTROL_BLOCK this routine will initialize
**	the hash table to be empty.  It will allocate DI_ENTRY_NUM entries and 
**	put them on the free list.  It will initialize all hash buckets to be
**	empty.
**
**	If this is a multi-threaded server (we can tell because cs_max_active
**	in the Cs_srv_block is non-zero), then attempt to initialize an
**	appropriate number of DI slave processes to perform asynchronous I/O.
**	If the initialization fails (presumably due to resource contraints
**	such as system swap space limitations), then we'll perform the I/O
**	synchronously; issue trace messages to this effect.
**
** Inputs:
**      htb              Pointer to the hash control block to be initialized.
**
** Outputs:
**	htb		 Initialized hash control block and table.
**
**    Returns:
**        OK
**	  DI_CANT_ALLOC(XX) Can't allocate dynamic memory.
**
**    Exceptions:
**        none
**
** Side Effects:
**        On a DI in slave process implementation sub process slaves
**		will be spawned.
**
** History:
**    06-mar-87 (mmm)
**          Created new for jupiter.
**    01-mar-89 (mmm)
**          Added TRdisplays for when disk I/O slave initialization fails.
**
**    04-sep-90 (jkb)
**	    Call CSslave_init even if II_NUM_SLAVES = 0 so the control
**          block is properly initialized
**    1--jan-91 (jkb)
**	    add check to see if Di_nslaves > DI_MAX_SLAVES
**    30-oct-1991 (bryanp)
**	    Added code to map ME_IO_MASK segments in each slave.
**	13-nov-1991 (bryanp)
**	    Don't try to initialize DI slaves unless this is a true
**	    multi-threaded server. Formerly, we always tried to initialize the
**	    slaves, even in pseudo-servers (such as the RCP), and expected to
**	    get a failure code back from CSslave_init(). With the advent of
**	    logfile slaves under the RCP, this technique is no longer usable.
**    30-nov-1992 (rmuth)
**	    Changed flow of code so that we allocate memory for the htb
**	    earlier on to reduce the window where this routine can be
**	    called twice. Initialise the DI_misc_sem.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	26-jul-1993 (mikem)
**	    Added DImo_init_slv_mem() call to register the DI slave MO 
**	    interface to performance statistics.
**	23-aug-1993 (bryanp)
**	    Isolated slave-mapping code into DI_lru_slmapmem for sharing.
**	23-aug-1993 (andys)
**	    Only call DImo_init_slv_mem() if Di_slave is non zero, i.e.	
**	    if we successfully started some slaves.
**	31-jan-1994 (bryanp) B56634
**	    Remove unused variables from DIlru_init; they were causing some
**		compilers to issue warning messages about "used before set".
**	12-may-1994 (rog)
**	    Call NMgtAt() to set Di_no_sticky_check correctly on SunOS.
**	17-May-1994 (daveb) 59127
**	    Fixed semaphore leaks, named sems.
**	03-Apr-1995 (canor01)
**	    Initialized local_htb explicitly instead of allocating zeroed memory
**	19-Jun-1994 (amo ICL)
**	    Added async io amendments
**	21-sep-1995 (itb ICL)
**	    Read the gather write configuration and set Di_gather_io.
**	07-nov-1996 (canor01)
**	    If async_io is OFF, check II_NUM_SLAVES to see if slaves should
**	    be started.
**	07-Oct-1997 (jenjo02)
**	    Initialize DI_sc_sem when slaves are being used.
**	08-Oct-1998 (jenjo02)
**	    Even if there are no slaves, CSslave_init() must be called
**	    to properly initialize the server's shared memory, in
**	    particular csev must register CS_event_shutdown as a
**	    atexit so that the shared memory gets clean up when the
**	    server terminates, else the RCP's checkdead thread will think
**	    the server has died.
**	02-Feb-1999 (schte01)
**	    Modified code so that if OS_THREADS and USE_IDLE_THREAD we don't
**       automatically assume slaves. If II_THREAD_TYPE has not been set
**       to internal (CS_is_mt), do not set number of slaves. 
**	24-Aug-1999 (jenjo02)
**	    Disallow GatherWrite if running Ingres threads.
**	27-Mar-2003 (jenjo02)
**	    Added init call to dodirect() during server startup
**	    to test/set/TRdisplay DIRECT_IO setting. SIR 109925
**	1-Jul-2005 (schka24)
**	    Start seq #'s at 1, not 0;  zero is "no sequence number".
**	28-Nov-2005 (kschendel)
**	    Init the zeroing buffer based on config param.
**	23-Jul-2008 (kschendel)
**	    Remove direct IO control from DI.  In order to get proper
**	    control of tables vs txn log vs work files, the callers have to
**	    figure out whether direct IO is requested.
**	11-Feb-2009 (smeke01) b120852
**	    Remove redundant message about fd_affinity=thread now that
**	    open file descriptor limit for Linux can be increased from 
**	    1024 to 8192.
*/
STATUS
DIlru_init(
    DI_HASH_TABLE_CB	**global_htb ,
    CL_ERR_DESC 	*err_code)
{
    DI_HASH_TABLE_CB	*local_htb;
    i4			bufsize;
    i4			i;
    fd_set		fdmask;
    char		*str;  /* no auto char type in AIX */
    ME_SEG_INFO		*seginfo;
    DI_SLAVE_CB		*disl;
    STATUS		status = OK;
    bool		have_sem = FALSE;
    STATUS		intern_status = OK, send_status = OK, me_status = OK;
    DI_OP       	di_op;
    char		sem_name[CS_SEM_NAME_LEN];

    FD_ZERO(&fdmask);

    do
    {
	/*
	**
	*/
	Di_fatal_err_occurred = FALSE;

        /*
        ** Initialise are misc semaphore
        */
    	status = CSw_semaphore(  &DI_misc_sem, CS_SEM_SINGLE,
					"DI MISC" );
    	if ( status != OK )
	{
	    Di_fatal_err_occurred = TRUE;
	    intern_status = DI_LRU_PSEM_ERR;
	    TRdisplay(
		"DIlru_init: Failed initialising DI semaphore, status=%x\n",
		status );
	    break;
	}

	/*
	** Block anyone using this structure until we have set it up
	*/
	CSp_semaphore(SEM_EXCL, &DI_misc_sem);
	have_sem = TRUE;

        local_htb = (DI_HASH_TABLE_CB *)MEreqmem((u_i4)0,
					     (u_i4)sizeof(*local_htb),
					     FALSE, &me_status);
	if ( status != OK )
	{
	    Di_fatal_err_occurred = TRUE;
	    intern_status = DI_LRU_GENVSEM_ERR;
	    TRdisplay( "DIlru_init: Failed releasing ME semaphore, status=%x\n",
		       status );
	    break;
	}

	/*
	** check allocated memory ok
	*/
        if ( (me_status != OK ) || (!local_htb) )
        {   
	    status = me_status;
	    intern_status = DILRU_MEREQMEM_ERR;
	    break;
        }

        *global_htb = local_htb;


	/*
	** Check whether async io available and in use
	*/

	status = PMget("ii.$.$.$.async_io",&str);

	if (status == OK)
	    Di_backend = TRUE;
	if ( (status == OK) &&
	     (str[0] != '\0') &&
	     (STcasecmp( str, "on" ) == 0) )
	{
	    TRdisplay("DIlru_init: async_io in use\n");
	    Di_async_io = TRUE;
	    Di_nslaves = 0;
	}
	else
	{
	    /*TRdisplay("DIlru_init: async_io not in use\n");*/
	    Di_async_io = FALSE;
	}

# ifdef OS_THREADS_USED
	/* GatherWrite, FD thread affinity - only in OS backends */
        if ( Cs_srv_block.cs_max_active > 0 )
	{
	    /*
	    ** Check whether gather write is available and in use.
	    ** The current implementation of gatherwrite uses writev(),
	    ** hence has no dependencies on async_io, but is supported
	    ** only in OS-threaded servers.
	    */

	    status = PMget("ii.$.$.$.gather_write",&str);

	    if ( (status == OK) )
	    {
		if ( (str[0] != '\0') &&
		     (STbcompare(str,0,"on",0,TRUE) == 0) )
		{
		    if ( CS_is_mt() )
		    {
			TRdisplay("%@ DIlru_init: GatherWrite in use\n");
			Di_gather_io = TRUE;
		    }
		    else
		    {
			TRdisplay("%@ Dilru_init: GatherWrite is not supported by Ingres threads\n");
			Di_gather_io = FALSE;
		    }
		}
		else                                                
		{
		    TRdisplay("%@ DIlru_init: GatherWrite not in use\n");
		    Di_gather_io = FALSE;
		}
	    }

	    /*
	    ** Check if FD thread affinity.
	    ** Allowed only with OS threads and without async_io.
	    */
	    Di_thread_affinity = FALSE;

	    status = PMget("ii.$.$.$.fd_affinity",&str);

	    if ( (status == OK) )
	    {
		if ( (str[0] != '\0') &&
		     (STbcompare(str,0,"thread",0,TRUE) == 0) )
		{
		    if ( Di_async_io )
			TRdisplay("%@ Dilru_init: FD Thread affinity is not supported with async_io\n");
		    else if ( ! CS_is_mt() )
			TRdisplay("%@ Dilru_init: FD Thread affinity is not supported by Ingres threads\n");
		    else
		    {
			TRdisplay("%@ DIlru_init: FD affinity: Thread\n");
			Di_thread_affinity = TRUE;
		    }
		}
		if ( !Di_thread_affinity )
		    TRdisplay("%@ DIlru_init: FD affinity: File\n");
	    }
	}
# endif /* OS_THREADS_USED */

	/*
	** If need DI slaves see how many we need.
	**
	** OS-threaded MT servers can no longer run with slaves
	** due to the demise of the IdleThread (there was never
	** a compelling reason to run slaves with OS threads anyway).
	*/
        if (!Di_async_io
# ifdef OS_THREADS_USED
# ifndef USE_IDLE_THREAD /* allow slaves if MT with IdleThread */
	   && !( CS_is_mt() )
# endif /* USE_IDLE_THREAD */
# endif /* OS_THREADS_USED */
	   )
        {
	    NMgtAt("II_NUM_SLAVES", &str);
	    if (str && *str)
	    {
	    	CVan(str, &Di_nslaves);
	    	if(Di_nslaves > DI_MAX_SLAVES)
		    Di_nslaves = DI_MAX_SLAVES;
	    }
# ifdef OS_THREADS_USED
	    else
	    {
		/*
		** OS threading runs without slaves.  However, if
		** we're running without MT support, default to 2
		** if the user has not specified a number.
		*/
# ifdef USE_IDLE_THREAD /*idle thrd use doesn't imply II_THREAD_TYPE internal */
	   if (! CS_is_mt() )
# endif /* USE_IDLE_THREAD */
		Di_nslaves = 2;
	    }
# endif /* OS_THREADS_USED */
        }

#if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
	/* Don't mess with unsetting the sticky bit if this is set. */
	NMgtAt("II_DIO_STICKY_SET", &str);
	if( Di_no_sticky_check = (str && *str != EOS) )
	{
	    TRdisplay("IIdio: Sticky bit not being checked.\n" );
	}
#endif

	/*
	** If this is not a pseudo server then start up the slaves.
	** Even if there are no slaves, the call to CSslave_init()
	** is needed to properly initialize the server's shared
	** memory segment.
	*/
        if ( Cs_srv_block.cs_max_active > 0 )
	{
	    if ( Di_nslaves )
		status = CSw_semaphore(  &DI_sc_sem, CS_SEM_SINGLE, "DI SC" );

	    status = CSslave_init(&Di_slave, CS_DI_SLAVE, Di_nslaves,
				    sizeof(DI_SLAVE_CB), 
				    Cs_srv_block.cs_max_active,
				    DI_complete, fdmask);

	    if ( (status != OK) && Di_nslaves )
	    {
	        TRdisplay("Warning: the system could not initialize \n");
	        TRdisplay("%d disk I/O slave processes to service %d users.\n", 
				    Di_nslaves, Cs_srv_block.cs_max_active);
	    }
	    if (status)
	    {
		intern_status = DI_SLAVE_INIT_ERROR;
		break;
	    }
	}
	else
	{
	    /* 
	    ** operate with DI in main process (standard model) 
	    */
	    Di_nslaves = 0;	
	}

	/*
	** Set the size of lru cache table for DIlru_allocate_more_slots()
	*/
	if (Di_nslaves)
	    Di_lru_cache_size = iiCL_get_fd_table_size() * Di_nslaves; 
	else
	    Di_lru_cache_size = iiCL_get_fd_table_size() -
                            (Cs_srv_block.cs_max_sessions + 1);

    	/* 
	** initialize empty lru queue 
	*/
    	local_htb->htb_fd_list.q_next = 
		local_htb->htb_fd_list.q_prev = 
			(QUEUE*)&local_htb->htb_fd_list;
	CS_synch_init(&local_htb->htb_fd_list_mutex);

    	/* 
	** sequence number is bumped by one throughout the lifetime of server 
	** Zero is "no sequence number", avoid it.
	*/
        local_htb->htb_seqno = 1;

        /*
        ** Finish initialization of htb
        */
        local_htb->htb_fd_list_count = 0;

	MEfill(sizeof(local_htb->htb_stat), 0, &local_htb->htb_stat);

	status = DI_lru_allocate_more_slots( &intern_status);
	if ( status != OK )
	    break;

    	if (Di_slave)
    	{
	    /*
	    ** Ask each slave to map in each shared memory segment which has 
	    ** been marked ME_IO_MASK. I/O to and from these segments can 
	    ** then be handled directly by the slaves without requiring 
	    ** intermediate buffer copies
	    */
	    for (seginfo = (ME_SEG_INFO *)ME_segpool.q_prev, status = OK;
	     	    (QUEUE *)seginfo != &ME_segpool;
	     	    seginfo = (ME_SEG_INFO *)seginfo->q.q_prev)
	    {
	        if (seginfo->flags & ME_IO_MASK)
		    status = DI_lru_slmapmem(seginfo, &intern_status,
						&send_status);
		if (status)
		    break;
	    } /* for */

	    if ( status != OK )
		break;

        } /* if Di_slave */
	else
	{
	    u_i4 align;

	    /* For non-slave, see if we have a zeroing buffer size in the
	    ** config.  If not, use the builtin default, and allocate the
	    ** zeroing buffer.  (Leave extra for page alignment.)
	    */

	    bufsize = DI_ZERO_BUFFER_SIZE;
	    if ( Cs_srv_block.cs_max_active > 0 )
	    {
		status = PMget("!.di_zero_bufsize",&str);
		if ( status == OK )
		{
		    if (CVal(str, &bufsize) != OK)
		    {
			TRdisplay("DIlru_init: ignoring invalid value for di_zero_bufsize (%s)\n",
				str);
			bufsize = DI_ZERO_BUFFER_SIZE;
		    }
		}
		status = OK;		/* Ignore "no such param" */
		TRdisplay("DIlru_init: using zero-buffer size %d\n",bufsize);
	    }
	    if (bufsize < DI_MAX_PAGESIZE)
		bufsize = DI_MAX_PAGESIZE;
	    align = DIget_direct_align();
	    if (align < DI_RAWIO_ALIGN)
		align = DI_RAWIO_ALIGN;
	    if (align < ME_MPAGESIZE)
		align = ME_MPAGESIZE;
	    Di_zero_buffer = MEreqmem(0, bufsize + align, TRUE, &me_status);
	    if (Di_zero_buffer == NULL || me_status != OK)
	    {   
		status = me_status;
		intern_status = DILRU_MEREQMEM_ERR;
		break;
	    }
	    Di_zero_buffer = ME_ALIGN_MACRO(Di_zero_buffer, align);
	    Di_zero_bufsize = bufsize;
	} /* if no slaves */


	have_sem = FALSE;
        CSv_semaphore( &DI_misc_sem);

	if ( send_status != OK )
	    break;

    	/* 
	** set flag indicating control block has been initiialized 
	*/
    	htb_initialized = TRUE;

    } while (FALSE );

    if ( status != OK )
	send_status = status;

    /*
    ** ERROR condition so lets release any resources that we may have
    ** aquired and return an error.
    */

    if ( send_status != OK )
    {
        if ( have_sem )
	{
	    status = CSv_semaphore( &DI_misc_sem);
	    if ( status != OK )
	    {
		Di_fatal_err_occurred = TRUE;
		TRdisplay(
		   "DIlru_init: Failed releaseing DI semaphore #2,status=%x\n",
		   status );
	    }
	}

        DIlru_set_di_error( &send_status, err_code, 
			    intern_status, DILRU_INIT_ERR );
    }

    if ((Di_slave) || (Di_async_io))
    	DImo_init_slv_mem(Di_nslaves, Di_slave);

    return(send_status);
}

/*{
** Name: DIlru_init_di - Initialize DI.
**
** Description:
**      Initializes DI.  The hash control block and table are initialized
**      and on a slave process implementation the slaves are spawned.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**    Returns:
**        OK
**        DI_CANT_ALLOC(XX) Can't allocate dynamic memory.
**
**    Exceptions:
**        none
**
** Side Effects:
**        On a DI in slave process implementation sub process slaves
**              will be spawned.
**
** History:
**    31-oct-89 (fls-sequent)
**          Created new for MCT.  MCT requires a way to force the DI slaves
**          to be forked before starting multi-threading.
**
*/
STATUS
DIlru_init_di( CL_ERR_DESC *err_code )
{
    STATUS      ret_val = OK;

    if (!htb_initialized)
    {
        ret_val = DIlru_init(&htb, err_code);
    }

    return(ret_val);
}

/*{
** Name: DIlru_open - insure that input file exists in open file cache
**
** Description:
**	Given a pointer to a DI_IO struct search for corresponding entry
**	in the open file cache.  If the file does not exist in the cache then
**	open it and place it in the cache.  If necessary call DI_lru_free
**	to free up a spot in the cache.
**
**	It is assumed that caller has correctly initialized the DI_IO
**	structure.  The DI_IO structure must have the path, pathlength,
**	filename, and filename length filled in.  It must also contain
**	the unique numeric id assigned by DIlru_uniq.  All the previous
**	initialization should take place in DIopen, and the information
**	is stored and saved in the DI_IO struct passed in by the caller
**	on each DI operation.
**
**	If the free list is empty another DI_ENTRY_NUM entries will be
**	allocated and put onto the free list.  The number of entries 
**	allocated continues to grow as long as new files can be open'd
**	successfully.  When opens begin to fail then entries will be
**	closed and put on the free list to be reused.
**
**	When successful DIlru_open() returns a pointer to an event control
**	block that has been reserved using CSreserve_event().  It is up to
**	the caller to free that event control block using CSfree_event().
**	Not doing this can lead to a depletion of event control blocks, 
**	which will cause the server fail in a variety of ways (hang, loop
**	in CSreserve_event, ...).  On failure no event control block is
**	reserved.
**
** Inputs:
**	f		 pointer to di file descriptor information.
**	flags		 Indicates if file should be created and open file
**			 to it returned.  Used mostly for temporary files.
**			 Also, may indicate a need to retry the open.
**
** Outputs:
**	evcb		 upon exit an event control block to be used for
**			 operations.  If DI in main process then a unix
**			 file descriptor is returned instead which is
**			 only guaranteed for the current DI call.
**	err_code	 system open() error info.
**
**    Returns:
**        OK
**	  DI_CANT_OPEN   Can't open a file due to system limit 
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    06-mar-87 (mmm)
**          Created new for jupiter.
**    21-jan-89 (mikem)
**	    O_SYNC support.
**    06-feb-89 (mikem)
**	    Better support for DI CL_ERR_DESC, including initializing to zero,
**	    returning an CL_ERR_DESC from DIlru_open().
**	    Added unix_open_flags member to the slave control block so we don't
**	    have to overload length anymore.
**    20-Mar-90 (anton)
**	    Serveral DI fixes - LRU multiple open race condition fixed
**		- LRU queue management fixed.
**    23-Mar-90 (anton)
**	    Fix the last fix - CSi_semaphore arguments were reversed
**    25-sep-1991 (mikem) integrated following change: 27-jul-91 (mikem)
**	    bug #38872
**	    DIopen() now sets io_open_flags to DI_O_OSYNC_MASK if an O_SYNC 
**	    open() is required by the system.  This mask is now checked rather
**	    than DI_SYNC_MASK which is only used by callers of DIopen() to 
**	    indicate that some available syncing mechanism must be used.
**    30-nov-1992 (rmuth)
**	    - Add error checking, restructured by taking a large chunk
**	      of the code and placing it in the routine
**	      DIlru_file_open.
**    30-feb-1993 (rmuth)
**	    Use DI_FILE_MODE for file modes, was hardcoded to 0777 before.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**    12-may-1994 (rog)
**	    Use O_DSYNC if it exists instead of O_SYNC.
**	15-mar-96 (nick)
**	    Remove overloading of variable in selection of slave to use.  This
**	    had the effect of causing a failure when all slaves had more than
**	    999 files open ; changing the algorithm means that not only 
**	    this doesn't now occur but we correctly flush out files as well
**	    if necessary.
**	07-Oct-1997 (jenjo02)
**	  o After waiting for DI_O_OPEN_IN_PROGRESS, the di_file entry may
**	    have been recycled and now belongs to another file. Check the
**	    unique file id after waiting and if different, re-search the
**	    cache.
**	  o Bump fd_refcount before calling DIlru_file_open() to prevent this
**	    cache entry from being released while the open progresses.
**	  o Move reused cache entry to the front of the lru queue
**	    to prevent it from being prematurely lru'd out.
**	16-Nov-1998 (jenjo02)
**	    Copy open flags to disl so that the type of file (DIO/LIO)
**	    can be discerned when suspending.
**  01-Apr-2004 (fanch01)
**      Add O_DIRECT support on Linux depending on the filesystem
**      properties, pagesize.  Fixups for misaligned buffers on read()
**      and write() operations.  Added page size parameter to IIdio_open().
**	14-Oct-2005 (jenjo02)
**	    (Slaves only). If di_file has a slave with the file open
**	    and another session is opening yet another FD
**	    (racestat == SL_OPEN_IN_PROGRESS), ignore the 
**	    "threshold" tests and use the Slave we found rather than
**	    opening another FD in another Slave.
*/
STATUS
DIlru_open(
    DI_IO	*f,
    i4		flags,
    DI_OP	*op,
    CL_ERR_DESC	*err_code)
{
    DI_FILE_DESC	*di_file = (DI_FILE_DESC*)NULL;
    bool		first_open_flag = TRUE;
    bool		have_sem       = FALSE;
    bool		reserve_ev_flag = FALSE;
    bool		slave_found;
    i4			i, j, k;
    register DI_SLAVE_CB *disl;
    STATUS		status = OK, 
			intern_status = OK; 

    op->di_flags = 0;
    CL_CLEAR_ERR( err_code );

    do
    {
	/*
	** See of we need to initialize the hash control block
	*/
	if (!htb_initialized)
	{
	    if (status = DI_lru_check_init( &intern_status, err_code ))
		break;
	}

	/*
	** If a fatal error occured in DI at some time then bring down 
	** the server to prevent possible corruption.
	*/
	if ( Di_fatal_err_occurred )
	{
	    TRdisplay(
	       "DIlru_open: Failing server due to unrecoverable DI error\n");
	    PCexit(FAIL);
	}

	/*
	** If runing without slaves then perform work inline
	*/
	if (!Di_slave)
	    return(DI_lru_inproc_open(f, flags & DI_FCREATE, op, err_code));

	/*
	** ICL rogt take event cb earlier, before we take semaphore
	** reserve an event control block
	*/
	status = CSreserve_event(Di_slave, &op->di_csevcb);
	if ( status != OK )
	{
	    intern_status = DI_LRU_CSRESERVE_ERR;
	    break;
	}

	reserve_ev_flag = TRUE;
	op->di_flags |= DI_OP_RESV;
	op->di_evcb = disl = (DI_SLAVE_CB *)(op->di_csevcb)->data;

	/* Copy flags to disl so the CSsuspend type can be determined */
	disl->io_open_flags = f->io_open_flags;

	disl->lru_close = -1;
	disl->pre_seek = -1;

	/* Send file properties from DI_IO to slave */
	FPROP_COPY(f->io_fprop,disl->io_fprop);

	/* overload length as pagesize for open call */
	disl->length = f->io_bytes_per_page;

	/* 
	** Does fd exist already?
	** If not, make one.
	*/
	status = DI_find_fd(f, TRUE, &di_file, err_code);

	if (di_file == (DI_FILE_DESC*)NULL)
	{
	    /* Not found and an entry could not be created */
	    break;
	}

	/* File is in the cache, its fd_mutex is locked */
	
	if (status == OK)
	{
	    /* File existed in the cache */
	    
	    if (flags & DI_FCREATE)
	    {
		status = DI_EXISTS;
		break;
	    }

	    if (di_file->fd_racestat != OK &&
		di_file->fd_racestat != SL_OPEN_IN_PROGRESS )
	    {
		status = di_file->fd_racestat;
		break;
	    }
	    
	    j = 0;
	    k = 0;
	    slave_found = FALSE;
	    status = gen_Psem(&DI_sc_sem);
	    if (status != OK )
	    {
		intern_status = DI_LRU_GENPSEM_ERR;
		break;
	    }

	    for (i = 0; i < Di_nslaves; i++)
	    {
		k += Di_slave_cnt[i];
		if (di_file->fd_fd_in_slave[i] >= 0)
		{
		    /* 
		    ** only interested in slaves where the
		    ** file is already open - if we don't find
		    ** such a slave or the outstanding operations
		    ** on the chosen slave exceed certain thresholds
		    ** ( see below ) then we'll try and use a new 
		    ** slave instead
		    */
		    if (!slave_found)
		    {
			slave_found = TRUE;
			j = Di_slave_cnt[i];
		    	disl->unix_fd = di_file->fd_fd_in_slave[i];
		    	disl->dest_slave_no = i;
		    }
		    else if (j > Di_slave_cnt[i])
		    {
		    	j = Di_slave_cnt[i];
		    	disl->unix_fd = di_file->fd_fd_in_slave[i];
		    	disl->dest_slave_no = i;
		    }
		}
	    }

	    status = gen_Vsem(&DI_sc_sem);
	    if ( status != OK )
	    {
		Di_fatal_err_occurred = TRUE;
		intern_status = DI_LRU_GENVSEM_ERR;
		TRdisplay(
		    "DIlru_open: Failed releasing SC semaphore, status=%x\n",
		     status);
		break;
	    }

	    if (slave_found && 
		    ( di_file->fd_racestat == SL_OPEN_IN_PROGRESS ||
		    ((j < 3) || ((3 * k) >= (2 * Di_nslaves * j)))) )
	    {
		/*
		** Increase reference count to indicate that an operation
		** is currently in progress on the file and hence we should
		** not close the file during a DIlru_flush operation
		** 
		** Do this only for LRU-able files.
		*/
		if (di_file->fd_refcnt >=0)
		{
		    op->di_file = (PTR)di_file;
		    op->di_pin = &di_file->fd_refcnt;
		    ++ di_file->fd_refcnt;
		    op->di_flags |= DI_OP_PIN;
		}

#ifdef	xDEV_TST
    		TRdisplay("DI_FILE @@ %p line %d\n", di_file, __LINE__);
#endif
		/* Release the file's mutex */
	        CS_synch_unlock( &di_file->fd_mutex );

		/*
		** Note we are returning whilst still holding an
		** event control block so the caller will have
		** to issue the DIlru_release
		*/
		return(OK);
	    }

#ifdef	xDEV_TST
	    TRdisplay("DIdebug: slave %d open %d total %d\n",
				    disl->dest_slave_no, j, k);
#endif
	    first_open_flag = FALSE;
	}

	/* 
	** get null terminated path and filename from DI_IO struct 
	*/
	MEcopy((PTR) f->io_pathname, f->io_l_pathname, (PTR) disl->buf);
	disl->buf[f->io_l_pathname] = '/';
	MEcopy((PTR) f->io_filename, f->io_l_filename, 
	       (PTR) &disl->buf[f->io_l_pathname + 1]);
	disl->buf[(f->io_l_pathname + f->io_l_filename + 1)] = EOS;
	
	/* 
	** now attempt to open the file 
	*/
	if (flags & DI_FCREATE)
	{
	    disl->unix_open_flags = (O_RDWR | O_EXCL | O_CREAT);
	    disl->pre_seek = DI_FILE_MODE;
	}
	else
	{
	    disl->unix_open_flags = O_RDWR;
	}

	/*
	** Use O_DSYNC if is exists.  This will sync the data without syncing
	** any unnecessary inode information.  It provides the smae behavior
	** as the SunOS sticky bit feature.
	*/

#ifdef O_DSYNC
	if (f->io_open_flags & DI_O_OSYNC_MASK)
	    disl->unix_open_flags |= O_DSYNC;
#endif

#if defined(O_SYNC) && !defined(O_DSYNC)
	if (f->io_open_flags & DI_O_OSYNC_MASK)
	    disl->unix_open_flags |= O_SYNC;
# if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
	else
	    f->io_open_flags |= DI_O_STICKY_UNSET;
# endif
#endif /* O_SYNC */
	    
	disl->file_op = DI_SL_OPEN;
	
	/* 
	** choose first victim slave, i.e. 
	** choose slave with fewest files open 
	*/
	j = 0;
	slave_found = FALSE;
	status = gen_Psem(&DI_sc_sem);
	if (status != OK )
	{
	    intern_status = DI_LRU_GENPSEM_ERR;
	    break;
	}
	for (i = 0; i < Di_nslaves; i++)
	{
	    if (first_open_flag)
	    {
		di_file->fd_fd_in_slave[i] = -1;
	    }

	    if (!slave_found)
	    {
		if (di_file->fd_fd_in_slave[i] == -1)
		{
		    /* 
		    ** first slave which hasn't already 
		    ** got this file open 
		    */
		    slave_found = TRUE;
		    j = Di_slave_open[i];
		    disl->dest_slave_no = i;
		}
	    }
	    else if ((j > Di_slave_open[i]) && 
			(di_file->fd_fd_in_slave[i] == -1))
	    {
		/* 
		** this slave hasn't got it open and has less
		** open files than the previous victim 
		*/
		j = Di_slave_open[i];
		disl->dest_slave_no = i;
	    }
	}
	status = gen_Vsem(&DI_sc_sem);
	if ( status != OK )
	{
	    Di_fatal_err_occurred = TRUE;
	    intern_status = DI_LRU_GENVSEM_ERR;
	    TRdisplay(
		"DIlru_open: Failed releasing SC semaphore, status=%x\n",
		 status);
	    break;
	}

	if (!slave_found)
	{
	    TRdisplay("DIpanic: open in new slave\n");
#ifdef	xDEV_TST
    	    TRdisplay("DI_FILE @@ %p line %d\n", di_file, __LINE__);
#endif
	    /* Return fd to free list and release its fd_mutex */
	    DI_free_insert(&di_file);

	    status = FAIL;
	    break;
	}

	/*
	** Increase reference count to indicate that an operation
	** is currently in progress on the file.
	** This prevents the file from being selected for LRU-ing.
	**
	** Do this only for LRU-able files.
	*/
	if (di_file->fd_refcnt >=0)
	{
	    op->di_file = (PTR)di_file;
	    op->di_pin = &di_file->fd_refcnt;
	    ++di_file->fd_refcnt;
	    op->di_flags |= DI_OP_PIN;
	}

	/* Note we hold the di_file's mutex while we await the open */

	/* No longer true; DI_lru_file_open will set racestat
	** to SL_OPEN_IN_PROGRESS, release the mutex, wait for
	** the open to complete, reacquire the mutex, 
	** clear racestat, and go on about its business.
	*/

	/*
	** Try and open the file, note this routine was created
	** from a great chunk of code which used to be here, 
	** hence it makes no sense to try and call it from
	** anywhere else.
	*/
        status = DI_lru_file_open( flags, disl, di_file, op, f, err_code,
				  &reserve_ev_flag, &intern_status );
	if ( status != OK )
	    break;

	disl->lru_close = -1;
	disl->pre_seek = -1;
	++Di_slave_open[disl->dest_slave_no];
	di_file->fd_fd_in_slave[disl->dest_slave_no] = disl->unix_fd;

	/* Count a lru_open */
	f->io_stat.lru_open++;
	htb->htb_stat.opens++;
	if (CSadjust_counter(&htb->htb_stat.fds_open,1) > htb->htb_stat.max_fds)
	    htb->htb_stat.max_fds = htb->htb_stat.fds_open;

    } while ( FALSE );

    /* Release the file's mutex */
    if (di_file)
	CS_synch_unlock( &di_file->fd_mutex );

    if ( status == OK )
	return( OK );

    /*
    ** Error condition so release any resources we may have aquired and
    ** set up the return error
    */
    {
	CL_ERR_DESC     lerr_code;
	STATUS		lstatus;

        if ( reserve_ev_flag)
	    (VOID ) DIlru_release(op, &lerr_code);

        DIlru_set_di_error( &status, err_code,
			    intern_status, DILRU_OPEN_ERR );

    	return( status );
    }
}

/*{
** Name: DI_lru_inproc_open - insure that input file exists in open file cache
**
** Description:
**	Given a pointer to a DI_IO struct search for corresponding entry
**	in the open file cache.  If the file does not exist in the cache then
**	open it and place it in the cache.  If necessary call DI_lru_free
**	to free up a spot in the cache.
**
**	It is assumed that caller has correctly initialized the DI_IO
**	structure.  The DI_IO structure must have the path, pathlength,
**	filename, and filename length filled in.  It must also contain
**	the unique numeric id assigned by DIlru_uniq.  All the previous
**	initialization should take place in DIopen, and the information
**	is stored and saved in the DI_IO struct passed in by the caller
**	on each DI operation.
**
**	If the free list is empty another DI_ENTRY_NUM entries will be
**	allocated and put onto the free list.  The number of entries 
**	allocated continues to grow as long as new files can be open'd
**	successfully.  When opens begin to fail then entries will be
**	closed and put on the free list to be reused.
**
** Inputs:
**	f		 pointer to di file descriptor information.
**	create_flag	 Indicates if file should be created and open file
**			 to it returned.  Used mostly for temporary files.
**
** Outputs:
**	op		 disk operation.
**	err_code	 system open() error info.
**	unix_fd		 upon exit copy of file descriptor of open file
**			 returned.  Only guaranteed to be valid for current
**			 DI call.
**
**    Returns:
**        OK
**	  DI_CANT_OPEN   Can't open a file due to system limit 
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    06-mar-87 (mmm)
**          Created new for jupiter.
**    06-feb-89 (mikem)
**	    Now returns a CL_ERR_DESC from DIlru_inproc_open().
**    07-mar-89 (roger)
**	    Initialize err_code in case failing w/o setting it.
**    02-feb-90 (jkb)
**	    Use daveb's IIdio routines to encapsultate sequent/direct io magic.
**    31-may-90 (blaise)
**          Integrated changes from termcl:
**              Be sure reference count is set to zero when allocating
**              DI_FILE_DESC's.
**    27-jul-91 (mikem)
**          bug #38872
**          DIopen() now sets io_open_flags to DI_O_OSYNC_MASK if an O_SYNC
**          open() is required by the system.  This mask is now checked rather
**          than DI_SYNC_MASK which is only used by callers of DIopen() to
**          indicate that some available syncing mechanism must be used.
**
**	12-dec-1991 (bryanp)
**	    It's not safe to depend on 'errno' when using IIdio* routines to
**	    access files. The value of errno is not dependable (for example,
**	    it may be zero even if the IIdio* call failed). Instead, the value
**	    of errno in the CL_ERR_DESC should be used, as that is the errno
**	    value which describes the reason why the call failed.
**    30-nov-1992 (rmuth)
**	    - Add error checking.
**	    - Do not put the Di_fatal_err_occurred check in this routine as
**	      it can be called by other DIlru routines and hence may stop
**	      the trace back message being issued.
**    30-feb-1993 (rmuth)
**	    Use DI_FILE_MODE for file modes, was hardcoded to 0777 before.
**    12-may-1994 (rog)
**	    Use O_DSYNC if it exists instead of O_SYNC.
**    01-jun-1995 (canor01)
**	    changed errno to errnum for reentrancy
**    29-oct-1996 (canor01)
**	    When running out of file descriptors, Solaris may possibly
**	    return 0 from open() instead of returning -1 and setting
**	    errno to EMFILE.  Treat the situations the same.
**	02-sep-1997 (wonst02)
**	    Added code to return DI_ACCESS for file open access error.
**	07-Oct-1997 (jenjo02)
**	  o Initially search cache using SHARED lock on DI_misc_sem.
**	    If we fail to find it, upgrade to EXCL and research before
**	    creating a new entry (OS threads only).
**	  o Add new entry to cache before releasing the semaphore and
**	    proceding with the open. That way other threads will find
**	    the newly created cache entry and will wait for the open
**	    to complete instead of creating a duplicate entry!
**	    Defer putting the new entry on the lru queue until the file
**	    has been successfully opened to prevent other threads from
**	    closing this file while the open is in progress.
**	  o After waiting for DI_O_OPEN_IN_PROGRESS, the di_file entry may
**	    have been recycled and now belongs to another file. Check the
**	    unique file id after waiting and if different, re-search the
**	    cache.
**	10-Nov-1997 (jenjo02)
**	    Backed out use of SHARED semaphore to search cache. If the
**	    file is found in the cache, DIlru_search() moves it to the 
**	    top of the LRU queue. Two threads doing this at once will
**	    sometimes corrupt the LRU queue, whose stability depends on 
**	    an EXCL DI_misc_sem.
**	03-Dec-1997 (jenjo02)
**	    Backed out bullet 2 from 07-Oct-1997 change, above. The non-slave
**	    DI code is not thread-safe enough to support multiple threads sharing the
**	    same fd/di_file. In particular, the number of references to a di_file
**	    is not maintained, so one thread can easily close or LRU-out a di_file
**	    in the process of being used by a second thread, leading to bad fd number
**	    errors. 
**
*/
static STATUS
DI_lru_inproc_open(
    DI_IO	*f,
    i4		create_flag,
    DI_OP	*op,
    CL_ERR_DESC	*err_code )
{
    DI_FILE_DESC	*di_file = (DI_FILE_DESC*)NULL;
    STATUS		big_status = OK, 
			intern_status = OK, 
			ret_status = OK;
    int			open_flag;
    i4			have_sem = FALSE;

    /* size includes null and connecting slash */

    char		pathbuf[DI_PATH_MAX + DI_FILENAME_MAX + 2];

    CL_CLEAR_ERR( err_code );

    /* 
    ** Does fd already exist?
    ** If not, create a new entry in the cache.
    */
    ret_status = DI_find_fd(f, TRUE, &di_file, err_code);

    if (di_file == (DI_FILE_DESC*)NULL)
    {
	/* File was not found and could not be added */
	return(ret_status);
    }

    if (ret_status == OK)
    {
	/* File was found in cache */

	if (create_flag)
	{
	    /* 
	    ** If we are trying to create a file and we have found
	    ** we already have it then error.
	    */
	    ret_status = DI_EXISTS;
	}
	else if (di_file->fd_racestat != OK)
	{
	    ret_status = di_file->fd_racestat;
	}
	else
	{
	    /*
	    ** Increase reference count to indicate that an operation
	    ** is currently in progress on the file.
	    ** This prevents the file from being selected as a
	    ** lru victim.
	    **
	    ** Do this only for LRU-able files.
	    */
	    if (di_file->fd_refcnt >=0)
	    {
		op->di_file = (PTR)di_file;
		op->di_pin = &di_file->fd_refcnt;
		++di_file->fd_refcnt;
		op->di_flags |= DI_OP_PIN;
	    }
	    op->di_fd = di_file->fd_unix_fd;
	}
	CS_synch_unlock(&di_file->fd_mutex);
	return(ret_status);
    }

    /* File not found but was added to cache, we hold its mutex */
    ret_status = OK;

    /*
    **
    ** Increase reference count to indicate that an operation
    ** is currently in progress on the file.
    ** This prevents the file from being selected as a
    ** lru victim.
    **
    ** Do this only for LRU-able files.
    */
    if (di_file->fd_refcnt >=0)
    {
	op->di_file = (PTR)di_file;
	op->di_pin = &di_file->fd_refcnt;
	++di_file->fd_refcnt;
	/* Set PIN flag below after the open works. */
    }

    /* 
    ** get null terminated path and filename from DI_IO struct 
    */
    MEcopy((PTR)f->io_pathname, f->io_l_pathname, (PTR) pathbuf);
    pathbuf[f->io_l_pathname] = '/';
    MEcopy((PTR) f->io_filename, f->io_l_filename, 
	   (PTR) &pathbuf[f->io_l_pathname + 1]);
    pathbuf[(f->io_l_pathname + f->io_l_filename + 1)] = '\0';

    /* 
    ** now attempt to open the file 
    */
    if (create_flag)
	open_flag = (O_RDWR | O_EXCL | O_CREAT);
    else if (f->io_mode == DI_IO_WRITE)
	open_flag = O_RDWR;
    else
	open_flag = O_RDONLY;

#ifdef O_DSYNC
    if (f->io_open_flags & DI_O_OSYNC_MASK)
	open_flag |= O_DSYNC;
#endif

#if defined(O_SYNC) && !defined(O_DSYNC)
    if (f->io_open_flags & DI_O_OSYNC_MASK)
	open_flag |= O_SYNC;
# if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
    else
	f->io_open_flags |= DI_O_STICKY_UNSET;
# endif

#endif /* O_SYNC */

    while ((di_file->fd_unix_fd = 	
	    IIdio_open( pathbuf, open_flag, DI_FILE_MODE,
					f->io_bytes_per_page, 
					&f->io_fprop,
					err_code)) <= 0)
    {
	/*
	** Note that if IIdio_open fails, it has set the reason for failure
	** into 'err_code'. Plain old 'errno' may be zero at this point, 
	** since IIdio_open may have called TRdisplay and thus reset errno.
	**
	** Thus, if you want to look at the errno which describes why the 
	** open failed, the proper thing to look at is 'err_code->errno'
	*/

	/*
	** Test to see if we have run out of file descriptors.
	** On some versions of Solaris, this can cause the
	** return from open() to be zero.  If this is the case,
	** treat it same as out of file descriptors.
	*/
	if ( err_code->errnum == EMFILE || 
	     err_code->errnum == ENFILE || 
	     di_file->fd_unix_fd == 0 )
	{
	    /*
	    ** Too many open files, close some files and try again
	    */
#ifdef xCL_ASYNC_IO
	    if (Di_async_io)                              
	    {                                             
		big_status = DIlru_aio_free( err_code );  
	    }                                             
	    else                                          
#endif /* xCL_ASYNC_IO */
		big_status = DIlru_flush( err_code );
	    if (big_status != OK )
	    {
		di_file->fd_racestat = FAIL;
		/* Reinsert fd into free list and release its fd_mutex */
		DI_free_insert(&di_file);
		break;
	    }

	    /*
	    ** Try and open file again
	    */
	    ret_status = OK;
	    continue;
	}
	else
	{
	    switch (err_code->errnum)
	    {
		case EACCES:
		    ret_status = DI_ACCESS;
		    break;
		case EEXIST:
		    ret_status = DI_EXISTS;
		    break;
		case EINVAL:
		    ret_status = DI_BADDIR;
		    break;
		case ENOENT:
		    if (create_flag)
			ret_status = DI_DIRNOTFOUND;
		    else
			ret_status = DI_FILENOTFOUND;
		    break;
		case ENOTDIR:
		    ret_status = DI_DIRNOTFOUND;
		    break;
		case ENOSPC:
		    ret_status = DI_EXCEED_LIMIT;
		    break;
#ifdef EDQUOT
		case EDQUOT:
		    ret_status = DI_EXCEED_LIMIT;
		    break;
#endif /* EDQUOT */
		default:
		    ret_status = DI_BADOPEN;
		    break;
	    }
	    
	    di_file->fd_racestat = ret_status;

	    /* Reinsert fd into free list and release fd_mutex */
	    DI_free_insert(&di_file);
	}
	break;
    }

    /*
    ** Did we open a file?
    */
    if (di_file)
    {
	/*
	** Success.
	*/
	if (di_file->fd_refcnt >=0)
	    op->di_flags |= DI_OP_PIN;

	op->di_fd = di_file->fd_unix_fd;

	/* Count a lru_open */
	f->io_stat.lru_open++;

        CS_synch_unlock( &di_file->fd_mutex );

	htb->htb_stat.opens++;
	if (CSadjust_counter(&htb->htb_stat.fds_open,1) > htb->htb_stat.max_fds)
	    htb->htb_stat.max_fds = htb->htb_stat.fds_open;

	return(OK);
    }

    if ( (Di_async_io) && (big_status == DILRU_ALL_PINNED_ERR))   
    {                                                             
	intern_status = DILRU_ALL_PINNED_ERR;                 
	big_status = DILRU_OPEN_ERR;                          
    }                                                             

    if ( big_status != OK )
	ret_status = big_status;

    /*
    ** Make sure we have cleaned everything up before
    ** we return.
    */

    if ( ret_status != OK )
	DIlru_set_di_error( &ret_status, err_code,intern_status, 
			    DILRU_INPROC_ERR );
    return(ret_status);

}

/*{
** Name: DI_find_fd - find/create fd for a file.
**
** Description:
**	Find or create open file using unique id from the DI_IO struct.
**
** Inputs:
**	f		 pointer to di file descriptor information.
**	create		 boolean, TRUE if file is to be added 
**			 to open file list if not found.
**
** Outputs:
**	di_file		 pointer to entry if found/created else
**			 set to NULL.
**
**    Returns:
**        OK
**			 
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    06-mar-87 (mmm)
**          Created new for jupiter.
**    30-nov-1992 (rmuth)
**	    Make sure DIlru self initialises.
**	22-Dec-1998 (jenjo02)
**	    If DI_FD_PER_THREAD is defined (dilru.h), open a file 
**	    descriptor for each thread needing one instead of 
**	    forcing all threads to use a single fd per file.
**	1-Jul-2005 (schka24)
**	    If we have to recycle when searching the io_fd_q (FD-per-thread
**	    only), start at the beginning.  (typo)
**	25-Jul-2005 (schka24)
**	    DI_IO pointer to fd desc in "io_uniq" is meaningless when doing
**	    fd-per-thread, because there's only one pointer slot and
**	    potentially N threads.  Depend solely on the io_fd_q.
**	    Mutex-protect the fd-q from writers while reading.  (htb list
**	    is good enough for writer-writer protection, but we don't want
**	    to lock the entire list just to check one FD queue.)
*/
static STATUS
DI_find_fd(
    DI_IO		*f,
    i4			create,
    DI_FILE_DESC	**di_file,
    CL_ERR_DESC 	*err_code )
{
    DI_FILE_DESC	*entry = (DI_FILE_DESC*)NULL;
    STATUS		status, big_status;
    i4			lru_is_locked = FALSE;

    /* Count another find request */
    htb->htb_stat.requests++;

    for (;;)
    {
	/*
	** First check if the DI_IO points to the correct
	** fd already.
	** Don't do this for fd-per-thread, as uniq_di_file is not set.
	** (It's pointless to try to set a single unprotected pointer when
	** N threads can be using the DI_IO.)
	*/
	if ( !(Di_thread_affinity)
	    && (entry = (DI_FILE_DESC*)f->io_uniq.uniq_di_file)
	    && entry->fd_uniq.uniq_di_file == (PTR)f
	    && entry->fd_uniq.uniq_seqno  == f->io_uniq.uniq_seqno
	    )
	{
	    /* Lock the di_file, check it again */
	    if (lru_is_locked)
		CS_synch_unlock(&htb->htb_fd_list_mutex);
	    CS_synch_lock(&entry->fd_mutex);
	    if (entry->fd_uniq.uniq_di_file == (PTR)f
		&& entry->fd_uniq.uniq_seqno  == f->io_uniq.uniq_seqno
		)
	    {
		/* Count a hit */
		htb->htb_stat.hits++;
		*di_file = entry;
		return(OK);
	    }
	    CS_synch_unlock(&entry->fd_mutex);
	    if (lru_is_locked)
		CS_synch_lock(&htb->htb_fd_list_mutex);
	}
# ifdef OS_THREADS_USED
	else if ( Di_thread_affinity )
	{
	    QUEUE		*p, *q;

	    q = &f->io_fd_q;
	    p = q;
	    CS_synch_lock(&f->io_fd_q_sem);

	    /* Search the DI_IO list of open fd's for an unref'd descriptor */
	    while ((p = p->q_next) != q)
	    {
		entry = (DI_FILE_DESC*)((char*)p - 
		    CL_OFFSETOF(DI_FILE_DESC, fd_io_q));
		
		if ( entry->fd_refcnt == 0 &&
		     entry->fd_unix_fd != -1 )
		{
		    /* Lock the entry, recheck it */
		    CS_synch_unlock(&f->io_fd_q_sem);
		    CS_synch_lock(&entry->fd_mutex);

		    if ( entry->fd_refcnt == 0 &&
			 entry->fd_unix_fd != -1 &&
			 entry->fd_uniq.uniq_di_file == (PTR)f &&
			 entry->fd_uniq.uniq_seqno == f->io_uniq.uniq_seqno )
		    {
			/* Count a hash hit */
			htb->htb_stat.io_hits++;
			*di_file = entry;
			return(OK);
		    }
		    /* Entry changed while waiting for mutex, restart search */
		    CS_synch_unlock(&entry->fd_mutex);
		    CS_synch_lock(&f->io_fd_q_sem);
		    p = q;
		}
		else 
		if (entry->fd_uniq.uniq_di_file != (PTR)f)
		    p = q;
	    }
	    CS_synch_unlock(&f->io_fd_q_sem);
	}
# endif /* OS_THREADS_USED */

	/* Didn't find it. */
	entry = (DI_FILE_DESC *)NULL;
	status = FAIL;

	/* 
	** FD for this file does not exist.
	**
	** If instructed, allocate a new one.
	*/
	if (create)
	{
# ifdef OS_THREADS_USED
	    if ( Di_thread_affinity )
	    {
		/* 
		** Go ahead and lock the fd queue EXCL. There's a chance
		** that another thread may release a usable
		** fd while we wait for the mutex, and we could retry
		** the fd search to try to find it, but why bother. 
		** All this means is that we may create an extra fd 
		** for the file, but it'll be added to the pool of fds 
		** for the file and will eventually get reused or lru'd out.
		*/
		CS_synch_lock(&htb->htb_fd_list_mutex);
	    }
	    else
# endif /* OS_THREADS_USED */
	    if (lru_is_locked == FALSE)
	    {
		/* 
		** Lock the fd list, then recheck in case another
		** thread created this fd while we waited
		** for its mutex.
		*/
		lru_is_locked = TRUE;
		CS_synch_lock(&htb->htb_fd_list_mutex);
		continue;
	    }

	    big_status = DI_free_remove(&entry, err_code);
	    if (big_status == OK)
	    {
		/* initialize the DI_FILE_DESC */

		entry->fd_uniq.uniq_di_file 	= (PTR)f;
		entry->fd_uniq.uniq_seqno	= f->io_uniq.uniq_seqno;
		entry->fd_racestat 		= OK;
		entry->fd_state 		= FD_IN_USE;
		entry->fd_unix_fd		= -1;
		entry->fd_refcnt 		= 0;

# ifdef OS_THREADS_USED
		if ( Di_thread_affinity )
		{
		    /* Insert last on DI_IO's list of fds */
		    CS_synch_lock(&f->io_fd_q_sem);
		    QUinsert(&entry->fd_io_q, f->io_fd_q.q_prev);
		    CS_synch_unlock(&f->io_fd_q_sem);
		    /* All fd's are LRU-able when running fd-per-thread,
		    ** because we need the refcount to tell us when
		    ** someone else is using the fd.  The only caller
		    ** that currently asks for non-LRU is the transaction
		    ** log, and that is just an optimization, not a
		    ** necessity.
		    */
		}
		else
# endif /* OS_THREADS_USED */
		{
		    /* Point the DI_IO to this DI_FILE_DESC */
		    f->io_uniq.uniq_di_file = (PTR)entry;
		    /*
		    ** If this file is not LRU-eligible, set the
		    ** reference count to -1.
		    */
		    if (f->io_open_flags & DI_O_NOT_LRU_MASK)
			entry->fd_refcnt = -1;
		}

		/* insert first in the lru queue */
		QUinsert(&entry->fd_q, &htb->htb_fd_list);
	    }
	    else
		status = big_status;

	    CS_synch_unlock(&htb->htb_fd_list_mutex);
	}

	*di_file = entry;
	return(status);
    }
}

/*{
** Name: DI_free_insert - insert a DI_FILE_DESC onto end offree list
**
** Description:
**	Insert a fd last on the free list.
**
** Inputs:
**	di_file		 pointer to pointer to fd with its
**			 fd_mutex locked.
**
** Outputs:
**	di_file		 mutex is released, pointer is nullified.
**	none.
**
**    Returns:
**        OK
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	19-Dec-1997 (jenjo02)
**	    created.
**	26-Jul-2005 (schka24)
**	    Lock readers from DI_IO's fd q; clobber unix fd.
*/
static VOID
DI_free_insert(
    DI_FILE_DESC	**di_file)
{
    DI_IO *f;

    CS_synch_lock(&htb->htb_fd_list_mutex);

# ifdef OS_THREADS_USED
    if ( Di_thread_affinity )
    {
	/* Remove fd from DI_IO's list */
	f = (DI_IO *) (*di_file)->fd_uniq.uniq_di_file;
	if (f != NULL)
	{
	    CS_synch_lock(&f->io_fd_q_sem);
	    QUremove(&(*di_file)->fd_io_q);
	    CS_synch_unlock(&f->io_fd_q_sem);
	}
    }
# endif /* OS_THREADS_USED */

    /* Relocate fd to the end of the lru list */
    QUremove(&(*di_file)->fd_q);
    QUinsert(&(*di_file)->fd_q, htb->htb_fd_list.q_prev);

    CS_synch_unlock(&htb->htb_fd_list_mutex);

    /* Annotate the fd to show that it's free */
    (*di_file)->fd_uniq.uniq_di_file = (PTR)NULL;
    (*di_file)->fd_uniq.uniq_seqno   = 0;
    (*di_file)->fd_state = FD_FREE;
    (*di_file)->fd_refcnt = 0;
    (*di_file)->fd_unix_fd = -1;

    /* release fd's mutex */
    CS_synch_unlock(&(*di_file)->fd_mutex);

    /* Clobber caller's di_file pointer */
    *di_file = (DI_FILE_DESC*)NULL;

    return;
}

/*{
** Name: DI_free_remove - Remove a DI_FILE_DESC from free list
**
** Description:
**	Remove a fd from the free list.
**
** Inputs:
**	none.
**
** Outputs:
**      di_file if one was found, with its fd_mutex locked EXCL.
**
**    Returns:
**      OK if di_file returned, error if none can be allocated.
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	19-Dec-1997 (jenjo02)
**	    created.
**	    Caller must hold htb_fd_list_mutex EXCL.
**	 2-sep-1999 (hayke02)
**	    We now call DIlru_flush() if DI_lru_allocate_more_slots()
**	    returns DILRU_CACHE_LIMIT. This change fixes bug 98622.
**	03-Oct-2000 hanal04 Bug 102756 INGSRV 1279
**          Release the htb_fd_list_mutex before requesting the fd_mutex.
**          Once we have the fd_mutex we can re-acquire the htb_fd_list_mutex
**          (without the risk of mutex deadlock between the two mutexes).
**          However, we must then check that the list did not change
**          during the window in which we did not hold the htb_fd_list_mutex.
**	14-Oct-1998 (jenjo02)
**	    01-sep-1998 change to DI_free_remove caused a mutex deadlock
**	    because the LRU list mutex is already held and DIlru_flush()
**	    attempts to reacquire it.
**
*/
static STATUS
DI_free_remove( 
    DI_FILE_DESC **di_f,
    CL_ERR_DESC *err_code )
{
    STATUS		status, intern_status;
    DI_FILE_DESC	*di_file;
    QUEUE		*p, *q, *s;

    q = &htb->htb_fd_list;

    do
    {
	p = q->q_prev;

	/* From the back of the LRU queue, find the first free entry */
	while (p != q)
	{
	    di_file = (DI_FILE_DESC*)p;

	    if (di_file->fd_state == FD_FREE)
	    {
		/* Must not request the fd_mutex whilst holding the 
		** htb_fd_list_mutex. Release the htb_fd_list_mutex
		** Then request the fd_mutex followed by the 
		** htb_fd_list_mutex.
		** We must then check the following;
		** 1)The selected entry is still on the list (s == p)
		** 2) The fd_state is still FD_FREE
		*/
		CS_synch_unlock(&htb->htb_fd_list_mutex);
		CS_synch_lock(&di_file->fd_mutex);
		CS_synch_lock(&htb->htb_fd_list_mutex);
                q = &htb->htb_fd_list;
		s = q->q_prev;
		while ((s != p) && (s != q))
		{
                    s = s->q_prev;
		}
		if ((di_file->fd_state == FD_FREE) && (s == p))
		{
		    /* Remove entry from the lru list */
		    QUremove(&di_file->fd_q);
		    *di_f = di_file;
		    return(OK);
		}
		/* Entry either changed while we waited for its mutex
		** or it was removed from the list before we could
		** re-acquire the list mutex. Start over from the back of
		** the queue.
		*/
		CS_synch_unlock(&di_file->fd_mutex);
		p = q->q_prev;
	    }
	    else
		p = p->q_prev;
	}

	/* No free entries. Try to allocate some more */
	status = DI_lru_allocate_more_slots( &intern_status );
	if ( status != OK )
	{
	    if ( status == DILRU_CACHE_LIMIT )
	    {
		/* Release LRU list sem. Both functions take and release it. */
		CS_synch_unlock(&htb->htb_fd_list_mutex);
#ifdef xCL_ASYNC_IO
		if ( Di_async_io )
		    status = DIlru_aio_free(err_code);
		else
#endif /* xCL_ASYNC_IO */
		    status = DIlru_flush( err_code );
		/* Reacquire LRU list sem. */
		CS_synch_lock(&htb->htb_fd_list_mutex);
	    }
	}
    } while (status == OK);

    *di_f = (DI_FILE_DESC*)NULL;
    return(status);
}

/*{
** Name: DIlru_uniq - Assign a unique id to file passed in.
**
** Description:
**	Given a DI_IO assign it a unique id that will be used by the
**	open file descriptor cache.  
**
**	Current implementation relies only on local memory.  For each
**	new file the DI_UNIQUE_ID struct is assigned the PID of the
**	process opening the file (makes id unique to the process) and
**	then a sequence number is assigned from the current process.
**
**	It is necessary that this id be unique throughout a server.  In
**	the future we may use shared memory for this.  For now we use
**	PID, time in seconds of startup of process, and sequence number.
**	There is currently a request into SCF to change the specification
**	of CS such that CSsid() would return an id that is unique throughout
**	the lifetime of the server. (Currently the unique id is made up of
**	the pid, the time to seconds and a per session counter - this is
**	not guaranteed unique but the process id count would have to loop
**	in less than a second to make it not unique).
**
** Inputs:
**	f		 pointer to di file descriptor information.
**
** Outputs:
**
**    Returns:
**        OK
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    06-mar-87 (mmm)
**          Created new for jupiter.
**    30-nov-1992 (rmuth)
**	    Make sure DIlru self initialises.
**	    Add err_code parameter.
**	07-Oct-1997 (jenjo02)
**	    Added semaphore protection of Di_seqno to preclude multiple
**	    concurrent threads from being assigned the same "unique" id.
**	19-Feb-1998 (jenjo02)
**	    Moved static Di_seqno inside of htb as htb_seqno.
**	1-Jul-2005 (schka24)
**	    Use thread-safe increment of seqno.
**
*/
STATUS
DIlru_uniq(
    DI_IO	*f,
    CL_ERR_DESC *err_code )
{

    /*
    ** See of we need to initialize the hash control block
    */
    if (!(htb_initialized))
    {
	STATUS	status, intern_status = OK;
	if (status = DI_lru_check_init( &intern_status, err_code ))
	    return(status);
    }

    /*
    ** If a fatal error occured in DI at some time then bring down 
    ** the server to prevent possible corruption.
    */
    if ( Di_fatal_err_occurred )
    {
	TRdisplay(
	   "DIlru_uniq: Failing server due to unrecoverable DI error\n");
	PCexit(FAIL);
    }

    /* generate a unique key for this file */
    f->io_uniq.uniq_di_file = (PTR)NULL;
    f->io_uniq.uniq_seqno   = CSadjust_counter(&htb->htb_seqno, 1);

    return(OK);
}

/*{
** Name: DIlru_dump - Dump LRU information to the log (debug)
**
** Description:
**
**	Dumps LRU statistics and lists to the log as a debugging aid.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**    Returns:
**	void
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	20-Jan-1998 (jenjo02)
**	    Added this comment, removed taking of DI_misc_sem.
**
*/
VOID
DIlru_dump( VOID )
{
    QUEUE		*p, *q;
    DI_FILE_DESC	*di_file;
    i4			i;

    TRdisplay("\n%@ ...DI State...\n");
    TRdisplay("sizeof(DI_FILE_DESC) %d\n", sizeof(DI_FILE_DESC));
    TRdisplay("number of DI_FILE_DESC %d\n", htb->htb_fd_list_count);
    TRdisplay("requests %d\n", htb->htb_stat.requests);
    TRdisplay("hits %d\n", htb->htb_stat.hits);
    TRdisplay("io hits %d\n", htb->htb_stat.io_hits);
    TRdisplay("max fds open %d\n", htb->htb_stat.max_fds);
    TRdisplay("fd opens %d\n", htb->htb_stat.opens);
    TRdisplay("fds open now %d\n", htb->htb_stat.fds_open);
    TRdisplay("fds lru'd %d\n", htb->htb_stat.lru_closes);

    TRdisplay("\nlru list:\n");
    q = &htb->htb_fd_list;
    p = q->q_next;
    while (p != q)
    {
	di_file = (DI_FILE_DESC *)p;
	/* Quit when we hit free entries */
	if (di_file->fd_state == FD_FREE)
	    break;
	TRdisplay("entry %p\n", di_file);
	TRdisplay("\tseqno: %x\n", di_file->fd_uniq.uniq_seqno);
	TRdisplay("\tdi_io: %p\n", di_file->fd_uniq.uniq_di_file);
	TRdisplay("\tstate: %w\n", "FREE,IN_USE", di_file->fd_state);
	TRdisplay("\trefcnt: %d\n", di_file->fd_refcnt);
	if (Di_slave)
	{
	    TRdisplay("\tfds:");
	    for (i = 0; i < Di_nslaves; i++)
		TRdisplay(" %d", di_file->fd_fd_in_slave[i]);
	    TRdisplay("\n");
	}
	else
	{
	    TRdisplay("\tfd: %d\n", di_file->fd_unix_fd);
	}
	p = p->q_next;
    }

    /* The free list picks up where the lru queue left off */
    TRdisplay("\nfree list:\n");
    while (p != q)
    {
	di_file = (DI_FILE_DESC *) p;
	TRdisplay("entry %p\n", di_file);
	TRdisplay("\tseqno: %x\n", di_file->fd_uniq.uniq_seqno);
	TRdisplay("\tdi_io: %p\n", di_file->fd_uniq.uniq_di_file);
	TRdisplay("\tstate: %w\n", "FREE,IN_USE", di_file->fd_state);
	TRdisplay("\trefcnt: %d\n", di_file->fd_refcnt);
	if (Di_slave)
	{
	    TRdisplay("\tfds:");
	    for (i = 0; i < Di_nslaves; i++)
		TRdisplay(" %d", di_file->fd_fd_in_slave[i]);
	    TRdisplay("\n");
	}
	else
	{
	    TRdisplay("\tfd: %d\n", di_file->fd_unix_fd);
	}
	p = p->q_next;
    }
}

/*{
** Name: DI_complete() - Event completion for DI.
**
** Description:
**	Used as the completion routine for DI events.  Causes
**	CSresume when a DI operation is finished.
**	Special NOSUSPEND option will just release the CS event block.
**
** Inputs:
**	evcb	Called as a completion routine.
**
** Outputs:
**	none
**
**	Returns:
**	    OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-Feb-89 (anton)
**          Written.
**	13-Sep-90 (anton)
**	    Use defered resume for MCT
**	26-jul-93 (mikem)
**	    return status to match CSslave_init() prototype.
*/
STATUS
DI_complete(
    CSEV_CB	*evcb )
{
    register DI_SLAVE_CB *disl;

    disl = (DI_SLAVE_CB *)evcb->data;
    if (disl->file_op & DI_NOSUSPEND)
	CSfree_event(evcb);
    else
	CSdef_resume(evcb);
    
    return(OK);
}


/*{
** Name: DIlru_pin - pin existing file descriptor
**
** Description:
**	This routine is used by gather-write, when it receives a
**	write request for a file that the thread already has queued.
**	It's OK for both requests to use the same Unix file descriptor
**	in this case.  (Indeed, it's highly desirable.)  The caller
**	will feed us a DI_OP that points to an existing, allocated, pinned
**	file descriptor block.  Our job is to increment the reference
**	count, so that the caller can be lazy and simply lru-release
**	every request it has when all the I/O is done.
**
**	We don't pre-check that the DI_OP isn't already pinned, although
**	we could.
**
**	This routine is only used in an OS-threaded server, and is
**	a no-op for an internal threads only server.
**
** Inputs:
**	op		 pointer to DI_OP operation info.
**
** Outputs:
**	none
**
** Side Effects:
**        none
**
** History:
**	25-Aug-2005 (schka24)
**	    Written for gather write and fd-per-thread.
**
*/
void
DIlru_pin(DI_OP	*op)
{

#ifdef OS_THREADS_USED

    DI_FILE_DESC *di_file = (DI_FILE_DESC *) op->di_file;

    /* Note: don't change this to CSadjust-counter unless you change
    ** refcount manipulation everywhere!
    */
    CS_synch_lock(&di_file->fd_mutex);
    ++ *op->di_pin;
    CS_synch_unlock(&di_file->fd_mutex);
    op->di_flags |= DI_OP_PIN;

#endif

} /* DIlru_pin */

/*{
** Name: DIlru_release - release DI event control block
**
** Description:
**
** Inputs:
**	op		 pointer to event control block.
**
** Outputs:
**
**    Returns:
**        OK
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-October-1992 (rmuth)
**	    Added header, make it return a status.
**    30-nov-1992 (rmuth)
**	    Add err_code parameter.
**	18-Feb-1998 (jenjo02)
**	    Added mutex to di_file which we need to take 
**	    in order to decrement the pin count.
**	26-Jul-2005 (schka24)
**	    Use adjust-counter instead, can be faster.
**	24-Aug-2005 (schka24)
**	    Oops, above was a dum-dum.  The ref-count is incremented in
**	    various places where the di-file mutex is held, but CSadjust-
**	    counter is not used.  The di-file mutex is NOT held at entry
**	    to this routine.  We either need to use CSadjust-counter
**	    everywhere, or mutex everywhere, so go back to the latter.
**
*/
STATUS
DIlru_release(
    register DI_OP	*op,
    CL_ERR_DESC *err_code )
{
    STATUS	status = OK;

    if (op->di_flags & DI_OP_RESV)
    {
	status = CSfree_event(op->di_csevcb);
	if (status != OK )
	{
	    DIlru_set_di_error( &status, err_code, 
				DI_LRU_CSFREE_EV_ERR, DILRU_RELEASE_ERR );
	}
    }

    if (status == OK && op->di_flags & DI_OP_PIN)
    {
	DI_FILE_DESC *di_file = (DI_FILE_DESC *) op->di_file;

	CS_synch_lock(&di_file->fd_mutex);
	--*op->di_pin;
	CS_synch_unlock(&di_file->fd_mutex);
	op->di_flags &= ~(DI_OP_PIN);
    }

    return(status);
}


/*{
** Name: DIlru_flush - LRU-close off n-files 
**
** Description:
**
** Inputs:
**	none
**
** Outputs:
**
**    Returns:
**        STATUS
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	18-Feb-1998 (jenjo02)
**	   Added these comments.
**	   Utilise new mutexing scheme.
**	   Determine number of files to close instead of using a
**	   fixed value.
**	30-Sep-2005 (jenjo02)
**	   While we were perhaps unable to close any files,
**	   perhaps some other threads were able to.
*/
STATUS
DIlru_flush(
    CL_ERR_DESC *err_code )
{
    DI_FILE_DESC	*di_file;
    QUEUE		*p, *q;
    STATUS		ret_status = OK, intern_status;
    i4			number_to_close;
    u_i4		current_lru_count;

    /*
    ** Save the current number of files closed by LRU activity.
    ** If this number is different after we wait for the mutex,
    ** then concurrent LRU-ing has happened, so we'll just return
    ** with the hope that sufficient FDs have been freed up.
    */
    current_lru_count = htb->htb_stat.lru_closes;

    CS_synch_lock(&htb->htb_fd_list_mutex);

    /* If some file(s) have been LRU'd, return optimistically */
    if (current_lru_count != htb->htb_stat.lru_closes)
    {
	CS_synch_unlock(&htb->htb_fd_list_mutex);
	return(OK);
    }

    /* 
    ** Close up to 1/8th of the current open and unreferenced files.
    */
    number_to_close = (htb->htb_stat.fds_open / 8) ? htb->htb_stat.fds_open / 8
					      : 1;

    q = &htb->htb_fd_list;
    p = q->q_prev;

    while (number_to_close && ret_status == OK && p != q)
    {
	di_file = (DI_FILE_DESC *)p;
	if (di_file->fd_state == FD_IN_USE && di_file->fd_refcnt == 0)
	{
	    /* Release the list, lock the file, recheck */
	    CS_synch_unlock(&htb->htb_fd_list_mutex);
	    CS_synch_lock(&di_file->fd_mutex);
	    /*
	    ** By the time we get the mutex, the di_file
	    ** may be closed, or open and referenced.
	    */
	    if (di_file->fd_state == FD_IN_USE && di_file->fd_refcnt == 0)
	    {
		/*
		** Try to close the file
		*/
		ret_status = DI_lru_file_close( &di_file, err_code,
					 &intern_status);
		number_to_close--;
		CSadjust_counter(&htb->htb_stat.lru_closes,1);
	    }
	    if (di_file)
		CS_synch_unlock(&di_file->fd_mutex);
	    CS_synch_lock(&htb->htb_fd_list_mutex);
	    p = q->q_prev;
	}
	else
	    p = p->q_prev;
    }

    /* Release the fd list */
    CS_synch_unlock(&htb->htb_fd_list_mutex);

    if ( ret_status == OK && current_lru_count == htb->htb_stat.lru_closes )
    {
	/* No files were closed */
	ret_status = DILRU_ALL_PINNED_ERR;
    }

    if ( ret_status != OK )
    {
	DIlru_set_di_error( &ret_status, err_code, intern_status,
			    DILRU_RELEASE_ERR);
    }

    return( ret_status );
}

/*{
** Name: DIlru_set_di_error - formats err_code to pass back failure info.
**
** Description:
**
**	This routine tries to pass back as much usefull information in the 
**	err_code structure on the failure as possible. 
**
** Inputs:
**	ret_status	The error returned from DI.
**	intern_err      Value to set err_code.intern to.
**
** Outputs:
**	err_code	ptr to err_code to return to called of DI
**	meaningfull_err	Error message to set ret_val to if ret_val is
**			the generic FAIL message.
**
**    Returns:
**        OK
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-October-1992 (rmuth)
** 	    Created.
**
*/
VOID
DIlru_set_di_error(
    i4     *ret_status,
    CL_ERR_DESC *err_code,
    i4     intern_err,
    i4     meaningfull_err )
{

    err_code->intern = intern_err;

    if ( *ret_status == FAIL )
	*ret_status = meaningfull_err;

    return;
}

/*{
** Name: DI_lru_check_init - Make sure DILRU has been initialised if not
**			    initialise it.
**
** Description:
**
** Inputs:
**	Number		Index into Di_slave_cnt array
**
** Outputs:
**	intern_status	Internal error indicating reason for failure.
**
**    Returns:
**        OK
**	  DI_LRU_PSEM_ERR
**	  DI_LRU_VSEM_ERR
**	  A range of DIlru errors from DIlru_init.
**
**    Exceptions:
**        none
**
** Side Effects:
**	  If all ok it returns leaving an outstanding P operation.
**
** History:
**    30-nov-1992 (rmuth)
** 	    Created.
**	07-Oct-1997 (jenjo02)
**	    Adjust check so that we don't have to take the 
**	    semaphore in the normal (already-initialized) case.
*/
static STATUS
DI_lru_check_init( 
    STATUS	*intern_status,
    CL_ERR_DESC *err_code )
{
    STATUS	status = OK;

    /*
    ** See of we need to initialize the hash control block
    */
    if (htb_initialized)
	return(OK);

    if (htb)
    {
	CSp_semaphore(SEM_EXCL, &DI_misc_sem);
	if (!htb_initialized)
	{
	    /* Initilisation problem so just bail */
	    Di_fatal_err_occurred = TRUE;
	    TRdisplay("Error occured Initialising DI structures\n");
	    status = FAIL;
	}
	CSv_semaphore(&DI_misc_sem);
    }
    else
    {
	/* 
	** Initialise DILRU 
	*/
	status = DIlru_init( &htb, err_code );
    }

    return( status );
		
}

/*{
** Name: DI_lru_file_open  	Open a file in a slave
**
** Description:
**	This routine was created from a great chunk of code in DIlru_open
**	it makes no sense to call it from anywhere else when it is called
**	whilst holding the DI_misc_sem and an Event if an error occurs
**	it may not hold these when it exits...it uses the boolean flags
**	passed into it to indicate this to the caller. This is pretty
**	yuk but dilru_open was over 600 lines and a major headache to
**	read.
**
** Inputs:
**	flags		Indicates if we should retry the open if it fails
**			due to too many files being open.
**	disl		Slave event control block.
**	di_file		File we are trying to open.
**
** Outputs:
**	intern_status	Internal error indicating reason for failure.
**	reserve_ev_flag Boolean indicating if we still are reserving
**			an event.
**
**    Returns:
**        OK
**	  Range of DILRU errors.
**
**    Exceptions:
**        none
**
** Side Effects:
**
** History:
**    30-nov-1992 (rmuth)
** 	    Created.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	08-Oct-1997 (jenjo02)
**	    After picking a victim file to close, make sure that the possibly
**	    new slave we pick hasn't already opened the file we're trying to
**	    open.
**	20-Jan-1998 (jenjo02)
**	    Caller must hold di_file's fd_mutex.
**	14-Oct-2005 (jenjo02)
**	    Do not hold fd_mutex while waiting for the Slave open.
**	    We got away with this when fd_mutex was a CS_SEMAPHORE
**	    as a blocked session will CSswitch; blocks on
**	    CS_SYNCH block the process and the Slave-opening session
**	    can't get dispatched when the open completes.
*/
static STATUS
DI_lru_file_open(
    i4				flags,
    register DI_SLAVE_CB	*disl,
    DI_FILE_DESC		*di_file,
    DI_OP			*op,
    DI_IO			*f,
    CL_ERR_DESC			*err_code,
    bool			*reserve_ev_flag,
    STATUS			*intern_status )
{

    DI_FILE_DESC	*vic_file;
    STATUS    		ret_status = OK, big_status;
    QUEUE		*q, *p;
    i4			i;
    bool      		try_open_again;
    bool      		open_failed;
    bool		flush_needed;
    bool		victim_found;

    do  
    {
	try_open_again = FALSE;
	open_failed    = FALSE;
	flush_needed   = FALSE;
	victim_found   = FALSE;

	/*
	** Issue a request to the slave to open the file
	*/
        big_status =DI_inc_Di_slave_cnt(disl->dest_slave_no, intern_status );
	if (big_status != OK )
	    break;

	CL_CLEAR_ERR( &disl->errcode );

	/*
	** Tell others a Slave open is in progress and release
	** the mutex. It's bad form to hold a mutex while
	** waiting for an out-of-process experience.
	*/
	di_file->fd_racestat = SL_OPEN_IN_PROGRESS;
	CS_synch_unlock(&di_file->fd_mutex);

	ret_status = 
	    DI_send_ev_to_slave(op, disl->dest_slave_no, intern_status);

	CS_synch_lock(&di_file->fd_mutex);
	di_file->fd_racestat = OK;

	big_status = DI_dec_Di_slave_cnt(disl->dest_slave_no,intern_status );
	if (big_status != OK )
	    break;

	/*
	** Make sure we made request to slaves ok
	*/
	if ( ret_status != OK )
	    break;

	/* Copy updated file properties from slave back to DI_IO */
	FPROP_COPY(disl->io_fprop,f->io_fprop);

	/*
	** Check status of open request
	*/
	if ( (ret_status = disl->status) != OK)
	{
	    STRUCT_ASSIGN_MACRO( disl->errcode, *err_code);

	    /*
	    ** If we failed as too many open files and the caller requested 
	    ** retry then try and close a file then try to open the file
	    ** again, otherwise deal with error and exit
	    */
	    if ( !(flags & DI_NORETRY) &&
	          ( disl->errcode.errnum == EMFILE || 
		    disl->errcode.errnum == ENFILE) )
	    {

		/* 
		** get new victim from lru 
		*/
		CS_synch_lock(&htb->htb_fd_list_mutex);
		q = &htb->htb_fd_list;
		p = q->q_prev;

		/*
		** Find a victim file in our slave to close, 
		** this is one which is open and currently not being 
		** referenced.
		**
		** It's our slave (disl->dest_slave_no) which has
		** run out of file descriptors, so only close
		** an unreferenced file opened by it.
		*/
		victim_found = FALSE;

		while (p != q)
		{
		    vic_file = (DI_FILE_DESC *)p;

		    if (vic_file->fd_state == FD_IN_USE &&
		        vic_file->fd_refcnt == 0 &&
			vic_file->fd_fd_in_slave[disl->dest_slave_no] >= 0)
		    {
			/* Unlock the lru list, lock the di_file, check again */
			CS_synch_unlock(&htb->htb_fd_list_mutex);
			CS_synch_lock(&vic_file->fd_mutex);
			if (vic_file->fd_state == FD_IN_USE &&
			    vic_file->fd_refcnt == 0 &&
			    vic_file->fd_fd_in_slave[disl->dest_slave_no] >= 0)
			{
			    victim_found = TRUE;
			    break;
			}
			CS_synch_unlock(&vic_file->fd_mutex);
			CS_synch_lock(&htb->htb_fd_list_mutex);
			p = q->q_prev;
		    }
		    else
			p = p->q_prev;
		}


		if ( victim_found )
		{
		    /* 
		    ** We found a victim to close that's opened
		    ** by our slave. Close the file by
		    ** setting lru_close to the fd number. The
		    ** slave open code checks this value and if
		    ** it is >= zero it closes this file before
		    ** opening the the other file.
		    */
		    try_open_again = TRUE;
		    disl->lru_close = vic_file->fd_fd_in_slave[disl->dest_slave_no];
		    vic_file->fd_fd_in_slave[disl->dest_slave_no] = -1;

		    /*
		    ** Decrease open file count for this slave
		    */
		    --Di_slave_open[disl->dest_slave_no];

		    /*
		    ** See if any other slaves have this file
		    ** open
		    */
		    for (i = 0; i < Di_nslaves; i++)
		    {
			if (vic_file->fd_fd_in_slave[i] >= 0)
			    break;
		    }

		    /*
		    ** If no other slave owns this file then
		    ** move it to the free queue
		    */
		    if (i == Di_nslaves)
		    {
#ifdef	xDEV_TST
			TRdisplay(
			    "DI_FILE @@ %p line %d\n", vic_file, __LINE__);
#endif
			DI_free_insert(&vic_file);
		    }
		    else
			CS_synch_unlock(&vic_file->fd_mutex);
		}
		else
		{
		    /* 
		    ** All files are currently being referenced so
		    ** no victim can be found
		    */
		    CS_synch_unlock(&htb->htb_fd_list_mutex);
		    open_failed = TRUE;
		    *intern_status = DILRU_ALL_PINNED_ERR;
		    big_status = DILRU_OPEN_ERR;
		}

	    }
	    else
	    {
		open_failed = TRUE;
	    }

	    if ( open_failed )
	    {
		/*
		** The open request failed for some reason so
		** map the error to a meaningfull error and 
		** cleanup
		*/
		switch (disl->errcode.errnum)
		{
		case EMFILE:	/* Caller requested no retry */
		    flush_needed = TRUE;
		    ret_status = DI_EXCEED_LIMIT;
		    break;
		case ENFILE:	/* Caller requested no retry */
		    flush_needed = TRUE;
		    ret_status = DI_EXCEED_LIMIT;
		    break;
		case EEXIST:
		    ret_status = DI_EXISTS;
		    break;
		 case EINVAL:
		    ret_status = DI_BADDIR;
		    break;
		 case ENOENT:
		    if (flags & DI_FCREATE)
			ret_status = DI_DIRNOTFOUND;
		    else
			ret_status = DI_FILENOTFOUND;
		    break;
		 case ENOTDIR:
		    ret_status = DI_DIRNOTFOUND;
		    break;
		 case ENOSPC:
		    ret_status = DI_EXCEED_LIMIT;
		    break;
#ifdef EDQUOT
		 case EDQUOT:
		    ret_status = DI_EXCEED_LIMIT;
		    break;
#endif /* EDQUOT */
		 default:
		    ret_status = DI_BADOPEN;
		    break;
		}
		di_file->fd_racestat = ret_status;

		/*
		** Unpin the di_files ourselves. We hold
		** the fd_mutex and don't want DIlru_release()
		** to try to take it again.
		*/
		if (op->di_flags & DI_OP_PIN)
		{
		    --*op->di_pin;
		    op->di_flags &= ~(DI_OP_PIN);
		}

		*reserve_ev_flag = FALSE;
		big_status = DIlru_release(op, err_code );
		if ( big_status != OK )
		    break;

#ifdef	xDEV_TST
    		TRdisplay("DI_FILE @@ %p line %d\n", di_file, __LINE__);
#endif

		/*
		** Set status to correct return
		*/
		big_status = di_file->fd_racestat ;

		/* Free fd and release its mutex */
		DI_free_insert(&di_file);
	
		/*
		** If we failed because there were too many open files
		** then try and close some files
		*/
		if ( flush_needed )
		{
		    big_status = DIlru_flush( err_code );
		    if ( big_status != OK )
			break;
		}
		break;
	    }
	}

    } while (try_open_again) ;

    if ( big_status != OK )
	ret_status = big_status;

    return( ret_status );
}

/*{
** Name: DI_lru_allocate_more_slots - Allocate more slots in the file
**				     descriptor cache
**
** Description:
**
** Inputs:
**	None
**
** Outputs:
**	intern_status	Internal error indicating reason for failure.
**
**    Returns:
**        OK
**	  A range of DIlru errors from DIlru_init.
**
**    Exceptions:
**        none
**
** Side Effects:
**
** History:
**    30-nov-1992 (rmuth)
** 	    Created.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	15-Jan-1998 (jenjo02)
**	    Caller is assumed to hold fd_list_mutex.
**	26-Jul-2005 (schka24)
**	    Display FD table size to the dbms log.
**
*/
static STATUS
DI_lru_allocate_more_slots( 
    STATUS	*intern_status )
{

    QUEUE		*p;
    STATUS		me_status, status = OK;
    DI_FILE_DESC	*di_file;
    i4			i, k;
    i4			j = DI_ENTRY_NUM;

    do
    {
	{
	    /* Has cache limit been reached already? */

	    if (Di_lru_clim_reached)
	    {
		status = DILRU_CACHE_LIMIT;
		return(status);
	    }

	    /* Calculate how many slots left before cache 
	    ** hits maximum size
	    */

	    j = Di_lru_cache_size - Di_fd_cache_size;

	    if (!(j > 0 ))
	    {
		/* Maximum size of cache reached */

		status=DILRU_CACHE_LIMIT;
		Di_lru_clim_reached = TRUE;
		return(status);
	    }
	}

	if (Di_backend)
	    TRdisplay("%@ DI: allocated %d fd slots (%d max, %d reserved)\n",
		j, Di_lru_cache_size, Cs_srv_block.cs_max_sessions+1);

	di_file = (DI_FILE_DESC *)MEreqmem((u_i4)0,
					    (u_i4)(j * sizeof(DI_FILE_DESC)),
			   		    FALSE, &me_status);

	if (( me_status != OK ) || (!di_file) )
	{
	    if (me_status != OK )
	        status = me_status;
	    else
		status = FAIL;
	    *intern_status =  DILRU_MEREQMEM_ERR;
	    break;
	}

	/* 
	** Initialize newly allocated entries and put each
	** on the end of the FD queue.
	*/
	for (i = 0; i < j; i++, di_file++)
	{
#ifdef	xDEV_TST
	    TRdisplay(
	    "DI_lru_allocate_more_slots: di= %p slot %d\n", di_file, i);
#endif
	    /* Initialize null background values in fd */
	    di_file->fd_racestat = OK;
	    di_file->fd_state = FD_FREE;
	    di_file->fd_refcnt = 0;
	    di_file->fd_uniq.uniq_di_file = (PTR)NULL;
	    di_file->fd_uniq.uniq_seqno = 0;

	    /* Initialize fd's semaphore */
	    CS_synch_init(&di_file->fd_mutex);

	    /* Also make sure that the fd's are properly initialized */
	    if (Di_slave)
	    {
		for (k = 0; k < Di_nslaves; k++)
		    di_file->fd_fd_in_slave[k] = -1;
	    }
	    else
		di_file->fd_unix_fd = -1;

	    /* 
	    ** put on the end of the fd queue 
	    */
	    QUinsert(&di_file->fd_q, htb->htb_fd_list.q_prev);
	}
	Di_fd_cache_size += i;
	htb->htb_fd_list_count += i;

    } while ( FALSE );
	    
    return(status);
}

/*{
** Name: DI_lru_file_close - close the specified file.
**
** Description:
**
** Inputs:
**	di_file		File to close.
**
** Outputs:
**	intern_status	Internal error indicating reason for failure.
**
**    Returns:
**        OK
**	  A range of DIlru errors .
**
**    Exceptions:
**        none
**
** Side Effects:
**
** History:
**    30-nov-1992 (rmuth)
** 	    Created.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	12-may-1994 (rog)
**	    Reset the file's sticky bit when we close the file if it was
**	    turned off.
**	18-Feb-1998 (jenjo02)
**	    Modified for new mutexing scheme. Caller must hold fd's fd_mutex,
**	    which will be released before returning.
**
*/
static 
STATUS
DI_lru_file_close(
    DI_FILE_DESC	**di_f,
    CL_ERR_DESC		*err_code,
    STATUS		*intern_status)
{
    STATUS		 big_status = OK, ret_status = OK;
    i4			 i;
    CSEV_CB		 *evcb;
    bool		 reserve_ev_flag = FALSE;
    register DI_SLAVE_CB *disl;
    DI_OP                di_op;
    DI_FILE_DESC	 *di_file = *di_f;

    do
    {
	/* Clobber caller's fd pointer */
	*di_f = (DI_FILE_DESC*)NULL;

	if (Di_slave)
	{
	    big_status = CSreserve_event(Di_slave, &evcb);
	    if ( big_status != OK )
	    {
		*intern_status = DI_LRU_CSRESERVE_ERR;
		break;
	    }
	    else
		reserve_ev_flag = TRUE;
	    
	    /* need to manufacture a di_op for the slave call */
	    di_op.di_flags = 0;
	    di_op.di_csevcb = evcb;
	    di_op.di_evcb = disl = (DI_SLAVE_CB *)evcb->data;

	    for (i = 0; i < Di_nslaves; i++)
	    {
		if (di_file->fd_fd_in_slave[i] >= 0)
		{
		    /*
		    ** slave (i) has the file open
		    */
		    disl->lru_close = -1;
		    disl->pre_seek = -1;
		    disl->file_op = DI_SL_CLOSE;
		    disl->unix_fd = di_file->fd_fd_in_slave[i];
		    disl->dest_slave_no = i;

		    big_status = DI_inc_Di_slave_cnt( i, intern_status);
		    if ( big_status != OK )
			break;

		    ret_status = DI_send_ev_to_slave(&di_op, i, intern_status);

		    big_status = DI_dec_Di_slave_cnt( i, intern_status);

		    if ( big_status != OK )
			break;

		    di_file->fd_fd_in_slave[i] = -1;
		    --Di_slave_open[i];

		    /*
		    ** If an error sending event to slave or
		    ** closing the file then exit
		    */
		    if ((ret_status != OK) || (disl->status != OK) )
			break;

		}
	    } /* for */

	    /*
	    ** If an error occurred while performing post or
	    ** preprocessing for the send event to slave call
	    ** then exit
	    */
	    if ( big_status != OK )
		break;

	    reserve_ev_flag = FALSE;
	    big_status = CSfree_event(evcb);
	    if ( big_status != OK )
	    {
		*intern_status = DI_LRU_CSFREE_EV_ERR;
		break;
	    }

	    /* Check request was sent to slave ok */
	    if ( ret_status != OK )
		break;

	    /* Make sure we closed all the files ok */
	    if ( disl->status != OK )
	    {
		ret_status = DI_BADCLOSE;
		STRUCT_ASSIGN_MACRO( disl->errcode, *err_code);
		break;
	    }
	}
	else
	{
	    /* 
	    ** No slaves so close the file inline 
	    */
#if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
	    /* Reset the sticky bit in case we turned it off. */
	    if (!Di_no_sticky_check)
	    {
#ifdef LARGEFILE64
		struct stat64 sbuf;

		if (!fstat64(di_file->fd_unix_fd, &sbuf))
#else /* LARGEFILE64 */
		struct stat sbuf;

		if (!fstat(di_file->fd_unix_fd, &sbuf))
#endif /* LARGEFILE64 */
		{
		    int new_perms = (unsigned)sbuf.st_mode & ~S_IFMT;

		    new_perms |= S_ISVTX;
		    (VOID) fchmod(di_file->fd_unix_fd, new_perms);
		}
	    }
#endif

	    if (close(di_file->fd_unix_fd) == (int) -1)
	    {
		ret_status = DI_BADCLOSE;
		SETCLERR(err_code, 0, ER_close);
	    }
	}

#ifdef	xDEV_TST
	TRdisplay("DI_FILE @@ %p line %d\n", di_file, __LINE__);
#endif

    } while (FALSE);

    if (big_status != OK )
	ret_status = big_status;

    CSadjust_counter(&htb->htb_stat.fds_open,-1);

    /*
    ** No matter what happens place on the free list and release fd_mutex. 
    ** This is the current behaviour and stops us at least losing entries.
    */
    DI_free_insert(&di_file);

    /*
    ** If this is still reserved we had a major error so just release
    ** the event don't worry about error handling
    */
    if (reserve_ev_flag)
        (VOID) CSfree_event(evcb);

    return( ret_status );
}

/*
** Name: DI_lru_slmapmem		- map this segment into each slave
**
** Description:
**	Mapping a shared memory I/O segment into slave processes allows the
**	slaves to perform I/O directly to/from that segment, bypassing the
**	copy through the server segment. This is performed automagically by
**	DI - when first starting up, it maps all known I/O segments; later,
**	if a request comes in to read or write from a segment which hasn't
**	been mapped yes, this routine is called to map the segment into each
**	slave process.
**
** Inputs:
**	seginfo				- segment to map.
**
** Outputs:
**	intern_status			- internal status code
**	send_status			- problem in slave
**
** REturns:
**	OK
**	other
**
** History:
**	23-aug-1993 (bryanp)
**	    Broken out from DIlru_init to be callable by DIread/DIwrite.
*/
STATUS
DI_lru_slmapmem(
ME_SEG_INFO	*seginfo,
STATUS		*intern_status,
STATUS		*send_status)
{
    STATUS		status;
    CSEV_CB		*evcb;
    DI_OP       	di_op;
    DI_SLAVE_CB		*disl;
    i4		i;

    status = CSreserve_event(Di_slave, &evcb);
    if ( status != OK )
    {
	*intern_status = DI_LRU_CSRESERVE_ERR;
	return (status);
    }

    /* need to manufacture a di_op for the slave call */
    di_op.di_flags = 0;
    di_op.di_csevcb = evcb;
    di_op.di_evcb = disl = (DI_SLAVE_CB *)evcb->data;

    for (i = 0;  i < Di_nslaves; i++)
    {
	disl->file_op = DI_SL_MAPMEM;
	MEcopy((PTR)seginfo->key, sizeof(seginfo->key),
		(PTR)disl->buf);
	disl->dest_slave_no = i;

	DI_slave_send( disl->dest_slave_no, &di_op,
		       &status, send_status, intern_status);
	if  ( ( status != OK ) || 
	      ( *send_status != OK )  ||
	      ( disl->status != OK ) )
	    break;
    }

    /*
    ** If an error occurred while performing post or
    ** preprocessing for the send event to slave call
    ** then exit. Otherwise the error is not too serious
    ** so release the evcb and carry on
    */
    if ( status != OK )
    {
	(void)CSfree_event(evcb);
	return (status);
    }

    status = CSfree_event(evcb);
    if ( status != OK )
    {
	*intern_status = DI_LRU_CSFREE_EV_ERR;
	return (status);
    }

    /*
    ** Had an error so exit
    */
    if ( *send_status != OK )
	return (status);
    else if (( *send_status = disl->status ) != OK )
	return (status);

    seginfo->flags |= ME_SLAVEMAPPED_MASK;

    return (OK);
}

/*{
** Name: DI_tidy_close
**
** Description:
**	Make sure all fds in the io_fd_q no longer point back to the DI_IO.
**
** Inputs:
**	f		 pointer to DI_IO
**
** Outputs:
**	f->io_fd_q	Any DI_FILE_DESC objects on the io_fd_q list will 
**			no longer point back to the DI_IO
**
**    Returns:
**        VOID
**			 
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    08-jul-09 (smeke01) b120853
**          First created (adapted from DI_find_fd). 
*/
static VOID
DI_tidy_close(
    DI_IO		*f)
{
    DI_FILE_DESC *entry;
    QUEUE *p, *q;

    q = &f->io_fd_q;
    p = q;
    CS_synch_lock(&f->io_fd_q_sem);

    while ((p = p->q_next) != q)
    {
	entry = (DI_FILE_DESC*)((char*)p - CL_OFFSETOF(DI_FILE_DESC, fd_io_q));
		
	if (entry->fd_uniq.uniq_di_file == (PTR)f)
	{
	    /* Lock the entry, recheck it */
	    CS_synch_unlock(&f->io_fd_q_sem);
	    CS_synch_lock(&entry->fd_mutex);

	    if (entry->fd_uniq.uniq_di_file == (PTR)f)
	    {
		TRdisplay(
		    "DIwarning: DI_tidy_close: in-use DI_FILE_DESC (%p) on DI_IO (%p) for file %12s %d (%d) fd %d\n",
			    entry,
			    f,
			    f->io_filename,
			    entry->fd_uniq.uniq_seqno,
			    entry->fd_refcnt,
			    entry->fd_unix_fd);

		/* Prevent later (stale) dereferencing of the DI_IO pointer */
		entry->fd_uniq.uniq_di_file = (PTR)NULL;
		entry->fd_uniq.uniq_seqno   = 0;
	    }
	    else 
	    {
		/*
		** If the entry does not point to our DI_IO, it must 
		** have been taken off the io_fd_q (whilst we were
		** waiting for the fd_mutex above). This is a good 
		** thing as it saves us having to take any cleanup
		** action. However the list has been broken so we 
		** need to start from the beginning again.
		*/
		p = q;
	    }

	    CS_synch_unlock(&entry->fd_mutex);
	    CS_synch_lock(&f->io_fd_q_sem);
	}
    }

    CS_synch_unlock(&f->io_fd_q_sem);
    return;
}
