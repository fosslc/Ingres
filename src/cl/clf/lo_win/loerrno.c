#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	<errno.h>
#include	"lolocal.h"
#include	<si.h>

/*
** History:
**	19-may-95 (emmag)
**	    Branched for NT.
**	18-feb-1997 (canor01)
**	    Add ERROR_SHARING_VIOLATION.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
*/

STATUS
LOerrno(errnum)
ULONG	errnum;		
{
	register STATUS		status_return;

        switch(errnum)
        {
          case ERROR_ACCESS_DENIED:
          case ERROR_DRIVE_LOCKED:
          case ERROR_CURRENT_DIRECTORY:
	  case ERROR_SHARING_VIOLATION:
                /* error occurred because path was inaccessible */
                status_return = LO_NO_PERM;
                break;

          case ERROR_TOO_MANY_OPEN_FILES:
                status_return = SI_EMFILE;
                break;

          case ERROR_CANNOT_MAKE:
          case ERROR_DISK_FULL:
                /* error occurred because of lack of space */
                status_return = LO_NO_SPACE;
                break;

          case ERROR_INVALID_DRIVE:
          case ERROR_FILE_NOT_FOUND:
          case ERROR_NOT_DOS_DISK:
          case ERROR_NOT_SAME_DEVICE:
          case ERROR_PATH_NOT_FOUND:
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
