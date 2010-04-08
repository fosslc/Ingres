/*
**	copy.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	copy.c - Copy data around in a table field.
**
** Description:
**	File contains routines to copy data around in a table
**	field.  Need for table field scrolling support.
**	Routines defined here are:
**	- FDcopytab - Copy data in a particular direction.
**
** History:
**	30-apr-1984	Improved interface to routines for performance (ncg)
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



FUNC_EXTERN	i4	FDchgtbrow();


/*{
** Name:	FDcopytab - Copy rows of data in a table field.
**
** Description:
**	Copy the rows of a table field a particular direction.
**	Copies the rows frow<->trow-1 to frow+1<->trow
**
** Inputs:
**	tbl	Table field where data is to be copied.
**	frow	From (pseudo) row number.
**	trow	To (pseudo) row number.
**
** Outputs:
**	Returns:
**		TRUE	If rows copied successfully.
**		FALSE	If from/to row is out of bounds.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	30-apr-1984	Improved interface to routines for performance (ncg)
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDcopytab(tbl, frow, trow)
TBLFLD	*tbl;
i4	frow;
i4	trow;
{
	i4	from,
		to;

	from = FDchgtbrow(tbl, frow);
	if (from >= tbl->tfrows)
		return (FALSE);

	to = FDchgtbrow(tbl, trow);
	if (to >= tbl->tfrows)
		return (FALSE);

	FDcopyup(tbl, from, to);
	return (TRUE);
}
