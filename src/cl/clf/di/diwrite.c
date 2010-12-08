/*
** Copyright (c) 2004 Ingres Corporation
**
*/

#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <systypes.h>
#include   <er.h>
#include   <cs.h>
#include   <fdset.h>
#include   <csev.h>
#include   <di.h>
#include   <dislave.h>
#include   "dilocal.h"
#include   <me.h>
#include   <cldio.h>
#include   <csinternal.h>
#include   "dilru.h"

/* unix includes */
#include    <errno.h>

#ifdef	    xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */

#ifdef      xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif	    /* xCL_007_FILE_H_EXISTS */

#include <signal.h>

/**
**
**  Name: DIWRITE.C - This file contains the UNIX DIwrite() routine.
**
**  Description:
**      The DIWRITE.C file contains the DIwrite() routine.
**
**        DIwrite - Writes a page to a file.
**
** History:
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	05-nov-87 (mikem)
**          set number of pages written.
**	29-mar-88 (anton)
**          DI in slave proces using CS events
**	17-oct-88 (jeff)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	24-jan-89 (roger)
**	    Make use of new CL error descriptor.
**	06-feb-89 (mikem)
**	    Return CL_ERR_DESC from DIlru_open().
**	28-Feb-1989 (fredv)
**	    Replaced xDI_xxx by corresponding xCL_xxx defs in clconfig.h.
**	    Include <systypes.h> and <seek.h>.
**	23-mar-89 (mikem)
**	    update io_system_eof when necessary (bug 4854).
**	25-Apr-89 (GordonW)
**	    Don't use bcopy, use MECOPY macro call. bcopy is unportable.
**	02-May-89 (GordonW)
**	    replace mecopy.h with me.h
**	12-jun-89 (rogerk & mikem)
**	    Change DIwrite to DIsense() the file if the write is beyond
**	    current eof (as maintained in the DI_IO structure).
**    10-jul-89 (mikem)
**	    Return DI_EXCEED_LIMIT if out of disk space, rather than
**	    BAD_WRITE.  Also add some debugging code to make it easier to
**	    test that the server handles out of disk space correctly (by
**	    returning out of disk space out of DIwrite based on a gloal set
**	    by DIalloc every N times called).
**    23-Jan-90 (anton)
**	    Call DI_sense instead of DIsense to prevent multiple DIlru_opens
**	    and use of two CSevcbs.
**	2-Feb-90 (anton)
**	    Don't always copy CL_ERR_DESC
**	6-Feb-90 (jkb)
**	    Change write to IIdio_write which combines the write and lseek
**	    commands and makes direct io available for Sequent
**	01-Oct-90 (anton)
**	    Reentrant CL changes and don't bother with MECOPY_VAR_MACRO
**	07-mar-91 (rudyw)
**	    ifdef a section of code to remove a test referencing EDQUOT
**	    which does not exist on odt_us5.
**	22-apr-91 (rudyw)
**	    The previous submission was ifdef'd for odt_us5 when in fact
**	    it should be ifdef'd based on EDQUOT. If EDQUOT is defined, the
**	    conditional test code with EDQUOT and ENOSPC will be used.
**	    Otherwise, the conditional test must only be for ENOSPC.
**	5-aug-1991 (bryanp)
**	    Added support for I/O directly from server shared memory,
**	    bypassing the copy through the server segment if possible. Any
**	    DIwrite call which is from shared memory is performed directly
**	    by the slave.
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**      03-mar-1992 (jnash)
**          Fix LG slave problem noted when Sun mmap() support
**          introduced, change slave logic to send to the slave the
**          segment id "key" rather than "segid" (segid value not
**          the same in the slave).
**	30-October-1992 (rmuth)
**	    Prototype, remove unused forward reference to lseek,
**	    include "dilru.h", remove include <seek.h> 
**	30-nov-1992 (rmuth)
**	    - Include <cldio.h>
**	    - DIlru error checking.
**      10-oct-1993 (mikem)
**          bug #47624
**          Bug 47624 resulted in CSsuspend()'s from the DI system being woken
**          up early.  Mainline DI would then procede while the slave would
**          actually be processing the requested asynchronous action.  Various
**          bad things could happen after this depending on timing: (mainline
**          DI would change the slave control block before slave read it,
**          mainline DI would call DIlru_release() and ignore it failing which
**          would cause the control block to never be freed eventually leading
**          to the server hanging when it ran out of slave control blocks, ...
**
**          Fixes were made to scf to hopefully eliminate the unwanted
**          CSresume()'s.  In addition defensive code has been added to DI
**          to catch cases of returning from CSresume while the slave is
**          operating, and to check for errors from DIlru_release().  Before
**          causing a slave to take action the master will set the slave
**          control block status to DI_INPROGRESS, the slave in turn will not
**          change this status until it has completed the operation.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	23-aug-1993 (bryanp)
**	    If segment isn't yet mapped, map it!
**	10-dec-1993 (rmuth)
**	    If fail the past io_allocated_eof test then make sure that we
**	    unset the errno value in CL_ERR_DESC set by SETCLERR. This was
**	    causing confusion as we were logging random errno's to the
**	    errlog.log
**      31-jan-94 (mikem)
**          sir #57671
**	    The transfer size of slave I/O is now stored in 
**	    Cs_srv_block.cs_size_io_buf, rather than a constant
**	    DI_FILE_BUF_SIZE.
**	18-apr-1994 (jnash)
**	    fsync project.  DIforce file if fsync() and no O_SYNC. 
**	20-jun-1995 (amo ICL)
**	    Added DI_async_write for async io
**	03-jun-1996 (canor01)
**	    Added semaphore protection for DI_IO structure for use with operating
**	    system threads.
**	18-nov-1996 (canor01)
**	    Put DI_async_write in appropriate "#ifdef" to prevent compiler
**	    errors on platforms where this is not supported.
**      22-jan-1997 (canor01)
**          When using slaves with OS threads, release DI_IO semaphore
**          earlier.
**    14-July-1997 (schte01)
**      For those platforms that do direct i/o (where the
**      seek and the write are separate functions), do not release and
**      reaquire the semaphore on the DI_IO block in DI_inproc_write.
**      This will protect against i/o being done by a different thread
**      in between the lseek and the write.
**    14-Aug-1997 (schte01)    
**      Add xCL_DIRECT_IO as a condition to the 14-July-1997 change
**      instead of the test for !xCL_ASYNCH_IO.
**	03-sep-1997 (muhpa01)
**	    Ifdef async I/O calls for hp8_us5 
**	11-Nov-1997 (jenjo02)
**	    Backed out change 432516 from diread, disense, diwrite.
**	    io_sem needs to be held during i/o operations to avoid 
**	    concurrency problems.
**	05-Nov-1997 (hanch04)
**	    Added code for 64 bit file system writes
**          Added LARGEFILE64 for files > 2gig
**          Change i4 to OFFSET_TYPE
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs to dilocal.h
**	    Take io_sem only when file's eof is being updated.
**	    Asynch fds are now pinned, so DIlru_release() must be called.
**	18-may-1998 (muhpa01)
**	    Removed hp8_us5 ifdef's for async calls - neither OS_THREADS nor
**	    async i/o is supported & the ifdef's were disabling for the
**	    hpb_us5 port, which does support async i/o.
**	01-oct-1998 (somsa01)
**	    Return DI_NODISKSPACE when we are out of disk space.
**	16-Nov-1998 (jenjo02)
**	    Distinguish Log writes from other Disk writes.
**	22-Dec-1998 (jenjo02)
**	    If DI_FD_PER_THREAD is defined, call IIdio_write() instead of
**	    pwrite().
**	29-Jun-1999 (jenjo02)
**	    Incorrectly turning off CS_DIOR_MASK | CS_LIOR_MASK in
**	    cs_memory instead of CS_DIOW_MASK | CS_LIOW_MASK.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Deleted cs_dio, cs_dio_done, cs_dio_wait, cs_dio_time stats,
**	    keep KB written stat.
**	01-nov-2002 (somsa01)
**	    Changed xCL_DIRECT_IO to xCL_NO_ATOMIC_READ_WRITE_IO.
**	31-May-2005 (schka24)
**	    Remove obsolete/wrong i2 cast on slave mecopy call.
**	8-Sep-2005 (schka24)
**	    Reverse dec-98 change, use pwrite if fd-per-thread.
**	15-Nov-2010 (kschendel) SIR 124685
**	    Delete unused variables.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Compiler warning fix.
**/

