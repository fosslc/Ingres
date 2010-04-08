/******************************************************************************
**
** Copyright (c) 1987, 2003 Ingres Corporation
**
******************************************************************************/

#include   <compat.h>
#include   <cs.h>
#include   <di.h>
#include   <er.h>
#include   <pc.h>
#include   <csinternal.h>
#include   "dilru.h"


extern STATUS DI_sense(DI_IO *, i4 *, CL_SYS_ERR *);
GLOBALREF i4 Di_writes ;	
GLOBALREF CS_SYSTEM        Cs_srv_block;

/* statics */

/******************************************************************************
**
** Name: DIwrite -  Writes  page(s) of a file to disk.
**
** Description:
**      The DIwrite routine is used to write pages of a direct access
**      file.  This routine should be flexible enough to write multiple
**      contiguous pages.  The number of pages to write is indicated
**      as an input parameter,  This value is updated to indicate the
**      actual number of pages written.  A synchronous write is preferred
**      but not required.
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      n                    Pointer to value indicating number of pages to
**			     write.
**      page                 Value indicating page(s) to write.
**      buf                  Pointer to page(s) to write.
**
** Outputs:
**      f        Updates the file control block.
**      n        Pointer to value indicating number of pages
**			     written.
**      err_code Pointer to a variable used to return operating system
**               errors.
**    Returns:
**          OK
**          DI_BADFILE       Bad file context.
**          DI_BADWRITE      Error writing.
**          DI_BADPARAM      Parameter(s) in error.
**          DI_ENDFILE       Write past end of file.
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
**	19-jul-95 (emmag)
**	    Modified to use overlapped I/O.
**	19-jul-95 (emmag)
**	    Added above comment which got lost in the integrate.  Also 
**	    modified SETWIN32ERR call to use Joe's new style.
**	30-aug-1995 (shero03)
**	    Added Di_reads for TMperfstat and update dio count in scb.
**	05-dec-1995 (canor01)
**	    Hold the DI_IO semaphore only until request is queued.
**	    Remove calls to SetFilePointer(), which is not needed with
**	    overlapped i/o.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	07-feb-1996 (canor01)
**	    Update the server dio count, since with overlapped i/o, we
**	    don't call CSsuspend().
**	09-feb-1996 (canor01)
**	    Get exclusive semaphore on DI_IO before updating it in DI_sense
**      22-mar-1996 (emmag)
**          WriteFile() with an overlapped structure can return FALSE, 
**	    even if the read has completed successfully and GetLastError() 
**	    returns OK.    Must make a call to GetOverlappedResult() to 
**	    sync things.   This problem has been highlighted by the move
**	    to TLS and is observed more frequently on SMP and fast single
**	    CPU machines.
**      15-may-1996 (sarjo01)
**          When WriteFile() returns an error condition (FALSE) in the
**          case of e.g. out-of-disk-space, CSv_semaphore() was being called
**          before GetLastError(), clearing the error and causing the
**          subsequent GetOverlappedResult() call to hang. The fix was to
**          switch the order of calling the 2 functions. Bug no. 76481 
**	17-apr-1997 (canor01)
**	    Add DIwrite_list() function to resolve links.
**      21-oct-1997 (canor01)
**          Support for larger files.
**      08-dec-1997 (canor01)
**          Implement LRU for open files (initial copy from Unix).
**      28-jan-1998 (canor01)
**          Optimize LRU--only call DIlru_open if file has been closed.
**      12-feb-1998 (canor01)
**          Increment reference count and release semaphore while doing
**          the actual read/write. Use the same overlapped event for all
**          reads and writes done by the thread.
**	01-oct-1998 (somsa01)
**	    In the case of ERROR_DISK_FULL, return DI_NODISKSPACE.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	10-jan-2000 (somsa01)
**	    Added setting of CS_DIOW_MASK and CS_LIOW_MASK, and obtain
**	    the CPU time for the write.
**	11-oct-2000 (somsa01)
**	    Deleted cs_dio, cs_dio_done, cs_dio_wait, cs_dio_time stats,
**	    keep KB written stat.
**	12-sep-2003 (somsa01)
**	    Removed security attributes setting, as we do not use it.
**	24-sep-2003 (somsa01)
**	    Moved around setting of cs_state and cs_memory to proper places,
**	    as well as the releaseing of the io_sem mutex. also, removed
**	    DIwrite_list(0, as we do not use this on Windows.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**
******************************************************************************/
STATUS
DIwrite(DI_IO      *f,
        i4         *n,
        i4         page,
        char       *buf,
        CL_SYS_ERR *err_code)
{
	STATUS  ret_val = OK;
	i4      num_of_pages;
	i4      last_page_to_write;
	i4      eof;
	LARGE_INTEGER   chptr_offset;
	ULONG   bytes_to_write;
	ULONG   bytes_written;
	CS_SCB	*scb;
	OVERLAPPED		Overlapped;
	DWORD   locstatus;
	i4	start_time;

	CSget_scb(&scb);

	/* default returns */

	CLEAR_ERR(err_code);

	num_of_pages       = *n;
	last_page_to_write = page + num_of_pages - 1;

	/*
	 * check file control block pointer, return if bad.
	 */

	if (f->io_type != DI_IO_ASCII_ID)
		return (DI_BADFILE);

	/*
	 * lock DI_IO structure
	 */

	CS_synch_lock(&f->io_sem);

	if ( f->io_nt_fh == INVALID_HANDLE_VALUE )
	    ret_val = DIlru_open( f, FALSE, err_code );
	if ( ret_val != OK )
	{
	    CS_synch_unlock( &f->io_sem );
	    return (ret_val);
	}

	f->io_refcnt++;

	/*
	 * if trying to write past eof, maybe another server has extended it,
	 * so update eof and check again
	 */

	if (last_page_to_write > f->io_system_eof) 
	{
		if (DI_sense(f, &eof, err_code) != OK) 
		{
			f->io_refcnt--;
			CS_synch_unlock(&f->io_sem);
			return DI_ENDFILE;
		}
		
		f->io_system_eof = max(f->io_alloc_eof, eof);

		if (last_page_to_write > f->io_system_eof) 
		{
			f->io_refcnt--;
			CS_synch_unlock(&f->io_sem);
			return DI_ENDFILE;
		}
	}

	/*
 	** Unlock DI_IO struct
 	*/
	CS_synch_unlock(&f->io_sem);

	chptr_offset.QuadPart = Int32x32To64( f->io_bytes_per_page, page );
	bytes_to_write = f->io_bytes_per_page * num_of_pages;

	/*
	** Set up the overlapped structure.
	*/

        Overlapped.hEvent = scb->cs_di_event;
	Overlapped.Offset = chptr_offset.LowPart;
	Overlapped.OffsetHigh = chptr_offset.HighPart;

	if (f->io_open_flags & DI_O_LOG_FILE_MASK)
	{
	    scb->cs_memory |= CS_LIOW_MASK;
	    scb->cs_liow++;
	    Cs_srv_block.cs_wtstatistics.cs_liow_done++;
	    Cs_srv_block.cs_wtstatistics.cs_liow_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_liow_kbytes
		+= bytes_to_write / 1024;
	}
	else
	{
	    scb->cs_memory |= CS_DIOW_MASK;
	    scb->cs_diow++;
	    Cs_srv_block.cs_wtstatistics.cs_diow_done++;
	    Cs_srv_block.cs_wtstatistics.cs_diow_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_diow_kbytes
		+= bytes_to_write / 1024;
	}
	scb->cs_state = CS_EVENT_WAIT;

	/*
	 * write data
	 */

	start_time = CS_checktime();
	if (WriteFile (f->io_nt_fh,
	              buf,
	              bytes_to_write,
		      &bytes_written,	
	              &Overlapped)) 
	{

		if (bytes_to_write != bytes_written) 
		{
			SETWIN32ERR(err_code, GetLastError(), ER_write);
			ret_val = DI_BADWRITE;
		}
	} 
	else 
	{
		locstatus = GetLastError();

                switch(locstatus)
                {
		    case ERROR_IO_PENDING:
			WaitForSingleObject ( Overlapped.hEvent, INFINITE);
			break;
	
		    case OK:
                        locstatus = GetOverlappedResult( f->io_nt_fh,
                                                         &Overlapped,
                                                         &bytes_written,
                                                         TRUE );
                        if ( locstatus == FALSE )
                        {
                            locstatus = GetLastError();
                            SETWIN32ERR(err_code, locstatus, ER_write);
			    if (locstatus == ERROR_DISK_FULL)
				ret_val = DI_NODISKSPACE;
			    else
                        	ret_val = DI_BADWRITE;
                        }
                        break;
			
		     
		    default:
			SETWIN32ERR(err_code, locstatus, ER_write);
			if (locstatus == ERROR_DISK_FULL)
			    ret_val = DI_NODISKSPACE;
			else
			    ret_val = DI_BADWRITE;
		}
	}

	if (f->io_open_flags & DI_O_LOG_FILE_MASK)
	    Cs_srv_block.cs_wtstatistics.cs_liow_time +=
		CS_checktime() - start_time;
	else
	    Cs_srv_block.cs_wtstatistics.cs_diow_time +=
		CS_checktime() - start_time;

	scb->cs_memory &= ~(CS_DIOR_MASK | CS_LIOR_MASK);
	scb->cs_state = CS_COMPUTABLE;

	CS_synch_lock( &f->io_sem );
	f->io_refcnt--;
	CS_synch_unlock( &f->io_sem );

	Di_writes++;	/* increment counter */

	return (ret_val);
}
