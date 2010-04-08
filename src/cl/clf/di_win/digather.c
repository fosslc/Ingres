/*
** Copyright (c) 1999, 2003 Ingres Corporation
*/
        
# include   <compat.h>
# include   <gl.h>
# include   <cs.h>
# include   <di.h>
# include   <er.h>
# include   <me.h>
# include   <tm.h>
# include   <tr.h>
# include   <csinternal.h>
# include   "dilru.h"

# include   "digather.h"

/*
** Name: digather.c - Functions used to control DI gatherWrite I/O
**
** Description:
**
**      This file contains the following external routines:
**
**	DIgather_setting()      -  Return sizeof(GIO) if GW is enabled
**	DIgather_write()        -  Buffer up/write out write I/O requests
**
**	Additionally, it contains the following static functions:
**
**	gather_list		- Batch up write requests
**	do_writegather		- perform WriteFileGather() call
**	force_list		- force all outstanding writes to disc
**
** History:
**	05-nov-1999 (somsa01)
**	    Created from the UNIX version.
**	11-oct-2000 (somsa01)
**	    Deleted cs_dio, cs_dio_done, cs_dio_wait, cs_dio_time stats,
**	    keep KB written stat.
**	13-nov-2000 (somsa01)
**	    In do_writegather(), cast NumEvents to an i4 before comparing
**	    it to zero.
**	30-apr-2001 (somsa01)
**	    Added io_count, pointer to caller-supplied stat where physical
**	    I/O counts are collected, and gw_pages, pointer to
**	    caller-supplied stat where count of pages written by
**	    WriteFileGather() (not single-page writes) is collected.
**	22-may-2001 (somsa01)
**	    Re-implemented previous change to not count the number of
**	    regular writes that could have been done, and to only count
**	    the number of multi-page GatherWrite's.
**	03-jul-2002 (somsa01)
**	    Cleaned up variable types in do_writegather().
**	19-sep-2003 (somsa01)
**	    In do_writegather(), use io_bytes_per_page rather than the size
**	    of gio_len to decide whether we can perform a WriteFileGather()
**	    or not, and fixed up asynchronous operations. Also, use the
**	    GatherWrite file handle for all operations.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
*/

/* Globals */
GLOBALREF i4		Di_gather_io;
GLOBALREF i4		Di_writes;
GLOBALREF CS_SYSTEM	Cs_srv_block;

static DI_TGIO 	*GWthreads = (DI_TGIO *)NULL;

static i4	check_list();
static STATUS	do_writegather( DI_TGIO *tgio, CL_ERR_DESC *err );
static VOID	gio_sort( DI_GIO *gio_array[], i4 lo, i4 hi );
static STATUS	force_list( CL_ERR_DESC *err_code );
static STATUS	gather_list( DI_GIO *gio, i4 *uqueued, CL_ERR_DESC *err_code);

extern STATUS	DI_sense(DI_IO *, i4 *, CL_SYS_ERR *);

static CS_SEMAPHORE GWthreadsSem ZERO_FILL;
static bool GWSemsInit = FALSE;

/*
**  Forward and/or External typedef/struct references.
*/

