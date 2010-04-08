
/*
**	iifsz.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	iifsz.c - get size of field display area.
**
** Description:
**	field display area
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
# include	<er.h>

FUNC_EXTERN FLDHDR *FDgethdr();
FUNC_EXTERN FLDVAL *FDgetval();
FUNC_EXTERN FLDTYPE *FDgettype();

/*{
** Name:	IIFRfsz - get size of field display area.
**
** Description:
**	utility used by inquire_frs for size of field display area.
**
** Inputs:
**	fld	field structure.
**
** Outputs:
**	srow, scol, nrow, ncol	starting rows & columns, number of
**				rows & columns
** History:
**	05/88 (bobm) written (snitched from FTfindwin, actually).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

IIFRfsz(fld, srow, scol, nrow, ncol)
FIELD *fld;
i4  *srow, *scol;
i4  *nrow, *ncol;
{
	FLDHDR *hdr;
	FLDVAL *val;
	FLDTYPE *type;

	if (fld->fltag == FTABLE)
	{
		hdr = &(fld->fld_var.fltblfld->tfhdr);
	}
	else
	{
		hdr = FDgethdr(fld);
	}

	val = FDgetval(fld);
	type = FDgettype(fld);

	*srow = hdr->fhposy + val->fvdatay;
	*scol = hdr->fhposx + type->ftdatax;

	*ncol = type->ftdataln;

	/* *ncol = 0 really shouldn't happen */
	*nrow = *ncol != 0 ? (type->ftwidth + *ncol - 1) / *ncol : 0;
}
