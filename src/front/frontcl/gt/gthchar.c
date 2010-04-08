
/*
**    Copyright (c) 2004 Ingres Corporation
*/

/*# includes */
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<termdr.h>
# include	<graf.h>
# include	"gtdev.h"
# include	<gtdef.h>

/**
** Name:    gthchar - hardware character routines
**
** Description:
**      GT hardware character routines, used when putting alpha text on
**	the graphics area of the screen.
**
**	NOTE:
**		3 distinct types of terminal are recognized, as given by
**		the termcap description:
**
**		1 - graphics overstrikes text.  indiacted by G_ovstk.
**			on these devices, we can't do clear-to-end of line
**			or delete-char to remove text, because graphics
**			will be wiped out.  We overprint with spaces.
**
**		2 - graphics on a separate plane.  G_gplane.  On these
**			devices we can't wipe out with spaces, because this
**			will leave "holes" in the graphics on some devices
**			like envisions where the on screen characters always
**			obscure one character area, even spaces.  Clear to end
**			and delete sequences are OK.
**
**		3 - if neither flag is set, we don't put up any alpha in the
**			graphics area.
**
**	GTscrlim - GT screen limit
**	GTscrref - GT screen position reference
**	GThchclr - clear hardware text from the screen
**	GThchstr - return length of hardware text which fits in a box.
**	GTrsbox - handle corner marker characters for screen restriction.
**
** History:    $Log - for RCS$
**	10/19/89 (dkh) - Added VOID return type declaration for GTscrlim().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

extern char *G_tc_cm, *G_tc_ce, *G_tc_dc, *G_tc_bl, *G_tc_be, *G_tc_rv, *G_tc_re;
extern i4  G_cols,G_lines;

FUNC_EXTERN i4  GTchout();
FUNC_EXTERN char *TDtgoto();


extern GR_FRAME *G_frame;

/*{
** Name:	GTscrlim - GT screen limit indicator
**
** Description:
**	indiactes right screen limit for graphics edit area
**
** Inputs:
**	row - 	row to indicate, < 0 for all.
**
** Outputs:
**
** History:
**	1/86 (rlm)	written
**	3/89 (Mike S)	Add GTlock for DG.
**	10/19/89 (dkh) - Added VOID return type declaration.
*/
VOID
GTscrlim(row)
register i4  row;
{
	register i4  lim,col;
	extern   i4  G_homerow;
	/* forget it if we can't mix text and graphics */
	if (!G_govst && !G_gplane)
		return;

	col = G_frame->hcol + 1;
	if (col >= G_cols)
		return;

	if (row < 0)
	{
		row = G_homerow;
		lim = G_SROW;
	}
	else
		lim = row+1;

# ifdef DGC_AOS
	GTlock(TRUE, 0, 0);
# endif
	for ( ;  row < lim; ++row)
	{
		TDtputs(TDtgoto(G_tc_cm,col,row),1,GTchout);
		GTchout('|');
	}
# ifdef DGC_AOS
	GTlock(FALSE, G_homerow, 0);
# endif
}

/*{
** Name:	GTscrref - manage screen reference indicator
**
** Description:
**	puts an indicator on the screen at the current GT cursor position.
**	removes any old indicator that might have been on the screen.
**
** Inputs:
**	c - indicator, or '\0' for none.
**
** Outputs:
**
** History:
**	1/86 (rlm)	written
*/

static i4  Mrow = -1, Mcol;

GTscrref(c)
char c;
{
# ifdef DGC_AOS
	GTlock(TRUE, 0, 0);
# endif
	/*
	** remove old mark if there is one
	*/
	if (Mrow >= 0)
	{
		GThchclr(Mrow,Mcol,1);
		Mrow = -1;
	}

	/* only put up the mark if mixed text/graphics OK */
	if (c != '\0' && (G_govst || G_gplane))
	{
		Mrow = G_frame->crow;
		Mcol = G_frame->ccol;
		TDtputs(TDtgoto(G_tc_cm,Mcol,Mrow),1,GTchout);
		TDtputs(G_tc_bl,1,GTchout);
		TDtputs(G_tc_rv,1,GTchout);
		GTchout(c);
		TDtputs(G_tc_re,1,GTchout);
		TDtputs(G_tc_be,1,GTchout);
	}

# ifdef DGC_AOS
	GTcsunl();
# else
	GTcursync();
# endif
}

/*
**	GTrsbox - display / remove 4 corner markers indicating screen
**	restriction.  sets a local flag to allow multiple calls of
**	GTrsbox(FALSE) to be harmless - only the first FALSE following
**	a true will do anything.  It is important that this is only called
**	with G_frame->lim denoting the restricted area, ie. called AFTER
**	setting G_frame->lim, and before resetting it.  These marks are
**	done in reverse video, but NOT flashing.  This should be a GT
**	internal call only.
**
** Inputs:
**	on - TRUE to mark, FALSE to remove marks
**	mark - mark to use, irrelevent if on = FALSE
*/

static bool Corners = FALSE;	/* current state of GTrsbox */

