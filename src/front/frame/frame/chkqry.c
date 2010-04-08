/*
**	chkqry.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	chkqry.c - Check a form in query mode.
**
** Description:
**	File contains routines to check fields in a form
**	that is in QUERY mode.
**	Routines defined here are:
**	- FDchkqry - Check a form in query mode.
**
** History:
**	JEN  -  1  Nov 1981
**	NCG  -  12 JAN 1984  (Queried on all rows of table field)
**	02/15/87 (dkh) - Added header.
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
# include	"fdfuncs.h"

FUNC_EXTERN i4	FDqrydata();
i4		FDfldtrv();
FUNC_EXTERN	FRAME	*FDFTgetiofrm();




/*{
** Name:	FDchkqry - Check a form in query mode.
**
** Description:
**	Check a form that is query mode.  We just check for
**	proper query operators (if any) and do simple data
**	type checking.  Only updateable fields are checked.
**	Real work is done by traversing the fields and
**	letting FDqrydata() do the work.
**
** Inputs:
**	frm	Form to check on.
**
** Outputs:
**	Returns:
**		TRUE	If fields are OK.
**		FALSE	If bad query operator or data found for a field.
**	Exceptions:
**		None.
**
** Side Effects:
**	A field that fails to pass becomes the current field in the form.
**
** History:
**	JEN  -  1  Nov 1981
**	NCG  -  12 JAN 1984  (Queried on all rows of table field)
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDchkqry(frm)
FRAME	*frm;
{
	i4	fldno;
	FRAME	*ofrm;

	ofrm = FDFTgetiofrm();
	FDFTsetiofrm(frm);

	FDFTsmode(FT_UPDATE);

	for (fldno = 0; fldno < frm->frfldno; fldno++)
	{
		if(FDfldtrv(fldno, FDqrydata, FDDISPROW) != TRUE)
		{
			frm->frcurfld = fldno;
			FDFTsetiofrm(ofrm);
			return (FALSE);
		}
	}

	FDFTsetiofrm(ofrm);
	return (TRUE);
}
