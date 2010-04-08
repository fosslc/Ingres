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
#ifdef            xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */

#ifdef      xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif            /* xCL_007_FILE_H_EXISTS */

#include    <errno.h>

/**
**
**  Name: DIGALLOC.C - This file contains the UNIX allocation routines
**		       which guarantee disc space to the client.
**
**  Description: 
**	This file contains the routines used to allocate disc space to
**	a file, this space is guaranteed to exist on disc once this
**	call has completed.
**
**          DIgalloc() - Allocate pages to a file.
**
**
**  History:
**      30-October-1992 (rmuth)
**          Created.
**	30-nov-1992 (rmuth)
**	    Various
**	     - include <cldio.h>
**	     - Call IIdio_get_file_eof instead of lseek as this can
**	       deal with both raw and ordinary files. Change some
**	       types accordingly.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	30-feb-1993 (rmuth)
**	    Use the global DIzero_buffer as opposed to the local
**	    zero_alloc. Also use DI_ZERO_BUFFER_SIZE as opposed
**	    to DI_FILE_BUF_SIZE which is also used during normal
**	    read and write operation.
**	    
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	19-apr-95 (canor01)
**	    added <errno.h>
**      03-Nov-1997 (hanch04)
**	    Change i4 to OFFSET_TYPE for > 2gig files
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Removed GLOBALREFs, changed Di_galloc_count to static.
**	22-Oct-1998 (jenjo02)
**	    Repositioned semaphore protection to cover seek/write
**	    combination.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Nov-2005 (kschendel)
**	    Make zeroing buffer a configurable size.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Use fallocate reservations before zeroing, if possible.
**	    Make io-sem a SYNCH.
**/


/*
**  Forward and/or External typedef/struct references.
*/

/*
**  Defines of other constants.
*/

/*
**  Forward and/or External function references.
*/
static STATUS 	DI_galloc(
			    DI_IO	*f,
			    i4	n,
			    DI_OP	*diop,
			    i4		*end_of_file,
			    CL_ERR_DESC *err_code);


/* typedefs go here */


/*
** Definition of all global variables owned by this file.
*/

/* count of DIgalloc() for debugging */
static i4             Di_galloc_count ZERO_FILL; 


/*
**  Definition of static variables and forward static functions.
*/


/*{
** Name: DIgalloc - Allocates N pages to a direct access file.
**
** Description:
**	The DIgalloc routine is used to add pages to a direct access
**	file, the disc space for these pages is guaranteed to exist
**	once the routine returns. The contents of the pages allocated
**	are undefined until a DIwrite to the page has happened.
**
**      This routine can add more than one page at a time by accepting 
**	a count of the number of pages to add.
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
**    30-nov-1992
**	    Call IIdio_get_file_eof instead of lseek as this can
**	    deal with both raw and ordinary files. Change some
**	    types accordingly.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	30-feb-1993 (rmuth)
**	    Use the global DIzero_buffer as opposed to the local
**	    zero_alloc.
**	23-jul-1996 (canor01)
**	    Semaphore protect the lseek/write combination when used with
**	    operating-system threads.
**	06-Jan-2005 (jenjo02)
**	    Fix inproc code to use proper version of write/pwrite/pwrite64
**	    instead of always IIdio_write. View bytes-to-be-written in
**	    page terms rather than bytes, expand DI_ZERO_BUFFER_SIZE.
**	30-Sep-2005 (jenjo02)
**	    Ok to use pwrite if FD_PER_THREAD.
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
**
** Design Details:
**
**	UNIX DESIGN:
**
**	Since the only way that UNIX allows one to allocate space to a
**	file is to write to the file. This routine will write zero
**	filled data to file for the required number of pages.
**
**	If we are running with slaves we will request the slave process
**	to do the actual I/O otherwise it is done inline.
**
**	For more information on DI see di.h
*/
STATUS
DIgalloc(
    DI_IO         	*f,
    i4	        n,
    i4	        *page,
    CL_ERR_DESC		*err_code )
{
    i4		end_of_file;
    STATUS	status = OK;
    DI_OP	diop;

    CL_CLEAR_ERR( err_code );

    /* Check file control block pointer, return if bad. */

    if (f->io_type != DI_IO_ASCII_ID)
	return(DI_BADFILE);

    /* Count a galloc */
    f->io_stat.galloc++;

    do
    {
        /* 
	** get file descriptor for this file 
	*/
        status = DIlru_open(f, FALSE, &diop, err_code);
	if ( status != OK )
	    break;

	status = DI_galloc( f, n,&diop, &end_of_file, err_code );

	if ( status != OK )
	{
	    CL_ERR_DESC lerr_code;

	    (VOID) DIlru_release(&diop, &lerr_code);
	}
	else
	{
	    status = DIlru_release(&diop, err_code);
	}

	/*
	** If we failed to extend the table then do not update 
	** the following
	*/
	if (status == OK )
	{
	    /*
	    ** Return page number of first page allocated
	    */
	    *page = end_of_file + 1;

#ifdef xDEV_TST
	    TRdisplay(
		    "DIgalloc: file: %t/%t, alloc_eof: %d, first: %d, count: %d\n",
		      f->io_l_pathname, f->io_pathname,
		      f->io_l_filename, f->io_filename,
		      f->io_alloc_eof, *page, n);
#endif
       
	}

    } while (FALSE);


    return( status );
}

