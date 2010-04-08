/*
**	iitscparm.c
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
** Name:	iitscparm.c	-	Paramaterized IItcoputio 
**
** Description:
**	Compatability cover to IIsetparam.
**
**	Public (extern) routines defined:
**		IItsc_param()
**	Private (static) routines defined:
**
** History:
**	27-apr-1983  -	Modified to interface with IItcolset (ncg)
**	19-mar-1987  -  Modified to call IItcoputio (drh)
**	08/14/87 (dkh) - ER changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/


/*{
** Name:	IItsc_param	-	Parameterized IItcoputio 
**
** Description:
**	This routine is a compatability cover for a call
**	to IIsetparam, with IItcolset as a parameter.
**
**	This routine is part of TBACC's external interface.
**	
** Inputs:
**	infmt		Target list format
**	inargv		Argument vector
**	transfunc	Translating function
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
**	
*/

IItsc_param(infmt, inargv, transfunc)
char	*infmt;				/* the target list format */
char	**inargv;			/* the argument vector */
i4	(*transfunc)();			/* translating function */
{
	return (IIsetparam(IItcoputio, infmt, inargv, transfunc));
}
