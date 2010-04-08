/*
**	Copyright (c) 2004 Ingres Corporation
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
# include	<frsctblk.h>
# include	<erfi.h>


/**
** Name:	fdinvis.c -	Routines to handle table field invisibility
**
** Description:
**	The routines in this file handle table field invisibility.
**
**	Routines defined are:
**		IIFDtvcTblVisibilityChg	Update tblfld structs after changed
**					visibility
**		IIFDmcvMakeColsVisible	Turn off all invisibility flags for
**					table field columns on the form
**		IIFDdvcDetermineVisCols	See if any table field columns are
**					invisible
**
** History:
**	08-sep-89 (bruceb)	Written
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDTYPE	*FDgettype();
FUNC_EXTERN	FLDVAL	*FDgetval();
    

/*{
** Name:	IIFDtvcTblVisibilityChg	- Update tblfld structs after changed
**					  visibility
**
** Description:
**	Update the size of the table field, and the starting offsets of the
**	table field columns, after something in the table field has changed
**	visibility.
**
** Inputs:
**	tbl		Table field to be updated.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	08-sep-89 (bruceb)	Written.
*/

VOID
IIFDtvcTblVisibilityChg(tbl)
TBLFLD	*tbl;
{
    bool	visible_column = FALSE;
    i4		colsep = 1;
    i4		i;
    i4		startpos = 1;
    FLDHDR	*thdr;
    FLDHDR	*chdr;
    FLDTYPE	*ctype;
    FLDCOL	**col;

    thdr = &(tbl->tfhdr);

    /* No need to update the table field if it isn't visible. */
    if (thdr->fhdflags & fdINVISIBLE)
    {
	return;
    }
    
    if (thdr->fhdflags & fdNOCOLSEP)
    {
	colsep = 0;
    }

    col = tbl->tfflds;
    for (i = 0; i < tbl->tfcols; i++, col++)
    {
	chdr = &((*col)->flhdr);
	if (!(chdr->fhdflags & fdINVISIBLE))
	{
	    visible_column = TRUE;
	    ctype = &((*col)->fltype);

	    /*
	    ** Set position of start of this column.
	    **
	    ** At the time this routine was written, all three structure
	    ** members contained the same value.  If this changes, this
	    ** routine will require major work.
	    */
	    chdr->fhtitx = chdr->fhposx = ctype->ftdatax = startpos;

	    /*
	    ** Next column will begin at a point to the right of this column
	    ** by the width of this column plus the column separator.
	    */
	    startpos += (chdr->fhmaxx + colsep);
	}
    }

    /* If no columns are visible now, then entire table field is invisible. */
    if (!visible_column)
    {
	thdr->fhdflags |= fdTFINVIS;
    }
    else
    {
	/* Adjust for the right edge of the table field. */
	if (!colsep)
	    startpos++;
	/* Set width of entire table field. */
	thdr->fhmaxx = startpos;
	thdr->fhdflags &= ~fdTFINVIS;
    }
}


/*{
** Name:	IIFDmcvMakeColsVisible	- Turn off all invisibility flags for
**					  table field columns on the form
**
** Description:
**	Turn off all column invisibility flags for table fields on the form.
**	If visibility actually changes for columns in a given table field,
**	call IIFDtvcTblVisibilityChg() to reconstruct that table field.
**	Used by printform so that FTbuild will display table fields with no
**	regard to visibility.
**
** Inputs:
**	frm		Frame to be updated.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	11-sep-89 (bruceb)	Written.
*/
VOID
IIFDmcvMakeColsVisible(frm)
FRAME	*frm;
{
    i4		i;
    i4		j;
    FIELD	**fld;
    TBLFLD	*tbl;
    FLDCOL	**col;
    bool	visibility_changed;

    fld = frm->frfld;
    for (i = 0; i < frm->frfldno; i++, fld++)
    {
	if ((*fld)->fltag == FTABLE)
	{
	    visibility_changed = FALSE;
	    tbl = (*fld)->fld_var.fltblfld;
	    col = tbl->tfflds;
	    for (j = 0; j < tbl->tfcols; j++, col++)
	    {
		if ((*col)->flhdr.fhdflags & fdINVISIBLE)
		{
		    visibility_changed = TRUE;
		    (*col)->flhdr.fhdflags &= ~fdINVISIBLE;
		}
	    }
	    if (visibility_changed)
	    {
		IIFDtvcTblVisibilityChg(tbl);
	    }
	}
    }
}

/*{
** Name:	IIFDdvcDetermineVisCols	- See if any table field columns are
**					  invisible
**
** Description:
**	Determine if any table field columns are invisible, and if so, call
**	IIFDtvcTblVisibilityChg() to reconstruct that table field.  Called
**	at runtime when the form is added.
**
** Inputs:
**	tbl		Table field to be checked and updated.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	11-sep-89 (bruceb)	Written.
*/
VOID
IIFDdvcDetermineVisCols(tbl)
TBLFLD	*tbl;
{
    i4		i;
    FLDCOL	**col;

    if (!(tbl->tfhdr.fhdflags & fdINVISIBLE))
    {
	col = tbl->tfflds;
	for (i = 0; i < tbl->tfcols; i++, col++)
	{
	    if ((*col)->flhdr.fhdflags & fdINVISIBLE)
	    {
		IIFDtvcTblVisibilityChg(tbl);
		break;
	    }
	}
    }
}