static STATUS
DI_galloc(
    DI_IO	*f,
    i4	n,
    DI_OP	*diop,
    i4		*end_of_file,
    CL_ERR_DESC *err_code)
{
    STATUS                      big_status = OK, small_status =OK;
    STATUS                      intern_status = OK;
    register DI_SLAVE_CB        *disl;
    i4				last_page;
    OFFSET_TYPE			lseek_ret;

    do
    {
# ifdef OS_THREADS_USED
	/* Seek/write must be semaphore protected */
	if ((f->io_fprop & FPROP_PRIVATE) == 0)
	    CS_synch_lock( &f->io_sem );
# endif /* OS_THREADS_USED */

        if (Di_slave)
        {
	    disl = diop->di_evcb;

	    disl->file_op = DI_SL_ZALLOC;
	    disl->length = n * f->io_bytes_per_page;
	    /* Pass file properties to slave */
	    FPROP_COPY(f->io_fprop,disl->io_fprop);

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
	     else
	     {
	        lseek_ret = disl->length;
	     }

	}
    	else
    	{
	    /* 
	    ** Running without slaves 
	    */
	    OFFSET_TYPE	lseek_offset;
	    i8		reservation;
	    i4		buf_size;
	    i4		bytes_written;
	    i4		pages_remaining = n;
	    i4		pages_at_a_time = Di_zero_bufsize /
					  f->io_bytes_per_page;

	    /* find current end-of-file */

	    lseek_ret = IIdio_get_file_eof(diop->di_fd, f->io_fprop);
	    if ( lseek_ret == (OFFSET_TYPE)-1L )
	    {
	    	SETCLERR(err_code, 0, ER_lseek);
	    	small_status = DI_BADINFO;
		break;
	    }
	    else
	    {
		lseek_offset = lseek_ret;
		/* If this filesystem can do reservations, see if we
		** should reserve more space.
		** Even though we have to write the zeros anyway, the
		** reservation may well be larger than the zeroing
		** buffer, and this way helps maintain contiguity.
		** Not worth it for tiny writes.
		*/
		if (pages_remaining > 2
		  && FPROP_ALLOCSTRATEGY_GET(f->io_fprop) == FPROP_ALLOCSTRATEGY_RESV)
		{
		    reservation = lseek_offset + (pages_remaining * f->io_bytes_per_page);
		    if (reservation > f->io_reserved_bytes)
		    {
			/* Re-check in case some other server reserved */
			small_status = IIdio_get_reserved(diop->di_fd,
				&f->io_reserved_bytes, err_code);
			if (small_status == OK && reservation > f->io_reserved_bytes)
			{
			    small_status = IIdio_reserve(diop->di_fd,
					f->io_reserved_bytes,
					reservation - f->io_reserved_bytes,
					err_code);
			    if (small_status == OK)
			    {
				f->io_reserved_bytes = reservation;
			    }
			    else
			    {
				if (small_status != DI_BADFILE)
				    break;
				/* Fallocate not supported, turn off
				** "reserve" strategy, continue without.
				*/
				small_status = OK;
				FPROP_ALLOCSTRATEGY_SET(f->io_fprop, FPROP_ALLOCSTRATEGY_VIRT);
			    }
			}
		    }
		} /* end reservations */

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
			pwrite64( diop->di_fd, Di_zero_buffer, 
				    buf_size, lseek_offset );
#else /*  LARGEFILE64 */
			pwrite( diop->di_fd, Di_zero_buffer, 
				    buf_size, lseek_offset );
#endif /* LARGEFILE64 */
# else /* OS_THREADS_USED  !xCL_NO_ATOMIC_READ_WRITE_IO */
		    bytes_written =
			IIdio_write( diop->di_fd, Di_zero_buffer, 
				    buf_size, 
				    lseek_offset, 
				    &lseek_offset, 
				    f->io_fprop,
				    err_code );
# endif /* OS_THREADS_USED */

		    if ( bytes_written != buf_size )
		    {
			SETCLERR(err_code, 0, ER_write);
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

	*end_of_file = ( lseek_ret / f->io_bytes_per_page) - 1;

    } while (FALSE);

    if (big_status == OK && small_status == OK)
    {
	/*
	** Update the current allocated end-of-file under mutex protection
	*/
	last_page = *end_of_file + n;
	if (last_page > f->io_alloc_eof)
	    f->io_alloc_eof = last_page;
    }

# ifdef OS_THREADS_USED
    if ((f->io_fprop & FPROP_PRIVATE) == 0)
	CS_synch_unlock( &f->io_sem );
# endif /* OS_THREADS_USED */

    if ( big_status != OK )
	small_status = big_status;

    if ( small_status != OK )
	DIlru_set_di_error( &small_status, err_code, intern_status,
			    DI_GENERAL_ERR);

    return(small_status);

}
