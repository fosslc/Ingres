/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<er.h>
# include	"gkscomp.h"
# include	<graf.h>
# include	"gtdev.h"
# include	<gtdef.h>

/**
** Name:	gtninit.c -	Graphics System Initialize (non-interactive)
**
** Description:
**	Initialize the Graphics system interface (GT) for non-interactive use.
**
**      GTninit()	initialize GT, non-interactive.
**
** History:
**		Set "Hold redraw" flag to FALSE. 1/89 Mike S
**
**		Revision 40.106  86/04/26  15:48:36  bobm
**		call GTdmpinit
**		
**		Revision 40.105  86/04/18  14:48:46  bobm
**		non-fatal error message routine for drawing errors
**		
**		Revision 40.104  86/04/17  14:53:24  wong
**		'mode_func()' should start in alpha mode.
**		
**		Revision 40.7  85/12/19  15:49:57  wong
**		Added new 'mode_func()' call to avoid pulling in Forms system.
**
**		Revision 40.6  85/12/18  18:49:52  wong
**		Abstracted 'GTopen()'.
**
**		Revision 40.1  85/12/02  11:40:59  bobm
**		Initial revision.
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Move prototype for mode_func() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**/

extern GR_FRAME *G_frame;	/* Graphics frame structure */

extern Gws	*G_ws;		/* GKS workstation pointer (referenced) */

extern bool	G_restrict;	/* Graphics restrict locator flag */

extern bool G_interactive;
extern bool G_holdrd;

extern i4	(*G_mfunc)();	/* Mode function pointer (set) */
extern i4	(*G_efunc)();	/* Error message function pointer (set) */

static	i4	mode_func();
/*{
** Name:	GTninit() -	initialize GT, non-interactive
**
** Description:
**	Initializes the Graphics system frame structure and the GT system for
**	non-interactive use.  The call to 'GTopen()' returns the TERMCAP
**	entry for the device and initializes the GT device variables and CCHART
**	system.  Basically, a chopped down version of 'GTinit()' that accepts a
**	device type for TERMCAP rather than interrogating the TERM environment
**	variables and doesn't set those parts of the Graphics frame structure
**	that apply to interactive use.
**
** Inputs:
**	pointer to frame.
**	device type.
**	device location.
**	preview level.
**
** Outputs:
**	Returns:
**		STATUS return
**		
** History:
**	10/85 (rlm)	written
**	12/85 (jhw)	Abstracted 'GTopen()'.
**	01-oct-91 (jillb/jroy--DGC)
**	    make function declaration consistent with its definition
*/

RTISTATUS
GTninit (frm, dev, loc, pvw)
register GR_FRAME *frm;
char *dev, *loc;
i4  pvw;
{
	i4	GTmsg_pause();

	GTdmpinit();	/* initialize scr dump routines */

	/* Non-interactive */

	G_interactive = FALSE;
	G_restrict = FALSE;
	G_holdrd = FALSE;
	G_mfunc = mode_func;
	G_efunc = GTmsg_pause;

	if (loc == (char *)NULL || *loc == '\0')
		loc = TTYLOC;

	/* Initialize Graphics device
	**	('G_mfunc' must be set before this call)
	*/

# ifdef VMS
	GTsflush();	/* force flush on VMS */
# endif
	if (GTopen(dev, loc, FALSE) != RTIOK)
		return RTIFAIL;

	/* Initialize frame */

	G_frame = frm;
	frm->gomask = 0;

	/* no menu-line transform */
	frm->ym = frm->xm = 1.0;
	frm->yb = frm->xb = 0.0;

	/* preview level */
	frm->preview = pvw;

	return RTIOK;
}

/*
** Name:	mode_func() -	Non-interactive mode switch function.
**
**	This routine switches the mode of the Graphics system.  All access of
**	this routine is through the 'G_mfunc' pointer, which is set above in
**	'GTninit()' to point here.  Note:  'GTopen()' leaves the device in alpha
**	mode, so this routine's state should also begin in alpha mode.
**
**	Use of a function pointer enables 'GTinit()' and 'GTninit()' to have
**	their own versions without having multiply defined symbols in the same
**	library.  Two versions must be provided so that exclusive non-
**	interactive use of the Graphics system need not link in large parts of
**	the Forms system.
**
**	This code was originally taken directly from VE, which did not provide
**	mode switches, but did provide 'cgraphmode()' and 'calphamode()'
**	routines to use.  This version (non-interactive) implements these
**	routines in-line.  The interactive version (see "gtinit.c") differs in
**	that when running in interactive mode it uses TERMCAP `overide's and
**	flushes the Forms system.
**
** Parameters:
**	mode -- (input only) Mode to which to switch.
**
** History:
**	12/85 (jhw) -- Modified from 'GTinit()' version.
*/

static
mode_func (mode)
i4  mode;
{
	static bool	Alphaflag = TRUE;	/* 'GTopen()' leaves device in
						**	alpha mode to start.
						*/

	void	Valphamode();
	void	Vgraphmode();

	if (mode == ALPHAMODE)
	{
		/* calphamode() */
		if (!Alphaflag)
		{
			Alphaflag = TRUE;

			gesc(Valphamode, G_ws);
			guwk(G_ws, PERFORM);
		}
	}
	else
	{
		/* cgraphmode() */
		if (Alphaflag)
		{
			Alphaflag = FALSE;

			gesc(Vgraphmode, G_ws);
			guwk(G_ws, PERFORM);
		}
	}
}
