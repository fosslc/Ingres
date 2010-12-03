/*
**Copyright (c) 2004 Ingres Corporation
*/
        
# include   <compat.h>
# include   <gl.h>
# include   <clconfig.h>
# include   <systypes.h>
# include   <errno.h>
# include   <clsigs.h>
# include   <cs.h>
# if defined(sgi_us5)
# include   <fdset.h>
# endif
# include   <csev.h>
# include   <di.h>
# include   <me.h>
# include   <nm.h>
# include   <tr.h>
# include   <dislave.h>
# include   "dilocal.h"
# include   <er.h>
#ifdef xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif /* xCL_006_FCNTL_H_EXISTS */

#if defined (xCL_ASYNC_IO) || defined (OS_THREADS_USED) 
# include   <diasync.h>
# include   <signal.h>
# ifdef OS_THREADS_USED
# include   <sys/stat.h>
# endif /* OS_THREADS_USED */

/* For Cs_srv_blk: */
#if defined(any_hpux)
# include    <csinternal.h>
#endif

/**
** Name: diasync.c - Functions used to control DI asynchronous I/O
**
** Description:
**
**      This file contains the following external routines:
**
**	DI_chk_aio_list()    -  Check for completed I/O's. 
**      DI_aio_inprog()      -  Check if async io in progress.
**	DI_aio_rw()          -  Do async operation on one page. 
**      DI_get_aiocb()       -  get an aiocb from free list.
**
**	Additionally, it contains the following routines, which are 
**	used internally:
**
**	append_aio()		- adds a DI_AIOCB to a list 
**	remove_aio()		- removes a DI_AIOCB from a list
**	DI_free_aio()		- moves a DI_AIOCB from an active list to 
**				  the freelist
**	DI_aio_pkg()		- Initialises a DI_AIOCB control block
**	getalbuf()		- get pointer to alligned i/o buffer
**	dolog()			- sets trace switch if II_DIO_TRACE set
**	nohold()		- sets nohold switch if II_AIO_NOHOLD set
**
** History:
**	21-sep-1995 (itb ICL)
**	    Added code to set and return the value of Di_gather_io.
**	20-oct-1995 (itb ICL)
**	    Lots of code fixes and rewrote the list handling functions
**	    to improve efficiency.  Added DI_check_list.
**	03-jun-1996 (canor01)
**	    For Solaris, which does not yet have support for Posix
**	    asynchronous i/o, add i/o threads implementation when
**	    operating-system threads are supported.
**	11-nov-1996 (hanch04)
**	    Added sui_us5 for aio
**	29-jan-1997 (hanch04)
**	    ifdef file for platforms without aio
**	24-feb-1997 (walro03)
**	    Must include fdset.h since protypes being included reference fdset.
**  29-Apr-1997 (merja01)
**      The axp_osf aiocb does not contain an aio_errno or aio_return.  
**      However, there are aio_error( aiocb *) and aio_return(aiocb *)
**      functions to extract these values.  I #ifdefed the code to use 
**      these functions where applicable for axp_osf.  
**      In addition, DI_aio_pkg initializes aio_flag, aio_iovcnt and
**      aio_iov.  These members are not part of the axp_osf aiocb as 
**      well.  Since these values don't seem to be referenced anywhere 
**      else in the code I used #ifdefs for axp_osf to get around the
**      initializations.
**      Removed definition of AIO_LISTIO_MAX.  This defined in 
**      /cl/clf/hdr/diasync.h
**	29-apr-1997 (popri01)
**	    Create a dummy routine to resolve the DIwrite_list loader error in
**	    iimerge.
**	29-apr-1997 (popri01)
**	    Although async i/o is not implemented for Unixware, fix some
**	    definitions to satisfy the compiler.
**	15-sep-1997 (hanch04)
**	    change if defs for axp_osf to be the default and dr6_us5 
**	    is the odd ball.
**	24-feb-1997 (walro03)
**	    Must include fdset.h since protypes being included reference fdset.
**	23-sep-1997 (muhpa01)
**	    Ifdef for hp8_us5 which supports OS_THREADS, but not async i/o,
**	    but does have aio.h present.
**	23-sep-1997 (hanch04)
**	    di_listio is not for OS_THREADS
**	24-sep-1997 (hanch04)
**	    Some OS use OS_THREADS for AIO
**    01-oct-1997 (muhpa01)
**        Add additional ifdef's for hp8_us5 needed to resolve DIwrite_list.
**        Also, reposition last #endif for hp8 which seems to have moved thru
**        previous integrations.  This allows exposure of the dummy version
**        of DIwrite_list for other platforms.
**	06-Oct-1997 (hanch04)
**	    change more if defs for axp_osf to be the default
**	03-Nov-1997 (hanch04)
**	    Change TRdisplay to print errnum, not address of err
**	    Added LARGEFILE64 for files > 2gig
**	03-nov-1997 (canor01)
**	    Add stack address parameter to CS_create_thread.
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Removed GLOBALREFs, changed *aioList to static.
**	09-mar-1998 (musro02)
**	    Added sqs_ptx to platforms needing fdset.h
**	26-Mar-1998 (allmi01)
**	    Added sgi_us5 to list of platforms that need to include fdset.h.
**	08-Jun-1998 (muhpa01)
**	    Add ifdef's for hpb_us5 to enable async I/O - similar to axp
**	    implementation.  Also, added call to initialize the events
**	    system in order to be able to process asyncio signals.
**	    Note: when hpb_us5 is defined, hp8_us5 is as well
**	16-Nov-1998 (jenjo02)
**	    Discriminate CSsuspend type I/O read or write.
**	09-Mar-1999 (schte01)
**       Changed newspace->next assignment so there is no dependency on 
**       order of evaluation (in DI_get_aiocb routine).
**	29-Jun-1999 (jenjo02)
**	    Relocated DIgather_setting() to new module digather.c, a
**	    non-async-dependent implementation.
**    21-Oct-1999 (johco01)
**       Define aio_errno and aio_return to use the structure aio_result_p
**       on Solaris Intel (sui_us5) when redefining these macros. Because
**       aio__error and aio__return are not defiend as members of the aiocb
**       structure on this platform.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**      08-Dec-1999 (podni01)  
**	 Put back all POSIX_DRAFT_4 related changes; replaced POSIX_DRAFT_4
**	 with DCE_THREADS to reflect the fact that Siemens (rux_us5) needs 
**	 this code for some reason removed all over the place.
**	28-feb-2000 (somsa01)
**	    stat64() should also be used when LARGEFILE64 is defined.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-nov-2001 (stial01)
**          Fix lseek offset calculation for LARGEFILE64 (b106470)
**	07-may-2002 (xu$we01)
**	    Fix compilier warnings.
**	12-nov-2002 (somsa01)
**	    On HP-UX, pass cs_stkcache to CS_create_thread().
**      03-Dec-2002 (hanje04)
**          The Linux pre compiler doesn't like the #ifdef
**          being used to call the CS_create_thread macro with a extra 
**          parameter for HP. Add a completely separate call as for DCE_THREAD
**	14-Oct-2005 (jenjo02)
**	    Deprecated Sequent-specific code.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	10-Nov-2010 (kschendel) SIR 124685
**	    Delete unused DIwrite_list.  Prototype fixes.
*/

