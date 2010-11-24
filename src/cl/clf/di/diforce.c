/*
**Copyright (c) 2004 Ingres Corporation
*/

#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <systypes.h>
#include   <errno.h>
#include   <er.h>
#include   <cs.h>
#include   <fdset.h>
#include   <csev.h>
#include   <di.h>
#include   <cldio.h>
#include   <dislave.h>
#include   "dilocal.h"
#include   "dilru.h"

#ifdef            xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */

#ifdef xCL_ASYNC_IO
#include   <diasync.h>
#endif /* xCL_ASYNC_IO */

/**
**
**  Name: DIFORCE.C - This file contains the UNIX DIforce() routine.
**
**  Description:
**      The DIFORCE.C file contains the DIforce() routine.
**
**        DIforce - Forces pages to disk.
**
** History:
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      29-mar-88 (anton)
**          DI in slave proces using CS events
**      17-oct-88 (anton)
** 	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	21-jan-89 (mikem)
**	    DI_SYNC_MASK support (O_SYNC, fsync()).
**	06-feb-89 (mikem)
**	    Added better support for DI CL_ERR_DESC, including initializing to 
**	    zero and passing back DIlru_open() err_code info.  And ifdef'd
**	    variables only used by "FSYNC" case to shut up lint.
**	2-Feb-90 (anton)
**	    Don't always copy CL_ERR_DESC
**	11-may-90 (blaise)
**	    Integrated changes from 61 & ug:
**		Interpret open_flags with DI_SYNC_MASK instead of with 
**		DI_O_FSYNC_MASK
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <csev.h>.
**	01-Oct-90 (anton)
**	    Reentrant CL changes
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
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	30-nov-1992 (rmuth)
**	      - Prototype and include "dilru.h"
**	      - Remove fwd reference to lseek, not used.
**	      - Add error checking.
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
**	18-apr-1994 (jnash)
**   	    fsync project.  DIforce() now calls fsync unconditionally
**	    (assuming that it exists).
**	19-apr-95 (canor01)
**	    added <errno.h>
**	02-Jan-1996 (walro03/mosjo01)
**		Bug 73577: On dg8_us5, fsync returns EOPNOTSUPP for character 
**		special files (like raw logs), not EINVAL.
**	01-may-1995 (amo ICL)
**	    Added anb's asyncio amendments
**	29-Apr-1997 (merja01)
**      Added ifdef for axp_osf because the aiocb does not have an 
**      aio_errno but does have a aio_error function.
**      02-may-1997 (bobmart)
**          Similar change for bug 73577 must be made for dgi_us5.
**	24-Oct-1997 (hanch04)
**	    Changed ifdef for axp_osf to be the default
**      03-Nov-1997 (hanch04)
**          Added LARGEFILE64 for files > 2gig
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs to dilocal.h
**	06-Mar-1987 (kosma01)
**	    rs4_us5 does not have an aio_fsync, using standard fsync instead.
**	01-jun-1998 (muhpa01)
**	    Add hpb_us5 to list of platforms which must use aio_error().
**	16-Nov-1998 (jenjo02)
**	    Distinguish CSsuspend types by read/write.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**      22-Jan-2007 (hanal04) Bug 117533
**          hpb compiler warning mesages when calling aio_error(). Call
**          aio_error64() instead.
**/

/* # defines */

/* typedefs */

/* forward references */
#ifdef xCL_010_FSYNC_EXISTS
static STATUS 	DI_force(
			    DI_IO	*f,
			    DI_OP	*diop,
			    CL_ERR_DESC	*err_code);
#endif /* FSYNC_EXISTS */


/* externs */

#ifdef xCL_ASYNC_IO
FUNC_EXTERN DI_AIOCB * DI_get_aiocb();
#endif /* xCL_ASYNC_IO */

/* statics */


