/******************************************************************************
**
** Copyright (c) 1987, 1998 Ingres Corporation
**
******************************************************************************/

#define	INCL_DOSERRORS

#include <compat.h>
#include <cs.h>
#include <di.h>
#include <er.h>
#include "dilru.h"

/******************************************************************************
**
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
**      18-jul-95 (reijo01)
**          Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**              the proper values.
**	09-feb-1996 (canor01)
**	    Get exclusive semaphore on DI_IO before updating it in DI_sense
**      08-dec-1997 (canor01)
**          Implement LRU for open files (initial copy from Unix).
**	28-jan-1998 (canor01)
**	    Optimize LRU--only call DIlru_open if file has been closed.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**
******************************************************************************/
STATUS
DIforce(DI_IO      *f,
        CL_SYS_ERR *err_code)
{
    STATUS ret_val = OK;

    CLEAR_ERR(err_code);

    if (f->io_type != DI_IO_ASCII_ID)
    	return DI_BADFILE;

    /*
     * Flush file buffers
     */

    if (f->io_open_flags & DI_O_FSYNC_MASK)
    {
        /*
         * lock DI_IO structure
         */

        CS_synch_lock(&f->io_sem);

        do
        {
            /*
            ** get file descriptor for this file
            */
	    if ( f->io_nt_fh == INVALID_HANDLE_VALUE )
                ret_val = DIlru_open(f, FALSE, err_code);
            if ( ret_val != OK )
                break;

    	    if (!FlushFileBuffers(f->io_nt_fh)) 
            {
    	        ret_val = DI_BADWRITE;
    	        SETWIN32ERR(err_code, GetLastError(), ER_force);
            }

	} while (FALSE);

        /*
         * Unlock struct
         */
    
        CS_synch_unlock(&f->io_sem);

    }

    return (ret_val);
}
