/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>

/**
** Name:	iirfparam.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIrfparam()
**	Private (static) routines defined:
**
** History:
**	25-feb-1987	Modified to call IIgetfldio instead of IIretfield (drh)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN i4	IIgetfldio();

/*{
** Name:	IIrf_param	-	Parameterized IIgetfldio interface
**
** Description:
**	Cover routine for a call to IIretparam.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	infmt		Ptr to the target list
**	inargv		Argument vector
**	transfunc	Data translating function
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	16-feb-1983  -  Extracted from original runtime.c (jen)
**	17-may-1983  -	Now calls general IIretparam() (ncg)
**	25-feb-1987  -  Changed to pass IIgetfldio as the routine to call
**			instead of IIretfield (drh).
**
**	
*/

IIrf_param(infmt, inargv, transfunc)
char	*infmt;				/* pointer to the target list */
char	**inargv;			/* argument vector */
i4	(*transfunc)();			/* data translating function */
{
	return (IIretparam(IIgetfldio, infmt, inargv, transfunc));
}
