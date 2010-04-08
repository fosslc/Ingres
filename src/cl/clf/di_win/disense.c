/******************************************************************************
**
** Copyright (c) 1987, 1998 Ingres Corporation
**
******************************************************************************/

#include   <compat.h>
#include   <cs.h>
#include   <di.h>
#include   <er.h>
#include   <limits.h>
#include   "dilru.h"

/* forward references */

STATUS DI_sense(DI_IO *, i4 *, CL_SYS_ERR *);

/* externs */

/* statics */

/******************************************************************************
**
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
**      18-jul-95 (reijo01)
**          Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**              the proper values.
**	09-feb-1996 (canor01)
**	    Get exclusive semaphore on DI_IO before updating it in DI_sense
**	21-oct-1997 (canor01)
**	    Support for larger files.
**      08-dec-1997 (canor01)
**          Implement LRU for open files (initial copy from Unix).
**      28-jan-1998 (canor01)
**          Optimize LRU--only call DIlru_open if file has been closed.
**	08-aug-1999 (mcgem01)
**	    Change longnat to i4.
**	27-oct-1999 (somsa01 for rosal05)
**	    Cross of fix for Jasmine bug 98754. In DI_sense(), if the store
**	    file has more than 0xFFFFFFFF pages the calculation for the
**	    high-order DWORD was wrong. Instead of dividing MAXDWORD by the
**	    bytes per page we should divide (MAXDWORD + 1) by the bytes per
**	    page. In the first case the result is a floating point number
**	    than due to automatic casting the result is lower than the actual
**	    number of pages in the store.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**
******************************************************************************/
STATUS
DIsense(DI_IO      *f,
        i4         *page,
        CL_SYS_ERR *err_code)
{
	STATUS          ret_val = OK;

	/* default returns */

	CLEAR_ERR(err_code);

	/* check file control block pointer, return if bad. */

	if (f->io_type != DI_IO_ASCII_ID)
		return DI_BADFILE;

	/*
	 * Lock the DI_IO stuct
	 */

	CS_synch_lock(&f->io_sem);

	if ( f->io_nt_fh == INVALID_HANDLE_VALUE )
	    ret_val = DIlru_open( f, FALSE, err_code );
	if ( ret_val == OK )
	{
	    ret_val = DI_sense(f, page, err_code);
	}

	/*
	 * Unlock the DI_IO stuct
	 */

	CS_synch_unlock(&f->io_sem);

	return (ret_val);
}
/******************************************************************************
**
** The system call is needed when attempting to find out how big a
** file that may be affected by more than one process at a time.  For
** instance if two different processes add a page to the file the
** system call is the only way to tell that the file is now 2 pages
** bigger rather than just one page bigger.
**
******************************************************************************/
STATUS
DI_sense(DI_IO      *f,
         i4         *last_page,
         CL_SYS_ERR *err_code)
{
	LARGE_INTEGER eof;
	STATUS ret_val = OK;
	DWORD  lasterror = 0;
	DWORD  reteof;

        eof.LowPart = 0;
        eof.HighPart = 0;

	if ((reteof = SetFilePointer(f->io_nt_fh,
	                            eof.LowPart,
	                            &eof.HighPart,
	                            FILE_END)) == -1 &&
	    (lasterror = GetLastError()) != NO_ERROR) 
	{
	    SETWIN32ERR(err_code, lasterror, ER_sense);
	    ret_val = DI_BADINFO;
	} 
	else 
	{
	    *last_page    = ( reteof / f->io_bytes_per_page) - 1;
	    if ( eof.HighPart )
	    {
		/* pow(2,32) =  4294967296 */
		*last_page += (long)(4294967296 / f->io_bytes_per_page) *
				eof.HighPart;
	    }

	    f->io_system_eof = max(f->io_system_eof, *last_page);
	}

	return (ret_val);
}
