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

/* externs */

extern STATUS DI_sense(DI_IO *, i4 *, CL_SYS_ERR *);

/* statics */

/******************************************************************************
**
** Name: DIgalloc - Allocates a page to a direct access file.
**
** Description:
**      The DIgalloc routine is used to add pages to a direct
**      access file.  This routine can add more than one page
**      at a time by accepting a count of the number of pages to add.
**
**      The end of file and allocated are not updated on disk until a DIflush
**      call is issued.  This insures that pages are not considered valid
**      until after they are formatted.  The allocation can be ignored if
**      the file is closed or the system crashes before the DIflush.
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
**        DI_BADEXTEND      Can't allocate disk space
**        DI_BADFILE        Bad file context.
**        DI_EXCEED_LIMIT   Too many open files.
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
**      21-oct-1997 (canor01)
**          Support for larger files.
**      08-dec-1997 (canor01)
**          Implement LRU for open files (initial copy from Unix).
**      28-jan-1998 (canor01)
**          Optimize LRU--only call DIlru_open if file has been closed.
**	31-aug-1998 (canor01)
**	    Correct parameters passed to DIlru_open.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**
******************************************************************************/
STATUS
DIgalloc(DI_IO     *f,
        i4         n,
        i4         *page,
        CL_SYS_ERR *err_code)
{
    LARGE_INTEGER eof;
    DWORD status = OK;

    CLEAR_ERR(err_code);

    /*
     * Check file control block pointer, return if bad.
     */

    if (f->io_type != DI_IO_ASCII_ID)
    	return (DI_BADFILE);

    /*
     * lock DI_IO struct
     */

    CS_synch_lock(&f->io_sem);

    /* get file descriptor for this file */
    do
    {

	if ( f->io_nt_fh == INVALID_HANDLE_VALUE )
            status = DIlru_open( f, FALSE, err_code);
        if ( status != OK )
            break;

        /*
        ** find current end-of-file
        */
        if (DI_sense(f, page, err_code) != OK) 
	{
	    status = DI_ENDFILE;
	    break;
        }
    
        /*
        ** set first new page allocated
        */

        *page = f->io_system_eof + 1;

        /*
        ** move to new end-of-file
        */
        eof.QuadPart = Int32x32To64( f->io_system_eof + n + 1,
    			         f->io_bytes_per_page );
    
        if ((SetFilePointer(f->io_nt_fh,
                             eof.LowPart,
                             &eof.HighPart,
                             FILE_BEGIN)) == -1) 
        {
            if ( (status = GetLastError()) != NO_ERROR )
            {
    	        SETWIN32ERR(err_code, GetLastError(), ER_alloc);
    	        status = DI_BADEXTEND;
		break;
            }
        }

        if (!SetEndOfFile(f->io_nt_fh)) 
        {
    	    SETWIN32ERR(err_code, GetLastError(), ER_alloc);
    	    status = DI_BADEXTEND;
	    break;
        }

        /*
        ** current last block in file
        */
    
        f->io_system_eof += n;


    } while (FALSE);
    
    CS_synch_unlock(&f->io_sem);

    return ( status );
}
