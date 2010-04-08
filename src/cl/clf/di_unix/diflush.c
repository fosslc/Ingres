/*
**Copyright (c) 2004 Ingres Corporation
*/

#include   <compat.h>
#include   <gl.h>
#include   <systypes.h>
#include   <clconfig.h>
#include   <cs.h>
#include   <fdset.h>
#include   <csev.h>
#include   <di.h>
#include   <dislave.h>
#include   "dilocal.h"

#ifdef	    xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */

#ifdef      xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif	    /* xCL_007_FILE_H_EXISTS */

#include    <errno.h>

/**
**
**  Name: DIFLUSH.C - This file contains the UNIX DIflush() routine.
**
**  Description:
**      The DIFLUSH.C file contains the DIflush() routine.
**
**        DIflush - Flushes header to a file.
**
** History:
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	17-oct-88 (jeff)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	24-jan-89 (roger)
**	    Make use of new CL error descriptor.
**	06-feb-89 (mikem)
**	    Initialize CL_ERR_DESC to zero.
**	27-Feb-1989 (fredv)
**	    Replaced xDI_xxx by corresponding xCL_xxx defs in clconfig.h.
**	    include <clconfig.h> and <systypes.h>
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	30-October-1992 (rmuth)
**	    Remove references to DI_ZERO_ALLOC, prototype function and
**	    remove forward reference to lseek.
**	30-nov-1992 (rmuth)
**	    Remove io_logical_eof stuff, as we now track things at the
**	    higher levels.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	19-apr-95 (canor01)
**	    added <errno.h>
**	25-Feb-1998 (jenjo02)
**	    Added stats to DI_IO.
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
** Name: DIflush - Flushes a file.
**
** Description:
**      The DIflush routine is used to write the file header to disk.
**      The file header contains the end of file and allocated information.
**      This routine maybe a null operation on systems that can't
**      support it.
**      
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
**          DI_BADEXTEND     Error flushing file header.
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
**	06-feb-89 (mikem)
**	    Initialize CL_ERR_DESC to zero.
**	30-October-1992 (rmuth)
**	    Remove references to DI_ZERO_ALLOC, prototype function.
**	30-nov-1992 (rmuth)
**	    Remove io_logical_eof stuff, as we now track things at the
**	    higher levels.
**
**
** Design Details:
**
**	UNIX DESIGN:
**
**	There is no concept of file header on disc, so just return.
**
*/
STATUS
DIflush(
    DI_IO          *f,
    CL_ERR_DESC     *err_code ) 
{

    CL_CLEAR_ERR( err_code );

    /* Check file control block pointer, return if bad. */

    if (f->io_type != DI_IO_ASCII_ID)
    {
	return(DI_BADFILE);
    }

    /* Count a flush */
    f->io_stat.flush++;

    return(OK);
}
