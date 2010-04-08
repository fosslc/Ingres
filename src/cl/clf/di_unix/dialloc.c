/*
** Copyright (c) 2004 Ingres Corporation
**
*/

#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <systypes.h>
#include   <fdset.h>
#include   <er.h>
#include   <cs.h>
#include   <csev.h>
#include   <di.h>
#include   <dislave.h>
#include   <cldio.h>
#include   "dilocal.h"
#include   "dilru.h"

/* unix includes */

#ifdef	    xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */

#ifdef      xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif	    /* xCL_007_FILE_H_EXISTS */

#include    <errno.h>

/**
**
**  Name: DIALLOC.C - This file contains the UNIX DIallocate() routine.
**
**  Description:
**      The DIALLOC.C file contains the DIallocate() routine.
**
**        DIalloc - Allocates pages to a file.
**
** History:
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      Revision 1.3  87/11/05  09:57:19  mikem
**      fixed alloc vs. real eof problems.
**      30-mar-88 (anton)
**          Di in slave
**	17-oct-88 (jeff)
** 	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	24-jan-89 (roger)
** 	    Make use of new CL error descriptor.
**	06-feb-89 (mikem)
**	    better support for DI CL_ERR_DESC, including initializing to zero
**	    and errors passed back from DIlru_open().
**	27-Feb-1989 (fredv)
**	    Replaced xDI_xxx by corresponding xCL_xxx defs in clconfig.h.
**	    Include <systypes.h>.
**	23-mar-89 (mikem)
**	    update io_system_eof when necessary (bug 4854).
**	03-aug-89 (mikem)
**	    Added debugging to stress the system's handling of out of disk
**	    space.  Define xOUT_OF_DISK_SPACE_TEST to get this code compiled in.
**	01-Oct-90 (anton)
**	    Changes for reentrant CL
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	30-October-1992 (rmuth)
**	    - Remove all references to file type DI_ZERO_ALLOC, this flag
**	      would cause us to physically allocate space on disc. The
**	      client is now required to call DIgalloc for this behaviour.
**	    - Prototype code.
**	    - Add xDEV_TST message to non-slave part of code.
**	30-nov-1992 (rmuth)
**	    Various
**	     - include <cldio.h>
**	     - Call IIdio_get_file_eof instead of lseek as this can
**	       deal with both raw and ordinary files. Change some
**	       types accordingly.
**	     - DIlru error checking changes.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	19-apr-95 (canor01)
**	    added <errno.h>
**	23-jul-1996 (canor01)
**	    Semaphore protect the lseek/write combination when used with
**	    operating-system threads.
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Removed GLOBALREFs, changed Di_alloc_count to static.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    28-May-2002 (hanal04) Bug 107686 INGSRV 1764
**          Resolve Sequent DIRECT I/O problems by redirecting DIalloc()
**          calls to use DIgalloc() instead.
**/


/*
**  Forward and/or External function references.
*/


/*
** Definition of all global variables owned by this file.
*/


/* 
** count of DIalloc() for debugging 
*/
static i4             Di_alloc_count ZERO_FILL; 


