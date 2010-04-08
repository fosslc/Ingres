# include <compat.h>
# include <gl.h>
# include <si.h>
# include <lo.h>

/*# include <io.h>*/
# include <fcntl.h>
# include <sys/stat.h>

# include "silocal.h"
/*
** Name: SIcreate
**
** Description:
**      Creates the file whose name is specified by lo. If the file already
**      exists, the function returns SI_CANT_OPEN.
**
** Inputs:
**      lo              ptr to a location containing a path and filename
**
** Outputs:
**      none
**
** Returns:
**      OK if the file was created
**
** History:
**      12-dec-1996 (wilan06)
**          Created
**	07-jan-2003 (somsa01)
**	    Updated to use CreateFile(), which is lower than open().
**	03-feb-2004 (somsa01)
**	    Backed out last changes for now.
*/
STATUS
SIcreate (LOCATION* lo)
{
    STATUS      status = OK;
    int         handle;
    char*       pathName = NULL;

    LOtos (lo, &pathName);
    handle = open (pathName, O_CREAT | O_EXCL, S_IREAD | S_IWRITE);
    if (handle != -1)
    {
        close (handle);
    }
    else
    {
        status = SI_CANT_OPEN;
    }
    return status;
}