/* # defines */

/* typedefs */

/* forward references */
static STATUS DI_inproc_write(
			    DI_IO	*f,
			    DI_OP	*diop,
			    char        *buf,
			    i4	page,
			    i4	num_of_pages,
			    CL_ERR_DESC *err_code );

static STATUS DI_async_write(
			    DI_IO	*f,
			    DI_OP	*diop,
			    char        *buf,
			    i4	page,
			    i4	num_of_pages,
			    CL_ERR_DESC *err_code );

static STATUS DI_slave_write(
			    DI_IO	*f,
			    DI_OP	*diop,
			    char        *buf,
			    i4	page,
			    i4	num_of_pages,
			    CL_ERR_DESC *err_code);


/* externs */

/* statics */


/*{
** Name: DIwrite -  Writes  page(s) of a file to disk.
**
** Description:
**      The DIwrite routine is used to write pages of a direct access 
**      file.  This routine should be flexible enough to write multiple
**      contiguous pages.  The number of pages to write is indicated
**      as an input parameter,  This value is updated to indicate the
**      actual number of pages written.  A synchronous write is preferred
**      but not required.
**   
**	The buffer address from which the data is to be written is examined
**	to see if it is in shared memory. If so, we then instruct the slave
**	to write the page(s) directly from the target buffer. Otherwise, we
**	copy the page(s) from the buffer into the server segment, and then
**	instruct the slave to write the page(s) from the server segment.
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      n                    Pointer to value indicating number of pages to 
**			     write.
**      page                 Value indicating page(s) to write.
**      buf                  Pointer to page(s) to write.
**      
** Outputs:
**      f                    Updates the file control block.
**      n                    Pointer to value indicating number of pages 
**			     written.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADFILE          Bad file context.
**          DI_BADWRITE         Error writing.
**          DI_BADPARAM         Parameter(s) in error.
**          DI_ENDFILE          Write past end of file.
**	    DI_BADLRU_RELEASE	Error releasing open file.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    26-mar-87 (mmm)    
**          Created new for 6.0.
**    06-feb-89 (mikem)
**	    Return CL_ERR_DESC from DIlru_open().
**    23-mar-89 (mikem)
**	    update io_system_eof when necessary (bug 4854).
**    10-jul-89 (rogerk & mikem)
**	    When asked to write a page that is past our cached EOF marker, call
**	    DIsense to check the actual EOF before signalling an error.  The
**	    page may have been allocated by a different server and our copy of
**	    the EOF has just not been updated yet.
**
**	    This case can also come up in a single server case now, as we 
**	    continue to cache pages in the buffer manager even when the table 
**	    is closed.  This can result in a DIwrite() being performed on a
**	    newly opened file without ever doing a DIread() (where eof info
**	    was previously obtained).
**    10-jul-89 (mikem)
**	    Return DI_EXCEED_LIMIT if out of disk space, rather than
**	    BAD_WRITE.  Also add some debugging code to make it easier to
**	    test that the server handles out of disk space correctly (by
**	    returning out of disk space out of DIwrite based on a gloal set
**	    by DIalloc every N times called).
**    23-Jan-90 (anton)
**	    Call DI_sense instead of DIsense to prevent multiple DIlru_opens
**	    and use of two CSevcbs.
**	2-Feb-90 (anton)
**	    Don't always copy CL_ERR_DESC
**	6-Feb-90 (jkb)
**	    Change write to IIdio_write which combines the write and lseek
**	    commands and makes direct io available for Sequent
**	5-aug-1991 (bryanp)
**	    Added support for I/O directly from server shared memory,
**	    bypassing the copy through the server segment if possible.
**      03-mar-1992 (jnash)
**          Fix LG slave problem noted when Sun mmap() support
**          introduced, change slave logic to send to the slave the
**          segment id "key" rather than "segid" (segid value not
**          the same in the slave).
**	30-October-1992 (rmuth)
**	    Prototype and make sure we have opened the file before
**	    we close it.
**	30-nov-1992 (rmuth)
**	    - Include <cldio.h>
**	    - DIlru error checking, this was a major restructuring of the
**	      code. No Change in functionality.
**	10-dec-1993 (rmuth)
**	    If fail the past io_allocated_eof test then make sure that we
**	    unset the errno value in CL_ERR_DESC set by SETCLERR. This was
**	    causing confusion as we were logging random errno's to the
**	    errlog.log
**      31-jan-94 (mikem)
**          sir #57671
**          The transfer size of slave I/O is now stored in
**          Cs_srv_block.cs_size_io_buf, rather than a constant
**          DI_FILE_BUF_SIZE.
**	18-apr-1994 (jnash)
**	    fsync project.  Call DIforce() on systems where fsync() exists 
**	    but O_SYNC does not (hopefully never). 
**	20-jun-1995 (amo ICL)
**	    Added call on DI_async_write for async io
**	20-Apr-1998 (merja01)
**		Move "#" to column 1 to correct compile errors on axp_osf.
**	01-oct-1998 (somsa01)
**	    Return DI_NODISKSPACE when we are out of disk space.
**	29-Oct-1998 (schte01)
**		Move "#" to column 1 to correct compile errors on axp_osf.
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
*/
STATUS
DIwrite(
    DI_IO	   *f,
    i4             *n,
    i4        page,
    char           *buf,
    CL_ERR_DESC    *err_code)
{
    STATUS			big_status = OK, small_status = OK, r_status;
    i4			num_of_pages;
    i4			last_page_to_write;
    CL_ERR_DESC    		lerr_code;
    DI_OP			diop;

    /* default returns */

    CL_CLEAR_ERR( err_code );

    num_of_pages = *n;
    *n = 0;
    if (num_of_pages <= 0)
	return (DI_BADPARAM);

    last_page_to_write = page + num_of_pages - 1;

    diop.di_flags = 0;

    if (f->io_type != DI_IO_ASCII_ID)
        return(DI_BADFILE);

    if (f->io_mode != DI_IO_WRITE)
        return(DI_BADWRITE);

    /* Count another write */
    f->io_stat.write++;

    /* 
    ** get open file descriptor for the file
    */
    if (big_status = DIlru_open(f, FALSE, &diop, err_code))
	return(big_status);

    /* 
    ** now check for write within bounds of the file 
    */
    if (last_page_to_write > f->io_alloc_eof) 
    {
	i4	real_eof;

	/*
	** DI_sense updates f->io_alloc_eof with the protection
	** of io_sem (OS_THREADS), so there's no need to
	** duplicate that update here.
	*/
	big_status = DI_sense(f, &diop, &real_eof, err_code);

	if (big_status == OK)
	{
	    if (last_page_to_write > f->io_alloc_eof)
	    {
		small_status = DI_ENDFILE;
		SETCLERR(err_code, 0, ER_write);

		/*
		** The above sets errno as errno will be left over from
		** a previous call zero it out to avoid confusion.
		*/
		err_code->errnum = 0;
	     }
	 }
    }

    if (big_status == OK && small_status == OK)
    {
#ifdef xOUT_OF_DISK_SPACE_TEST
	if ((f->io_open_flags & DI_O_NODISKSPACE_DEBUG) &&
	    (last_page_to_write > f->io_logical_eof)    &&
	    (last_page_to_write <= f->io_alloc_eof))
	{
	    f->io_open_flags &= ~DI_O_NODISKSPACE_DEBUG;

	    small_status = DI_NODISKSPACE;
	    SETCLERR(err_code, 0, ER_write);
	    err_code->errnum = ENOSPC;

	    TRdisplay(
		"DIwrite(): Returning false DI_NODISKSPACE, page %d\n", page);
	}
	else
#endif /* xOUT_OF_DISK_SPACE_TEST */
	    
	{
	    if (Di_slave)
	    {
		big_status = DI_slave_write( f, &diop, buf, page, num_of_pages,
					     err_code );
	    }
	    else
# if defined(OS_THREADS_USED) || defined(xCL_ASYNC_IO)
	    if (Di_async_io)
	    {
		big_status = DI_async_write( f, &diop, buf, page, num_of_pages,
					      err_code );
	    }
	    else
# endif /* OS_THREADS_USED || xCL_ASYNC_IO */
	    {
		big_status = DI_inproc_write( f, &diop, buf, page, num_of_pages,
					      err_code );
	    }

	    if (big_status == OK && small_status == OK)

# if defined(xCL_010_FSYNC_EXISTS) && !defined(O_SYNC)
	    {
		/*
		** Due to lru activity, this code assumes that a force on any 
		** file descriptor forces pages for all open files.  If not 
		** the case, fsync() logic must be installed in the slave.
		*/
		big_status = DIforce( f, err_code );
	    }
	    if (big_status == OK && small_status == OK)
# endif

		*n = num_of_pages;
	}
    }

    r_status = DIlru_release(&diop, &lerr_code);

    if (big_status)
	return( big_status );
    else if (small_status)
	return( small_status );

    return(r_status);

}