/*{
** Name: DIgather_write	- GatherWrite external interface.
**
** Description:
**	This routine is called by any thread that wants to write multiple pages 
**	The caller indicates via the op parameter whether the write request 
**	should be batched up or/and the batch list should be written to disc.
**
** Inputs:
**	i4 op 		- indicates what operation to perform:
**				DI_QUEUE_LISTIO - adds request to threads 
**						  gatherwrite queue.
**				DI_FORCE_LISTIO - causes do_writegather() to
**						  be called for queued GIOs.
**				DI_CHECK_LISTIO - checks if this thread has
**						  any outstanding I/O requests.
**      DI_IO *f        - Pointer to the DI file context needed to do I/O.
**      i4    *n        - Pointer to value indicating number of pages to  
**                        write.                                          
**      i4     page     - Value indicating page(s) to write.              
**      char *buf       - Pointer to page(s) to write.                    
**	(evcomp)()	- Ptr to callers completion handler.
**      PTR closure     - Ptr to closure details used by evcomp.
**                                                                           
** Outputs:                                                                  
**      f               - Updates the file control block.                 
**      n               - Pointer to value indicating number of pages written.
**      err_code        - Pointer to a variable used to return operating system
**                        errors.                                         
**
**      Returns:
**          OK                         	Function completed normally. 
**          non-OK status               Function completed abnormally
**					with a DI error number.
**
**	Exceptions:
**	    none
**
** Side Effects:
**      The completion handler (evcomp) will do unspecified work. 
**
** History:
**	05-nov-1999 (somsa01)
**	    Created.
**	15-Mar-2010 (smeke01) b119529
**	    DM0P_BSTAT fields bs_gw_pages and bs_gw_io are now u_i8s.
*/
STATUS
DIgather_write( i4 op, char *gio_p, DI_IO *f, i4 *n, i4 page, char *buf, 
  VOID (*evcomp)(), PTR closure, i4 *uqueued, u_i8 *gw_pages, u_i8 *io_count,
  CL_ERR_DESC *err_code)
{
    STATUS	ret_val = OK;
    DI_GIO	*gio = (DI_GIO *)gio_p;
    i4		num_of_pages;
    i4		last_page_to_write;
    i4		eof;

    if ( GWSemsInit == FALSE )
    {
	CSw_semaphore( &GWthreadsSem, CS_SEM_SINGLE, "DI GatherWrite sem" );
	GWSemsInit = TRUE;
    }

    /* default returns */
    if (err_code)
	CLEAR_ERR(err_code);

    if ( op & DI_QUEUE_LISTIO )
    {
        num_of_pages = *n;
	*n = 0;
        last_page_to_write = page + num_of_pages - 1;

	/*
	** Check file control block pointer, return if bad.
	*/
        if (f->io_type != DI_IO_ASCII_ID)
            return(DI_BADFILE);

	/*
	** lock DI_IO structure
	*/
	CS_synch_lock(&f->io_sem);

	if ( f->io_nt_gw_fh == INVALID_HANDLE_VALUE )
	    ret_val = DIlru_open( f, FALSE, err_code );
	if ( ret_val != OK )
	{
	    CS_synch_unlock( &f->io_sem );
	    return (ret_val);
	}
	f->io_refcnt++;

	/* 
	** if trying to write past eof, maybe another server has extended it,
	** so update eof and check again
	*/
	if ( last_page_to_write > f->io_system_eof )
	{
	    /*
	    ** There may be pending writes which haven't been forced
	    ** which would extend the file to a new eof.
	    ** Before sensing, make sure they have been forced.
	    */
	    if ( check_list() )
	    {
		CS_synch_unlock(&f->io_sem);
		ret_val = force_list( err_code );
		CS_synch_lock(&f->io_sem);
	    }

	    if ( ret_val == OK )
		ret_val = DI_sense(f, &eof, err_code);

	    if ( ret_val != OK )
	    {
		f->io_refcnt--;
		CS_synch_unlock(&f->io_sem);
		return(DI_ENDFILE);
	    }

	    f->io_system_eof = max(f->io_alloc_eof, eof);

	    if (last_page_to_write > f->io_system_eof)
	    {
		f->io_refcnt--;
		CS_synch_unlock(&f->io_sem);
		return(DI_ENDFILE);
	    }
	}

	/*
	** Unlock DI_IO struct. io_refcnt will be decremented (by the
	** number of buffers) when we actually write out the buffers.
	*/
	CS_synch_unlock(&f->io_sem);

	gio->gio_evcomp = evcomp;
	gio->gio_data = closure;
	gio->gio_f = f;
	gio->gio_buf = buf;
	gio->gio_offset = (OFFSET_TYPE)Int32x32To64(f->io_bytes_per_page, page);
	gio->gio_len = f->io_bytes_per_page * num_of_pages;
	gio->gio_gw_pages = gw_pages;
	gio->gio_io_count = io_count;

	ret_val = gather_list( gio, uqueued, err_code );

	if ( ret_val == OK )
	    *n = num_of_pages;
    }
    else if (op & DI_FORCE_LISTIO )
    {
        return(force_list( err_code ));
        /* Won't return here until all requests have completed */
    }
    else if (op & DI_CHECK_LISTIO )
    {
	return(check_list());
    }

    return( ret_val );
}

