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
#include   <cldio.h>
#include   <dislave.h>
#include   "dilocal.h"
#include   "dilru.h"
#include   <errno.h>

/**
**
**  Name: DICREATE.C - This file contains the UNIX DIcreate() routine.
**
**  Description:
**      The DICREATE.C file contains the DIcreate() routine.
**
**        DIcreate - Creates a new file.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**	31-aug-88 (mmm)
**	    Fixed problem with checking return value from DIlru_open().
**	    CSfree_event() only should be called when DIlru_open() has been
**	    successful and we are running with slave processes.
**	30-mar-88 (anton)
**          di in slave
**	05-aug-88 (roger)
**	    UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
**	17-oct-88 (jeff)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	05-jan-89 (jeff)
**	    added traceing and changed to return with file closed (bug fix)
**	24-jan-89 (roger)
**	    Make use of new CL error descriptor.
**	06-feb-89 (mikem)
**	    better support for DI CL_ERR_DESC, including initializing to zero,
**	    errors passed back from DIlru_open(), and only pass errors back 
**	    from DIlru_close() if open didn't fail.
**	13-feb-89 (jeff)
**	    Added new flags to DIlru_open.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <csev.h>.
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	30-nov-1992 (rmuth)
**	    Prototype and remove forward reference to DIlru_uniq.
**	    Add error code to DIlru_uniq.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      26-jul-1993 (mikem)
**          Include systypes.h now that csev.h exports prototypes that include
**          the fd_set structure.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	19-apr-95 (canor01)
**	    added <errno.h>
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs to dilocal.h
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

/* externs */

/* statics */


/*{
** Name: DIcreate - Creates a new file.
**
** Description:
**      The DIcreate routine is used to create a direct access file.  
**      Space does not need to be allocated at time of creation. 
**      If it is more efficient for a host operating system to
**      allocate space at creation time, then space can be allocated.
**      The size of the page for the file is fixed at create time
**      and cannot be changed at open.
**
**	File should not be left open.
**      
**   
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      path                 Pointer to an area containing path name.
**      pathlength           Length of pathname.
**      filename             Pointer to an area containing file name.
**      filelength           Length of file name.
**      pagesize             Value indicating the size of the page
**                           in bytes.
**      
** Outputs:
**      f                    Updates the file control block.
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADDIR        Error in path specification.
**          DI_BADCREATE     Error creating file.
**          DI_BADFILE       Bad file context.
**          DI_BADPARAM      Parameter(s) in error.
**          DI_EXCEED_LIMIT  Too many open files.
**          DI_EXISTS        File already exists.
**          DI_DIRNOTFOUND   Path not found.
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
**	    better support for DI CL_ERR_DESC, including initializing to zero,
**	    errors passed back from DIlru_open(), and only pass errors back 
**	    from DIlru_close() if open didn't fail.
**    30-nov-1992 (rmuth)
**	    Prototype and add DIlru error checking.
**	22-Dec-1998 (jenjo02)
**	    Initialize (new) io_fd_q.
**	26-Jul-2006 (schka24)
**	    Need mutex for above q initialized so lru doesn't barf.
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
*/
STATUS
DIcreate(
    DI_IO          *f,
    char           *path,
    u_i4          pathlength,
    char           *filename,
    u_i4          filelength,
    i4             pagesize,
    CL_ERR_DESC    *err_code )
{
    STATUS	ret_val;
    DI_OP	diop;

    /* default return */
    ret_val = OK;
    CL_CLEAR_ERR( err_code );

    /*	Check for some bad parameters. */

    if (pathlength > DI_PATH_MAX	|| 
	pathlength == 0			|| 
	filelength > DI_FILENAME_MAX	|| 
	filelength == 0)
    {
	return (DI_BADPARAM);		
    }

    /* Initialize file control block. */

    /* (canor01) structure members appear to be individually initialized */
    /* MEfill((u_i2) sizeof(DI_IO), (u_char) '\0', (PTR) f); */

    /* Zero statistics in DI_IO structure */
    MEfill(sizeof(f->io_stat), 0, &f->io_stat);

    /* save path and filename in DI_IO struct to cache open file descriptors. */

    MEcopy((PTR) path, pathlength, (PTR) f->io_pathname);
    f->io_l_pathname = pathlength;
    MEcopy((PTR) filename, filelength, (PTR) f->io_filename);
    f->io_l_filename = filelength;

#ifdef	xDEV_TST
    TRdisplay("DIcreate: %t/%t\n", f->io_l_pathname, f->io_pathname,
	      f->io_l_filename, f->io_filename);
#endif

    f->io_bytes_per_page = pagesize;
    f->io_open_flags = 0;
    f->io_stat.create++;
    /* Init file properties */
    FPROP_CLEAR(f->io_fprop);

    f->io_fd_q.q_next = f->io_fd_q.q_prev = (QUEUE*)&f->io_fd_q;
#ifdef OS_THREADS_USED
    if ( Di_thread_affinity )
	CS_synch_init(&f->io_fd_q_sem);
#endif

    do
    {
	/* 
	** assign a unique id to this file (number is unique to the current 
	** server). This unique id is used in subsequent DI calls to 
	** identify the file in question without having to do comparisons on 
	** full path and file name.
	*/
	ret_val = DIlru_uniq(f, err_code);
	if (ret_val != OK )
	    break;

	/* 
	** open this file, closing another DI file if necessary (ie. if we 
	** have run out of open file descriptors.)
	** NOTE: We used to pass in the DI_NORETRY flag anyway have taken
	** this out.
	*/
	ret_val = DIlru_open(f, DI_FCREATE, &diop, err_code);
	if ( ret_val != OK )
	    break;

	ret_val = DIlru_release(&diop, err_code);
	if ( ret_val != OK )
	    break;

	ret_val = DIlru_close(f, err_code);
	if ( ret_val != OK )
	    break;

    } while (FALSE);

    /* Clear file properties in case DI_IO reused */
    FPROP_CLEAR(f->io_fprop);

#ifdef OS_THREADS_USED
    if ( Di_thread_affinity )
	CS_synch_destroy(&f->io_fd_q_sem);
#endif

    return(ret_val);
}
