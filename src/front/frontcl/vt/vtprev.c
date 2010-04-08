
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"vtframe.h"
# include	<er.h>

/**
** Name:	vtpreview.c - form preveiw functions	
**
** Description:
**		This file defines functions used by vifred to preview
**		a form on the screen.
**
** This file defines:
**
**	VTFormPreview	- 	Display a popup form in preview mode 
**	VTUpdPreview   -	Upate preview form with hilighted corners
**	VTKillPreview   -	Kill the window associated with a form preview
**
** History:
**	05/26/88 (tom) - written for vifred popup form support
**      06/12/90 (esd) Enhanced VTFormPreview to insert attributes
**                     if and only if appropriate for current form
**	04-sep-90 (bruceb)
**		Added param to call on VTdispbox()--for use by FT3270 only.
**	04/23/92 (dkh) - Changed VTUpdPreview() to be a VOID since it
**			 does not return any values.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Mar-2005 (lakvi01)
**          Corrected function prototypes.
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */


/*{
** Name:	VTFormPreview - preview a form in a screen sized window	
**
** Description:
** 		Preview a form in a screen sized window. This supports
**		vifred's form attributte "VisuallyAdjust" menuitem
**
** Inputs:
**	newfrm	- new frame structure which is to contain the window
**		  we are creating.  The following structure elements
**		  are also relevent:
**			frposy, frposx  -  position of the popup
**			frmaxy, frmaxx  -  size of the popup
**			frmflags .. fdBOXFR  bit - says if we box it
**	edtfrm  - Vifred's main edit frame structure which contains
**		  the window ptr which contians the screen image of 
**		  the popup form (this image is written over the 
**		  screen sized window that we create here).
**		  
**
** Outputs:
**	<param name>	<output value description>
**
**	Returns:
**		bool	TRUE  - all went well
**			FALSE - an error occured and an error message 
**				was displayed.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	05/26/88 (tom) - written
*/
bool
VTFormPreview(newfrm, edtfrm)
FRAME	*newfrm;
FRAME	*edtfrm;
{
reg	WINDOW	*win;
reg	WINDOW	*swin;
reg	i4	i;
reg	i4	sy;
reg	i4	sx;
reg	i4	nlines;
reg	i4	ncols;
	bool	attr_space = (edtfrm->frmflags & fdFRMSTRTCHD) != 0;

	/* create a screen sized window (minus the menu line) so that when 
	   we pass the new frame to vtcursor it is limited to just the 
	   screen area, and won't scroll */
	if ( (win = TDnewwin(LINES - 1, 0, 0, 0)) == (WINDOW*) NULL)
		return (FALSE);

	newfrm->frscr = (FTINTRN*)win;

	swin = (WINDOW*) edtfrm->frscr;

	sy = newfrm->frposy - 1; 
	sx = newfrm->frposx - 1;
	nlines = newfrm->frmaxy; 
	ncols = newfrm->frmaxx; 


	if (newfrm->frmflags & fdBOXFR)
	{
	    VTdispbox(newfrm, sy, sx, nlines, ncols, 0, FALSE, FALSE, FALSE);

	    /* adjust to paint the form inside border */
	    sy++;
	    sx += 1 + attr_space;
	    nlines -= 2;
	    ncols -= 2 + 2 * attr_space;
	}

	/* set flag to say that box characters must be fixed up. */
	win->_flags |= _BOXFIX;

	/* overwrite our new window with the contents of the edit window */
	for (i = 0; i < nlines; i++)
	{
		MEcopy(swin->_y[i], ncols, &win->_y[sy + i][sx]);
		MEcopy(swin->_da[i], ncols, &win->_da[sy + i][sx]);
		MEcopy(swin->_dx[i], ncols, &win->_dx[sy + i][sx]);
	}
	TDtouchwin(win);
	TDrefresh(win);
	return (TRUE);
}
/*{
** Name:	VTUpdPreview - update preview form
**
** Description:
** 		Update the preview form with hilighting as specifed.
**		This function is used to turn on and off the hilighted
**		corners of the form as we process a VisuallAdjust 
**		sequence.
**
** Inputs:
**	FRAME *prevfrm	- The preview form.. Which was setup via a call to 
**			  VTFormPreview.
**	bool hilite;	- hilight flag
**
** Outputs:
**	Returns:
**		none
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/21/88 (tom) - written for vifred popup form support
*/
VOID
VTUpdPreview(frm, hilite)
FRAME	*frm;
bool	hilite;
{
	if (hilite)
	{
		VTCornerHilite(frm->frscr, 
			frm->frposy - 1, frm->frposx - 1, 
			frm->frmaxy, frm->frmaxx,  hilite);
	}
	TDtouchwin(frm->frscr);
	TDrefresh(frm->frscr);
}

/*{
** Name:	VTCornerHilite	- Highlight the corners of an area of a window
**
** Description:
**		Highlight the corners of an area of a window
** 
** Inputs: 
**	WINDOW	*win;	- ptr to window 
**	i4 sy, sx;	- start y and x coord
**	i4 sy, sx;	- start y and x coord
**	bool	hilite;	- flag to say if we hilight or not
**
** Outputs:
**
**	Returns:
**		note
**
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**
** History:
**	<manual entries>
*/
VOID
VTCornerHilite(win, sy, sx, ny, nx, hilite)
WINDOW	*win;
register i4	sy;
register i4	sx;
register i4	ny;
register i4	nx;
bool	hilite;		/* currently ignored */
{
	char	*p[4];
	i4	i;
	i4	attr;
	i4	VTdistinguish();
	
	p[0] = win->_da[sy] + sx;
	p[1] = win->_da[sy + ny - 1] + sx;
	p[2] = win->_da[sy] + sx + nx - 1;
	p[3] = win->_da[sy + ny - 1] + sx + nx - 1;

	attr = FTattrxlate(VTdistinguish(0L));

	for (i = 0; i < 4; i++ )
	{
		*p[i] = (*p[i] & ~_DAMASK) | attr;
	}
}

/*{
** Name:	VTKillPreview	- kill the preview window 
**
** Description:
**		call TDdelwin with the preview window as an argument
**
** Inputs:
**	frame	- frame containing the window to get rid of 
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	05/26/88 (tom) - written
*/
VOID
VTKillPreview(frm)
FRAME	*frm;
{

	TDdelwin((WINDOW*) frm->frscr);
}