/*{
** Name: DIgather_setting -  returns the gather write setting.
**
** Description:
**	If Di_gather_io is TRUE, returns sizeof(DI_GIO),
**	otherwise returns zero.
**
** Inputs:
**	None.
**      
** Outputs:
**      None.
**
** Returns:
**      sizeof(DI_GIO) or zero.
** 
** Exceptions:
**      none
**
** Side Effects:
**      None.
**
** History:
**	19-May-1999 (jenjo02)
**	    Created.
*/
i4
DIgather_setting()
{
    if ( Di_gather_io )
	return( sizeof(DI_GIO) );
    else
	return(0);
}

/*{
** Name: gather_list -  Gather write requests together.
**
** Description:
**	This routine batches up write requests for later submission via     
**	the WriteFileGather() routine.
**
** Inputs:
**	DI_GIO * gio	  - gio Control block for write operation.
**      
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_MEREQMEN_ERR       - MEreqmem failed.
**    Exceptions:
**        none
**
** Side Effects:
**        Will call do_writev if number of write requests has reached
**        GIO_MAX_QUEUED.
**
** History:
**	19-May-1999 (jenjo02)
**	    Created.
**	09-Jul-1999 (jenjo02)
**	    Watch I/O queue as it's being constructed. If pre-ordered,
**	    skip the quicksort.
**	24-sep-2003 (somsa01)
**	    Renamed WaitArray[] to AsynchIOWaitArray[]. Also, use the
**	    GatherWrite file handle for all operations.
*/
static STATUS
gather_list( DI_GIO *gio, i4 *uqueued, CL_ERR_DESC *err_code)
{
    DI_TGIO     *tgio;
    STATUS 	status = OK;
    CS_SCB	*scb;

    CSget_scb(&scb);

    if ( (tgio = (DI_TGIO *)scb->cs_ditgiop) == (DI_TGIO *)NULL ||
	  tgio->tgio_scb != scb )
    {
	/*
	** No TGIO for this thread, so reuse an inactive one
	** or allocate a new one.
	*/
	CSp_semaphore( TRUE, &GWthreadsSem );

	for ( tgio = GWthreads; 
	      tgio && tgio->tgio_state != TGIO_INACTIVE;
	      tgio = tgio->tgio_next );
    
	if (tgio == NULL)
	{
	    SECURITY_ATTRIBUTES	sa;
	    i4			i;

	    tgio = (DI_TGIO *)MEreqmem(0, sizeof(DI_TGIO), TRUE, NULL);
	    if (tgio == NULL)
	    {
		CSv_semaphore( &GWthreadsSem );
		return( DI_MEREQMEM_ERR);
	    }
	    tgio->tgio_next = GWthreads;
	    GWthreads = tgio;

	    /*
	    ** Set up the event handle for this tgio
	    */
	    iimksec(&sa);
	    for (i=0; i<MAXIMUM_WAIT_OBJECTS; i++)
	    {
		tgio->AsynchIOWaitArray[i] =
					CreateEvent(&sa, FALSE, FALSE, NULL);
	    }
	}

	scb->cs_ditgiop = (PTR)tgio;
	tgio->tgio_scb = scb;
	tgio->tgio_uqueued = uqueued;
	*tgio->tgio_uqueued = tgio->tgio_queued = 0;

	tgio->tgio_state = TGIO_ACTIVE;

	CSv_semaphore( &GWthreadsSem );
    }

    /* If the queue is full, force the writes */
    if ( tgio->tgio_queued == GIO_MAX_QUEUED )
	status = do_writegather( tgio, err_code );

    /*
    ** Check for out of sequence GIO.
    ** If all I/O's are presented in file handle/offset order,
    ** a sort won't be needed.
    */
    if ( (tgio->tgio_state & TGIO_UNORDERED) == 0 && tgio->tgio_queued )
    {
	DI_GIO	*pgio = tgio->tgio_queue[tgio->tgio_queued - 1];
	
	if ( gio->gio_f->io_nt_gw_fh < pgio->gio_f->io_nt_gw_fh ||
	    (gio->gio_f->io_nt_gw_fh == pgio->gio_f->io_nt_gw_fh &&
	     gio->gio_offset < pgio->gio_offset) )
	{
	    tgio->tgio_state |= TGIO_UNORDERED;
	}
    }

    /* Add this request to the queue */
    tgio->tgio_queue[tgio->tgio_queued++] = gio;
    
    /* Update caller's queued count */
    *tgio->tgio_uqueued = tgio->tgio_queued;

    return( status );
}

