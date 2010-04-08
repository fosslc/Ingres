/*
**	straverse.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	straverse.c - Special field traversal routines.
**
** Description:
**	Allows a routine to be run on each component of a field
** 	if the field is a regular field, then the routine is run of
**	its header, type and value
**	if it is a table field then it is run on each row and column
**	Routines defined:
**	- FDstbltrv - Special table field traversal.
**
** History:
**	11-4-84 - Duplicated from original traverse for speed (dkh)
**	02/17/87 (dkh) - Adde header.
**	02/22/91 (dkh) - Fixed bug 35879 (by rolling back changes to bug
**			 32798; Fix for 32798 will be done differently).
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

i4	FDsregtrv();

i4
FDsfldtrv(fld, routine, top)
FIELD	*fld;
i4	(*routine)();
i4	top;
{
	if (fld->fltag == FTABLE)
		return FDstbltrv(fld->fld_var.fltblfld, routine, top);
	else
		return FDsregtrv(fld->fld_var.flregfld, routine);
}

/*
** clear a regular field
*/

i4
FDsregtrv(fd, routine)
REGFLD	*fd;
i4	(*routine)();
{
	return(*routine)(&fd->flhdr, &fd->fltype, &fd->flval);
}

/*
** clear a table field
*/

/*{
** Name:	FDstbltrv - Special table field traversal.
**
** Description:
**	Special table field traversal.  Similar to FDtbltrv()
**	in that the passed in routine is called on each row/column
**	in the table field.  Except that only the FLDHDR, FLDTYPE
**	and FLDVAL pointers for a field is passed to the routine.
**	This is a faster path since we don't have to deal with
**	translating to and from the FT field identification format
**	that FDtbltrv() supports.
**
** Inputs:
**	tbl	Table field to traverse.
**	routine	Routine to call on each row/column.
**	top	Pseudo row number indicating the last row to traverse.
**		Valid values are:
**		- FDALLROW - all the rows.
**		- FDDISPROW - up to the last row with data.
**		- FDTOPROW - only the first row.
**		- anything else - only the first row.
**
** Outputs:
**	Returns:
**		TRUE	Traversal completed with no errors.
**		FALSE	Traversal failed on a row/column.
**	Exceptions:
**		None.
**
** Side Effects:
**	Any row/column that fails becomes the current row/column.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
*/
i4
FDstbltrv(tbl, routine, top)
TBLFLD	*tbl;
i4	(*routine)();
i4	top;
{
	register i4		i;
	i4			oldrow;
	i4			lastrow = 1;

	oldrow = tbl->tfcurrow;
	switch (top)
	{
	  case FDALLROW:
		lastrow = tbl->tfrows;
		break;

	  case FDDISPROW:
		lastrow = tbl->tflastrow + 1;
		break;

	  case FDTOPROW:
		lastrow = 1;
	}
	for (i = 0; i < lastrow; i++)
	{
		tbl->tfcurrow = i;
		if (!FDsrowtrv(tbl, i, routine))
			return (FALSE);
	}
	tbl->tfcurrow = oldrow;
    	return (TRUE);
}

FDsrowtrv(tbl, rownum, routine)
TBLFLD	*tbl;
i4	rownum;
i4  	(*routine)();
{
	i4	j;
	FLDVAL	*win;
	FLDCOL	**type;
	i4	oldcol;

	oldcol = tbl->tfcurcol;
	win = fdtblacc(tbl, rownum, 0);
	for(j = 0, type = tbl->tfflds; j < tbl->tfcols;
	    j++, win++, type++)
	{
		/* update current column for each loop */
		tbl->tfcurcol = j;
		if (!(*routine)(&(*type)->flhdr, &(*type)->fltype, win))
			return (FALSE);
	}
	tbl->tfcurcol = oldcol;
	return (TRUE);
}




/*{
** Name:	IIFDlrwLastRowTrv - Traverse last row in table field.
**
** Description:
**	This routine is used to specially traverse the last row of a
**	table field.  We only called the passed routine on a
**	column in the row if the column as the I_CHG bit set.
**
**	This is necessary to make sure that we don't change the
**	values in the last row unnecessarily when FDckffld is
**	called.
**
** Inputs:
**	tbl		Table field to traverse on.
**	rownum		The physical screen row number (zero based) to traverse.
**	routine		The magic routine to be called for each column
**			if fdI_CHG is set for the column.
**
** Outputs:
**
**	Returns:
**		TRUE	If "routine" traversed all columns OK.
**		FALSE	If traversal of columns failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/22/91 (dkh) - Initial version.
*/
i4
IIFDlrtLastRowTrv(tbl, rownum, routine)
TBLFLD	*tbl;
i4	rownum;
i4  	(*routine)();
{
	i4	j;
	FLDVAL	*win;
	FLDCOL	**type;
	i4	oldcol;
	i4	*pflags;

	oldcol = tbl->tfcurcol;
	win = fdtblacc(tbl, rownum, 0);
	for(j = 0, type = tbl->tfflds; j < tbl->tfcols;
	    j++, win++, type++)
	{
		pflags = tbl->tffflags + (rownum * tbl->tfcols) + j;
		if (!(*pflags & fdI_CHG))
		{
			continue;
		}
		/* update current column for each loop */
		tbl->tfcurcol = j;
		if (!(*routine)(&(*type)->flhdr, &(*type)->fltype, win))
			return (FALSE);
	}
	tbl->tfcurcol = oldcol;
	return (TRUE);
}
