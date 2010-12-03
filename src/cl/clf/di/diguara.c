/*
**Copyright (c) 2004 Ingres Corporation
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
#include   <cldio.h>
#include   "dilocal.h"
#include   "dilru.h"

#ifdef      xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif            /* xCL_007_FILE_H_EXISTS */

/* unix includes */
#ifdef            xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */


#include <seek.h>
#include <errno.h>

/**
**
**  Name: DIGUARA.C - This file contains the UNIX routines used to guarantee
**		       disc space.
**		       which guarantee disc space to the client.
**
**  Description: 
**	This file contains the routines used to guarantee that disc
**	space will exist on disc when needed.
**
**          DIguarantee_space() - Guarantee space is on disc.
**
**
**
**  History:
**      30-October-1992 (rmuth)
**          Created.
**	30-nov-1992 (rmuth)
**	    DIlru error checking.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	30-feb-1993 (rmuth)
**	    Use the global DIzero_buffer as opposed to the local 
**	    zero_alloc. Also use DI_ZERO_BUFFER_SIZE as opposed to
**	    DI_FILE_BUF_SIZE as this is used for normal read/write
**	    operations.
**	26-Apr-93 (vijay)
**	    Include file.h before seek.h.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	18-apr-1994 (jnash)
**	    fsync project.  On systems without O_SYNC, DIforce()
**	    must be called at end of guarantee space operation.
**	19-apr-95 (canor01)
**	    added <errno.h>
**      23-jul-1996 (canor01)
**          Semaphore protect the lseek/write combination when used with
**          operating-system threads.
**	24-Nov-1997 (allmi01)
**	    Bypassed defintion of lseek on Silicon Graphics (sgi_us5)
**	    as it already exists in <unistd.h>.
**      05-Nov-1997 (hanch04)
**          Remove declare of lseek, done in seek.h
**          Added LARGEFILE64 for files > 2gig
**          Change i4 to OFFSET_TYPE
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs to dilocal.h
**	22-Oct-1998 (jenjo02)
**	    Repositioned semaphore protection to cover entire
**	    seek/write combination.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-nov-2001 (stial01)
**          Fix lseek offset calculation for LARGEFILE64 (b106470)
**	28-Nov-2005 (kschendel)
**	    Use dynamically sized zeroing buffer.
**	15-Nov-2010 (kschendel) SIR 124685
**	    Delete unused variables.
**/


/*
**  Forward and/or External typedef/struct references.
*/


static STATUS 	DI_guarantee(
			    DI_IO         	*f,
			    DI_OP		*diop,
			    i4	        start_pageno,
			    i4		number_pages,
			    CL_ERR_DESC		*err_code );


/*
**  Forward and/or External function references.
*/



