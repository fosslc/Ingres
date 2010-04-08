/*
**  widefrm.c
**
**  Low level function to widen a form.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 10/02/85 (dkh)
**	04/16/87 (dkh) -Integrated rpm, unix, cms changes.
**	08/14/87 (dkh) - ER changes.
**	06/29/88 (tom) - eliminated roundup algorithm because 
**			 popup forms do much better when expansions are
**			 minimized, and it is a fairly non essential
**			 optimization to expand larger than currently
**			 needed just to (possibly) avoid a future
**			 window size expansion.
**      06/12/90 (esd) - Make the member frmaxx in FRAME during vifred
**                       include room for an attribute byte to the left
**                       of the end marker, as well as the end marker.
**                       (We won't always insert this attribute
**                       before the end marker, but we always leave
**                       room for it).
**	06/29/92 (dkh) - Added support for rulers.
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


VOID
vfwider(frm, numcols)
FRAME	*frm;
i4	numcols;
{
	i4	size;

	/* see if we must expand the window via a vtlayer call..
	   Note that during vifred operation the frmaxx struct member
	   contains the sizeof the edit window and not the size of 
	   the form which will possibly be smaller than the window size. */
	if (endxFrm + numcols + 3 >= frm->frmaxx)
	{
		VTwider(frm, numcols);
		frm->frmaxx += numcols;
	}
	endxFrm += numcols;

	/*
	**  Make the horizontal cross hair longer if it is displayed.
	*/
	if (IIVFxh_disp)
	{
		vfersPos(IIVFhcross);
		unLinkPos(IIVFhcross);
		IIVFhcross->ps_endx += numcols;
		insPos(IIVFhcross, FALSE);
	}

	vfChkPopup();
}
