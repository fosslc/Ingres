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
# if defined(ts2_us5) || defined(sgi_us5) || defined(sui_us5)
# include   <fdset.h>
# endif
# include   <csev.h>
# include   <cssminfo.h>
# include   <di.h>
# include   <me.h>
# include   <tr.h>
# include   <dislave.h>
# include   "dilocal.h"
# include   <er.h>
#ifdef xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif /* xCL_006_FCNTL_H_EXISTS */

#if defined (xCL_ASYNC_IO) || defined (OS_THREADS_USED) 
# if !defined(hp8_us5)
# include   <diasync.h>
# include   <signal.h>
# ifdef OS_THREADS_USED
# include   <sys/stat.h>
# endif /* OS_THREADS_USED */

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
**	DIwrite_list()	     -  Buffer up/write out lio_listio requests
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
**	DI_listio_write		- POSIX lio_listio wrapper
**	gather_listio		- Batch up write requests
**	do_listio		- perform lio_listio call
**	force_listio		- force all outstanding writes to disc
**	DI_complete_listio	- lio_listio completion handler for DI
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

# if !defined(LARGEFILE64) && ( defined(sparc_sol) || defined(sui_us5) )

# define aio_errno	aio__error
# define aio_return	aio__return

# endif /* LARGEFILE64 */

# if defined(usl_us5)
/* 
** Some platforms have different names for the error and return fields
** These should really not be referenced by name at all, only
** by function call.
*/
#if defined(sui_us5)
# define aio_errno    aio_resultp.aio_errno
# define aio_return   aio_resultp.aio_return
#else
# define aio_errno    aio__error
# define aio_return   aio__return
#endif

# endif /* usl_us5 */

#ifdef dr6_us5
# define aio_read	aioread
# define aio_write	aiowrite
#endif /* dr6_us5 */

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

static STATUS do_listio( DI_LISTIOCB * currThread);
static STATUS gather_listio( DI_AIOCB *aio, CL_ERR_DESC *err);

# if defined(any_hpux)
GLOBALREF CS_SYSTEM	Cs_srv_block;  /* Overall ctl struct */
# endif  /* HP-UX */


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
# endif /* hp8_us5 */

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
#ifdef dr6_us5
        if ( iolist_ptr->aio.aio_errno != EINPROGRESS )
#else
#ifdef LARGEFILE64
        if ((aio_error64(&iolist_ptr->aio)) != EINPROGRESS )
#else /* LARGEFILE64 */
        if ((aio_error(&iolist_ptr->aio)) != EINPROGRESS )
#endif /* LARGEFILE64 */
#endif  /* dr6_us5 */
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

# if !defined(hp8_us5)
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
#ifdef dr6_us5
#ifndef MINX
	   if (tptr->aio.aio_filedes == fd) &&
#else
	   if (tptr->aio.aio_fildes == fd) &&
#endif /* MINX */
#else
	   if (tptr->aio.aio_fildes == fd) &&
#endif /* dr6_us5
#ifdef dr6_us5 
	      (tptr->aio.aio_errno == EINPROGRESS)
#else
	      ((aio_error(&tptr->aio)) == EINPROGRESS)
#endif  /* dr6_us5 */
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
#ifdef dr6_us5
        if ( nohold() > 1)
            newptr->aio.aio_flag = AIO_NOHOLD;
        else
            newptr->aio.aio_flag = 0;
        newptr->aio.aio_key = (VOID *) NULL;
        newptr->aio.aio_iovcnt = 0;
#ifndef MINX
        newptr->aio.aio_iov = (struct iov *)0;
#else
        newptr->aio.aio_iov = (struct iovec *)0;
#endif /* MINX */
        newptr->aio.aio_errno = EINPROGRESS;
#else  
#endif /* dr6_us5 */

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
#ifdef dr6_us5 
	if ( actual >= 0 || aio->aio.aio_errno != EAGAIN || cnt == AIO_RETRIES )
#else
#ifdef LARGEFILE64
	if ( actual >= 0 || ((aio_error64(&aio->aio)) != EAGAIN) || cnt == AIO_RETRIES )
#else /* LARGEFILE64 */
	if ( actual >= 0 || ((aio_error(&aio->aio)) != EAGAIN) || cnt == AIO_RETRIES )
#endif /* LARGEFILE64 */
#endif /* dr6_us5 */
	    break;

#ifdef dr6_us5
	aiopoll( NULL, 0 );