/*{
** Name: gio_sort -  Perform Quicksort of GIO queue.
**
** Description:
**	Sorts the unordered GIO queue into (file handle,offset) order
**	using Quicksort algorithm in preparation for WriteFileGather()
**	collection.
**
**	Credit for the code goes to 
**	"Handbook of Algorithms and Data Structures", by
**	Gaston H. Gonnet & Ricardo Baeza-Yates.
**
** Inputs:
**	DI_GIO	 *gio_array[]	The current partition of tgio->tgio_queued
**				to be sorted.
**	i4	 *lo		The low bound of the partition
**	i4	 *hi		The high bound of the partition
**      
** Outputs:
**      none
**
**    Returns:
**      none
**         
**    Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
**	19-May-1999 (jenjo02)
**	    Created.
*/
static VOID
gio_sort( DI_GIO *gio_array[], 
register i4 lo, 
register i4 hi )
{
    register i4		i, j;
    DI_GIO		*gio;

    while ( hi > lo )
    {
	i = lo;
	j = hi;
	gio = gio_array[lo];

	/* Split array into two partitions */
	while ( i < j )
	{
	    for ( ; 
		  gio_array[j]->gio_f->io_nt_gw_fh > gio->gio_f->io_nt_gw_fh ||
		  (gio_array[j]->gio_f->io_nt_gw_fh == gio->gio_f->io_nt_gw_fh &&
		   gio_array[j]->gio_offset > gio->gio_offset);
		  j-- );
	    for ( gio_array[i] = gio_array[j];
		  i<j && (gio_array[i]->gio_f->io_nt_gw_fh < gio->gio_f->io_nt_gw_fh ||
			 (gio_array[i]->gio_f->io_nt_gw_fh == gio->gio_f->io_nt_gw_fh &&
			  gio_array[i]->gio_offset < gio->gio_offset));
		  i++ );
	    gio_array[j] = gio_array[i];
	}
	gio_array[i] = gio;

	/* Sort recursively, the smallest partition first */
	if ( i - lo < hi - i )
	{
	    if ( i-1 > lo )
		gio_sort( gio_array, lo, i-1 );
	    lo = i+1;
	}
	else
	{
	    if ( hi > i+1 )
		gio_sort( gio_array, i+1, hi );
	    hi = i-1;
	}
    }
    return;
}

