/* static char	*Sccsid = "@(#)LOcreate.c	3.9	9/20/84"; */

#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	<errno.h>
#include	"lolocal.h"
#include	<ex.h>
#include	<lo.h>
#include	<nm.h>
#include	<pc.h>
#include	<si.h>
#include	<st.h>

/*LOcreate
**
**	Create location.
**
**	The file or directory specified by lo is created if possible. The
**	file or directory is empty, but the exact meaning of empty is imple-
**	mentation-dependent.
**
**	success: LOcreate creates the file or directory and returns-OK.
**	failure: LOcreate creates no file or directory.
**
**		LOcreate returns NO_PERM if the process does not have
**		  permissions to create file or directory. 
**		LOcreate returns NO_PERM if mkdir tries to create a directory 
**		  under a nonexistant directory.  
**		LOcreate will return NO_SPACE if not enough space to create. 
**		LOcreate returns NO_SUCH if path does not exist. 
**		LOcreate returns FAIL if some other situation comes up.
**
**	On SV, LOcreate will return FAIL for any error creating a directory,
**	  since we have no errno from the failed mkdir program.
**
**Copyright (c) 2004 Ingres Corporation
**
**	History
**		08/08/85 - zapped LOspecial() and chowndir.  BSD42 runs
**			   mkdir with the effective user's permissions
**			   (no problem). System V mkdir uses access() which
**			   runs with the real users permission.  "chowndir"
**			   (which ran as root and used mknod, not mkdir)
**			   came into existence because we were trying to
**			   create directories with a full-path-name so
**			   the real user was required to have search
**			   permission along the entire path (not very
**			   secure for a DBMS).  We now LOchange() to the
**			   "base" directory where the new directory will
**			   be created and do the mkdir there.  This
**			   requires the real user to have write
**			   permission in the "base" directory; therefore
**			   the INGRES protection scheme is changed so
**			   that the "base" directories under data, ckp,
**			   jnl, etc. are PEworld("rwxrwx").  data, ckp
**			   jnl, etc. remain PEworld("rwx--") so the data
**			   is still protected (modulo UNIX).  When LOcreate()
**			   is used to create databases (ckps, jnls) it's now
**			   necessary to use PEumask("rwxrxw"), otherwise
**			   the REAL user's permissions may preclude ingres
**			   from accessing the database (ckp, jnl).
**
**			   It should also be noted that the database directories
**			   will be owned by the REAL user on SV and "ingres"
**			   (the effective user) on BSD42.  This is not a
**			   change but has been a point of confusion.
**
**		11/12/85 (gb) -- creat() returns file descriptor not FILE *.
**			   raise EX_NO_FD if run out of Unix file descriptors.
**		02/23/89 (daveb, seng)
**			rename LO.h to lolocal.h
**	26-may-89 (daveb)
**		silence warnings by including <st.h> to declare STrindex().
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Add "include <clconfig.h>";
**		Remove EX_NO_FD stuff leftover from 5.0.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-nov-2004 (hayke02)
**	    Change mkdir() permissions from 777 to 700.
**	15-Jun-2004 (schka24)
**	    Watch out for auto variable overrun.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	    Declare errno correctly (i.e. via errno.h).
*/

STATUS
LOcreate(loc)
register LOCATION	*loc;
{
	i4		file_desc;
	STATUS		ret_val;
# ifndef	xCL_008_MKDIR_EXISTS
	char		string[MAXSTRINGBUF];
	FILE		*streamptr;
	char		*dbname;
	bool		stacked_dir = FALSE;
# endif		/* xCL_008_MKDIR_EXISTS */


	STATUS 		LOerrno();
	FILE		*popen();
	i4		creat();


	ret_val = OK;

	if (loc->desc == PATH)
	{
		/* then create a directory */
		if ((loc->path == NULL) || (*(loc->path) == '\0'))
		{
			/* no path in location */
			ret_val = LO_NO_SUCH;
		}
		else	/* try to do it */
		{
			char	dir_buf[MAX_LOC + 1];

			/* a path string does exist--try to make a directory  */
			STlcopy(loc->string, dir_buf, sizeof(dir_buf)-1);

# ifdef	xCL_008_MKDIR_EXISTS

			/* Use the mkdir system call; much easier than on SV */
			if( OK != mkdir(dir_buf, 0700) )
				ret_val = LOerrno( errno );

# else

			if (dbname = STrchr(dir_buf, SLASHSTRING))
			{
				LOCATION	base_loc;
				char		t_buf[MAX_LOC + 1];

				STcopy(dbname + 1, t_buf);
				*dbname = NULLCHAR;

				LOfroms(PATH, dir_buf, &base_loc);

				/*
				** LOsave/LOreset so user knows where
				** they are and can use relative paths
				** (though I don't believe INGRES uses
				** this "feature").  Do LOreset() even
				** if LOchange() fails to clear queue.
				*/

				ret_val = FAIL;

				if ((stacked_dir = (LOsave() == OK))	&&
					((ret_val = LOchange(&base_loc)) == OK))
				{
					STcopy(t_buf, dir_buf);
				}
			}

			STprintf(string, "/bin/mkdir %s", dir_buf);
			if( (streamptr = popen(string, "r")) == NULL)
			{
				ret_val = FAIL;
			}
			else
			{
				/*
				** We have no idea what the real cause
				**	of the error was;
				*/

				ret_val = (pclose(streamptr) ? FAIL : OK);
			}

			if (stacked_dir)
				LOreset();

# endif	/* xCL_008_MKDIR_EXISTS */
		}
	}
	else	/* not a PATH */
	{
		/* try to create a file */
		/* the creat command creates a file with a mode
		** set by the second argument modified by "umask"
		** which will insure proper permission settings of
		** mode of the file
		*/

                if ((file_desc = creat(loc->string, 0664)) == -1)
                	ret_val = LOerrno(errno);
                else
			close(file_desc);
	}

	return(ret_val);
}