/*{
** Name: DI_slave_write -  Request a slave to writes page(s) to a file on disk.
**
** Description:
**	This routine was created to make DIwrite more readable once
**	error checking had been added. See DIwrite for comments.
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**	diop		     Pointer to dilru file context.
**      buf                  Pointer to page(s) to write.
**      page                 Value indicating page(s) to write.
**	num_of_pages	     number of pages to write
**      
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**	    other errors.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-nov-1992 (rmuth)
**	    Created.
**      10-oct-1993 (mikem)
**          bug #47624
**          Bug 47624 resulted in CSsuspend()'s from the DI system being woken
**          up early.  Mainline DI would then procede while the slave would
**          actually be processing the requested asynchronous action.  Various
**          bad things could happen after this depending on timing: (mainline
**          DI would change the slave control block before slave read it,
**          mainline DI would call DIlru_release() and ignore it failing which
**          would cause the control block to never be freed eventually leading
**          to the server hanging when it ran out of slave control blocks, ...
**
**          Fixes were made to scf to hopefully eliminate the unwanted
**          CSresume()'s.  In addition defensive code has been added to DI
**          to catch cases of returning from CSresume while the slave is
**          operating, and to check for errors from DIlru_release().  Before
**          causing a slave to take action the master will set the slave
**          control block status to DI_INPROGRESS, the slave in turn will not
**          change this status until it has completed the operation.
**
**          The off by one error was caused by the CSsuspend called by 
**	    DI_slave_send() returning early in the case of a DIwrite() of one 
**	    page.  The old write loop would increment disl->pre_seek before the
**	    slave had actually read the control block so the slave would write 
**	    the data from page N to the N+1 location in the file.  The 
**	    DI_INPROGRESS flag should stop this, and at least in the one page 
**	    write case we no longer increment disl->pre_seek.
**	23-aug-1993 (bryanp)
**	    If segment isn't yet mapped, map it!
**	01-oct-1998 (somsa01)
**	    Return DI_NODISKSPACE when we are out of disk space.
*/
static STATUS
DI_slave_write(
    DI_IO	*f,
    DI_OP	*diop,
    char        *buf,
    i4	page,
    i4	num_of_pages,
    CL_ERR_DESC *err_code)
{
    register DI_SLAVE_CB	*disl;
    ME_SEG_INFO			*seginfo;
    bool			direct_write;
    STATUS			big_status = OK, small_status = OK;
    STATUS			intern_status = OK, status;

    /* unix variables */
    int		bytes_to_write;

    do
    {
        disl = diop->di_evcb;
	bytes_to_write = (f->io_bytes_per_page * (num_of_pages));

	/*
	** Determine whether we're writing from shared memory, and set
	** up the segment ID and offset correctly.
	*/
	seginfo = ME_find_seg( buf, (char *)buf + bytes_to_write,
			       &ME_segpool);

	if (seginfo != 0 && (seginfo->flags & ME_SLAVEMAPPED_MASK) == 0)
	{
	    status = DI_lru_slmapmem(seginfo, &intern_status, &small_status);
	    if (status)
		break;
	}

        if (seginfo != 0 && (seginfo->flags & ME_SLAVEMAPPED_MASK) != 0)
	{
	    direct_write = TRUE;
	    MEcopy( (PTR)seginfo->key, sizeof(disl->seg_key),
	            (PTR)disl->seg_key);

	    disl->seg_offset = (char *)buf - (char *)seginfo->addr;
	}
	else
	{
	    direct_write = FALSE;
	    seginfo = ME_find_seg(disl->buf, disl->buf, &ME_segpool);
	    if (seginfo)
	    {
	        MEcopy( (PTR)seginfo->key, sizeof(disl->seg_key),
			(PTR)disl->seg_key);

	        disl->seg_offset= (char *)disl->buf - (char *)seginfo->addr;
	    }
	    else
	    {
	        small_status = DI_BADWRITE;
	    	break;
	    }
	}

	/* Send file properties to slave */
	FPROP_COPY(f->io_fprop,disl->io_fprop);

	disl->pre_seek = 
	    (OFFSET_TYPE)(f->io_bytes_per_page) * (OFFSET_TYPE)(page);
	disl->file_op = DI_SL_WRITE;

	/*
	** Write the data
	*/
	do 
	{
	    if (direct_write)
		disl->length = bytes_to_write;
	    else
	    {
		disl->length = min(bytes_to_write, Cs_srv_block.cs_size_io_buf);
		MEcopy((PTR)buf, disl->length, (PTR)disl->buf);
	    }

	    DI_slave_send( disl->dest_slave_no, diop,
		           &big_status, &small_status, &intern_status);

	    if (( big_status != OK ) || (small_status != OK ))
		break;

	    if ((small_status = disl->status) != OK )
	    {
		STRUCT_ASSIGN_MACRO(disl->errcode, *err_code);
	    }

	    if ((small_status != OK) || (disl->length == 0))
	    {
		switch( err_code->errnum )
		{
		case EFBIG:
		    small_status = DI_BADEXTEND;
		    break;
		case ENOSPC:
		    small_status = DI_NODISKSPACE;
		    break;
#ifdef EDQUOT
		case EDQUOT:
		    small_status = DI_EXCEED_LIMIT;
		    break;
#endif
		default:
		    small_status = DI_BADWRITE;
		    break;
		}

		break;
	    }

	    bytes_to_write -= disl->length;
	    buf += disl->length;
	    if (bytes_to_write > 0)
		disl->pre_seek += (OFFSET_TYPE)disl->length;

	} while ( bytes_to_write > 0);

    } while (FALSE);


    if (big_status != OK )
	small_status = big_status;

    if (small_status != OK )
        DIlru_set_di_error( &small_status, err_code, intern_status,
			    DI_GENERAL_ERR);

    return( small_status );
}

