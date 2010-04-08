/*
NO_OPTIM = i64_aix
*/
# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<frsctblk.h>
# include	<frserrno.h>
# include       <er.h>
# include	"fdfuncs.h"


/*
**	tbutil.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	tbutil.c - Utility routines for table field manipulation.
**
** Description:
**	This file contains several utility routines for table field
**	manipulation.
**
** Defines   -
**		FDclrrow(tf, row) 	 	- clears specified row
**		FDclrcol(tf, row, col) 	 	- clears specified column
**		FDckrow (tf, row)	 	- validates row
**		FDckcol (tf, row, col) 	 	- validates column
**		FDgotocol(tf, col)	 	- resumes on a column
**		FDcoltrv(tf, row, col, routine)	- traverses a column
**		FDcolop (tf, row, col, oper)   	- queries column
**		FDinsqcol (tf, row, col, oper) 	- insert query operator
** Local     -
**		FDtblOK (tf, row) 	 	- checks validity of arguments
** Return    -
**		all the routines return TRUE for success, or FALSE for failure.
** History:
**	15-mar-1983	Written	 	(ncg)
**	21-nov-1983	Added FDgotocol (ncg)
**	21-dec-1983	Added FDcoltrv to fix bug 1861 (ncg)
**	12-jan-1984	Added Query mode and FDcolop, FDinsqcol (ncg)
**	30-apr-1984	Improved interface to routines (ncg)
**	05-jan-1987 (peter)	Changed RTfnd* to FDfnd.
**	02/10/87 (dkh) - Added support for ADTs.
**	02/14/87 (dkh) - Added header
**	19-jun-87 (bruceb)	Code cleanup.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/16/87 (dkh) - Integrated table field change bit.
**	12-jul-89 (bruceb)
**		Clear row and clear column now account for derived columns.
**	25-jul-89 (bruceb)
**		'Clear row display' routine added to clear the display
**		ignoring the data or change bits, and ignoring constant-value
**		derived columns.
**		Set 'in_validation' while validation occurs.
**		Set up for derivation aggregate processing.
**		Added IIFDndcNoDeriveClr() to indicate that FDclrrow()
**		should clear with no regard to derivations.  (They'll be
**		handled as appropriate by the caller.)
**	18-apr-90 (bruceb)
**		Lint change; FDtfsgfix called with 2 args, not 3.
**	10/11/90 (dkh) - Put in fix to make sure source columns
**			 for aggregates show up when a new row is opened up.
**	12/12/92 (dkh) - Fixed trigraph warnings from acc.
**	03/14/93 (dkh) - Fixed bug 49883.  When doing a clearrow for
**			 a table field fake row, we don't clear columns
**			 with edit masks enabled.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-jul-2001 (toumi01)
**	    NO_OPTIM i64_aix for now to avoid compiler bug:
**	    **COMPILER ERROR**: Line 267, File \
**		/tbuild/010206c/tobey/include/flow.ch
**	    FlowGraph[64] out of range 0-6
**	    <<< FIXME - this is temporary for AIX 5L 5.1 beta >>>
**/

FUNC_EXTERN i4		FDclr();		/* clears display area */
FUNC_EXTERN i4		FDdat();		/* clears data area    */
FUNC_EXTERN i4		IIFDccb();		/* clears data area    */
FUNC_EXTERN FLDCOL	*FDfndcol();
FUNC_EXTERN	i4	(*IIseterr())();
FUNC_EXTERN	i4	IIFDdecDerErrCatcher();
FUNC_EXTERN	VOID	IIFDiaaInvalidAllAggs();
FUNC_EXTERN	VOID	IIFViaInvalidateAgg();

GLOBALREF	FRS_GLCB	*IIFDglcb;

i4	FDcoltrv();

static	bool	no_derive_clear = FALSE;

static	bool	fakerowopt = FALSE;


