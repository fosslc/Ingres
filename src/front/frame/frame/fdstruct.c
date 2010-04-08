/*
**	fdstruct.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	fdstruct.c - Utility routines for data structure access.
**
** Description:
**	File contains utility routine to get access to forms structures.
**	Routines defined:
**	- FDgethdr - Return FLDHDR pointer to a field.
**	- FDgettype - Return FLDTYPE pointer to a field.
**	- FDgetval - Return FLDVAL pointer to a field.
**
** History:
**	01/07/87 (dkh) - Initial version - taken from fdstart.c
**	02/15/87 (dkh) - Added header.
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



/*{
** Name:	FDgethdr - Return FLDHDR pointer for field.
**
** Description:
**	Given a FIELD pointer, return the FLDHDR pointer for the
**	field.  For a table field, return the FLDHDR pointer for the
**	current column.
**
** Inputs:
**	fld	Pointer to field for getting a FLDHDR pointer.
**
** Outputs:
**	Returns:
**		ptr	FLDHDR pointer for field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
FLDHDR *
FDgethdr(fld)
    reg FIELD	*fld;
{
    reg FLDCOL	*col;
    reg TBLFLD	*tbl;

    if (fld->fltag == FREGULAR)
    	return (&fld->fld_var.flregfld->flhdr);
    tbl = fld->fld_var.fltblfld;
    col = tbl->tfflds[tbl->tfcurcol];
    return (&col->flhdr);
}




/*{
** Name:	FDgettype - Return FLDTYPE pointer for field.
**
** Description:
**	Given a FIELD pointer, return the FLDTYPE pointer for the
**	field.  For a table field, return the FLDTYPE pointer for the
**	current column.
**
** Inputs:
**	fld	Pointer to field for getting a FLDTYPE pointer.
**
** Outputs:
**	Returns:
**		ptr	FLDTYPE pointer for field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
FLDTYPE *
FDgettype(fld)
    reg FIELD	*fld;
{
    reg FLDCOL	*col;
    reg TBLFLD	*tbl;

    if (fld->fltag == FREGULAR)
    	return (&fld->fld_var.flregfld->fltype);
    tbl = fld->fld_var.fltblfld;
    col = tbl->tfflds[tbl->tfcurcol];
    return (&col->fltype);
}




/*{
** Name:	FDgetval - Return FLDVAL pointer for field.
**
** Description:
**	Given a FIELD pointer, return the FLDVAL pointer for the
**	field.  For a table field, return the FLDVAL pointer for the
**	current cell (intersection of the current column and row).
**
** Inputs:
**	fld	Pointer to field for getting a FLDVAL pointer.
**
** Outputs:
**	Returns:
**		ptr	FLDVAL pointer for field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
FLDVAL *
FDgetval(fld)
    reg FIELD	*fld;
{
    reg FLDVAL	*val;
    reg TBLFLD	*tbl;

    if (fld->fltag == FREGULAR)
    	return (&fld->fld_var.flregfld->flval);
    tbl = fld->fld_var.fltblfld;
    val = tbl->tfwins+(tbl->tfcurrow * tbl->tfcols) + tbl->tfcurcol;
    return (val);
}