#endif /* dr6_us5 */
    }
 
    if ( actual == 0)
    {
#ifdef dr6_us5
        while (aio->aio.aio_errno == EINPROGRESS)
#else
#ifdef LARGEFILE64
        while (( aiostatus = aio_error64(&aio->aio) ) == EINPROGRESS)
#else /* LARGEFILE64 */
        while (( aiostatus = aio_error(&aio->aio) ) == EINPROGRESS)
#endif /* LARGEFILE64 */
#endif  /* dr6_us5 */
        {
/*
            if ( (status=CSsuspend( op == O_RDONLY ? CS_DIOR_MASK : CS_DIOW_MASK, 0, 0) ) 
					!= OK)
            {
                DIlru_set_di_error( status, err, DI_LRU_CSSUSPEND_ERR, 
                                    DI_GENERAL_ERR);
		break;
            }
*/
        }
    }
#ifdef dr6_us5
    if (  aio->aio.aio_errno == 0 )
	actual = aio->aio.aio_retval;
#else
    if ( aiostatus == OK )
#ifdef LARGEFILE64
	actual = aio_return64(&aio->aio);
#else /* LARGEFILE64 */
	actual = aio_return(&aio->aio);
#endif /* LARGEFILE64 */
#endif  /* dr6_us5 */
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
#ifdef dr6_us5
	    TRdisplay("DI_aio_rw: bad %s, fd %d errno %d\n",
				opmsg, fd, aio->aio.aio_errno );
#else
#ifdef LARGEFILE64
	    TRdisplay("DI_aio_rw: bad %s, fd %d errno %d\n",
				opmsg, fd, aio_error64(&aio->aio) );
#else /* LARGEFILE64 */
	    TRdisplay("DI_aio_rw: bad %s, fd %d errno %d\n",
				opmsg, fd, aio_error(&aio->aio) );
#endif /* LARGEFILE64 */
#endif  /* dr6_us5 */
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
# endif /* hp8_us5 */

/*{
** Name: DIwrite_list	- POSIX lio_listio external interface.
**
** Description:
**	This routine is called by any thread that wants to write multiple pages 
**	The caller indicates via the op parameter whether the write request 
**	should be batched up or/and the batch list should be written to disc.
**
** Inputs:
**	i4 op 		- indicates what operation to perform:
**				DI_QUEUE_LISTIO - adds request to threads 
**						  listio queue.
**				DI_FORCE_LISTIO - causes lio_listio() to be 
**						  called using listio queue.
**      DI_IO *f        - Pointer to the DI file context needed to do I/O.
**      i4  *n          - Pointer to value indicating number of pages to  
**                        write.                                          
**      i4 page    - Value indicating page(s) to write.              
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
**	01-mar-1995 (anb)
**          Created.
**  29-Apr-1997 (merja01)
**          Fixed call to CSv_semaphore by removing TRUE.  This function
**          releases the semaphore and only takes a single argument.
*/
STATUS
DIwrite_list( i4  op, DI_IO *f, i4  *n, i4 page, char *buf, 
	      STATUS (*evcomp)(), PTR closure, CL_ERR_DESC *err_code)
{
    STATUS			big_status = OK, small_status = OK;

#ifdef xCL_ASYNC_IO
    i4			num_of_pages;
    i4			last_page_to_write;
    CL_ERR_DESC    		lerr_code;
    DI_OP			diop;

    if ( op & DI_QUEUE_LISTIO)
    {
        /* default returns */

	CL_CLEAR_ERR( err_code );

        num_of_pages = *n;
        *n = 0;
        last_page_to_write = page + num_of_pages - 1;

        diop.di_flags = 0;

        if (f->io_type != DI_IO_ASCII_ID)
        {
            return(DI_BADFILE);
        }

        if (f->io_mode != DI_IO_WRITE)
        {
            return(DI_BADWRITE);
        }

        do
        {
	    /* 
	    ** get open file descriptor for the file
	    */
	    big_status = DIlru_open(f, FALSE, &diop, err_code);
	    if ( big_status != OK )
	        break;

	    /* 
	    ** now check for write within bounds of the file 
	    */
	    if (last_page_to_write > f->io_alloc_eof) 
	    {
	        i4	real_eof;

		/*
		** DI_sense updates f->io_alloc_eof with the protection
		** of io_sem (OS_THREADS), so there's no need to
		** duplicate that update here.
		*/
	        big_status = DI_sense(f, &diop, &real_eof, err_code);

	        if ( big_status != OK )
	        {
	           (VOID) DIlru_release(&diop, &lerr_code );
		    break;
	        }

	        if (last_page_to_write > f->io_alloc_eof)
	        {
		    big_status = DIlru_release(&diop, err_code );
		    if ( big_status != OK )
		        break;

		    small_status = DI_ENDFILE;
	            SETCLERR(err_code, 0, ER_write);

		    /*
		    ** The above sets errno as errno will be left over from
		    ** a previous call zero it out to avoid confusion.
		    */
		    err_code->errnum = 0;

		    break;
	        }
	     }


#ifdef xOUT_OF_DISK_SPACE_TEST
	     if ((f->io_open_flags & DI_O_NODISKSPACE_DEBUG) &&
	        (last_page_to_write > f->io_logical_eof)    &&
	        (last_page_to_write <= f->io_alloc_eof))
	     {
	        f->io_open_flags &= ~DI_O_NODISKSPACE_DEBUG;

	        big_status = DIlru_release(&diop, err_code );
	        if ( big_status != OK )
		    break; 

	        small_status = DI_EXCEED_LIMIT;
	        errno = ENOSPC;
	        SETCLERR(err_code, 0, ER_write);

	        TRdisplay(
		"DIwrite_list(): Returning false DI_EXCEED_LIMIT, page %d\n", page);
	        break;
	    }
#endif /* xOUT_OF_DISK_SPACE_TEST */
	    
	    big_status = DI_listio_write( f, &diop, buf, page, num_of_pages,
				         evcomp, closure, err_code );
	    if ( big_status != OK )
	    {
	        (VOID) DIlru_release(&diop, &lerr_code);
	        break;
	    }

	    *n = num_of_pages;

        } while (FALSE);

    }

    if (op & DI_FORCE_LISTIO)
    {
        /* Won't return here until all writes have completed or failed */
        big_status=force_listio( err_code);
    }

    if (op & DI_CHECK_LISTIO)
    {
	return(DI_check_list());
    }

    if (big_status != OK )
    {
	return( big_status );
    }
#endif /* xCL_ASYNC_IO */

    return( small_status );
}

