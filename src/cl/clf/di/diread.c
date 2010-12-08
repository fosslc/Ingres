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
#include   "dilru.h"
#include   <csinternal.h>

#include    <errno.h>

#ifdef     xCL_006_FCNTL_H_EXISTS
#include   <fcntl.h>
#endif     /* xCL_006_FCNTL_H_EXISTS */

#ifdef     xCL_007_FILE_H_EXISTS
#include   <sys/file.h>
#endif     /* xCL_007_FILE_H_EXISTS */

/**
**
**  Name: DIREAD.C - This file contains the UNIX DIread() routine.
**
**  Description:
**      The DIREAD.C file contains the DIread() routine.
**
**        DIread - Reads a page from a file.
**
** History:
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	29-mar-88 (anton)
**          DI in slave proces using CS events
**	28-sep-88 (jeff)
**	    update pages parameter if call fails.
**	17-oct-88 (jeff)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	24-jan-89 (roger)
**	    Make use of new CL error descriptor.
**      06-feb-89 (mikem)
**	    Cleared CL_ERR_DESC, pass back err_code from DIlru_open(), 
**	    invalidate io_type on error, and "#include me.h".
**	28-Feb-1989 (fredv)
**	    Replaced xDI_xxx by corresponding xCL_xxx defs in clconfig.h.
**	    Include <systypes.h> and <seek.h>.
**      23-mar-89 (mmm)
**	    bug fix for b4854 (see bug fix workaround comments in header and
**	    in code.)
**	02-May-89 (GordonW)
**	    replace mecopy.h with me.h
**	2-Feb-90 (anton)
**	    Don't always copy CL_ERR_DESC
**	6-Feb-90 (jkb)
**	    Add IIdio_read so direct io is available for Sequent.
**	01-Oct-90 (anton)
**	    Reentrant CL changes & Don't bother with MECOPY_VAR_MACRO
**	5-aug-1991 (bryanp)
**	    Added support for I/O directly to server shared memory, bypassing
**	    the copy through the server segment if possible. Any DIread call
**	    which is into shared memory is performed directly by the slave.
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	12-dec-1991 (bryanp)
**	    Added support for DIread on a raw log file for LG's use.
**	03-mar-1992 (jnash)
**	    Fix LG slave problem noted when Sun mmap() support 
** 	    introduced, change slave logic to send to the slave the 
**	    segment id "key" rather than "segid" (segid value not 
**	    the same in the slave).
**	30-October-1992 (rmuth)
**	    Prototype, remove unused forward reference to lseek,
**	    include "dilru.h", remove <seek.h>
**	30-nov-1992 (rmuth)
**	    Various
**	     - Include <cldio.h>
**	     - Use DI_sense to find out size of file.
**	     - DIlru error checking
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	23-aug-1993 (bryanp)
**	    If memory segment isn't yet mapped, map it.
**      10-dec-1993 (rmuth)
**          If fail the past io_allocated_eof test then make sure that we
**          unset the errno value in CL_ERR_DESC set by SETCLERR. This was
**          causing confusion as we were logging random errno's to the
**          errlog.log
**      31-jan-94 (mikem)
**          sir #57671
**          The transfer size of slave I/O is now stored in
**          Cs_srv_block.cs_size_io_buf, rather than a constant
**          DI_FILE_BUF_SIZE.
**      19-apr-1995 (canor01)
**          change errno to errnum for reentrancy
**	20-jun-1995 (amo ICL)
**	    Added DI_async_read for async io
**	03-jun-1996 (canor01)
**	    Add semaphore protection for DI_IO structure for use with
**	    operating-system threads.
**	18-nov-1996 (canor01)
**	    Put DI_async_read in appropriate "#ifdef" to prevent compiler
**	    errors on platforms where this is not supported.
**	22-jan-1997 (canor01)
**	    When using slaves with OS threads, release DI_IO semaphore
**	    earlier.
**	14-apr-1997 (schte01 for kosma01)
**	    Change an apperant typo, xDL_ASYNC_IO to xCL_ASYNC_IO.
**    14-July-1997 (schte01)
**      For those platforms that do direct i/o (where the
**      seek and the read are separate functions), do not release and
**      reaquire the semaphore on the DI_IO block in DI_inproc_read.
**      This will protect against i/o being done by a different thread
**      in between the lseek and the read.
**    14-Aug-1997 (schte01)    
**      Add xCL_DIRECT_IO as a condition to the 14-July-1997 change
**      instead of the test for !xCL_ASYNCH_IO.
**    03-sep-1997 (muhpa01)
**      Ifdef async calls for hp8_us5
**	11-Nov-1997 (jenjo02)
**	    Backed out change 432516 from diread, disense, diwrite.
**	    io_sem needs to be held during i/o operations to avoid 
**	    concurrency problems.
**	21-Oct-1997 (jenjo02)
**	    Rearranged code such that io_sem is only taken and held when
**	    neccessary, i.e., while io_alloc_eof is being updated.
**      03-Nov-1997 (hanch04)
**          Added LARGEFILE64 for files > 2gig
**          Change i4 to OFFSET_TYPE
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs to dilocal.h
**	    Take io_sem only when io_alloc_eof is being updated.
**	    Asynch fds are now pinned, so DIlru_release() must be called.
**	18-may-1998 (muhpa01)
**	    Remove ifdef's for async calls for hp8_us5 - neither OS_THREADS
**	    nor async i/o is supported & the ifdef's were disabling for
**	    the hpb_us5 port, which does support async i/o.
**	16-Nov-1998 (jenjo02)
**	    Distinguish Log reads from other Disk reads.
**	22-Dec-1998 (jenjo02)
**	    If DI_FD_PER_THREAD is defined, call IIdio_read() instead of
**	    pread().
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Deleted cs_dio, cs_dio_done, cs_dio_wait, cs_dio_time stats,
**	    keep KB read stat.
**	01-nov-2002 (somsa01)
**	    Changed xCL_DIRECT_IO to xCL_NO_ATOMIC_READ_WRITE_IO.
**	31-May-2005 (schka24)
**	    Remove obsolete/wrong i2 cast on slave mecopy call.
**	8-Sep-2005 (schka24)
**	    Reverse dec-98 change, call pread if fd-per-thread.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.  Test for backend before messing with
**	    SCB states.
**	15-Nov-2010 (kschendel) SIR 124685
**	    Delete unused variables.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Compiler warning fix.
**/

