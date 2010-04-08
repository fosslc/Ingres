/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<er.h>
# include	"gkscomp.h"
# include	<cm.h>
# include	<lo.h>
# include	<termdr.h>
# include	<grerr.h>
# include	"gtdev.h"
# include	<gtdef.h>
# include	"ergt.h"

/**
** Name:	gtws.c -	Graphics System Open/Close Workstation Module.
**
** Description:
**	Contains routines that open and close the Graphics System workstation
**	with regard to the C-Chart GKS packages and the GT device variables
**	initialization.	 (The device being the workstation to C-Chart GKS.)
**
**	GTopen()	start CCHART GKS and initialize GT device variables.
**	GTclose()	close CCHART GKS.
**
** History:
**	86/01/06  23:59:57  wong
**	Initial revision (abstracted from "gtinit.c".)
**
**	86/04/15  15:34:45  wong
**	Always clear text when opening separate plane GKS device.
**	(BUG #8780:  GKS only clears graphics plane, not text.)
**
**	86/04/17  14:53:51  wong
**	Force alpha mode.
**
**	7/10/86 (bab)	- Fix for bug 9756.  If driver code isn't
**		found, trying to edit (not plot), and device is
**		indelible, error is "Not an edit device" instead
**		of simply "Unsupported graphics device".
**	12/24/87 (dkh) - Changes to TDtgetent interface.
**	2/9/89	 (Mike S) - add "down" and "backspace" termcap strings.
**	02/18/89 (dkh) - Yet more performance changes.
**	2/27/89	 (Mike S) - metafile-related changes
**	3/21/89  (Mike S) - Use GTgkserr() to check errors.
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Oct-2005 (hanje04)
**	    Move prototype for graphics_close() to prevent compiler
**          errors with GCC 4.0 due to conflict with implicit declaration.
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

extern Gws	*G_ws;		/* GKS workstation pointer (set) */
extern Gws	*G_mfws;	/* GKS metafile workstation pointer (set) */
extern Gdefloc	*G_dloc;	/* GKS locator device (set) */

extern bool G_gkserr;		/* GKS Error Flag (switched) */
extern bool G_idraw;		/* GKS Internal Routine Flag (initialized) */

/* Graphics TERMCAP (set) */
extern char *G_tc_GE, *G_tc_GL;
extern char *G_tc_ce, *G_tc_cm, *G_tc_up, *G_tc_nd;
extern char *G_tc_dc, *G_tc_rv, *G_tc_re, *G_tc_cl;
extern char *G_tc_bl, *G_tc_be;
extern char *G_tc_do, *G_tc_bc;

extern i4	G_lines;	/* Graphics Lines on device (set) */
extern i4	G_cols;		/* Graphics Columns on device (set) */

extern i4  (*G_mfunc)();	/* Mode function pointer (referenced) */

FUNC_EXTERN	VOID	TDtcaploc();
FUNC_EXTERN	bool	TDtgetflag();

/* Is workstation location the terminal?  (Actually, standard out.) */

static bool	is_term;

static	i4		graphics_close();

/*{
** Name:	GTopen() -	Start C-Chart GKS, Initialize GT Device.
**
**	This routine reads a device description from the TERMCAP file and then
**	initializes the GT globals that describe the device.  The C-Chart GKS
**	package is also started.  It is largely independent of whether or not
**	the Graphics system is to be used in the non-interactive mode.
**
**	This routine forces the GKS device to be in alpha mode, so the calling
**	routines can depend on that fact.
**
**	Note:  The function pointer, 'G_mfunc' must be set before calling this
**	routine.
**
** Parameters:
**	dev -- (input only) The type of graphics device.
**
**	loc -- (input only) The name of the graphics device.
**
**	edit -- (input only) A flag set when initializing for
**		interactive use.
**
** Returns:
**	OK -- No errors.
**	FAIL -- Errors in the TERMCAP entry or in C-Chart GKS.
**
** Side Effects:
**	Sets the following GT globals:
**		G_ws
**		G_mfws
**		G_dloc
**		G_dev		(G_cnum, G_mnum, G_aspect, G_mf,
**				 G_indelible, G_gplane, G_govst)
**		G_lines
**		G_cols
**		G_tc_*		Graphics Termcap (GE, GL, cm, up, nd,
**					ce, dc, rv, re, cl, do, bc)
**		G_idraw
**
** History:
**	01/86 (jhw) -- Abstracted from 'GTninit()' and 'GTinit()'.
**	04/86 (jhw) -- Send clear screen for separate text/graphic plane device.
**	2/22/89 (Mike S) -- get number of marker types supported.
**			metafile-related changes.
**	01-oct-91 (jillb/jroy--DGC)
**	    make function declaration consistent with its definition
*/