# if !defined(hp8_us5)
/*{
** Name: DI_listio_write -  POSIX lio_listio wrapper.
**
** Description:
**	This function batches up I/O calls that will eventually be written
**	to disc simultaneously.  Below this level the I/O list might reach
**	a maximum size and be written out in a batch anyway.
**
** Inputs:
**      DI_IO *f        - Pointer to the DI file context needed to do I/O.
**      diop		     Pointer to dilru file context.
**      buf                  Pointer to page(s) to write.
**      page                 Value indicating page(s) to write.
**      num_of_pages	     number of pages to write
**      evcomp               completion routine
**      closure              info used by the completion routine
**      
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**	    other errors.
**    Exceptions:
**        none
**
** Side Effects:
**        If number of write requests reaches AIO_LISTIO_MAX then
**	  the output requests in the list will be done.
**
** History:
**    01-mar-1995 (anb)
**	    Created.
**    11-may-1999 (hweho01)
**          aio_errno is obsolete in this function, it should
**          be removed.
*/
static STATUS
DI_listio_write(
    DI_IO	*f,
    DI_OP	*diop,
    char        *buf,
    i4	page,
    i4	num_of_pages,
    STATUS (*evcomp)(), 
    PTR closure, 
    CL_ERR_DESC *err_code)
{
    STATUS	status = OK;

    DI_AIOCB * aio;
#if !defined(any_aix) && !defined(sui_us5)
    char *iobuf = buf;

    /* unix variables */
    i4		bytes_to_write;
    OFFSET_TYPE lseek_offset;
    i4		bytes_written;

    /* 
    ** seek to place to write 
    */
    lseek_offset = (OFFSET_TYPE)f->io_bytes_per_page * (OFFSET_TYPE)page;
    bytes_to_write = (f->io_bytes_per_page * (num_of_pages));
	    
    /*
    ** If alignment is required, then copy data buffer to an aligned
    ** buffer for the io request.
    **
    ** Buffer alignment is only required on raw devices.
    */
    if (fisraw((i4)diop->di_fd) && (((i4)(SCALARP)buf) & (IO_ALIGN - 1)) )
    {
    	if ( !(iobuf = getalbuf( bytes_to_write, err_code )) )
	{
	    return -1;
	}
	MEcopy( buf, bytes_to_write, iobuf );
    }

    aio = DI_aio_pkg( (i4)diop->di_fd, iobuf, bytes_to_write, lseek_offset);
    if ( !aio)
    {
        return(DI_MEREQMEM_ERR);
    }
    aio->aio.aio_lio_opcode = LIO_WRITE;
    aio->evcomp = evcomp;
    aio->diop = *diop;
    aio->data = closure;

    status = gather_listio( aio, err_code );

#endif /* !aix !sui_us5 */
    return( status );
}