/*{
** Name:	FDclrrow - Clear a row in a table field.
**
** Description:
**	Clear one of the display rows (and the corresponding data storage
**	area) of the passed in table field.  Clearing the data storage
**	really puts the system default value into the storage area.
**
** Inputs:
**	tf	Table field containing row to be cleared.
**	row	Row number of row to clear.
**
** Outputs:
**	Returns:
**		TRUE	The row was cleared successfully.
**		FALSE	If an invalid row was passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	no_derive_clear is reset to FALSE;
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDclrrow(tf, row)
TBLFLD	*tf;
i4	row;
{
	FRAME	*frm;
	i4	fldno;
	i4	j;
	FLDVAL	*val;
	FLDCOL	**col;
	FLDHDR	*hdr;
	FIELD	fld;
	i4	(*oldproc)();
	bool	ignore_derivations = no_derive_clear;

	no_derive_clear = FALSE;

	/* check validity of table field */
	if (!FDtblOK(tf, &row))
	    return (FALSE);

	FDftbl(tf, &frm, &fldno);
	if (frm == NULL)
	{
	    IIFDerror(TBLNOFRM, 0, (char *) NULL);
	    PCexit (-1);
	}

	/* Traverse the row erasing everything that isn't derived. */
	IIFDiaaInvalidAllAggs(frm, tf);
	val = tf->tfwins + (row * tf->tfcols);
	col = tf->tfflds;
	frm->frres2->rownum = row;
	fld.fltag = FTABLE;
	fld.fld_var.fltblfld = tf;
	for (j = 0; j < tf->tfcols; j++, val++, col++)
	{
	    hdr = &(*col)->flhdr;
	    /* Do nothing if the column is derived. */
	    if (!(hdr->fhd2flags & fdDERIVED) || ignore_derivations)
	    {
		/* Clear the display. */
		if (!(fakerowopt && (hdr->fhd2flags & fdUSEEMASK)))
		{
			if (!FDclr(frm, fldno, FT_UPDATE, FT_TBLFLD, j, row))
		    		return (FALSE);
		}
		/* Clear the data. */
		if (!FDdat(hdr, &(*col)->fltype, val))
		    return (FALSE);
		if (!IIFDccb(frm, fldno, FT_UPDATE, FT_TBLFLD, j, row))
		    return (FALSE);
	    }
	}

	for (j = 0, col = tf->tfflds; j < tf->tfcols; j++, col++)
	{
	    hdr = &(*col)->flhdr;
	    if (!(hdr->fhd2flags & fdDERIVED) || ignore_derivations)
	    {
		if ((hdr->fhdrv != NULL) && (!ignore_derivations))
		{
		    oldproc = IIseterr(IIFDdecDerErrCatcher);

		    /* Invalidate the derivation dependents of this column. */
		    _VOID_ IIFDsdvSetDeriveVal(frm, &fld, j);

		    _VOID_ IIseterr(oldproc);
		}
	    }
	}
	col = tf->tfflds;

	return (TRUE);
}


