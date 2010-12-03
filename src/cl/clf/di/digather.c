/*
**Copyright (c) 2004 Ingres Corporation
*/
        
# include   <compat.h>
# include   <gl.h>
# include   <clconfig.h>
# include   <systypes.h>
# include   <errno.h>
# include   <cs.h>
#ifndef VMS
# include   <csev.h>
# include   <cssminfo.h>
#endif
# include   <di.h>
# include   <me.h>
# include   <tr.h>
#ifndef VMS
# include   <dislave.h>
# include   "dilocal.h"
# include   "dilru.h"
#endif
# include   <er.h>

#ifndef VMS
# include   "digather.h"
#endif

#ifdef usl_us5
#define IOV_MAX 16
#endif

#ifdef dgi_us5
#define IOV_MAX MAXIOVCNT
#endif

# ifdef OSX
# define IOV_MAX 1024
# endif
/**
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
**	do_writev		- perform writev call
**	force_list		- force all outstanding writes to disc
**
** History:
**	11-May-1999 (jenjo02)
**	    Created from diasync.c
**	09-Jul-1999 (jenjo02)
**	    Watch I/O queue as it's being constructed. If pre-ordered,
**	    skip the quicksort.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	29-Sep-2000 (jenjo02)
**	    Deleted cs_dio, cs_dio_done, cs_dio_wait, cs_dio_time stats,
**	    keep KB written stat.
**	09-Apr-2001 (jenjo02)
**	    Added io_count, pointer to caller-supplied stat where
**	    physical I/O counts are collected, and gw_pages,
**	    pointer to caller-supplied stat where count of pages
**	    written by writev (not single-page writes) is collected.
**      28-nov-2001 (stial01)
**          Fix lseek offset calculation for LARGEFILE64 (b106470)
**      18-mar-2002 (xu$we01)
**          Provided definition for MAXIOV on dgi_us5.
**	01-nov-2002 (somsa01)
**	    Changed xCL_DIRECT_IO to xCL_NO_ATOMIC_READ_WRITE_IO.
**	22-Apr-2005 (hanje04)
**	    Define IOV_MAX for mac_osx.
** 	    TODO: derive this parameter
**	    Cannot quite figure this out on OSX.  The sysconf manual page 
**	    describes the standard _SC_IOV_MAX parameter but it does not 
**	    exist in the unistd.h header. Probably something that should be 
**	    reported to Apple.
**	    Add prototypes for gather_list and force_list to quiet compiler
**	    warnings.
**	25-Jan-2006 (jenjo02)
**	    Delay opening FDs until ready to write(v) to keep from
**	    contributing to ALL_PINNED errors.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**      01-nov-2010 (joea)
**          Merge with VMS version.
*/
/* Globals */
GLOBALREF	i4	Di_gather_io;	/* GLOBALDEF in dilru.c */

#ifndef VMS
static DI_TGIO 	*GWthreads = (DI_TGIO *)NULL;

static i4	 check_list();
static STATUS	 do_writev( DI_TGIO *tgio, CL_ERR_DESC *err );
static VOID	 gio_sort( DI_GIO *gio_array[], i4 lo, i4 hi );
static STATUS    force_list( CL_ERR_DESC *err_code );
static STATUS	 gather_list( DI_GIO *gio, i4 *uqueued, CL_ERR_DESC *err_code);

static CS_SYNCH GWthreadsSem ZERO_FILL;
static bool GWSemsInit = FALSE;
#endif

static i4	iov_max;

