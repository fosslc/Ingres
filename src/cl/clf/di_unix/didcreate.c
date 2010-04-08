/*
**Copyright (c) 2004 Ingres Corporation
*/
#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <si.h>		/* for FILE definition - must be first */
#include   <er.h>
#include   <cs.h>
#include    <fdset.h>
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
**  Name: DIDCREATE.C - This file contains the UNIX DIdircreate() routine.
**
**  Description:
**      The DIDCREATE.C file contains the DIdircreate() routine.
**
**        DIdircreate - Creates a directory.
**
** History:
 * 
**      26-mar-87 (mmm)    
**          Created new for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      08-aug-88 (mmm)
**	    Made all directories built by DIdircreate() be permission 777.
**      31-aug-88 (mikem)
**	    made directories created be mod 777.
**      17-oct-88 (jeff)
**	    new DI lru calls - pinning of open files in a multi-thread 
**	    environment.
**      08-nov-88 (roger)
**	    Fix typo- commented-out #ifdef key.
**      24-jan-89 (roger)
**	    Make use of new CL error descriptor.
**	06-feb-89 (mikem)
**	    better support for DI CL_ERR_DESC, including initializing to zero,
**	    and new chdir() error.  All set io_type field of DI_IO control
**	    block to DI_IO_DCREATE_INVALID to catch possible DI usage bugs.
**	    Also added "#include <me.h>".
**	2-Feb-1989 (fredv)
**	   Included cl headers si.h, clconfig.h, systypes.h instead of system
**	  	headers stdio.h	,sys/types.h.
**	   Renamed DI_42_dircreate and DI_sv_dircreate to DI_sys_dircreate
**		and DI_prog_dircreate respectively. Also used ifdef
**		xCL_008_MKDIR_EXISTS instead of xDI_001_MKDIR_SYS_CALL/
**		xDI_002_MKDIR_ROUTINE.
**	11-may-90 (blaise)
**	   Integrated ug changes into 63p library:
**		Added missing declaration for oumask in DI_prog_dircreate()
**		Call getcwd instead of getwd if xCL_067_GETCWD_EXISTS is
**		defined
**	21-may-90 (blaise)
**	    Move <clconfig.h> include to pickup correct ifdefs in <csev.h>.
**	15-nov-1991 (bryanp)
**	    Moved "dislave.h" to <dislave.h> to share it with LG.
**	30-nov-1992 (rmuth)
**	    Prototype.
**	8-jun-93 (ed)
**	    move si.h after compat.h
**	    
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Jun-2008 (bonro01)
**	    Remove deprecated function getwd() and xCL_067_GETCWD_EXISTS
**	    since all current platforms have getcwd()
**/

/* # defines */

/* typedefs */

/* forward references */

#ifdef	xCL_008_MKDIR_EXISTS
static STATUS DI_sys_dircreate(
			    	char           *path,
			    	u_i4          pathlength,
			    	char           *dirname,
			    	u_i4          dirlength,
			    	CL_ERR_DESC     *err_code);
#else
static STATUS DI_prog_dircreate(
				char           *path,
				u_i4          pathlength,
				char           *dirname,
				u_i4          dirlength,
				CL_ERR_DESC     *err_code);
#endif

FILE			*popen();
int			pclose();
int			stat();

/* externs */

/* statics */



