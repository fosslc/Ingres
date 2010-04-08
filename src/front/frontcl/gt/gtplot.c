/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<termdr.h>
# include	<graf.h>
# include	<te.h>
# include	<cm.h>

/**
** Name:	gtplot.c -	Graphics System Plot Frame Module.
**
** Description:
**	Plots frame on plot device.  This assumes the Graphics system is being
**	run interactively.
**
**      GTplot()	plot frame.
**
** History:
**	17-nov-88 (bruceb)
**		Added dummy arg in call on TEget.  (arg used in timeout.)
**	02/23/89 (dkh) - Added include of "cm.h" to solve compilation
**			 problems on VMS.
**	3/25/89 (Mike S) - Added GTlock for DG
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for bell_prompt() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**/

/* local prototypes */
static i4 bell_prompt();

GLOBALREF char *G_tc_cl;

GLOBALREF GR_FRAME *G_frame;

GLOBALREF bool G_plotref;

GLOBALREF bool G_interactive;

GLOBALREF i4  G_datchop;

GLOBALREF char *G_ttyloc;
GLOBALREF char *G_myterm;

/*{
** Name:	GTplot -	Plot frame.
**
** Description:
**	Plots frame.  Must swap workstation, turn off adjustment for
**	menu line.  If plot device is users terminal, wait for return before
**	proceeding, and dev argument is unused.  NOTE: this routine is ONLY
**	for running plots on NEW devices when 'GTinit()' has already been
**	called.  'GTplot()' recovers original workstation opened by 'GTinit()'.
**
** Inputs:
**	dev 	plot device.
**	loc	device location.
**	pvw	preview level for plot.
**	ps	prompt string for successful plot on file.
**
** Outputs:
**	Returns:
**		none
**	Exceptions:
**
** History:
**	11/85 (rlm)	written
**	01/86 (jhw)	Pushed TERMCAP access down into 'GTchange_ws()'.
**	6/86 (rlm)	Changes for new driver on same terminal.
**	3/25/89 (Mike S) Add GTlock calls for DG.
*/

STATUS
GTplot (dev, loc, pvw, ps)
char *dev;
char *loc;
i4  pvw;
char *ps;
{
	bool myterm;
	bool clearterm;
	u_i2 omask;
	float oxm,oym,oxb,oyb;
	i4 opvw;
	bool ointr;
	i4 ochop;
	extern i4  G_homerow;

	i4	GTchout();

	/*
	** from this point on, we are changing global parameters.
	** In returns, be careful to reset all changed data.
	** If plotting on screen, prompt with empty string so as to
	** avoid messing up plot.  We set clearterm flag to allow us
	** to clear the screen before restoring old terminal if plotting
	** to the screen with a new device.
	*/

	ointr = G_interactive;
	G_interactive = FALSE;

	clearterm = FALSE;
	myterm = TRUE;
	if (loc == (char *)NULL || *loc == '\0')
	{
		clearterm = TRUE;
		loc = G_ttyloc;
		ps = (char *) NULL;
	}
	else
		myterm = FALSE;
	if (dev == (char *)NULL || *dev == '\0')
		dev = G_myterm;
	else
		myterm = FALSE;

	if (clearterm)
	{	/* Plot to terminal */
# ifdef DGC_AOS
		GTlock(TRUE, 0, 0);
# endif
		GTclear();
		TDtputs(G_tc_cl, 1, GTchout);
		GTaflush();
	}

	if (!myterm)
	{	/* Plot to new workstation (which may actually be terminal) */
		if (GTchange_ws(dev, loc) != OK)
		{
			if (clearterm)
			{
# ifdef DGC_AOS
				GTlock(FALSE, G_homerow, 0);
# endif
				G_plotref = TRUE;
			}
			G_interactive = ointr;
			return FAIL;
		}
	}
	
	/* turn off edit bits */
	omask = G_frame->gomask;
	G_frame->gomask = 0;

	/* remove menu-line transform */
	oym = G_frame->ym;
	oyb = G_frame->yb;
	oxm = G_frame->xm;
	oxb = G_frame->xb;
	G_frame->ym = G_frame->xm = 1.0;
	G_frame->yb = G_frame->xb = 0.0;

	/* preview level */
	opvw = G_frame->preview;
	G_frame->preview = pvw;

	/* no data chopping */
	ochop = G_datchop;
	G_datchop = MAX_POINTS;

	/* OK, let 'er rip! */
	GTrefresh();

	/* restore old parameters */
	G_interactive = ointr;

	if (myterm)
	{
		G_plotref = TRUE;
		bell_prompt();
		TDtputs(G_tc_cl, 1, GTchout);
		GTcursync();
	}
	else
	{
		if (clearterm)
		{
			bell_prompt();
			GTclear();
		}
		GTrestore_ws();
		if (clearterm)
		{
			G_plotref = TRUE;
			TDtputs(G_tc_cl, 1, GTchout);
			GTcursync();
		}
		else
			GTfmsg_pause(ps);
	}

	G_frame->gomask = omask;
	G_frame->ym = oym;
	G_frame->yb = oyb;
	G_frame->xm = oxm;
	G_frame->xb = oxb;
	G_frame->preview = opvw;
	G_datchop = ochop;

# ifdef DGC_AOS
	if (clearterm) GTcsunl();
# endif
	return (OK);
}

/*
** we DON'T want to call GTfmsg_pause with a different device open -
** it may not have control strings, or they might screw up our output.
** we want to strictly prompt with a bell
*/
static
bell_prompt()
{
	char	output[2];
	char	*ptr;

	/*
	** Wait for a <cr>, passing possible escape sequence typed by user
	** through to device.  By making it as if the user had simply typed
	** the bell as the first char, we avoid having to flush the FT stream
	*/

	output[1] = EOS;
	ptr = output;
	for (*ptr = BELL; *ptr != '\r' && *ptr != '\n'; *ptr = TEget((i4)0))
	{
		GTputs(ptr);
	}
}
