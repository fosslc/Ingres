/******************************************************************************
**
** Copyright (c) 1987, 1997 Ingres Corporation
**
******************************************************************************/

#include <compat.h>
#include <cs.h>
#include <di.h>
#include <er.h>

/* # defines */

/* typedefs */

/* forward references */

/* externs */

STATUS DI_sense(DI_IO *, i4 *, CL_SYS_ERR *);

/* statics */

/******************************************************************************
**
** Name: DIflush - Flushes a file.
**
** Description:
**      The DIflush routine is used to write the file header to disk.
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
**      18-jul-95 (reijo01)
**          Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**              the proper values.
**	09-feb-1996 (canor01)
**	    Get exclusive semaphore on DI_IO before updating it in DI_sense
**	01-apr-1996 (canor01)
**	    Check for a valid GetLastError() after call to SetFilePointer().
**	21-oct-1997 (canor01)
**	    Support for larger files.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**
******************************************************************************/
STATUS
DIflush(DI_IO      *f,
        CL_SYS_ERR *err_code)
{
	i4 last_page;
	STATUS  status;
	LARGE_INTEGER eof;

	CLEAR_ERR(err_code);

	if (f->io_type != DI_IO_ASCII_ID)
		return DI_BADFILE;

	/*
	 * lock DI_IO struct
	 */

	CS_synch_lock(&f->io_sem);

	/*
	 * find current end-of-file
	 */

	if (DI_sense(f, &last_page, err_code) != OK) 
	{
		CS_synch_unlock(&f->io_sem);
		return DI_ENDFILE;
	}

	/*
	 * move to new end-of-file
	 */

	eof.QuadPart = Int32x32To64(f->io_system_eof + 1, f->io_bytes_per_page);

	if ((SetFilePointer(f->io_nt_fh,
	                          eof.LowPart,
	                          &eof.HighPart,
	                          FILE_BEGIN)) == -1) 
	{
		if ( ( status = GetLastError() ) != NO_ERROR )
		{
			CS_synch_unlock(&f->io_sem);
			SETWIN32ERR(err_code, status, ER_flush);
			return DI_BADEXTEND;
		}
	}

	if (!SetEndOfFile(f->io_nt_fh)) 
	{
		status = GetLastError();
		CS_synch_unlock(&f->io_sem);
		SETWIN32ERR(err_code, status, ER_flush);
		return DI_BADEXTEND;
	}

	/*
	 * unlock DI_IO struct
	 */

	CS_synch_unlock(&f->io_sem);
	return OK;
}