/*{
** Name: DIforce - Forces all pages to the disk.
**
** Description:
**      The DIforce routine is used to force all pages held by an operating 
**	system to disk.  This is not necessary on VMS so this routine will just
**	return.  This routine should wait for completion of all I/O to insure 
**	all pages are correctly on disk.  If an error occurs it should return 
**	DI_BADWRITE.
**   
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADFILE       Bad file context.
**          DI_BADWRITE      Error forcing pages to disk.
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
**	    DI_SYNC_MASK support (O_SYNC, fsync()).
**	06-feb-89 (mikem)
**	    Added better support for DI CL_ERR_DESC, including initializing to 
**	    zero and passing back DIlru_open() err_code info.  And ifdef'd
**	    variables only used by "FSYNC" case to shut up lint.
**	21-Apr-89 (GordonW)
**	    change "#ifdef FSYNC_EXISTS" -> it's correct xCL_xx define.
**	2-Feb-90 (anton)
**	    Don't always copy CL_ERR_DESC
**	25-sep-1991 (mikem) integrated following change: 27-jul-91 (mikem)
**	    DIopen() now sets the flag DI_O_FSYNC_MASK if fsync() should be
**	    used to sync a DIforce of a file.  Change the code to use 
**	    DI_O_FSYNC_MASK rather than DI_SYNC_MASK.
**	30-nov-1992 (rmuth)
**	    - Prototype.
**	    - Add error checking.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	18-apr-1994 (jnash)
**   	    fsync project.  DIforce() now calls fsync unconditionally
**	    (assuming that it exists).
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
**	11-Jan-2008 (kschendel) b122122
**	    Force has long been a no-op on unix, but that's incorrect.
**	    It should fsync or fdatasync the file unless the file is
**	    already open in sync mode.
*/
STATUS
DIforce(
    DI_IO          *f,
    CL_ERR_DESC     *err_code)
{
    STATUS			status = OK;

#ifdef xCL_010_FSYNC_EXISTS
    DI_OP			diop;
#endif /* FSYNC_EXISTS */

    /* default return values */
    CL_CLEAR_ERR( err_code );

    /* Check file control block pointer, return if bad. */

    if (f->io_type != DI_IO_ASCII_ID)
	return(DI_BADFILE);

    /* Count a force */
    f->io_stat.force++;


    /* Don't do anything to the file if it's open O_SYNC or with direct
    ** IO.  Otherwise the caller wants the file sync'ed, so do it.
    */

    if ( (f->io_open_flags & DI_O_OSYNC_MASK) == 0
      && (f->io_fprop & FPROP_DIRECT) == 0)
    {
#if ! defined(xCL_010_FSYNC_EXISTS)
	/* Yarggh!  no fsync and file has no osync.  This must be some
	** obsolete or improperly ported platform.  Use global sync.
	*/
	sync();

#else

	do
	{
	    /* 
	    ** get file descriptor for this file 
	    */
	    status = DIlru_open(f, FALSE, &diop, err_code);
	    if ( status != OK )
		break;

	    status = DI_force( f, &diop, err_code );
	    if ( status != OK )
	    {
		CL_ERR_DESC lerr_code;

		(VOID) DIlru_release(&diop, &lerr_code);
	    }
	    else
	    {
		status = DIlru_release(&diop, err_code);
	    }

	} while (FALSE);
#endif /* FSYNC_EXISTS */
    }


    return (status);
}

#ifdef xCL_010_FSYNC_EXISTS

/* Although it may be that fdatasync is good enough, it's not
** clear that all filesystems implement it properly (including update
** of file size changes).  Stick to fsync for now;  if testing shows
** that fdatasync is OK for a particular platform, it can be changed.
*/

#  define FSYNC fsync		/* Nothing using fdatasync at the moment */

static STATUS
DI_force(
    DI_IO	*f,
    DI_OP	*diop,
    CL_ERR_DESC	*err_code)
{

    STATUS	big_status = OK, small_status = OK, intern_status = OK;
    register DI_SLAVE_CB	*disl;

    do
    {
        if (Di_slave)
	{
	    disl = diop->di_evcb;
	    disl->file_op = DI_SL_SYNC;
	    /* Send file properties to slave */
	    FPROP_COPY(f->io_fprop,disl->io_fprop);

	    DI_slave_send( disl->dest_slave_no, diop,
			   &big_status, &small_status, &intern_status);

	    if (big_status != OK )
	        break;

	    if ( small_status == OK )
	    {
	        if ((small_status = disl->status) != OK )
	        {
		    STRUCT_ASSIGN_MACRO(disl->errcode, *err_code);
		}
	    }
	}
	else
	{
	    /* 
	    ** put code in here for fsync issues 
	    */
#ifdef xCL_ASYNC_IO
            if( Di_async_io)
	    {
                DI_AIOCB *aio;
                aio=DI_get_aiocb();
#ifdef dr6_us5
                aio->aio.aio_filedes=diop->di_fd;
#else
                aio->aio.aio_fildes=diop->di_fd;
#endif /* dr6_us5 */
#ifdef LARGEFILE64
                if(aio_fsync64( O_SYNC, &aio->aio))
#elif defined(any_aix)
                if(fsync( aio->aio.aio_fildes ))
#else
                if(aio_fsync( O_SYNC, &aio->aio))
#endif /* LARGEFILE64 */
                {
	        SETCLERR(err_code, 0, ER_fsync);
                    small_status = FAIL;
                    break;
                }
                else
                {
                    if( (small_status=CSsuspend( CS_DIOW_MASK, 0, 0) ) != OK)
                    {
                        DIlru_set_di_error( &small_status, err_code, 
				DI_LRU_CSSUSPEND_ERR, DI_GENERAL_ERR);
		        break;
                    }
#if defined(axp_osf) 
                    if (  (aio_error(&aio->aio)) != 0 )
#else
#ifdef LARGEFILE64
                    if (  (aio_error64(&aio->aio)) != 0 )
#else /* LARGEFILE64 */
                    if (  (aio_error(&aio->aio)) != 0 )
#endif /* LARGEFILE64 */
#endif
                    {
	                SETCLERR(err_code, 0, ER_fsync);
	                small_status = FAIL;
                        break;
                    }
                }
            }
            else 
#endif /* xCL_ASYNC_IO */
	    if (FSYNC(diop->di_fd) < 0)
	    {
#ifdef xCL_092_NO_RAW_FSYNC
		/* AIX returns EINVAL on character special files */
#if defined( dg8_us5 ) || defined( dgi_us5 ) 
		if (errno != EOPNOTSUPP)
#else
		if (errno != EINVAL) 
#endif /* dg8_us5 */
#endif /* xCL_092_NO_RAW_FSYNC */
		{
	            SETCLERR(err_code, 0, ER_fsync);
	            small_status = FAIL;
		}
	    }
	}

    } while (FALSE);

    if ( big_status != OK )
	small_status = big_status;

    if ( small_status != OK  )
        DIlru_set_di_error( &small_status, err_code, intern_status,
			    DI_GENERAL_ERR);

    return( small_status );
}
#endif /* FSYNC_EXISTS */
