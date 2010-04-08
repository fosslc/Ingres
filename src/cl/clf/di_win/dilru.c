/*
** Copyright (c) 1985, 2004 Ingres Corporation
*/

#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <systypes.h>
#include   <cv.h>
#include   <er.h>
#include   <di.h>
#include   <pc.h>
#include   <pm.h>
#include   <qu.h>
#include   <me.h>
#include   <nm.h>
#include   <tr.h>
#include   <cs.h>
#include   <st.h>
#include   <csinternal.h>
#include   <errno.h>
#include   "dilru.h"

/**
**
**  Name: DILRU.C - This file contains the UNIX internal DI cache routines.
**
**  Description:
**      The DILRU.C file contains all the routines needed to perform LRU
**	caching of per session file descriptors.  Routines are called by
**	both other DI routines and other backend CL modules which may
**	need to free up a file descriptor.
**
**	  DIlru_close - close a given file, freeing a file descriptor.
**	  DIlru_free - pick victim file and close , freeing a file descriptor.
**	  DIlru_init  - initialize hash control block
**	  DIlru_open - open file returning pointer to file descriptor.
** 
**	The DI module must support up to 256 open files (TCB's).  Since
**	most if not all UNIX boxes can not support this many open file
**	descriptors a cache of currently open files is supported by this
**	module.
**
**	At DIopen time a path and filename are passed into the function.
**	At this time an new numeric id (unique to the server) is assigned
**	to the file by DIlru_uniq.  Then DI attempts to open the file using
**	DIlru_open.  This function first attempts an open() on the file.
**	If it gets an error, then it closes a file using (DIlru_close) in
**	the descriptor cache, and retries the open until it either
**	succeeds in opening the file or runs out of files to close.
**
**	When it succeeds in opening the file it adds an entry to the
**	file descriptor cache.  The number of entries in the descriptor
**	cache is <= the number of files a single process can open.
**
**	On successive DI calls a pointer to a DI_IO struct is passed in.
**	In this structure is the unique id assigned by DIlru_uniq.  The
**	unique id is used to do a hash search of the open file descriptor
**	cache.  If the file is found then the current open file descriptor
**	is used, else the path and filename information stored in the DI_IO
**	by DIopen are used to open the file (using DIlru_close if necessary).
**
**	Jupiter Issues:
**	DI likes to do all I/O in slave processes via the CS event support.
**	However, many utilities need to use DI as well so DI will operate
**	in the main process if needed.  DIlru_open will fill in a DI_OP
**	control block if slave driven or a unix file descriptor if DI is in
**	the main process.  The DI_OP structure must be released with the
**	call DIlru_release.
**
**	Notes:
**	
**	18-dec-1992 (rmuth)
**
**	Whilst resructuring a large amount of this code i noticed the 
**	following design issues...thought i would note them here otherwise
**	they will be lost forever.
**
**	1. DIlru_file_open makes a request to a slave to open a file,
**	   if the request fails due to too many open files in that slave
**	   and the user has requested RETRY we inact the following 
**	   algorithm.
**		a. Search for a file which does not have an outstanding
**		   slave operation on it, ie fd_refcnt == 0.
**	        b. Once the above has been found see if that file is
**		   currently open in any slave. Note this search
**		   starts at slave one and moves to slave N.
**		c. If both the above have been satisfied then send a request
**		   to that slave requesting that it close the victim file
**		   and open the new file.
**
**		If no victim file can be found then return an error.
**
**	   The above algoritm unnecessarily closes files, as it does not take
**	   into account the fact that altough the slave we made the initial
**	   request to has no vacant FD's one of the other slaves may have
**	   vacant FD's.
**
**	   Hence in the future we may want to improve the above algorithm
**	   to first try the other slaves before it closes a file. Note this
**	   probably not an issues as by the time we have got to this state
**	   the system is probably pretyy sluggish hence a 'close' here or
**	   there would be no great deal.
**
**	   The reason for the above is that the code was orginally written
**	   when there we no slave processes it was all done inline.
**
**	2. The fd_refcnt is used to indicate that there is an outstanding
**	   slave operation on this file and hence we should not DIlru
**	   close the file. This flag need to be at the granularity of
**	   slave instead of all slaves, as we can close the file in 
**	   slave A if the operation is outstanding in Slave B.
**
**
** History:
**	08-dec-1997 (canor01)
**	    Copied from Unix CL.
**	28-jan-1998 (canor01)
**	    Major revision. The previous code did a hash-table lookup
**	    on every call to DI in order to retrieve the file descriptor
**	    (or reopen the file if it was closed).  Since NT can have
**	    very large numbers of files open, it is probably better to
**	    move the overhead from normal DI calls to the LRU calls.
**	    All opened files are kept on a least-recently-opened queue.
**	    When file descriptors are exhausted, the earliest-opened
**	    file is closed.  While this may cause a little more thrashing
**	    when resources are restricted, in most cases it will give a
**	    performance improvement.
**	12-feb-1998 (canor01)
**	    Do not LRU close the file if there is a positive reference
**	    count (read or write in progress).
**      04-mar-1998 (canor01)
**          Parameters to DIlru_flush were changed on Unix side.  Change
**          them here too.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	05-nov-1999 (somsa01)
**	    Added setting of Di_gather_io.
**	11-nov-1999 (somsa01)
**	    In DIlru_inproc_open(), if DI_O_OSYNC_MASK and Di_gather_io are
**	    set, DI_O_LOG_FILE_MASK is not set, and if we are dealing with
**	    at least 4K pages, add FILE_FLAG_NO_BUFFERING to the list of
**	    file open flags.
**	13-jul-2001 (somsa01)
**	    In DIlru_inproc_open(), if II_WRITE_THROUGH_CACHE is set, follow
**	    the UNIX model when deciding when to write through the disk
**	    cache.
**	12-dec-2001 (somsa01)
**	    For Win64, follow the UNIX model when deciding when to write
**	    through the disk cache.
**	08-may-2003 (somsa01)
**	    If we're not using gather_write and are dealing with OSYNC,
**	    use FILE_FLAG_WRITE_THROUGH.
**	24-sep-2003 (somsa01)
**	    We now use a new handle identifier for gather write operations.
**	    Initialize / cleanup this handle as well when opening files.
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**	07-oct-2003 (somsa01)
**	    Only open the gather write file handle if the normal file handle
**	    was successfully obtained.
**	26-jan-2004 (somsa01)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	21-nov-2005 (drivi01)
**	    Added routine to flush cache buffers before dbms closes handles
**	    to any files.
**      20-apr-2005 (rigka01) bug 114010, INGSRV3180
**        Sessions hang in a mutex deadlock on DI LRU SEM and
**        DI <filename> SEM.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
*/


