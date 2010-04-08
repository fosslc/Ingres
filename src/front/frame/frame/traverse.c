/*
**	traverse.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	traverse.c - General routines to traverse a field/form.
**
** Description:
**	Allows a routine to be run on each component of a field.
**	If the field is a regular field, then the routine is run
**	of its header, type and value.	If it is a table field then
**	it is run on each row and column.
**	Routines defined in this file are:
**	- FDFTsetiofrm - Sets static frame pointer for traversal.
**	- FDFTgetiofrm - Return the pointer set by FDFTsetiofrm.
**	- FDFTsmode - Set a static mode.
**	- FDfldtrv - Traverse a field.
**	- FDcmptbl - Get the field number of a table field in a form.
**	- FDftbl - Set form pointer and field number of table field.
**	- FDtbltrv - Traverse a table field.
**	- FDrowtrv - Traverse a row in a table field.
**
** History:
**	02/15/87 (dkh) - Added header.
**	06/20/86 (scl) Changed variable name(s) to avoid namespace
**		conflict: tbltrvfldno -> trvtblfldno   (tbltrvfrm)
**	19-jun-87 (bab) Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	17-apr-90 (bruceb)
**		Call on FDemptyrow takes 2 fewer args--unused.
**	02/22/91 (dkh) - Fixed bug 35879 (by rolling back change for bug
**			 32798; Fix for 32798 will be done differently).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-Aug-2007 (hanal04) Bug 118248
**          Make FDFTiofrm a global. IIdelfrm() needs to initialise FDFTiofrm
**          when freeing the associated memory.
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
# include	<er.h>
# include	"erfd.h"


FUNC_EXTERN	i4	FDnodata();
FUNC_EXTERN	i4	FDdat();
FUNC_EXTERN	i4	IIFDlrtLastRowTrv();

GLOBALDEF	FRAME	*FDFTiofrm = NULL;
static	i4	FDFTmode = -1;
static	FRAME	*tbltrvfrm = NULL;
static	i4	trvtblfldno = -1;
static	bool	fromtbltrv = FALSE;

GLOBALREF	FRAME	*frcurfrm;



/*{
** Name:	FDFTsetiofrm - Set a static FRAME pointer for I/O.
**
** Description:
**	Set a static to the passed in FRAME pointer so that I/O
**	will occur correctly.  This is necessary if the forms
**	system is processing a clearing a form that is not
**	currently displayed.  In this case, the display buffer
**	for the form must be cleared.
**
** Inputs:
**	frm	FRAME pointer to set.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDFTsetiofrm(frm)
FRAME	*frm;
{
	FDFTiofrm = frm;
}


FRAME *
FDFTgetiofrm()
{
	return(FDFTiofrm);
}



/*{
** Name:	FDFTsmode - Set static to appropriate FIELD chain.
**
** Description:
**	Set static to FT_UPDATE or FT_DISPONLY so that the correct
**	field is accessed as a result of a field traversal on a
**	form that is not currently displayed.
**
** Inputs:
**	mode	Either FT_UPDATE or FT_DISPONLY.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDFTsmode(mode)
i4	mode;
{
	FDFTmode = mode;
}

/*
**  Please note that this routine does not
**  take into account checking for empty
**  table fields.  Consequently this routine
**  should not be called to check for valid
**  data in a table field. (dkh)
*/

i4
FDfldtrv(fieldno, routine, top)
i4	fieldno;
i4	(*routine)();
i4	top;
{
	FIELD	*fld;
	TBLFLD	*tbl;
	reg	i4	i;
	reg	i4	j;
	i4	oldrow;
	i4	oldcol;
	i4	maxcol;
	i4	lastrow = 1;

	if (FDFTmode == FT_UPDATE)
	{
		fld = FDFTiofrm->frfld[fieldno];
	}
	else
	{
		fld = FDFTiofrm->frnsfld[fieldno];
	}

	if (fld->fltag == FTABLE)
	{
		tbl = fld->fld_var.fltblfld;
		oldrow = tbl->tfcurrow;
		oldcol = tbl->tfcurcol;
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
		maxcol = tbl->tfcols;
		for (i = 0; i < lastrow; i++)
		{
			for (j = 0; j < maxcol; j++)
			{
				if (!(*routine)(FDFTiofrm, fieldno, FDFTmode, FT_TBLFLD, j, i))
				{
					return(FALSE);
				}
			}
		}
		tbl->tfcurrow = oldrow;
		tbl->tfcurcol = oldcol;
		return(TRUE);

	}
	else
	{
		return((*routine)(FDFTiofrm, fieldno, FDFTmode, FT_REGFLD, (i4) 0, (i4) 0));
	}
}




