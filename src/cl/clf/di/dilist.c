/*
**Copyright (c) 2004 Ingres Corporation
*/

#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <systypes.h> 
#include   <sys/stat.h> 
#include   <errno.h>
#include   <er.h>
#include   <cs.h>
#include   <fdset.h>
#include   <csev.h>
#include   <di.h>
#include   <me.h>
#include   <st.h>
#include   <diracc.h>
#include   <dislave.h>
#include   <handy.h>
#include   "dilocal.h"
#include   "dilru.h"

/**
**
**  Name: DILIST.C - This file contains the UNIX directory listing routines.
**
**  Description:
**      The DIALLOC.C file contains the routines necessary to implement the
**	directory and file listing routines.  This code was taken from LOlist()
**	and modified to meet the new interface.
**
**        DIlistdir - List all directories in a directory.
**        DIlistfiles - List all files in a directory.
**
** History:
**      
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	07-jan-88 (mikem)
**          bug fixes to make dilist return full pathnames.
**	19-may-88 (anton)
**          forgot to close the file reading the directory
**	17-oct-88 (jeff)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	24-jan-89 (roger)
**	    Make use of new CL error descriptor.
**	06-feb-89 (mikem)
**	    Initialize DI CL_ERR_DESC to zero.
**	    Also set io_type field of DI_IO control block to 
**	    DI_IO_DI_IO_LDIR_INVALID or DI_IO_LFILE_INVALID to trap DI usage
**	    bugs.
**	    Also added "#include <me.h>".
**	24-Feb-1989 (fredv)
**	    Use <systypes.h> instead of <sys/types.h>.
**	    Replaced xDI_xxx by corresponding xCL_xxx defs in clconfig.h.
**	    Split out directory access code into diracc.h for sharing with LO.
**	    Fix bugs involving terminating names, and bad assumptions
**	 	about . and ..
**	    Delete duplicate copy of the opendir emulation. It is now in
**		handy/diremul.c.
**	11-may-90 (blaise)
**	    Integrated code from 61 & ug:
**		Remove code to null-terminate d_name; readdir already does this.
**	5-aug-1991 (bryanp)
**	    Rename "FILE" to "LIST_FILES" to avoid conflict with <lo.h>.
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	30-November-1992 (rmuth)
**	    Prototype.
**	26-apr-1993 (bryanp)
**	    Finished prototyping.
**	    Made DI_list static.
**	    Please note that arg_list is a PTR, not a (PTR *).
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jul-1993 (mikem)
**	    Added include of st.h to pick up prototypes.
**	17-aug-1993 (tomm)
**	    Must include fdset.h since protypes being included reference
**	    fdset.
**	20-apr-1994 (mikem)
**	    bug #62347
**	    In order to provide better error logging diagnostics added new cl
**	    internal error DI_OPENDIR_FAILED, which will also log the name
**	    of the directory that the failure occurred on.  It is still up to
**	    the calling routine to actually call the correct logging routines
**	    to get the error into the errlog.log.
**	03-jun-1996 (canor01)
**	    Made readdir_buf into a structure aligned for struct dirent
**	    to avoid alignment problems when used with reentrant readdir.
**	24-Nov-1997 (allmi01)
**	    Bypassed the definition of lseek on Silicon Graphics (sgi_us5)
**	    as it already exists in <unistd.h>.
**      05-Nov-1997 (hanch04)
**          Remove declare of lseek, done in seek.h
**	01-Dec-1998 (hanch04)
**	    Add stat64 for LARGEFILE64
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Jul-2005 (hanje04)
**	    There appears to be a bug in readdir()/readdir_r() on Mac OSX
**	    such that iiCLreaddir() returns NULL before it gets to to the
**	    end of directory list, when the directory contains more than
**	    about 200 entires. This can result in a failure to remove all
**	    files in a directory without DI_list() reporting an error.
**	    This in turn will result in rmdir() failing when it is
**	    subsequently called to remove the directoy. 
**	    A "Google search" for "readdir mac os x" reveals that other
**	    have also hit this problem and that it can be worked around
**	    by reseting the directory entry pointer (dirp) to to the
**	    beginning of the list (using rewinddir()) and processing
**	    the list again. Hence, we need to go around the loop twice
**	    on Mac OSX.
**/

