/*
** Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <rusage.h>
#include    <fdset.h>
#ifdef xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif /* xCL_006_FCNTL_H_EXISTS */
#ifdef xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif /* xCL_007_FILE_H_EXISTS */
#include    <diracc.h>
#include    <errno.h>
#include    <er.h>
#include    <cs.h>
#include    <csev.h>
#include    <me.h>
#include    <tr.h>
#include    <tm.h>
#include    <sys/stat.h>
#include    <seek.h>
#include    <clnfile.h>
#include    <cldio.h>
#include    <di.h>
#include    <dislave.h>
#include    "dilocal.h"
#include    "dilru.h"

/**
**
**  Name: DISLAVE.C - DI slave code.
**
**  Description:
**	This file defines all code for DI that wil exist in the slave
**	process image.  It should only be linked in slave processes.
**
**          DI_init_slave() - do DI specific slave initialization
**	    DI_slave()	    - Slave portion of DI event handleing.
**
**
**  History:
**      01-mar-88 (anton)
**          Created.
**	06-feb-89 (mikem)
**	    use new member unix_open_flags instead of length.  Define
**	    OFFSET_TYPE for lint usage.
**	28-Feb-1989 (fredv)
**	    Re-arranged the headers order. Use <systypes.h> and <diracc.h>
**		instead of <sys/types.h> and <dirac.h>. Added ifdefs around 
**		<sys/file.h>. Inlcude <seek.h> and <fdset.h>.
**	25-Apr-89 (GordonW)
**	    lseek returns a "OFFSET_TYPE" not an int.
**	6-Feb-90 (jkb)
**	    Change reads,writes ... to IIdio_read, IIdio_write,... to
**          combine lseeks and reads/writes and make direct io available
**	    for Sequent.
**	    Change fdseeks to a pointer and dynamically allocate memory for
**	    it.  If setdtablesize is used more than NOFILE of fds may be
**          available.
**	16-Mar-90 (anton)
**	    IIdio_{open,read,write} all set CL_ERR_DESC so we should not
**	    Added some xDEV_TST TRdisplays
**	15-may-90 (blaise)
**	    Integrated changes from 61 & ug:
**		If O_SYNC is defined, combine it into unix_open_flags
**      27-jul-91 (mikem)
**          bug #38872
**	    A previous integration made changes to the DI_SYNC_MASK was handled
**	    and the result of that change was that fsync() was enabled on all i
**	    DIforce() calls on platforms which include a fsync() option.  The
**	    change to fix this bug change affects diopen.c, dilru.c, dislave.c,
**          dislave.h, and diforce.c.  It reintegrates the old mechanism where
**          DI_SYNC_MASK is only used as a mechanism for users of DIopen() to
**          indicate that the files they have specified must be forced to disk
**          prior to return from a DIforce() call.  One of the constants
**          DI_O_OSYNC_MASK or DI_O_FSYNC_MASK is then stored in the DI_IO
**          structure member io_open_flags.  These constants are used by the
**          rest of the code to determine what kind of syncing, if any to
**          perform at open/force time.
**	30-oct-1991 (bryanp)
**	    Added support for DI_SL_MAPMEM, and for direct slave access to
**	    server shared memory. We are now able to read and write buffers
**	    directly to/from the server shared memory, avoiding the intermediate
**	    copy to the server segment.
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**      10-dec-91 (jnash)
**          Move SENSE code into handy to make it work for raw logs.  Create
**          handy IIdio_get_file_eof function.
**      03-mar-1992 (jnash)
**          Fix LG slave problem noted when Sun mmap() support
**          introduced, change slave logic to send to the slave the
**          segment id "key" rather than "segid" (segid value not
**          the same in the slave).
**	30-October-1992 (rmuth)
**	    Remove the ZPAD slave function and add the DI_SL_GUARANTEE
**	    function.
**      05-Nov-1992 (mikem)
**          su4_us5 port.  Changed direct calls to CL_NFILE to calls to new
**          function iiCL_get_fd_table_size(). 
**	03-jan-1992 (rmuth)
**	    For the DI_SL_ZALLOC function use IIdio_get_file_eof function.
**      10-mar-1992 (mikem) integrated 6.4 change into 6.5: 29-oct-92 (mikem)
**          bug #47624
**          Bug 47624 resulted in CSsuspend()'s from the DI system being woken
**          up early.  Mainline DI would then procede while the slave would
**          actually be processing the requested asynchronous action.  Various
**          bad things could happen after this depending on timing: (mainline
**          DI would change the slave control block before slave read it,
**          mainline DI would call DIlru_release() and ignore it failing which
**          would cause the control block to never be freed eventually leading
**          to the server hanging when it ran out of slave control blocks, ...
**
**          Fixes were made to scf to hopefully eliminate the unwanted
**          CSresume()'s.  In addition defensive code has been added to DI
**          to catch cases of returning from CSresume while the slave is
**          operating, and to check for errors from DIlru_release().  Before
**          causing a slave to take action the master will set the slave
**          control block status to DI_INPROGRESS, the slave in turn will not
**          change this status until it has completed the operation.
**	30-feb-1993 (rmuth)
**	    Add include for dilocal.h, so that we can use the global
**	    DIzero_buffer instead of our local copy as this structure is
**	    large. Also use DI_ZERO_BUFFER_SIZE as opposed to 
**	    DI_FILE_BUF_SIZE which is used for normal read/write operations.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jul-1993 (mikem)
**	    Added DI_COLLECT_STATS operation to collect TMperfstat() info
**	    in a slave and pass it back to the server.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in DI_slave().
**	13-nov-1995 (schte01)
**		Removed register notation on disl variable for axp_osf due to
**		register corruption.
**	24-jan-1996 (nick)
**	    Open and truncate the file we are about to delete.  #47513
**	24-Nov-1997 (allmi01)
**	    Bypass the function definition for lseek on Silicon Graphics
**	    (sgi_us5) as it already exists in <unistd.h>.
**	05-Nov-1997 (hanch04)
**	    Remove declare of lseek, done in seek.h
**          Added LARGEFILE64 for files > 2gig
**          Change i4 to OFFSET_TYPE
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved GLOBALREFs to dilocal.h
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	28-feb-2000 (somsa01)
**	    Cleaned up typecast of stat when LARGEFILE64 is defined.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Nov-2005 (kschendel)
**	    Make zeroing buffer size a config parameter.
**/