/*{
** Name:	FDclrcol - Clear a column.
**
** Description:
**	Clear the display and the data storage area of the specified
**	ROW_COLUMN of the table field. If the column name is
**	NULL then use the current column.  Clearing the data storage
**	really puts the system default value into the storage area.
**
** Inputs:
**	tf	Table field where ROW_COLUMN resides.
**	row	Row number to use in the clearing function.
**	colname	Column name to use in the clearing function.
**
** Outputs:
**	Returns:
**		TRUE	If the ROW_COLUMN was cleared.
**		FALSE	If a bad row number or unknown column was
**			passed in.
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
FDclrcol(tf, row, colname)
register TBLFLD	*tf;
i4		row;
char		*colname;
{
	FLDCOL	*col;
	FLDHDR	*hdr;
	FRAME	*frm;
	i4	fldno;
	FIELD	fld;
	i4	(*oldproc)();

	/* check validity of table field */
	if (!FDtblOK(tf, &row))
		return (FALSE);

	/* current column */
	if (colname == NULL)
	{
		col = NULL;
	}
	/* check validity of column */
	else if ((col = FDfndcol(tf, colname)) == NULL)
	{
		return (FALSE);
	}

	FDftbl(tf, &frm, &fldno);
	if (frm == NULL)
	{
	    IIFDerror(TBLNOFRM, 0, (char *) NULL);
	    PCexit (-1);
	}

	hdr = &(col->flhdr);
	if (hdr->fhd2flags & fdDERIVED)
	{
	    IIFDerror(E_FI2267_8807_SetDerived, 0, (char *) NULL);
	    return(TRUE);	/* FALSE would generate a second error msg. */
	}
	FDcoltrv(tf, row, col, FDclr);
	FDscoltrv(tf, row, col, FDdat);
	FDcoltrv(tf, row, col, IIFDccb);

	if (hdr->fhdrv != NULL)	/* Derivation source field. */
	{
	    if (tf->tfhdr.fhd2flags & fdAGGSRC)
	    {
		IIFViaInvalidateAgg(hdr, fdIASET);
		frm->frmflags |= fdINVALAGG;
	    }

	    frm->frres2->rownum = row;
	    fld.fltag = FTABLE;
	    fld.fld_var.fltblfld = tf;

	    oldproc = IIseterr(IIFDdecDerErrCatcher);

	    /* Invalidate the derivation dependents of the column. */
	    _VOID_ IIFDsdvSetDeriveVal(frm, &fld, col->flhdr.fhseq);

	    _VOID_ IIseterr(oldproc);
	}
	return (TRUE);
}



/*{
** Name:	FDckrow - Check a row in table field.
**
** Description:
**	Validate the data in the passed row of the table field. Besides
**	validation, this routine is also usful in making sure that the
**	displayed data corresponds to the data being stored, by a call
**	to FDgetdata().  If the table field is in query mode, then
**	FDqrydta() is called instead.
**
**	If validation check fails, then the table field's current row is
**	assigned to the invalid row.  Note that to call FDgetdata() which
**	calls vl_evalfld() one must assign the current row to the row being
**	validated - this is beacuse vl_evaltree() uses only the current row!
**	(It would be nice if it passed a row number to it?)
**
** Inputs:
**	tf	Table field to check.
**	row	Specific row to check on.
**
** Outputs:
**	Returns:
**		TRUE	If validation passed.
**		FALSE	If an invalid row passed in or invalid data
**			was found in the row that was checked.
**	Exceptions:
**		None.
**
** Side Effects:
**	If the validation fails, then the invalid row becomes the
**	current row for the table field.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDckrow(tf, row)
TBLFLD	*tf;
i4	row;
{
	i4	oldrow;
	FRAME	*frm;
	i4	dummy;

	/* check validity of table field */
	if (!FDtblOK(tf, &row))
		return (FALSE);

	FDftbl(tf, &frm, &dummy);
	if (frm == NULL)
	{
	    IIFDerror(TBLNOFRM, 0, (char *) NULL);
	    PCexit (-1);
	}

	/* save current row for success */
	oldrow = tf->tfcurrow;
	/* assign for later v_evaltree and for bad validations */
	tf->tfcurrow = row;
	/* traverse row validating each column */
	if (tf->tfhdr.fhdflags & fdtfQUERY)
	{
		if (!FDrowtrv(tf, row, FDqrydata))
		{
		    return (FALSE);
		}
	}
	else
	{
		IIFDglcb->in_validation |= VLD_ROW;

		IIFDiaaInvalidAllAggs(frm, tf);
		if (!FDrowtrv(tf, row, FDgetdata))
		{
		    IIFDglcb->in_validation &= ~VLD_ROW;
		    return (FALSE);
		}

		IIFDglcb->in_validation &= ~VLD_ROW;
	}
	/* validated, reassign current row */
	tf->tfcurrow = oldrow;
	return (TRUE);
}


