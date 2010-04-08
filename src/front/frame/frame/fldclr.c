/*
**	fldclr.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	fldclr.c - Clear out a single field.
**
** Description:
**	This routine contains a routine to clear a single
**	field.  The routine is:
**	- FDfldclr - Clear a field.
**
** History:
**	JRC  - 20 AUG 1982 (written)
**	02/15/87 (dkh) - Added header.
**	09/01/87 (dkh) - Added change bit for datasets.
**	09/16/87 (dkh) - Integrated table field change bit.
**	16-jun-89 (bruceb)
**		Entire revamp of FDfldclr(), both of the interface
**		and of the contents.  For derived fields.
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
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<erfi.h>
# include       <er.h>

FUNC_EXTERN	i4	FDclr();
FUNC_EXTERN	STATUS	IIFDsdvSetDeriveVal();


/*{
** Name:	FDfldclr - Clear a single simple field.
**
** Description:
**	Given a field number, clear out the display buffer
**	for the field.  This routine is only called for simple fields.
**
** Inputs:
**	frm		Frame containing the field to clear.
**	fldchain	Field is updatable or non-sequenced.
**	fldno		Field number of field to clear.
**
** Outputs:
**
**	Returns:
**		None.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/20/82 (jen) - Initial version.
**	02/15/87 (dkh) - Added procedure header.
**	02/05/88 (dkh) - Fixed jup bug 1965.
**	16-jun-89 (bruceb)	Total rewrite.
*/
VOID
FDfldclr(frm, fldchain, fldno)
FRAME	*frm;
i4	fldchain;
i4	fldno;
{
    FLDHDR	*hdr;

    /*
    ** Don't clear out derived fields.  Generate error message and return.
    */
    if (fldchain == FT_UPDATE)
    {
	hdr = &(frm->frfld[fldno]->fld_var.flregfld->flhdr);
	if (hdr->fhd2flags & fdDERIVED)
	{
	    IIFDerror(E_FI2267_8807_SetDerived, 0, (char *) NULL);
	    return;
	}
    }
    FDclr(frm, fldno, fldchain, FT_REGFLD, (i4)0, (i4)0);
    if ((fldchain == FT_UPDATE) && hdr->fhdrv && hdr->fhdrv->deplist)
    {
	/*
	** Invalidate any dependents (since FDclr turned off fdVALCHKED
	** and so all dependents will believe the field invalid).
	*/
	_VOID_ IIFDsdvSetDeriveVal(frm, frm->frfld[fldno], BADCOLNO);
    }
}
