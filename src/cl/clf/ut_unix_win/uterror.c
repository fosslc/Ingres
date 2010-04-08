/*	SCCS ID = %W%  %G%	*/

# include	<compat.h>
# include	<gl.h>
# include	"utlocal.h"
# include	<er.h>
# include	<si.h>

/*
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		UTerror.c
**
**	Function:
**		UTerror
**
**	Arguments:
**		STATUS		Status;		* index into  CLerror file *
**
**	Result:
**		print error message associated with passed in Status value.
**
**	Side Effects:
**		None
**
**	History:
**		03/83 -- (gb)
**			written
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      20-mar-97 (mcgem01)
**          Rename UT.h to utlocal.h to avoid conflict with gl\hdr\hdr
*/


STATUS
UTerror(Status)
STATUS		Status;
{
	char	UT_err_buf[ER_MAX_LEN];


	ERreport(Status, UT_err_buf);

	SIfprintf(stdout, "%s\n", UT_err_buf);
	
	return OK;
}