/*{
** Name: gather_listio -  Gather write requests together.
**
** Description:
**	This routine batches up write requests for later submission via     
**	the lio_listio() routine.
**
** Inputs:
**	DI_AIOCB * aio	  - aio Control block for write operation.
**      
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**          OK
**          DI_BADEXTEND       - MEreqmem failed.
**	    DI_NO_AIO_RESOURCE - The kernel has insufficient resources at
**                               this time to satisfy the request.
**    Exceptions:
**        none
**
** Side Effects:
**        Will actually call do_listio if number of write requests reaches
**        AIO_LISTIO_MAX.
**
** History:
**    01-mar-1995 (anb)
**	    Created.
*/
static STATUS
gather_listio( DI_AIOCB *aio, CL_ERR_DESC *err)
{
#if defined(xCL_ASYNC_IO) || defined(OS_THREADS_USED)
    DI_LISTIOCB *tptr;
    DI_AIOCB *aptr = NULL;
    DI_LISTIOCB *newone;
    STATUS status = OK;
    i4  cnt=0;

# ifdef OS_THREADS_USED
    CSp_semaphore( TRUE, &listioThreadsSem );
# endif /* OS_THREADS_USED */

    for ( tptr = listioThreads; tptr != NULL; tptr = tptr->next )
    {
        if ( tptr->sid == aio->sid)
            break;
    }

    if ( tptr == NULL)
    {
        /* No cb for this thread yet so create one */
    
        newone = (DI_LISTIOCB *)MEreqmem(0,
		(sizeof( DI_LISTIOCB )), TRUE, NULL);
        if ( !newone)
        {
	    return( DI_MEREQMEM_ERR);
        }
        newone->sid = aio->sid;
        newone->next = (DI_LISTIOCB *) NULL;
        newone->list = (DI_AIOCB *) NULL;
        newone->listio_entries = 0;
        newone->outstanding_aio = 0;
        if ( listioThreads == NULL)
	{
	    listioThreads = newone;
	    listioThreads->prev = (DI_LISTIOCB*) NULL;
	}
	else
	{
	    for (tptr = listioThreads; tptr->next != NULL; tptr = tptr->next)
		;
	    tptr->next = newone;
	    newone->prev = tptr;
	}
	tptr = newone;
    }

    append_aio( &tptr->list, aio );

    tptr->listio_entries += 1; 
    tptr->state = AIOLIST_ACTIVE;
    if ( tptr->listio_entries == AIO_LISTIO_MAX )
        do
        {
            status = do_listio( tptr );
            if ( status )
            {
		/*
		** Managed to queue some I/O's so just carry on
		** until the thread is forced out.
		*/
                if ( tptr->listio_entries < AIO_LISTIO_MAX )
                    break;
#ifdef dr6_us5
                /* Wait until at least one kernel resource is available */
                aiopoll( NULL, 0 );
#endif /* dr6_us5 */
            }
	    cnt++;
        }
        while (status && cnt <= AIO_RETRIES);

# ifdef OS_THREADS_USED
    CSv_semaphore( &listioThreadsSem );
# endif /* OS_THREADS_USED */

    if ( status && (cnt > AIO_RETRIES) )
    {
        if ( err)
            SETCLERR( err, 0, ER_lio_listio);
        return( DI_NO_AIO_RESOURCE);
    }
#endif /* xCL_ASYNC_IO || OS_THREADS_USED */
    return( OK);
}

