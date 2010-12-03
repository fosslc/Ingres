/*
**Copyright (c) 2004 Ingres Corporation
*/

#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <systypes.h>
#include   <er.h>
#include   <cs.h>
#include   <fdset.h>
#include   <csev.h>
#include   <di.h>
#include   <me.h>
#include   <st.h>
#include   <dislave.h>
#include   "dilocal.h"
#include   "dilru.h"

/* unix include */
#include   <errno.h>

/**
**
**  Name: DIRENAME.C - This file contains the UNIX DIrename() routine.
**
**  Description:
**      The DIRENAME.C file contains the DIrename() routine.
**
**        DIrename - Changes name of a file.
**
** History:
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	17-oct-88 (jeff)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	05-jan-89 (jeff)
**	    if renameing an open file, be sure to change the file name in the 
**	    DI_IO.
**	24-jan-89 (roger)
**	    Make use of new CL error descriptor.
**	06-feb-89 (mikem)
**	    Clear the CL_ERR_DESC.
**	13-feb-89 (jeff)
**	    Add call to DIlru_flush to close off files and retry if a rename
**	    fails.  (also added RENAME_EXIST case)
**	24-Feb-1989 (fredv)
**	    Removed bogus reference to lseek.
**	    Used xCL_xxx from clconfig.h instead of RENAME_EXIST.
**	21-may-90 (blaise)
**	    Move <clconfig.h> include to pickup correct ifdefs in <csev.h>.
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	15-apr-1992 (bryanp)
**	    Document that DI_IO argument is no longer used. No longer support
**	    renaming of open files.
**	30-nov-1992 (rmuth)
**	    - Prototype and include "dilru.h"
**	    - DIlru error checking
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      26-jul-1993 (mikem)
**          Include systypes.h now that csev.h exports prototypes that include
**          the fd_set structure.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**      17-sep-1994 (nanpr01)
**          - Needs to check for interrupted system calls specially for
**            SIGUSR2. Curren implementation of 1 more retry is optimistic.
**            In lot of UNIX systems, link, unlink, rename cannot be 
**            interrupted(HP-UX).But Solaris returns EINTR. Bug # 57938. 
**      10-oct-1994 (nanpr01)
**          - Wrong number of parameter in DIlru_flush. Bug # 64169
**	20-Feb-1998 (jenjo02)
**	    DIlru_flush() prototype changed, it now computes the number of
**	    FDs to close instead of being passed an arbitrary number.
**	    Cleaned up handling of errno, which will be invalid after calling
**	    DIlru_flush().
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # defines */
/*
**  Local Definitions - copied from dilru.c
*/
#define		SEM_SHARE	0
#define		SEM_EXCL	1

/* typedefs */

/* forward references */

/* externs */
GLOBALREF DI_HASH_TABLE_CB    *htb; /* hash table control block */
GLOBALREF i4		    htb_initialized;
					   /* Is htb properly initialized? */

/* statics */


