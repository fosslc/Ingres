/*
**  FTmarkfield
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	04/20/90 (dkh) - Changed handling for table fields
**			 to fix us #595.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"


WINDOW	*FTfindwin();


VOID
FTmarkfield(frm, fieldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fieldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	FIELD	*fld;
	FLDHDR	*hdr;
	TBLFLD	*tbl;
	WINDOW	*win;

	if (disponly == FT_DISPONLY)
	{
		fld = frm->frnsfld[fieldno];
	}
	else
	{
		fld = frm->frfld[fieldno];
	}

	if (fld->fltag == FTABLE && col == -1)
	{
		tbl = fld->fld_var.fltblfld;
		hdr = &(tbl->tfhdr);
		win = TDsubwin(FTwin, hdr->fhmaxy, hdr->fhmaxx, hdr->fhposy, hdr->fhposx, FTutilwin);
	}
	else
	{
		win = FTfindwin(fld, FTwin, FTutilwin);
	}

	if (win == NULL)
	{
		return;
	}

	TDtouchwin(win);

}
