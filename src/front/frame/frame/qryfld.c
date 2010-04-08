# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	"fdfuncs.h"
# include	<si.h>
# include	<er.h>


/*
**	qryfld.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	qryfld.c - Get the query value for a field.
**
** Description:
**	File contains routine(s) to get the query value for a field.
**	Routines defined:
**	- FDqryfld - Get the qyer value for a field.
**
** History:
**	02/17/87 (dkh) - Added header.
**	08/14/87 (dkh) - ER changes.
**	09/16/87 (dkh) - Integrated table field change bit.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


FUNC_EXTERN	REGFLD	*FDqryfdop();



/*{
** Name:	FDqryfld - Get the query value for a field.
**
** Description:
**	Get the query value for a field.  The query value does not
**	include the query operator.
**
**	For 6.0, just pass back the pointer to the DB_DATA_VALUE
**	for the field.
**
** Inputs:
**	frm		Form containing field.
**	name		Name of regular field to get value on.
**	pdbv		DB_DATA_VALUE pointer to set.
**
** Outputs:
**	Returns:
**		TRUE	If query value was obtained.
**		FALSE	If field not found or data type mismatch.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	JEN - 02 Sep 1982 (written)
**	NCG - 25 Sep 1984 (added FDputoper)
**	02/17/87 (dkh) - Added procedure header.
**	03/07/87 (dkh) - Added support for ADTs.
*/
i4
FDqryfld(frm, name, pdbv)	/* FDQRYFLD: */
FRAME		*frm;		/* frame to extract data from */
char		*name;		/* name of field to be examined */
DB_DATA_VALUE	**pdbv;		/* union of legal C types */
{
	REGFLD	*fld;		/* temporary field pointer */
	FLDVAL	*val;

	if ((fld = FDqryfdop(frm, name)) == NULL)
	{
		return(FALSE);
	}

	val = &(fld->flval);
	*pdbv = val->fvdbv;
	return(TRUE);
}

/*
** FDputoper() - Insert the query operator in front of data in a specific
**		 field winfdow. Should only be called in Query mode.
*/

static char querymap [7][3] = { ERx(""),	/* fdNOP */
				ERx(""),	/* fdEQ */
				ERx("!="),	/* fdNE */
				ERx("<"),	/* fdLT */
				ERx(">"),	/* fdGT */
				ERx("<="),	/* fdLE */
				ERx(">=")	/* fdGE */ };

i4
FDputoper(oper, frm, fldno, disponly, fldtype, col, row)
i4	oper;
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	register char	*cp;

	if (oper < fdNOP || oper > fdGE)
		return (FALSE);

	if (oper == fdNOP)
	{
		/*
		**  It is assumed that FTclrfld() will take care
		**  of setting the count field of the long
		**  text structure to zero.
		*/
		FDclr(frm, fldno, disponly, fldtype, col, row);
		IIFDccb(frm, fldno, disponly, fldtype, col, row);
	}
	else
	{
		/*
		**  Have to use FTinsrtstr to insert query operator
		*/
		cp = querymap[oper];
		FTinsrtstr(frm, fldno, disponly, fldtype, col, row, cp);
	}
	return (TRUE);
}