/*{
** Name: do_listio -  Perform lio_listio() call.
**
** Description:
**	This routine calls lio_listio to queue the write requests that have 
**	been gathered.
**
** Inputs:
**	DI_LISTIOCB * currThread  - Control block for current thread. 
**      
** Outputs:
**    None.
**
** Returns:
**    OK
**    FAIL - The lio_listio call failed to queue some or all of the requests.
**
**    Exceptions:
**        none
**
** Side Effects:
**        Any write requests that are submitted are removed from this threads
**        control block and placed on the global outstanding aio list.
**
** History:
**    01-mar-1995 (anb)
**	    Created.
**	29-Apr-1997 (merja01)
**      Added axp_osf ifdef to account for aiocb differences.
*/
static STATUS
do_listio( DI_LISTIOCB * currThread)
{
    STATUS status=OK;
    i4  ret_val = 0;
    i4  count;
    DI_AIOCB *iocb_ptr;
    DI_AIOCB **base_ptr;

#if defined(xCL_ASYNC_IO) 
    if ( currThread == NULL)
        return( FAIL);

    /*
    ** Need to build an array of pointers to the aiocb's of
    ** our I/O requests.
    */

    iocb_ptr = currThread->list;
    for (count=0; count < AIO_LISTIO_MAX; count++)
    {
	if (iocb_ptr)
	{
	    currThread->gather[count] = &iocb_ptr->aio;
	    iocb_ptr = iocb_ptr->next;
	}
	else
	    /* null pointers for unused slots */
#ifdef LARGEFILE64
	    currThread->gather[count] = (aiocb64_t *)NULL;
#else /* LARGEFILE64 */
	    currThread->gather[count] = (aiocb_t *)NULL;
#endif /* LARGEFILE64 */
    }

# ifdef xCL_ASYNC_IO
#ifdef MINX
    ret_val = aio_list( &currThread->gather[0], 
                        currThread->listio_entries, NULL );
#else
#ifdef LARGEFILE64
    ret_val = lio_listio64( LIO_NOWAIT, &currThread->gather[0], 
                          currThread->listio_entries, NULL );
#else /* LARGEFILE64 */
    ret_val = lio_listio( LIO_NOWAIT, &currThread->gather[0], 
                          currThread->listio_entries, NULL );
#endif /* LARGEFILE64 */
#endif /* MINX */
# else /* xCL_ASYNC_IO */
# ifdef OS_THREADS_USED
    ret_val = DI_thread_listio( LIO_NOWAIT, &currThread->gather[0], 
                                currThread->listio_entries, NULL );
# endif /* OS_THREADS_USED */
# endif /* xCL_ASYNC_IO */

    /*
    ** Here lieth a problem. 
    ** If lio_listio returns -1 it (in this case) indicates that some writes
    ** failed to be queued. These will need to be resubmitted, but we must not
    ** attempt to resubmit those that have been successfully queued as by the
    ** time they are dealt with again their DI control blocks may have been 
    ** freed. So we move the successfully queued ones to the global I/O list
    ** and leave the others to be submitted in a later call to do_listio.
    */
    iocb_ptr = currThread->list;
    base_ptr = &currThread->list;
    while( iocb_ptr != NULL)
    {
	/*
	** aio_errno is originally set 'in progress' but it is
	** possible that it has already completed OK.
	*/
#ifdef dr6_us5
	if ( (iocb_ptr->aio.aio_errno == EINPROGRESS) ||
	    (iocb_ptr->aio.aio_errno == 0))
#else
#ifdef LARGEFILE64
	if ( ((aio_error64(&iocb_ptr->aio)) == EINPROGRESS) ||
	    ((aio_error64(&iocb_ptr->aio)) == 0))
#else /* LARGEFILE64 */
	if ( ((aio_error(&iocb_ptr->aio)) == EINPROGRESS) ||
	    ((aio_error(&iocb_ptr->aio)) == 0))
#endif /* LARGEFILE64 */
#endif  /* dr6_us5 */
        {
            remove_aio( base_ptr, iocb_ptr);
# ifdef OS_THREADS_USED
	    CSp_semaphore( TRUE, &aioListSem );
# endif /* OS_THREADS_USED */
            append_aio( &aioList, iocb_ptr);
            currThread->listio_entries -= 1;
            currThread->outstanding_aio += 1;
	    iocb_ptr = *base_ptr;
# ifdef OS_THREADS_USED
	    CSv_semaphore( &aioListSem );
# endif /* OS_THREADS_USED */
        }
	else
	{
	    base_ptr = &iocb_ptr->next;
	    iocb_ptr = iocb_ptr->next;
	}
    }
    if (ret_val)
    {
        return( FAIL);
    }
#endif /* xCL_ASYNC_IO */
    return( OK);
}