/*
**  Forward and/or External typedef/struct references.
*/

/* externs */


/*
**  Static variables
*/
static	OFFSET_TYPE *fdseeks;


/*{
** Name: DI_init_slave() - do DI specific slave initialization.
**
** Description:
**	Setup slave for DI processing.  Right now, just clear umask.
**
** Inputs:
**      fd_msk			mask of file descriptors passed from server
**				(ignored)
**	num_fds			number of bits in mask (ignored)
**
** Outputs:
**
**	Returns:
**	    OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-mar-88 (anton)
**          Created.
**	6-Feb-90 (jkb)
**	    Allocate memory for fdseeks to accommodate maximum number of fd's
**	8-Feb-90 (anton)
**	    Call MEreqmem properly and test for failure
**	    Also, add num_fds parameter - not used but passed
*/
/* ARGSUSUED */
STATUS
DI_init_slave(fd_msk, num_fds)
fd_set	fd_msk;
i4	num_fds;
{
    (void) umask(0);

    /* handle as many fd's as we can */
    fdseeks = (OFFSET_TYPE *) MEreqmem ((u_i4)0, 
		   	   (u_i4)(iiCL_get_fd_table_size() * sizeof(*fdseeks)),
			   TRUE, (STATUS *)NULL);
    if (fdseeks == NULL)
	return(FAIL);

    /* Allocate a zeroing buffer.  For slaves, don't bother with reading
    ** any config params, just use the default size.
    */
    Di_zero_buffer = MEreqmem(0, DI_ZERO_BUFFER_SIZE+ME_MPAGESIZE, TRUE, NULL);
    if (Di_zero_buffer == NULL)
	return (FAIL);
    Di_zero_buffer = ME_ALIGN_MACRO(Di_zero_buffer, ME_MPAGESIZE);
    Di_zero_bufsize = DI_ZERO_BUFFER_SIZE;
    return (OK);
}

