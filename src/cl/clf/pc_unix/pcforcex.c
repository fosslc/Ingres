/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <pc.h>
#include    <errno.h>
#include    <clsigs.h>

/*
** Name: PCforce_exit	    - force this process to exit.
**
** Description:
**	This function can be called to force the indicated process to exit.
**
** Inputs:
**	pid		    Process to force to exit
**
** Outputs:
**	none
**
** Returns:
**	STATUS
**
** History:
**	3-jul-1992 (bryanp)
**	    Created
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/
STATUS
PCforce_exit(PID pid, CL_ERR_DESC *err_code)
{
    if (kill(pid, SIGKILL) == -1)
	return (FAIL);

    return (OK);
}