/* Globals */

/*
** Allocated table, non-zero if file is raw.
*/
static char *rawfd = NULL;
# if defined(thr_hpux)
static int events_init = FALSE;
# endif


static DI_AIOCB *aioList=(DI_AIOCB *)NULL;
static DI_AIOCB *aioFreeList=(DI_AIOCB *)NULL;
static DI_LISTIOCB *listioThreads=(DI_LISTIOCB *)NULL;
# ifdef OS_THREADS_USED
static CS_SEMAPHORE listioThreadsSem ZERO_FILL;
static CS_SEMAPHORE aioListSem ZERO_FILL;
static CS_SEMAPHORE aioFreeListSem ZERO_FILL;
static bool listioSemsInit = FALSE;
# endif /* OS_THREADS_USED */

# define IO_ALIGN       512
# define AIO_ALLOC      32
# define AIO_RETRIES    10

# if !defined(LARGEFILE64) && defined(sparc_sol)

# define aio_errno	aio__error
# define aio_return	aio__return

# endif /* LARGEFILE64 */

# if defined(usl_us5)
/* 
** Some platforms have different names for the error and return fields
** These should really not be referenced by name at all, only
** by function call.
*/
# define aio_errno    aio__error
# define aio_return   aio__return

# endif /* usl_us5 */


