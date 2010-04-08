/*
**	chkfrm.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	chkfrm.c - Check the entire form.
**
** Description:
**	File contains routines to check an entir form.  This is
**	the result of a "## validate" statement.
**	Routines defined here are:
**	- FDchkfrm - Check an entire form.
**
** History:
**	17-jan-1984	- Added Query mode for table fields, and
**			  speeded up code a bit. (ncg)
**	02/15/87 (dkh) - Added header.
**	25-jul-89 (bruceb)
**		Set 'in_validation' while validating.
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
# include	<frsctblk.h>
# include	"fdfuncs.h"

FUNC_EXTERN i4	FDgetdata();
FUNC_EXTERN i4	FDqrydata();
FUNC_EXTERN i4	FDnodata();
i4		FDfldtrv();
FUNC_EXTERN	FRAME	*FDFTgetiofrm();
FUNC_EXTERN	VOID	IIFDiaaInvalidAllAggs();

GLOBALREF	FRS_GLCB	*IIFDglcb;


/*{
** Name:	FDchkfrm - Check an entire form.
**
** Description:
**	Called when a " ## validate " is issued, and the form IS NOT in
**	Query mode.  Also called when " ## enddisplay " is issued in the
**	same mode.  If the form has a table field in Query mode,
**	then use FDqrydata on that field, because a table field can
**	be in query mode independently of the form.  Even DISPLAY
**	ONLY fields are checked here.
**
** Inputs:
**	frm	Form to check on.
**
** Outputs:
**	Returns:
**		TRUE	If all fields pass.
**		FALSE	If any field fails.
**	Exceptions:
**		None.
**
** Side Effects:
**	If a field fails the check, it becomes the current field.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDchkfrm(frm)
FRAME	*frm;
{
	register i4	fldno;
	register FIELD	**fld;
	TBLFLD		*tfld;
	i4		tbqry;
	FRAME		*ofrm;
	i4		(*routine)();

	IIFDglcb->in_validation |= VLD_FORM;

	ofrm = FDFTgetiofrm();
	FDFTsetiofrm(frm);

	FDFTsmode(FT_UPDATE);

	for (fldno = 0, fld = frm->frfld; fldno < frm->frfldno; fldno++, fld++)
	{
		if ((*fld)->fltag == FTABLE)
		{
			tfld = (*fld)->fld_var.fltblfld;

			/*
			**  If table field is READ ONLY then
			**  behave as if it's OK.  Part of fix
			**  for BUGS 5199 and 4531. (dkh)
			*/

			if (tfld->tfhdr.fhdflags & fdtfREADONLY)
			{
				continue;
			}

			tbqry = tfld->tfhdr.fhdflags & fdtfQUERY;
			/*
			**  Will call FDnodata if table field
			**  is in append mode and lastrow equals
			**  display row.  Fix for BUG 5199. (dkh)
			*/
			if (tbqry)
			{
				routine = FDqrydata;
			}
			else
			{
				IIFDiaaInvalidAllAggs(frm, tfld);
				if (tfld->tfhdr.fhdflags & fdtfAPPEND)
				{
					routine = FDnodata;
				}
				else
				{
					routine = FDgetdata;
				}
			}
			if (!FDtbltrv(tfld, routine, FDDISPROW))
			{
				frm->frcurfld = fldno;
				FDFTsetiofrm(ofrm);
				IIFDglcb->in_validation &= ~VLD_FORM;
				return(FALSE);
			}
		}
		else
		{
			if(!FDfldtrv(fldno, FDgetdata, FDDISPROW))
			{
				frm->frcurfld = fldno;
				FDFTsetiofrm(ofrm);
				IIFDglcb->in_validation &= ~VLD_FORM;
				return (FALSE);
			}
		}
	}

	/*
	** Sequence through display only fields to get their values
	** for appending rows.  Fixes bug #741  - nml.
	** No need to check for table fields here (ncg).
	*/

	FDFTsmode(FT_DISPONLY);

	for (fldno = 0; fldno < frm->frnsno; fldno++)
	{
		FDfldtrv(fldno, FDgetdata, FDDISPROW);
	}

	FDFTsetiofrm(ofrm);

	IIFDglcb->in_validation &= ~VLD_FORM;
	return (TRUE);
}