/*
**  Forward and/or External typedef/struct references.
*/
static STATUS DIlru_check_init( STATUS	    *intern_status,
				CL_ERR_DESC *err_code);

static STATUS DI_lru_free(
				CL_ERR_DESC	*err_code );

/* externs */

GLOBALREF CS_SYSTEM	Cs_srv_block;

GLOBALREF CS_SEMAPHORE	DI_misc_sem;

GLOBALREF QUEUE		DI_lru_queue;

GLOBALREF bool		DI_lru_initialized;

GLOBALREF i4		Di_gather_io;

/*
**  Local Definitions.
*/
#define		SEM_SHARE	0
#define		SEM_EXCL	1


/*
**  Definition of static variables and forward static functions.
*/

i4 		   Di_fatal_err_occurred ZERO_FILL;
			/*
			** This is used to indicate that we got an error
			** in DI where we do not want to continue operation
			** as we may corrupt something as the DI internal
			** structures may be knackered hence we PCexit.
			** We use this flag instead of just calling PCexit
			** when we encounter the error to try and give
			** time for the upper layers to log some traceback
			** messages.
			*/

#if defined(i64_win) || defined(a64_win)
static bool	DIunix_cache_style = TRUE;
#else
static bool	DIunix_cache_style = FALSE;
#endif  /* i64_win || a64_win */


/*
** Name: DIget_direct_align - Get platform direct I/O alignment requirement
**
** Description:
**	Some platforms require that buffers used for direct I/O be aligned
**	at a particular address boundary.  This routine returns that
**	boundary, which is guaranteed to be a power of two.  (If some
**	wacky platform has a non-power-of-2 alignment requirement, we'll
**	return the nearest power of 2, but such a thing seems unlikely.)
**
**	I suppose this could be a manifest constant, like DI_RAWIO_ALIGN,
**	but doing it as a routine offers a bit more flexibility.
**
**	It seems that Windows requires address alignment to be a multiple
**	of the volume sector size.  I will wait for a platform expert to
**	decide what exactly that means.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns required power-of-2 alignment for direct I/O;
**	may be zero if no alignment needed, or direct I/O not supported.
**
** History:
**	6-Nov-2009 (kschendel) SIR 122757
**	    Stub for Windows.
*/

u_i4
DIget_direct_align(void)
{
    /* Direct (unbuffered) IO not yet supported for Windows */
    return (0);
} /* DIget_direct_align */

/*{
** Name: DIlru_close - Close a given DI file.
**
** Description:
**	Will close a given DI file.  Called from DIclose when dbms actually
**	closes the file.  This will insure that a session that closes a file
**	which it has opened will actually close it in DIclose.  If file is
**	not in the open file cache then return OK since this means it was
**	LRU'd out (closed).
**
**	The slot in the open file cache is put on the free list.
**
**	In the multiple session per server case DIclose may not be called by the
**	session which originally opened the file.  This can take place in the
**	following scenario:
**
**		Session A opens the file.  Session B opens the file.  Now both
**		sessions have their own separate UNIX file descriptors for the
**		file.  DMF decides not to cache the open file any more so it
**		issues a DIclose() on the file - DMF has decided to do this
**		processing a request from Session B.  Session B will call 
**		DIlru_close() actually closing the UNIX file descriptor.  The
**		architecture makes it difficult if not impossible for Session B
**		to inform Session A to close the file; so the file remains open
**		in Session A.  Eventually as session A opens more files the file
**		in question will be closed by the lru algorithm. 
**
**	Also it is assumed that all open files for a session are closed when the
**	process who owns that session exits.
**
** Inputs:
**	f		    Pointer to the DI file context needed to do I/O.
**
** Outputs:
**	err_code	    Pointer to a variable used to return OS errors.
**
**    Returns:
**        OK		    Successfully closed file, freeing a file descriptor.
**	  DI_CANT_CLOSE	    No available file descriptors to close.
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
*/
STATUS
DIlru_close(
    DI_IO		*f,
    CL_ERR_DESC	*err_code )
{
    STATUS		big_status =OK, ret_status = OK, intern_status =OK;
    bool		have_sem = FALSE;

    do
    {
	/*
	** If a fatal error occured in DI at some time then bring down 
	** the server to prevent possible corruption.
	*/
	if ( Di_fatal_err_occurred )
	{
	    TRdisplay(
	       "DIlru_close: Failing server due to unrecoverable DI error\n");
	    PCexit(FAIL);
	}

	big_status = CSp_semaphore(1, &DI_misc_sem);
	if ( big_status != OK )
	{
	    intern_status = DI_LRU_PSEM_ERR;
	    break;
	}
	else
	    have_sem = TRUE;

        if ( f->io_nt_fh != 0 && 
	     f->io_nt_fh != INVALID_HANDLE_VALUE &&
	     f->io_refcnt == 0 )
        {
	    FlushFileBuffers(f->io_nt_fh);
            CloseHandle(f->io_nt_fh);
            f->io_nt_fh = INVALID_HANDLE_VALUE;
	    if (f->io_nt_gw_fh != 0 && f->io_nt_gw_fh != INVALID_HANDLE_VALUE)
	    {
		CloseHandle(f->io_nt_gw_fh);
		f->io_nt_gw_fh = INVALID_HANDLE_VALUE;
	    }

            QUremove((QUEUE *)f);
            QUinit((QUEUE *)f);
	    f->io_queue = DI_IO_NO_QUEUE;
        }

	/*
	** If we still have the DI_misc_sem then release it
	*/
	if ( have_sem )
	{
	   big_status = CSv_semaphore( &DI_misc_sem);
	   if ( big_status != OK )
	   {
		intern_status = DI_LRU_VSEM_ERR;
		Di_fatal_err_occurred = TRUE;
		TRdisplay(
		  "DIlru_close: Failed releasing DI semaphore, status=%x\n",
		  big_status);
		break;
	   }
	}

    } while (FALSE);

    /* 
    ** invalidate the control block 
    */
    f->io_type = DI_IO_LRUCLOSE_INVALID;


    if ( big_status != OK )
	ret_status = big_status;

    if (ret_status != OK )
	DIlru_set_di_error( &ret_status, err_code, intern_status, 
			    DILRU_CLOSE_ERR);


    return( ret_status );
}