/*{
** Name:	FDckcol - Validate a column in a table field.
**
** Description:
**	Validate the data for a specific ROW_COLUMN.  If the column name is
**	NULL then validate the current column. This routine also
**	assures that the data displayed corresponds to the data in the
**	storage area, by a call to FDgetdata().  FDqrydata() is called
**	if table field is in query mode.
**
**	If validation check fails, then the table field's current row is
**	assigned to the invalid row, and the current column set to the bad
**	column.  To see why we save see note in FDckrow().
**
** Inputs:
**	tf	Table field to be checked.
**	row	Row number to use in the check.
**	colname	Column name of column to check.
**
** Outputs:
**	Returns:
**		TRUE	If validation passed.
**		FALSE	If invalid row number or unknown column passed in
**			or validtion failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	See description above.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDckcol(tf, row, colname)
register TBLFLD	*tf;
i4		row;
char		*colname;
{
	FLDCOL		*col;
	FLDHDR		*hdr;
	i4		oldrow;
	i4		oldcol;
	FRAME		*frm;
	i4		dummy;

	/* check validity of table field */
	if (!FDtblOK(tf, &row))
		return (FALSE);

	FDftbl(tf, &frm, &dummy);
	if (frm == NULL)
	{
	    IIFDerror(TBLNOFRM, 0, (char *) NULL);
	    PCexit (-1);
	}

	/* save current row and column numbers for success */
	oldrow = tf->tfcurrow;
	oldcol = tf->tfcurcol;

	/* find current column window if no column name */
	if (colname == NULL)
	{
		col = NULL;
	}
	/* find corresponding column */
	else if ((col = FDfndcol(tf, colname)) == NULL)
	{
		/*
		**  Fix for BUG 7631. (dkh)
		*/
		IIFDerror(TBBADCOL, 2, tf->tfhdr.fhdname, colname);
		return (FALSE);
	}

	hdr = &(col->flhdr);
	/* assign row and column for bad validations */
	tf->tfcurrow = row;
	tf->tfcurcol = hdr->fhseq;

	if (tf->tfhdr.fhdflags & fdtfQUERY)
	{
		if (!FDcoltrv(tf, row, col, FDqrydata))
		{
		    return (FALSE);
		}
	}
	else
	{
		IIFDglcb->in_validation |= VLD_COL;

		frm->frres2->rownum = row;
		if (hdr->fhdrv && (tf->tfhdr.fhd2flags & fdAGGSRC))
		{
		    IIFViaInvalidateAgg(hdr, fdIAGET);
		    frm->frmflags |= fdINVALAGG;
		}
		if (!FDcoltrv(tf, row, col, FDgetdata))
		{
		    IIFDglcb->in_validation &= ~VLD_COL;
		    return (FALSE);
		}

		IIFDglcb->in_validation &= ~VLD_COL;
	}

	tf->tfcurrow = oldrow;
	tf->tfcurcol = oldcol;
	return (TRUE);
}


/*{
** Name:	FDgotocol - Go to a particular column.
**
** Description:
**	Given a table field and column name set the current field in the
**	form to the table field and the current column for the table
**	field to the passed in column.  It is an error if the column
**	does not exist in the table field.
**
** Inputs:
**	frm	Pointer to form where table field resides.
**	tf	Pointer to table field where column resides.
**	colname	Name of column to make current.
**
** Outputs:
**	Returns:
**		TRUE	When update was successful.
**		FALSE	If passed in column does not exist.
**	Exceptions:
**		None.
**
** Side Effects:
**	If the table field is not the current field already, call
**	FDsgtfix() to do any necessary cleanup.  This is necessary
**	in case the current field is a table field and the system
**	is in the middle of a scroll{up/down} activation.  The current
**	table field's current row number will be invalid in this
**	situation.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDgotocol(frm, tf, colname)
FRAME		*frm;
register TBLFLD	*tf;
char		*colname;
{
	register FLDCOL	*col;

	/* check validity of column */
	if ((col = FDfndcol(tf, colname)) == NULL)
		return (FALSE);

	/*
	**  Fix the current row of the current field if we
	**  are moving to a different field out of an
	**  actviate scroll{up/down} block.
	*/
	if (frm->frcurfld != tf->tfhdr.fhseq)
	{
		FDtfsgfix(frm, tf->tfhdr.fhseq);
	}

	frm->frcurfld = tf->tfhdr.fhseq;
	tf->tfcurcol = col->flhdr.fhseq;
	return (TRUE);
}


