# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<pc.h>
# include	<ex.h>
# include	<si.h>
# include	<lo.h>
# include	"lolocal.h"
# include	<errno.h>
# include	<sys/stat.h>

/*
**Copyright (c) 2004 Ingres Corporation
**
**
**  LOdelete -- Delete a location.
**
**	The file or directory specified by loc is destroyed if possible.
**
**	success:  LOdelete deletes the file or directory and returns-OK.
**		  LOdelete will not recursively delete directories.  If
**		  a directory exist within the directory then it will fail.
**	failure:  LOdelete deletes no file or directory.
**
**		For a file, returns NO_PERM if the process does not have
**		  permissions to delete the given file or directory.  LOdelete
**		  returns NO_SUCH if part of the path, the file, or the direc-
**		  tory does not exist.
**
**	History:
**		10/87 (bobm) - integrate 5.0 changes
**
**		Revision 1.2  86/04/24  16:52:28  daveb
**		Remove comment cruft.
**		Use full pathnames for system commands.
**		Use capability baxe x42_FILESYS instead of BSD
**		  to help 4.n's masquerading as System V.
**		
**		Revision 1.2  86/03/10  01:04:33  perform
**		New pyramid compiler can't handle declaration of static's
**		within scope of use.
** 
**		Revision 30.7  86/06/27  12:59:56  boba
**		Special code for hp5.
**		
**		Revision 30.6  86/03/03  12:11:20  adele
**		Define x42_FILESYS for hp9000s200
**		
**		Revision 30.5  86/02/11  22:06:13  daveb
**		Use /bin/rmdir on SV instead of plain old rmdir.  This
**		is better security and marginally faster
**		
**		Revision 30.4  85/10/16  18:19:49  daveb
**		Use BSD calls correctly
**		
**		Revision 30.3  85/10/07  17:04:02  daveb
**		Use BSD rmdir system call if
**		available.
**
**		Revision 30.2  85/10/07  16:51:33  daveb
**		Correct definition of return status when delete
**		of a directory fails.
**
**		02/23/88 (daveb, seng)
**			rename LO.h to lolocal.h
**		28-Feb-1989 (fredv)
**			Include <systypes.h>.
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>".
**
**		08-jan-90 (sylviap)
**			Added PCcmdline parameters.
**      	08-mar-90 (sylviap)
**              	Added a parameter to PCcmdline call to pass an 
**			CL_ERR_DESC.
**	31-jan-92 (bonobo)
**	    Replaced multiple question marks; ANSI compiler thinks they
**	    are trigraph sequences.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-may-1998 (canor01)
**	    When encountering a subdirectory, just skip it and continue
**	    deleting files.  The call will fail on the directory delete.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	    Declare errno correctly (i.e. via errno.h).
*/

static STATUS	LOdeldir(char *dir);

STATUS
LOdelete(register LOCATION *loc)
{
	STATUS		status_return;

	STATUS		LOdeldir();

	if (loc->string == NULL)
	{
		/* no path in location */

		status_return = LO_NO_SUCH;
	}
	else if ((loc->path	  != NULL)	&&
		  (*(loc->path)   != '\0')	&&
		  ((loc->fname    == NULL)	||
		   (*(loc->fname) == '\0')))
	{
		status_return = LOdeldir(loc->string);
	}
	else
	{
		status_return = (unlink(loc->string) ? LO_NO_PERM : OK);
	}

	return(status_return);
}

/*
	LOdeldir()
*/

static	STATUS
LOdeldir(char *dirname)
{
# if defined(xCL_009_RMDIR_EXISTS) || defined(HP9000S500)
	STATUS		LOerrno();
# else	
	char		dirbuf[MAX_LOC + 1];
# endif
	CL_ERR_DESC	err_code;
	char		filename[MAX_LOC + 1];
	LOCATION	loc1,
			loc2,
			loc3;
	i2		is_dir;
	STATUS		Status;

	EXinterrupt(EX_OFF);

	LOfroms(PATH, dirname,   &loc1);
	LOcopy(&loc1, filename,  &loc3);

	while ((Status = LOlist(&loc1, &loc2)) == OK)
	{
		LOstfile(&loc2, &loc3);

		LOisdir(&loc3, &is_dir);

		if (is_dir != ISDIR)
			unlink(filename);
		else
		{
			Status = LO_NEST_DIR;
			continue;
		}
	}

	if (Status == ENDFILE)
	{
		/*
		** can't unlink directories unless you are root (?)
		*/

# ifdef xCL_009_RMDIR_EXISTS

		/* Use the system call and get real status */
		Status = ( rmdir( dirname ) ? LOerrno( errno ) : OK );

# else
# ifdef HP9000S500
		{
			i4	pid;

			if ((pid = fork()) == 0)
			{
				setuid(geteuid());
				exit(rmdir(dirname));
			}
			else if (pid > 0)
			{
				wait(&Status);
			}
			else
			{
				Status = FAIL;
			}
		}
# else
		/* Fake it using the program; no detailed status possible */
		STprintf(dirbuf, "/bin/rmdir %s", dirname);
		Status = (PCcmdline((LOCATION *) NULL, dirbuf, PC_WAIT, 
			NULL, &err_code) ? FAIL : OK) ;

# endif 	/* HP9000S500 */
# endif 	/* xCL_009_RMDIR_EXISTS */
	}

	EXinterrupt(EX_ON);

	return(Status);
}