VOID
GTrsbox(on,mark)
bool on;
char mark;
{
	i4 r_low,r_hi,c_low,c_hi;

	/* forget it if we can't mix text and graphics */
	if (!G_govst && !G_gplane)
		return;

	/* if "off" and already done, forget it */
	if (!on && !Corners)
		return;

# ifdef DGC_AOS
	GTlock(TRUE, 0, 0);
# endif
	/*
	** calculate screen locations from lim item
	** remember floating point y values REVERSED from rows
	*/
	GTxftoa ((G_frame->lim)->xlow,(G_frame->lim)->yhi,&r_low,&c_low);
	GTxftoa ((G_frame->lim)->xhi,(G_frame->lim)->ylow,&r_hi,&c_hi);

	if (Corners = on)
	{
		/*
		** I AM assuming "safe to move in reverse video" here
		*/
		TDtputs(G_tc_rv,1,GTchout);
		TDtputs(TDtgoto(G_tc_cm,c_low,r_low),1,GTchout);
		GTchout(mark);
		TDtputs(TDtgoto(G_tc_cm,c_low,r_hi),1,GTchout);
		GTchout(mark);
		TDtputs(TDtgoto(G_tc_cm,c_hi,r_low),1,GTchout);
		GTchout(mark);
		TDtputs(TDtgoto(G_tc_cm,c_hi,r_hi),1,GTchout);
		GTchout(mark);
		TDtputs(G_tc_re,1,GTchout);
	}
	else
	{
		/*
		** "OFF" operation.  We distinguish between separate and
		** single plane devices, and recover the screen limit
		** indicators if needed.
		*/
		GThchclr(r_low,c_low,1);
		GThchclr(r_low,c_hi,1);
		GThchclr(r_hi,c_low,1);
		GThchclr(r_hi,c_hi,1);
	}

# ifdef DGC_AOS
	GTcsunl();
# else
	GTcursync();
# endif
}

/*
**	GThchstr - return allowed length of hardware characters inside
**	graphics box.  return 0 for box too small for initial string or
**	character height, and -1 for terminal with bad alpha characteristics
**	On positive return passes back row and column to display string.
**
**	ifdef'ed out by TEXTONSCREEN because this routine is only used
**	by code ifdef'ed in with this symbol in GTtexted
*/

#ifdef TEXTONSCREEN
GThchstr (str,ext,row,col)
char *str;	/* initial string */
EXTENT *ext;
i4  *row,*col;
{
	i4 len;
	float bottom,top;

	if (!G_govst && !G_gplane)
		return (-1);

	len = (ext->xhi - ext->xlow)/G_frame->csx;
	len -= 2;
	if (len < STlength(str))
		return (0);

	/* put in middle left of box, then convert back to check height */
	GTxftoa (ext->xlow,(ext->ylow + ext->yhi)/2.0,row,col);
	++*col;
	GTxatof (*row,*col,&bottom,&top);
	top += G_frame->csy/2.0;
	bottom = top - G_frame->csy;
	if (top > ext->yhi || bottom < ext->ylow)
		return (0);

	return (len);
}
#endif
/*
**	remove hardware characters from graphics screen, either by
**	clear-to-end, delete char, or overprint with spaces, as appropriate
**	Assumptions: no characters exist to the right, we will redraw if
**	this is a graphics overstrike terminal, and we have obliterated
**	something significant.  We DO handle the screen limit indicator.
*/
GThchclr (row,col,len)
i4  row,col;	/* position */
i4  len;	/* characters to remove */
{
	char *TDtgoto();
	i4 lcol;
	extern i4  G_homerow;

# ifdef DGC_AOS
	GTlock(TRUE, 0, 0);
# endif
	if (G_gplane)
	{
		if (G_tc_ce != NULL && *G_tc_ce != '\0')
		{
			TDtputs(TDtgoto(G_tc_cm,col,row),1,GTchout);
			TDtputs(G_tc_ce,1,GTchout);
		}
		else
		{
			/*
			** if we can't clear to end, we have to remove
			** the limit indicator as well
			*/
			lcol = G_frame->hcol + 1;
			if (lcol < G_cols)
			{
				TDtputs(TDtgoto(G_tc_cm,lcol,row),1,GTchout);
				TDtputs(G_tc_dc,1,GTchout);
				TDtputs(TDtgoto(G_tc_cm,col,row),1,GTchout);
			}
			for ( ; len > 0; --len)
				TDtputs(G_tc_dc,1,GTchout);
		}
		/* recover limit indicator */
		GTscrlim(row);
	}
	else
	{
		if (G_govst)
		{
			TDtputs(TDtgoto(G_tc_cm,col,row),1,GTchout);
			for ( ; len > 0; --len)
				GTchout (' ');
		}

		/*
		**	else PUNT!! device has "bad alpha", and shouldn't
		**	have displayed hardware characters in the first place
		*/
	}
# ifdef DGC_AOS
	GTlock(FALSE, G_homerow, 0);
# endif
}