RTISTATUS
GTopen (dev, loc, edit)
char	*dev,
	*loc;
bool	edit;
{
	static char	Csbufr[256];	/* expanded TERMCAP values */

	register Gcofac *fac;
	Gmkfac		*mkfac;
	register char	*driver;	/* drive code */
	char		*mf_driver;	/* Metafile driver */
	register i4	tmpnum;
	char		*tc_area;

	char		*TDtgetstr();
	Gws		*copg();
	Gws		*gopwk();
	Gcofac		*gqcf();
	Gmkfac		*gqpmf();
	Gdefloc		*gqdlc();
	Gwscat		gqwkca(), wscat;
	Gint		Valphamode();
	Gint		Vstring();
	Gerror		GTgkserr();
	LOCATION	tloc;
	char		locbuf[MAX_LOC + 1];
	char		tbuf[2048];

	if (G_mfunc == NULL)
		GTsyserr(S_GT0011_No_mode_func,0);

	TDtcaploc(&tloc, locbuf);

	/* Get TERMCAP entry */
	if (TDtgetent(tbuf, dev, &tloc) != 1)
	{
		IIUGerr(E_GT001A_UNKNOWN, 0, 1, dev);
		return RTIFAIL;
	}
	tc_area = Csbufr;

	/* check for metafile */
	G_mf = TDtgetflag(ERx("Gm")) && !edit;

	/* driver code for plot */
	driver = TDtgetstr(edit ? ERx("Ge") : ERx("Gp"), &tc_area);
	if (G_mf) mf_driver = TDtgetstr(ERx("Ge"), &tc_area);

	/* flag for "indelible" graphics */
	G_indelible = TDtgetflag(ERx("Gi"));

	if (driver == (char *)NULL || *driver == '\0')
	{
		/* if no "Ge", but "Gi", than non-edit graphics device */
		if (edit && G_indelible)
			IIUGerr(BAD_EDEVICE, 0, 1, dev);
		else	/* non-supported device */
			IIUGerr(BAD_DEVICE, 0, 1, dev);
		return RTIFAIL;
	}
	
	/* TERMCAP strings */
	G_tc_GE = TDtgetstr(ERx("GE"), &tc_area);
	G_tc_GL = TDtgetstr(ERx("GL"), &tc_area);
	G_tc_cm = TDtgetstr(ERx("cm"), &tc_area);
	G_tc_up = TDtgetstr(ERx("up"), &tc_area);
	G_tc_nd = TDtgetstr(ERx("nd"), &tc_area);
	G_tc_ce = TDtgetstr(ERx("ce"), &tc_area);
	G_tc_dc = TDtgetstr(ERx("dc"), &tc_area);
	G_tc_rv = TDtgetstr(ERx("rv"), &tc_area);
	G_tc_re = TDtgetstr(ERx("re"), &tc_area);
	G_tc_cl = TDtgetstr(ERx("cl"), &tc_area);
	G_tc_bl = TDtgetstr(ERx("bl"), &tc_area);
	G_tc_be = TDtgetstr(ERx("be"), &tc_area);
	G_tc_do = TDtgetstr(ERx("do"), &tc_area);
	G_tc_bc = TDtgetstr(ERx("bc"), &tc_area);

	/* obtain workstation */

	G_gkserr = FALSE;
	G_mfws = (Gws *)NULL;
	if (G_mf)
	{
		G_ws = copg(NULLDEV, driver);
		if (G_ws != (Gws *)NULL)
			G_mfws = gopwk(loc, mf_driver);
	}
	else
	{
		G_ws = copg(loc, driver);
	}
	if (G_ws == (Gws *)NULL)
	{
		/* Determine what caused the error (Bug 4250) */
		wscat = gqwkca(driver);
		if (GTgkserr() == NO_ERROR)
			IIUGerr(BAD_FILE, 0, 1, *loc == '-' ? ERx("terminal") : loc);
		else
			IIUGerr(E_GR0045_bad_plot_dev, 0, 1, driver);
		if (G_mfws != (Gws *)NULL) gclwk(G_mfws);
		return RTIFAIL;
	}
	if (G_mf && (G_mfws == (Gws *)NULL))
	{
		wscat = gqwkca(mf_driver);
		if (GTgkserr() == NO_ERROR)
			IIUGerr(BAD_FILE, 0, 1, *loc == '-' ? ERx("terminal") : loc);
		else if (mf_driver == NULL || *mf_driver == EOS)
			IIUGerr(E_GR0046_no_mf_driver, 0, 1, dev);
		else
			IIUGerr(E_GR0047_bad_mf_driver, 0, 1, mf_driver);
		gdacwk(G_ws);
		gclwk(G_ws);
		return RTIFAIL;
	}
	if (G_mf) gacwk(G_mfws);
	G_gkserr = TRUE;

	FEset_exit(graphics_close);

	/* set terminal flag */
	is_term = STcompare(loc, ERx("-")) == 0 || STcompare(loc, TTYLOC) == 0;

	/* obtain color availability */
	fac = gqcf(driver);
	G_cnum = fac->coavail == MONOCHROME ? 2 : fac->colours;

	/* obtain number of marker types available */
	mkfac = gqpmf(driver);
	G_mnum = mkfac->types;
	
	/*
	** obtain locater device
	**
	**	Bug in C-Chart causes failure in 'copc()' when 'gqdlc()' called
	**	and no locator is supported for the graphics device.  So
	**	TERMCAP entry defined to flag this fact.
	*/
	if (!edit || !TDtgetflag(ERx("Gl")))
		G_dloc = (Gdefloc *)NULL;
	else	/* should have locator device */
		if ((G_dloc = gqdlc(driver, 1)) == (Gdefloc *)NULL)
		{
			GTerror(BAD_LOCATOR, 1, dev);
			return RTIFAIL;
		}

	/* Lines and columns */
	if ((G_lines = TDtgetnum(ERx("li"))) <= 0)
		G_lines = 24;
	if ((G_cols = TDtgetnum(ERx("co"))) <= 0)
		G_cols = 80;

	/* flag for wide lines */
	G_wline = TDtgetflag(ERx("Gw"));

	/* aspect ratio */
	if ((tmpnum = TDtgetnum(ERx("Ga"))) < 0)
		G_aspect = DEFASPECT;
	else
		G_aspect = ((float) tmpnum) / 1000.0;

	/* graphics / text interaction flags for this device */
	if (TDtgetflag(ERx("Gs")))
	{
		/*
		**	on a separate plane device, we need ce or dc to
		**	erase characters from graphics screen.
		*/
		G_gplane = ((G_tc_ce != (char *)NULL && *G_tc_ce != '\0') ||
				(G_tc_dc != (char *)NULL && *G_tc_dc != '\0'));
		G_govst = FALSE;
	}
	else
	{
		G_gplane = FALSE;
		G_govst = TDtgetflag(ERx("Go"));
	}

	/* open cchart, set alpha mode */
	copc(1, 2, stderr);
	cnewws(G_ws);
	gesc(Valphamode, G_ws);		/* Force alpha mode */

	/* Clear text whenever GKS doesn't.
	**
	** This is whenever the GKS device has separate text/graphics planes.
	*/
	if (G_gplane)
	{ /* Clear text on workstation */
		register char	*gp;
		Gescstring	clear;

		clear.ws = G_ws;
		/* Output clear string w/o pad */
		gp = G_tc_cl;
		while (CMdigit(gp))
			++gp;
		clear.s = gp;
		gesc(Vstring, &clear);	/* output to GKS device (alpha mode) */
	}

	G_idraw = FALSE;

	return RTIOK;
}

/*{
** Name:	GTclose() -	Close C-Chart GKS.
**
** Description:
**	Close down the Graphics system.	 This closes C-Chart, clears the
**	graphics device (if it's indelible or doesn't overstrike text
**	over graphics) and leaves it in non-graphics mode, and closes GKS.
**
** History:
**	01/86 (jhw) -- Written.
**	5/22/86 (bab)	- No longer clears the graphics device because
**			  of being indelible.  Fix for bug 9399
**	2/28/89 (Mike S) - Close metafile workstation.
**	01-oct-91 (jillb/jroy--DGC)
**	    make function declaration consistent with its definition
*/

void
GTclose()
{
	graphics_close();
	FEclr_exit(graphics_close);
}

static i4
graphics_close ()
{
	if (G_ws != (Gws *)NULL)
	{	/* Close C-Chart and GKS */
		cclc();
		/*
		** fix for bug 9399 -
		** was:
		**	if (G_indelible || (! G_govst && is_term))
		*/
		if (!G_govst && is_term)
			GTclear();
		(*G_mfunc)(ALPHAMODE);
		if (G_mfws)
		{
			gdacwk(G_mfws);
			gclwk(G_mfws);
		}
		cclg(G_ws);
	}
}
