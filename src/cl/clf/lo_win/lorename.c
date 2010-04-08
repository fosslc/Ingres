# include	<compat.h>
# include	<gl.h>
# include	<pc.h>
# include	<lo.h>
# include	"LOerr.h"

/* LOrename
**
**	Copyright (c) 1995, Ingres Corporation
**	History
**	19-may-95 (emmag)
**	    Branched for NT.
**	07-jan-2004 (somsa01)
**	    Re-worked to use MoveFile(), which performs the copy/delete
**	    atomically.
**	03-feb-2004 (somsa01)
**	    Backed out last change for now.
*/

STATUS
LOrename(old_loc, new_loc)
LOCATION	*old_loc;
LOCATION	*new_loc;
{
	STATUS	ret_val = OK;
	i2	whatzit;
	ULONG   status;


	LOisdir(old_loc, &whatzit);

	if (whatzit != ISFILE)
	{
		ret_val = LO_NOT_FILE;
	}
	else
	{
            status = CopyFile((LPSTR)old_loc->path,(LPSTR)new_loc->path,FALSE);
            if (status==TRUE)
   	    {
                if(DeleteFile(old_loc->path) == TRUE)
                    ret_val = OK;
                else
                    ret_val = FAIL;
            }
            else
            {
                ret_val = FAIL;
            }

	}
	return (ret_val);
}
