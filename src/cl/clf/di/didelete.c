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
#include   <me.h>
#include   <cldio.h>
#include   <dislave.h>
#include   "dilocal.h"
#include   "dilru.h"
#include   <errno.h>

#ifdef xCL_006_FCNTL_H_EXISTS
#include   <fcntl.h>
#endif /* xCL_006_FCNTL_H_EXISTS */
#include   <sys/stat.h>

/**
**
**  Name: DIDELETE.C - This file contains the UNIX DIdelete() routine.
**
**  Description:
**      The DIDELETE.C file contains the DIdelete() routine.
**
**        DIdelete -  Deletes a file.
**
** History:
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      30-aug-88 (anton)
**          di in slave
**	29-aug-88 (jeff)
**	    fix bug in measurement of slave activity
**	17-oct-88 (jeff)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	05-jan-89 (jeff)
**	    added traceing
**	24-jan-89 (roger)
**	    Make use of new CL error descriptor.
**	06-feb-89 (mikem)
**	    Initialize the CL_ERR_DESC to zero.
**	    Also added "#include <me.h>".
**      31-mar-89 (mikem)
**	    bug #5282:
**	    Error handling in this routine was changed 1/24/89 to make
**	    use of the new CL_ERR_DESC.  Unfortunately at that time the
**	    code which maps unix errors to the proper DI_* error was 
**	    moved to only handle the non-slave di case.  Any error returned
**	    from a delete (most notably the expected error DI_FILENOTFOUND)
**	    would be returned as whatever the slave set it to (FAIL).  In
**	    turn any mainline code explicitly expecting a DI_FILENOTFOUND
**	    error would fail in unpredictable ways.  The code has been changed
**	    back to handle error mapping for both cases.
**	2-Feb-90 (anton)
**	    Don't always copy CL_ERR_DESC
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <csev.h>.
**	01-Oct-90 (anton)
**	    Reentrant CL changes
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	30-nov-1992 (rmuth)
**	    Prototype and add error checking.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      26-jul-1993 (mikem)
**          Include systypes.h now that csev.h exports prototypes that include
**          the fd_set structure.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	19-apr-1995 (canor01)
**		change errno to errnum
**	13-nov-1995 (schte01)
**		Removed register notation on disl variable for axp_osf due to
**		register corruption.
**	24-jan-1996 (nick)
**	    Truncate the file we are deleting ; this obviates the problem
**	    of another process holding this file open and preventing immediate
**	    release of the disk space used. #47513
**      03-Nov-1997 (hanch04)
**          Added LARGEFILE64 for files > 2gig
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs to dilocal.h
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  01-Apr-2004 (fanch01)
**      Add O_DIRECT support on Linux depending on the filesystem
**      properties, pagesize.  Fixups for misaligned buffers on read()
**      and write() operations.  Added page size parameter to IIdio_open().
**      22-Apr-2005 (hanje04)
**	    Remove define of lseek() and unlink(), system functions shouldn't 
**	    be protyped in Ingres code.
**/

/* # defines */

/* typedefs */

/* forward references */

/* externs */

/* statics */


