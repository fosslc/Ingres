/******************************************************************************
**
** Copyright (c) 1987, 1998 Ingres Corporation
**
******************************************************************************/

#include   <compat.h>
#include   <cs.h>
#include   <di.h>
#include   <er.h>
#include   "dilru.h"

/******************************************************************************
**
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
**          DI_BADCLOSE         Close operation failed.
**          DI_BADFILE       Bad file context.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**      18-jul-1995 (reijo01)
**          Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**              the proper values.
**	05-dec-1995 (canor01)
**	    Release the DI_IO semaphore when file is closed.
**      08-dec-1997 (canor01)
**          Implement LRU for open files (initial copy from Unix).
**	28-jan-1998 (canor01)
**	    Don't remove the semaphore if the DI_IO block is invalid.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**
**
******************************************************************************/
STATUS
DIclose(DI_IO      *f,
	CL_SYS_ERR *err_code)
{
    STATUS ret_val = OK;

    /* default returns */

    CLEAR_ERR(err_code);

    /* Check file control block pointer, return if bad. */

    if (f->io_type != DI_IO_ASCII_ID) 
    {
    	ret_val = DI_BADFILE;
    } 
    else 
    {
	CS_synch_lock(&f->io_sem);

	ret_val = DIlru_close( f, err_code );

        /* invalidate this file pointer */
        f->io_type = DI_IO_CLOSE_INVALID;

	CS_synch_unlock(&f->io_sem);

        /* release the semaphore */
        CS_synch_destroy( &f->io_sem );

    }

    return (ret_val);
}
