/*
**	iiredisp.c
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
# include	<rtvars.h>

/**
** Name:	iiredisp.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIredisp()
**	Private (static) routines defined:
**
** History:
**	11/11/87 (dkh) - Code cleanup.
**	08/17/91 (dkh) - Added change to take care sync'ing up table field
**			 highlighted rows when user does a table field
**			 scroll and then a redisplay.  This is a less
**			 disruptive fix than the one in ft!ftsyndsp.c
**			 which becomes unnecessary with this fix.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN	i4	FDredisp();

/*{
** Name:	IIredisp	-	Redisplay current form
**
** Description:
**	An EQUEL '## REDISPLAY' statement will generate a call to
**	this routine.  Calls FDredisp to redisplay the current form.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Example and Code Generation:
**	## redisplay
**
**	IIredisp();
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

i4
IIredisp()
{
	FRAME	*frm;

	if (!RTchkfrs(IIstkfrm))
		return ((i4) FALSE);

	frm = IIstkfrm->fdrunfrm;

	/*
	**  Set rebuild flag so that all the components of
	**  a form is rebuilt.  This takes care of sync'ing
	**  table field highlighting to the most current
	**  row if user code has executed somesort of scroll
	**  statement and then executed a redisplay statement.
	*/
	frm->frmflags |= fdREBUILD;

	IIprev = frm;

	return(FDredisp(frm));
}