/*{
** Name: DIguarantee_space - Guarantee space is on disc.
**
** Description:
**	This routine will guarantee that once the call has completed
**	the space on disc is guaranteed to exist. The range of pages
**	must have already been reserved by DIalloc or DIgalloc the
**	result of calling this on pages not in this range is undefined.
**	
**   Notes:
**
**   a. On file systems which cannot pre-allocate space to a file this 
**      means that DIguarantee_space will be required to reserve the space 
**      by writing to the file in the specified range. On File systems which 
**      can preallocate space this routine will probably be a nop.
**
**   b. Currently the main use of this routine is in reduction of I/O when 
**      building tables on file systems which cannot preallocate space. See 
**      examples.
**
**   c. DI does not keep track of space in the file which has not yet
**      been guaranteed on disc this must be maintained by the client.
**
**   d. DI does not keep track of what pages have been either guaranteed on
**      disc or DIwritten and as this routine will overwrite whatever data may
**      or maynot already be in the requested palce the client has the ability 
**      to overwrite valid data.
**
**   e. The contents of the pages guaranteed by DIguarantee_space
**      are undefined until a DIwrite() to the page has happened. A DIread
**      of these pages will not fail but the contents of the page returned is
**      undefined.
**
** Inputs:
**      f               Pointer to the DI file context needed to do I/O.
**	start_pageno	Page number to start at.
**	number_pages	Number of pages to guarantee.
**
** Outputs:
**      err_code         Pointer to a variable used to return operating system 
**                       errors.
**    Returns:
**        OK
**        DI_BADEXTEND      	Can't allocate disk space
**        DI_BADFILE        	Bad file context.
**        DI_EXCEED_LIMIT   	Too many open files.
**	  DI_BADLRU_RELEASE	Problem releasing open file.
**
**        If running with slaves  :
**	  DI_GENERAL_ERR	More info in the err_code.intern field.
**	  
**	  If running without slaves :
**	  DI_BADINFO	    	Error finding current end-of-file.
**				
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-October-1992 (rmuth)
**	Created.
**    30-nov-1992 (rmuth)
**	DIlru error checking.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**    30-feb-1993 (rmuth)(
**	Use the global DIzero_buffer as opposed to the local 
**	zero_alloc.
**    18-apr-1994 (jnash)
**	    fsync project.  On systems without O_SYNC, DIforce()
**	    must be called at end of guarantee space operation.
**	06-Jan-2005 (jenjo02)
**	    Fix inproc code to use proper version of write/pwrite/pwrite64
**	    instead of always IIdio_write. View bytes-to-be-written in
**	    page terms rather than bytes, expand DI_ZERO_BUFFER_SIZE.
**	    IIdio_write() would fail when finishing off a load of
**	    very large file.
**	30-Sep-2005 (jenjo02)
**	    Ok to use pwrite if FD_PER_THREAD.
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**
** Design Details:
**
**	UNIX DESIGN:
**
**	Since the only way that UNIX allows one to guarantee that disc
**	space for a file will be available when needed this routine will
**	write zero filled data to the requested range of pages.
**
**	If we are running with slaves we will request the slave process
**	to do the actual I/O otherwise it is done inline.
**
**	For more information see di.h
*/
STATUS
DIguarantee_space(
    DI_IO         	*f,
    i4	        start_pageno,
    i4		number_pages,
    CL_ERR_DESC		*err_code )
{
    STATUS	status = OK;
    DI_OP	diop;

    CL_CLEAR_ERR( err_code );

    /* Check file control block pointer, return if bad. */

    if (f->io_type != DI_IO_ASCII_ID)
	return(DI_BADFILE);

    /* Count a guara */
    f->io_stat.guara++;

    /* get file descriptor for this file */

    do
    {
    	status = DIlru_open(f, FALSE, &diop, err_code);
	if ( status != OK )
	    break;

	status = DI_guarantee( f, &diop, start_pageno, number_pages,
			       err_code );

	if ( status != OK )
	{
	    CL_ERR_DESC	lerr_code;

	    (VOID)DIlru_release(&diop, &lerr_code);
	}
	else
	{
	    status = DIlru_release(&diop, err_code);
	}

    } while (FALSE);

    return( status );
}