/*{
** Name: DIlru_free - Close a DI file using lru algorithm.
**
** Description:
**	Will close a DI file using a lru algorithm.  Can be called from
**	anywhere in the CL.  This routine will be called by any CL routine
**	called by the dbms which fails while trying to open a file because
**	there are no available file descriptors.  Callers should loop
**	if they encounter the same out of file descriptor error until 
**	DIlru_free indicates that it can't close any more files.
**
**	The last entry in the lru queue is chosen as the victim.  That 
**	file descriptor is closed.  The entry is put on the free list.
**
**	NOTE: this is now only used when DI is operating in the main
**		process and not a slave async model.
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**    Returns:
**        OK		    Successfully closed file, freeing a file descriptor.
**	  DI_CANT_CLOSE	    No available file descriptors to close.
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
*/
STATUS
DIlru_free( 
    CL_ERR_DESC *err_code)
{
    STATUS		big_status, status, intern_status = OK;

    status = DIlru_check_init( &intern_status, err_code );
    if ( status != OK )
        return (status);

    if (big_status = CSp_semaphore(1, &DI_misc_sem))
    {
	intern_status = DI_LRU_PSEM_ERR;
    }
    else
    {
	status = DI_lru_free( err_code );

	if (big_status = CSv_semaphore(&DI_misc_sem))
	    intern_status = DI_LRU_VSEM_ERR;
    }

    if (big_status != OK)
	status = big_status;
    
    if (status != OK)
	DIlru_set_di_error( &status, err_code, intern_status, DILRU_FREE_ERR );

    return(status);
}

/*{
** Name: DIlru_init - Initialize hash control block.
**
** Description:
**	Given a pointer to a HASH_CONTROL_BLOCK this routine will initialize
**	the hash table to be empty.  It will allocate DI_ENTRY_NUM entries and 
**	put them on the free list.  It will initialize all hash buckets to be
**	empty.
**
**	If this is a multi-threaded server (we can tell because cs_max_active
**	in the Cs_srv_block is non-zero), then attempt to initialize an
**	appropriate number of DI slave processes to perform asynchronous I/O.
**	If the initialization fails (presumably due to resource contraints
**	such as system swap space limitations), then we'll perform the I/O
**	synchronously; issue trace messages to this effect.
**
** Inputs:
**      none
**
** Outputs:
**	none
**
**    Returns:
**        OK
**	  DI_CANT_ALLOC(XX) Can't allocate dynamic memory.
**
**    Exceptions:
**        none
**
** Side Effects:
**        On a DI in slave process implementation sub process slaves
**		will be spawned.
**
** History:
**	05-nov-1999 (somsa01)
**	    Added setting of Di_gather_io.
*/
STATUS
DIlru_init(
    CL_ERR_DESC 	*err_code)
{
    STATUS		status = OK;
    bool		have_sem = FALSE;
    STATUS		intern_status = OK, send_status = OK, me_status = OK;
    char		*str;

    do
    {
	/*
	**
	*/
	Di_fatal_err_occurred = FALSE;

        /*
        ** Initialise misc semaphore
        */
    	status = CSw_semaphore(  &DI_misc_sem, CS_SEM_SINGLE,
					"DI LRU SEM" );
    	if ( status != OK )
	{
	    Di_fatal_err_occurred = TRUE;
	    intern_status = DI_LRU_PSEM_ERR;
	    TRdisplay(
		"DIlru_init: Failed initialising DI semaphore, status=%x\n",
		status );
	    break;
	}

	/*
	** Block anyone using this structure until we have set it up
	*/
	status = CSp_semaphore(1, &DI_misc_sem);
	if ( status != OK )
	{
	    intern_status = DI_LRU_PSEM_ERR;
	    break;
	}
	else
	{
	    have_sem = TRUE;
	}

	/*
	** Check whether gather write is available and in use.
	*/
	status = PMget("!.gather_write",&str);
	if (status == OK)
	{
	    if ( (str[0] != '\0') &&
		 (STbcompare(str, 0, "on", 0, TRUE) == 0) )
	    {
		TRdisplay("%@ DIlru_init: GatherWrite in use\n");
		Di_gather_io = TRUE;
	    }
	    else
	    {
		TRdisplay("%@ DIlru_init: GatherWrite not in use\n");
		Di_gather_io = FALSE;
	    }
	}

        QUinit(&DI_lru_queue);
        DI_lru_initialized = TRUE;

	have_sem = FALSE;
        status = CSv_semaphore( &DI_misc_sem);
	if ( status != OK )
	{
	    intern_status = DI_LRU_VSEM_ERR;
	    Di_fatal_err_occurred = TRUE;
	    TRdisplay("DIlru_init: Failed releasing DI semaphore, status=%x\n",
		      status);
	    break;
	}

	if ( send_status != OK )
	    break;

    	/* 
	** set flag indicating control block has been initiialized 
	*/

    } while (FALSE );

    if ( status != OK )
	send_status = status;

    /*
    ** ERROR condition so lets release any resources that we may have
    ** aquired and return an error.
    */

    if ( send_status != OK )
    {
        if ( have_sem )
	{
	    status = CSv_semaphore( &DI_misc_sem);
	    if ( status != OK )
	    {
		Di_fatal_err_occurred = TRUE;
		TRdisplay(
		   "DIlru_init: Failed releaseing DI semaphore #2,status=%x\n",
		   status );
	    }
	}

        DIlru_set_di_error( &send_status, err_code, 
			    intern_status, DILRU_INIT_ERR );
    }

    return(send_status);
}

