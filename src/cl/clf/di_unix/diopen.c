/*
**Copyright (c) 2004 Ingres Corporation
*/

#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <systypes.h>
#include   <cs.h>
#include   <fdset.h>
#include   <csev.h>
#include   <di.h>
#include   <me.h>
#include   <qu.h>
#include   <st.h>
#include   <cldio.h>
#include   <dislave.h>
#include   "dilocal.h"
#include   "dilru.h"
#include   <systypes.h>

#ifdef            xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */

#ifdef      xCL_007_FILE_H_EXISTS
# if !defined(hp3_us5) && !defined(axp_osf)
#define		KERNEL		/* needed to bypass "include <sys/types.h>" */
# endif /* hp3_us5 & axp_osf */
#include    <sys/file.h>
#endif            /* xCL_007_FILE_H_EXISTS */

#include    <errno.h>

/**
**
**  Name: DIOPEN.C - This file contains the UNIX DIopen() routine.
**
**  Description:
**      The DIOPEN.C file contains the DIopen() routine.
**
**        DIalloc - Allocates pages to a file.
**
** History: 
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	29-mar-88 (anton)
**          DI in slave proces using CS events
**	17-oct-88 (anton)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	20-dec-88 (anton)
** 	    added testing code
**	21-jan-89 (mikem)
**	    DI_SYNC_MASK support (O_SYNC, fsync())
**	06-feb-89 (mikem)
**	    Cleared CL_ERR_DESC, pass back err_code from DIlru_open(), 
**	    invalidate io_type on error, and "#include me.h".
**	13-Feb-89 (anton)
**	    New flags for DIlru_open.
**	24-Feb-1989 (fredv)
**	    Replaced xDI_xxx's with corresponding xCL_xxx.
**	    Removed bogus reference to lseek().
**	23-mar-89 (mmm)
**	    Initialize the io_system_eof (bug XXXX)
**	20-Mar-90 (anton)
**	    Include clconfig.h to enable O_SYNC.
**	13-Apr-90 (kimman)
**          Added <systypes.h> to prevent typedef problems with fcntl
**	14-May-90 (blaise)
**	    Integrated changes from 61 and ug into the 63p library:
**		Added kludge to bypass "include <sys/types.h>" in <sys/file.h>;
**		<sys/file.h>;
**		interpret open_flags with DI_SYNC_MASK.
**	02-jul-91 (johnr)
**	    Excluded KERNEL define for hp3_us5.
**      27-jul-91 (mikem)
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
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	30-October-1992 (rmuth)
**	    Remove references to DI_ZERO_ALLOC and prototype.
**	30-nov-1992 (rmuth)
**	    Add parameter to DIlru_uniq and DILRU error checking.
**      18-dec-1992 (smc)
**          Excluded KERNEL define for axp_osf.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      26-jul-1993 (mikem)
**          Include systypes.h now that csev.h exports prototypes that include
**          the fd_set structure.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	18-apr-1994 (jnash)
**	    fsync project.  Add DI_USER_SYNC_MASK DIopen flag option,
**	    indicating that user will sync file via DIforce() when he
**	    needs to. 
**	19-apr-95 (canor01)
**	    added <errno.h>
**	03-jun-1996 (canor01)
**	    Initialized semaphore in DI_IO block for use with operating-system
**	    threads.
**	05-nov-1997 (canor01)
**	    Do not initialize semaphore if file open failed.
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs to dilocal.h
**	    Added stats, DI_LOG_FILE_MASK, DI_NOT_LRU_MASK.
**	22-Dec-1998 (jenjo02)
**	    Initialize (new) io_fd_q.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # defines */

/* typedefs */

/* forward references */


/* externs */

/* statics */


