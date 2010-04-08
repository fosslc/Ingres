/*
**	chgtbrow.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	chgtbrow.c - Convert pseudo row number to a real one.
**
** Description:
**	File contains a routine to convert (normalize) a pseudo
**	row number to a real one.  Routine is:
**	- FDchgtbrow - Normalize a row number.
**
** History:
**	15-feb-1985	extracted from code in copy.c, tbutil.c (bab)
**	02/15/87 (dkh) - Added header.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h> 


/*
** CHGTBROW.c
**
** FDchgtbrow(tbl, row)
**	Normalizes the row number.
**
*/


/*{
** Name:	FDchgtbrow - Normalize a row number.
**
** Description:
**	Given a pseudo row number, normalize it based on the
**	number of rows in the passed in table field.
**	Valid pseudo row numbers are:
**	- fdtfTOP - the top row number.
**	- fdtfBOT - the bottom row number.
**	- fdtfCUR - the current row number.
**	Anything else is simply passed back.
**
** Inputs:
**	tbl	Table field to normalize against.
**	row	Pseudo row number to normalize.
**
** Outputs:
**	Returns:
**		row	The normalize row number.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	15-feb-1985	extracted from code in copy.c, tbutil.c (bab)
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDchgtbrow(tbl, row)
TBLFLD	*tbl;
i4	row;
{
	i4	result;

	switch (row)
	{
	  case fdtfTOP:
		result = 0;
		break;

	  case fdtfBOT:
		result = tbl->tfrows-1;
		break;

	  case fdtfCUR:
		result = tbl->tfcurrow;
		break;

	  default:
		result = row;
		break;
	}

	return (result);
}