/*{
** Name: DIdircreate - Creates a directory.
**
** Description:
**      The DIdircreate will take a path name and directory name 
**      and create a directory in this location.  
**      
** UNIX design:
**	The creation of directories in the dbms presents a special problem
**	which is further complicated by the ingres protection system and the
**	differences in system V and BSD implementation of routines to create
**	directories.  
**
**	The directories that are created by this function are database, journal
**	and ckp directories.  The directory in which they are created is owned
**	by INGRES and has permission 700 (to stop all other users from getting
**	to the directories below).  The ingres process which is running 
**	executing this command will be installed as a "setuid" program, so that
**	users running the program will have effective uid INGRES and real uid
**	random_user.
**
**	The current BSD (4.2 and later) mkdir command creates a directory 
**	given a full pathname (or relative) correctly in these circumstances.
**
**	However system V does not provide an equivalent functionality.  The
**	mknod() command must be run with effective uid root.  The only other
**	routine to make a directory available is the system level command
**	"mkdir".  The system V mkdir uses access() which runs with the real 
**	users permission.  Therefore mkdir fails if given a full pathname
**	with the above stated permission characteristics.   To get around
**	this, in the SV environment, we change our cwd to be the "base" 
**	directory in which we want to create the new directory.  We then use 
**	the user level "mkdir" to create the new directory.  This requires the 
**	real user to have write permission in the "base" directory; therefore
** 	the INGRES protection scheme is changed so that the "base" directories 
**	under data, ckp, jnl, etc. are PEworld("rwxrwx").  data, ckp jnl, etc. 
**	remain PEworld("rwx--") so the data is still protected (modulo UNIX).  
**	When DIcreate() is used to create databases (ckps, jnls) it's now
**	necessary to use PEumask("rwxrxw"), otherwise the REAL user's 
**	permissions may preclude ingres from accessing the database (ckp, jnl).
**
**	It should also be noted that the database directories will be owned 
**	by the REAL user on SV and "ingres" (the effective user) on BSD42.  
**	This is not a change but has been a point of confusion.
**
**   
** Inputs:
**      f                    Pointer to file context area.
**      path                 Pointer to the path name.
**      pathlength           Length of path name.
**      dirname              Pointer to the directory name.
**      dirlength            Length of directory name.
** Outputs:
**      err_code             Pointer to a variable used
**                           to return operating system 
**                           errors.
**    Returns:
**        OK
**        DI_BADDIR          Path specification error.
**        DI_BADPARAM        Parameter(s) in error.
**        DI_EXCEED_LIMIT    Open file limit exceeded.
**        DI_EXISTS          Already exists.
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
**	    better support for DI CL_ERR_DESC, including initializing to zero,
**	    and new chdir() error.  All set io_type field of DI_IO control
**	    block to DI_IO_DCREATE_INVALID to catch possible DI usage bugs.
**    30-nov-1992 (rmuth)
**	    Prototype.
*/
STATUS
DIdircreate(
    DI_IO          *f,
    char           *path,
    u_i4          pathlength,
    char           *dirname,
    u_i4          dirlength,
    CL_ERR_DESC     *err_code)
{
    STATUS	ret_val;

    /* default return values */

    ret_val = OK;
    CL_CLEAR_ERR( err_code );
    
    /* Check input parameters. */

    if (pathlength > DI_PATH_MAX	|| 
	pathlength == 0			|| 
	dirlength > DI_FILENAME_MAX	|| 
	dirlength == 0)
    {
	return(DI_BADPARAM);
    }


#ifdef	xCL_008_MKDIR_EXISTS

    ret_val = DI_sys_dircreate(path, pathlength, dirname, dirlength, err_code);


#else /* xCL_008_MKDIR_EXISTS */

    ret_val = DI_prog_dircreate(path, pathlength, dirname, dirlength, err_code);

#endif	/* xCL_008_MKDIR_EXISTS */

    /* for sanity mark DI_IO as invalid (ie. not an open file) */

    f->io_type = DI_IO_DCREATE_INVALID;

    return(ret_val);
}

#ifdef	xCL_008_MKDIR_EXISTS

/*{
** Name: DI_sys_dircreate - Create a directory on a 4.2 filesystem.
**
** Description:
**	Use the mkdir() system call which has exactly the correct semantics
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
**        DI_BADDIR          Otherwise.
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
**    08-aug-88 (mmm)
**	    Made all directories built by this routine be permission 777.
**    30-nov-1992 (rmuth)
**	    Prototype.
**    15-sep-1994 (nanpr01)
**	    Donot return from DI_sys_dircreate call with a good return 
**          value when mkdir failed.
**    24-nov-2004 (hayke02)
**	    All directories built by this routine will now have permission 700.
*/
static STATUS
DI_sys_dircreate(
    char           *path,
    u_i4          pathlength,
    char           *dirname,
    u_i4          dirlength,
    CL_ERR_DESC     *err_code)
{
    STATUS	ret_val;

    /* unix variables */
    int		oumask;
    char	full_path[DI_FULL_PATH_MAX];

    /* default return values */

    ret_val = OK;
    
    /* get null terminated path and filename DI_IO struct */

    MEcopy((PTR) path, pathlength, (PTR) full_path);
    full_path[pathlength] = '/';
    MEcopy((PTR) dirname, dirlength, (PTR) &full_path[pathlength + 1]);
    full_path[pathlength + dirlength + 1] = '\0';

    /* Use the mkdir system call; much easier than on SV */

    /* DI directories will always be created with 777 permissions.  Security
    ** will remain since the "data, jnl, ckp directories will be 700
    */
    /* hayke02 (24-nov-2004)
    ** create dirs with 700 permissions
    */
    oumask = umask(077);
    if (mkdir(full_path, 0700) != 0)
    {
	SETCLERR(err_code, 0, ER_mkdir);

	if ((errno == ENOTDIR) || (errno == ENOENT))
	{
	    ret_val = DI_DIRNOTFOUND;
	}
	else if (errno == EEXIST)
	{
	    ret_val = DI_EXISTS;
	}
	else ret_val = DI_BADDIR;
    }
    umask(oumask);

    return(ret_val);
}
#else	/* xCL_008_MKDIR_EXISTS */