/*{
** Name: DIlru_init_di - Initialize DI.
**
** Description:
**      Initializes DI.  The hash control block and table are initialized
**      and on a slave process implementation the slaves are spawned.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**    Returns:
**        OK
**        DI_CANT_ALLOC(XX) Can't allocate dynamic memory.
**
**    Exceptions:
**        none
**
** Side Effects:
**        On a DI in slave process implementation sub process slaves
**              will be spawned.
**
** History:
**
*/
STATUS
DIlru_init_di( CL_ERR_DESC *err_code )
{
    STATUS      ret_val = OK;

    if (!DI_lru_initialized)
        ret_val = DIlru_init(err_code);

    return(ret_val);
}

/*{
** Name: DIlru_open - insure that input file exists in open file cache
**
** Description:
**	Given a pointer to a DI_IO struct search for corresponding entry
**	in the open file cache.  If the file does not exist in the cache then
**	open it and place it in the cache.  If necessary call DI_lru_free
**	to free up a spot in the cache.
**
**	It is assumed that caller has correctly initialized the DI_IO
**	structure.  The DI_IO structure must have the path, pathlength,
**	filename, and filename length filled in.  It must also contain
**	the unique numeric id assigned by DIlru_uniq.  All the previous
**	initialization should take place in DIopen, and the information
**	is stored and saved in the DI_IO struct passed in by the caller
**	on each DI operation.
**
**	If the free list is empty another DI_ENTRY_NUM entries will be
**	allocated and put onto the free list.  The number of entries 
**	allocated continues to grow as long as new files can be open'd
**	successfully.  When opens begin to fail then entries will be
**	closed and put on the free list to be reused.
**
**	When successful DIlru_open() returns a pointer to an event control
**	block that has been reserved using CSreserve_event().  It is up to
**	the caller to free that event control block using CSfree_event().
**	Not doing this can lead to a depletion of event control blocks, 
**	which will cause the server fail in a variety of ways (hang, loop
**	in CSreserve_event, ...).  On failure no event control block is
**	reserved.
**
** Inputs:
**	f		 pointer to di file descriptor information.
**	flags		 Indicates if file should be created and open file
**			 to it returned.  Used mostly for temporary files.
**			 Also, may indicate a need to retry the open.
**
** Outputs:
**	err_code	 system open() error info.
**
**    Returns:
**        OK
**	  DI_CANT_OPEN   Can't open a file due to system limit 
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	02-dec-1997 (canor01)
**	    Created from Unix DI code.
*/
STATUS
DIlru_open(
    DI_IO	*f,
    i4 		flags,
    CL_ERR_DESC	*err_code)
{
    bool		first_open_flag;
    bool		have_sem;
    bool		reserve_ev_flag;
    STATUS		status, intern_status; 

    if (f->io_nt_fh != 0 && f->io_nt_fh != INVALID_HANDLE_VALUE)
	return (OK);

    CL_CLEAR_ERR(err_code);
    first_open_flag = TRUE;
    have_sem = FALSE;
    reserve_ev_flag = FALSE;
    status = OK;
    intern_status = OK;

    do
    {
	/*
	** See of we need to initialize the hash control block
	*/
	status = DIlru_check_init( &intern_status, err_code );
	if ( status != OK )
	    break;

	/*
	** If a fatal error occured in DI at some time then bring down 
	** the server to prevent possible corruption.
	*/
	if ( Di_fatal_err_occurred )
	{
	    TRdisplay(
	       "DIlru_open: Failing server due to unrecoverable DI error\n");
	    PCexit(FAIL);
	}

	/*
	** If runing without slaves then perform work inline
	*/
	return(DIlru_inproc_open(f, flags & DI_FCREATE, err_code));

    } while ( FALSE );

    return ( status );
}