/*{
** Name: DIalloc - Allocates a page to a direct access file.
**
** Description:
**      The DIalloc routine is used to add pages to a direct
**      access file.  This routine can add more than one page
**      at a time by accepting a count of the number of pages to add.
**   
**	According to the generic DI design, callers will not expect soft
**	allocated pages to persist until a DIflush call is done.
**	In practice, unix DIflush has been a no-op for a long time,
**	and nothing expects DIalloc to allocate DIsense-able space
**	that hasn't been written.
**
**	So, all that callers can require DIalloc to do is to update
**	the internal EOF pointer that says "it's ok to read or write
**	these pages with DIread or DIwrite".  Some callers MAY wish
**	to do more, however, such as preallocate additional space
**	for better disk allocation.
**
**	Putting this all together, here's the rules:
**
**	1. DIalloc may or may not allocate any disk space at all.
**	2. DIalloc may or may not hard-allocate persistent space.
**	3. Callers can assume that they may DIwrite into soft allocated
**	   pages, but not necessarily DIread them until they are hard
**	   allocated, or written.
**	4. A DIsense after a DIalloc can return anything from highest-
**	   actually-written to highest-allocated depending on the
**	   allocation method.
**
**	One soft-allocation option available on Linux is to "fallocate"
**	the disk space.  This is a persistent reservation that allocates
**	disk space to the file.  However, the reserved space is hidden,
**	in that EOF is not moved unless one actually writes to the file.
**
** Inputs:
**      f                Pointer to the DI file
**                       context needed to do I/O.
**      n                The number of pages to allocate.
**
** Outputs:
**      page             Pointer to variable used to 
**                       return the page number of the
**                       first page allocated.
**      err_code         Pointer to a variable used
**                       to return operating system 
**                       errors.
**    Returns:
**        OK
**        DI_BADINFO	    	Could not find current end-of-file.
**        DI_BADFILE        	Bad file context.
**        DI_EXCEED_LIMIT   	Too many open files.
**
**	  A range of errors from unexpected errors in DIlru code.
**
**        If running with slaves  :
**        DI_BADCAUSE_EVENT     Error sending event to slaves.
**        DI_BAD_SUSPEND        Error suspending thread.
**	  DI_LRU_CSCAUSE_EV_ERR Error reserving an event.
**
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    11-sep-85 (jennifer)
**          Created new for 5.0.
**    20-mar-86 (jennifer)
**          Changed i4 to i4  and err_code to CL_ERR_DESC to meet 
**          CL coding standard.
**    05-mar-87 (mmm)
**	    ported to UNIX.
**    06-feb-89 (mikem)
**	    better support for DI CL_ERR_DESC, including initializing to zero
**	    and errors passed back from DIlru_open().
**    23-mar-89 (mikem)
**	    update io_system_eof when necessary (bug 4854).
**    03-aug-89 (mikem)
**	    Added debugging to stress the system's handling of out of disk
**	    space.  We will force DIwrite() to return "DI_EXCEEED_LIMIT" once
**	    every 10 disk space allocations.
**    30-October-1992 (rmuth)
**	    - Remove all references to file type DI_ZERO_ALLOC, this flag
**	      would cause us to physically allocate space on disc. The
**	      client is now required to call DIgalloc for this behaviour.
**	    - Prototype code.
**	    - Add xDEV_TST message to non-slave part of code.
**    30-nov-1992
**	    - Call IIdio_get_file_eof instead of lseek as this can
**	      deal with both raw and ordinary files. Change some
**	      types accordingly.
**	    - This routine can now return more than just DI_xxx error
**	      messages but these messages are only when a very 
**	      unexpected error has occured.
**    23-jul-1996 (canor01)
**	    Semaphore protect the lseek/write combination when used with
**	    operating-system threads.
**    28-May-2002 (hanal04) Bug 107686 INGSRV 1764
**          We must use DIgalloc() if we are using Direct I/O for
**          Sequent. Failure to do so will generate sparse files
**          which will cause DirectRead() to return EINVAL when
**          it tries to read a hole.
**    02-Apr-2004 (fanch01)
**      Extend the file with the strategy handy recommends.  Useful
**      on Linux which supports different filesystems with different
**      extend performance characteristics.
**    13-apr-04 (toumi01)
**	Move stack variable declarations to support "standard" C compilers.
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
**	    Removed Sequent-specific code, deprecated platform.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Add fallocate allocation method, a takeoff on the XFS
**	    reservations that Datallegro used.  (The old XFS reservation
**	    ioctl's are deprecated in favor of fallocate, as of about
**	    2.6.23 onward.)  fallocate should work on ext4 and possibly
**	    btrfs as well as xfs.
**	    Make io-sem a SYNCH object.
*/
STATUS
DIalloc(
    DI_IO         *f,
    i4	   n,
    i4	  *page,
    CL_ERR_DESC    *err_code )
{
    i4		toss_eof;
    DI_OP	diop;
    STATUS	status = OK;
    i4		strategy;
    i4		unix_err;
    off_t	unix_offset;
    ssize_t	bytes_written;

    CL_CLEAR_ERR( err_code );

    /* Check file control block pointer, return if bad. */

    if (f->io_type != DI_IO_ASCII_ID)
	return(DI_BADFILE);
    if (n < 0)
	return (DI_BADPARAM);

    /* Count another allocate */
    f->io_stat.alloc++;

    /* Single-page allocs don't bother actually allocating space.
    ** (Might as well just write append.)
    ** Don't do anything in-process if slaves.
    */
    strategy = FPROP_ALLOCSTRATEGY_GET(f->io_fprop);
    if (n <= 1 || Di_slave)
	strategy = FPROP_ALLOCSTRATEGY_VIRT;

    /* Private files with "virtual" allocation strategy just update
    ** the virtual EOF pointer, no need for sense.
    */
    if (f->io_fprop & FPROP_PRIVATE && strategy == FPROP_ALLOCSTRATEGY_VIRT)
    {
	*page = f->io_alloc_eof + 1;
	f->io_alloc_eof = f->io_alloc_eof + n;
	return (OK);
    }

    /* For non-private files, someone else might have allocated since
    ** we did it last, need to sense EOF
    */

    /* get file descriptor for this file */
    status = DIlru_open( f, FALSE, &diop, err_code);
    if ( status != OK )
	return(status);
    /*
    ** DI_sense updates f->io_alloc_eof with the protection
    ** of io_sem (OS_THREADS), so there's no need to
    ** duplicate that update here.
    */
    status = DI_sense( f, &diop, &toss_eof, err_code );

    /*
    ** If sense failed, give up.
    */
    if (status != OK )
    {
	CL_ERR_DESC lerr_code;
	(VOID) DIlru_release(&diop, &lerr_code);
	return(status);
    }

# ifdef OS_THREADS_USED
    if ((f->io_fprop & FPROP_PRIVATE) == 0)
	CS_synch_lock( &f->io_sem );
# endif /* OS_THREADS_USED */

    /* perform the extend based on file's strategy */
    switch (strategy)
    {
	case FPROP_ALLOCSTRATEGY_VIRT:
	    /* "Virtual" allocate, does nothing except change the
	    ** EOF pointer in the DI_IO, below.
	    */
	    break;

	case FPROP_ALLOCSTRATEGY_TRNC:
	    /* use ftruncate() to extend the file */
	    /* the +1 is because alloc-eof is the "last page", -1 if none */

	    unix_offset = (off_t) (f->io_alloc_eof + n + 1) * f->io_bytes_per_page;
# ifdef LARGEFILE64
	    unix_err = ftruncate64 (diop.di_fd, unix_offset);
# else /*  LARGEFILE64 */
	    unix_err = ftruncate (diop.di_fd, unix_offset);
# endif /* LARGEFILE64 */
	    if ( unix_err )
	    {
		SETCLERR(err_code, 0, ER_ioctl);
		switch( err_code->errnum )
		{
		    case EFBIG:
			status = DI_BADEXTEND;
			break;
		    case ENOSPC:
			status = DI_EXCEED_LIMIT;
			break;
		    default:
			status = DI_BADWRITE;
			break;
		}
		TRdisplay("DIalloc: failed to truncate extend file: %t/%t, alloc_eof: %d, first: %d, count: %d\n",
				f->io_l_pathname, f->io_pathname,
				f->io_l_filename, f->io_filename,
				f->io_alloc_eof, *page, n);
	    }
	    break;

	case FPROP_ALLOCSTRATEGY_RESV:
	    /* Use fallocate() to reserve disk space for the file.
	    ** Don't do anything if enough space is already reserved.
	    */
	    unix_offset = (off_t) (f->io_alloc_eof + 1 + n) * f->io_bytes_per_page;
	    if (unix_offset <= f->io_reserved_bytes)
		break;			/* Nothing to do... */

	    /* Re-check the end of the reservation in case some other
	    ** server changed it since we looked last.
	    */
	    status = IIdio_get_reserved(diop.di_fd, &f->io_reserved_bytes, err_code);
	    if (status != OK || unix_offset <= f->io_reserved_bytes)
		break;
	    /* Extend by desired-allocation minus already-reserved.  (The
	    ** desired-allocation is relative to the current observed EOF,
	    ** there may be blocks reserved beyond that already.)
	    */
	    status = IIdio_reserve(diop.di_fd, f->io_reserved_bytes,
			unix_offset - f->io_reserved_bytes, err_code);
	    if (status == OK)
	    {
		f->io_reserved_bytes = unix_offset;
		/* Note that EOF does not change... */
	    }
	    else
	    {
		if (status == DI_BADFILE)
		{
		    /* BADFILE is the signal that the OS or filesystem
		    ** does not support ALLOCSTRATEGY_RESV.  Something in
		    ** the file-open checking was over-optimistic.  Fall
		    ** back to "VIRT" strategy.
		    */
		    status = OK;
		    strategy = FPROP_ALLOCSTRATEGY_VIRT;
		    FPROP_ALLOCSTRATEGY_SET(f->io_fprop, strategy);
		}
	    }
	    break;

	case FPROP_ALLOCSTRATEGY_EXTD:
	    /* Extend by pwriting one page at the last page of the allocation.
	    ** This might work for filesystems that don't do sparse files.
	    ** Remember alloc-eof is "last page currently".
	    */
	    unix_offset = (off_t) (f->io_alloc_eof + n) * f->io_bytes_per_page;
# ifdef LARGEFILE64
	    bytes_written = pwrite64( (int)diop.di_fd, Di_zero_buffer,
			f->io_bytes_per_page, unix_offset );
# else /*  LARGEFILE64 */
	    bytes_written = pwrite( (int)diop.di_fd, Di_zero_buffer,
			f->io_bytes_per_page, unix_offset );
# endif /* LARGEFILE64 */
	    if ( bytes_written != f->io_bytes_per_page )
	    {
		SETCLERR(err_code, 0, ER_write);
		switch( err_code->errnum )
		{
		    case EFBIG:
			status = DI_BADEXTEND;
			break;
		    case ENOSPC:
			status = DI_EXCEED_LIMIT;
			break;
		    default:
			status = DI_BADWRITE;
			break;
		}
		TRdisplay("DIalloc: failed to write extend file: %t/%t, alloc_eof: %d, first: %d, count: %d\n",
				f->io_l_pathname, f->io_pathname,
				f->io_l_filename, f->io_filename,
				f->io_alloc_eof, *page, n);
	    }
	    break;

	default:
		/* reserved, here only to provide a fullcase select */
		/* reasonable use is for replacing dodirect(), DIgalloc() */
	    break;
    }

    if (status == OK)
    {
	/*
	** Return page number of first page allocated
	*/
	*page = (i4) (f->io_alloc_eof + 1);

	/*
	** Update the current allocated end-of-file
	*/
	f->io_alloc_eof = f->io_alloc_eof + n;
    }

# ifdef OS_THREADS_USED
    if ((f->io_fprop & FPROP_PRIVATE) == 0)
	CS_synch_unlock( &f->io_sem );
# endif /* OS_THREADS_USED */

    (void) DIlru_release(&diop, err_code);

#ifdef xDEV_TST
	TRdisplay("DIalloc: file: %t/%t, alloc_eof: %d, first: %d, count: %d\n",
    		  f->io_l_pathname, f->io_pathname,
	      	  f->io_l_filename, f->io_filename,
		  f->io_alloc_eof, *page, n);
#endif
   
# ifdef xOUT_OF_DISK_SPACE_TEST
    	if ((++Di_alloc_count % 10) == 0)
        {
	    f->io_open_flags |= DI_O_NODISKSPACE_DEBUG;
	    TRdisplay("DIalloc() setting bit so DIwrite call will return \n");
	    TRdisplay("false DI_EXCEED_LIMIT. Adding page %d\n", *page);
    	}
# endif /* xOUT_OF_DISK_SPACE_TEST */

    return( status );
}