/*
** FDCOLTRV() - traverse a column using the passed routine. If no coumn is
**		passed use the current one.
*/

i4
FDcoltrv(tf, row, col, routine)
register TBLFLD	*tf;
i4		row;
register FLDCOL	*col;
i4		(*routine)();
{
	i4	fldno;
	FRAME	*frm;
	i4	column;

	FDftbl(tf, &frm, &fldno);
	if (frm == NULL)
	{
		IIFDerror(TBLNOFRM, 0, (char *) NULL);
		PCexit (-1);
	}

	if (col == NULL)
	{
		/* is current column in range */
		if (tf->tfcurcol < 0 || tf->tfcurcol >= tf->tfcols)
			return (FALSE);
		col = tf->tfflds[tf->tfcurcol];
	}

	column = col->flhdr.fhseq;

	return((*routine)(frm, fldno, FT_UPDATE, FT_TBLFLD, column, row));
}

/*
** FDSCOLTRV() - traverse a column using the passed routine. If no coumn is
**		passed use the current one.
*/

i4
FDscoltrv(tf, row, col, routine)
register TBLFLD	*tf;
i4		row;
register FLDCOL	*col;
i4		(*routine)();
{
	FLDVAL	*val;

	if (col == NULL)
	{
		/* is current column in range */
		if (tf->tfcurcol < 0 || tf->tfcurcol >= tf->tfcols)
			return (FALSE);
		col = tf->tfflds[tf->tfcurcol];
	}
	/* get corresponding value, using 2-D construction of "tfwins" */
    	val = tf->tfwins +(row * tf->tfcols) + col->flhdr.fhseq;
	return (*routine)(&col->flhdr, &col->fltype, val);
}

/*{
** Name:	FDcolop - Get query operator for a cell.
**
** Description:
**	Get the query operator for a visible cell in a table field.
**	The name of the column may be NULL, which means to use
**	the current column.  It is an error if the specified
**	column does not exist, if the DB_DATA_VALUE where the
**	operator is to be placed is not a non-NULLable integer
**	data type or if the table field is not in query mode.
**
**	Optimization note: this routine can be combined with
**	FDqryop() but may be done at a later time.
**
** Inputs:
**	tf	Pointer to a TBLFLD structure.
**	row	Row number that cell is on.
**	colname	Column name, may be NULL to denote current column.
**
** Outputs:
**	oper	Pointer to a i4  to put operator.
**
**	Returns:
**		TRUE	If everything worked.
**		FALSE	If one of the errors listed above occurs.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/11/87 (dkh) - Added support for ADTs.
*/
i4
FDcolop(tf, row, colname, oper)
register TBLFLD	*tf;
i4		row;
char		*colname;
i4		*oper;
{
	FLDCOL		*col;

	/* check validity of table field */
	if (!FDtblOK(tf, &row))
		return (FALSE);

	/* current column */
	if (colname == NULL)
	{
		col = NULL;
	}
	/* find corresponding column */
	else if ((col = FDfndcol(tf, colname)) == NULL)
	{
		return (FALSE);
	}
	if ((tf->tfhdr.fhdflags & fdtfQUERY) && (oper != NULL))
	{
		if (FDcoltrv(tf, row, col, FDqrydata))
		{
		    	*oper = col->fltype.ftoper;
			return (TRUE);
		}
	}
	return (FALSE);
}


