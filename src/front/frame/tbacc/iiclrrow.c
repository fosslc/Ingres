/*
**	iiclrrow.c
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
# include	<frserrno.h>
# include	<er.h>
# include	"ertb.h"
# include	<ug.h>
# include	<rtvars.h>

/**
** Name:	iiclrrow.c
**
** Description:
**	Clear a row of a tablefield
**
**	Public (extern) routines defined:
**		IItclrrow()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	12/12/87 (dkh) - Changed winerr to IIUGerr.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF	TBSTRUCT	*IIcurtbl;

/*{
** Name:	IItclrrow	-	Clear a tablefield row
**
** Description:
**	Clears the current row of the current tablefield in the
**	current frame.	Uses IIcurtbl to determine which tablefield
**	is current, and IIcurtbl->tb_rnum-1 to get the row number
**	to clear.  Calls FDclrrow to do the actual work.
**
**	This routine is part of TBACC's external interface.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	i4	TRUE	Row was cleared
**		FALSE	Error in clearing
**
** Exceptions:
**	none
**
** Examples and Code Generation:
**	## clearrow f1 t2
**
**	if (IItbsetio(cmCLEARR, "f1", "t2", rowCURR) != 0) {
**	  IItclrrow();
**	}
**
** Side Effects:
**
** History:
**	15-mar-1983	- written (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
**
*/

IItclrrow()
{
	if (IIfrmio == NULL)
	{
		/* no form is currently active to run Equel/Forms */
		IIFDerror(RTFRACT, 0, NULL);
		return (FALSE);
	}
	/* clear displayed row */
	if (!FDclrrow(IIcurtbl->tb_fld, IIcurtbl->tb_rnum -1))
	{
		i4	rownum;

		rownum = IIcurtbl->tb_rnum;
		IIUGerr(E_TB0002_Cannot_clear_row, UG_ERR_ERROR, 1, &rownum);
		return (FALSE);
	}
	return (TRUE);
}