/*{
** Name: DIrename - Renames a file. 
**
** Description:
**      The DIrename will change the name of a file. 
**	The file MUST be closed.  The file can be renamed
**      but the path cannot be changed.  A fully qualified
**      filename must be provided for old and new names.
**      This includes the type qualifier extension.
**   
** Inputs:
**	di_io_unused	     UNUSED DI_IO pointer (always set to 0 by caller)
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      oldfilename          Pointer to old file name.
**      oldlength            Length of old file name.
**      newfilename          Pointer to new file name.
**      newlength            Length of new file name.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**        OK
**        DI_BADRNAME        Any i/o error during rename.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_DIRNOTFOUND     Path not found.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    26-mar-87 (mmm)    
**          Created new for 6.0.
**    06-feb-89 (mikem)
**	    Clear the CL_ERR_DESC.
**	15-apr-1992 (bryanp)
**	    Remove DI_IO argument and no longer support renaming open files.
**    30-nov-1992 (rmuth)
**	    - Prototype.
**	    - DIlru error checking
**    17-sep-1994 (nanpr01)
**          - Needs to check for interrupted system calls specially for
**            SIGUSR2. Curren implementation of 1 more retry is optimistic.
**            In lot of UNIX systems, link, unlink, rename cannot be 
**            interrupted(HP-UX).But Solaris returns EINTR. Bug # 57938. 
**    10-oct-1994 (nanpr01)
**          - Wrong number of parameter in DIlru_flush. Bug # 64169
**	20-Feb-1998 (jenjo02)
**	    DIlru_flush() prototype changed, it now computes the number of
**	    FDs to close instead of being passed an arbitrary number.
**	    Cleaned up handling of errno, which will be invalid after calling
**	    DIlru_flush().
**  15-Apr-2004 (fanch01)
**      Force closing of LRU file descriptors when a rename error is
**      is encountered.  Only occurs on a rename failure and the only
**      file that is closed is the file associated with the error.
**      Relieves problems on filesystems which don't accomodate renaming
**      open files.  "Interesting" semaphore usage is consistent with other
**      DI usage.
**	21-Apr-2004 (schka24)
**	    retry declaration got misplaced somehow, fix so it compiles.
**	26-Jul-2005 (schka24)
**	    Don't flush fd's on any random rename failure.  Do a better job
**	    of re-verifying the fd and di-io after locking the fd when we're
**	    searching for a file-open conflict.
**	30-Sep-2005 (jenjo02)
**	    htb_fd_list_mutex, fd_mutex are now CS_SYNCH objects.
*/
STATUS
DIrename(
    DI_IO	   *di_io_unused,
    char           *path,
    u_i4          pathlength,
    char           *oldfilename,
    u_i4          oldlength,
    char           *newfilename,
    u_i4          newlength,
    CL_ERR_DESC     *err_code)
{
    char    oldfile[DI_FULL_PATH_MAX];
    char    newfile[DI_FULL_PATH_MAX];
    STATUS  ret_val, intern_status;
    CL_ERR_DESC	    local_err;

    /* unix variables */
    int	    os_ret;

	/* retry variables */
	i4 retry = 0, failflag = 0;

    /* default returns */
    ret_val = OK;

    if ((pathlength > DI_PATH_MAX)	|| 
	(pathlength == 0)		||
	(oldlength > DI_FILENAME_MAX)	|| 
	(oldlength == 0)		|| 
	(newlength > DI_FILENAME_MAX)	||
	(newlength == 0))
	return (DI_BADPARAM);		

    /* get null terminated path and filename for old file */

    MEcopy((PTR) path, pathlength, (PTR) oldfile);
    oldfile[pathlength] = '/';
    MEcopy((PTR) oldfilename, oldlength, (PTR) &oldfile[pathlength + 1]);
    oldfile[pathlength + oldlength + 1] = '\0';

    /* get null terminated path and filename for new file */

    MEcopy((PTR) path, pathlength, (PTR) newfile);
    newfile[pathlength] = '/';
    MEcopy((PTR) newfilename, newlength, (PTR) &newfile[pathlength + 1]);
    newfile[pathlength + newlength + 1] = '\0';

	do
	{
		if (retry > 0 && failflag++ == 0)
			TRdisplay("%@ DIrename: retry on %t/%t\n",
					  pathlength, path, oldlength, oldfilename);
		retry = 0;
		CL_CLEAR_ERR( err_code );
#ifdef	xCL_035_RENAME_EXISTS
		/* Now rename the file. */    
		while  ((os_ret = rename(oldfile, newfile)) == -1) 
		{
			SETCLERR(err_code, 0, ER_rename);
			if (err_code->errnum != EINTR)
				break;
		}
#else /* xCL_035_RENAME_EXISTS */
		/* Now rename the file. */    
		while ((os_ret = link(oldfile, newfile)) == -1) 
		{
			SETCLERR(err_code, 0, ER_rename);
			if (err_code->errnum != EINTR)
				break;
		}
		if (os_ret != -1)
		{
			while ((os_ret = unlink(oldfile)) == -1) 
			{
				if (err_code->errnum != EINTR)
					break;
			}
		}
#endif /* xCL_035_RENAME_EXISTS */

		/* if the rename failed, see if we're holding the file open */
		if (os_ret == -1 && htb_initialized)
		{
			QUEUE *p, *q, *next;
			CS_synch_lock(&htb->htb_fd_list_mutex);
			q = &htb->htb_fd_list;
			for (p = q->q_prev; p != q; p = next)
			{
				DI_FILE_DESC *di_file = (DI_FILE_DESC *) p;
				DI_IO *di_io = (DI_IO *) di_file->fd_uniq.uniq_di_file;
				next = p->q_prev;
				if (di_io != NULL && di_file->fd_state == FD_IN_USE
				  && di_io->io_type == DI_IO_ASCII_ID
				  && pathlength == di_io->io_l_pathname
				  && oldlength == di_io->io_l_filename)
				{
					CS_synch_unlock(&htb->htb_fd_list_mutex);
					CS_synch_lock(&di_file->fd_mutex);
					/* Make sure it's still the right
					** DI_IO and compare the filename */
					if ((DI_IO *) di_file->fd_uniq.uniq_di_file == di_io &&
					      di_file->fd_state == FD_IN_USE &&
					      di_file->fd_unix_fd != -1 &&
					      !(di_io->io_open_flags & DI_O_NOT_LRU_MASK) &&
						di_io->io_type == DI_IO_ASCII_ID &&
						pathlength == di_io->io_l_pathname &&
						MEcmp((PTR) di_io->io_pathname, path, pathlength) == 0
						&& oldlength == di_io->io_l_filename &&
						MEcmp((PTR) di_io->io_filename, oldfilename,
							  oldlength) == 0)
					{
						/* have a match, print out stats */
						/* try to close it */
						CS_synch_unlock(&di_file->fd_mutex);
						DIlru_close(di_io, &local_err);
						retry++;
					}
					else
						CS_synch_unlock(&di_file->fd_mutex);
					CS_synch_lock(&htb->htb_fd_list_mutex);
				}
			}
			CS_synch_unlock(&htb->htb_fd_list_mutex);
		}
	} while (retry);

    if (os_ret == -1)
    {
	if ((err_code->errnum == ENOTDIR) || (err_code->errnum == EACCES))
	{
	    ret_val = DI_DIRNOTFOUND;
	}
	else
	{
	    ret_val = DI_BADRNAME;
	}
    }
    else
	CL_CLEAR_ERR( err_code );

    return(ret_val);
}