/*{
** Name: DIopen - Opens a file.
**
** Description:
**      The DIopen routine is used to open a direct access file.  
**      It will open the file for read or write access and will
**      place no locks on the file.
**      If the path does not exists the error DI_DIRNOTFOUND
**      must be returned before returning DI_FILENOTFOUND.
**      If possible the operating system should check that the pages
**      size at open is the same as the page size at create.  If 
**      the page sizes at open and create time don't match
**	DI_BADPARAM should be returned.
**
**	The DIopen() command also determines actions performed
**	by DIwrite() on this file.  The "flag" argument contains
**	information which is interpreted by system dependant implementations
**	of database I/O.  (On unix systems there are different options
**	for guaranteeing writes to disk - each of which come with certain
**	performance penalties - so that one might want them enabled in 
**	certain situations (ie. database files), but not in others 
**	(ie. temp files)).
**   
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      path                 Pointer to the directory name
**                           for the file.
**      pathlength           Length of path name.
**      filename             Pointer to file name.
**      filelength           Length of file name.
**      pagesize             Value indicating size of page 
**                           in bytes. Must match size at create.
**      mode                 Value indicating the access mode.
**                           This must be DI_IO_READ or DI_IO_WRITE.
**	flags		     Used to alter system specific behaviour of IO
**			     If a unix vendor provides alternate control of
**			     disk I/O though new system calls or flags, it
**			     may be necessary to add to these flags.
**
**			DI_SYNC_MASK	Synchronous writing, ie when the
**					write returns, we can assume it's
**					on the disk.  On unix this means
**					O_SYNC (or equivalent, eg O_DSYNC;
**					we only care about data and filesize,
**					not access times etc.)
**			DI_USER_SYNC_MASK  Caller doesn't need force-to-disk
**					on every write, caller will DIforce
**					if data must be landed on disk.
**					Note that on unix, this is the same as
**					no sync-flags at all.
**			DI_DIRECTIO_MASK  Asks for direct (unbuffered) I/O
**					if supported by the platform and
**					filesystem.
**			DI_PRIVATE_MASK  Hints that the file is not sharable,
**					ie is strictly private to a thread.
**      
** Outputs:
**      f                    Updates the file control block.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADDIR        Path specification error.
**          DI_BADOPEN       Error openning file.
**          DI_BADFILE       Bad file context.
**          DI_BADPARAM      Parameter(s) in error.
**          DI_DIRNOTFOUND   Path not found.
**          DI_EXCEED_LIMIT  Open file limit exceeded.
**          DI_FILENOTFOUND  File not found.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    26-mar-87 (mmm)    
**          Created new for 6.0.
**    21-jan-89 (mikem)
**	    DI_SYNC_MASK support (O_SYNC, fsync())
**    06-feb-89 (mikem)
**	    Cleared CL_ERR_DESC, pass back err_code from DIlru_open(), 
**	    invalidate io_type on error, and "#include me.h".
**    23-mar-89 (mmm)
**	    Initialize the io_system_eof (bug XXXX)
**    27-jul-91 (mikem)
**	    Reimplement DI_SYNC_MASK support which was changed with the 
**	    14-May-90 (blaise) submission.  Determine sync operation at 
**	    DIopen().  If both fsync and O_SYNC are available on a machine
**	    choose only to use O_SYNC since that is more efficient.  This
**	    decision is only made in this file, all other routine trust
**	    the flag setting of "io_open_flags".
**    30-October-1992 (rmuth)
**	    Remove references to DI_ZERO_ALLOC and prototype.
**    30-nov-1992 (rmuth)
**	    Add parameter to DIlru_uniq and DILRU error checking.
**    18-apr-1994 (jnash)
**	    Add DI_USER_SYNC_MASK DIopen flag.  Opening a table
**	    in this manner implies that the caller will sync the
**	    table via DIforce() prior to close.  
**	03-jun-1996 (canor01)
**	    Initialized semaphore in DI_IO block for use with operating-system
**	    threads.
**	23-sep-1997 (kosma01)
**	    removed unused argument on call to STprintf to create a semaphore name.
**	22-Dec-1998 (jenjo02)
**	    Initialize (new) io_fd_q.
**	25-Jul-2005 (schka24)
**	    Initialize mutex for above queue.
**	30-Sep-2005 (jenjo02)
**	    io_fd_q_sem now a CS_SYNCH object.
**	06-Oct-2005 (jenjo02)
**	    DI_FD_PER_THREAD now config param
**	    ii.$.$.$.fd_affinity:THREAD, Di_thread_affinity = TRUE,
**	    still dependent on OS_THREADS_USED.
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
**	11-Jan-2008 (kschendel) b122122
**	    Fix up flag settings for "user sync".  This mode implies that
**	    some caller may / will use DIforce to perform flushes at times
**	    that seem good to the caller.  If there is an fsync, flag
**	    the di-file with "user sync" regardless of O_SYNC existence.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Update direct-io handling, double check page size.
*/
STATUS
DIopen(
    DI_IO          *f,
    char           *path,
    u_i4          pathlength,
    char           *filename,
    u_i4          filelength,
    i4             pagesize,
    i4             mode,
    u_i4      flags,
    CL_ERR_DESC     *err_code)
{
    STATUS	ret_val;
    DI_OP	diop;

    /* default returns */
    ret_val = OK;
    CL_CLEAR_ERR( err_code );

    /* Check input parameters. */

    if ((pathlength > DI_PATH_MAX)	|| 
	(pathlength == 0)		||
	(filelength > DI_FILENAME_MAX)	||
	(filelength == 0)		||
	pagesize < 512 || pagesize > DI_MAX_PAGESIZE ||
	(pagesize & 511) != 0          ||
        (mode != DI_IO_READ && mode != DI_IO_WRITE))
    {
	return (DI_BADPARAM);		
    }

    /* Initialize file control block. */

    /* save path and filename in DI_IO struct to cache open file descriptors. */

    MEcopy((PTR) path, pathlength, (PTR) f->io_pathname);
    f->io_l_pathname = pathlength;
    MEcopy((PTR) filename, filelength, (PTR) f->io_filename);
    f->io_l_filename = filelength;

#ifdef	xDEV_TST
    TRdisplay("DIopen: %t/%t\n", f->io_l_pathname, f->io_pathname,
	      f->io_l_filename, f->io_filename);
#endif

    f->io_bytes_per_page = pagesize;
    f->io_type = DI_IO_ASCII_ID;

    /* Init file properties */
    FPROP_CLEAR(f->io_fprop);
    /* No direct IO or private if slaves */
    if (! Di_slave)
    {
	/* If DIRECTIO wanted on this file, suggest the hint. */
	if ( flags & DI_DIRECTIO_MASK )
	    f->io_fprop |= FPROP_DIRECT_REQ;
	/* If file is thread-private, set hint for everyone */
	if (flags & DI_PRIVATE_MASK)
	    f->io_fprop |= FPROP_PRIVATE;
    }

    f->io_fd_q.q_next = f->io_fd_q.q_prev = (QUEUE*)&f->io_fd_q;

    MEfill(sizeof(f->io_stat), 0, &f->io_stat);
    f->io_stat.open++;
        /* need to initialize the file descriptor queue mutex semaphore */
#ifdef OS_THREADS_USED
    /* This we need before DIlru? calls */
    if ( Di_thread_affinity )
    {
	CS_synch_init(&f->io_fd_q_sem);
    }
    CS_synch_init(&f->io_sem);		/* Let's init this too */
#endif

    /* for now only save the current flags if the machine will support them. */

    f->io_open_flags = 0;

    if (flags & DI_LOG_FILE_MASK)
    {
	f->io_open_flags |= DI_O_LOG_FILE_MASK;
	/* Force the log files to be non-LRUable */
	f->io_open_flags |= DI_O_NOT_LRU_MASK;
    }

    if (flags & DI_NOT_LRU_MASK)
	f->io_open_flags |= DI_O_NOT_LRU_MASK;

    if (flags & DI_SYNC_MASK)
    {

#ifdef O_SYNC

	/* O_SYNC is the preferred mode if both O_SYNC and fsync exist. */
	f->io_open_flags |= DI_O_OSYNC_MASK;

#elif defined(xCL_010_FSYNC_EXISTS)

	/* FSYNC tells the rest of DI to fsync after each operation.
	** This is presumably a less effective mode and is not preferred.
	*/
	f->io_open_flags |= DI_O_FSYNC_MASK;

#endif

    }
    else if (flags & DI_USER_SYNC_MASK)
    {

#if !defined(xCL_010_FSYNC_EXISTS) && defined(O_SYNC)
	/*
	** If FSYNC doesn't exist, or if port does not choose to implement 
	** this open mode, use O_SYNC.
	*/
	f->io_open_flags |= DI_O_OSYNC_MASK;
#endif
	/* else don't flag the file at all, explicit force's will sync it */
    }

    /* we always open the file for read/write - but DIwrite must not allow 
    ** writes to a file opened for reading only.
    */
    f->io_mode = mode;

    /* assign a unique id to this file (number is unique to the current server).
    ** This unique id is used in subsequent DI calls to identify the file in
    ** question without having to do comparisons on full path and file name.
    */
    do
    {
        ret_val = DIlru_uniq(f, err_code);
	if (ret_val != OK )
	    break;

	/* 
	** open this file, closing another DI file if necessary 
	** (ie. if we have run out of open file descriptors.)
	** NOTE: We passed NORETRY here even though the above comment
	** contradicts this, changed call so that we RETRY.
	*/

        ret_val = DIlru_open(f, FALSE, &diop, err_code);
	if ( ret_val != OK )
	    break;

	f->io_alloc_eof = -1;
	f->io_reserved_bytes = 0;

        ret_val = DIlru_release(&diop, err_code );
    } while (FALSE);

    if (ret_val != OK )
    {
	/* for sanity mark DI_IO as not a valid open file */

	f->io_type = DI_IO_OPEN_INVALID;
# ifdef OS_THREADS_USED
	if ( Di_thread_affinity )
	{
	    CS_synch_destroy(&f->io_fd_q_sem);
	}
	CS_synch_destroy(&f->io_sem);
# endif
    }

    return(ret_val);
}
