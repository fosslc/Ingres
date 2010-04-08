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
** Name:	iisfparam.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIsf_param()
**	Private (static) routines defined:
**
** History:
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN i4	IIputfldio();

/*{
** Name:	IIsf_param	-	Parameterized IIputfldio interface
**
** Description:
**	Cover routine (for compatability?) that calls IIsetparam.
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
**	17-may-1983  -	Now calls general IIsetparam() (ncg)
**	25-feb-1987  -  Modified to pass IIputfldio as the routine to
**			call, instead of IIsetfield (drh)
**	
*/

IIsf_param(infmt, inargv, transfunc)
char	*infmt;				/* pointer to the target list */
char	**inargv;			/* argument vector */
i4	(*transfunc)();			/* data translating function */
{
	return (IIsetparam(IIputfldio, infmt, inargv, transfunc));
}