# if defined(OS_THREADS_USED) && !defined(xCL_ASYNC_IO)
/* structure to pass params to async i/o read/write thread */
struct _II_DIO_RWPARMS
{
    int op;
    int fd;
    char *buf;
    int len;
    OFFSET_TYPE off;
    OFFSET_TYPE *loffp;
    CL_ERR_DESC *err;
};
typedef struct _II_DIO_RWPARMS II_DIO_RWPARMS;

/* structure to pass params to async i/o open thread */
struct _II_DIO_OPARMS
{
    char *file;
    int mode;
    int perms;
    CL_ERR_DESC *err;
};
typedef struct _II_DIO_OPARMS II_DIO_OPARMS;

# endif /* OS_THREADS_USED */

/*
**  Forward and/or External typedef/struct references.
*/
static char *getalbuf( i4  size, CL_ERR_DESC *err );


/*{
** Name: fisraw() -- is the fd pointing to a raw device?
**
** Description:
**      Tells whether the fd opened by a previous call to IIdio_open
**      was to a raw device.
**
** Inputs:
**      fd                      -- file desscriptor of interest.
**
** Outputs:
**      returns TRUE or FALSE.
**
** Side Effects:
**      Examines the symbol "II_DIO_TRACE", and will only return true
**      if is set and non-null.
**
** History:
**      12-jul-89 (daveb)
**              Created.
*/
static i4  
fisraw( i4  fd )
{
    return( rawfd && rawfd[fd] );
}



/*
** List handling functions
*/
static VOID
append_aio( DI_AIOCB **whichList, DI_AIOCB *aio)
{
    DI_AIOCB *workList;

    if ( *whichList != (DI_AIOCB *)NULL)
    {
	workList=*whichList;
        while( workList->next != (DI_AIOCB *)NULL)
            workList=workList->next;
        workList->next=aio;
	aio->prev=workList;
    }
    else
    {
        *whichList = aio;
	aio->prev=(DI_AIOCB *)NULL;
    }
    aio->next=(DI_AIOCB *)NULL;
}
static VOID
remove_aio( DI_AIOCB **whichList, DI_AIOCB *aio)
{
    if (aio->prev == (DI_AIOCB *)NULL)
    {
	/*
	** This is the first entry in the list.  So need
	** to change the root list pointer.
	*/
	*whichList=aio->next;
    }
    else
    {
	/*
	** Standard removal code.
	*/
	aio->prev->next = aio->next;
    }

    if ( aio->next != (DI_AIOCB *)NULL)
	aio->next->prev=aio->prev;
}

/*{
** Name: DI_free_aio - Clean up DI_AIOCB structures.
**
** Description:
**    This routine removes an DI_AIOCB from aioList to the free list. 
**
** Inputs:
**	aiocb - Pointer to DI_AIOCB structure to be placed on free-list.
**
** Output:
**	none.
**
**      Returns:
**          None.
**
**	Exceptions:
**	    none
**
** Side Effects:
**      None.
**
** History:
**	01-mar-1995 (anb)
**          Created.
*/
static VOID
DI_free_aio( DI_AIOCB *aiocb)
{
# ifdef OS_THREADS_USED
    CSp_semaphore( TRUE, &aioListSem );
# endif /* OS_THREADS_USED */

    remove_aio( &aioList, aiocb);
# ifdef OS_THREADS_USED
    CSv_semaphore( &aioListSem );
    CSp_semaphore( TRUE, &aioFreeListSem );
# endif /* OS_THREADS_USED */
    aiocb->next = aioFreeList;
    aiocb->prev = (DI_AIOCB *)NULL;
    aioFreeList = aiocb;
# ifdef OS_THREADS_USED
    CSv_semaphore( &aioFreeListSem );
# endif /* OS_THREADS_USED */
}