/* # defines */
#define			DIRECTORY	1	/* directory argument to local 
						** listing routine
						*/

#define			LIST_FILES	2	/* file argument to local 
						** listing routine
						*/

/* typedefs */

/* forward references */

/* externs */

/* statics */

static	STATUS	DI_list(
		    char	*path,
		    u_i4	pathlength,
		    i4		list_type,
		    STATUS	(*func)(),
		    PTR		arg_list,
		    CL_ERR_DESC	*err_code);


/*{
** Name: DIlistdir - List all directories in a directory.
**
** Description:
**      The DIlistdir routine will list all directories 
**      that exist in the directory(path) specified.  
**      The directories are not listed in any specific order.
**      This routine expects
**      as input a function to call for each directory found.  This
**      insures that all resources tied up by this routine will 
**      eventually be freed.  The function passed to this routine
**      must return OK if it wants to continue with more directories,
**      or a value not equal to OK if it wants to stop listing.
**	If the function returns anything by OK, then DIlistdir
**	will return DI_ENDFILE.
**      The function must have the following call format:
**
**          STATUS
**          funct_name(arg_list, dirname, dirlength, err_code)
**          PTR        arg_list;       Pointer to argument list. 
**          char       *dirname;        Pointer to directory name.
**          i4         dirlength;       Length of directory name.
**          CL_ERR_DESC *err_code;       Pointer to operating system
**                                      error codes (if applicable).
**   
** Inputs:
**      f                    Pointer to file context area.
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      fnct                 Function to pass the 
**                           directory entries to.
**      arg_list             Pointer to function argument
**                           list.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**        DI_BADDIR          Path specification error.
**        DI_ENDFILE         Error returned from client handler
**                           or all files listed.
**        DI_BADPARAM        Parameters in error.
**        DI_DIRNOTFOUND     Path not found.
**        DI_BADLIST         Error trying to list objects.
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
**	    Also set io_type field of DI_IO control block to 
**	    DI_IO_DI_IO_LDIR_INVALID to trap DI usage bugs.
**	    Also added "#include <me.h>".
**	26-apr-1993 (bryanp)
**	    arg_list is a PTR, not a (PTR *).
*/
STATUS
DIlistdir(
DI_IO          *f,
char           *path,
u_i4          pathlength,
STATUS         (*func)(),
PTR            arg_list,
CL_ERR_DESC	*err_code)
{
    /* for sanity mark DI_IO as not a valid open file */

    f->io_type = DI_IO_LDIR_INVALID;

    return(DI_list(path, pathlength, DIRECTORY, func, arg_list, err_code));
}