/*{
** Name: force_listio -  Force all outstanding writes to disc.
**
** Description:
**	This routine attempts to write all outstanding listio requests.
**
** Inputs:
**	DI_AIOCB * aio	  - aio Control block for write operation.
**      
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
** Returns:
**      OK
**      FAIL 		   - No listio control block found for this thread.
**	DI_NO_AIO_RESOURCE - The kernel has insufficient resources at
**                           this time to satisfy the request.
** Exceptions:
**      none
**
** Side Effects:
**      None.
**
** History:
**    01-mar-1995 (anb)
**	    Created.
**    29-Apr-1997 (merja01)
**      Added ifdefs for dr6_us5 for aiopoll.  This function only
**      appears to be available on the ICL platforms and is ifdefed
**      for dr6_us5 in other places in the code.  
*/
static STATUS
force_listio( CL_ERR_DESC *err)
{
    DI_LISTIOCB *tptr;
    DI_AIOCB *aptr = NULL;
    DI_AIOCB *iolist_ptr = NULL;
    STATUS status=FAIL;
    STATUS big_error=OK;
    i4  cnt=0;
    CS_SID sid;

#ifdef xCL_ASYNC_IO
    CSget_sid(&sid);

# ifdef OS_THREADS_USED
    CSp_semaphore( TRUE, &listioThreadsSem );
# endif /* OS_THREADS_USED */
    for ( tptr = listioThreads; tptr != NULL; tptr = tptr->next )
    {
        if ( tptr->sid == sid)
            break;
    }

    if ( (tptr == NULL) || (tptr->state == AIOLIST_INACTIVE))
    {
	/*
	** No cb for this thread or nothing on it's I/O list,
	** therefore no work to do.
	*/
# ifdef OS_THREADS_USED
        CSv_semaphore( &listioThreadsSem );
# endif /* OS_THREADS_USED */
	return(OK);
    }
    
    /*
    ** Loop until successfully fired off all the requests in the list.
    */

    while( tptr->listio_entries > 0 &&
           status &&
           cnt <= AIO_RETRIES)
    {
        status=do_listio( tptr);
        if (status)
#ifdef  dr6_us5
            /* 
            ** Ensure don't attempt re-try until at least one kernel 
            ** resource is available
            */
            aiopoll( NULL, 0);
#endif  /*dr6_us5*/
        cnt++;
    }

    if ( status && (cnt > AIO_RETRIES) )
    {
        if ( err)
            SETCLERR( err, 0, ER_lio_listio);
# ifdef OS_THREADS_USED
        CSv_semaphore( &listioThreadsSem );
# endif /* OS_THREADS_USED */
        return( DI_NO_AIO_RESOURCE);
    }

    /*
    ** The thread is only resumed when all replies have come back.
    */
    tptr->state = AIOLIST_FORCE;

    status = OK;
    if (tptr->outstanding_aio != 0)
	if ( (status=CSsuspend( CS_DIOW_MASK, 0, 0) ) != OK)
	    DIlru_set_di_error( status, err, DI_LRU_CSSUSPEND_ERR, 
							DI_GENERAL_ERR);

    if (!status)
    {
# ifdef OS_THREADS_USED
	CSp_semaphore( TRUE, &aioListSem );
# endif /* OS_THREADS_USED */
	iolist_ptr = aioList;
	while( iolist_ptr != NULL)
	    if (iolist_ptr->sid == sid && iolist_ptr->resumed)
	    {
		status=DI_complete_listio( iolist_ptr);
		if ( status)
		    big_error=DI_BADWRITE;
		aptr = iolist_ptr;
		iolist_ptr = iolist_ptr->next;
		DI_free_aio( aptr);
            }
	    else
		iolist_ptr = iolist_ptr->next;
# ifdef OS_THREADS_USED
	CSv_semaphore( &aioListSem );
# endif /* OS_THREADS_USED */
    }

    tptr->state = AIOLIST_INACTIVE;

# ifdef OS_THREADS_USED
    CSv_semaphore( &listioThreadsSem );
# endif /* OS_THREADS_USED */

    if ( big_error)
    {
	return( big_error);
    }
#endif /* xCL_ASYNC_IO */
    return( status);
}

/*{
** Name: DI_complete_listio -  lio_listio completion handler for DI.
**
** Description:
**	This routine tidies up after a listio op finishes.
**
** Inputs:
**	DI_AIOCB * aio - control block holding completion data.
**      
** Outputs:
**      None.
**
** Returns:
**      OK
**      FAIL 
** 
** Exceptions:
**      none
**
** Side Effects:
**      Will execute the callers completion handler if data ptr not null.
**
** History:
**    01-mar-1995 (anb)
**	    Created.
*/
static STATUS
DI_complete_listio( DI_AIOCB *aio)
{
    STATUS status=OK;
    STATUS big_status=OK;
    CL_ERR_DESC err_code;

    /*
    ** DIwrite_list copies DIwrite, except that this line needs to execute
    ** after the asynchronous write completes.
    */
    big_status=DIlru_release(&aio->diop, &err_code);

    status=(aio->evcomp)( aio->data, big_status);
    return( big_status?big_status:status );
}