/*{
** Name: DI_slave() - Slave portion of DI event handleing.
**
** Description:
**	This routine maps DI event control blocks to UNIX system calls
**	in order to allow server processes to do asyncronous I/O.
**	Read and write operations have pre-seek operations available
**	since it is possible that two sessions in a server would fight over
**	file positions.  The open operation has an lru parameter allowing
**	a file to be lru'ed out durring the same operation as an open
**	since multiply sessions may be doing opens with lru required at
**	the same time.
**
**	The list directory Di operation is done in server at high cost.
**	But servers are not supposed to do that.
**
**	DI has been enhanced so that as much of the I/O as possible is done
**	from shared memory, and DI now passes READ and WRITE requests as
**	(segment ID,offset) pairs, where the segment may be the server segment
**	or it may be a mainline segment such as the DMF buffer cache. We are
**	now able to read and write buffers directly to/from the server shared
**	memory, avoiding the intermediate copy to the server segment.
**
**      It is important for disk slave operations to not change disl->status
**      field until the operation has been completed.  Asynchronous callers
**      of this routine will initialize the disl->status field to DI_INPROGRESS
**      and expect this status to not change until the operation is complete.
**      This protocol is currently used to catch the error case where the
**      caller is CSresume()'d prematurely.
**
** Inputs:
**	evcb				CS event control block
**
** Outputs:
**	evcb				results placed in control block
**
**	Returns:
**	    OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    All sorts of file operations take place.
**
** History:
**      25-mar-88 (anton)
**          Created.
**	13-mar-89 (russ)
**	    Changing FSYNC_EXISTS ifdef to xCL_010_FSYNC_EXISTS.
**	6-Feb-90 (jkb)
**	    Change reads,writes ... to IIdio_read, IIdio_write,... to
**          combine lseeks and reads/writes and make direct io available
**	    for Sequent.
**	16-Mar-90 (anton)
**	    IIdio_{open,read,write} all set CL_ERR_DESC so we should not
**	    Added some xDEV_TST TRdisplays
**	30-oct-1991 (bryanp)
**	    Added support for DI_SL_MAPMEM, and for direct slave access to
**	    server shared memory. This basically involved changes to the
**	    READ and WRITE operations so that the target address is passed as
**	    a (segment ID,offset) pair, which is translated back into an
**	    address in the slave. This address may be in the server segment,
**	    or it may be in some mainline segment such as the DMF buffer cache.
**	10-dec-91 (jnash)
**	    Move SENSE code into handy to make it work for raw logs.  Create
**	    IIdio_get_file_eof function.
**      03-mar-1992 (jnash)
**          Fix problem noted when mmap() support introduced, change
**          slave logic to use the DI_SLAVE_CB "key" field rather than
**          "segid" to obtain segment of interest (segid value not the
**          same in the slave as in sender).
**	06-apr-1992 (jnash)
**	    Change call to MEget_pages to not request memory
**	    locking via ME_LOCK_MASK.  This flag is used for locking DMF
**	    cache only.
**	30-October-1992 (rmuth)
**	    - Remove ZPAD.
**	    - Add DI_SL_GUARANTEE,this seeks to the requested position and then
**	      writes the requested amount of zero filled space.
**	    - Change the DI_SL_MAPMEM TRdisplay message to xDEV_TST instead
**	      of xDEBUG.
**	03-jan-1992 (rmuth)
**	    For the DI_SL_ZALLOC function use IIdio_get_file_eof function.
**      10-mar-1992 (mikem) integrated 6.4 change into 6.5: 29-oct-92 (mikem)
**          bug #47624
**          Bug 47624 resulted in CSsuspend()'s from the DI system being woken
**          up early.  Mainline DI would then procede while the slave would
**          actually be processing the requested asynchronous action.  Various
**          bad things could happen after this depending on timing: (mainline
**          DI would change the slave control block before slave read it,
**          mainline DI would call DIlru_release() and ignore it failing which
**          would cause the control block to never be freed eventually leading
**          to the server hanging when it ran out of slave control blocks, ...
**
**          Fixes were made to scf to hopefully eliminate the unwanted
**          CSresume()'s.  In addition defensive code has been added to DI
**          to catch cases of returning from CSresume while the slave is
**          operating, and to check for errors from DIlru_release().  Before
**          causing a slave to take action the master will set the slave
**          control block status to DI_INPROGRESS, the slave in turn will not
**          change this status until it has completed the operation.
**	30-feb-1993 (rmuth)
**	    Use the global DIzero_buffer as opposed to the local zero_buffer.
**	    Also use DI_ZERO_BUFFER_SIZE as opposed to DI_FILE_BUF_SIZE 
**	    which is used for normal read/write operations.
**	26-jul-1993 (mikem)
**	    Added DI_COLLECT_STATS operation to collect TMperfstat() info
**	    in a slave and pass it back to the server.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	12-may-1994 (rog)
**	    When we close a file, reset the sticky bit if necessary.
**	24-jan-1996 (nick)
**	    Attempt to open the file we have been requested to delete
**	    using O_TRUNC then immediately close.  This ensures that another
**	    process with this file open won't delay the reclamation of the
**	    disk space associated with the file. #47513
**       4-Dec-2002 (hanal04) Bug 107709 INGSRV 1770
**          If we have LARGEFILE64 support we need to call lseek64()
**          to avoid EINVAL from lseek() which does not support
**          large file sizes.
**  01-Apr-2004 (fanch01)
**      Add O_DIRECT support on Linux depending on the filesystem
**      properties, pagesize.  Fixups for misaligned buffers on read()
**      and write() operations.  Added page size parameter to IIdio_open().
**	14-Oct-2005 (jenjo02)
**	    Chris's file descriptor properties now cached in io_fprop
**	    (file properties) and established on the first open, 
**	    not every open.
**	7-Mar-2007 (kschendel) SIR 122512
**	    Perform delete with dummy fprop, don't care about file/fs
**	    properties on a file that is going away.
*/