/*{
** Name: DI_prog_dircreate - Create a directory on a system V file system.
**
** Description:
**
**	Since the mkdir() system call doesn't exist use the /bin/mkdir
**	system program.  Use the full path to the executable to avoid
**	trojan horse problem (ie. someone copying their own program called
**	mkdir to early in their path and having the backend which is running
**	as ingres execute it).
**
**	Because of /bin/mkdir permission anomalies described above, we must
**	change our current working directory and create the directory relative
**	to cwd.
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
**	  DI_EXCEED_LIMIT    Ran out of open file descriptors.
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
**    08-aug-88 (mmm)
**	    Made all directories built by this routine be permission 777.
**    30-nov-1992 (rmuth)
**	    Prototype.
**    24-nov-2004 (hayke02)
**	    All directories built by this routine will now have permission 700.
**	23-Aug-2005 (schka24)
**	    Studio-10 compiler caught missing arg to lru-free (I had the
**	    secrets definitions messed up, which is the only reason it was
**	    compiling this version).
*/
static STATUS
DI_prog_dircreate(
    char           *path,
    u_i4          pathlength,
    char           *dirname,
    u_i4          dirlength,
    CL_ERR_DESC     *err_code)
{
    STATUS	ret_val;

    /* unix variables */
    int		oumask;
    char	current_wd[256];
    char	path_str[DI_PATH_MAX + 1]; 
    char	dirname_str[DI_FILENAME_MAX + 1];
    char	cmd_str[sizeof("/bin/mkdir ") + DI_FILENAME_MAX];
    FILE	*streamptr;
    struct stat	buf;

    /* default return values */

    ret_val = OK;
    
    for (;;)
    {
	/* break on errors */

	/* save current working directory */
        if (getcwd(current_wd, sizeof(current_wd)) == NULL)
	{
	    /* if this fails we will just not try to get back to this 
	    ** directory.  I don't think the code counts on getting back
	    ** anyway.  As documented getcwd only fails if size is 0
	    ** (error already caught above) or if size is not big enough
	    ** to hold string (by picking 256 I think this has been
	    ** eliminated)
	    */
	    current_wd[0] = '.';
	    current_wd[1] = '\0';
	}

	/* create null terminated directory string */

	MEcopy((PTR) path, pathlength, (PTR) path_str);
	path_str[pathlength] = '\0';

	/* create null terminated directory name strint */

	MEcopy((PTR) dirname, dirlength, (PTR) dirname_str);
	dirname_str[dirlength] = '\0';

	/* change current working directory to there */

	if (chdir(path_str) == -1)
	{
	    SETCLERR(err_code, 0, ER_chdir);
	    ret_val = DI_DIRNOTFOUND;
	    break;
	}

	/* see if directory exists */

	if (stat(dirname_str, &buf) == 0)
	{
	    /* if stat returns successfully then file must exist */

	    ret_val = DI_EXISTS;
	    break;
	}
	else if (errno != ENOENT)
	{
	    /* if stat fails it had better be because the file doesn't
	    ** exist.  Other errors shouldn't be possible.
	    */

	    SETCLERR(err_code, 0, ER_stat);

	    ret_val = DI_BADDIR;
	    break;
	}
	    
	/* DI directories will always be created with 777 permissions.  Security
	** will remain since the "data, jnl, ckp directories will be 700
	*/
	/* hayke02 (24-nov-2004)
	** create dirs with 700 permissions
	*/
	oumask = umask(077);

	/* now use the system mkdir command which will work on SV */
	STprintf(cmd_str, "/bin/mkdir %s", dirname_str);

	while ((streamptr = popen(cmd_str, "r")) == (FILE *)NULL)
	{
	    if ((errno == EMFILE) || (errno == ENFILE))
	    {
		if (DIlru_free(err_code) == OK)
		{
		    continue;
		}
		else
		{
		    SETCLERR(err_code, 0, ER_popen);
		    ret_val = DI_EXCEED_LIMIT;
		    break;
		}
	    }
	}
        umask(oumask);

	if (ret_val == OK)
	{
	    /*
	    ** We have no idea what the real cause
	    **	of the error was;
	    */
	    i4  cmdstat = pclose(streamptr) >>8;

	    if ((ret_val = (cmdstat ? FAIL : OK)) != OK)
	    {
	        /*
	        ** FIX ME: add internal CL error message for /bin/mkdir failure.
	        **
	        ** something like: E_CLnnnn mkdir program exited with %0d

	        SETCLERR(err_code, E_CLnnnn, 0);
        	errdesc->moreinfo[0].size = sizeof(cmdstat);
        	errdesc->moreinfo[0].data._nat = cmdstat;

	        **
	        */
	    }
	}

	if (current_wd[0] != '\0')
	{
	    /* if this fails we will just not try to get back to this 
	    ** directory.  I don't think the code counts on getting back
	    ** anyway.  As documented getcwd only fails if size is 0
	    ** (error already caught above) or if size is not big enough
	    ** to hold string (by picking 256 I think this has been
	    ** eliminated)
	    */
	    chdir(current_wd);
	}
	    
	break;
    }

    return(ret_val);
}
#endif	/* xCL_008_MKDIR_EXISTS */
