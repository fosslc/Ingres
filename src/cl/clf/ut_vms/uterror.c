# include	<compat.h> 
#include    <gl.h>
# include	<array.h>
# include	<ut.h>
# include	<er.h> 
# include	<si.h> 

/*
 *	Copyright (c) 1983 Ingres Corporation
 *
 *	Name:
 *		UTerror.c
 *
 *	Function:
 *		UTerror
 *
 *	Arguments:
 *		STATUS		Status;		* index into  CLerror file *
 *
 *	Result:
 *		print error message associated with passed in Status value.
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83 -- (gb)
 *			written
 *
 *
**      16-jul-93 (ed)
**	    added gl.h
 */

UTerror(Status, outf)
STATUS		Status;
FILE		*outf;
{
	char	UT_err_buf[ER_MAX_LEN];


	ERreport(Status, UT_err_buf);
	if(outf)
		SIfprintf(outf, "%s\n", UT_err_buf);
	else
		SIfprintf(stdout, "%s\n", UT_err_buf);
}	