/*{
** Name: DIlistfile - List all files in a directory.
**
** Description:
**      The DIlistfile routine will list all files that exist
**      in the directory(path) specified.  This routine expects
**      as input a function to call for each file found.  This
**      insures that all resources tied up by this routine will 
**      eventually be freed.  The files are not listed in any
**      specific order. The function passed to this routine
**      must return OK if it wants to continue with more files,
**      or a value not equal to OK if it wants to stop listing.
**	If the function returns anything by OK, then DIlistfile
**	will return DI_ENDFILE.
**      The function must have the following call format:
**
**          STATUS
**          funct_name(arg_list, filename, filelength, err_code)
**          PTR        arg_list;       Pointer to argument list 
**          char       *filename;       Pointer to directory name.
**          i4         filelength;      Length of directory name. 
**          CL_ERR_DESC *err_code;       Pointer to operating system
**                                      error codes (if applicable). 
**   
** Inputs:
**      f                    Pointer to file context area.
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      fnct                 Function to pass the 
**                           directory entries to.
**      arg_list             Pointer to function argument
**                           list.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**        DI_BADDIR          Path specification error.
**        DI_ENDFILE         Error returned from client handler or
**                           all files listed.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_DIRNOTFOUND     Path not found.
**        DI_BADLIST         Error trying to list objects.
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
**	    Set io_type field of DI_IO control block to 
**	    DI_IO_LFILE_INVALID to trap DI usage bugs.
**	26-apr-1993 (bryanp)
**	    arg_list is a PTR, not a (PTR *).
*/
STATUS
DIlistfile(
DI_IO          *f,
char           *path,
u_i4          pathlength,
STATUS         (*func)(),
PTR            arg_list,
CL_ERR_DESC     *err_code)
{
    /* for sanity mark DI_IO as not a valid open file */

    f->io_type = DI_IO_LFILE_INVALID;

    return(DI_list(path, pathlength, LIST_FILES, func, arg_list, err_code));
}
/*{
** Name: DI_list - Use BSD directory listing interface to list files
**
** Description:
**	List all files/directories in a given directory depending on 
**	list_type argument.  BSD directory listing interface is used.
**	Those machines which don't support this interface (ie. SV) should
**	define xDI_007_NO_DIRFUNC which will cause Doug Gwen's directory
**	listing emulation code to be compiled into the system and used
**	instead.
**
** Inputs:
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**	list_type	     type of object to list (DIRECTORY or LIST_FILES)
**	func		     function to call for each object found.
**	arg_list	     arguments to call the function with.
**
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**
**    Returns:
**        DI_BADDIR          Path specification error.
**        DI_ENDFILE         Error returned from client handler or
**                           all files listed.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_DIRNOTFOUND     Path not found.
**        DI_BADLIST         Error trying to list objects.
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    26-mar-87 (mmm)    
**          Created new for 6.0.
**	26-apr-1993 (bryanp)
**	    Arg_list is a PTR, not a (PTR *).
**	    Made this function static, since it's only used in this file.
**	20-apr-1994 (mikem)
**	    bug #62347
**	    In order to provide better error logging diagnostics added new cl
**	    internal error DI_OPENDIR_FAILED, which will also log the name
**	    of the directory that the failure occurred on.  It is still up to
**	    the calling routine to actually call the correct logging routines
**	    to get the error into the errlog.log.
**      23-may-95 (tutto01)
**          Changed readdir call to the reentrant iiCLreaddir.
**	03-jun-1996 (canor01)
**	    Made readdir_buf into a structure aligned for struct dirent
**	    to avoid alignment problems when used with reentrant readdir.
**	07-Jul-2005 (hanje04)
**	    There appears to be a bug in readdir()/readdir_r() on Mac OSX
**	    such that iiCLreaddir() returns NULL before it gets to to the
**	    end of directory list, when the directory contains more than
**	    about 200 entires. This can result in a failure to remove all
**	    files in a directory without reporting an error. This in turn
**	    will result in rmdir() failing when it is subsequently called
**	    to remove the directoy. 
**	    A "Google search" for "readdir mac os x" reveals that other
**	    have also hit this problem and that it can be worked around
**	    by reseting the directory entry pointer (dirp) to to the
**	    beginning of the list (using rewinddir()) and processing
**	    the list again. Hence, we need to go around the loop twice
**	    on Mac OSX.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
*/