static STATUS
DI_guarantee(
    DI_IO         	*f,
    DI_OP		*diop,
    i4	        start_pageno,
    i4		number_pages,
    CL_ERR_DESC		*err_code )
{

    STATUS                      big_status = OK, small_status =OK;
    STATUS                      intern_status = OK;
    register DI_SLAVE_CB        *disl;
    i4				end_of_guaranteed_space;
    OFFSET_TYPE			lseek_ret;
    int				unix_fd;

    do
    {
# ifdef OS_THREADS_USED
	/* Wrap the seek/write in a semaphore */
	if ((f->io_fprop & FPROP_PRIVATE) == 0)
	    CS_synch_lock( &f->io_sem );
# endif /* OS_THREADS_USED */

        if (Di_slave)
        {
	    disl = diop->di_evcb;

	    disl->file_op  = DI_SL_GUARANTEE;
	    disl->pre_seek = (OFFSET_TYPE)start_pageno * (OFFSET_TYPE)f->io_bytes_per_page;
	    disl->length   = number_pages * f->io_bytes_per_page;

	    /* Send file properties to slave */
	    FPROP_COPY(f->io_fprop, disl->io_fprop);

	    DI_slave_send( disl->dest_slave_no, diop,
			   &big_status, &small_status, &intern_status );

	    if (( big_status != OK ) || ( small_status != OK ))
		break;

	    if ( disl->status != OK )
	    {
		STRUCT_ASSIGN_MACRO(disl->errcode, *err_code);
		small_status = DI_BADEXTEND;
		break;
	    }

    	}
    	else
    	{
	    /* 
	    ** Running without slaves 
	    */
	    i4  bytes_written;
	    i4	buf_size;

	    OFFSET_TYPE lseek_offset = 
		(OFFSET_TYPE)start_pageno * (OFFSET_TYPE)f->io_bytes_per_page;

	    unix_fd = diop->di_fd;

	    /* 
	    ** Move to position in file where we want to start guaranteeing
	    ** the space. We are passed in an Ingres page number which is
	    ** one more than the physical page number on disc
	    */
#ifdef LARGEFILE64
	    if ((lseek_ret = lseek64( unix_fd, (OFFSET_TYPE) lseek_offset, 
					L_SET)) == (OFFSET_TYPE) -1L)
#else /* LARGEFILE64 */
	    if ((lseek_ret = lseek( unix_fd, (OFFSET_TYPE) lseek_offset, 
					L_SET)) == (OFFSET_TYPE) -1L)
#endif  /* LARGEFILE64 */
	    {
	    	SETCLERR(err_code, 0, ER_lseek);
	    	small_status = DI_BADINFO;
		break;
	    }
	    else
	    {
		/*
		** Make sure we are positioned in the correct place
		*/
		if ( lseek_ret != lseek_offset )
		{
		    SETCLERR(err_code, 0, ER_lseek);
		    small_status = DI_BADEXTEND;
		    break;
		}
		else
		{
		    i4	pages_remaining = number_pages;
		    i4	pages_at_a_time = Di_zero_bufsize /
					  f->io_bytes_per_page;

		    while ( pages_remaining > 0 )
		    {
			if ( pages_remaining < pages_at_a_time )
			    buf_size = pages_remaining *
					f->io_bytes_per_page;
			else
			    buf_size = Di_zero_bufsize;

# if  defined(OS_THREADS_USED) && !defined(xCL_NO_ATOMIC_READ_WRITE_IO)
			bytes_written =
#ifdef LARGEFILE64
			    pwrite64( unix_fd, Di_zero_buffer,
					buf_size, lseek_offset );
#else /*  LARGEFILE64 */
			    pwrite( unix_fd, Di_zero_buffer,
					buf_size, lseek_offset );
#endif /* LARGEFILE64 */
# else /* OS_THREADS_USED  !xCL_NO_ATOMIC_READ_WRITE_IO */
			bytes_written =
			    IIdio_write( unix_fd, Di_zero_buffer,
					buf_size, 
					lseek_offset, 
					&lseek_offset, 
					f->io_fprop,
					err_code );
# endif /* OS_THREADS_USED */

			if ( bytes_written != buf_size )
			{
			    small_status = DI_BADEXTEND;
			    break;
			}

			lseek_offset += buf_size;
			pages_remaining -= pages_at_a_time;
		    }

		    if ( small_status != OK )
			break;
		}
	    }
	}
    } while (FALSE);

    if (big_status == OK && small_status == OK)
    {
	end_of_guaranteed_space = start_pageno + number_pages - 1;
	/*
	** The following should be a no-op as we should have either
	** called DIalloc or DIgalloc to update this value before we 
	** called this routine
	*/
	f->io_alloc_eof = max( f->io_alloc_eof, end_of_guaranteed_space);
    }

# ifdef OS_THREADS_USED
    if ((f->io_fprop & FPROP_PRIVATE) == 0)
	CS_synch_unlock( &f->io_sem );
#endif

#if defined(xCL_010_FSYNC_EXISTS) && !defined(O_SYNC)
    if (big_status == OK && small_status == OK)
    {
	/*
	** On systems that have no O_SYNC but that do have fsync, 
	** file synchronization is required at this point.
	*/
	small_status = DIforce(f, err_code);
    }
#endif

    if ( big_status != OK)
	small_status = big_status;

    if (small_status != OK)
	DIlru_set_di_error( &small_status, err_code, intern_status,
			    DI_GENERAL_ERR);

    return(small_status);
}