/*{
** Name: DI_get_aiocb - Allocate a DI_AIOCB structure.
**
** Description:
**    This routine returns a pointer to a DI_AIOCB structure.
**
**    The routine allocates from a free list, if the list is empty, then
**    it uses MEreqmem to allocate more space (in block of 32).
**
** Inputs:
**	none.
**
** Output:
**          None.
**
** Returns:
**	aiocprev - Pointer to DI_AIOCB structure.
**	NULL	 - memory allocation failure
**
** Exceptions:
**	    none
**
** Side Effects:
**      None.
**
** History:
**	01-mar-1995 (anb)
**          Created.
*/
DI_AIOCB *
DI_get_aiocb( VOID)
{
    DI_AIOCB *newspace = (DI_AIOCB *)NULL;

# ifdef OS_THREADS_USED
    if ( listioSemsInit == FALSE )
    {
	CSw_semaphore( &listioThreadsSem, CS_SEM_SINGLE, "DI listio sem" );
	CSw_semaphore( &aioListSem, CS_SEM_SINGLE, "DI aiolist sem" );
	CSw_semaphore( &aioFreeListSem, CS_SEM_SINGLE, "DI aiofreelist sem" );
    }
    CSp_semaphore( TRUE, &aioFreeListSem );
# endif /* OS_THREADS_USED */

    if ( aioFreeList == (DI_AIOCB *)NULL )
    {
        /* Allocate more space for free list */
        i4  cnt=0;

        newspace = (DI_AIOCB *)MEreqmem(0,
		((sizeof( DI_AIOCB ))*AIO_ALLOC), TRUE, NULL);
        if ( !newspace )
        {
	    return( (DI_AIOCB*) NULL);
        }
        aioFreeList = newspace;
        for( cnt=1;cnt<AIO_ALLOC;cnt++)
		 {
            (DI_AIOCB *)++newspace;
            newspace->next = (DI_AIOCB *)newspace;
		  }
        newspace->next = (DI_AIOCB *)NULL;
    }
    /* Remove DI_AIOCB from start of free list */
    newspace = aioFreeList;
    aioFreeList = newspace->next;
# ifdef OS_THREADS_USED
    CSv_semaphore( &aioFreeListSem );
# endif /* OS_THREADS_USED */

    return( newspace);
}

/*{
** Name: DI_chk_aio_list - Check for completed I/O's. 
**
** Description:
**    This routine checks its list of DI_AIOCB's to see if any have been
**    marked as finished. If so then, the corresponding threads event ctrl
**    block is marked as ready for resumption.
**
** Inputs:
**	none.
**
** Output:
**	none.
**
**      Returns:
**          OK                         	    Function completed normally. 
**          non-OK status                   Function completed abnormally.
**
**	Exceptions:
**	    none
**
** Side Effects:
**      Marks thread(s) as ready for resumption.
**
** History:
**	01-mar-1995 (anb)
**          Created.
**	20-oct-1995 (itb)
**	    Only resume a gather write thread when everythings done.
**	29-Apr-1997 (merja01)
**      Added axp_osf ifdef to account for aiocb differences.
**      
*/
STATUS
DI_chk_aio_list(void)
{
#ifdef xCL_ASYNC_IO
    DI_AIOCB *iolist_ptr;
    DI_LISTIOCB *threadptr;

# ifdef OS_THREADS_USED
    CSp_semaphore( TRUE, &aioListSem );
# endif /* OS_THREADS_USED */
    iolist_ptr = aioList;
    while ( iolist_ptr != NULL)
    {
#ifdef LARGEFILE64
        if ((aio_error64(&iolist_ptr->aio)) != EINPROGRESS )
#else /* LARGEFILE64 */
        if ((aio_error(&iolist_ptr->aio)) != EINPROGRESS )
#endif /* LARGEFILE64 */
        {
            if ( iolist_ptr->resumed != TRUE )
            {
		/*
		** If this thread is doing gather write then we
		** only resume if we are forcing the I/O's out and
		** there are no other outstanding I/O's.
		** We will always mark the I/O as resumed.
		*/

		iolist_ptr->resumed = TRUE;

		if ( Di_gather_io )
		{
# ifdef OS_THREADS_USED
		    CSp_semaphore( TRUE, &listioThreadsSem );
# endif /* OS_THREADS_USED */
		    threadptr = listioThreads;

		    while ( threadptr != (DI_LISTIOCB *)0 )
		    {
			if ( threadptr->sid == iolist_ptr->sid )
			    break;
			threadptr = threadptr->next;
		    }

		    if (threadptr == NULL || threadptr->state == AIOLIST_INACTIVE)
			/* Single aio being done on this thread */
			CSresume(iolist_ptr->sid);
		    else
		    {
			threadptr->outstanding_aio -= 1;
			if ((threadptr->state == AIOLIST_FORCE) &&
			    (threadptr->outstanding_aio == 0))
			    CSresume(iolist_ptr->sid);
		    }
# ifdef OS_THREADS_USED
		    CSv_semaphore( &listioThreadsSem );
# endif /* OS_THREADS_USED */
		}
                else
		    /* Always resume if not doing gather write */
		    CSresume(iolist_ptr->sid);
            }
        } /* endif EINPROG */
        iolist_ptr=iolist_ptr->next;
    } /* endwhile */
# ifdef OS_THREADS_USED
    CSv_semaphore( &aioListSem );
# endif /* OS_THREADS_USED */
#endif /* xCL_ASYNC_IO */
    return(OK);
}

