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

#include   <errno.h>
#include   <systypes.h>

#ifdef	    xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */

#ifdef      xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif	    /* xCL_007_FILE_H_EXISTS */
/**
**
**  Name: DISENSE.C - This file contains the UNIX DIsense() routine.
**
**  Description:
**      The DISENSE.C file contains the DIsense() routine.
**
**        DIsense - Verifies a page exists in file.
**
** History:
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	05-nov-87 (mikem)
**          figure end of file correctly.
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
**	    Include <systypes.h> and <seek.h>
**	23-mar-89 (mikem)
**	    update io_system_eof when necessary (bug 4854).
**	01-Oct-90 (anton)
**	    Reentrant CL changes
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	30-nov-1992 (rmuth)
**	    Various changes :
**	       - Prototype 
**	       - include "dilru.h" and <cldio.h>
**	       - Call IIdio_get_file_eof instead of lseek as this can
**		 deal with both raw and ordinary files. Change some
**		 types accordingly.
**	       - DIlru error checking.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jul-1993 (mikem)
**	    Include systypes.h now that csev.h exports prototypes that include
**	    the fd_set structure.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	19-apr-95 (canor01)
**	    added <errno.h>
**	11-Nov-1997 (jenjo02)
**	    Backed out change 432516 from diread, disense, diwrite.
**	    io_sem needs to be held during i/o operations to avoid 
**	    concurrency problems.
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs to dilocal.h
**	22-Oct-1998 (jenjo02)
**	    With OS_THREADS, io_sem must be held while the seek
**	    is taking place to avoid corrupting the seek address
**	    in the FD for another thread.
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
** Name: DIsense - Determines end of file.
**
** Description:
**      The DIsense routine is used to find the end of a direct
**      access file.  This routine will always determine the last page
**      used (set by DIflush) by accessing the information from the 
**      file header on disk.  The last page used may be less than
**      the actual physical allocation.
**      (See the file header for explanation of file header contents.)
**      If the file is empty the DIsense call returns -1.  The
**      file must be open to issue a DIsense call.
**
**      The end of file and allocated are not updated until a DIflush
**      call is issued.  This insures that pages are not considered valid 
**      until after they are formatted.
**
**      The DIsense is not used for temporary files.  Their
**      end of file is maintained in the DMF Table Control Block (TCB)
**      structure.  This does not create a problem since temporary
**      files are exclusively visible to only one server and one
**      thread within a server.
**   
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
** Outputs:
**      page                 Pointer to a variable used
**                           to return the page number of the
**                           last page used.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**        OK
**        DI_BADINFO         Error occured.
**        DI_BADFILE         Bad file context.
**        DI_BADPARAM        Parameter(s) in error.
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
**    23-Jan-90 (anton)
**	    Broke out working code into DI_sense which is also
**	    used by DIwrite.  This prevents two CSevcb's from being
**	    reserved durring DIwrite.
**	30-nov-1992 (rmuth)
**	    Various changes :
**	       - Prototype 
**	       - Call IIdio_get_file_eof instead of lseek as this can
**		 deal with both raw and ordinary files. Change some
**		 types accordingly.
**	       - DIlru error checking.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	03-jun-1996 (canor01)
**	    When using operating-system threads, protect the DI_IO before
**	    calling lseek and updating structure.
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
*/
STATUS
DIsense(
    DI_IO          *f,
    i4        *page,
    CL_ERR_DESC     *err_code)
{
    STATUS		ret_val;
    DI_OP		diop;

    /* default returns */

    ret_val = OK;
    CL_CLEAR_ERR( err_code );

    diop.di_flags = 0;

    /* 
    ** check file control block pointer, return if bad. 
    */
    if (f->io_type != DI_IO_ASCII_ID)
        return( DI_BADFILE );

    do
    {
	/* 
	** get open file descriptor for the file 
	*/
	ret_val = DIlru_open(f, FALSE, &diop, err_code);
	if ( ret_val != OK )
	    break;

	ret_val = DI_sense(f, &diop, page, err_code);
	if ( ret_val != OK )
	{
	    CL_ERR_DESC	lerr_code;

	    (VOID) DIlru_release(&diop, &lerr_code);
	}
	else
	{
            ret_val = DIlru_release(&diop, err_code);
	}

    } while (FALSE);

    return(ret_val);
}

STATUS
DI_sense(
    DI_IO	*f,
    DI_OP	*diop,
    i4	*end_of_file,
    CL_ERR_DESC	*err_code)
{
    STATUS			big_status = OK, small_status =OK;
    STATUS			intern_status = OK;
    register DI_SLAVE_CB	*disl;
    OFFSET_TYPE			lseek_ret;

    /* Count another sense */
    f->io_stat.sense++;
    
    do
    {
# ifdef OS_THREADS_USED
	if ((f->io_fprop & FPROP_PRIVATE) == 0)
	    CS_synch_lock( &f->io_sem );
# endif /* OS_THREADS_USED */
	
	if (Di_slave)
	{
	    disl = diop->di_evcb;
	    disl->file_op = DI_SL_SENSE;

	    /* Send file properties to slave */
	    FPROP_COPY(f->io_fprop,disl->io_fprop);

	    DI_slave_send( disl->dest_slave_no, diop,
			   &big_status, &small_status, &intern_status );
	    if (big_status == OK && small_status == OK)
	    {
		if ((small_status = disl->status) != OK )
		    STRUCT_ASSIGN_MACRO(disl->errcode, *err_code);
		else
		    lseek_ret = disl->length;
	    }
	}
	else
	{
	    /* The system call is needed when attempting to find
	    ** out how big a file that may be affected by more than
	    ** one process at a time.  For instance if two different
	    ** processes add a page to the file the system call is
	    ** the only way to tell that the file is now 2 pages
	    ** bigger rather than just one.
	    */
	    lseek_ret = IIdio_get_file_eof(diop->di_fd, f->io_fprop);
	    
	    if (lseek_ret == -1L)
	    {
		/* lseek failed */
		SETCLERR(err_code, 0, ER_lseek);
		small_status = DI_BADINFO;
	    }
	}

	if (big_status == OK && small_status == OK)
	{
	    *end_of_file = ( lseek_ret / f->io_bytes_per_page) - 1;

	    f->io_alloc_eof = max( f->io_alloc_eof, *end_of_file );
	}
# ifdef OS_THREADS_USED
	if ((f->io_fprop & FPROP_PRIVATE) == 0)
	    CS_synch_unlock( &f->io_sem );
# endif /* OS_THREADS_USED */

    } while (FALSE);

    if (big_status != OK )
	small_status = big_status;

    if ( small_status != OK)
	DIlru_set_di_error( &small_status, err_code, intern_status,
			    DI_GENERAL_ERR);
    
    return(small_status);
}