/*{
** Name: DI_inproc_write -   writes page(s) to a file on disk.
**
** Description:
**	This routine was created to make DIwrite more readable once
**	error checking had been added. See DIwrite for comments.
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**	diop		     Pointer to dilru file context.
**      buf                  Pointer to page(s) to write.
**      page                 Value indicating page(s) to write.
**	num_of_pages	     number of pages to write
**      
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**	    other errors.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-nov-1992 (rmuth)
**	    Created.
**    03-jun-1996 (canor01)
**	    Note in the scb that this is a DI wait.
**    05-May-1997 (merja01)
**      Changed preprocessor stmt for pwrite.  Not all platforms
**      using OS_THREADS have a pwrite function.  This function
**      seems to only be available on Solaris 2.4 where async IO
**      is not yet supported.
**    14-July-1997 (schte01)
**      For those platforms that do direct i/o (where the
**      seek and the write are separate functions), do not release and
**      reaquire the semaphore on the DI_IO block. This will protect
**      against i/o being done by a different thread in between the 
**      lseek and the write.
**    14-Aug-1997 (schte01)    
**      Add xCL_DIRECT_IO as a condition to the 14-July-1997 change
**      instead of the test for !xCL_ASYNCH_IO.
**	22-Dec-1998 (jenjo02)
**	    If DI_FD_PER_THREAD is defined, call IIdio_write() instead of
**	    pwrite().
**	01-oct-1998 (somsa01)
**	    Return DI_NODISKSPACE when we are out of disk space.
**  01-Apr-2004 (fanch01)
**      Add O_DIRECT support on Linux depending on the filesystem
**      properties, pagesize.  Fixups for misaligned buffers on read()
**      and write() operations.
**    13-apr-04 (toumi01)
**	Move stack variable declaration to support "standard" C compilers.
**	29-Jan-2005 (schka24)
**	    Ditch attempt to gather diow timing stats, not useful in
**	    the real world and generates excess syscalls on some platforms.
**	15-Mar-2006 (jenjo02)
**	    io_sem is not needed with thread affinity.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH, avoid entirely if PRIVATE.
**	    Delete copy-to-align, caller is supposed to do it now.
**	    Don't attempt SCB updating if not backend.
*/
static STATUS
DI_inproc_write(
    DI_IO	*f,
    DI_OP	*diop,
    char        *buf,
    i4	page,
    i4	num_of_pages,
    CL_ERR_DESC *err_code )
{
    STATUS	status = OK;
    CS_SCB	*scb;
    i4		saved_state;

    /* unix variables */
    int		bytes_written;
    int		bytes_to_write;
    OFFSET_TYPE lseek_offset;
    /* 
    ** seek to place to write 
    */
    lseek_offset = 
	(OFFSET_TYPE)(f->io_bytes_per_page) * (OFFSET_TYPE)page;

    bytes_to_write = (f->io_bytes_per_page * (num_of_pages));

    if (Di_backend)
    {
	CSget_scb(&scb);
	if ( scb )
	{
	    saved_state = scb->cs_state;
	    scb->cs_state = CS_EVENT_WAIT;

	    if (f->io_open_flags & DI_O_LOG_FILE_MASK)
	    {
		scb->cs_memory = CS_LIOW_MASK;
		scb->cs_liow++;
		Cs_srv_block.cs_wtstatistics.cs_liow_done++;
		Cs_srv_block.cs_wtstatistics.cs_liow_waits++;
		Cs_srv_block.cs_wtstatistics.cs_liow_kbytes
		    += bytes_to_write / 1024;
	    }
	    else
	    {
		scb->cs_memory = CS_DIOW_MASK;
		scb->cs_diow++;
		Cs_srv_block.cs_wtstatistics.cs_diow_done++;
		Cs_srv_block.cs_wtstatistics.cs_diow_waits++;
		Cs_srv_block.cs_wtstatistics.cs_diow_kbytes
		    += bytes_to_write / 1024;
	    }
	}
    }

# if  defined(OS_THREADS_USED) && defined(xCL_NO_ATOMIC_READ_WRITE_IO)
    if ( !Di_thread_affinity && (f->io_fprop & FPROP_PRIVATE) == 0)
	CS_synch_lock( &f->io_sem );
# endif /* OS_THREADS_USED && !xCL_NO_ATOMIC_READ_WRITE_IO */

# if  defined(OS_THREADS_USED) && !defined(xCL_NO_ATOMIC_READ_WRITE_IO)
    bytes_written =
#ifdef LARGEFILE64
 	 pwrite64( (int)diop->di_fd, buf, bytes_to_write, lseek_offset );
#else /*  LARGEFILE64 */
 	 pwrite( (int)diop->di_fd, buf, bytes_to_write, lseek_offset );
#endif /* LARGEFILE64 */
# else /* OS_THREADS_USED  !xCL_NO_ATOMIC_READ_WRITE_IO */
    bytes_written =
 	 IIdio_write( (int)diop->di_fd, buf, bytes_to_write,
 	 	       lseek_offset, 0, 
		       f->io_fprop,
		       err_code );
# endif /* OS_THREADS_USED */

    if ( bytes_written != bytes_to_write )
    {
	SETCLERR(err_code, 0, ER_write);

	switch( err_code->errnum )
	{
	case EFBIG:
	    status = DI_BADEXTEND;
	    break;
	case ENOSPC:
	    status = DI_NODISKSPACE;
	    break;
#ifdef EDQUOTA
	case EDQUOT:
	    status = DI_EXCEED_LIMIT;
	    break;
#endif
	default:
	    if (err_code->errnum == 0)
		status = DI_ENDFILE;
	    else
		status = DI_BADWRITE;
	    break;
	}
    }

# if  defined(OS_THREADS_USED) && defined(xCL_NO_ATOMIC_READ_WRITE_IO)
    if ( !Di_thread_affinity && (f->io_fprop & FPROP_PRIVATE) == 0)
	CS_synch_unlock( &f->io_sem );
# endif /* OS_THREADS_USED && xCL_NO_ATOMIC_READ_WRITE_IO */

    if ( Di_backend && scb )
    {

	scb->cs_memory &= ~(CS_DIOW_MASK | CS_LIOW_MASK);
	scb->cs_state = saved_state;
    }

    return( status );
}
# if defined(OS_THREADS_USED) || defined(xCL_ASYNC_IO)
/*{
** Name: DI_async_write -   writes page(s) to a file on disk.
**
** Description:
**	This routine was created to interface with async io routines
**	where such routines are available
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**	diop		     Pointer to dilru file context.
**      buf                  Pointer to page(s) to write.
**      page                 Value indicating page(s) to write.
**	num_of_pages	     number of pages to write
**      
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**	    other errors.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    20-jun-1995 (amo ICL)
**	    Created.
**	01-oct-1998 (somsa01)
**	    Return DI_NODISKSPACE when we are out of disk space.
*/
static STATUS
DI_async_write(
    DI_IO	*f,
    DI_OP	*diop,
    char        *buf,
    i4	page,
    i4	num_of_pages,
    CL_ERR_DESC *err_code )
{
    STATUS	status = OK;
    CS_SCB	*scb;
    int		saved_state;
    i4	start_time, elapsed;

    /* unix variables */
    OFFSET_TYPE lseek_offset;
    int		bytes_written;
    int		bytes_to_write;

    /* 
    ** seek to place to write 
    */
    lseek_offset = (OFFSET_TYPE)f->io_bytes_per_page * (OFFSET_TYPE)page;
    bytes_to_write = (f->io_bytes_per_page * (num_of_pages));

    CSget_scb(&scb);
    if ( scb )
    {
	saved_state = scb->cs_state;
	scb->cs_state = CS_EVENT_WAIT;

	if (f->io_open_flags & DI_O_LOG_FILE_MASK)
	{
	    scb->cs_memory = CS_LIOW_MASK;
	    scb->cs_liow++;
	    Cs_srv_block.cs_wtstatistics.cs_liow_done++;
	    Cs_srv_block.cs_wtstatistics.cs_liow_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_liow_kbytes
		+= bytes_to_write / 1024;
	}
	else
	{
	    scb->cs_memory = CS_DIOW_MASK;
	    scb->cs_diow++;
	    Cs_srv_block.cs_wtstatistics.cs_diow_done++;
	    Cs_srv_block.cs_wtstatistics.cs_diow_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_diow_kbytes
		+= bytes_to_write / 1024;
	}
	start_time = CS_checktime();
    }

# if defined(OS_THREADS_USED) && !defined(xCL_ASYNC_IO)
    bytes_written =
 	 DI_thread_rw( O_WRONLY, diop, buf, bytes_to_write,
 	 	       lseek_offset, NULL, err_code);
# else /* OS_THREADS_USED */
    bytes_written =
 	 DI_aio_rw( O_WRONLY, diop, buf, bytes_to_write,
 	 	       lseek_offset, NULL, err_code);
# endif /* OS_THREADS_USED */
    if ( bytes_written != bytes_to_write )
    {
	SETCLERR(err_code, 0, ER_write);

	switch( err_code->errnum )
	{
	case EFBIG:
	    status = DI_BADEXTEND;
	    break;
	case ENOSPC:
	    status = DI_NODISKSPACE;
	    break;
#ifdef EDQUOTA
	case EDQUOT:
	    status = DI_EXCEED_LIMIT;
	    break;
#endif
	default:
	    if (err_code->errnum == 0)
		status = DI_ENDFILE;
	    else
		status = DI_BADWRITE;
	    break;
	}

    }

    if ( scb )
    {
	elapsed = CS_checktime() - start_time;

	scb->cs_memory &= ~(CS_DIOW_MASK | CS_LIOW_MASK);
	scb->cs_state = saved_state;
	if (f->io_open_flags & DI_O_LOG_FILE_MASK)
	    Cs_srv_block.cs_wtstatistics.cs_liow_time +=
		elapsed;
	else
	    Cs_srv_block.cs_wtstatistics.cs_diow_time +=
		elapsed;
    }

    return( status );
}
# endif /* OS_THREADS_USED || xCL_ASYNC_IO */