/*{
** Name: DI_aio_inprog - Check if async io in progress.
**
** Description:
**
**	Called when closing a file (implicitly or explicitly), if async io
**	is being used.
**
**	Searches the DI_AIOCB list for the requested file and checks whether
**	EINPROGRESS  is set.  
**
** Inputs:
**	fd - unix file descriptor for file to be closed
**
** Outputs:
**	none
**
** Returns:
**	FAIL - EINPROGRESS set for file
**	OK   - EINPROGRESS not set or file no longer in list
**
** History:
**	18oct95 (amo ICL)
**		created.
**	29-Apr-1997 (merja01)
**      Added axp_osf ifdef to account for aiocb differences.
*/

STATUS
DI_aio_inprog(i4 fd)
{
#ifdef XCL_ASYNC_IO
	DI_AIOCB *tptr;

# ifdef OS_THREADS_USED
        CSp_semaphore( TRUE, &aioListSem );
# endif /* OS_THREADS_USED */
	tptr = aioList;
	while (tptr != NULL)
	{
	   if (tptr->aio.aio_fildes == fd) &&
	      ((aio_error(&tptr->aio)) == EINPROGRESS)
	   {
		return(FAIL);
	   }

	   tptr=tptr->next;
	} /* endwhile */
# ifdef OS_THREADS_USED
        CSv_semaphore( &aioListSem );
# endif /* OS_THREADS_USED */
#endif /* XCL_ASYNC_IO */
	return(OK);
}


/*{
** Name: DI_aio_pkg - Initialise a DI_AIOCB control block.
**
** Description:
**    This routine initialises an DI_AIOCB structure.
**    If necessary memory is allocated via MEreqmem. 
**
** Inputs:
**	i4	fd	- file descriptor.
**      char *  iobuf   - aligned buffer to write from/read to.
**      i4      len	- length of data.
**      long	off     - file offset.
**
** Output:
**	none.
**
** Returns:
**      DI_AIOCB *  - Ptr to DI_AIOCB structure
**	null	    - memory allocation failure
**
** Exceptions:
**	none
**
** Side Effects:
**      None.
** History:
**	01-mar-1995 (anb)
**          Created.
**	29-Apr-1997 (merja01)
**      Added axp_osf ifdef to account for aiocb differences.
*/
static DI_AIOCB *
DI_aio_pkg( i4  fd, char *iobuf, i4  len, OFFSET_TYPE off)
{
#ifdef xCL_ASYNC_IO 
    CS_SID sid;
    DI_AIOCB * newptr;

    newptr = DI_get_aiocb();
    if ( newptr)
    {
        CSget_sid( &sid);
        newptr->sid = sid;
        newptr->resumed = FALSE;
        newptr->data = NULL;
# if defined(thr_hpux)
	newptr->aio.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
# endif
        newptr->aio.aio_sigevent.sigev_signo = SIGUSR2;
        newptr->aio.aio_reqprio = 0;

	newptr->aio.aio_fildes = fd;  /* assign fd for axp_osf */
        newptr->aio.aio_buf = iobuf;
        newptr->aio.aio_nbytes = len;
        newptr->aio.aio_offset = off;
    }
    return( newptr);
#endif /* xCL_ASYNC_IO */
}

