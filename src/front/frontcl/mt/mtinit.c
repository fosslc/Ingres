/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"mtloc.h"
# include	<termdr.h>
# include	<fstm.h>

GLOBALREF	WINDOW	*titlscr;
GLOBALREF	WINDOW	*dispscr;
GLOBALREF	WINDOW	*statscr;
GLOBALREF	WINDOW	*bordscr;
GLOBALREF	i4	(*MTputmufunc)();
GLOBALREF	VOID	(*MTdmpmsgfunc)();
GLOBALREF	bool	(*MTfsmorefunc)();
GLOBALREF	VOID	(*MTdmpcurfunc)();
GLOBALREF	VOID	(*MTdiagfunc)();

GLOBALREF	i4	IITDflu_firstlineused;

static		WINDOW	*scr_img = NULL;

/*{
**  FSinit  --  Initialize the windows for FSTM.
**
**  This routines allocates and initializes the windows
**  needed by the fullscreen browser routines.
**
**  Inputs:
**	None.
**
**  Returns:
**	None.
**
**  Outputs:
**	None.
**
**  Side Effects:
**	None.
**
**  History:
**	08/25/87 (scl) Put back some FT3270 #ifdefs
**	11/12/87 (dkh) - Parts of routine FSinit() moved to FT directory and
**			 renamed MTinit().
**	11/21/87 (dkh) - Eliminated any possible back references.
**	26-jan-89 (bruceb)
**		Offset the location and size of the output screen windows 
**		if the first line of the screen used by the FRS is not
**		the top line of the screen.
**	17-oct-89 (sylviap)
**		Changed MTfsmorefunc to be a boolean function from returning a
**		i4.
**	03-jan-90 (sylviap)
**		Added a new parameter to create a title window on the output 
**		browser.  If NULL, then don't put up a title.
**	04/27/90 (dkh) - Added support for popups in scrollable output.
**      24-sep-96 (mcgem01)
**              Global data moved to mtdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
STATUS
MTinit(fscb, title)
FSMTCB	*fscb;
char	*title;
{
	i4	i;
	i4	len;
	i4	starty = IITDflu_firstlineused;

	if ((title != NULL) && (*title != EOS))
	{
		/* 
		** If a title was passed, then create another window at the
		** top of the screen to display it.
		*/
		if ((titlscr = TDnewwin(1, COLS, starty, 0)) == NULL)
		{
			return (FAIL);
		}
		len = STlength(title); 		/* Center the title */
		i = ((COLS - len)/2) + 1;
		TDmove(titlscr, 0, i);		
		TDaddstr(titlscr, title);	/* put up title */
		/* 
		** All other windows will be one row smaller to make room 
		** for the title 
		*/
		starty++;	
	}
	if ((statscr = TDnewwin(1, COLS, starty, 0)) == NULL)
	{
		return (FAIL);
	}

	if ((dispscr = TDnewwin(LINES - 3 - starty, COLS, 1 + starty, 0))
	    == NULL)
	{
		return (FAIL);
	}

	if ((bordscr = TDnewwin(1, COLS, LINES - 2, 0)) == NULL)
	{
		return (FAIL);
	}
	for (i = 0; i < COLS; i++)
	{
		TDtfdraw(bordscr, 0, i, '=');
	}

	MTputmufunc= fscb->mfputmu_proc;
	MTdmpmsgfunc = fscb->mfdmpmsg_proc;
	MTfsmorefunc = fscb->mffsmore_proc;
	MTdmpcurfunc = fscb->mfdmpcur_proc;
	MTdiagfunc = fscb->mffsdiag_proc;

	return(OK);
}



/*{
** Name:	IIMTgl - Get the screen image of scrollable output.
**
** Description:
**	This is a callback routine that is passed to the FT layer
**	by IIMTlf() so that FT can know what the background image
**	is when scrollable output displays popups.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		win	A window to the screen image that is currently
**			displayed by scrollable output.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	04/24/90 (dkh) - Initial version.
*/
WINDOW *
IIMTgl()
{
	/*
	**  If first call, allocate the window, etc.
	**  Note that there is no need to worry about
	**  the width of the screen changing at this point.
	*/
	if (scr_img == NULL)
	{
		if ((scr_img = TDnewwin(LINES - 1 - IITDflu_firstlineused,
			COLS, IITDflu_firstlineused, 0)) == NULL)
		{
			return (NULL);
		}
	}

	/*
	**  Create one image to hand back to FT.
	**  Don't worry about compensating for IITDflu_firstlineused
	**  here since FT will do that for us.  In fact, we want
	**  to uncompensate it.
	*/
	if (titlscr)
	{
		TDoverlay(titlscr, 0, 0, scr_img,
			titlscr->_begy - IITDflu_firstlineused, 0);
	}
	if (dispscr)
	{
		TDoverlay(dispscr, 0, 0, scr_img,
			dispscr->_begy - IITDflu_firstlineused, 0);
	}
	if (statscr)
	{
		TDoverlay(statscr, 0, 0, scr_img,
			statscr->_begy - IITDflu_firstlineused, 0);
	}
	if (bordscr)
	{
		TDoverlay(bordscr, 0, 0, scr_img,
			bordscr->_begy - IITDflu_firstlineused, 0);
	}

	/*
	**  Copy the individual windows into scr_img before
	**  returning.
	*/

	return(scr_img);
}



/*{
** Name:	IIMTlf - Hook to snapshot scrollable output screen
**			 image for FT layer.
**
** Description:
**	This routine enables/disables snapshotting of scrollable output
**	screen image for FT.  This allows FT to properly display the
**	background image when popups are displayed on top of scrollable
**	output.
**
** Inputs:
**	set	TRUE, enable the feature.
**		FALSE, disable the feature.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	04/24/90 (dkh) - Initial version.
*/
VOID
IIMTlf(set)
i4	set;
{
	if (set)
	{
		IIFTsvlSetVifredLayout(IIMTgl);
	}
	else
	{
		IIFTsvlSetVifredLayout(NULL);
	}
}