/*{
** Name: do_writegather -  Perform WriteFileGather() call.
**
** Description:
**	This function collects the queued write requests, 
**	chooses the optimum function to perform the write(s),
**	and invokes the completion handler for each request.
**
** Inputs:
**	DI_TGIO * tgio  - Control block for current thread. 
**      
** Outputs:
**    None.
**
** Returns:
**    OK
**    FAIL - One of more of the write requests failed.
**
**    Exceptions:
**        none
**
** Side Effects:
**	  The completion handler for each I/O request is invoked.
**
** History:
**	05-nov-1999 (somsa01)
**	    Created.
**	13-nov-2000 (somsa01)
**	    Cast NumEvents to an i4 before comparing it to zero.
**	30-apr-2001 (somsa01)
**	    Increment first gio's io_count stat for each physical I/O,
**	    gw_pages for multi-page writes.
**	22-may-2001 (somsa01)
**	    Re-implemented previous change to not count the number of
**	    regular writes that could have been done, and to only count
**	    the number of multi-page GatherWrite's.
**	03-jul-2002 (somsa01)
**	    Cleaned up variable types.
**	19-sep-2003 (somsa01)
**	    Use io_bytes_per_page rather than the size of gio_len to decide
**	    whether we can perform a WriteFileGather() or not, and fixed up
**	    asynchronous operations. Also, use the GatherWrite file handle
**	    for all operations.
*/
static STATUS
do_writegather( DI_TGIO * tgio, CL_ERR_DESC *err_code )
{
    STATUS 			ret_val = OK, ret_val2 = OK;
    i4 				i, j, k, gio_index, num_gios;
    DI_GIO 			*gio;
    DI_IO			*f;
    OFFSET_TYPE 		next_offset, lseek_offset;
    OFFSET_TYPE			bytes_to_write, bytes_written;
    OFFSET_TYPE			buf_left, buf_copied;
    LARGE_INTEGER		chptr_offset;
    FILE_SEGMENT_ELEMENT	SegmentArray[GIO_MAX_QUEUED+1];
    i4				num_writegather = 0, num_write = 0;
    i4				num_writegather_gio = 0, num_write_gio = 0;
    DWORD			locstatus, NumEvents = -1;
    OVERLAPPED			Overlapped;
    i4				saved_state, start_time;

    /* If unordered, sort the queued list into file handle,offset order */
    if ( tgio->tgio_state & TGIO_UNORDERED )
    {
	gio_sort( tgio->tgio_queue, 0, tgio->tgio_queued-1 );
	tgio->tgio_state &= ~(TGIO_UNORDERED);
    }

    /*
    ** Method:
    **
    **	Collect requests by file handle/offset into a
    **  FILE_SEGMENT_ELEMENT array until the next file offset becomes
    **  discontiguous. Buffers passed to WriteFileGather() must be the
    **  size of a system memory page (ME_MPAGESIZE) and must be aligned
    **  on a system memory page boundary.
    **
    **  If we cannot use the WriteFileGather() function, then coalecse the
    **  requests on contiguous buffer addresses.
    **
    **  A FILE_SEGMENT_ELEMENT pointer is a 64-bit value. The final element
    **  of the array is a 64-bit NULL pointer.
    **
    **  Up to GIO_MAX_QUEUED FILE_SEGMENT_ELEMENT buffers can be written
    **  by a single WriteFileGather(). This is not an OS restriction, but
    **  rather a restriction that we have imposed since we do not want to
    **  lock out an excessive amount of buffers during the GatherWrite
    **  procedure. Note that the actual max amount of buffers inversely
    **  scales by a power of two as the pagesize increases; thus, for 8K
    **  buffers the max is actually GIO_MAX_QUEUED/(8192/ME_MPAGESIZE).
    **
    **	If but a single FILE_SEGMENT_ELEMENT results, the
    **  probably-more-efficient function WriteFile() is called instead
    **  of WriteFileGather().
    **
    */
    j = k = gio_index = 0;

    while (k < tgio->tgio_queued)
    {
	/*
	** "i" indexes the current FILE_SEGMENT_ELEMENT element
	** "gio_index" is the first GIO used in this FILE_SEGMENT_ELEMENT array
	** "k" is the current GIO in the queue
	*/
	i = num_gios = 0;

	gio = tgio->tgio_queue[k];
	f = gio->gio_f;
	lseek_offset = next_offset = gio->gio_offset;
	ret_val2 = OK;

	if (f->io_open_flags & DI_O_OSYNC_MASK &&
	    f->io_bytes_per_page >= ME_MPAGESIZE)
	{
	    do
	    {
		buf_left = gio->gio_len;
		buf_copied = 0;
		while (buf_left && i < GIO_MAX_QUEUED)
		{
		    SegmentArray[i].Alignment = (ULONGLONG)0;
		    SegmentArray[i++].Buffer =
				(char *)gio->gio_buf + buf_copied;
		    buf_left -= ME_MPAGESIZE;
		    buf_copied += ME_MPAGESIZE;
		}

		if (i == GIO_MAX_QUEUED)
		    break;

		next_offset += gio->gio_len;
		num_gios++;

	    } while (++k < tgio->tgio_queued
		     && (gio = tgio->tgio_queue[k])
		     && gio->gio_f->io_nt_gw_fh == f->io_nt_gw_fh
		     && gio->gio_offset == next_offset);
	}
	else
	{
	    /*
	    ** Since we cannot use the WriteFileGather() function, let's
	    ** at least coalesce the requests on the buffer addresses which
	    ** are contiguous for the single WriteFile() operation which
	    ** will occur.
	    */
	    SegmentArray[i].Alignment = (ULONGLONG)0;
	    SegmentArray[i++].Buffer = gio->gio_buf;

	    do
	    {
		next_offset += gio->gio_len;
		num_gios++;

	    } while (++k < tgio->tgio_queued
		     && (gio = tgio->tgio_queue[k])
		     && gio->gio_f->io_nt_gw_fh == f->io_nt_gw_fh
		     && gio->gio_offset == next_offset
		     && (char *)SegmentArray[0].Buffer +
			(next_offset - lseek_offset) == gio->gio_buf);
	}

	/* "k" indexes the next, unprocessed GIO */

	bytes_to_write = next_offset - lseek_offset;

	/* Count a single I/O write for server */
	Di_writes++;
	tgio->tgio_scb->cs_diow++;
	Cs_srv_block.cs_wtstatistics.cs_diow_done++;

	/* Count a single I/O wait for server */
	Cs_srv_block.cs_wtstatistics.cs_diow_waits++;

	/* Accumulate number of KB written by this I/O */
	Cs_srv_block.cs_wtstatistics.cs_diow_kbytes +=
						(u_i4)(bytes_to_write / 1024);

#ifdef xDEBUG
	{
	    i4	x;
	    i4	offset = lseek_offset;
	    for (x = 0; x < i; x++)
	    {
	TRdisplay("%@ %x do_writegather: %t, todo %d, file handle %x, i=%d, SegmentArray[%d], pages %d-%d, base %x, len %d\n",
		    tgio->tgio_scb->cs_self, 
		    f->io_l_filename, f->io_filename,
		    tgio->tgio_queued - j,
		    f->io_nt_gw_fh, i,x, offset / f->io_bytes_per_page, 
		    ((offset + ME_MPAGESIZE) / f->io_bytes_per_page) - 1,
		    SegmentArray[x].Buffer, ME_MPAGESIZE);

		offset += ME_MPAGESIZE;
	    }
	}
#endif /* xDEBUG */

	/*
	** Set up the overlapped structure.
	*/
	chptr_offset.QuadPart = lseek_offset;
	Overlapped.hEvent = tgio->AsynchIOWaitArray[++NumEvents];
	Overlapped.Offset = chptr_offset.LowPart;
	Overlapped.OffsetHigh = chptr_offset.HighPart;

	/*
	** If more than one FILE_SEGMENT_ELEMENT, use WriteFileGather().
	*/
	if (i > 1)
	{
	    /*
	    ** Add the 64-bit NULL terminator in the
	    ** FILE_SEGMENT_ELEMENT array.
	    */
	    SegmentArray[i].Alignment = (ULONGLONG)0;

	    num_writegather++;
	    num_writegather_gio += k - gio_index;

	    /*
	    ** Accumulate multi-page write stats.
	    ** Using the first gio, count
	    ** the number of multi-pages written (k - j)
	    ** and a single I/O.
	    */
	    gio = tgio->tgio_queue[j];
	    if (gio->gio_io_count)
		++*gio->gio_io_count;
	    if (gio->gio_gw_pages)
		*gio->gio_gw_pages += k - j;

	    saved_state = tgio->tgio_scb->cs_state;
	    tgio->tgio_scb->cs_state = CS_EVENT_WAIT;
	    tgio->tgio_scb->cs_memory |= CS_DIOW_MASK;
	    start_time = CS_checktime();

	    ret_val2 = WriteFileGather(f->io_nt_gw_fh,
				       SegmentArray,
				       (DWORD)bytes_to_write,
				       NULL,
				       &Overlapped);
	}
	else
	{
	    num_write++;
	    num_write_gio += k - gio_index;

	    saved_state = tgio->tgio_scb->cs_state;
	    tgio->tgio_scb->cs_state = CS_EVENT_WAIT;
	    tgio->tgio_scb->cs_memory |= CS_DIOW_MASK;
	    start_time = CS_checktime();

	    ret_val2 = WriteFile(f->io_nt_gw_fh,
				 SegmentArray[0].Buffer,
				 (DWORD)bytes_to_write,
				 (LPDWORD)&bytes_written,
				 &Overlapped);
	}

	if (ret_val2)
	{
	    if (!i && (bytes_to_write != bytes_written))
	    {
		SETWIN32ERR(err_code, GetLastError(), ER_write);
		ret_val2 = DI_BADWRITE;
	    }
	    else
		ret_val2 = OK;
	}
	else
	{
	    locstatus = GetLastError();

	    switch(locstatus)
	    {
		case ERROR_IO_PENDING:
		    break;

		case OK:
		    locstatus = GetOverlappedResult(
					f->io_nt_gw_fh,
					&Overlapped,
					(LPDWORD)&bytes_written,
					TRUE);

		    if (locstatus == FALSE)
		    {
			locstatus = GetLastError();
			SETWIN32ERR(err_code, locstatus, ER_write);
			if (locstatus == ERROR_DISK_FULL)
			    ret_val2 = DI_NODISKSPACE;
			else
			    ret_val2 = DI_BADWRITE;
		    }
		    else
		    {
			if (bytes_to_write != bytes_written)
			{
			    SETWIN32ERR(err_code, GetLastError(), ER_write);
			    ret_val2 = DI_BADWRITE;
			}
		    }
		    break;

		default:
		    SETWIN32ERR(err_code, locstatus, ER_write);
		    if (locstatus == ERROR_DISK_FULL)
			ret_val2 = DI_NODISKSPACE;
		    else
			ret_val2 = DI_BADWRITE;
	    }
	}

	Cs_srv_block.cs_wtstatistics.cs_diow_time
	    += CS_checktime() - start_time;
	tgio->tgio_scb->cs_memory &= ~(CS_DIOW_MASK);
	tgio->tgio_scb->cs_state = saved_state;

	/* Preserve the worst status from all the writes */
	ret_val = (ret_val) ? ret_val : ret_val2;

	CS_synch_lock( &f->io_sem );
	f->io_refcnt -= num_gios;
	CS_synch_unlock( &f->io_sem );

	gio_index = k;

	/*
	** If we've reached our limit for number of asynchronous I/O waits,
	** let's wait for what is pending to complete before moving on.
	*/
	if (NumEvents == MAXIMUM_WAIT_OBJECTS - 1)
	{
	    WaitForMultipleObjects( NumEvents+1, tgio->AsynchIOWaitArray, TRUE,
				    INFINITE );

	    NumEvents = -1;

	    do 
	    {
		gio = tgio->tgio_queue[j];

		(gio->gio_evcomp)( gio->gio_data, ret_val2, &err_code );

	    } while (++j < k);
	}
    }

    /*
    ** Now, run the completion routine for each GIO written.
    */
    if ((i4)NumEvents >= 0)
    {
	WaitForMultipleObjects( NumEvents+1, tgio->AsynchIOWaitArray, TRUE,
			 	INFINITE );

	do 
	{
	    gio = tgio->tgio_queue[j];

	    (gio->gio_evcomp)( gio->gio_data, ret_val, &err_code );

	} while (++j < k);
    }

#ifdef xDEBUG
    TRdisplay("%@ %x do_writegather: %d write requests completed using %d(%d) WriteFileGather, %d(%d) write\n",
		tgio->tgio_scb->cs_self, 
		tgio->tgio_queued, 
		num_writegather, num_writegather_gio,
		num_write, num_write_gio);
#endif /* xDEBUG */

    /* Clear the queued count(s) */
    tgio->tgio_queued = *tgio->tgio_uqueued = 0;

    return(ret_val);
}