/*{
** Name: DIlru_inproc_open - insure that input file exists in open file cache
**
** Description:
**	Given a pointer to a DI_IO struct search for corresponding entry
**	in the open file cache.  If the file does not exist in the cache then
**	open it and place it in the cache.  If necessary call DI_lru_free
**	to free up a spot in the cache.
**
**	It is assumed that caller has correctly initialized the DI_IO
**	structure.  The DI_IO structure must have the path, pathlength,
**	filename, and filename length filled in.  It must also contain
**	the unique numeric id assigned by DIlru_uniq.  All the previous
**	initialization should take place in DIopen, and the information
**	is stored and saved in the DI_IO struct passed in by the caller
**	on each DI operation.
**
**	If the free list is empty another DI_ENTRY_NUM entries will be
**	allocated and put onto the free list.  The number of entries 
**	allocated continues to grow as long as new files can be open'd
**	successfully.  When opens begin to fail then entries will be
**	closed and put on the free list to be reused.
**
** Inputs:
**	f		 pointer to di file descriptor information.
**	create_flag	 Indicates if file should be created and open file
**			 to it returned.  Used mostly for temporary files.
**
** Outputs:
**	err_code	 system open() error info.
**	fh		 upon exit copy of file descriptor of open file
**			 returned.  Only guaranteed to be valid for current
**			 DI call.
**
**    Returns:
**        OK
**	  DI_CANT_OPEN   Can't open a file due to system limit 
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	11-nov-1999 (somsa01)
**	    If DI_O_OSYNC_MASK and Di_gather_io are set, DI_O_LOG_FILE_MASK
**	    is not set, and if we are dealing with at least 4K pages, add
**	    FILE_FLAG_NO_BUFFERING to the list of file open flags.
**	13-jul-2001 (somsa01)
**	    If II_WRITE_THROUGH_CACHE is set, follow the UNIX model when
**	    deciding when to write through the disk cache.
**	12-dec-2001 (somsa01)
**	    For Win64, follow the UNIX model when deciding when to write
**	    through the disk cache.
**	07-oct-2003 (somsa01)
**	    Only open the gather write file handle if the normal file handle
**	    was successfully obtained.
*/
STATUS
DIlru_inproc_open(
    DI_IO	*f,
    i4 		create_flag,
    CL_ERR_DESC	*err_code )
{
    STATUS		big_status = OK, 
			intern_status = OK, 
			ret_status = OK;
    bool		have_sem = FALSE;
    DWORD		errnum;
    DWORD		attrs, attrs_gw = 0;
    DWORD		access;
    CS_SCB		*scb;
    static bool		Read_Cache_Flag = FALSE;

    /* size includes null and connecting slash */

    char		pathbuf[DI_PATH_MAX + DI_FILENAME_MAX + 2];

    CSget_scb(&scb);

    CL_CLEAR_ERR(err_code);

    big_status = CSp_semaphore(1, &DI_misc_sem);
    if ( big_status != OK )
	intern_status = DI_LRU_PSEM_ERR;
    else
	have_sem = TRUE;

    do
    {
	/*
	** Let's find out if we're following the UNIX convention of
	** selective cache write-through.
	*/
#if !defined(i64_win) && !defined(a64_win)
	if (!Read_Cache_Flag)
	{
	    char	*cp = NULL;

	    NMgtAt( "II_WRITE_THROUGH_CACHE", &cp);
	    if (cp && *cp)
	    {
		if (STcasecmp(cp, "TRUE") == 0)
		    DIunix_cache_style = TRUE;
	    }

	    Read_Cache_Flag = TRUE;
	}
#endif  /* i64_win && a64_win */

	/* 
	** get null terminated path and filename from DI_IO struct 
	*/
	MEcopy((PTR)f->io_pathname, (u_i2) f->io_l_pathname, (PTR) pathbuf);
	pathbuf[f->io_l_pathname] = '\\';
	MEcopy((PTR) f->io_filename, (u_i2) f->io_l_filename, 
	       (PTR) &pathbuf[f->io_l_pathname + 1]);
	pathbuf[(f->io_l_pathname + f->io_l_filename + 1)] = '\0';

	/* 
	** now attempt to open the file 
	*/
	attrs = FILE_FLAG_OVERLAPPED;

	/*
	** Turn off OS file caching/buffering if Di_gather_io,
	** DI_O_OSYNC_MASK are set, if DI_O_LOG_FILE_MASK is
	** not set, and if we are dealing with at least 4K pages.
	*/
	if (f->io_open_flags & DI_O_LOG_FILE_MASK)
	    attrs |= FILE_FLAG_SEQUENTIAL_SCAN;
	else if (f->io_open_flags & DI_O_OSYNC_MASK)
	{
	    /*
	    ** If DIunix_cache_style is turned on, or Gather Write
	    ** is turned on and we're dealing with at least ME_MPAGESIZE
	    ** sized pages (they lie on a system memory page boundary),
	    ** turn off disk cacheing.
	    */
	    if (DIunix_cache_style)
		attrs |= FILE_FLAG_WRITE_THROUGH;

	    if (Di_gather_io && (f->io_bytes_per_page >= ME_MPAGESIZE))
		attrs_gw |= FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH;
	}
	else
	    attrs |= FILE_FLAG_RANDOM_ACCESS;

        access = (f->io_mode == DI_IO_READ?   GENERIC_READ:
                                              GENERIC_READ | GENERIC_WRITE);

	f->io_open_flags |= DI_O_OPEN_IN_PROGRESS;

	have_sem = FALSE;
	big_status = CSv_semaphore( &DI_misc_sem);
	if ( big_status != OK )
	{
	    Di_fatal_err_occurred = TRUE;
	    intern_status = DI_LRU_VSEM_ERR;
	    TRdisplay(
	       "DIlru_inproc: Failed releasing DI semaphore #2,status=%x\n",
	       big_status );
	    break;
	}

        scb->cs_memory |= CS_DIO_MASK;
        scb->cs_state = CS_EVENT_WAIT;

	while ( 
	  (f->io_nt_fh = CreateFile(pathbuf,
                               access,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               (create_flag == DI_FCREATE) ? 
					      CREATE_NEW : OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | attrs,
                               0) ) == INVALID_HANDLE_VALUE)
        {
	    errnum = GetLastError();
	    SETWIN32ERR(err_code, errnum, ER_open);

            big_status = CSp_semaphore(1, &DI_misc_sem);
            if ( big_status != OK )
            {
                big_status =  DI_LRU_PSEM_ERR;
                break;
            }
            else
                have_sem = TRUE;

	    if ( errnum == ERROR_NO_SYSTEM_RESOURCES ||
		 errnum == ERROR_TOO_MANY_OPEN_FILES ||
		 errnum == ERROR_NO_MORE_FILES ||
		 errnum == ERROR_NOT_ENOUGH_MEMORY ||
		 errnum == ERROR_OUTOFMEMORY ||
		 errnum == ERROR_HANDLE_DISK_FULL ||
		 errnum == ERROR_OUT_OF_STRUCTURES ||
		 errnum == ERROR_NONPAGED_SYSTEM_RESOURCES ||
		 errnum == ERROR_PAGED_SYSTEM_RESOURCES )
	    {
		/*
		** Too many open files, close a file and try again
		*/
		big_status = DI_lru_free( err_code );      
		if (big_status != OK )
		{
		    break;
		}
		/*
		** Try and open file again
		*/
		ret_status = OK;
	    }
	    else
	    {
                switch (errnum) 
		{
                case ERROR_PATH_NOT_FOUND:
                        ret_status =  DI_DIRNOTFOUND;
                        break;
                case ERROR_FILE_NOT_FOUND:
                        ret_status =  DI_FILENOTFOUND;
                        break;
                case ERROR_ACCESS_DENIED:
                        ret_status =  DI_ACCESS;
                        break;
		case ERROR_WORKING_SET_QUOTA:
		case ERROR_COMMITMENT_LIMIT:
		case ERROR_DISK_FULL:
			ret_status = DI_EXCEED_LIMIT;
			break;
                case ERROR_CANNOT_MAKE:
                case ERROR_FILE_EXISTS:
                        ret_status = DI_EXISTS;
                        break;
                default:
                        ret_status = (create_flag == DI_FCREATE) ?
			             DI_BADCREATE : DI_BADOPEN;
                        break;
                }

	    }

            have_sem = FALSE;
            big_status = CSv_semaphore( &DI_misc_sem);
            if ( big_status != OK )
            {
                Di_fatal_err_occurred = TRUE;
                intern_status = DI_LRU_VSEM_ERR;
                TRdisplay(
                   "DIlru_inproc: Failed releasing DI semaphore #3, s=%x\n",
                    big_status );
                break;
            }
 
            if ( ret_status != OK )
                break;
        }

	if (ret_status == OK && Di_gather_io)
	{
	    attrs_gw |= attrs;
	    while ( 
		(f->io_nt_gw_fh = CreateFile(pathbuf,
					     access,
					     FILE_SHARE_READ | FILE_SHARE_WRITE,
					     NULL,
					     OPEN_EXISTING,
					     FILE_ATTRIBUTE_NORMAL | attrs_gw,
					     0) ) == INVALID_HANDLE_VALUE)
	    {
		errnum = GetLastError();
		SETWIN32ERR(err_code, errnum, ER_open);

		if (errnum == ERROR_NO_SYSTEM_RESOURCES ||
		    errnum == ERROR_TOO_MANY_OPEN_FILES ||
		    errnum == ERROR_NO_MORE_FILES ||
		    errnum == ERROR_NOT_ENOUGH_MEMORY ||
		    errnum == ERROR_OUTOFMEMORY ||
		    errnum == ERROR_HANDLE_DISK_FULL ||
		    errnum == ERROR_OUT_OF_STRUCTURES ||
		    errnum == ERROR_NONPAGED_SYSTEM_RESOURCES ||
		    errnum == ERROR_PAGED_SYSTEM_RESOURCES)
		{
		    /*
		    ** Too many open files, close a file and try again
		    */
		    big_status = CSp_semaphore(1, &DI_misc_sem);
		    if (big_status != OK)
		    {
			big_status =  DI_LRU_PSEM_ERR;
			break;
		    }
		    else
			have_sem = TRUE;

		    big_status = DI_lru_free( err_code );      
		    if (big_status != OK)
			break;

		    have_sem = FALSE;
		    big_status = CSv_semaphore( &DI_misc_sem);
		    if (big_status != OK)
		    {
			Di_fatal_err_occurred = TRUE;
			intern_status = DI_LRU_VSEM_ERR;
			TRdisplay(
		      "DIlru_inproc: Failed releasing DI semaphore #3, s=%x\n",
		      big_status );
			break;
		    }

		    /*
		    ** Try and open file again
		    */
		    ret_status = OK;
		}
		else
		{
		    switch (errnum) 
		    {
			case ERROR_PATH_NOT_FOUND:
			    ret_status =  DI_DIRNOTFOUND;
			    break;

			case ERROR_FILE_NOT_FOUND:
			    ret_status =  DI_FILENOTFOUND;
			    break;

			case ERROR_ACCESS_DENIED:
			    ret_status =  DI_ACCESS;
			    break;

			case ERROR_WORKING_SET_QUOTA:
			case ERROR_COMMITMENT_LIMIT:
			case ERROR_DISK_FULL:
			    ret_status = DI_EXCEED_LIMIT;
			    break;

			case ERROR_CANNOT_MAKE:
			case ERROR_FILE_EXISTS:
			    ret_status = DI_EXISTS;
			    break;

			default:
			    ret_status = DI_BADOPEN;
			    break;
		    }

		}

		if (ret_status != OK)
		    break;
	    }
	}

        scb->cs_memory &= ~CS_DIO_MASK;
        scb->cs_state = CS_COMPUTABLE;

	/*
	** If a big error occurred don't bother trying to continue
	*/
	if ( big_status != OK )
	    break;

	big_status = CSp_semaphore(1, &DI_misc_sem);
	if ( big_status != OK )
	{
	    intern_status = DI_LRU_PSEM_ERR;
	    break;
	}
	else
	    have_sem = TRUE;
    
	/*
	** If we successfully opened the file add it to the cache
	*/
	if ( ret_status == OK )
	{
            QUinsert((QUEUE*)f, &DI_lru_queue);
	    f->io_queue = DI_IO_LRU_QUEUE;
	}

	f->io_open_flags &= ~(DI_O_I_WANT|DI_O_OPEN_IN_PROGRESS);

    } while (FALSE);

    if ( big_status != OK )
	ret_status = big_status;

    /*
    ** Make sure we have cleaned everything up before
    ** we return. IF we fail releasing a semaphore overwrite any 
    ** other error as this is going to bring the server down and
    ** we want to record why it came down.
    */
    if ( have_sem )
    {
        big_status = CSv_semaphore( &DI_misc_sem);
	if ( big_status != OK )
	{
	    ret_status = big_status;
	    intern_status = DI_LRU_VSEM_ERR;
	    Di_fatal_err_occurred = TRUE;
	    TRdisplay(
	       "DIlru_inproc: Failed releasing DI semaphore #5, s=%x\n",
	        big_status );
	}
    }

    if ( ret_status != OK )
	DIlru_set_di_error( &ret_status, err_code,intern_status, 
			    DILRU_INPROC_ERR );
    return(ret_status);

}

