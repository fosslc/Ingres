# include	<compat.h>
# include	<gl.h>
# include       <clconfig.h>
# include	<systypes.h>
# include	<me.h>
# include	<lo.h>
# include	<st.h>
# include	"lolocal.h"
# include	<diracc.h>
# include	<sys/stat.h>
# include	<errno.h>
# include	<handy.h>

/*
** Copyright (c) 1987, 2000 Ingres Corporation
**
** LOwnext -- Get next file in directory
**
**	LOwnext will return the next file in a directory opened by LOwcard
**
**	Parameters:
**		lc - the context block containing open directory pointer /
**			pattern match information
**
**	Outputs:
**		loc - next file
**
**		Returns:
**			OK if file found
**			ENDFILE if no more files
**			or FAIL.
**
**	Called by:
**		ABF
**
**	Side Effects:
**
**	History:
**
**		3/87 (bobm)	written
**		02/23/89 (daveb, seng)	rename LO.h to lolocal.h
**				moved dir.h to hdr/diracc.h
**                         	so we can share directory routines with di.
**		28-Feb-1989 (fredv)
**			Include <systypes.h>.
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug::
**		Remove code to null-terminate d_name, readdir already does this.
**	sept. 90 (bobm) - fix reassignment of '.' in wildcard check.
**	21-may-92 (dave)
**	    Don't pass back "." and "..".
**	27-may-92 (andys)
**	    Look at each thing we return to determine if it is a file or a
**	    directory and use the appropriate call to LOfroms. This is 
**	    necessary because other LO functions such as LOdelete use 
**	    the existence of fname to determine whether they are dealing 
**	    with a file or directory. This information is filled in by 
**	    the LOfroms call depending on the flags we pass here.
**	06-aug-92 (andys)
**	    Use STequal instead of STcompare as per DaveB suggestion.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      23-may-95 (tutto01)
**          Changed readdir call to the reentrant iiCLreaddir.
**	03-jun-1996 (canor01)
**	    Made readdir_buf into a structure aligned for struct dirent
**	    to avoid alignment problems when used with reentrant readdir.
**	24-apr-1997 (canor01)
**	    If the suffix is NULL and file is found with no suffix,
**	    we still need to check the prefix for a match.
**	    Previously it was matching any file with no suffix.
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	28-feb-2000 (somsa01)
**	    Use stat64() if LARGEFILE64 is defined.
*/


STATUS
LOwnext(lc,loc)
LO_DIR_CONTEXT *lc;
LOCATION *loc;
{
	DIR 		*dirp;
	struct dirent 	*drec;
	char 		*dotptr;
#ifdef LARGEFILE64
	struct stat64   statinfo;
#else /* LARGEFILE64 */
	struct stat     statinfo;
#endif /* LARGEFILE64 */
	struct
	{
	    struct dirent   dt;
	    char            buf[MAX_LOC+1];
	} readdir_buf;

	
	if (lc->magic != LODIRMAGIC)
		return(FAIL);

	dirp = (DIR *) lc->dirp;

	while ((drec = iiCLreaddir(dirp, (struct dirent *) &readdir_buf)) != NULL)
	{
		if (drec->d_ino == 0)
		{
			/* this file was deleted */
			continue;
		}

		STcopy(drec->d_name, lc->fn);

		/*
		**  Skip current and parent directories.
		*/
		if (STequal(drec->d_name, ".") ||
			STequal(drec->d_name, ".."))
		{
			continue;
		}

		/*
		** wildcard checks
		*/
		if ((dotptr = STrchr(lc->fn,'.')) != NULL)
		{
			*dotptr = '\0';
			++dotptr;
			if (lc->suffix != NULL &&
					STcompare(lc->suffix,dotptr) != 0)
				continue;
			if (lc->prefix != NULL &&
					STcompare(lc->prefix,lc->fn) != 0)
				continue;
			--dotptr;
			*dotptr = '.';
		}
		else
		{
			if (lc->suffix != NULL)
				continue;
			if (lc->prefix != NULL &&
					STcompare(lc->prefix,lc->fn) != 0)
				continue;
		}

		/*
		** passed wildcard checks - file is one we're interested
		** in.
		*/

		/*
		** is what we've found a directory or a file?
		*/
#ifdef LARGEFILE64
		if (stat64(lc->path, &statinfo) != 0)
#else /* LARGEFILE64 */
		if (stat(lc->path, &statinfo) != 0)
#endif /* LARGEFILE64 */
			return(FAIL);

		if (statinfo.st_mode & S_IFDIR)
			return(LOfroms(PATH,lc->path,loc));     /* directory */
		else
			return(LOfroms(PATH&FILENAME,lc->path,loc)); /* file */
	}

	/* end of directory list */
	return(ENDFILE);
}
