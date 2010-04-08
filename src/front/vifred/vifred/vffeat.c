/*
**  vffeat.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	02/04/85 (drh)	Added return after vfseqnext call - integration of
**			VMS bug fixes.
**	24-apr-89 (bruceb)
**		Zapped nonseq field code; no longer nonseq fields.
**      06/09/90 (esd) - Check IIVFsfaSpaceForAttr instead of
**                       calling FTspace, so that whether or not
**                       to leave room for attribute bytes can be
**                       controlled on a form-by-form basis.
**	22-jun-89 (bruceb)
**		The 'next' and 'previous' commands when in VF_SEQ mode
**		skip derived fields.
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
# include	 <vt.h> 

FUNC_EXTERN	POS	*nextFeat();
FUNC_EXTERN	POS	*prevFeat();

static	i4	featmode = VF_NORM;

VFfeatmode(mode)
i4	mode;
{
	featmode = mode;
}

VFfeature(cury, curx, mode, newy, newx)
i4	cury;
i4	curx;
i4	mode;
i4	*newy;
i4	*newx;
{
	POS	*ps;

	globy = cury;
	globx = curx;

	if (featmode == VF_SEQ && (mode == VF_NEXT || mode == VF_PREV))
	{
		vfseqnext(mode, newy, newx);
		return;
	}
	if (mode == VF_NEXT)
	{
		ps = nextFeat();
	}
	else if (mode == VF_PREV)
	{
		ps = prevFeat();
	}
	else if (mode == VF_BEGFLD)
	{
		ps = onPos(globy, globx);
		if (ps != NULL)
		{
			*newx = ps->ps_begx;
		}
		else
		{
			*newx = 0;
		}
	}
	else if (mode == VF_ENDFLD)
	{
		ps = onPos(globy, globx);
		if (ps != NULL)
		{
			*newx = ps->ps_endx;
		}
		else
		{
			*newx = endxFrm - IIVFsfaSpaceForAttr;
		}
	}
	else
	{
		*newy = cury;
		*newx = curx;
		return;
	}
	if (mode == VF_NEXT || mode == VF_PREV)
	{
		*newy = ps->ps_begy;
		*newx = ps->ps_begx;
	}
	else
	{
		*newy = cury;
	}
	globy = *newy;
	globx = *newx;
}

VOID
vfseqnext(mode, newy, newx)
i4	mode;
i4	*newy;
i4	*newx;
{
	reg	FLDHDR	*hdr = NULL;
	FIELD	*fld;
	POS	*curps;
	POS	*(*dir)();

	if (mode == VF_NEXT)
	{
		dir = nextFeat;
	}
	else
	{
		dir = prevFeat;
	}
	for (;;)
	{
		curps = (*dir)();
		globx = curps->ps_begx;
		globy = curps->ps_begy;
		switch(curps->ps_name)
		{
			case PFIELD:
				hdr = FDgethdr(curps->ps_feat);
				if (hdr->fhd2flags & fdDERIVED)
					hdr = NULL;	/* Skip the field. */
				break;

			case PTABLE:
				fld = (FIELD *) curps->ps_feat;
				hdr = &(fld->fld_var.fltblfld->tfhdr);
				break;

			default:
				break;
		}
		if (hdr)
		{
			*newy = globy;
			*newx = globx;
			return;
		}
	}
}
