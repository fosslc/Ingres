/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**	vfgetdata.c
**
**	contains:
**		vfgetDataWin()
**
**	history:
**		1/28/85 (peterk) - split off from frame.c
**		03/13/87 (dkh) - Added support for ADTs.
**		19-sep-89 (bruceb)
**			Call vfgetSize() with a precision argument.  For
**			decimal support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"

/*
** get the beginning and ending coordinates of the fields data window
*/
VOID
vfgetDataWin(fd, ps)
REGFLD	*fd;
reg POS	*ps;
{
	i4	extent;
	reg FLDTYPE	*type;
	reg FLDVAL	*val;
	FLDHDR		*hdr;

	hdr = &fd->flhdr;
	type = &fd->fltype;
	val = &fd->flval;
	ps->ps_begx = hdr->fhposx + type->ftdatax;
	ps->ps_begy = hdr->fhposy + val->fvdatay;
	vfgetSize(type->ftfmt, type->ftdatatype, type->ftlength, type->ftprec,
		&(ps->ps_endy), &(ps->ps_endx), &extent);
	ps->ps_endx = ps->ps_begx + ps->ps_endx - 1;
	ps->ps_endy = ps->ps_begy + ps->ps_endy - 1;
}

