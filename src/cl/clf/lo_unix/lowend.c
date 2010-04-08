
# include	<compat.h>
# include	<gl.h>
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
** LOwend -- Terminate directory search.
**
**	LOwend will terminate a directory search started by LOwcard.
**
**	Parameters:
**		lc - the context block containing open directory pointer.
**
**	Outputs:
**
**		Returns:
**			OK
**			FAIL.
**
**	Called by:
**		ABF
**
**	Side Effects:
**		closes directory file.
**
**	History:
**
**		3/87 (bobm)	written
**		02/23/89 (daveb, seng)	rename LO.h to lolocal.h
**				moved dir.h to hdr/diracc.h
**                         	so we can share directory routines with di.
**		28-Feb-1989 (fredv)
**			Include <systypes.h>
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/


STATUS
LOwend(lc)
LO_DIR_CONTEXT *lc;
{
	DIR *dirp;
	
	if (lc->magic != LODIRMAGIC)
		return(FAIL);

	dirp = (DIR *) lc->dirp;
	lc->magic = LODIRMAGIC + 1;

	closedir(dirp);

	return(OK);
}
