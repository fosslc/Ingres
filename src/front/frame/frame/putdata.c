/*
** Copyright (c) 2004 Ingres Corporation
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


/*
**	PUTDATA.c  -  Put Data into a Field
**	
**	Fdputdata() places the contents of the field storage area,
**	formats that data, converts it to a string and places it into
**	the field data window.
**	
**	This routine is used for placing data into the frame.
**	
**	Arguments:  fld - field to place data into
**	
**	History:  1 Nov 1981 - Written (jen)
**		  6 Jan 1982 - Added format for floating point (jen)
**		 18 Feb 1983 - Number defaults set to left justified (nml)
**		 20 Oct 1983 - Brought over fixes:
**				BUG #1414 fixed - char fields with rt just.
**				format specified are now rt justified. (nml)
**				NOTE - leading blanks are ignored if field is
**				right justified.
**			     - Increased bufr size to handle text - using 
**			       DB_MAXSTRING defined in ingconst.h (nml)
**		19-jun-87 (bab)	Code cleanup.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDTYPE	*FDgettype();
FUNC_EXTERN	FLDVAL	*FDgetval();
FUNC_EXTERN	FIELD	*FDfldofmd();
FUNC_EXTERN	i4	FDdatafmt();

i4
FDputdata(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDHDR	*hdr;
	FLDTYPE	*type;
	FLDVAL	*val;
	i4	ocol;
	i4	orow;

	fld = FDfldofmd(frm, fldno, disponly);

	if (fldtype == FT_TBLFLD)
	{
		tbl = fld->fld_var.fltblfld;
		ocol = tbl->tfcurcol;
		orow = tbl->tfcurrow;
		tbl->tfcurcol = col;
		tbl->tfcurrow = row;
	}

	hdr = FDgethdr(fld);
	type = FDgettype(fld);
	val = FDgetval(fld);

	if (fldtype == FT_TBLFLD)
	{
		tbl->tfcurcol = ocol;
		tbl->tfcurrow = orow;
	}

	return(FDdatafmt(frm, fldno, disponly, fldtype, col, row, hdr, type,
		val));
}