/*{
** Name: DI_check_list	 -  Return a non-zero value if there are outstanding
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
**      0 		   - No outstanding I/O's.
**	1		   - There are outstanding I/O's.
**
** Exceptions:
**      none
**
** Side Effects:
**      None.
**
** History:
**    09-nov-1995 (itb)
**	    Created.
*/
static STATUS
DI_check_list()
{
    DI_LISTIOCB *tptr;
    STATUS status;
    CS_SID sid;

    CSget_sid(&sid);

# ifdef OS_THREADS_USED
    CSp_semaphore( TRUE, &listioThreadsSem );
# endif /* OS_THREADS_USED */
    for ( tptr = listioThreads; tptr != NULL; tptr = tptr->next )
    while( tptr != NULL)
    {
        if ( tptr->sid == sid)
            break;
    }

    if ( (tptr == NULL) || (tptr->state == AIOLIST_INACTIVE))
        /*
	** No cb for this thread or nothing on it's I/O list,
	** therefore no work to do.
	*/
	status = 0;
    else
	status = 1;
# ifdef OS_THREADS_USED
    CSv_semaphore( &listioThreadsSem );
# endif /* OS_THREADS_USED */
    
    return(status);
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

static
void *
DI_thrp_open( void *parm )
{
    II_DIO_OPARMS	*oparms = (II_DIO_OPARMS *) parm;
    int fd;
    STATUS memstatus;                 /* dummy parameter for MEreqmem */
#ifdef LARGEFILE64
    struct stat64 sbuf;
#else /* LARGEFILE64 */
    struct stat sbuf;
#endif /*  LARGEFILE64 */
    i4 tablesize;

    if ( oparms->err )
	CL_CLEAR_ERR( oparms->err );

    /* create a rawfd[] array of the right size the first time through */

    if ( !rawfd )
    {
        tablesize = iiCL_get_fd_table_size();
        rawfd = (char*)MEreqmem(0, tablesize, TRUE, &memstatus);
        if ( !rawfd )
        {
            CS_exit_thread( (void *) (SCALARP)-1 );
        }
    }

#if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
    {
        static bool     checked = 0;
        static bool     stickycheck = 0;
        char            *cp;

        /* Don't mess with unsetting the sticky bit if this is set. */
        if ( !checked )
        {
            NMgtAt("II_DIO_STICKY_SET", &cp);
            if ( stickycheck = (cp && *cp != EOS) )
            {
                TRdisplay("IIdio: Sticky bit not being checked.\n" );
            }

            checked = TRUE;
        }

        if (((oparms->mode & O_SYNC) == 0) && 
	    ((oparms->mode & O_CREAT) == 0) && !stickycheck)
        {
            /*
            ** Turn off the sticky bit if we're not using O_SYNC.
            ** The sticky bit feature was only designed to work with
            ** files that were opened with O_SYNC.
            **
            ** Note that we're ignoring the return status because all of the
            ** serious file problems will be caught be the open() call.
            */

#ifdef LARGEFILE64
            if (!stat64(oparms->file, &sbuf))
#else /* LARGEFILE64 */
            if (!stat(oparms->file, &sbuf))
#endif /* LARGEFILE64 */
            {
                int     new_perms = (unsigned)sbuf.st_mode & ~S_IFMT;

                new_perms &= ~S_ISVTX;
                (VOID) chmod(oparms->file, new_perms);
            }
        }
    }
# endif

#ifdef LARGEFILE64
    if ( (fd = open64( oparms->file, oparms->mode, oparms->perms )) == -1 )
#else /* LARGEFILE64 */
    if ( (fd = open( oparms->file, oparms->mode, oparms->perms )) == -1 )
#endif /*  LARGEFILE64 */
    {
        if ( oparms->err )
            SETCLERR(oparms->err, 0, ER_open);
    }
    else
    {
        /* "can't fail" */
#ifdef LARGEFILE64
        (void) fstat64( fd, &sbuf );
#else /* LARGEFILE64 */
        (void) fstat( fd, &sbuf );
#endif /*  LARGEFILE64 */
        if ( sbuf.st_mode & S_IFCHR )
            rawfd[ fd ] =  TRUE;
        else
            rawfd[ fd ] =  FALSE;

#if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
        {
            /*
            ** On SunOS systems, if we set the sticky bit and clear the
            ** execute bits, non-critical inode updates will be deferred
            ** when the file is open for O_SYNC.  The feature may not be
            ** enabled on all systems; so, ignore any errors.
            **
            ** Try to do this just at file creation time--assume that
            ** if file is empty and O_CREAT is set, then this is file
            ** creation time.
            **
            ** If we are opening a file with O_SYNC, and the sticky
            ** bit got turned off for some reason, let's turn it back on.
            **
            ** For Solaris, we use O_DSYNC and avoid this whole mess.
            **
            */
            if ( ((((oparms->mode & O_CREAT) == O_CREAT) && sbuf.st_size == 0)
                  || (((oparms->mode & O_SYNC) == O_SYNC)
                     && ((sbuf.st_mode & S_ISVTX) == 0)))
                 && (rawfd[ fd ] == FALSE)
               )
            {
                int     new_perms = (unsigned)sbuf.st_mode & ~S_IFMT;

                new_perms |= S_ISVTX;
                new_perms &= ~(S_IEXEC | S_IXGRP | S_IXOTH);
                (VOID) fchmod(fd, new_perms);
            }
        }
#endif /* xCL_081_SUN_FAST_OSYNC_EXISTS */

    }

    if ( dolog() > 0 )
        TRdisplay("IIdio_open: file %s mode 0x%x on fd %d [%s]\n",
                oparms->file, oparms->mode, fd, 
		rawfd[fd] ? "raw disk" : "regular file" );

    CS_exit_thread( (void *) (SCALARP)fd );
}

int
DI_thread_open( char *file,
	 	int  mode,
		int  perms,
		CL_ERR_DESC *err )
{
    II_DIO_OPARMS	op, *oparms = &op;
    CS_THREAD_ID	tid;
    STATUS		status;
    int			ret_val;

    oparms->file = file;
    oparms->mode = mode;
    oparms->perms = perms;
    oparms->err = err;
		
#if defined(DCE_THREADS)
    CS_create_thread( 32768, DI_thrp_open, oparms, &tid, CS_JOINABLE, &status );
#elif defined(any_hpux)
    CS_create_thread( 32768, NULL, DI_thrp_open, oparms, 
		      &tid, CS_JOINABLE,
		      Cs_srv_block.cs_stkcache,
		      &status );
#else
    CS_create_thread( 32768, NULL, DI_thrp_open, oparms,
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
        CS_join_thread(tid,(void**)&ret_val);

    return (ret_val);
}

static
int
DI_thread_listio( int mode,
		  aiocb_t **list,
		  int nent,
		  struct sigevent *sig )
{
    register int i;
    STATUS status = OK;
    II_DIO_RWPARMS *diop;
    struct di_lio_ret
    {
	CS_THREAD_ID tid;
        II_DIO_RWPARMS dioparms;
	CL_ERR_DESC  err;
    } *lio_ret;


    lio_ret = (struct di_lio_ret *) MEreqmem( 0, 
					      (nent*sizeof(struct di_lio_ret)), 
				      	      TRUE, NULL );
    if ( lio_ret == NULL )
	return -1;

    /* fire off the different I/O's */
    for ( i = 0; i < nent; i++ )
    {
	if ( list[i]->aio_lio_opcode == LIO_READ )
	    diop->op = O_RDONLY;
	else if ( list[i]->aio_lio_opcode == LIO_WRITE )
	    diop->op = O_WRONLY;
	else
	{
	    nent--;
	    i--;
	    continue;
	}
	diop = &lio_ret[i].dioparms;
	diop->fd = list[i]->aio_fildes;
	diop->buf = (char *) list[i]->aio_buf;
	diop->len = list[i]->aio_nbytes;
	diop->off = list[i]->aio_offset;
	diop->loffp = 0;
	diop->err = &lio_ret[i].err;
#ifdef dr6_us5
	list[i]->aio_errno = EINPROGRESS;
#endif  /* dr6_us5 */
	
#if defined(DCE_THREADS)
    CS_create_thread( 32768, DI_thrp_rw, diop,
                          &lio_ret[i].tid, CS_JOINABLE, &status );
#elif defined(any_hpux)
    CS_create_thread( 32768, NULL, DI_thrp_rw, diop,
		      &lio_ret[i].tid, CS_JOINABLE,
		      Cs_srv_block.cs_stkcache,
		      &status );
#else
    CS_create_thread( 32768, NULL, DI_thrp_rw, diop,
                      &lio_ret[i].tid, CS_JOINABLE,
                      &status );
#endif

	if ( status )
	{
#ifdef dr6_us5
	    list[i]->aio_errno = errno;
#endif /* dr6_us5 */
            SETCLERR( &lio_ret[i].err, status, 0 );
	}
	/* sleep until I/O completes */
	if ( status == OK )
#ifdef dr6_us5
	    CS_join_thread( lio_ret[i].tid, (void**)&list[i]->aio_return );
#else
	    CS_join_thread( lio_ret[i].tid, NULL);
#endif /* dr6_us5 */
#ifdef dr6_us5
	if ( lio_ret[i].err.errnum != OK )
	    list[i]->aio_errno = lio_ret[i].err.errnum;
#endif /* dr6_us5 */
    }
    MEfree( (PTR)lio_ret );
}
# endif /* OS_THREADS_USED */
# endif /* hp8_us5 */
# else /* xCL_ASYNC_IO || OS_THREADS_USED */ 


#if !defined (xCL_ASYNC_IO) && !defined (OS_THREADS_USED)

/* Dummy function to resolve link symbols if async i/o not supported */
STATUS
DIwrite_list( i4  op, DI_IO *f, i4  *n, i4 page, char *buf,
	      STATUS (*evcomp)(), PTR closure, CL_ERR_DESC *err_code)
{
    return(OK);
}

#endif

# endif /* xCL_ASYNC_IO || OS_THREADS_USED */ 
