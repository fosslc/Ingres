/*
**Copyright (c) 2004 Ingres Corporation
*/

/* stdio must be first so that NULL redefined message isn't printed */


#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <si.h>		/* for FILE definition */
#include   <er.h>
#include   <cs.h>
#include   <fdset.h>
#include   <csev.h>
#include   <di.h>
#include   <me.h>
#include   <dislave.h>
#include   "dilocal.h"

/* unix includes */

#include   <errno.h>		/* for E* errors */
#include   <systypes.h>		/* for struct stat */
#include   <sys/stat.h>		/* for struct stat */

/**
**
**  Name: DIDDELETE.C - This file contains the UNIX DIdirdelete() routine.
**
**  Description:
**      The DIDDELETE.C file contains the DIdirdelete() routine.
**
**        DIdirdelete - Deletes a directory.
**
** History:
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	17-oct-88 (jeff)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**	24-jan-88 (roger)
**	    Make use of new CL error descriptor.
**	06-feb-89 (mikem)
**	    better support for DI CL_ERR_DESC, including initializing to zero.
**	    Also set io_type field of DI_IO control block to 
**	    DI_IO_DDELETE_INVALID to catch possible DI usage bugs.
**	    Also added "#include <me.h>".
**	24-Feb-1989 (fredv)
**	    Replaced all xDI_...'s with appropriate xCL_...'s
**	    Changed DI_42_deletedir to DI_sys_deletedir.
**	    Changed DI_sv_deletedir to DI_prog_deletedir.
**	    Removed HP9000/500 ancient rmdir stuff, it is obsolete.
**	21-may-90 (blaise)
**	    Move <clconfig.h> include to pickup correct ifdefs in <csev.h>.
**	5-aug-1991 (bryanp)
**	    Added support for ommitting the dirname and dirlength args.
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	30-nov-1992 (rmuth)
**	    Prototype.
**	8-jun-1993 (ed)
**	    move si.h behind compat.h
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**      05-Nov-1997 (hanch04)
**          Remove declare of lseek, done in seek.h
**	24-Nov-1997 (allmi01)
**		Bypassed defintion of lseek() on Silicon Graphics (sgi_us5)
**		as it already exists in <unistd.h>.
**	03-Dec-1998 (muhpa01)
**	    Removed declare of lseek for hp8_us5 as well.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	08-aug-2000 (hayke02)
**		Bypass defintion of lseek() on hp8_us5.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-Jan-2001 (wansh01)
**	    Removed declare of lseek for dgi_us5 as well.
**	23-Sep-2003 (hanje04)
**	    Don't declare lseek for Linux
**	10-Dec-2004 (bonro01)
**	    lseek() is already defined in a64_sol
**      22-Apr-2005 (hanje04)
**	    Remove define of lseek() and unlink(), system functions shouldn't 
**	    be protyped in Ingres code.
**/

/* # defines */

/* typedefs */

/* forward references */

#ifdef	xCL_009_RMDIR_EXISTS
static STATUS 	DI_sys_deletedir(
				char           *path,
				u_i4          pathlength,
				char           *dirname,
				u_i4          dirlength,
				CL_ERR_DESC     *err_code );
#else
static STATUS DI_prog_deletedir(
				char           *path,
				u_i4          pathlength,
				char           *dirname,
				u_i4          dirlength,
				CL_ERR_DESC     *err_code);
#endif	/* xCL_009_RMDIR_EXISTS */

/* externs */

/* statics */


/*{
** Name: DIdirdelete  - Deletes a directory.
**
** Description:
**      The DIdirdelete routine is used to delete a direct access 
**      directory.  The name of the directory to delete should
**      be specified as path and dirname, where dirname does not
**      include a type qualifier or version and the path does
**	not include the directory name (which it normally does for
**	other DI calls).  The directory name may be  converted to a filename
**	of the directory and use DIdelete to delete it.
**
**	If the dirname and dirlength are ommitted, then we presume
**	that the pathname is the fully-specified name of the directory
**	to delete. That is, we presume that we are to delete the
**	directory whose name is the last component of 'path'. This is
**	not elegant, but it is "grandfathered" in because that's the
**	behaviour of the VMS CL and the mainline code unfortunately
**	depends on it.
**
** Inputs:
**      f                    Pointer to file context.
**      path                 Pointer to the area containing 
**                           the path name for this directory.
**      pathlength           Length of path name.
**      dirname              Pointer to the area containing
**                           the directory name.
**      dirlength            Length of directory.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**        OK
**	  DI_DIRNOTFOUND     Path not found.
**	  DI_FILENOTFOUND    File not found.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_BADDELETE       Error trying to delete directory.
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    26-mar-87 (mmm)    
**          Created new for 6.0.
**	06-feb-89 (mikem)
**	    better support for DI CL_ERR_DESC, including initializing to zero.
**	    Also set io_type field of DI_IO control block to 
**	    DI_IO_DDELETE_INVALID to catch possible DI usage bugs.
**	5-aug-1991 (bryanp)
**	    Added support for ommitting the dirname and dirlength args.
**	    6.5 destroydb (dmm_destroy()) depends on this behavior.
**	30-nov-1992 (rmuth)
**	    Prototype.
*/
STATUS
DIdirdelete(
    DI_IO          *f,
    char           *path,
    u_i4          pathlength,
    char           *dirname,
    u_i4          dirlength,
    CL_ERR_DESC     *err_code)
{
    STATUS	ret_val;

    /* unix variables */

    /* default return values */

    ret_val = OK;
    CL_CLEAR_ERR( err_code );
    
    /* Check input parameters. */

    if (pathlength > DI_PATH_MAX	|| 
	pathlength == 0			|| 
	dirlength > DI_FILENAME_MAX)
    {
	return(DI_BADPARAM);
    }

#ifdef	xCL_009_RMDIR_EXISTS

    ret_val = DI_sys_deletedir(path, pathlength, dirname, dirlength, err_code);

#else /* xCL_009_RMDIR_EXISTS */

    ret_val = DI_prog_deletedir(path, pathlength, dirname, dirlength, err_code);

#endif	/* xCL_009_RMDIR_EXISTS */


    /* for sanity mark DI_IO as not a valid open file */

    f->io_type = DI_IO_DDELETE_INVALID;

    return(ret_val);
}


