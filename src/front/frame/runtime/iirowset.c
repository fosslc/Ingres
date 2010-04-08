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
** Name:	iirowset.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIrowset()
**	Private (static) routines defined:
**
** History:
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	TBSTRUCT	*RTgettbl();

/*{
** Name:	IIrowset	-	QBF interface to move to row/col
**
** Description:
**	This is an interface for QBF ONLY!!
**
**	Move the cursor to a specified row and column
**
** Inputs:
**	formname	Name of the form
**	tablename	Name of the tablefield
**	row		Row number to resume to (1-based)
**	colname		Column name to resume to
**
** Outputs:
**
** Returns:
**	STATUS
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

STATUS
IIrowset(char *formname, char *tablename, i4 row, char *colname)
{
	RUNFRM		*runf;
	TBSTRUCT	*tblstr;
	TBLFLD		*tbl;

	if (((runf = RTfindfrm(formname)) == NULL)
	    || ((tblstr = RTgettbl(runf, tablename)) == NULL))
	{
		return(FAIL);
	}

	tbl = tblstr->tb_fld;
	if (row < 1 || row > tbl->tfrows)
	{
		return(FAIL);
	}
	/*
	**  Do zero-indexing.
	*/

	tbl->tfcurrow = row - 1;

	/*
	**  Set column.
	*/
	if (!FDgotocol(runf->fdrunfrm, tbl, colname))
	{
		return(FAIL);
	}
	return(OK);
}
