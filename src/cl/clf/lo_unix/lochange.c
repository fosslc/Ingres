/*
**Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#include	<cs.h>
#include	<errno.h>

/* Externs */
GLOBALREF bool		iiloChanged;	/* defined in "logt.c" */

/*{
** Name:	LOchange() -	Change location.
**
**	Change the LOCATION of the current process.  Return status. In VMS,
**	LOchange(xx:) means LOchange(xx:[0,0])
**
**	success:  LOchange changes the location of the current process to loc 
**		  and returns OK.
**	failure:  LOchange does not change location of current process.
**		  LOchange returns NO_PERM if the process does not have
**		  permissions to change the current process location.  LOchange
**		  returns NO_SUCH if the location doesn't exist.
**		  LOchange returns FAIL if some other situation comes up.
**
** Side Effects:
**	Sets 'iiloChanged' for 'LOgt()' whenever the directory is changed.
**
** History:
**	03/09/83 -- (mmm)
**		written
**	05/17/88 (jhw) -- Added set of 'iiloChanged'.
**	02/23/89 (daveb, seng)
**		rename LO.h to lolocal.h
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>".
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

STATUS
LOchange( loc )
register LOCATION	*loc;
{
	STATUS		LOerrno();

	if ( loc->fname != NULL && *loc->fname != EOS )
		return LO_NO_SUCH;	/* can't be file name */

	if ( loc->path == NULL )
		return LO_NO_SUCH;	/* must be path */

	if ( chdir(loc->string) < 0 )
	{
		return LOerrno( (i4)errno ); 
	}

	iiloChanged = TRUE;

	return OK;
}