/*{
** Name: DI_aio_rw - POSIX asynchronous I/O interface.
**
** Description:
**	This routine is called by any UNIX server that wants to use real 
**	asynchronous I/O as opposed to UNIX slave processes.
**
** Inputs:
**      i4  op	          - The type of operation - O_WRONLY for write and 
**                          O_RDONLY for read.
**      DI_OP *diop	  - The dilru file control block.
**      char * buf	  - The buffer from which to write from or to read into.
**      i4  len	          - The length of buf.
**      long off	  - The seek offset.
**      long *loffp	  - A pointer to the known current offset ( or NULL).
**      CL_ERR_DESC * err - A CL_ERR_DESC to fill on error.
**                                                                           
** Outputs:                                                                  
**      long *loffp	  - Updated offset value.
**
**      Returns:
**          n     actual number of bytes transferred.
**          -1    Function completed abnormally.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	None.
**
** History:
**	01-mar-1995 (anb)
**          Created.
**	29-Apr-1997 (merja01)
**      Added axp_osf ifdef to account for aiocb differences.
**	03-Nov-1997 (hanch04)
**	    Change TRdisplay to print errnum, not address of err
*/
i4
DI_aio_rw(
  i4  op,
  DI_OP *diop,
  char * buf,
  i4  len,
  OFFSET_TYPE off,
  OFFSET_TYPE *loffp,
  CL_ERR_DESC *err)
{
    i4  actual = -1;

#ifdef xCL_ASYNC_IO
    char *iobuf = buf;
    i4  errcode;
    char *opmsg;
    DI_AIOCB * aio;
    i4	cnt;
    i4	fd=diop->di_fd;
    STATUS status=OK;
    STATUS aiostatus=OK;

# if defined(thr_hpux)
    /* 
    ** For hpb_us5, the events system needs to be initialized to be able
    ** to process asyncio signals.
    */
    if ( events_init == FALSE )
    {
	events_init = TRUE;
	CS_event_init( 1, 0 );
    }
# endif

    if ( err )
	CL_CLEAR_ERR( err );

    /*
    ** If alignment is required, then copy data buffer to an aligned
    ** buffer for the io request.
    **
    ** Buffer alignment is only required on raw devices.
    */
    /* lint truncation warning if size of ptr > int, but code valid */
    if (fisraw((i4)fd) && ((i4)buf & (IO_ALIGN - 1)) )
    {
    	if ( !(iobuf = getalbuf( len, err )) )
	{
		return -1;
	}
	MEcopy( buf, len, iobuf );
    }

    aio = DI_aio_pkg( fd, iobuf, len, off);
    if ( !aio)
    {
        return( -1);
    }

# ifdef OS_THREADS_USED
    CSp_semaphore( TRUE, &aioListSem );
# endif /* OS_THREADS_USED */
    /* Add to end of outstanding I/O list */
    append_aio( &aioList, aio);
# ifdef OS_THREADS_USED
    CSv_semaphore( &aioListSem );
# endif /* OS_THREADS_USED */

    if ( op == O_RDONLY )
    {
        errcode = ER_aioread;
        opmsg = "aioread";
    }
    else
    {
        errcode = ER_aiowrite;
        opmsg = "aiowrite";
    }

    for ( cnt=1 ; cnt <= AIO_RETRIES ; cnt++ )
    {
	if ( op == O_RDONLY )
#ifdef LARGEFILE64
	    actual = aio_read64( &aio->aio );
#else /* LARGEFILE64 */
	    actual = aio_read( &aio->aio );
#endif /*  LARGEFILE64 */
	else
#ifdef LARGEFILE64
	    actual = aio_write64( &aio->aio );
#else /* LARGEFILE64 */
	    actual = aio_write( &aio->aio );
#endif /*  LARGEFILE64 */
#ifdef LARGEFILE64
	if ( actual >= 0 || ((aio_error64(&aio->aio)) != EAGAIN) || cnt == AIO_RETRIES )
#else /* LARGEFILE64 */
	if ( actual >= 0 || ((aio_error(&aio->aio)) != EAGAIN) || cnt == AIO_RETRIES )
#endif /* LARGEFILE64 */
	    break;

    }
 
    if ( actual == 0)
    {
#ifdef LARGEFILE64
        while (( aiostatus = aio_error64(&aio->aio) ) == EINPROGRESS)
#else /* LARGEFILE64 */
        while (( aiostatus = aio_error(&aio->aio) ) == EINPROGRESS)
#endif /* LARGEFILE64 */
        {
	    /* nothing */
        }
    }
    if ( aiostatus == OK )
#ifdef LARGEFILE64
	actual = aio_return64(&aio->aio);
#else /* LARGEFILE64 */
	actual = aio_return(&aio->aio);
#endif /* LARGEFILE64 */
    else
    {
	errno = aiostatus;
	actual = -1;
    }

    if ( op == O_RDONLY && actual > 0 && iobuf != buf )
	MEcopy( iobuf, actual, buf );

    if ( actual == -1 )
    {
        if ( err )
	    SETCLERR( err, 0, errcode );

        if ( dolog() )
#ifdef LARGEFILE64
	    TRdisplay("DI_aio_rw: bad %s, fd %d errno %d\n",
				opmsg, fd, aio_error64(&aio->aio) );
#else /* LARGEFILE64 */
	    TRdisplay("DI_aio_rw: bad %s, fd %d errno %d\n",
				opmsg, fd, aio_error(&aio->aio) );
#endif /* LARGEFILE64 */
    }

    if ( dolog() > 1 )
	TRdisplay(
"DI_aio_rw: op %d fd %d buf %p len %d off %d errnum %d\n\tactual %d\n",
		op, fd, buf, len, off, err->errnum, actual );

    DI_free_aio( aio);
#else

    errno=ENOSYS;

#endif /* xCL_ASYNC_IO */
    return( actual );
}


