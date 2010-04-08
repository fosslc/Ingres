/*
** Copyright (c) 1983, 2001 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include        <clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#include	<tm.h>		/* to get SYSTIME declaration */
#include	<systypes.h>
#include	<errno.h>
#include	<sys/stat.h>

/*LOlast
**
**	Location last modified.
**
**	Get last modification date of a LOCATION.
**
**		History
**			03/09/83 -- (mmm)
**				written
**			02/23/89 -- (daveb, seng)
**				rename LO.h to lolocal.h
**			28-Feb-1989 (fredv)
**				Include <systypes.h>
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>".
**	31-jul-2001 (somsa01)
**	    Use _stati64() if LARGEFILE64 is defined.
*/

/* static char	*Sccsid = "@(#)LOlast.c	3.1  9/30/83"; */


STATUS
LOlast(loc, last_mod)
register LOCATION	*loc;
register SYSTIME	*last_mod;

{
	STATUS		LOerrno();
#ifdef LARGEFILE64
	struct _stati64	statinfo;	/* the layout of the struct stat as defined in <stat.h> */
#else
	struct stat	statinfo;	/* the layout of the struct stat as defined in <stat.h> */
#endif  /* LARGEFILE64 */
	register i2	os_exitstatus;
	STATUS		status;
	extern int	errno;		/* system supplied variable of type int */


	/* statinfo is a buffer into which information is placed 
	** by stat concerning the file.  
	*/
#ifdef LARGEFILE64
	os_exitstatus = _stati64(loc->string, &statinfo);
#else
	os_exitstatus = stat(loc->string, &statinfo);
#endif  /* LARGEFILE64 */

	if (os_exitstatus != 0)
	{
		/* error occurred on stat call */
		last_mod->TM_secs = -1;
		last_mod->TM_msecs = -1;

		status = LOerrno((i4)errno);

	}
	else
	{
		/* return time_return location last modified */
		last_mod->TM_secs = statinfo.st_mtime;
		last_mod->TM_msecs = 0;
		status = OK;

	}

	return(status);
}
