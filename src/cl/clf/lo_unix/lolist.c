
# include	<compat.h>
# include	<gl.h>
# include       <clconfig.h>
# include	<systypes.h>
# include	<me.h>
# include	<st.h>
# include	<ex.h>
# include	<lo.h>
# include	"lolocal.h"
# include	<diracc.h>
# include	<sys/stat.h>
# include	<errno.h>
# include	<handy.h>

/*
** Copyright (c) 1985, 2000 Ingres Corporation
**
** LOlist -- List Contents of Directory
**
**	LOlist will return  the next file or directory in the passed in
**	location.  The initial call gets the first file -- subsequent calls
**	return successive locations.   LOlist will return OK as long as there
**	was a file to return.  If one wishes to stop retrieving locations
**	before the end call LOendlist.
**
**	Parameters:
**
**		inputloc: pointer to location from which to search for files.
**		outputloc:pointer to location where retrieved location is stored.
**		flag: returns ISDIR if location returned is a directory.
**		      returns ISFILE if location returned is a file.
**
**	Returns:
**		LOlist returns OK as long as there was another file 
**		to be found.
**
**	Called by:
**
**	Side Effects:
**		First call opens a file pointer associated with the inputed
**		locations directory file.  Subsequent calls use this file 
**		pointer to get the next location.
**
**	Trace flags:
**
**	Error Returns:
**		Returns non zero on error.
**
**	History:
**
**		4/1/83  -- (mmm) written
**		7/20/83 -- (fhc) changed to work with unix 4.1c
**		2/23/89 -- (daveb, seng) rename LO.h to lolocal.h
**				  moved dir.h to hdr/diracc.h
**                         	  so we can share directory routines with di.
**		28-Feb-1989 (fredv)
**			Include <systypes.h>.
**			Added ifdef xCL_025_DIRENT_EXISTS to select the right
**				directory structure.
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Remove code to null-terminate d_name, readdir already does this;
**		Remove EX_NO_FD stuff leftover from 5.0.
**	24-may-90 (blaise)
**	    Removed line of code which null-terminates d_name (missed in the
**	    above change).
**
**	NOTE - this routine is actually obsolete, and FE use of same
**		should be replaced with LOwcard / LOwnext / LOwend
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-may-95 (tutto01)
**	    Changed readdir call to the reentrant iiCLreaddir.
**	03-jun-1996 (canor01)
**	    Made readdir_buf into a structure aligned for struct dirent
**	    to avoid alignment problems when used with reentrant readdir.
**	28-feb-2000 (somsa01)
**	    Use stat64() when LARGEFILE64 is defined.
*/


STATUS
LOlist(inputloc, outputloc)
register LOCATION	*inputloc;
register LOCATION	*outputloc;
{
	LOCATION	dirloc;
	DIR		*dirp;
	char		tempbuf[MAX_LOC+1];
	LOCATION	tloc;
	char	tbuf[MAX_LOC+1];

#ifdef xCL_025_DIRENT_EXISTS
	struct dirent	*dirptr;
#else /* xCL_025_DIRENT_EXISTS */
	struct direct	*dirptr;
#endif /* xCL_025_DIRENT_EXISTS */
#ifdef LARGEFILE64
	struct   stat64	statinfo;
#else /* LARGEFILE64 */
	struct   stat	statinfo;
#endif /* LARGEFILE64 */
	register STATUS status;
	struct
	{
            struct dirent   dt;
            char            buf[MAX_LOC+1];
	} readdir_buf;

	status = OK;

	/* the file to be opened is: "input path"/. */

	LOcopy(inputloc, tempbuf, &dirloc);
	LOfstfile(".", &dirloc);

	if (inputloc->fp != NULL)
	{
		/*
		** Coerce the handle in the input location
		** back to the DIR handle we filled in before.
		*/
		dirp = (DIR *)inputloc->fp;
	}
	else
	{
		/* this is the first call so open up the directory file */

		if (status = LOexist(&dirloc))
			return(status);

		/* the directory does exist */

		dirp = opendir(inputloc->string);

		/* 
		** Ugly pun to hide the DIR structure.  Hope
		** No-one does any raw i/o on the location...
		*/
		inputloc->fp = (FILE *)dirp;

		if (dirp == NULL)
			return(FAIL);
	}


	while ((dirptr = iiCLreaddir(dirp, 
				     (struct dirent *) &readdir_buf)) != NULL)
	{
		if (dirptr->d_ino == 0)
		{
			/* this file was deleted */
			continue;
		}

		STcopy(dirptr->d_name, tbuf);

		if( !STcompare( tbuf, "." ) || !STcompare( tbuf, ".." ) )
			continue;

		STcopy(inputloc->string, tempbuf);
		LOfroms(PATH,   tempbuf, &tloc);
		if (LOfstfile(tbuf, &tloc))
 			return (FAIL);

#ifdef LARGEFILE64
		if (stat64(tempbuf, &statinfo))
#else /* LARGEFILE64 */
		if (stat(tempbuf, &statinfo))
#endif /* LARGEFILE64 */
		{
			if (errno == ENOENT)
				continue;
			return(FAIL);
		}
		/* This is used by LOdbname */
		else if (statinfo.st_mode & S_IFDIR)
			LOfroms(PATH, tbuf, outputloc);
		else
			LOfroms(FILENAME, tbuf, outputloc);

		return(OK);
	}

	/* reached end of the directory file */

	LOendlist(inputloc);

	return(ENDFILE);
}
