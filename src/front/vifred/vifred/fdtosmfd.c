/*
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	Sccsid[] = "@(#)fdtosmlfd.c	30.1	1/28/85";
**
**  contains:
**	FDToSmlfd()
**
**  history:
**	1/28/85 (peterk) - split off from feature.c
**	08/14/87 (dkh) - ER changes.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<er.h>
# include	"ervf.h"
/*
** fill a small field with information from a position
*/

VOID
FDToSmlfd(fd, ps)
register SMLFLD *fd;
POS		*ps;
{
	register FIELD	*ft;
	FLDHDR		*hdr;
	POS		data;

	if (ps->ps_name != PFIELD)
		syserr(ERget(S_VF0068_FDToSmlfd__passed_non));
	ft = (FIELD *) ps->ps_feat;
	hdr = FDgethdr(ft);
	fd->tx = hdr->fhtitx + hdr->fhposx;
	fd->ty = hdr->fhtity + hdr->fhposy;
	fd->tex = fd->tx + STlength(hdr->fhtitle) - 1;
	fd->tey = fd->ty;
	fd->tstr = hdr->fhtitle;
	vfgetDataWin(ft->fld_var.flregfld, &data);
	fd->dx = data.ps_begx;
	fd->dy = data.ps_begy;
	fd->dex = data.ps_endx;
	fd->dey = data.ps_endy;
}