/*{
** Name:	FDinsqcol - Insert a query operator for a column
**
** Description:
**	Insert a query operator in front of the data for a cell
**	in a table field.  Useful for putting back the query
**	operator when table field scrolling is happening.
**	Should only be called in Query mode.
**	If input argument "val" is non-NULL then use it and
**	argument "col" to determine where query operator should
**	go.  Otherwise, Use "colname" and "row" to determine where to put
**	the query operator.  It is an error if the DB_DATA_VALUE
**	for the operator is not a non-NULLable integer data type.
**
**	Note:
**		FDcopyup/down calls this routine after already checking out the
**		column. If colval is set then immediately insert query values.
**		Called from IIdisprow() and Copy().
**
** Inputs:
**	tf	Pointer to a TBLFLD structure.
**	row	Row number of cell in table field.
**	colname	Name of column in table field.
**	oper	Pointer to DB_DATA_VALUE holding the query operator.
**	colval	Pointer to a FLDVAL structure to use in lieu of
**		colname.
**	realcol	Pointer to a FLDCOL strucutre to use in liue of
**		colname.
**
** Outputs:
**	None.
**
**	Returns:
**		TRUE	If operator was successfully placed.
**		FALSE	If there is no where to place the operator.
**	Exceptions:
**		None.
**
** Side Effects:
**	The query operator for the cell is set to the passed in value.
**	In addition, the display for the field is updated with the
**	operator being placed at the front of the field.
**
** History:
**	xx/xx/xx (ncg) - Updated to use FDputoper().
**	02/11/87 (dkh) - Added support for ADTs.
*/
i4
FDinsqcol(tf, row, colname, oper, colval, realcol)
register TBLFLD	*tf;
i4		row;
char		*colname;
i4		oper;
FLDVAL		*colval;
FLDCOL		*realcol;
{
	FLDCOL		*col;
	i4		fldno;
	FRAME		*frm;

	if (colval)
	{
		col = realcol;
	}
	else
	{
		/* check validity of table field */
		if (!FDtblOK(tf, &row))
			return (FALSE);

		/* find corresponding column */
		if ((col = FDfndcol(tf, colname)) == NULL)
			return (FALSE);
	}

	FDftbl(tf, &frm, &fldno);
	if (frm == NULL)
	{
		IIFDerror(TBLNOFRM, 0, (char *) NULL);
		PCexit(-1);
	}

	return(FDputoper(oper, frm, fldno, FT_UPDATE, FT_TBLFLD,
		col->flhdr.fhseq, row));
}


/*{
** Name:	FDtblOK - Check row number for validity.
**
** Description:
**	Given a table field and a pseudo row number, convert the
**	pseudo row number to a real row number and check that the
**	converted row number is valid for the passed in table field.
**	Pseudo row numbers are:
**	- fdtfTOP - Row number of the TOP row.
**	- fdtfBOT - Row number of the BOTTOM row.
**	- fdtfCUR - Row number of the CURRENT row.
**	- Anything else will be simple be returned.
**	Conversion is done by FDchgtbrow().
**
** Inputs:
**	tf	Pointer to the table field to check on.
**	row	Pseudo row number.
**
** Outputs:
**	row	Converted row number.
**
**	Returns:
**		TRUE	If row number is valid.
**		FALSE	If table field pointer is NULL or the row number
**			is out of bounds
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
FDtblOK(tf, row)
TBLFLD	*tf;
i4	*row;
{
	FUNC_EXTERN	i4	FDchgtbrow();

	/* validate row number */
	if (tf == NULL)
		return (FALSE);
	*row = FDchgtbrow(tf, *row);

	if (*row < 0 || *row >= tf->tfrows)
		return (FALSE);

	return (TRUE);
}


/*{
** Name:	IIFDndcNoDeriveClr	- Indicate that FDclrrow should ignore
**					  derivations when clearing the row.
**
** Description:
**	Indicate that FDclrrow should ignore derivations when clearing the
**	row.  This is so that the caller of FDclrrow can do it's own
**	processing without FDclrrow interfering.
**
** Inputs:
**
** Outputs:
**	Returns:
**		VOID
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	24-aug-89 (bruceb)	Written.
*/
VOID
IIFDndcNoDeriveClr()
{
    no_derive_clear = TRUE;
}


/*{
** Name:	IIFDscoSetCROpt - Set Clear Row option
**
** Description:
**	Set option to let FDclrrow() handle edit masks differently
**	when clearing a row.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	03/14/93 (dkh) - Initial version.
*/
void
IIFDscoSetCROpt(opt)
bool	opt;
{
	fakerowopt = opt;
}
