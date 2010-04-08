/*
**  FTfindwin.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "@(#)ftfindwin.c	1.1	1/23/85";
**
**  History:
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"



WINDOW *
FTfindwin(fld, parent, subw)
FIELD	*fld;
WINDOW	*parent;
WINDOW	*subw;
{
	FLDHDR	*hdr;
	FLDVAL	*val;
	FLDTYPE	*type;

	if (fld->fltag == FTABLE)
	{
		hdr = &(fld->fld_var.fltblfld->tfhdr);
	}
	else
	{
		hdr = (*FTgethdr)(fld);
	}

	val = (*FTgetval)(fld);
	type = (*FTgettype)(fld);

	return(TDsubwin(parent, (type->ftwidth + type->ftdataln - 1)/type->ftdataln,
		type->ftdataln, hdr->fhposy + val->fvdatay,
		hdr->fhposx + type->ftdatax, subw));
}
