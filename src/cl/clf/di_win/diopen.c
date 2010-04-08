/******************************************************************************
**
** Copyright (c) 1987, 2003 Ingres Corporation
**
******************************************************************************/

#define    INCL_DOSERRORS

#include <compat.h>
#include <cs.h>
#include <di.h>
#include <er.h>
#include <me.h>
#include <st.h>
#include "dilru.h"

/* # defines */

/* typedefs */

/* forward references */

/* externs */

GLOBALREF i4 Di_moinited;	/* in DIMO */

/* statics */

/******************************************************************************
**
** Name: DIopen - Opens a file.
**
** Description:
**      The DIopen routine is used to open a direct access file.
**      It will open the file for read or write access and will
**      place no locks on the file.
**      If the path does not exists the error DI_DIRNOTFOUND
**      must be returned before returning DI_FILENOTFOUND.
**      If possible the operating system should check that the
**      size at open is the same as the size at create.  If
**      they don't match it should return DI_BADPARAM.
**
** Inputs:
**      f                    Pointer to the DI file
**                           context needed to do I/O.
**      path                 Pointer to the directory name
**                           for the file.
**      pathlength           Length of path name.
**      filename             Pointer to file name.
**      filelength           Length of file name.
**      pagesize             Value indicating size of page
**                           in bytes. Must match size at create.
**      mode                 Value indicating the access mode.
**                           This must be DI_IO_READ or DI_IO_WRITE.
**      flags                Used to alter system specific behaviour of I/O.
**                           If a unix vendor provides alternate control of
**                           disk I/O though new system calls and flags, it
**                           may be necessary to add to these flags.
**
**                              DI_FSYNC_MASK   on unix systems which support
**                                              the fsync() system call use it
**                                              to guarantee consistency of
**                                              writes.
**
**                              DI_OSYNC_MASK   on unix systems which support
**                                              an option to the open() system
**                                              call which causes writes to disk
**                                              to be synchronous, use this
**                                              feature to guarantee consistency
**                                              of writes.
**
** Outputs:
**      f                    Updates the file control block.
**      err_code             Pointer to a variable used
**                           to return operating system
**                           errors.
**
** Returns:
**      OK
**      DI_BADDIR        Path specification error.
**      DI_BADOPEN       Error openning file.
**      DI_BADFILE       Bad file context.
**      DI_BADPARAM      Parameter(s) in error.
**      DI_DIRNOTFOUND   Path not found.
**      DI_EXCEED_LIMIT  Open file limit exceeded.
**      DI_FILENOTFOUND  File not found.
**
** Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**      10-jul-1995 (canor01)
**          save the pathname and filename in the DI_IO structure for
**          possible later use by DIdelete()
**      18-jul-1995 (reijo01)
**          Changed SETWIN32ERR so that it will populate the CL_ERR_DESC with
**              the proper values.
**	19-jul-95 (emmag)
**	    Open file with FILE_FLAG_OVERLAPPED since we'll now be using an
**	    overlapped structure in the async I/O.
**	30-aug-1995 (shero03)
**	    Call DImo_init.
**	05-dec-1995 (canor01)
**	    Initialize and name DI_IO semaphore on file open.
**	24-feb-1997 (cohmi01)
**	    Initialize err_code, return DI_EXCEED_LIMIT for additional limits
**	    that may be reached, incl. inability to allocate mutex. (b80242)
**	04-mar-1997 (canor01)
**	    cs_thread_ordinal member of semaphore is now cs_sid.
**	02-sep-1997 (wonst02)
**	    Added code to return DI_ACCESS for file open access error.
**      08-dec-1997 (canor01)
**          Implement LRU for open files (initial copy from Unix). Move the
**	    actual file open into dilru.c.
**	28-jan-1998 (canor01)
**	    Simplify the LRU code for NT by keeping all open files on a
**	    queue.
**      12-feb-1998 (canor01)
**          Increment reference count and release semaphore while doing
**          the actual read/write.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	12-nov-1999 (somsa01)
**	    Set DI_O_LOG_FILE_MASK if DI_LOG_FILE_MASK is set.
**	24-jul-2000 (somsa01)
**	    cs_sid is now cs_thread_id.
**	23-sep-2003 (somsa01)
**	    Initialize f->io_nt_gw_fh to INVALID_HANDLE_VALUE.
**      20-apr-2005 (rigka01) bug 114010, INGSRV3180
**        Sessions hang in a mutex deadlock on DI LRU SEM and
**        DI <filename> SEM.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**
******************************************************************************/
STATUS
DIopen(DI_IO      *f,
       char       *path,
       u_i4       pathlength,
       char       *filename,
       u_i4       filelength,
       i4         pagesize,
       i4         mode,
       u_i4       flags,
       CL_SYS_ERR *err_code)
{
    STATUS   ret_val = OK;
    CS_SCB	*scb;

    CLEAR_ERR(err_code);

    if (!Di_moinited)
         DImo_init();

    /* Check input parameters. */

    if ((pathlength > DI_PATH_MAX) ||
        (pathlength == 0) ||
        (filelength > DI_FILENAME_MAX) ||
        (filelength == 0) ||
        (mode != DI_IO_READ &&
         mode != DI_IO_WRITE)) {
    	return (DI_BADPARAM);
    }

    CSget_scb(&scb);

    /* save path and filename in DI_IO struct to cache open file descriptors. */
    MEcopy((PTR) path, (u_i2) pathlength, (PTR) f->io_pathname);
    f->io_l_pathname = pathlength;
    MEcopy((PTR) filename, (u_i2) filelength, (PTR) f->io_filename);
    f->io_l_filename = filelength;

    f->io_system_eof     = 0xffffffff;
    f->io_type           = DI_IO_ASCII_ID;
    f->io_mode           = mode;
    f->io_bytes_per_page = pagesize;

    f->io_refcnt = 0;

    f->io_open_flags = 0;

    f->io_queue = DI_IO_NO_QUEUE;

    if ( flags & DI_SYNC_MASK )
	f->io_open_flags |= DI_O_OSYNC_MASK;
    else if ( flags & DI_USER_SYNC_MASK )
	f->io_open_flags |= DI_O_USYNC_MASK;

    if ( flags & DI_LOG_FILE_MASK )
	f->io_open_flags |= DI_O_LOG_FILE_MASK;

    CS_synch_init(&f->io_sem);

    CS_synch_lock(&f->io_sem);

    QUinit((QUEUE *)f);

    /* assign a unique id to this file (number is unique to the current server).
    ** This unique id is used in subsequent DI calls to identify the file in
    ** question without having to do comparisons on full path and file name.
    */
    do
    {
	f->io_nt_fh = f->io_nt_gw_fh = INVALID_HANDLE_VALUE;
 
        /*
        ** open this file, closing another DI file if necessary
        ** (ie. if we have run out of open file descriptors.)
        ** NOTE: We passed NORETRY here even though the above comment
        ** contradicts this, changed call so that we RETRY.
        */
 
        ret_val = DIlru_open(f, FALSE, err_code);
        if ( ret_val != OK )
            break;
 
        f->io_alloc_eof = -1;
 
    } while (FALSE);

    CS_synch_unlock(&f->io_sem);


    if ( ret_val != OK )
    {
        CS_synch_destroy( &f->io_sem);

        /* for sanity mark DI_IO as not a valid open file */
 
        f->io_type = DI_IO_OPEN_INVALID;
    }

    return (ret_val);
}
