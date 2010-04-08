# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<multi.h>
# include	<ex.h>
# include	<me.h>
# include	<er.h>
# include	<erfi.h>
# include	<scroll.h>

/*
**  fdvalid.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	9/21/84 -
**		Stolen from getdata. (dkh)
**	19-jun-87  (bruceb) Code cleanup.
**	06/19/87 (dkh) - Fixed mandatory and nullable field behavior.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	09/11/87 (dkh) - Added code to delete EX handler when exiting.
**	10/12/87 (dkh) - Fixed jup bug 434.
**	10/14/87 (dkh) - Made error messages trappable.
**	10/29/87 (dkh) - Fixed so that clearing out a nullable field
**			 will result in a NULL value for the field.
**	09-nov-87 (bruceb)
**		For scrolling fields, create a temporary format
**		with the same data type as before, but with a single
**		row of width equivalent to the entire scrolling buffer.
**		This is used when transferring data between the
**		display and internal buffers to avoid truncation of
**		the data to the size of the field's visible window.
**	12/23/87 (dkh) - Performance changes.
**	09-feb-88 (bruceb)
**		fdSCRLFD now in fhd2flags; can't use most significant
**		bit of fhdflags.
**	07-apr-88 (bruceb)
**		Changed from using sizeof(DB_TEXT_STRING)-1 to using
**		DB_CNTSIZE.  Previous calculation is in error.
**	15-apr-1988 (danielt)
**		Added Matching EXdelete to EXdeclare.
**	29-apr-88 (bruceb)
**		After a failed fmt_cvt(), check against ABS of the datatype.
**	16-may-88 (bruceb)
**		Only update the field flags (fdI_CHG and fdVALCHKED) if
**		FDdatafmt succeeds.
**	06/18/88 (dkh) - Integrated CMS changes.
**	10/13/88 (dkh) - Fixed jup bug 2712.
**	13-oct-88 (sylviap)
**		Added TITAN changes.  DB_MAXSTRING -> DB_GW4_MAXSTRING.
**	11/01/88 (dkh) - Performance changes.
**	12/01/88 (dkh) - Fixed cross field validation problem.
**	19-jun-89 (bruceb)
**		Abstracted contents of FDvalidate into IIFDvfValidateFld().
**		Added new parameter that indicates if some error occurred
**		inside of IIFDsdv().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/


FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDVAL	*FDgetval();
FUNC_EXTERN	FLDTYPE *FDgettype();
FUNC_EXTERN	STATUS	FDflterr();
FUNC_EXTERN	i4	FDdatafmt();
FUNC_EXTERN	i4	FDqrydata();
FUNC_EXTERN	EX	FDadfxhdlr();
FUNC_EXTERN	VOID	IIFDssfSetScrollFmt();
FUNC_EXTERN	STATUS	IIFDsdvSetDeriveVal();
FUNC_EXTERN	i4	(*IIseterr())();
FUNC_EXTERN	i4	IIFDdecDerErrCatcher();


GLOBALREF	PTR	IIFDcdCvtData;


i4
FDvalidate(frm, fieldno, fldtype, col, row, error)
FRAME	*frm;
i4	fieldno;
i4	fldtype;
i4	col;
i4	row;
bool	*error;
{
    FIELD	*fld;
    TBLFLD	*tbl;
    i4		retval = TRUE;
    i4		(*oldproc)();
    FLDHDR	*hdr;
    i4		ocol;
    i4		orow;


    fld = frm->frfld[fieldno];
    *error = FALSE;

    if (fldtype == FT_TBLFLD)
    {
	tbl = fld->fld_var.fltblfld;
	if (tbl->tfhdr.fhdflags & fdtfQUERY)
	{
	    return(FDqrydata(frm, fieldno, FT_UPDATE, fldtype, col, row));
	}
	ocol = tbl->tfcurcol;
	orow = tbl->tfcurrow;
	tbl->tfcurcol = col;
	tbl->tfcurrow = row;
	hdr = FDgethdr(fld);
	tbl->tfcurcol = ocol;
	tbl->tfcurrow = orow;
	frm->frres2->rownum = row;
    }
    else
    {
	col = BADCOLNO;
	hdr = FDgethdr(fld);
    }

    retval = IIFDvfValidateFld(frm, fieldno, FT_UPDATE, fldtype,
	col, row, (bool)TRUE);

    if (hdr->fhdrv)
    {
	oldproc = IIseterr(IIFDdecDerErrCatcher);

	if (IIFDsdvSetDeriveVal(frm, fld, col) == FAIL)
	    *error = TRUE;

	_VOID_ IIseterr(oldproc);
    }

    return(retval);
}