/*{
** Name: getalbuf() -- get pointer to an aligned i/o buffer of a size.
**
** Description:
**	Returns a pointer to an IO_ALIGNed buffer of at least the specifified
**	size.
**
** Inputs:
**	size		- actual size of the buffer you want.
**	err		- CL error description of interest on failure.
**
** Outputs:
**	returns pointer to a buffer, or NULL (with err set) on failure.
**
** Side Effects:
**	May allocate memory, and uses static data.
**
** History:
**	11-jul-89 (daveb)
**		Created.
**	30-November-1992 (rmuth)
**		Prototype.
*/

static
char * 
getalbuf( 
    i4  size,
    CL_ERR_DESC *err )
{
    static char *space = NULL;		/* unaligned space */
    static char *albuf = NULL;		/* aligned ptr into above */
    static i4  spacelen;		/* length of space */
    STATUS memstatus;                   /* dummy parameter for MEreqmem */ 

    /* get bigger buffer if needed */
    if ( !space || spacelen < (size + IO_ALIGN) )
    {
	if ( space )
	    (void) MEfree( space );

	spacelen = size + IO_ALIGN;
	space = MEreqmem( 0, spacelen, 0, &memstatus );
	albuf = ME_ALIGN_MACRO( space, IO_ALIGN );
    }

    return( albuf );
}


/*{
** Name: dolog() -- Should we log dio events?
**
** Description:
**	Tells whether II_DIO_TRACE has turned on logging.
**
** Inputs:
**	none.
**
** Outputs:
**	returns TRUE or FALSE.
**
*
** History:
**	12-jul-89 (daveb)
**		Created.
**	30-November-1992 (rmuth)
*		Prototype.
*/
static i4  dolog( VOID )
{
    static i4  checked = 0;
    static i4  dotrace;

    char *cp;
    if ( !checked )
    {
	checked = 1;
	NMgtAt( "II_DIO_TRACE", &cp );
	if (cp)
	    if (*cp == 'y')
		dotrace = 99;
	    else
		dotrace = atoi(cp);
	else
	    dotrace = 0;
    }
    return( dotrace ); 
}

static i4  nohold( VOID )
{
    static i4  h_checked = 0;
    static i4  dohold;

    char *cp;
    if ( !h_checked )
    {
	h_checked = 1;
	NMgtAt( "II_AIO_NOHOLD", &cp );
	if (cp)
	    if (*cp == 'y')
		dohold = 99;
	    else
		dohold = atoi(cp);
	else
	    dohold = 0;
    }
    return( dohold );
}