/*{
** Name: DIlru_flush - Close a number of DI files using lru algorithm.
**
** Description:
**      Will close an arbitrary number of DI file using a lru algorithm.  
**	Can be called from anywhere in the CL.  This routine may be called 
**	by any CL routine called by the dbms which fails while trying to 
**	open a file because there are no available file descriptors or
**	other resources.
**
**      The last entry in the lru queue is chosen as the victim.  That
**      file descriptor is closed.  
**
** Inputs:
**      none.
**
** Outputs:
**      none.
**
**    Returns:
**        OK                Successfully closed file, freeing a file descriptor.
**        DI_LRU_ALL_PINNED_ERR  All files in use and can't be closed.
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**	13-mar-1998 (canor01)
**	    Semaphore on closed file was being held.  Make sure to release it.
*/

STATUS
DIlru_flush( CL_ERR_DESC *err_code )
{
    STATUS		ret_status = OK, intern_status, big_status = OK;
    bool		have_sem = FALSE;
    bool		closed_one = FALSE;
    DI_IO               *f;
    i4 			n = 16;


    do
    {
        ret_status = DIlru_check_init( &intern_status, err_code );
        if ( ret_status != OK )
            break;

	while (n--)
        {
	    big_status = CSp_semaphore(1, &DI_misc_sem);
	    if ( big_status != OK )
	    {
		intern_status = DI_LRU_PSEM_ERR;
		break;
	    }
	    if ( (f = (DI_IO *)DI_lru_queue.q_prev) == (DI_IO *)&DI_lru_queue )
	    {
		ret_status = DILRU_ALL_PINNED_ERR;
	    }
	    /* Before getting the file sem, release LRU queue list sem.
	    ** LRU list sem can only be gotten after getting file sem
	    ** otherwise it is possible that DIrename and DIclose 
	    ** will deadlock.
	    */
            big_status = CSv_semaphore( &DI_misc_sem);
            if ( big_status != OK )
            {
                intern_status = DI_LRU_VSEM_ERR;
                Di_fatal_err_occurred = TRUE;
                TRdisplay(
                "%@ DIlru_flush:1:Failed releasing DI semaphore, status=%x\n",
                  big_status);
                break;
            }
	    if ( ret_status == DILRU_ALL_PINNED_ERR )
		break;
	    if (! have_sem )
	    {
                CS_synch_lock(&f->io_sem);
	        have_sem = TRUE;
	    }
	    /* Check that the file is on the list before 
	    ** doing anything with it.
	    */
	    if (f->io_queue == DI_IO_LRU_QUEUE)
	    {
		/* Re-get the LRU list sem */ 
		big_status = CSp_semaphore(1, &DI_misc_sem); 
		if ( big_status != OK )
		{
			intern_status = DI_LRU_PSEM_ERR;
			if ( have_sem )
			{
				have_sem = FALSE;
				CS_synch_unlock(&f->io_sem);
			}
			break;
		}
		if ( f->io_nt_fh != 0 && 
		     f->io_nt_fh != INVALID_HANDLE_VALUE &&
		     f->io_refcnt == 0 )
		{
                    FlushFileBuffers(f->io_nt_fh);
		    CloseHandle(f->io_nt_fh);
		    f->io_nt_fh = INVALID_HANDLE_VALUE;
		    if (f->io_nt_gw_fh != 0 &&
			f->io_nt_gw_fh != INVALID_HANDLE_VALUE)
		    {
			CloseHandle(f->io_nt_gw_fh);
			f->io_nt_gw_fh = INVALID_HANDLE_VALUE;
		    }

		    if(QUremove((QUEUE *)f) == NULL)
			QUinit((QUEUE *)f);
                    ret_status = OK;
		    if ( !closed_one )
			closed_one = TRUE;
		    f->io_queue = DI_IO_NO_QUEUE;

		}
		big_status = CSv_semaphore( &DI_misc_sem);
		if ( big_status != OK )
		{
		    intern_status = DI_LRU_VSEM_ERR;
		    Di_fatal_err_occurred = TRUE;
		    TRdisplay(
                    "%@ DIlru_flush:2:Failed releasing DI semaphore, status=%x\n",
			big_status);
		    if ( have_sem )
		    {
			have_sem = FALSE;
			CS_synch_unlock(&f->io_sem);
		    }
		    break;
		}	
	    }
	    if ( have_sem )
	    {
		have_sem = FALSE;
                CS_synch_unlock(&f->io_sem);
	    }	
        }
    } while (FALSE);

    if ( closed_one )
	ret_status = OK;

    if ( big_status != OK )
	ret_status = big_status;

    if ( ret_status != OK )
	DIlru_set_di_error( &ret_status, err_code, intern_status,
			    DILRU_RELEASE_ERR);

    return( ret_status );
}

