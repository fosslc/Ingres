
#include	<ds.h>

/*
**Copyright (c) 2004 Ingres Corporation
**
** dsclose.c -- Close shared data region
**
**	History:
**Revision 30.1  85/08/14  19:09:28  source
**llama code freeze 08/14/85
**
**		Revision 3.0  85/05/08  20:22:02  wong
**		Added copyright notice.
**		
**		Revision 1.3  85/04/29  13:17:26  joe
**		Check for Null file pointer
**
**		Revision 1.2  85/03/25  15:43:48  ron
**		Integration from VMS - now uses a file pointer and CL routines
**		instead of file descriptors and Unix system calls.
**
**		Revision 1.1  85/02/13  09:37:53  ron
**		Initial version of new DS routines from the CFE project.
**      27-may-97 (mcgem01)
**          Clean up compiler warnings.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
*/

STATUS
DSclose(sh_desc)
SH_DESC	*sh_desc;
{
	STATUS status;

	if (sh_desc->sh_type == IS_FILE && sh_desc->sh_reg.file.fp != NULL) 
	{
		status = SIclose(sh_desc->sh_reg.file.fp);
		sh_desc->sh_reg.file.fp = NULL;
		return status;
	}
}