static STATUS
DI_list(
char		*path,
u_i4		pathlength,
i4		list_type,
STATUS		(*func)(),
PTR		arg_list,
CL_ERR_DESC	*err_code)
{
    STATUS	ret_val;
    u_i4	entry_length;

    /* unix variables */
    char		input_path[DI_FULL_PATH_MAX];
    char		full_name[DI_FULL_PATH_MAX];
    DIR			*dirp;
#ifdef xCL_025_DIRENT_EXISTS
    struct dirent	*dir_entry;
#else  /* xCL_025_DIRENT_EXISTS */
    struct direct	*dir_entry;
#endif /* xCL_025_DIRENT_EXISTS */
#ifdef LARGEFILE64
    struct stat64	stat_info;
#else /* LARGEFILE64 */
    struct stat		stat_info;
#endif /* LARGEFILE64 */
    struct
    {
	struct dirent   dt;
        char		buf[DI_PATH_MAX+1];
    } readdir_buf;

    /* default returns */
    ret_val = OK;
    CL_CLEAR_ERR( err_code );

    /* Check input parameters */
    if ((pathlength > DI_PATH_MAX) || (pathlength == 0))
	return(DI_BADPARAM);

    /* get null terminated path to the directory to open 'path/.' */

    MEcopy((PTR) path, pathlength, (PTR) input_path);
    input_path[pathlength] = '/';
    input_path[pathlength + 1] = '.';
    input_path[pathlength + 2] = '\0';

    /* break out on errors */
    while ((dirp = opendir(input_path)) == (DIR *)NULL)
    {
	SETCLERR(err_code, 0, ER_opendir);

	if (errno == EMFILE || errno == ENFILE)
	{
	    if (DIlru_free(err_code) != OK)
	    {
		ret_val = DI_BADLIST;
		break;
	    }
	}
	else
	{
	    ret_val = DI_DIRNOTFOUND;
	    break;
	}
    }

    if (ret_val)
    {
	i4	length = min(pathlength + 2, CLE_INFO_MAX);

	err_code->intern = DI_OPENDIR_FAILED;

	err_code->moreinfo[0].size = length;
	STncpy(err_code->moreinfo[0].data.string, input_path, length);
	err_code->moreinfo[0].data.string[ length ] = EOS;
    }

    /* now input_path string will point to directory (not directory/.) */

    input_path[pathlength] = '\0';

    if (!ret_val)
    {

	/* now loop through all entries in the directory */

	ret_val = DI_ENDFILE;

# ifdef OSX
	/*
	**  FIX ME!!
	**  There appears to be a bug in readdir()/readdir_r() on Mac OSX
	**  such that iiCLreaddir() returns NULL before it gets to to the
	**  end of directory list, when the directory contains more than
	**  about 200 entires.
	** 
	**  This can be worked around by reseting the entry pointer (dirp) to
	**  to the beginning of the list (using rewinddir()) and processing
	**  the list again. Hence the extra while loop.
	*/
	i4 read_cnt=0;
	while( read_cnt < 2 )
	{
# endif
	while ((dir_entry = iiCLreaddir(dirp, (struct dirent *) &readdir_buf)) != NULL)
	{
	    if (dir_entry->d_ino == 0)
	    {
		/* this file was deleted */
		continue;
	    }

	    /* 
	    ** Ignore . and .. which might not exist or be first.
	    ** (From 5.0 lolist).
	    */
	    if( STcompare( ".", dir_entry->d_name ) == 0
	    	|| STcompare( "..", dir_entry->d_name ) == 0 )
	    	continue;

	    STpolycat(3, input_path, "/", dir_entry->d_name, full_name);

#ifdef LARGEFILE64
	    if (stat64(full_name, &stat_info))
#else /* LARGEFILE64 */
	    if (stat(full_name, &stat_info))
#endif /* LARGEFILE64 */
	    {
		SETCLERR(err_code, 0, ER_stat);

		if (errno == ENOENT)
		    continue;
		ret_val = DI_BADLIST;
		break;
	    }

	    if (((list_type == DIRECTORY) && (stat_info.st_mode & S_IFDIR)) ||
		((list_type == LIST_FILES) && (!(stat_info.st_mode & S_IFDIR))))
	    {
		entry_length = STlength(dir_entry->d_name);
		
		if ((*func)(arg_list, dir_entry->d_name, entry_length, err_code))
		{
		    ret_val = DI_ENDFILE;
		    break;
		}
	    }
	}
# ifdef OSX
	/* reset the list and back 'round the readdir() loop */
	rewinddir(dirp);
	read_cnt++;
	}
# endif
	closedir(dirp);
    }

    return(ret_val);
}