/*{
** Name: DIlru_set_di_error - formats err_code to pass back failure info.
**
** Description:
**
**	This routine tries to pass back as much usefull information in the 
**	err_code structure on the failure as possible. 
**
** Inputs:
**	ret_status	The error returned from DI.
**	intern_err      Value to set err_code.intern to.
**
** Outputs:
**	err_code	ptr to err_code to return to called of DI
**	meaningfull_err	Error message to set ret_val to if ret_val is
**			the generic FAIL message.
**
**    Returns:
**        OK
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-October-1992 (rmuth)
** 	    Created.
**
*/
VOID
DIlru_set_di_error(
    i4          *ret_status,
    CL_ERR_DESC *err_code,
    i4          intern_err,
    i4          meaningfull_err )
{

    err_code->intern = intern_err;

    if ( *ret_status == FAIL )
	*ret_status = meaningfull_err;

    return;
}

/*{
** Name: DIlru_check_init - Make sure DILRU has been initialised if not
**			    initialise it.
**
** Description:
**
** Inputs:
**	Number		Index into Di_slave_cnt array
**
** Outputs:
**	intern_status	Internal error indicating reason for failure.
**
**    Returns:
**        OK
**	  DI_LRU_PSEM_ERR
**	  DI_LRU_VSEM_ERR
**	  A range of DIlru errors from DIlru_init.
**
**    Exceptions:
**        none
**
** Side Effects:
**	  If all ok it returns leaving an outstanding P operation.
**
** History:
**    30-nov-1992 (rmuth)
** 	    Created.
**	07-Oct-1997 (jenjo02)
**	    Adjust check so that we don't have to take the 
**	    semaphore in the normal (already-initialized) case.
*/
static STATUS
DIlru_check_init( 
    STATUS	*intern_status,
    CL_ERR_DESC *err_code )
{
    STATUS	status = OK;

    /*
    ** See of we need to initialize the hash control block
    */
    if (DI_lru_initialized)
	return(OK);

    /* 
    ** Initialise DILRU 
    */
    status = DIlru_init( err_code );

    return( status );
		
}