/*{
** Name: force_list -  Force all outstanding writes to disc.
**
** Description:
**	This routine attempts to write all outstanding requests.
**
** Inputs:
**	none
**      
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
** Returns:
**      STATUS		     The outcome of the write requests.
** Exceptions:
**      none
**
** Side Effects:
**      None.
**
** History:
**	05-nov-1999 (somsa01)
**	    Created.
*/
static STATUS
force_list( CL_ERR_DESC *err_code )
{
    DI_TGIO 	*tgio;
    STATUS 	ret_val = OK;
    CS_SCB 	*scb;

    CSget_scb(&scb);

    if ( (tgio = (DI_TGIO *)scb->cs_ditgiop) &&
	  tgio->tgio_scb == scb &&
	  tgio->tgio_queued )
    {
	/* Write all remaining on queue */
	ret_val = do_writegather( tgio, err_code );

	/* Clear the thread's TGIO pointer, make it available to others */
	tgio->tgio_scb->cs_ditgiop = (PTR)NULL;
	tgio->tgio_state = TGIO_INACTIVE;
    }

    return( ret_val );
}

/*{
** Name: check_list	 -  Return the number of outstanding
**			    I/O's on this thread.
**
** Description:
**	Checks for oustanding I/O's on this thread.
**
** Inputs:
**	None.
**      
** Outputs:
**      None.
**
** Returns:
**      i4 		   Number of outstanding I/O's.
**
** Exceptions:
**      none
**
** Side Effects:
**      None.
**
** History:
**	19-May-1999 (jenjo02)
**	    Created.
*/
static i4
check_list()
{
    DI_TGIO	*tgio;
    CS_SCB	*scb;

    CSget_scb(&scb);

    if ( (tgio = (DI_TGIO *)scb->cs_ditgiop) &&
	  tgio->tgio_scb == scb )
    {
	return(tgio->tgio_queued);
    }
    return(0);
}
