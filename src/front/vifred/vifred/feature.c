/*
**  feature.c
**  	contains routines which deal with position
**  	structures
**
**  Copyright (c) 2004 Ingres Corporation
**
**  history:
**	2/1/85 (peterk) - nextFeat, prevFeat split off into feature2.c.
**	08/14/87 (dkh) - ER changes.
**	07-sep-89 (bruceb)
**		Call VTputstring with trim attributes now, instead of 0.
**      06/12/90 (esd) - Add extra parm to VTputstring to indicate
**                       whether form reserves space for attribute bytes
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
# include	<er.h>
# include	"ervf.h"

/*
** hi light the passed position
*/

vfhiLite(ps)
POS	*ps;
{
	if (ps == NULL)
		return;
	VThilite(frame, ps->ps_begy, ps->ps_begx, ps->ps_endy, ps->ps_endx);
}


/*
** reallocate a feature and put it in the passed window
** the info in the ps is correct
** the win should be cleared before calling
*/

VOID
newFeature(ps)
register POS	*ps;
{
	switch (ps->ps_name)
	{
	case PSPECIAL:
	case PTRIM:
	{
		register TRIM	*tr;

		tr = (TRIM *) ps->ps_feat;
		tr->trmx = ps->ps_begx;
		tr->trmy = ps->ps_begy;
		VTputstring(frame, ps->ps_begy, ps->ps_begx,
			tr->trmstr, tr->trmflags, FALSE, IIVFsfaSpaceForAttr);
		break;
	}

	case PBOX:
		reDisplay(ps);
		break;
		
	case PFIELD:
	{
		SMLFLD	fd;

		FDToSmlfd(&fd, ps);
		moveFld(ps, &fd);
		break;
	}

# ifndef FORRBF
	case PTABLE:
		moveTab(ps);
		break;
# endif

	default:
		syserr(ERget(S_VF0069__rnewFeature__unknown),
			ps->ps_name);
		break;
	}
}

/*
** move the table field to the new position given by ps
*/

# ifndef FORRBF
moveTab(ps)
register POS	*ps;
{
	register TBLFLD *tab;
	register FLDHDR *hdr;
	FIELD		*fld;

	fld = (FIELD *) ps->ps_feat;
	if (fld->fltag != FTABLE)
		syserr(ERget(S_VF006A__r_nmoveTab__passed_n));
	tab = fld->fld_var.fltblfld;
	hdr = &tab->tfhdr;
	hdr->fhposx = ps->ps_begx;
	hdr->fhposy = ps->ps_begy;
	vfFlddisp(fld);
}
# endif

/*
** move a field to a new position
** information in ps is right
*/

VOID
moveFld(ps, fd)
register POS	*ps;
SMLFLD		*fd;
{
	register i4	x,
			y;
	FLDHDR		*hdr;

	hdr = FDgethdr((FIELD *) ps->ps_feat);
	x = ps->ps_begx - hdr->fhposx;
	y = ps->ps_begy - hdr->fhposy;
	fd->tx += x;
	fd->ty += y;
	fd->tex += x;
	fd->tey += y;
	fd->dx += x;
	fd->dy += y;
	fd->dex += x;
	fd->dey += y;
	moveComp(ps, fd);
}