/* # defines */

/* typedefs */

/* forward references */
static STATUS DI_slave_read(
			    DI_IO	*f,
			    DI_OP	*diop,
			    char        *buf,
			    i4	page,
			    i4	num_of_pages,
			    i4	*n,
			    CL_ERR_DESC *err_code);

static STATUS DI_inproc_read(
			    DI_IO	*f,
			    DI_OP	*diop,
			    char        *buf,
			    i4	page,
			    i4	num_of_pages,
			    i4	*n,
			    CL_ERR_DESC *err_code );

static STATUS DI_async_read(
			    DI_IO	*f,
			    DI_OP	*diop,
			    char        *buf,
			    i4	page,
			    i4	num_of_pages,
			    i4	*n,
			    CL_ERR_DESC *err_code );


/* externs */

/* statics */


/*{
** Name: DIread - Read a page of a file.
**
** Description:
**      The DIread routine is used to read pages of a direct access 
**      file.   For the large block read option, the number of pages
**      to read is an input parameter to this routine.  It will
**      return the number of pages it read, since at
**      end of file it may read less pages than requested.
**      If multiple page reads are requested, the buffer is assumed
**      to be large enough to hold n pages.  The size of a page is 
**      determined at create.
**      
**	BUG FIX WORKAROUND (b4854):
**	
**	The current mainline code (in the case of reading from a 
**	temporary file) expects that a DIread past logical
**	end of file, but within allocated end of file will return with
**	no error.  The value of the data retrieved is undefined.  To
**	make the current code work, DIread on unix has been changed to
**	meet these requirements, but it hoped that mainline code in the
**	future will be changed to not rely on this behaviour.
**
**	The buffer address into which the data is to be read is examined to
**	see if it is in shared memory. If so, we then instruct the slave to
**	read the page(s) directly into the target buffer. Otherwise, we read
**	the page(s) into the buffer in the server segment, and then copy the
**	data from the server segment to the target address.
**      
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      n                    Pointer to value of number of pages to read.
**      page                 Value indicating page to begin reading.
**      buf                  Pointer to the area to hold
**                           page(s) being read.
**      
** Outputs:
**      n                    Number of pages read.
**      f                    Updates the file control block.
**      buf                  Pointer to the page read.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADFILE       	Bad file context.
**          DI_BADREAD       	Error reading file.
**          DI_BADPARAM      	Parameter(s) in error.
**          DI_ENDFILE       	Not all blocks read.
**	    DI_BADLRU_RELEASE	Error releasing file.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    26-mar-87 (mmm)    
**          Created new for 6.0.
**    23-mar-89 (mmm)
**	    bug fix for b4854 (see bug fix workaround comments in header and
**	    in code.)
**	25-Apr-89 (GordonW)
**	    Don't use "bcopy" but use MECOPY macro call. bcopy is unportable.
**	07-may-89 (russ)
**	    Add missing semicolon to MECOPY_VAR_MACRO.
**	2-Feb-90 (anton)
**	    Don't always copy CL_ERR_DESC
**	6-Feb-90 (jkb)
**	    Add IIdio_read so direct io is available for Sequent.
**	5-aug-1991 (bryanp)
**	    Added support for I/O directly to server shared memory, bypassing
**	    the copy through the server segment if possible.
**	12-dec-1991 (bryanp)
**	    Added support for DIread on a raw log file for LG's use. In this
**	    case, all that had to happen was to replace the direct "lseek"
**	    call with a call to the IIdio code which supports file size
**	    determination for raw log files.
**      03-mar-1992 (jnash)
**          Fix LG slave problem noted when Sun mmap() support
**          introduced, change slave logic to send to the slave the
**          segment id "key" rather than "segid" (segid value not
**          the same in the slave).
**	30-October-1992 (rmuth)
**	    Prototype.
**	30-nov-1992 (rmuth)
**	    - Use DI_sense to find out the size of a file.
**	    - DIlru error checking
**      10-dec-1993 (rmuth)
**          If fail the past io_allocated_eof test then make sure that we
**          unset the errno value in CL_ERR_DESC set by SETCLERR. This was
**          causing confusion as we were logging random errno's to the
**          errlog.log
**      31-jan-94 (mikem)
**          sir #57671
**          The transfer size of slave I/O is now stored in
**          Cs_srv_block.cs_size_io_buf, rather than a constant
**          DI_FILE_BUF_SIZE.
**	20-jun-1995 (amo ICL)
**	    Added call on DI_async_read for async io
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
*/
STATUS
DIread(
    DI_IO	   *f,
    i4        *n,
    i4        page,
    char           *buf,
    CL_ERR_DESC     *err_code )
{
    STATUS		small_status = OK, big_status = OK, r_status;
    i4		num_of_pages;
    i4		last_page_to_read;
    DI_OP		diop;
    CL_ERR_DESC         lerr_code ;


    /* default returns */
    CL_CLEAR_ERR( err_code );

    num_of_pages = *n;
    last_page_to_read = page + num_of_pages - 1;

    *n = 0;
    diop.di_flags = 0;
    if (num_of_pages <= 0)
	return (DI_BADPARAM);

    /* 
    ** check file control block pointer, return if bad. 
    */
    if (f->io_type != DI_IO_ASCII_ID)
	return( DI_BADFILE );

    /* Count another read */
    f->io_stat.read++;

    if (big_status = DIlru_open(f, FALSE, &diop, err_code))
	return(big_status);

    /*
    ** Sanity check to make sure we are reading within the bounds of
    ** the file. Note: we may still be reading garbage pages--it is
    ** up to the upper layers to guarantee that we are not doing this
    */
    if (last_page_to_read > f->io_alloc_eof )
    {
	i4 	real_eof;

	/*
	** DI_sense updates f->io_alloc_eof with the protection
	** of io_sem (OS_THREADS), so there's no need to
	** duplicate that update here.
	*/
	big_status = DI_sense(f, &diop, &real_eof, err_code);

	if (big_status == OK)
	{
	    if (last_page_to_read > f->io_alloc_eof)
	    {
		small_status = DI_ENDFILE;
		SETCLERR(err_code, 0, ER_read);

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
	if (Di_slave)
	{
	    big_status = DI_slave_read( f, &diop, buf, page, num_of_pages,
					n, err_code );
	}
	else
# if defined(OS_THREADS_USED) || defined(xCL_ASYNC_IO)
	if (Di_async_io)
	{
	    big_status = DI_async_read( f, &diop, buf, page, num_of_pages,
					 n, err_code );
	}
	else
# endif /* OS_THREADS_USED || xCL_ASYNC_IO */
	{
	    big_status = DI_inproc_read( f, &diop, buf, page, num_of_pages,
					 n, err_code );
	}
    }

    r_status = DIlru_release(&diop, &lerr_code);

    if ( big_status )
	return( big_status );
    else if (small_status)
	return( small_status);
    return(r_status);

}

/*{
** Name: DI_slave_read -  Request a slave to read page(s) from a file on disk.
**
** Description:
**	This routine was created to make DIread more readable once
**	error checking had been added. See DIread for comments.
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**	diop		     Pointer to dilru file context.
**      buf                  Pointer to page(s) to read.
**      page                 Value indicating page(s) to read.
**	num_of_pages	     number of pages to read.
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
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	23-aug-1993 (bryanp)
**	    If memory segment isn't yet mapped, map it.
*/
static STATUS
DI_slave_read(
    DI_IO	*f,
    DI_OP	*diop,
    char        *buf,
    i4	page,
    i4	num_of_pages,
    i4	*n,
    CL_ERR_DESC *err_code)
{
    register DI_SLAVE_CB        *disl;
    ME_SEG_INFO                 *seginfo;
    bool                        direct_read;
    STATUS			small_status = OK, big_status = OK, 
				intern_status = OK, status;
    
    /* unix variables */
    int		bytes_to_read;
    int		bytes_read = 0;

    do
    {
	disl = diop->di_evcb;
	    
	disl->pre_seek = 
	    (OFFSET_TYPE)(f->io_bytes_per_page) * (OFFSET_TYPE)(page);
	bytes_to_read	= f->io_bytes_per_page * num_of_pages;
	    
	/*
	** determine whether we're reading into shared memory, and set
	** up the segment ID and offset correctly
	*/
	seginfo = ME_find_seg( buf, (char *)buf + bytes_to_read,
	   		       &ME_segpool);

	if (seginfo != 0 && (seginfo->flags & ME_SLAVEMAPPED_MASK) == 0)
	{
	    status = DI_lru_slmapmem(seginfo, &intern_status, &small_status);
	    if (status)
		break;
	}
	
	if (seginfo != 0 && (seginfo->flags & ME_SLAVEMAPPED_MASK) != 0)
	{
	    MEcopy( (PTR)seginfo->key, sizeof(disl->seg_key),
		    (PTR)disl->seg_key);

	    disl->seg_offset = (char *)buf - (char *)seginfo->addr;
	    direct_read = TRUE;
	}
	else
	{
	    direct_read = FALSE;
	    seginfo = ME_find_seg(disl->buf, disl->buf, &ME_segpool);
	    if (seginfo)
	    {
		MEcopy( (PTR)seginfo->key, sizeof(disl->seg_key),
		        (PTR)disl->seg_key);

		disl->seg_offset= (char *)disl->buf - (char *)seginfo->addr;
	    }
	    else
	    {
		small_status = DI_BADREAD;
		break;
	    }
	}


	/* 
	** seek to place to read 
	*/
	do 
	{
	    disl->file_op = DI_SL_READ;

	    /* Send file properties to slave */
	    FPROP_COPY(f->io_fprop,disl->io_fprop);
	    
	    if (direct_read)
		disl->length = bytes_to_read;
	    else
		disl->length = min(bytes_to_read, Cs_srv_block.cs_size_io_buf);

	    DI_slave_send( disl->dest_slave_no, diop,
			   &big_status, &small_status, &intern_status);
	    if (( big_status != OK ) || ( small_status != OK ))
		break;

	    if ((small_status = disl->status) != OK ) 
	    {
		STRUCT_ASSIGN_MACRO(disl->errcode, *err_code);
		small_status = DI_BADREAD;
		break;
	    }
	    else
	    {
		if ( disl->length == 0 )
		{
		    small_status = DI_ENDFILE;
#ifdef	xDEV_TST

		    TRdisplay("num_pages %d\n, read_op = %x", 
				  num_of_pages, 0x70000000);
		    DIlru_dump();
#endif	/* xDev_TST */
		    break;

		}
	    }

	    /*
	    ** Read data ok 
	    */
	    if (! direct_read)
	    {
		MEcopy((PTR)disl->buf, disl->length, (PTR)buf);
		buf += disl->length;
	    }

	    bytes_to_read -= disl->length;
	    disl->pre_seek += (OFFSET_TYPE)disl->length;
	    bytes_read	   += disl->length;

	} while ( bytes_to_read > 0);
    } while (FALSE);

    if ( bytes_read > 0 )
	*n = bytes_read / f->io_bytes_per_page;

    if ( big_status != OK )
	small_status = big_status;

    if (small_status != OK )
	DIlru_set_di_error( &small_status, err_code, intern_status,
			    DI_GENERAL_ERR);

    return(small_status);
  }

/*{
** Name: DI_inproc_read -   read page(s) from a file on disk.
**
** Description:
**	This routine was created to make DIread more readable once
**	error checking had been added. See DIread for comments.
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**	diop		     Pointer to dilru file context.
**      buf                  Pointer to page(s) to read.
**      page                 Value indicating page(s) to read.
**	num_of_pages	     number of pages to read
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
**    14-July-1997 (schte01)
**      For those platforms that do direct i/o (where the
**      seek and the read are separate functions), do not release and
**      reaquire the semaphore on the DI_IO block. This will protect
**      against i/o being done by a different thread in between the 
**      lseek and the read.
**    14-Aug-1997 (schte01)    
**      Add xCL_DIRECT_IO as a condition to the 14-July-1997 change
**      instead of the test for !xCL_ASYNCH_IO.
**	22-Dec-1998 (jenjo02)
**	    If DI_FD_PER_THREAD is defined, call IIdio_read() instead of
**	    pread().
**  01-Apr-2004 (fanch01)
**      Add O_DIRECT support on Linux depending on the filesystem
**      properties, pagesize.  Fixups for misaligned buffers on read()
**      and write() operations.
**    13-apr-04 (toumi01)
**	Move stack variable declaration to support "standard" C compilers.
**	29-Jan-2005 (schka24)
**	    Ditch attempt to gather dior timing stats, not useful in
**	    the real world and generates excess syscalls on some platforms.
**	15-Mar-2006 (jenjo02)
**	    io_sem is not needed with thread affinity.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Remove copy to aligned buffer, caller is supposed to do it.
*/
static STATUS
DI_inproc_read(
    DI_IO	*f,
    DI_OP	*diop,
    char        *buf,
    i4	page,
    i4	num_of_pages,
    i4	*n,
    CL_ERR_DESC *err_code )
{
    STATUS	status = OK;
    CS_SCB	*scb;
    i4		saved_state;

    /* unix variables */
    int		unix_fd;
    int		bytes_read = 0;
    int		bytes_to_read;
    OFFSET_TYPE	lseek_offset;

    /*
    ** Seek to place to read
    */
    lseek_offset  = (OFFSET_TYPE)f->io_bytes_per_page * (OFFSET_TYPE)page;

    bytes_to_read = f->io_bytes_per_page * num_of_pages;
    unix_fd = diop->di_fd;

    if (Di_backend)
    {
	CSget_scb(&scb);
	if ( scb )
	{
	    saved_state = scb->cs_state;
	    scb->cs_state = CS_EVENT_WAIT;

	    if (f->io_open_flags & DI_O_LOG_FILE_MASK)
	    {
		scb->cs_memory = CS_LIOR_MASK;
		scb->cs_lior++;
		Cs_srv_block.cs_wtstatistics.cs_lior_done++;
		Cs_srv_block.cs_wtstatistics.cs_lior_waits++;
		Cs_srv_block.cs_wtstatistics.cs_lior_kbytes
		    += bytes_to_read / 1024;
	    }
	    else
	    {
		scb->cs_memory = CS_DIOR_MASK;
		scb->cs_dior++;
		Cs_srv_block.cs_wtstatistics.cs_dior_done++;
		Cs_srv_block.cs_wtstatistics.cs_dior_waits++;
		Cs_srv_block.cs_wtstatistics.cs_dior_kbytes
		    += bytes_to_read / 1024;
	    }
	}
    }

# if defined( OS_THREADS_USED ) && (defined (xCL_NO_ATOMIC_READ_WRITE_IO))
    if ( !Di_thread_affinity && (f->io_fprop & FPROP_PRIVATE) == 0)
    {
	CS_synch_lock( &f->io_sem );
    }
# endif /* OS_THREADS_USED && xCL_NO_ATOMIC_READ_WRITE_IO */

# if defined( OS_THREADS_USED ) && (! defined (xCL_NO_ATOMIC_READ_WRITE_IO))
#ifdef LARGEFILE64
    bytes_read = pread64( unix_fd, buf, bytes_to_read, lseek_offset );
#else /* LARGEFILE64 */
    bytes_read = pread( unix_fd, buf, bytes_to_read, lseek_offset );
#endif /* LARGEFILE64 */

    if ( bytes_read != bytes_to_read )
    {
	SETCLERR(err_code, 0, ER_read);
# else /* OS_THREADS_USED */

    bytes_read = IIdio_read( unix_fd, buf, bytes_to_read,
 	    			  lseek_offset, 0, 
				  f->io_fprop,
				  err_code );

    if ( bytes_read != bytes_to_read )
    {
# endif /* OS_THREADS_USED && ! xCL_NO_ATOMIC_READ_WRITE_IO */

	if (bytes_read == -1)
	{
	    status = DI_BADREAD;
	}
	else
	{
	    status = DI_ENDFILE;
	}
    }
# if defined( OS_THREADS_USED ) && (defined (xCL_NO_ATOMIC_READ_WRITE_IO) )
    if ( !Di_thread_affinity && (f->io_fprop & FPROP_PRIVATE) == 0)
	CS_synch_unlock( &f->io_sem );
# endif /* OS_THREADS_USED && xCL_NO_ATOMIC_READ_WRITE_IO */

    if (Di_backend)
    {
	if ( scb )
	{
	    scb->cs_memory &= ~(CS_DIOR_MASK | CS_LIOR_MASK);
	    scb->cs_state = saved_state;
	}
    }

    if ( bytes_read > 0 )
	*n = bytes_read / f->io_bytes_per_page;

    return(status);
}

# if defined(OS_THREADS_USED) || defined(xCL_ASYNC_IO)
/*{
** Name: DI_async_read -   read page(s) asynchronously from a file on disk.
**
** Description:
**	This routine was created to interface with async io routines
**	where such routines are available.
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**	diop		     Pointer to dilru file context.
**      buf                  Pointer to page(s) to read.
**      page                 Value indicating page(s) to read.
**	num_of_pages	     number of pages to read
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
*/
static STATUS
DI_async_read(
    DI_IO	*f,
    DI_OP	*diop,
    char        *buf,
    i4	page,
    i4	num_of_pages,
    i4	*n,
    CL_ERR_DESC *err_code )
{
    STATUS	status = OK;
    CS_SCB	*scb;
    int		saved_state;
    i4 		start_time;

    /* unix variables */
    int		bytes_read = 0;
    int		bytes_to_read;
    OFFSET_TYPE	lseek_offset;

    /*
    ** Seek to place to read
    */
    lseek_offset  = (OFFSET_TYPE)(f->io_bytes_per_page) * (OFFSET_TYPE)(page);
    bytes_to_read = f->io_bytes_per_page * num_of_pages;

    CSget_scb(&scb);
    if ( scb )
    {
	saved_state = scb->cs_state;
	scb->cs_state = CS_EVENT_WAIT;

	if (f->io_open_flags & DI_O_LOG_FILE_MASK)
	{
	    scb->cs_memory = CS_LIOR_MASK;
	    scb->cs_lior++;
	    Cs_srv_block.cs_wtstatistics.cs_lior_done++;
	    Cs_srv_block.cs_wtstatistics.cs_lior_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_lior_kbytes
		+= bytes_to_read / 1024;
	}
	else
	{
	    scb->cs_memory = CS_DIOR_MASK;
	    scb->cs_dior++;
	    Cs_srv_block.cs_wtstatistics.cs_dior_done++;
	    Cs_srv_block.cs_wtstatistics.cs_dior_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_dior_kbytes
		+= bytes_to_read / 1024;
	}
	/* Clock the read */
	start_time = CS_checktime();
    }

# if defined(OS_THREADS_USED) && !defined(xCL_ASYNC_IO)
    bytes_read = DI_thread_rw( O_RDONLY, diop, buf, bytes_to_read,
 	    			      lseek_offset, NULL, err_code);
# else /* OS_THREADS_USED */
    bytes_read = DI_aio_rw( O_RDONLY, diop, buf, bytes_to_read,
 	    			  lseek_offset, NULL, err_code);
# endif /* OS_THREADS_USED */
    if ( bytes_read != bytes_to_read )
    {
	SETCLERR(err_code, 0, ER_read);

	if (bytes_read == -1)
	{
	    status = DI_BADREAD;
	}
	else
	{
	    status = DI_ENDFILE;
	}
    }

    if ( scb )
    {
	scb->cs_memory &= ~(CS_DIOR_MASK | CS_LIOR_MASK);
	scb->cs_state = saved_state;
	if (f->io_open_flags & DI_O_LOG_FILE_MASK)
	    Cs_srv_block.cs_wtstatistics.cs_lior_time 
		+= CS_checktime() - start_time;
	else
	    Cs_srv_block.cs_wtstatistics.cs_dior_time
		+= CS_checktime() - start_time;
    }

    if ( bytes_read > 0 )
	*n = bytes_read / f->io_bytes_per_page;

    return(status);
}
# endif /* OS_THREADS_USED || xCL_ASYNC_IO */