#ifdef	xCL_009_RMDIR_EXISTS
/*{
** Name: DI_sys_deletedir - Delete a directory on a 4.2 filesystem.
**
** Description:
**	Use the rmdir() system call which has exactly the correct semantics
**	which DIdircreate needs.
**
** Inputs:
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      dirname              Pointer to the directory name.
**      dirlength            Length of directory name.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**
**    Returns:
**        OK
**        DI_EXISTS          Already exists.
**        DI_DIRNOTFOUND     Path not found.
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
**    24-Feb-1989 (fredv)
**	    Changed DI_42_deletedir to DI_sys_deletedir because the mkdir
**		is no longer 4.2 sys call.
**	5-aug-1991 (bryanp)
**	    Added support for ommitting the dirname and dirlength args.
**    30-nov-1992 (rmuth)
**	    Prototype.
*/
static STATUS
DI_sys_deletedir(
    char           *path,
    u_i4          pathlength,
    char           *dirname,
    u_i4          dirlength,
    CL_ERR_DESC     *err_code)
{
    STATUS	ret_val;

    /* unix variables */
    char	full_path[DI_FULL_PATH_MAX];

    /* default return values */

    ret_val = OK;
    
    /* get null terminated path and filename */

    MEcopy((PTR) path, pathlength, (PTR) full_path);
    if (dirlength)
    {
	full_path[pathlength] = '/';
	MEcopy((PTR) dirname, dirlength, (PTR) &full_path[pathlength + 1]);
	full_path[pathlength + dirlength + 1] = '\0';
    }
    else
    {
	full_path[pathlength] = '\0';
    }

    /* Use the system call to do the delete */

    if (rmdir(full_path) == -1)
    {
	SETCLERR(err_code, 0, ER_rmdir);

	if ((errno == ENOTDIR) || (errno == EACCES))
	{
	    ret_val = DI_DIRNOTFOUND;
	}
	else if (errno == ENOENT)
	{
	    ret_val = DI_FILENOTFOUND;
	}
	else
	{
	    ret_val = DI_BADDELETE;
	}
    }

    return(ret_val);
}

#else

/*{
** Name: DI_prog_deletedir - Delete a directory on a system V filesystem.
**
** Description:
**	No rmdir() system call exists so we have to fork the program
**	/bin/rmdir.  Use full path to avoid security problems (ie. if user
**	makes a program called rmdir in their path before the real rmdir, then
**	the backend which is running as ingres might execute it causing havoc.)
**
** Inputs:
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      dirname              Pointer to the directory name.
**      dirlength            Length of directory name.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**
**    Returns:
**        OK
**        DI_EXISTS          Already exists.
**        DI_DIRNOTFOUND     Path not found.
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
**    24-Feb-1989 (fredv)
**	    Renamed DI_sv_deletedir to DI_prog_deletedir.
**	5-aug-1991 (bryanp)
**	    Added support for ommitting the dirname and dirlength args.
**    30-nov-1992 (rmuth)
**    	  Prototype.
*/

static STATUS
DI_prog_deletedir(
    char           *path,
    u_i4          pathlength,
    char           *dirname,
    u_i4          dirlength,
    CL_ERR_DESC     *err_code)
{
    STATUS	ret_val;

    /* unix variables */
    int		os_ret;
    char	cmd_buf[sizeof("rmdir > /dev/null 2>&") + DI_FULL_PATH_MAX];
    char	full_path[DI_FULL_PATH_MAX];
    struct stat	stat_buf;

    /* default return values */

    ret_val = OK;

    /* get null terminated path and filename */

    MEcopy((PTR) path, pathlength, (PTR) full_path);
    if (dirlength)
    {
	full_path[pathlength] = '/';
	MEcopy((PTR) dirname, dirlength, (PTR) &full_path[pathlength + 1]);
	full_path[pathlength + dirlength + 1] = '\0';
    }
    else
    {
	full_path[pathlength] = '\0';
    }

    /* check for existence of directory */

    if (stat(full_path, &stat_buf) != 0)
    {
	SETCLERR(err_code, 0, ER_stat);

	if (errno == ENOTDIR)
	{
	    ret_val = DI_DIRNOTFOUND;
	}
	else
	{
	    ret_val = DI_FILENOTFOUND;
	}
	return(ret_val);
    }

    /* TODO: see if directory is empty */
    
    /* Fake it using the program; no detailed status possible */

    STprintf(cmd_buf, "/bin/rmdir %s > /dev/null 2>&1", full_path);

    if (os_ret = system(cmd_buf) >>8)
    {
	/*
        ** FIX ME: add internal CL error message for /bin/rmdir failure.
        **
        ** something like: E_CLnnnn rmdir program exited with %0d

        SETCLERR(err_code, E_CLnnnn, 0);
        errdesc->moreinfo[0].size = sizeof(os_ret);
        errdesc->moreinfo[0].data._nat = os_ret;

        **
        */
	ret_val = DI_BADDELETE;
    }

    return(ret_val);
}
#endif	/* xCL_009_RMDIR_EXISTS */
