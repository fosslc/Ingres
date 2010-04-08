/*
**  VTbox.c
**
**  Copyright (c) 2004 Ingres Corporation
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"vtframe.h"

/*{
** Name:	VTdispbox	- Display a box on a frame window.
**
** Description:
**	Called to display box features on the form's screen.
**	Currently this routine is called by 
**		ft 	- to display box features.
**		vifred 	- to display box features and boxed fields.
**	
**
** Inputs:
**	FRAME	*frm;		- frame struct to use
**	i4	starty, startx;	- start line and column
**	i4	nlines, ncols;	- number of lines and columns
**	i4	attr;		- base form system attribute of box 
**	bool	hilight;	- flag to say if it is to be distinguished
**	bool	restrict;	- flag to say if we are to restrict box
**				  character encroachment.
**	bool	dummy;		- used only in FT3270 version.
**
** Outputs:
**	Returns:
**		none
**
**	Exceptions:
**
** Side Effects:
**		the box is drawn on the frame screen image.. 
**		the window structure flag bit is posted to say that
**		it is necessary to fix up box characters we are painting.
**
** History:
**	05/19/88  (tom)	- taken out of vt for box feature addition
**	04-sep-90 (bruceb)
**		Added dummy param to VTdispbox; used only in FT3270 version.
**	18-apr-01 (hayke02)
**		Required change to avoid conflict with reserved word restrict.
**		Identified after latest compiler OSFCMPLRS440 installed on
**		axp.osf which includes support for this based on the C9X
**		review draft.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/
VOID
VTdispbox(frm, starty, startx, nlines, ncols, attr, hilight, iirestrict, dummy)
FRAME	*frm;
i4	starty;
i4	startx;
i4	nlines;
i4	ncols;
i4	attr;
bool	hilight;
bool	iirestrict;
bool	dummy;
{
	i4	regattr;
	i4	hiattr;

	i4 VTdistinguish();

	hiattr = regattr = FTattrxlate(attr);

	if (hilight)
	{
		/* if there is no reverse video or blinking then we 
		   just paint the corners with $'s */
		if (RV == NULL || BL == NULL)
		{
			TDsbox(frm->frscr, starty, nlines, startx, ncols, 
			       '|', '-', '$', regattr, regattr, iirestrict);
			return;
		}
		else	/* otherwise modify attribute to make it standout */
		{
			hiattr = FTattrxlate(VTdistinguish(attr));
		}
	}

	TDsbox(frm->frscr, starty, nlines, startx, ncols, 
		'|', '-', '+', regattr, hiattr, iirestrict);
}

/*{
** Name:	VTdistinguish - return modified attribute 
**
** Description:
**	Return modified form system attribute so that it will standout.
**
** Inputs:
**	i4 attr;	- form system attribute to modify
**
** Outputs:
**
**	Returns:
**		i4	- the modified attribute
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
*/

i4
VTdistinguish(attributes)
i4	attributes;
{
	return ((attributes ^fdRVVID) | fdBLINK);
}


