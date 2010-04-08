/*
**Copyright (c) 2004 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	<errno.h>
#include	"lolocal.h"
#include	<si.h>

/*
** History:
**		02/23/89 (daveb, seng)
**			rename LO.h to lolocal.h
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>".
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

/*LOerrno
**
**	Given a unix errno return a compat status.
**
*/

/* static char	*Sccsid = "@(#)LOerrno.c	3.1  9/30/83"; */

STATUS
LOerrno(errnum)
i4	errnum;		/* system int size */

{
	register STATUS		status_return;


	switch(errnum)
	{
	  case EPERM:
	  case EACCES:
		/* error occurred because path was inaccessable */
		status_return = LO_NO_PERM;
		break;

	  case EMFILE:
		/* too many open files */
		status_return = SI_EMFILE;
		break;

	  case ENFILE:
	  case ENOSPC:
		/* error occurred because of lack of space */
		status_return = LO_NO_SPACE;
		break;

	  case ENOENT:
	  case ENOTDIR:
		/* error occurred path didn't exist */
		status_return = LO_NO_SUCH;
		break;

	  default:
		/* some other error */
		status_return = FAIL;
		break;

	}

	return(status_return);
}