# if defined(OS_THREADS_USED) && !defined(xCL_ASYNC_IO)
/*{
** Name: DI_thrp_rw   -  Thread I/O function.
**
** Description:
**      Completes the I/O synchronously (for this thread) and exits.
**
** Inputs:
**      pointer to II_DIO_RWPARMS parameter block that packages the
**	standard I/O parameters into a single parameter, which is
**	all a thread startup function will accept.
**
** Outputs:
**      CL_ERR_DESC * err - filled on error.
**
** Returns:
**      n                  - number of bytes read or written
**     -1                  - error occurred.
**
** Exceptions:
**      none
**
** Side Effects:
**      None.
**
** History:
**    03-jun-1996 (canor01)
**          Created.
**    19-Oct-2006 (hanal04) bug 116895
**          If we have LARGEFILE64 support we need to call pread64() and
**          pwrite64() as pread() and pwrite() will hit errors on files
**          greater than 2GB.
*/
static void *DI_thrp_rw( void *thrparms )
{
    II_DIO_RWPARMS *dioparms = thrparms;
    i4  actual = -1;
    i4  errcode;
    char *opmsg;

        if ( dioparms->op == O_RDONLY )
        {
            errcode = ER_read;
            opmsg = "pread";
#ifdef LARGEFILE64
            actual = pread64( (int)dioparms->fd, dioparms->buf, dioparms->len,
                            dioparms->off );
#else /*  LARGEFILE64 */
            actual = pread( dioparms->fd, dioparms->buf, dioparms->len,
                            dioparms->off );
#endif /* LARGEFILE64 */
        }
        else
        {
            errcode = ER_write;
            opmsg = "pwrite";
#ifdef LARGEFILE64
            actual = pwrite64( (int)dioparms->fd, dioparms->buf, dioparms->len,
                             dioparms->off );
#else /*  LARGEFILE64 */
            actual = pwrite( dioparms->fd, dioparms->buf, dioparms->len,
                             dioparms->off );
#endif /* LARGEFILE64 */
        }

        if ( actual == -1 )
        {
            if ( dioparms->err )
                SETCLERR( dioparms->err, errno, errcode );

            if ( dolog() )
                TRdisplay("IIdio_rw: bad %s, fd %d errno %d\n",
                                opmsg, dioparms->fd, errno );
        }

    if ( dolog() > 1 )
        TRdisplay(
"IIdio_rw: op %d fd %d buf %p len %d off %d loffp %p err %x\n\tactual %d\n",
                dioparms->op, dioparms->fd, dioparms->buf, dioparms->len,
                dioparms->off, dioparms->loffp, dioparms->err, actual );

    CS_exit_thread((void *)(SCALARP)actual);
}

/*{
** Name: DI_thread_rw - Thread asynchronous I/O interface.
**
** Description:
**      This routine is called by any UNIX server that wants to use 
**      asynchronous I/O emulated by I/O threads as opposed to UNIX 
**	slave processes.  Mostly for when Posix async I/O does not exist.
**
**	This function packages the parameters into a single structure
**	that can be passed to the thread startup function, DI_thrp_rw().
**
** Inputs:
**      i4  op            - The type of operation - O_WRONLY for write and
**                          O_RDONLY for read.
**      DI_OP *diop       - The dilru file control block.
**      char * buf        - The buffer from which to write from or to read into.
**      i4  len           - The length of buf.
**      long off          - The seek offset.
**      long *loffp       - A pointer to the known current offset ( or NULL).
**      CL_ERR_DESC * err - A CL_ERR_DESC to fill on error.
**
** Outputs:
**      long *loffp       - Updated offset value.
**
**      Returns:
**          n     actual number of bytes transferred.
**          -1    Function completed abnormally.
**
**      Exceptions:
**          none
**
** Side Effects:
**      None.
**
** History:
**      03-jun-1996 (canor01)
**          Created.
*/
int 
DI_thread_rw(
                int op,
                DI_OP *diop,
                char *buf,
                int len,
                OFFSET_TYPE off,
                OFFSET_TYPE *loffp,
                CL_ERR_DESC *err)
{
    II_DIO_RWPARMS dioparm, *dioparms = &dioparm;
    CS_THREAD_ID tid;
    i4  actual = -1;
    STATUS status;

    /* initialize dio logging */
    (void) dolog();

    /* package parameters into a single structure to pass to thread */
    dioparms->op = op;
    dioparms->fd = diop->di_fd;
    dioparms->buf = buf;
    dioparms->len = len;
    dioparms->off = off;
    dioparms->loffp = loffp;
    dioparms->err = err;

#if defined(DCE_THREADS)
    CS_create_thread( 32768, DI_thrp_rw, dioparms, &tid, CS_JOINABLE, &status );
#elif defined(any_hpux)
    CS_create_thread( 32768, NULL, DI_thrp_rw, dioparms, 
		      &tid, CS_JOINABLE,
		      Cs_srv_block.cs_stkcache,
		      &status );
#else
    CS_create_thread( 32768, NULL, DI_thrp_rw, dioparms,
                      &tid, CS_JOINABLE,
                      &status );
#endif

    if ( status ) 
    {
        if ( err )
            SETCLERR( err, status, 0 );
    }

    /* sleep until i/o completes */
    if ( status == OK )
        CS_join_thread(tid,(void**)&actual);

    return (actual);
}


# endif /* OS_THREADS_USED */

# endif /* xCL_ASYNC_IO || OS_THREADS_USED */ 