/*{
** Name: DIdelete - Deletes a file.
**
** Description:
**      The DIdelete routine is used to delete a direct access file.
**      The file does not have to be open to delete it.  If the file is
**      open it will be deleted when the last user perfroms a DIclose.
**
**	DIdelete truncates the file before deleting it.  This is stupid,
**	but potentially necessary in a multi-server environment;  one
**	DBMS server might have the file open because of a cached TCB,
**	while another server might delete the file.  The server holding
**	the stale FD open may not notice the deletion for a long time,
**	and in the meantime it's holding the disk space.  By truncating
**	the file first, we return the disk space.
**
**	Callers can avoid this behavior by passing NULL as the DI_IO
**	pointer.  Looking at DI_PRIVATE might be even better, except
**	that the DI_IO input is bogus for DI file create/delete ops,
**	and there is no reason to think that the caller DI_IO has anything
**	meaningful inside it.
**
** Inputs:
**      f                    Pointer to DI IO control block, or NULL if
**			     the file can be just deleted without truncating.
**      path                 Pointer to the area containing 
**                           the path name for this file.
**      pathlength           Length of path name.
**      filename             Pointer to the area containing
**                           the file name.
**      filelength           Length of file name.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**        OK
**        DI_BADFILE         Bad file context.
**        DI_BADOPEN         Error openning file.
**        DI_DIRNOTFOUND     Path not found.
**        DI_BADDELETE       Error deleting file.
**        DI_BADPARAM        Parameter(s) in error.
**	  DI_EXCEED_LIMIT    Open file limit exceeded.
**        DI_FILENOTFOUND    File not found.
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
**	    Initialize the CL_ERR_DESC to zero.
**    31-mar-89 (mikem)
**	    bug #5282:
**	    Error handling in this routine was changed 1/24/89 to make
**	    use of the new CL_ERR_DESC.  Unfortunately at that time the
**	    code which maps unix errors to the proper DI_* error was 
**	    moved to only handle the non-slave di case.  Any error returned
**	    from a delete (most notably the expected error DI_FILENOTFOUND)
**	    would be returned as whatever the slave set it to (FAIL).  In
**	    turn any mainline code explicitly expecting a DI_FILENOTFOUND
**	    error would fail in unpredictable ways.  The code has been changed
**	    back to handle error mapping for both cases.
**	2-Feb-90 (anton)
**	    Don't always copy CL_ERR_DESC
**	30-nov-1992 (rmuth)
**	    Prototype and add error checking.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	24-jan-1996 (nick)
**	    #47513. Truncate the file before deleting it.
**	18-Jul-2005 (schka24)
**	    Pass above open a constant instead of possibly uninitialized
**	    junk from the DI_IO.  I really hate this open/truncate business,
**	    as it slows down file deletes, but it will have to stay until
**	    the upper levels can do better about closing TCB's when other
**	    servers are deleting files.
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
**	7-Mar-2007 (kschendel) SIR 122512
**	    Signal don't truncate first, with NULL file ptr input.
**	    (A clumsy way to signal it, but expedient.)
*/
STATUS
DIdelete(
    DI_IO          *f,
    char           *path,
    u_i4          pathlength,
    char           *filename,
    u_i4          filelength,
    CL_ERR_DESC     *err_code )
{
    STATUS	small_status = OK, big_status = OK, intern_status = OK;
    CSEV_CB	*evcb;
    DI_OP	di_op;
#ifdef axp_osf
    DI_SLAVE_CB *disl;
#else
    register    DI_SLAVE_CB *disl;
#endif /* axp_osf */

    /* unix variables */
    char	pathbuf[DI_FULL_PATH_MAX];

    small_status = OK;
    CL_CLEAR_ERR( err_code );

    /* Check input parameters. */

    if (pathlength > DI_PATH_MAX	|| 
	pathlength == 0			|| 
	filelength > DI_FILENAME_MAX	|| 
	filelength == 0)
    {
	return(DI_BADPARAM);
    }

    if (Di_slave)
    {
	bool	reserve_ev_flag = FALSE;

	do
	{
	    /* 
	    ** get null terminated path and filename from input parameters 
	    */
	    big_status = CSreserve_event(Di_slave, &evcb);

	    if ( big_status != OK )
	    {
		intern_status = DI_LRU_CSRESERVE_ERR;
		break;
	    }
	    else
		reserve_ev_flag = TRUE;

	    /* need to manufacture a di_op for the slave call */
	    di_op.di_flags = 0;
	    di_op.di_csevcb = evcb;
	    di_op.di_evcb = disl = (DI_SLAVE_CB *)evcb->data;

	    MEcopy((PTR) path, pathlength, (PTR) disl->buf);
	    disl->buf[pathlength] = '/';

	    MEcopy((PTR) filename, filelength,
		   (PTR) &disl->buf[pathlength + 1]);
	    disl->buf[pathlength + filelength + 1] = '\0';

#ifdef	xDEV_TST
	    TRdisplay("DIdelete: %s\n", disl->buf);
#endif
	    
	    disl->file_op = DI_SL_DELETE;
	    /* 
	    ** we should send a delete to whatever slave is the least busy 
	    ** but we also send it to slave 0 
	    */
	    disl->dest_slave_no = 0;
	    DI_slave_send( disl->dest_slave_no, &di_op,
			   &big_status, &small_status, &intern_status );
	    if ( big_status != OK )
		break;

	    reserve_ev_flag = FALSE;
	    big_status = CSfree_event(evcb);
	    if ( big_status != OK )
	    {
		intern_status = DI_LRU_CSFREE_EV_ERR;
		break;
	    }


	    if (small_status == OK )
	    {
		if ((small_status = disl->status ) != OK )
		{
		    STRUCT_ASSIGN_MACRO(disl->errcode, *err_code);
		}
	    }

	} while (FALSE);

	/*
	** If true an error occurred so just free the thing
	*/
	if ( reserve_ev_flag )
	    (VOID) CSfree_event(evcb);

	/*
	** If a do not bother to continue error then overwrite 
	*/
	if ( big_status != OK )
	    small_status = big_status;

	
    }
    else
    {
#ifdef LARGEFILE64
	struct stat64	fd_stat;
#else /* LARGEFILE64 */
	struct stat	fd_stat;
#endif /* LARGEFILE64 */

	MEcopy((PTR) path, pathlength, (PTR) pathbuf);
	pathbuf[pathlength] = '/';
	MEcopy((PTR) filename, filelength, 
		(PTR) &pathbuf[pathlength + 1]);
	pathbuf[pathlength + filelength + 1] = '\0';

	if (f != NULL)
	{
	    /* Need to truncate before deleting */
	    int		unix_fd;
	    i4		dummy_fprop = FPROP_OPEN;
	    CL_ERR_DESC	local_err;

	    /* open to truncate if we can then close */
	    /* Pass any old bogus bytes-per-page, we aren't going to do
	    ** any real I/O anyway.
	    ** Pass "been opened" fprop because we don't need to figure
	    ** out filesystem properties, file is going away.
	    */
	    if ((unix_fd = IIdio_open(pathbuf, (O_RDWR | O_TRUNC), 
		    DI_FILE_MODE, 2048, 
		    &dummy_fprop,
		    &local_err)) != -1)
	    {
		(void)close(unix_fd);
	    }
	}
	if (unlink(pathbuf) == -1)
	{
	    SETCLERR(err_code, 0, ER_unlink);
	    small_status = DI_BADDELETE;
	}
    }

    /*
    ** If an error occured see if it is a known error if so map
    ** accordingly
    */
    if (small_status != OK )
    {
	if ((err_code->errnum == ENOTDIR) || (err_code->errnum == EACCES))
	{
	    small_status = DI_DIRNOTFOUND;
	}
	else if (err_code->errnum == ENOENT)
	{
	    small_status = DI_FILENOTFOUND;
	}

	DIlru_set_di_error( &small_status, err_code, intern_status,
			    DI_GENERAL_ERR);
    }

    return(small_status);
}
