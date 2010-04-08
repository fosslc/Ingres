/*
**	iiderive.c
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
# include	<afe.h>

/**
** Name:	iiderive.c
**
** Description:
**	Functions to return row number, change flags, and internal dbv
**	for table field cells for use in derivation processing.
**
**	Public (extern) routines defined:
**		IITBgidGetIDBV()
**		IITBgrGetRow()
**		IITBgfGetFlags()
**	Private (static) routines defined:
**
** History:
**	10-jul-89 (bruceb)	Written.
**	13-apr-92 (seg)
**	    Can't dereference or do arithmetic on a PTR variable.  Must cast.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


/*{
** Name:	IITBgidGetIDBV	-	Return internal dbv for a TF cell
**
** Description:
**	Return a pointer to an internal dbv for a table field cell.  If
**	the cell is part of the on-screen table field, point to the idbv
**	in the FLDVAL structure for the cell.  Otherwise, return a pointer
**	into the dataset.
**
**	This routine expects that the frame properly contains information
**	on the cell's row number and/or dataset row and that this routine is
**	called only for the table field referenced in the frame.
**
** Inputs:
**	frm		Pointer to the frame containing the table field.
**	fld		Pointer to the table field.
**	colno		Sequence number of the table field column.
**
** Outputs:
**
** Returns:
**	Pointer to a dbv for the table field cell.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	10-jul-89 (bruceb)	Written.
**	13-apr-92 (seg)
**	    Can't dereference or do arithmetic on a PTR variable.  Must cast.
**	
*/

DB_DATA_VALUE	*
IITBgidGetIDBV(FRAME *frm, FIELD *fld, i4 colno)
{
    i4			rownum;
    i4			i;
    TBLFLD		*tf;
    DB_DATA_VALUE	*idbv;
    TBROW		*dsrow;
    RUNFRM		*runf;
    TBSTRUCT		*tb;
    ROWDESC		*rd;
    COLDESC		*cd;
    FLDVAL		*val;

    tf = fld->fld_var.fltblfld;
    rownum = frm->frres2->rownum;

    if (rownum == NOTVISIBLE)
    {
	dsrow = (TBROW *)frm->frres2->dsrow;
	runf = RTfindfrm(frm->frname);
	tb = IItfind(runf, tf->tfhdr.fhdname);
	rd = tb->dataset->ds_rowdesc;
	for (i = 0, cd = rd->r_coldesc; i < colno; i++, cd = cd->c_nxt)
	    ;
	idbv = &cd->c_dbv;
	idbv->db_data = (PTR)(cd->c_offset + (char *)dsrow->rp_data);
    }
    else
    {
	val = tf->tfwins + (rownum * tf->tfcols) + colno;
	idbv = val->fvdbv;
    }

    return(idbv);
}


/*{
** Name:	IITBgrGetRow	-	Return row number of the TF cell
**
** Description:
**	Return the row number of a table field cell.  This is 0 based,
**	and will return NOTVISIBLE if the row is in a part of the dataset
**	not currently displayed on the form.
**
**	This routine expects that the frame properly contains information
**	on the cell's row number and that this routine is called only
**	for the table field referenced in the frame.
**
** Inputs:
**	frm		Pointer to the frame containing the table field.
**
** Outputs:
**
** Returns:
**	Row number for the table field cell.  (NOTVISIBLE if an invisible row.)
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	10-jul-89 (bruceb)	Written.
**	
*/

i4
IITBgrGetRow(frm)
FRAME	*frm;
{
    return(frm->frres2->rownum);
}


/*{
** Name:	IITBgfGetFlags	-	Return change/validation flags for
**					the TF cell
**
** Description:
**	Return the flag region of a table field cell.  This either points
**	to the tffflags entry for the visible section of a table field or
**	into the dataset if the row is not visible.
**
**	This routine expects that the frame properly contains information
**	on the cell's row number and/or dataset row and that this routine is
**	called only for the table field referenced in the frame.
**
** Inputs:
**	frm		Pointer to the frame containing the table field.
**	fld		Pointer to the table field.
**	colno		Sequence number of the table field column.
**
** Outputs:
**
** Returns:
**	Pointer to the change/validation flag region for a table field cell.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	10-jul-89 (bruceb)	Written.
**	
*/
i4	*
IITBgfGetFlags(FRAME *frm, FIELD *fld, i4 colno)
{
    i4			rownum;
    i4			i;
    i4			*tfflags;
    TBLFLD		*tf;
    TBROW		*dsrow;
    RUNFRM		*runf;
    TBSTRUCT		*tb;
    ROWDESC		*rd;
    COLDESC		*cd;

    tf = fld->fld_var.fltblfld;
    rownum = frm->frres2->rownum;

    if (rownum == NOTVISIBLE)
    {
	dsrow = (TBROW *)frm->frres2->dsrow;
	runf = RTfindfrm(frm->frname);
	tb = IItfind(runf, tf->tfhdr.fhdname);
	rd = tb->dataset->ds_rowdesc;
	for (i = 0, cd = rd->r_coldesc; i < colno; i++, cd = cd->c_nxt)
	    ;
	tfflags = (i4 *)(cd->c_chgoffset + (char *)dsrow->rp_data);
    }
    else
    {
	tfflags = tf->tffflags + (rownum * tf->tfcols) + colno;
    }

    return(tfflags);
}
