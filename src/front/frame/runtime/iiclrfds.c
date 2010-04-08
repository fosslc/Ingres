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
# include	<rtvars.h>

/**
** Name:	iiclrflds.c	-	Clear all fields in the frame.
**
** Description:
**
** History:
**	22-feb-1983  -  Extracted from original runtime.c (jen)
**	25-mar-1983  -	Modified to clear table fields. (ncg)
**	08/11/91 (dkh) - Passed FRAME pointer to IITBtclr() so that clearing
**			 a table field will clear out dataset attributes
**			 from the table field cells holding area as well.
**	03/01/92 (dkh) - Renamed IItclr() to IITBtclr() to avoid name
**			 space conflicts with eqf generated symbols.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*{
** Name:	IIclrflds	-	Clear all fields in current frame
**
** Description:
**	Clear all fields in the currently displayed frame.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**
** Outputs:
**
** Returns:
**	i4	The return code from FDclrflds.
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## clear field all
**
**	IIclrflds();
**
** Side Effects:
**
** History:
**	
*/

IIclrflds()
{
	TBSTRUCT	*tb;

	/*
	**	Check to see that the forms system has been
	**	initialized and that there is a current frame.
	*/
	if (!RTchkfrs(IIstkfrm))
		return (FALSE);

	/*
	** First clear table fields, so as to save data sets.
	** There is an extra step of re-clearing the table field later;
	** this can be avoided by just calling FDclrfld() for each field.
	*/
	for (tb = IIstkfrm->fdruntb; tb != NULL; tb = tb->tb_nxttb)
	{
			IITBtclr(tb, IIstkfrm->fdrunfrm);
	}
	return (FDclrflds(IIstkfrm->fdrunfrm));
}
