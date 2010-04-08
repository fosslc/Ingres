/*
**	getcol.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	getcol.c - Get value for a cell in a table field.
**
** Description:
**	Gets a value for a cell in a table field.
**	Routine defined here is:
**	- FDgetcol - Get value for a cell in a table field.
**
** History:
**	02/15/87 (dkh) - Added header.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


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
# include	<frserrno.h>
# include       <er.h>
# include	"fdfuncs.h"



FUNC_EXTERN	FLDCOL	*FDfndcol();


/*{
** Name:	FDgetcol - Get value for a cell in a table field.
**
** Description:
**	Get the data for a cell in a table fiela
**	The exact row depends on the state of the table field.
**	If it is scrolling then it is toprow or botrow, depending on
**	the direction.
**
**	Note that to call FDgetdata() which calls v_evalfld() one must
**	assign the current row to the row being validated - this is because
**	v_evaltree() uses only the current row.
**	(It would be nice if it passed a row number to it?)
**
**
** Inputs:
**	Validate	Get latest data from display buffer.
**	tf		Table field cell is in.
**	row		Pseudo row number for cell.
**	colname		Name of column for cell.
**
** Outputs:
**	pdbv	Pointer to field's DB_DATA_VALUE.
**
**	Returns:
**		TRUE	If data was successfully obtained.
**		FALSE	Could not get cell value.
**	Exceptions:
**		None.
**
** Side Effects:
**	Field value is updated as well.
**
** History:
**	19-jan-1984	Modified (ncg)
**	30-apr-1984	Improved interface to routines for performance (ncg)
**	23-nov-1985	Fix for BUG 6026. (dkh)
**	07-may-1986	Call to FDstorput passes (char *data) because
**			FDstorput now expects it.
**	05-jan-1987 (peter)	Change RTfnd* to FDfnd.
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDgetcol(validate, tf, row, colname, pdbv)
i4		validate;
register TBLFLD	*tf;
i4		row;
char		*colname;
DB_DATA_VALUE	**pdbv;
{
	register FLDCOL	*col;
    	FLDVAL		*val;
	i4		oldrow;		/* for call to FDgetdata */
	i4		fldno;
	FRAME		*frm;

	if (!FDtblOK(tf, &row))
		return (FALSE);

	if ((col = FDfndcol(tf, colname)) == NULL)
	{
		/*
		**  Fix for BUG 6026. (dkh)
		*/
		IIFDerror(TBBADCOL, 2, (&(tf->tfhdr))->fhdname, colname);

		return (FALSE);
	}

	FDftbl(tf, &frm, &fldno);
	if (frm == NULL)
	{
		IIFDerror(TBLNOFRM, 0, (char *) NULL);
		PCexit (-1);
	}


	val = tf->tfwins +(row * tf->tfcols) + col->flhdr.fhseq;
	if (validate)	/* check data before returning */
	{
		if (tf->tfhdr.fhdflags & fdtfQUERY)
		{
			/* query the data from frame and verify */

			if (!FDqrydata(frm, fldno, FT_UPDATE,
					FT_TBLFLD, col->flhdr.fhseq, row))
				return (FALSE);
		}
		else
		{
			/*
			** Get the data from the frame and verify.  Save
			** current row for later, and assign new for following
			** v_evaltree.	On all accounts return current row.
			*/
			oldrow = tf->tfcurrow;
			tf->tfcurrow = row;

			if (!FDgetdata(frm, fldno, FT_UPDATE,
				FT_TBLFLD, col->flhdr.fhseq, row))
			{
				tf->tfcurrow = oldrow;
				return (FALSE);
			}
			tf->tfcurrow = oldrow;
		}
	}

	*pdbv = val->fvdbv;
	return(TRUE);
}