/*{
** Name:	FDcmptbl - Find field number of table field in form.
**
** Description:
**	Given a table field and a form, find the table field in the
**	form.  If the table field is in the form, return the field
**	sequence number of the table field.  Otherwise, return -1.
**	Since table fields can not be display only, that chain of
**	fields is not checked.
**
** Inputs:
**	frm	Form to look for table field in.
**	tbl	Table field to find in form.
**
** Outputs:
**	Returns:
**		-1	If table field not in form.
**		seqno	Field sequence number of table field in form.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDcmptbl(frm, tbl)
FRAME	*frm;
TBLFLD	*tbl;
{
	FIELD	**fld;
	i4	i;
	i4	maxfld;

	for (i = 0, maxfld = frm->frfldno, fld = frm->frfld; i < maxfld; i++, fld++)
	{
		if ((*fld)->fltag == FTABLE)
		{
			if (tbl == (*fld)->fld_var.fltblfld)
			{
				return(i);
			}
		}
	}
	return (-1);
}






/*{
** Name:	FDftbl - Set form pointer a field number of table field.
**
** Description:
**	Given a table field point, find it either in the form pointed
**	to by FDFTiofrm or frcurfrm.  If found, return the form that
**	it was found in and the field number of the table field in the
**	appropriate passed in parameters.  Look in FDFTiofrm first.
**
** Inputs:
**	tbl	Table field to look for.
**
** Outputs:
**	frmptr	Set to FRAME pointer that table field is in.
**	fldptr	Set to field number of table field.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDftbl(tbl, frmptr, fldptr)
TBLFLD	*tbl;
FRAME	**frmptr;
i4	*fldptr;
{
	i4	fldno;

	*frmptr = NULL;

	if (FDFTiofrm != NULL && (fldno = FDcmptbl(FDFTiofrm, tbl)) != -1)
	{
		*frmptr = FDFTiofrm;
		*fldptr = fldno;
		return;
	}
	if (frcurfrm != NULL && (fldno = FDcmptbl(frcurfrm, tbl)) != -1)
	{
		*frmptr = frcurfrm;
		*fldptr = fldno;
	}
}


/*
** clear a table field
*/

/*{
** Name:	FDtbltrv - Traverse a table field.
**
** Description:
**	Traverse a table field by calling FDrowtrv() for each of the
**	rows in the specified range.
**
** Inputs:
**	tbl	Table field to traverse.
**	routine Routine to call for each cell.
**	top	Encoding for the range of rows to traverse.
**
** Outputs:
**	Returns:
**		TRUE	If no errors in traversal.
**		FALSE	If traversal produced errors.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDtbltrv(tbl, routine, top)
TBLFLD	*tbl;
i4	(*routine)();
i4	top;
{
	register i4		i;
	i4			oldrow;
	i4			lastrow = 1;

	FDftbl(tbl, &tbltrvfrm, &trvtblfldno);
	if (tbltrvfrm == NULL)
	{
		syserr(ERget(E_FD0022_Cant_find));
	}

	fromtbltrv = TRUE;

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
		/*
		**  Allow for last row to be empty.
		**  If row not empty then do validation
		**  check with FDrowtrv.
		**  Fix to BUG 5199. (dkh)
		*/
		if (i == lastrow - 1 && routine == FDnodata)
		{
			if (FDemptyrow(tbl, i))
			{
				/*
				**  Clear the field storage area (fvstore)
				**  for this row.  In the event that row
				**  was emptied by spacing over the previous
				**  contents, must sync up fvstore with the
				**  display.  Fix for bug 8638.	 (bab)
				*/
				IIFDlrtLastRowTrv(tbl, i, FDdat);
				continue;
			}
		}
		tbl->tfcurrow = i;
		if (!FDrowtrv(tbl, i, routine))
		{
			fromtbltrv = FALSE;
			return (FALSE);
		}
	}
	tbl->tfcurrow = oldrow;
	fromtbltrv = FALSE;
	return (TRUE);
}

FDrowtrv(tbl, rownum, routine)
TBLFLD	*tbl;
i4	rownum;
i4	(*routine)();
{
	FRAME	*frm;
	i4	fldno;
	i4	j;
	i4	oldcol;

	if (!fromtbltrv)
	{
		FDftbl(tbl, &frm, &fldno);
		if (frm == NULL)
		{
			syserr(ERget(E_FD0023_Cant_find));
		}
	}
	else
	{
		frm = tbltrvfrm;
		fldno = trvtblfldno;
	}

	oldcol = tbl->tfcurcol;
	for (j = 0; j < tbl->tfcols; j++)
	{
		/* update current column for each loop */
		tbl->tfcurcol = j;
		if (!(*routine)(frm, fldno, FT_UPDATE, FT_TBLFLD, j, rownum))
			return (FALSE);
	}
	tbl->tfcurcol = oldcol;
	return (TRUE);
}