/*{
** Name: DI_lru_free - Close a DI file using lru algorithm.
**
** Description:
**	Will close a DI file using a lru algorithm.  Can be called from
**	anywhere in the CL.  This routine will be called by any CL routine
**	called by the dbms which fails while trying to open a file because
**	there are no available file descriptors.  Callers should loop
**	if they encounter the same out of file descriptor error until 
**	DI_lru_free indicates that it can't close any more files.
**
**	The last entry in the lru queue is chosen as the victim.  That 
**	file descriptor is closed.  The entry is put on the free list.
**
**	NOTE: this is now only used when DI is operating in the main
**		process and not a slave async model.
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**    Returns:
**        OK		    Successfully closed file, freeing a file descriptor.
**	  DI_CANT_CLOSE	    No available file descriptors to close.
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    06-mar-87 (mmm)
**          Created new for jupiter.
**    30-nov-1992 (rmuth)
**	    - Do not put the Di_fatal_err_occurred check in this routine as
**	      it can be called by other DIlru routines and hence may stop
**	      the trace back message being issued.
**	    - Track close failure.
**	    - Prototype.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x into %p for pointer values.
**    12-may-1994 (rog)
**	    Reset the file's sticky bit when we close the file if it was
**	    turned off.
**	04-Nov-1997 (jenjo02)
**	    Caller must hold DI_misc_sem. Changed to static function to 
**	    distinguish from external DIlru_free() callers who do not
**	    hold the mutex.
*/
static STATUS
DI_lru_free( 
    CL_ERR_DESC *err_code)
{
    STATUS		status = OK;
    DI_IO		*f;

    status = DILRU_ALL_PINNED_ERR;
    while ( (f = (DI_IO *)DI_lru_queue.q_prev) != (DI_IO *)&DI_lru_queue )
    {
        CS_synch_lock(&f->io_sem);
        if ( f->io_nt_fh != 0 && 
	     f->io_nt_fh != INVALID_HANDLE_VALUE &&
	     f->io_refcnt == 0 )
        {
	    FlushFileBuffers(f->io_nt_fh);
            CloseHandle(f->io_nt_fh);
            f->io_nt_fh = INVALID_HANDLE_VALUE;
	    if (f->io_nt_gw_fh != 0 && f->io_nt_gw_fh != INVALID_HANDLE_VALUE)
	    {
		CloseHandle(f->io_nt_gw_fh);
		f->io_nt_gw_fh = INVALID_HANDLE_VALUE;
	    }

	    if (QUremove((QUEUE *)f) == NULL)
		QUinit((QUEUE *)f);
	    f->io_queue = DI_IO_NO_QUEUE;
	    status = OK;
            CS_synch_unlock(&f->io_sem);
	    break;
        }
        CS_synch_unlock(&f->io_sem);
    }
    return(status);
}
