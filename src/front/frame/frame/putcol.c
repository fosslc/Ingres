# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
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
# include	"fdfuncs.h"
# include       <er.h>


/*
**	putcol.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	putcol.c - Put data to a cell in a table field.
**
** Description:
**	Put passed in data to a particular cell (row x column) in
**	a table field.
**	Routines defined:
**	- FDputcol - Put data to a cell in a table field.
**
** History:
**	30-apr-1984	Improved interface to routines for performance (ncg)
**	24-sep-1984     Added the putoper() option. (ncg)
**	23-nov-1985	Fix for BUG 6026. (dkh)
**	05-jan-1987 (peter)	Change RTfnd* to FDfnd*.
**	02/17/87 (dkh) - Added header.
**	03/06/87 (dkh) - Added support for ADTs.
**      09/26/86 (a.dea) -- CMS WSC compiler doesn't deal well
**                     passing a structure. Change to addr.
**	25-jun-87 (bab)	Code cleanup.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	12/13/87 (dkh) - Performance changes.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

FUNC_EXTERN	FLDCOL	*FDfndcol();


/*
** FDputcol
** put the data in the column col of datarow
*/

/*{
** Name:	FDputcol - Put data to a cell in a table field.
**
** Description:
**	Put data into a specific row/column location is the
**	display of a table field.  Changed to use DB_DATA_VALUES
**	for 6.0.
**
** Inputs:
**	tf		Table field where data will go.
**	row		Row number of desired location, may be a pseudo
**			row number.
**	colname		Column name of desired location.
**	dbv		DB_DATA_VALUE containing new value.
**
** Outputs:
**	Returns:
**		TRUE	If update was successful.
**		FALSE	If data could not be placed at desired
**			location.  Reasons include a badly
**			specified location or data type mismatch.
**	Exceptions:
**		None.
**
** Side Effects:
**	Display buffer will be updated with the textual representation
**	of the data.  Also, if the table field is in the currently
**	displayed form, the data may appear on the screen as well.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
**	03/06/87 (dkh) - Added support for ADTs.
*/

i4
FDputcol(tf, row, colname, dbv)
TBLFLD		*tf;
i4		row;
char		*colname;
DB_DATA_VALUE	*dbv;
{
	FLDCOL	*col;
    	FLDVAL	*val;
	i4	fldno;
	FRAME	*frm;

	if (!FDtblOK(tf, &row))
		return (FALSE);

	FDftbl(tf, &frm, &fldno);
	if (frm == NULL)
	{
		IIFDerror(TBLNOFRM, 0, (char *) NULL);
		PCexit(-1);
	}


	if ((col = FDfndcol(tf, colname)) == NULL)
	{
		/*
		**  Fix for BUG 6026. (dkh)
		*/
		IIFDerror(TBBADCOL, 2, (&(tf->tfhdr))->fhdname, colname);

		return (FALSE);
	}

	tf->tflastrow = max(row, tf->tflastrow);
    	val = tf->tfwins +(row * tf->tfcols) + col->flhdr.fhseq;

	/*
	**  Since this is called from TBACC, the type should match
	**  exactly.  Therefore, we just do a MEcopy.
	*/

	MEcopy(dbv->db_data, (u_i2) dbv->db_length, val->fvdbv->db_data);

	/*
	**  Format the new value.
	*/
	return(FDputdata(frm, fldno, FT_UPDATE, FT_TBLFLD,
		col->flhdr.fhseq, row));
}
