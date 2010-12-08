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
#include   "dilocal.h"
#include   "dilru.h"

/* unix includes */

#ifdef      xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */

#ifdef      xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif      /* xCL_007_FILE_H_EXISTS */

#include    <errno.h>

/**
**
**  Name: DICLOSE.C - This file contains the UNIX DIclose() routine.
**
**  Description:
**      The DICLOSE.C file contains the DIclose() routine.
**
**        DIclose - Closes a file.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**	05-aug-88 (roger)
** 	    UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
**	07-oct-88 (jeff)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	05-jan-89 (jeff)
**	    added traceing
**	24-jan-89 (roger)
**	    Make use of new CL error descriptor.
**	06-feb-89 (mikem)
**	    better support for DI CL_ERR_DESC, including initializing to zero.
**	    Also mark io_type as DI_IO_CLOSE_INVALID to track down di problems
**	    with people using non-open DI_IO structures in DI calls.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <csev.h>.
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	15-jul-92 (swm)
**	    Add include <seek.h> so we can compile on a DRS6000 (as there
**	    is now a reference to L_XTND).
**	30-October-1992 (rmuth)
**	    Remove the ZPAD option the user is now required to call
**	    DIguarantee_space. Move the #include <seek.h> as we #define
**	    L_SET, LINCR and L_XTND .
**	30-nov-1992 (rmuth)
**	    - Remove lseek stuff.
**	    - DIlru error checking.
**	    - This routine can now return more than just DI_xxx error
**	      messages but these messages are only returned when a very 
**	      unexpected error has occured.
**
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	19-apr-95 (canor01)
**	    added <errno.h>
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**/

/* # defines */

/* typedefs */

/* forward references */

/* externs */


/*{
** Name: DIclose - Closes a file.
**
** Description:
**      The DIclose routine is used to close a direct access file.  
**      It will check to insure the file is open before trying to close.
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADFILE       Bad file context.
**	    
**	    A range of errors from unexpected events in DILRU code.
** 
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    26-mar-87 (mmm)
**          Created new for 5.0.
**    06-feb-89 (mikem)
**	    better support for DI CL_ERR_DESC, including initializing to zero.
**	    Also mark io_type as DI_IO_CLOSE_INVALID to track down di problems
**	    with people using non-open DI_IO structures in DI calls.
**    30-October-1992 (rmuth)
**	    Remove all the ZERO_ALLOC stuff and prototype it.
**    30-nov-1992 (rmuth)
**	    - Remove lseek stuff.
**	    - DIlru error checking.
**    03-jun-1996 (canor01)
**	    Release the semaphore used with operating-system threads.
**	25-Feb-1998 (jenjo02)
**	    Added stats to DI_IO.
**    15-Apr-2004 (fanch01)
**      Change behavior to tolerate multiple closes.
**	26-Jul-2005 (schka24)
**	    Release io q semaphore too.
**	30-Sep-2005 (jenjo02)
**	    io_fd_q_sem now a CS_SYNCH object.
**	06-Oct-2005 (jenjo02)
**	    DI_FD_PER_THREAD now config param
**	    ii.$.$.$.fd_affinity:THREAD, Di_thread_affinity = TRUE,
**	    still dependent on OS_THREADS_USED.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**	17-Nov-2010 (kschendel) SIR 124685
**	    Compiler saw that status wasn't always set, fix.  (Problem would
**	    only occur on an error path.)
*/
STATUS
DIclose(
    DI_IO          *f,
    CL_ERR_DESC     *err_code )
{
    STATUS      status;

    /* default returns */
    CL_CLEAR_ERR( err_code );
    status = OK;

    /* Check file control block pointer, return if bad.*/

    if (f->io_type != DI_IO_ASCII_ID && f->io_type != DI_IO_CLOSE_INVALID)
    {
	status = DI_BADFILE;
    }
    else if (f->io_type == DI_IO_ASCII_ID)
    {
#ifdef	xDEV_TST
	TRdisplay("DIclose: %t/%t\n", f->io_l_pathname, f->io_pathname,
		  f->io_l_filename, f->io_filename);
#endif
	/* Count another close */
	f->io_stat.close++;
	status = DIlru_close(f, err_code);
    }

    if (f->io_type != DI_IO_CLOSE_INVALID)
    {
# ifdef OS_THREADS_USED
	/* Release the semaphore */
	CS_synch_destroy( &f->io_sem );
	if ( Di_thread_affinity )
	    CS_synch_destroy( &f->io_fd_q_sem );
# endif /* OS_THREADS_USED */
    }

    /* invalidate this file pointer */

    f->io_type = DI_IO_CLOSE_INVALID;

    return(status);
}
