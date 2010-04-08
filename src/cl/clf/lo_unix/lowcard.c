
# include	<compat.h>
# include	<gl.h>
# include	<st.h>
# include       <clconfig.h>
# include       <systypes.h>
# include	<ex.h>
# include	<lo.h>
# include	"lolocal.h"
# include	<diracc.h>
# include	<sys/stat.h>
# include	<errno.h>

/*
**Copyright (c) 2004 Ingres Corporation
**
** LOwcard -- List Contents of Directory
**
**	LOwcard will open a directory for subsequent LOwnext operations.
**	The directory search has limited wildcarding capabilities, partitioning
**	the name in the same manner as LOdetail.
**
**	Parameters:
**		dirloc	directory location.
**		prefix	desired prefix, or NULL to wildcard.
**		suffix	desired suffix (extension), or NULL to wildcard.
**		version	desired version, ignored because UNIX has no versions.
**
**	Outputs:
**		lc - the context block containing open directory pointer /
**			pattern match information for subsequent LOwnext calls.
**
**		Returns:
**			OK if directory opened.
**
**	Called by:
**		ABF
**
**	Side Effects:
**		Opens a directory file.
**
**	History:
**
**		3/87 (bobm)	written
**		2/23/89 (daveb, seng) 	rename LO.h to lolocal.h
**				moved dir.h to hdr/diracc.h
**                              so we can share directory routines with di.
**		28-Feb-1989 (fredv)
**			Include <systypes.h>.
**		Sept-17-90 (bobm)
**			Make return first file, as per spec.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
*/


STATUS
LOwcard(dirloc,prefix,suffix,version,lc)
LOCATION *dirloc;
char *prefix;
char *suffix;
char *version;
register LO_DIR_CONTEXT *lc;
{
	LOCATION	tloc;

	DIR		*dirp;
	STATUS		status;

	lc->magic = LODIRMAGIC - 1;	/* in case we fail */

	/*
	** specify directory by setting filename to "."
	*/
	LOcopy(dirloc, lc->bufr, &tloc);
	LOfstfile(".", &tloc);

	if ((status = LOexist(&tloc)) != OK)
		return(status);

	/*
	** get a pointer to string representing directory.  We could probably
	** set this directly, but we might as well be careful.
	*/
	LOtos(&tloc,&(lc->path));

	/* 
	** open the directory.
	*/
  	if ((dirp = opendir(lc->path)) == (DIR *)NULL)
 		return(FAIL);

	/*
	** point to the filename portion of the path.  We are assuming
	** it ends in "/." here.  Point to the trailing period.
	*/
	lc->fn = lc->path + STlength(lc->path) - 1;

	/*
	** record the other pertinent information for lc.
	*/
	lc->dirp = (PTR) dirp;
	lc->magic = LODIRMAGIC;
	lc->suffix = suffix;
	lc->prefix = prefix;

	/*
	** fetch first file, as per spec.
	*/
	return LOwnext(lc, dirloc);
}