STATUS
DI_slave(evcb)
CSEV_CB	*evcb;
{
#ifdef axp_osf
    DI_SLAVE_CB *disl;
#else
    register DI_SLAVE_CB	*disl;
#endif /* axp_osf */
    int				i = 0;
    i4			opcode;
    OFFSET_TYPE			lseek_ret;
    PTR				segaddr;
    SIZE_TYPE			allocated_pages;
    char			*effective_buf_addr;
    int				buf_size;
    STATUS			local_status;

    disl = (DI_SLAVE_CB *)evcb->data;

    CL_CLEAR_ERR( &disl->errcode );
    local_status = OK;

    opcode = (disl->file_op & ~DI_NOSUSPEND);

    switch (opcode)
    {
    case DI_SL_READ:

	if ((local_status = ME_offset_to_addr(disl->seg_key, disl->seg_offset,
						&effective_buf_addr)) != OK)
	{
	    TRdisplay("DI_SL_READ: seg_key %s, off %d, error=%x\n",
				disl->seg_key, disl->seg_offset, local_status);
	    break;
	}
#ifdef	xDEV_TST
	TRdisplay("DI_SL_READ: seg_key %s, off %d, maps to %p\n",
				disl->seg_key, disl->seg_offset,
				effective_buf_addr);
#endif

	if ((i = IIdio_read(disl->unix_fd, effective_buf_addr, disl->length,
		(OFFSET_TYPE)disl->pre_seek, &fdseeks[disl->unix_fd], 
		disl->io_fprop,
		&disl->errcode)) < 0)
	{
	    local_status = FAIL;
	}
	else
	{
	    disl->length = i;
	}
	break;

    case DI_SL_WRITE:
        if ((local_status = ME_offset_to_addr(disl->seg_key, disl->seg_offset,
                                                &effective_buf_addr)) != OK)
	{
	    TRdisplay("DI_SL_WRITE: seg_key %s, off %d, error=%x\n",
				disl->seg_key, disl->seg_offset, local_status);
            break;
	}
#ifdef	xDEV_TST
	TRdisplay("DI_SL_WRITE: seg_key %s, off %d, maps to %p\n",
				disl->seg_key, disl->seg_offset,
				effective_buf_addr);
#endif

	if ((i = IIdio_write(disl->unix_fd, effective_buf_addr, disl->length, 
		(OFFSET_TYPE)disl->pre_seek, &fdseeks[disl->unix_fd], 
		disl->io_fprop,
		&disl->errcode)) < 0)
	{
	    local_status = FAIL;
	}
	else
	{
	    disl->length = i;
	}
	break;

    case DI_SL_OPEN:
	if (disl->lru_close >= 0)
	{
#if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
	    /* Reset the sticky bit in case we turned it off. */
	    if (!Di_no_sticky_check)
	    {
#ifdef LARGEFILE64
		struct stat64 sbuf;

		if (!fstat64(disl->lru_close, &sbuf))
#else /* LARGEFILE64 */
		struct stat sbuf;

		if (!fstat(disl->lru_close, &sbuf))
#endif /*  LARGEFILE64 */
		{
		    int new_perms = (unsigned)sbuf.st_mode & ~S_IFMT;

		    new_perms |= S_ISVTX;
		    (VOID) fchmod(disl->lru_close, new_perms);
		}
	    }
#endif
#ifdef	xDEV_TST
	    TRdisplay("DI LRU CLOSE fd %d", disl->lru_close);
#endif
	    if (close(disl->lru_close) < 0)
		disl->lru_close = errno;
	    else
		disl->lru_close = -1;
#ifdef	xDEV_TST
	    TRdisplay(" result %d\n", disl->lru_close);
#endif
	}
	if ((disl->unix_fd =
	     IIdio_open(disl->buf, disl->unix_open_flags, disl->pre_seek,
			 disl->length, 
			 &disl->io_fprop,
			 &disl->errcode)) < 0)
	{
	    local_status = FAIL;
	}
	else
	{
	    fdseeks[disl->unix_fd] = 0;
	}
	break;

    case DI_SL_CLOSE:
#if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
	/* Reset the sticky bit in case we turned it off. */
	if (!Di_no_sticky_check)
	{
#ifdef LARGEFILE64
	    struct stat64 sbuf;

	    if (!fstat64(disl->unix_fd, &sbuf))
#else /* LARGEFILE64 */
	    struct stat sbuf;

	    if (!fstat(disl->unix_fd, &sbuf))
#endif /*  LARGEFILE64 */
	    {
		int new_perms = (unsigned)sbuf.st_mode & ~S_IFMT;

		new_perms |= S_ISVTX;
		(VOID) fchmod(disl->unix_fd, new_perms);
	    }
	}
#endif
	if (close(disl->unix_fd) < 0)
	{
	    SETCLERR(&disl->errcode, 0, ER_close);
	    local_status = FAIL;
	}

#ifdef	xDEV_TST
	TRdisplay("DI_SL_CLOSE fd %d status %d errno %d\n", disl->unix_fd,
		  local_status, errno);
#endif
	break;

    case DI_SL_SENSE:
	if (( lseek_ret = IIdio_get_file_eof(disl->unix_fd,
				disl->io_fprop)) < 0)
	{
	    SETCLERR(&disl->errcode, 0, ER_lseek);
	    local_status = DI_BADINFO;
	}
	else
	{
	    fdseeks[disl->unix_fd] = disl->length = lseek_ret;
	}
#ifdef	xDEV_TST
	TRdisplay("DI_SL_SENSE fd %d length %d \n", disl->unix_fd, 
			disl->length);
#endif
	break;


    case DI_SL_GUARANTEE :
#ifdef LARGEFILE64
        if ((lseek_ret =
                lseek64(disl->unix_fd, (OFFSET_TYPE) disl->pre_seek, L_SET))
                        < (OFFSET_TYPE) 0)
#else /* LARGEFILE64 */
	if ((lseek_ret = 
		lseek(disl->unix_fd, (OFFSET_TYPE) disl->pre_seek, L_SET)) 
			< (OFFSET_TYPE) 0)
#endif  /* LARGEFILE64 */
	{
	    SETCLERR(&disl->errcode, 0, ER_lseek);
	    local_status = DI_BADINFO;
	    break;
	}
	
	/*
	** Make sure we are positioned in the correct place 
	*/
	if ( lseek_ret != disl->pre_seek )
	{
	    SETCLERR(&disl->errcode, 0, ER_lseek);
	    local_status = DI_BADEXTEND;
	    break;
	}

#ifdef	xDEV_TST
	TRdisplay("DI_SL_GUARANTEE fd %d position at %d \n", disl->unix_fd, 
			disl->pre_seek);
#endif
	/*
	** Lets go and guarantee the space requested
	*/
	for ( ;disl->length > 0;)
	{
	    if ( disl->length > Di_zero_bufsize )
		buf_size = Di_zero_bufsize;
	    else
		buf_size = disl->length;

            if (( IIdio_write( disl->unix_fd,
			       Di_zero_buffer,
			       buf_size,
			       (OFFSET_TYPE)disl->pre_seek,
			       &fdseeks[disl->unix_fd],
			       disl->io_fprop,
			       &disl->errcode )) != buf_size )
	    {
		local_status = FAIL;
		break;
	    }

	    disl->length -= buf_size;
	    disl->pre_seek += buf_size;

#ifdef	xDEV_TST
	TRdisplay("DI_SL_GUARANTEE fd %d Guaranteed to %d \n", 
			disl->unix_fd, disl->pre_seek);
#endif
	}

	break;

    case DI_SL_ZALLOC:
	if (( lseek_ret = IIdio_get_file_eof(disl->unix_fd,
					disl->io_fprop)) < 0)
	{
	    SETCLERR(&disl->errcode, 0, ER_lseek);
	    local_status = DI_BADINFO;
	    break;
	}
	disl->pre_seek = lseek_ret;

#ifdef	xDEV_TST
	TRdisplay("DI_SL_ZALLOC fd %d eof %d \n", disl->unix_fd, 
			disl->pre_seek);
#endif

	for (;disl->length > 0;)
	{
	    if ( disl->length > Di_zero_bufsize )
		buf_size = Di_zero_bufsize;
	    else
		buf_size = disl->length;

	    if (( IIdio_write( disl->unix_fd, 
			       Di_zero_buffer, 
			       buf_size,
			      (OFFSET_TYPE)disl->pre_seek, 
			      &fdseeks[disl->unix_fd], 
			      disl->io_fprop,
		              &disl->errcode)) != buf_size )
	    {
	        local_status = FAIL;
		break;
	    }
	    disl->length -= buf_size;
	    disl->pre_seek += buf_size;

#ifdef	xDEV_TST
	TRdisplay("DI_SL_ZALLOC fd %d zalloc to %d \n", disl->unix_fd, 
			disl->pre_seek);
#endif
        }
	if (disl->length > 0)
	    break;
        disl->length = lseek_ret;
	break;


    case DI_SL_SYNC:

# ifdef	xCL_010_FSYNC_EXISTS
	if (fsync(disl->unix_fd) < 0)
	{
	    SETCLERR(&disl->errcode, 0, ER_fsync);
	    local_status = FAIL;
	}
# else
	local_status = FAIL;
# endif
	break;

    case DI_SL_DELETE:
	{
#ifdef LARGEFILE64
	    struct stat64	fd_stat;

	    if ((stat64(disl->buf, &fd_stat) == 0) && (fd_stat.st_nlink == 1))
#else /* LARGEFILE64 */
	    struct stat		fd_stat;

	    if ((stat(disl->buf, &fd_stat) == 0) && (fd_stat.st_nlink == 1))
#endif /* LARGEFILE64 */
	    {
		int	unix_fd;
		i4 dummy_fprop = FPROP_OPEN;	/* Don't care about fprops */
		CL_ERR_DESC	local_err;

		if ((unix_fd = IIdio_open(disl->buf, (O_RDWR | O_TRUNC), 
			DI_FILE_MODE, 0, 
			&dummy_fprop,
			&local_err)) != -1)
		{
		    (void)close(unix_fd);
		}
	    }
	    if (unlink(disl->buf) < 0)
	    {
	    	SETCLERR(&disl->errcode, 0, ER_unlink);
	    	local_status = FAIL;
	    }
	}
	break;

    case DI_SL_MKDIR:
	if (mkdir(disl->buf, disl->length) < 0)
	{
	    SETCLERR(&disl->errcode, 0, ER_mkdir);
	    local_status = FAIL;
	}
	break;

    case DI_SL_RMDIR:
	if (rmdir(disl->buf) < 0)
	{
	    SETCLERR(&disl->errcode, 0, ER_rmdir);
	    local_status = FAIL;
	}
	break;

    case DI_COLLECT_STATS:
	if (TMperfstat((TM_PERFSTAT *) disl->buf, &disl->errcode))
	    local_status = FAIL;
	break;

    case DI_SL_MAPMEM:
	local_status = MEget_pages(ME_MSHARED_MASK|ME_IO_MASK,
					(i4)0, disl->buf, &segaddr,
				&allocated_pages, &disl->errcode);
#ifdef xDEV_TST
	if (local_status)
	    TRdisplay("DI_SL_MAPMEM: Error %x on key=%s\n", local_status,
			disl->buf);
	else
	    TRdisplay("DI_SL_MAPMEM: map %s to addr %p\n", disl->buf, segaddr);
#endif
	break;

    case DI_SL_LSDIR:
    default:
	local_status = FAIL;
	break;
    }

    /* change the "shared-memory" status once the slave-op has completed, by
    ** delaying this callers can write some defensive code to catch cases where
    ** they are woken prematurely, while slave operation is still in progress.
    */
    disl->status = local_status;

    return(OK);
}
