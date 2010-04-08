/*
**		Copyright (c) 1983 Ingres Corporation
*/
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
**	06-may-1996 (canor01)
**	    Cleaned up compiler warnings.
**
*/

STATUS
LOcreate(loc)
register LOCATION	*loc;
{
    HANDLE      file_desc;
    STATUS	ret_val;
    ULONG	status;

    ret_val = OK;

    if (loc->desc == PATH)
    {
        /* then create a directory */
        if (!loc->path || !*loc->path) /* no path in location */
                ret_val = LO_NO_SUCH;
        else if (CreateDirectory(loc->string, NULL) == FALSE)
        {
                status = GetLastError();
                ret_val = LOerrno( status );
        }
    }
    else        /* not a PATH */
    {
        if ((file_desc = CreateFile(loc->string, GENERIC_WRITE,
                                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,NULL)) 
					              == INVALID_HANDLE_VALUE)
        {
                status = GetLastError();
                ret_val = LOerrno(status);
        }
        else
                CloseHandle(file_desc);
    }
    return ret_val;
}
