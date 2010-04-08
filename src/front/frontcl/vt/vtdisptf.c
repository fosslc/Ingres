/*
**  VTdisptf.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History
**
**  6/22/85 - Added support for titleless table fields. (dkh)
**	08/14/87 (dkh) - ER changes.
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
# include	"vtframe.h"
# include	<er.h>

FUNC_EXTERN	WINDOW	*TDmakenew();


VTdisptf(frm, tab, hilight)
FRAME	*frm;
TBLFLD	*tab;
bool	hilight;
{
	reg	FLDHDR	*hdr;
	FLDCOL	**col;
	i4	i;
	i4	attr;
	WINDOW	*VTutilwin;
	WINDOW	*win;
	i4	VTdistinguish();

	win = frm->frscr;
	hdr = &tab->tfhdr;
	hdr->fhdflags |= fdBOXFLD;
	VTutilwin = TDmakenew(win->_maxy, win->_maxx, 0, 0);
	if (VTutilwin == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("VTdisptf"));
	}
	VTutilwin->_parent = win;
	VTutilwin->_flags |= _SUBWIN;
	VTutilwin->_flags &= ~(_FULLWIN | _SCROLLWIN);
	TDsubwin(win, hdr->fhmaxy, hdr->fhmaxx, hdr->fhposy,
		hdr->fhposx, VTutilwin);

	/* Clear the area inside the table field so that box characters
	   do not show through */
	VTClearArea(frm, 
		hdr->fhposy + 1, hdr->fhposx + 1,
		hdr->fhmaxy - 2, hdr->fhmaxx - 2);

	FTdisptf(VTutilwin, tab, hdr);

	if (!(hdr->fhdflags & (fdNOTFTOP | fdTFXBOT)))
	{
		if (hilight)
		{
			attr = FTattrxlate( VTdistinguish((i4)0) );
			TDcorners(VTutilwin, attr);
		}
	}

	for (i = 0, col = tab->tfflds; i < tab->tfcols; i++, col++)
	{
		FTdispcol(VTutilwin, tab, hdr, &((*col)->flhdr));
	}

	TDdelwin(VTutilwin);

	/* we have now painted the box characters into the 
	   parent's virtual screen buffer.. so we must post on the parent
	   window that the box characters need fixing up */
	win->_flags |= _BOXFIX;
}
