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

/* externs */

extern STATUS DI_sense(DI_IO *, i4 *, CL_SYS_ERR *);
GLOBALREF i4 Di_reads ;	
GLOBALREF CS_SYSTEM        Cs_srv_block;

/* statics */

/******************************************************************************
**
** Name: DIread - Read a page of a file.
**
** Description:
**      The DIread routine is used to read pages of a direct access
**      file.   For the large block read option, the number of pages
**      to read is an input parameter to this routine.  It will
**      return the number of pages it read, since at
**      end of file it may read less pages than requested.
**      If multiple page reads are requested, the buffer is assumed
**      to be large enough to hold n pages.  The size of a page is
**      determined at create.
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      n                    Pointer to value of number of pages to read.
**      page                 Value indicating page to begin reading.
**      buf                  Pointer to the area to hold
**                           page(s) being read.
**
** Outputs:
**      n                    Number of pages read.
**      f                    Updates the file control block.
**      buf                  Pointer to the page read.
**      err_code             Pointer to a variable used
**                           to return operating system
**                           errors.
**    Returns:
**          OK
**          DI_BADFILE       Bad file context.
**          DI_BADREAD       Error reading file.
**          DI_BADPARAM      Parameter(s) in error.
**          DI_ENDFILE       Not all blocks read.
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
**	    Modified to do an async read using an overlapped structure.
**	30-aug-1995 (shero03)
**	    Added Di_reads for TMperfstat and update dio count in scb.
**	05-dec-1995 (canor01)
**	    Hold the DI_IO semaphore only until request is queued.
**	    Remove calls to SetFilePointer(), which is not needed with
**	    overlapped i/o.
**      12-Dec-95 (fanra01)
**          Extracted data for DLLs on windows NT.
**	07-feb-1996 (canor01)
**	    Update the server dio count, since with overlapped i/o, we
**	    don't call CSsuspend().
**	09-feb-1996 (canor01)
**	    Get exclusive semaphore on DI_IO before updating it in DI_sense
**	19-mar-1996 (canor01)
**	    ReadFile() with overlapped can return FALSE, even if the read
**	    has completed successfully and GetLastError() returns OK.  Must
**	    call GetOverlappedResult() to sync things.
**	20-mar-1996 (canor01)
**	    The above call to GetOverlappedResult() needs to set fWait to
**	    TRUE, since the IO may not have completely completed.
**	07-aug-1997 (canor01)
**	    Incorporate mulwi01's changes: 1) number of pages read should
**	    be returned to caller; 2) partial read is DI_ENDFILE, not
**	    DI_BADREAD.
**	03-nov-1997 (canor01)
**	    Support for larger files.
**      08-dec-1997 (canor01)
**          Implement LRU for open files (initial copy from Unix).
**	28-jan-1998 (canor01)
**	    Optimize LRU--only call DIlru_open if file has been closed.
**	12-feb-1998 (canor01)
**	    Increment reference count and release semaphore while doing
**	    the actual read/write. Use the same overlapped event for all
**	    reads and writes done by the thread.
**	18-mar-1998 (canor01)
**	    Restore change from 07-aug-1997 that was lost in cross-integration.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	10-jan-2000 (somsa01)
**	    Added setting of CS_DIOR_MASK and CS_LIOR_MASK, and obtain
**	    the CPU time for the read.
**	11-oct-2000 (somsa01)
**	    Deleted cs_dio, cs_dio_done, cs_dio_wait, cs_dio_time stats,
**	    keep KB read stat.
**	12-sep-2003 (somsa01)
**	    Removed security attributes setting, as we do not use it.
**	24-sep-2003 (somsa01)
**	    Moved around setting of cs_state and cs_memory to proper places,
**	    as well as the releaseing of the io_sem mutex.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**
******************************************************************************/
STATUS
DIread(DI_IO      *f,
       i4         *n,
       i4         page,
       char       *buf,
       CL_SYS_ERR *err_code)
{
	STATUS  ret_val = OK;
	ULONG   num_of_pages;
	i4      last_page_to_read;
	i4      eof;
	LARGE_INTEGER   chptr_offset;
	ULONG   bytes_to_read;
	ULONG   bytes_read;
	CS_SCB	*scb;
        OVERLAPPED              Overlapped;
	DWORD	locstatus;
	i4	start_time;


	CSget_scb(&scb);

	/*
	 * default returns
	 */

	CLEAR_ERR(err_code);

	num_of_pages      = *n;
	last_page_to_read = page + num_of_pages - 1;

	*n = 0;

	/*
	 * check file control block pointer, return if bad.
	 */

	if (f->io_type != DI_IO_ASCII_ID)
		return DI_BADFILE;

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
	 * if trying to read past eof, maybe another server has extended it,
	 * so update eof and check again
	 */

	if (last_page_to_read > f->io_system_eof) 
	{
		if (DI_sense(f, &eof, err_code) != OK) 
		{
                        f->io_refcnt--;
			CS_synch_unlock(&f->io_sem);
			return DI_ENDFILE;
		}
		if (last_page_to_read > f->io_system_eof) 
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
	bytes_to_read = f->io_bytes_per_page * num_of_pages;


        /*
        ** Set up the overlapped structure.
        */

        Overlapped.hEvent = scb->cs_di_event;
	Overlapped.Offset = chptr_offset.LowPart;
	Overlapped.OffsetHigh = chptr_offset.HighPart;

	if (f->io_open_flags & DI_O_LOG_FILE_MASK)
	{
	    scb->cs_memory |= CS_LIOR_MASK;
	    scb->cs_lior++;
	    Cs_srv_block.cs_wtstatistics.cs_lior_done++;
	    Cs_srv_block.cs_wtstatistics.cs_lior_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_lior_kbytes += bytes_to_read / 1024;
	}
	else
	{
	    scb->cs_memory |= CS_DIOR_MASK;
	    scb->cs_dior++;
	    Cs_srv_block.cs_wtstatistics.cs_dior_done++;
	    Cs_srv_block.cs_wtstatistics.cs_dior_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_dior_kbytes += bytes_to_read / 1024;
	}
	scb->cs_state = CS_EVENT_WAIT;

	/*
	 * read data
	 */

	start_time = CS_checktime();
	if (ReadFile(f->io_nt_fh,
	             buf,
	             bytes_to_read,
	             &bytes_read,
	             &Overlapped)) 
	{
                if (bytes_to_read != bytes_read)
                {
                        SETWIN32ERR(err_code, GetLastError(), ER_read);
                        if ( bytes_read <= 0 )
                                ret_val = DI_BADREAD;
                        else
                                ret_val = DI_ENDFILE;
                }
	} 
	else 
	{
		locstatus = GetLastError();

		switch(locstatus)
		{
		    case OK:
		    case ERROR_IO_PENDING:
                        locstatus = GetOverlappedResult( f->io_nt_fh,
                                                         &Overlapped,
                                                         &bytes_read,
                                                         TRUE );
                        if ( locstatus == FALSE )
                        {
                            locstatus = GetLastError();
                            SETWIN32ERR(err_code, GetLastError(), ER_read);
                            if ( locstatus == ERROR_HANDLE_EOF )
                                ret_val = DI_ENDFILE;
                            else
                                ret_val = DI_BADREAD;
                        }
                        break;
                    case ERROR_HANDLE_EOF:
                        SETWIN32ERR(err_code, GetLastError(), ER_read);
                        ret_val = DI_ENDFILE;
                        break;
		    default:
			SETWIN32ERR(err_code, locstatus, ER_read);
			ret_val = DI_BADREAD;
			break;
		}

	}

	if (f->io_open_flags & DI_O_LOG_FILE_MASK)
	    Cs_srv_block.cs_wtstatistics.cs_lior_time
		+= CS_checktime() - start_time;
	else
	    Cs_srv_block.cs_wtstatistics.cs_dior_time
		+= CS_checktime() - start_time;

	scb->cs_memory &= ~(CS_DIOR_MASK | CS_LIOR_MASK);
	scb->cs_state = CS_COMPUTABLE;

        CS_synch_lock( &f->io_sem );
        f->io_refcnt--;
        CS_synch_unlock( &f->io_sem );

	Di_reads++;  /* increment counter */

        if ( bytes_read )
                *n = bytes_read / f->io_bytes_per_page;

	return ret_val;
}