#define	DEBUG_THIS_PUPPY	
#undef	DEBUG_THIS_PUPPY	

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
**				DI_FORCE_LISTIO - causes do_writev() to be 
**						  called for queued GIOs.
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
**	19-May-1999 (jenjo02)
**	    Created.
**	20-jul-1999 (popri01)
**	    On Unixware (usl_us5), IOV MAX can only be determined
**	    at run-time, so use a hard-coded value of 16 (which
**	    guarantees portability).
**	    Also, add explicit cast for iov_base arithmetic.
**	03-Apr-2002 (bonro01)
**	    SGI is also missing IOV_MAX.  Re-wrote the routine to 
**	    determine IOV_MAX at run-time for SGI and Unixware.
**      26-Nov-2002 (inifa01)
**	    Crossed in and Ammended above change to fix compile
**	    problems.
**	09-Jan-2003 (bonro01)
**	    Unixware compiler could not generate variable size local
**	    structure for IOV_MAX.
**	25-Aug-2005 (schka24)
**	    Don't lru-open the file here, do it when we're going to
**	    really queue the request.
**	30-Sep-2005 (jenjo02)
**	    GWthreadsSem now a CS_SYNCH object.
**	15-Mar-2010 (smeke01) b119529
**	    DM0P_BSTAT fields bs_gw_pages and bs_gw_io are now u_i8s.
*/
STATUS
DIgather_write( i4 op, char *gio_p, DI_IO *f, i4 *n, i4 page, char *buf, 
  VOID (*evcomp)(), PTR closure, i4 *uqueued, u_i8 *gw_pages, u_i8 *io_count, 
  CL_ERR_DESC *err_code)
{
    STATUS	big_status = OK, small_status = OK;

#ifdef OS_THREADS_USED

    DI_GIO		*gio = (DI_GIO *)gio_p;
    i4			num_of_pages;
    i4			last_page_to_write;
    CL_ERR_DESC		lerr_code;

    if ( GWSemsInit == FALSE )
    {
	CS_synch_init( &GWthreadsSem );
	GWSemsInit = TRUE;
    }

    /* default returns */
    if (err_code)
	CL_CLEAR_ERR( err_code );

    if ( op & DI_QUEUE_LISTIO )
    {
        num_of_pages = *n;
        *n = 0;
        last_page_to_write = page + num_of_pages - 1;

        if (f->io_type != DI_IO_ASCII_ID)
            return(DI_BADFILE);

        if (f->io_mode != DI_IO_WRITE)
            return(DI_BADWRITE);

	/* 
	** now check for write within bounds of the file 
	*/
	if ( last_page_to_write > f->io_alloc_eof )
	{
	    i4	real_eof;

	    /*
	    ** There may be pending writes which haven't been forced
	    ** which would extend the file to a new eof.
	    ** Before sensing, make sure they have been forced.
	    */
	    if ( check_list() )
		big_status = force_list( err_code );

	    /*
	    ** DIsense updates f->io_alloc_eof with the protection
	    ** of io_sem (OS_THREADS), so there's no need to
	    ** duplicate that update here.
	    */
	    if ( big_status == OK )
		big_status = DIsense(f, &real_eof, err_code);

	    if ( big_status == OK && last_page_to_write > f->io_alloc_eof )
	    {
		small_status = DI_ENDFILE;
		SETCLERR(err_code, 0, ER_write);

		/*
		** The above sets errno as errno will be left over from
		** a previous call zero it out to avoid confusion.
		*/
		err_code->errnum = 0;
	    }
	}

	if ( big_status == OK && small_status == OK )
	{
	    /* Record another write */
	    f->io_stat.write++;
	    gio->gio_evcomp = evcomp;
	    gio->gio_data = closure;
	    gio->gio_f = f;
	    gio->gio_buf = buf;
	    gio->gio_offset = (OFFSET_TYPE)f->io_bytes_per_page * (OFFSET_TYPE)page;
	    gio->gio_len = f->io_bytes_per_page * num_of_pages;
	    gio->gio_gw_pages = gw_pages;
	    gio->gio_io_count = io_count;

	    big_status = gather_list( gio, uqueued, err_code );
	}

	if ( big_status == OK && small_status == OK )
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

#endif /* OS_THREADS_USED */

    return( big_status ? big_status : small_status );
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
#if !defined(VMS)
    if ( Di_gather_io )
	return( sizeof(DI_GIO) );
    else
#endif
	return(0);
}

#ifdef OS_THREADS_USED

/*{
** Name: gather_list -  Gather write requests together.
**
** Description:
**	This routine batches up write requests for later submission via     
**	the writev() routine.
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
**	25-Aug-2005 (schka24)
**	    Don't blindly lru-open each request;  instead, see if the
**	    file (DI_IO) is already on the queue, and share its fd with the
**	    queued request.  This is essential when doing fd-per-thread,
**	    as each call to lru-open would allocate a new fd, thus negating
**	    the ability to do a writev!  When not doing fd-per-thread,
**	    this change is effectively a no-op.
**	    Also, return without queueing if queue-flush fails.
**	25-Jan-2006 (jenjo02)
**	    Defer lru-open until do_writev to prevent hogging FDs
**	    while waiting for futhur writes.
*/
static STATUS
gather_list( DI_GIO *gio, i4 *uqueued, CL_ERR_DESC *err_code)
{
    DI_GIO	*qgio;			/* a GIO on the queue already */
    DI_OP	*diop;
    DI_TGIO     *tgio;
    STATUS 	status = OK;
    CS_SCB	*scb;
    i4		i;

    CSget_scb(&scb);

    if ( (tgio = (DI_TGIO *)scb->cs_ditgiop) == (DI_TGIO *)NULL ||
	  tgio->tgio_scb != scb )
    {
	/*
	** No TGIO for this thread, so reuse an inactive one
	** or allocate a new one.
	*/
	CS_synch_lock( &GWthreadsSem );

	for ( tgio = GWthreads; 
	      tgio && tgio->tgio_state != TGIO_INACTIVE;
	      tgio = tgio->tgio_next );
    
	if (tgio == NULL)
	{
	    tgio = (DI_TGIO *)MEreqmem(0,
		    sizeof( DI_TGIO ),
		      TRUE, NULL);
	    if (tgio == NULL)
	    {
		CS_synch_unlock( &GWthreadsSem );
		return( DI_MEREQMEM_ERR);
	    }
	    tgio->tgio_next = GWthreads;
	    GWthreads = tgio;
	}

	scb->cs_ditgiop = (PTR)tgio;
	tgio->tgio_scb = scb;
	tgio->tgio_uqueued = uqueued;
	*tgio->tgio_uqueued = tgio->tgio_queued = 0;

	tgio->tgio_state = TGIO_ACTIVE;

	CS_synch_unlock( &GWthreadsSem );
    }

    /* If the queue is full, force the writes.
    ** If this fails, we get blamed, but someone has to report it.
    */
    if ( tgio->tgio_queued == GIO_MAX_QUEUED )
    {
	status = do_writev( tgio, err_code );
	if (status != OK)
	    return (status);
    }

    /*
    ** Check for out of sequence GIO.
    ** If all I/O's are presented in file/offset order,
    ** a sort won't be needed.
    */
    if ( (tgio->tgio_state & TGIO_UNORDERED) == 0 && tgio->tgio_queued )
    {
	qgio = tgio->tgio_queue[tgio->tgio_queued - 1];
	
	if ( gio->gio_f < qgio->gio_f ||
	    (gio->gio_f == qgio->gio_f &&
	     gio->gio_offset < qgio->gio_offset) )
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
**	Sorts the unordered GIO queue into (f,offset) order
**	using Quicksort algorithm in preparation for writev
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
**	25-Jan-2006 (jenjo02)
**	    Sort rather by file,offset, as FDs won't be known until
**	    we're ready to do the write.
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
		  gio_array[j]->gio_f > gio->gio_f ||
		  (gio_array[j]->gio_f == gio->gio_f &&
		   gio_array[j]->gio_offset > gio->gio_offset);
		  j-- );
	    for ( gio_array[i] = gio_array[j];
		  i < j && (gio_array[i]->gio_f < gio->gio_f ||
			    (gio_array[i]->gio_f == gio->gio_f &&
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
** Name: do_writev -  Perform writev() call.
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
**	19-May-1999 (jenjo02)
**	    Created.
**	09-Jul-1999 (jenjo02)
**	    If queued list is ordered, skip the quicksort.
**	09-Apr-2001 (jenjo02)
**	    Increment first gio's io_count stat for each physical I/O,
**	    gw_pages for multi-page writes.
**	05-Nov-2002 (jenjo02)
**	    Cleaned up use of io_sem: only write() and writev() need
**	    the mutex to protect the (separate) seek. pwrite(64)
**	    atomically seeks and does not need the mutex.
**	25-Aug-2005 (schka24)
**	    Don't bother with IO timing, too slow on some platforms (Linux)
**	    and the results aren't all that interesting.
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
**	24-Jan-2006 (jenjo02)
**	    Break on change in file ("f"), then lru-open to get an
**	    FD, do the write(v), and lru_release the FD. This keeps
**	    gather_write from hogging FDs while waiting for the
**	    signal to actually do something.
**	15-Mar-2006 (jenjo02)
**	    f->io_sem not needed if running with thread affinity,
**	    the fd is not shared by multiple threads.
*/
static STATUS
do_writev( DI_TGIO * tgio, CL_ERR_DESC *err_code )
{
    CL_ERR_DESC lerr_code;
    STATUS 	big_status = OK, small_status = OK;
    i4 		i, j, k;
    DI_GIO 	*gio, *first_gio;
    DI_IO	*f;
    DI_OP	*diop;
    OFFSET_TYPE next_offset, lseek_offset;
    i4		bytes_to_write, bytes_written;
    i4		saved_state;

    i4		num_writev = 0, num_write = 0;
    i4		num_writev_gio = 0, num_write_gio = 0;

#if defined(sgi_us5)
    if( iov_max == 0 )
    {
	iov_max = sysconf(_SC_IOV_MAX);
	if( iov_max <= 0 )
        {
	    iov_max = 16;	/* arbitrary minimum value */
#ifdef DEBUG_THIS_PUPPY
	    TRdisplay("%@ %x do_writev: %t ERROR sysconf failed with %d\n",
		    tgio->tgio_scb->cs_self, 
		    f->io_l_filename, f->io_filename,
		    iov_max);
#endif /* DEBUG_THIS_PUPPY */
        }
        else if( iov_max > 2048 )
        {
	    iov_max = 2048;	/* arbitrary maximum value */
        }
    }
#else
    iov_max = IOV_MAX;
#endif

    /* If unordered, sort the queued list into file,offset order */
    if ( tgio->tgio_state & TGIO_UNORDERED )
    {
	gio_sort( tgio->tgio_queue, 0, tgio->tgio_queued-1 );
	tgio->tgio_state &= ~(TGIO_UNORDERED);
    }


    /*
    ** Method:
    **
    **	Collect requests by file/offset into an iovec until
    **	the next file offset becomes discontiguous. Additionally, if
    **	the buffer addresses are contiguous, colaesce those requests.
    **
    **  Up to IOV_MAX iovecs can be written by a single writev().
    **
    **	If but a single iovec results, the probably-more-efficient
    **	function (p)write() is called instead of writev().
    */
    k = 0;

    while ( (j = k) < tgio->tgio_queued )
    {
	#if defined(sgi_us5)
		struct iovec iov[iov_max];
	#else
		struct iovec iov[IOV_MAX];
	#endif

	/*
	** "i" indexes the current iovec element
	** "j" is the first GIO used in this iovec array
	** "k" is the current GIO in the queue
	*/
	i = 0;
	
	gio = first_gio = tgio->tgio_queue[j];
	f = gio->gio_f;
	lseek_offset = next_offset = gio->gio_offset;
	small_status = OK;

	iov[0].iov_base = gio->gio_buf;
	iov[0].iov_len  = 0;

	do
	{
	    /* If this buffer is contiguous with previous, coalesce it */
	    if ( (char *)iov[i].iov_base + iov[i].iov_len == gio->gio_buf )
	    {
		iov[i].iov_len += gio->gio_len;
	    }
	    /* Initialize next iovec if any remain */
	    else if ( i < iov_max - 1 )
	    {
		i++;
		iov[i].iov_base = gio->gio_buf;
		iov[i].iov_len  = gio->gio_len;
	    }
	    else
		break;

	    next_offset += gio->gio_len;

	} while ( ++k < tgio->tgio_queued
		    && (gio = tgio->tgio_queue[k])
		    && gio->gio_f == f
		    && gio->gio_offset == next_offset );

	/* "k" indexes the next, unprocessed GIO */

	bytes_to_write = next_offset - lseek_offset;
	
	saved_state = tgio->tgio_scb->cs_state;
	tgio->tgio_scb->cs_state = CS_EVENT_WAIT;
	tgio->tgio_scb->cs_memory = CS_DIOW_MASK;

	/* Accumulate multi-page write stats */
	if ( k - j > 1 )
	{
	    /*
	    ** Using the first gio, count
	    ** the number of multi-pages written (k-j)
	    ** and a single I/O.
	    */
	    if ( first_gio->gio_io_count )
		++*first_gio->gio_io_count;
	    if ( first_gio->gio_gw_pages )
		*first_gio->gio_gw_pages += k - j;
	}

	/* Count a single I/O write for server */
	tgio->tgio_scb->cs_diow++;
	Cs_srv_block.cs_wtstatistics.cs_diow_done++;

	/* Count a single I/O wait for server */
	Cs_srv_block.cs_wtstatistics.cs_diow_waits++;

	/* Accumulate number of KB written by this I/O */
	Cs_srv_block.cs_wtstatistics.cs_diow_kbytes
	    += bytes_to_write / 1024;
	
	/* Now get an FD to do the write(v) */
	diop = (DI_OP*)&first_gio->gio_diop;
	if ( big_status = DIlru_open(f, FALSE, diop, err_code) )
	    return(big_status);

#ifdef DEBUG_THIS_PUPPY
	{
	    i4	x;
	    i8	offset = lseek_offset;

	    TRdisplay("%@ %p do_writev: %~t doing %d todo %d fd %d lseek from %ld\n",
		    tgio->tgio_scb->cs_self, 
		    f->io_l_filename, f->io_filename,
		    i+1, tgio->tgio_queued - j,
		    diop->di_fd, offset);
	    for (x = 0; x <= i; x++)
	    {
	TRdisplay("%@ do_writev: iovec[%d] base %p bytes %d (page %d for %d)\n",
			x,
			iov[x].iov_base, iov[x].iov_len,
			(i4)(offset/f->io_bytes_per_page),
			iov[x].iov_len/f->io_bytes_per_page);
		offset += iov[x].iov_len;
	    }
	}
#endif /* DEBUG_THIS_PUPPY */

	/* If more than one iovec, seek and use writev */
	if ( i++ )
	{
	    /* writev needs seek mutex protection */
	    if ( !Di_thread_affinity )
		CS_synch_lock( &f->io_sem );
	    
	    num_writev++;
	    num_writev_gio += k - j;

	    bytes_written = 
		IIdio_writev( diop->di_fd, 
				(char *)iov,
				i,
				lseek_offset, 0, 
				f->io_fprop,
				err_code);
	    if ( !Di_thread_affinity )
		CS_synch_unlock( &f->io_sem );
	}
	else
	{
	    num_write++;
	    num_write_gio += k - j;

# if  !defined(xCL_NO_ATOMIC_READ_WRITE_IO)
	    /* pwrite(64) needs no seek mutex protection */
	    bytes_written =
#ifdef LARGEFILE64
	     pwrite64( diop->di_fd, 
			iov[0].iov_base, 
			bytes_to_write, 
			lseek_offset );
#else /*  LARGEFILE64 */
	     pwrite( diop->di_fd,
			iov[0].iov_base, 
			bytes_to_write, 
			lseek_offset );
#endif /* LARGEFILE64 */
	    if (bytes_written != bytes_to_write)
		SETCLERR(err_code, 0, ER_write);
# else /* !xCL_NO_ATOMIC_READ_WRITE_IO */
	    /* write() needs seek mutex protection */
	    if ( !Di_thread_affinity )
		CS_synch_lock( &f->io_sem );

	    bytes_written =
	     IIdio_write( diop->di_fd,
			    iov[0].iov_base, 
			    bytes_to_write, 
			    lseek_offset, 0, 
			    f->io_fprop,
			    err_code );
	    if ( !Di_thread_affinity )
		CS_synch_unlock( &f->io_sem );

# endif /* !xCL_NO_ATOMIC_READ_WRITE_IO */
	}

	/* Release the FD */
	(VOID)DIlru_release( diop, &lerr_code );
	    
	tgio->tgio_scb->cs_memory &= ~(CS_DIOW_MASK);
	tgio->tgio_scb->cs_state = saved_state;

	if (bytes_written != bytes_to_write)
	{
	    switch ( err_code->errnum )
	    {
		case EFBIG:
		    small_status = DI_BADEXTEND;
		    break;
		case ENOSPC:
		    small_status = DI_EXCEED_LIMIT;
		    break;
#ifdef EDQUOTA
		case EDQUOT:
		    small_status = DI_EXCEED_LIMIT;
		    break;
#endif
		default:
		    if (err_code->errnum == 0)
			small_status = DI_ENDFILE;
		    else
			small_status = DI_BADWRITE;
		    break;
	    }
	    /* Preserve the worst status from all the writes */
	    big_status = (big_status) ? big_status : small_status;
	}

	/* Invoke completion handler for each GIO written */
	do 
	{
	    gio = tgio->tgio_queue[j];
	    (gio->gio_evcomp)( gio->gio_data, small_status, err_code );

	} while ( ++j < k );
    }

#ifdef DEBUG_THIS_PUPPY
    TRdisplay("%@ %p do_writev: %d write requests completed using %d(%d) writev, %d(%d) write\n",
		tgio->tgio_scb->cs_self, 
		tgio->tgio_queued, 
		num_writev, num_writev_gio,
		num_write, num_write_gio);
#endif /* DEBUG_THIS_PUPPY */

    /* Clear the queued count(s) */
    tgio->tgio_queued = *tgio->tgio_uqueued = 0;

    return( big_status );
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
**	19-May-1999 (jenjo02)
**	    Created.
*/
static STATUS
force_list( CL_ERR_DESC *err_code )
{
    DI_TGIO 	*tgio;
    STATUS 	big_status = OK;
    CS_SCB 	*scb;

    CSget_scb(&scb);

    if ( (tgio = (DI_TGIO *)scb->cs_ditgiop) &&
	  tgio->tgio_scb == scb &&
	  tgio->tgio_queued )
    {
	/* Write all remaining on queue */
	big_status = do_writev( tgio, err_code );

	/* Clear the thread's TGIO pointer, make it available to others */
	tgio->tgio_scb->cs_ditgiop = (PTR)NULL;
	tgio->tgio_state = TGIO_INACTIVE;
    }

    return( big_status );
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
# endif /* OS_THREADS_USED */
