/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <pc.h>
#include    <errno.h>

/*
** Name: PCis_alive	    - Is this process still alive?
**
** Description:
**	This function can be called to determine whether a given process is
**	alive or not.
**
** Inputs:
**	pid		    Process to check
**
** Outputs:
**	none
**
** Returns:
**	TRUE		    Process appears to be alive. We also return TRUE if
**			    we can't tell whether the process is alive because
**			    a system problem keeps us from finding out.
**	FALSE		    Process is definitely NOT alive
**
** History:
**	3-jul-1992 (bryanp)
**	    Created
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/
bool
PCis_alive(PID pid)
{
    if (kill(pid,0) == -1)
    {
	if (errno == ESRCH)
	    return (FALSE);
    }

    return (TRUE);
}
