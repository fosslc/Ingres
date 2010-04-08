/*
**	iitrcparm.c
**
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
** Name:	iitrcparm.c	-	Parameterized IItretcol
**
** Description:
**	Seems to be a compatability cover for an IIretparam call.
**
**	Public (extern) routines defined:
**		IItrc_param()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN	i4	IItcogetio();		/* final routine to call */

/*{
** Name:	IItrc_param	-	Parameterized IItcogetio 
**
** Description:
**	This routine seems to exist for compatability reasons.
**
**	It is basically just a cover for a call to IIretparam, with
**	IItcogetio passed as a parameter.
**
**	This routine is part of TBACC's external interface.
**	
** Inputs:
**	infmt		Ptr to the target list
**	inargv		Ptr to the argument vector
**	transfunc	Translating function
**
** Outputs:
**
** Returns:
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	27-apr-1983  -	Modified to interface with IItretcol(). (ncg)
**	19-mar-1987  -  Modified to call IItcogetio instead of IItcolret (drh)
*/

IItrc_param(infmt, inargv, transfunc)
char	*infmt;				/* pointer to the target list */
char	**inargv;			/* argument vector */
i4	(*transfunc)();			/* translating function */
{
	return (IIretparam(IItcogetio, infmt, inargv, transfunc));
}
