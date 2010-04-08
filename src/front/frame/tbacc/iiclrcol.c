/*
**	iiclrcol.c
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
** Name:	iiclrcol.c
**
** Description:
**	Clear a column in a given row of a tablefield
**
**	Public (extern) routines defined:
**		IItclrcol()
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
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

GLOBALREF	TBSTRUCT	*IIcurtbl;

/*{
** Name:	IItclrcol	-	Clear a tblfld column/row cell
**
** Description:
**	Check that the frame is set up and that table is ok.
**	Clear the specific column -- FDclrcol().
**
**	Note that IIcurtbl is used to identify the tablefield that
**	the column is located in, and the cell cleared is in the
**	current row of that tablefield (IIcurtbl->tb_rnum-1).
**
**	This routine is part of TBACC's external interface.
**
** Inputs:
**	colname		Name of the column to clear
**
** Outputs:
**
** Returns:
**	i4	TRUE	tablefield column cleared
**		FALSE	error
**
** Exceptions:
**	none
**
** Examples and Code Generation:
**	## clearrow f1 t2 3 (col4, col5)
**
**	if (IItbsetio(cmCLEARR, "f1", "t2", 3) != 0) {
**	  IItclrcol("col4");
**	  IItclrcol("col5");
**	}
**
** Side Effects:
**
** History:
**	29-mar-1983	- written (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
*/

IItclrcol(colname)
char	*colname;
{
	char		*colptr;		/* pointer to col name	*/
	char		c_buf[MAXFRSNAME+1];

	if (IIfrmio == NULL)		/* no form active */
	{
		IIFDerror(RTFRACT, 0, NULL);
		return (FALSE);
	}
	if ((colptr=IIstrconv(II_CONV, colname, c_buf, (i4)MAXFRSNAME)) ==
		NULL)
	{
		/* no column name given */
		IIFDerror(TBNOCOL, 1, (char *) IIcurtbl->tb_name);
		return (FALSE);
	}
	/* clear displayed column */
	if (!FDclrcol(IIcurtbl->tb_fld, IIcurtbl->tb_rnum - 1, colptr))
	{
		IIUGerr(E_TB0001_Cannot_clear_column, 
			UG_ERR_ERROR, 1, colptr);
		return (FALSE);
	}
	return (TRUE);
}
