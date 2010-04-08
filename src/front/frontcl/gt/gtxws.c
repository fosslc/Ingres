/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<er.h>
# include	"gkscomp.h"
# include	<lo.h>
# include	<me.h>
# include	<termdr.h>
# include	<graf.h>
# include	<grerr.h>
# include	"gtdev.h"
# include	<gtdef.h>
# include	"ergt.h"

/**
** Name:    gtxws.c -	Graphic Systems Switch Workstation Module.
**
** Description:
**	Some general utilities for changing or reseting the C-Chart GKS
**	package.  These are `common' routines needed for both interactive
**	and non-interactive plotting.
**
**	GTreset()	reset cchart
**	GTchange_ws()	change cchart workstation
**	GTrestore_ws()	restore cchart workstation
**	GTreplace_ws()	replace cchart workstation
**
**	GTchange_ws should be used if you wish to be able to recover
**	your old workstation.  GTreplace_ws should be used if you
**	don't.	GTchange_ws/GTrestore_ws push/pop a very small pushdown
**	stack of workstations.	GTreplace_ws is simply a call to
**	GTchange_ws with a pop of the stack frame to avoid eventual
**	overflow.
**
** History:
**	23-oct-86 (bab)
**		Removed spurious extern of G_frame.
**	12/24/87 (dkh) - Changes to TDtgetent interface.
**	02/18/89 (dkh) - Yet more performance changes.
**	2/27/89 (Mike S.) - Metafile-related changes
**			- Explicit check for GKS drivers
**	3/21/89 (Mike S) - Use GTgkserr to check errors.
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	26-aug-93 (dianeh)
**		Added #include of <me.h>.
**	
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN bool	TDtgetflag();

# define	WSTK_LEN 4	/* should be ample length */

# ifdef DEBUG
# define assert(ex, str)	{if (!ex) GTsyserr(str,0);}
# else
# define assert(ex, str)	;
# endif

extern Gws	*G_ws;		/* GKS workstation pointer */

extern Gws	*G_mfws;	/* GKS metafile workstation pointer */

extern bool	G_restrict;	/* Graphics restrict locator flag */

extern bool	G_gkserr;	/* GKS SysError Flag (switched) */

/* Saved workstation stack */
static struct
{
	Gws	*ws;
	Gws	*mfws;
	GT_DEVICE dev;
} Wstack [WSTK_LEN];
static i4  Ws_idx = 0;


/*{
** Name:	GTreset - reset cchart
**
** Description:
**	resets cchart to make a new drawing.  Done by closing /
**	reopening package, which is supposed to do nothing but
**	initialization of status variables
**
** Inputs:
**	none
**
** Outputs:
**
** Side Effects:
**	none
**
** History:
**	8/85 (rlm)	written
*/

void
GTreset ()
{
	cclc();
	copc(1, 2, stderr);
	cnewws(G_ws);
}

/*{
** Name:	GTchange_ws() - Change C-Chart workstation.
**
** Description:
**	This routine activates a new workstation for drawing a graph.  It
**	shuts everything down, saving the old workstation variables, and
**	and reopens with a new workstation, saving old one.
**
** Inputs:
**	dev (required input) -- device type name
**	loc (required input)) -- workstation location
**
** Outputs:
**	Returns:
**		STATUS code.
**	Exceptions:
**		GTsyserr
**
** Side Effects:
**	Sets 'G_ws' and 'G_dev'.
**
** History:
**	10/85 (rlm)	written
**	01/86 (jhw)	Pushed TERMCAP access down here from 'GTplot()'.
**	2/22/89 (Mike S)Set G_mnum
*/

RTISTATUS
GTchange_ws (dev, loc)
char *dev;
char *loc;
{
	register Gcofac *fac;
	Gmkfac		*mkfac;
	register char	*driver;
	char		*mf_driver;
	char		Csbufr[64];
	char		tbufr[2048];
	i4		num;
	char		*tc_area;
	bool		tmpg_mf;

	char	*TDtgetstr();
	Gws	*gopwk();
	Gcofac	*gqcf();
	Gmkfac	*gqpmf();
	Gwscat  gqwkca(), wscat;
	Gerror 	GTgkserr();

	assert(dev != (char *)NULL && dev != '\0', S_GT0012_No_device);
	assert(loc != (char *)NULL && loc != '\0', S_GT0013_No_loc);

	/* Get TERMCAP entry */
	if (TDtgetent(tbufr, dev, NULL) < 1)
	{
		GTperror(E_GT001A_UNKNOWN, 1, dev);
		return RTIFAIL;
	}

	tc_area = Csbufr;

	/* driver code for plot */
	driver = TDtgetstr(ERx("Gp"), &tc_area);
	if (driver == (char *)NULL || *driver == '\0')
	{
		GTperror(BAD_DEVICE, 1, dev);
		return RTIFAIL;
	}
	wscat = gqwkca(driver);
	if (GTgkserr() != NO_ERROR)
	{
		GTperror(E_GR0045_bad_plot_dev, 1, driver);
		return RTIFAIL;
	}

	/* See if it's a metafile plot */
	if ( tmpg_mf = TDtgetflag( ERx("Gm") ) )
	{
		mf_driver = TDtgetstr(ERx("Ge"), &tc_area);
		if (mf_driver == NULL || *mf_driver == EOS)
		{
			GTperror(E_GR0046_no_mf_driver, 1, dev);
			return RTIFAIL;
		}
		wscat = gqwkca(mf_driver);
		if (GTgkserr() != NO_ERROR)
		{
			GTperror(E_GR0047_bad_mf_driver, 1, mf_driver);
			return RTIFAIL;
		}
	}

	/* save old workstation */
	if (Ws_idx >= WSTK_LEN)
		return RTIFAIL;
	Wstack[Ws_idx].ws = G_ws;
	Wstack[Ws_idx].mfws = G_mfws;
	MEcopy((char *) &G_dev, sizeof(GT_DEVICE), (char *) &(Wstack[Ws_idx].dev));
	G_mf = tmpg_mf;

	/* obtain new workstation */
	G_gkserr = FALSE;
	if (G_mf)
	{
		G_mfws = gopwk(loc, mf_driver); 
		if (G_mfws != (Gws *)NULL) 
			G_ws = gopwk(NULLDEV, driver);
		else
			G_ws = (Gws *)NULL;
	}
	else
	{
		G_ws = gopwk(loc, driver);
		G_mfws = (Gws *)NULL;
	}
	G_gkserr = TRUE;
	if (G_ws == (Gws *)NULL)
	{
		/* restore old workstation */
		if (G_mfws != NULL) gclwk(G_mfws);
		G_ws = Wstack[Ws_idx].ws;
		G_mfws = Wstack[Ws_idx].mfws;
		MEcopy((char *) &(Wstack[Ws_idx].dev), sizeof(GT_DEVICE), 
			(char *) &G_dev);
		GTperror(BAD_FILE, 1, loc);
		return RTIFAIL;
	}

	/* switch active workstation (to new) */
	gdacwk(Wstack[Ws_idx].ws);
	if (Wstack[Ws_idx].dev.g_mf) gdacwk(Wstack[Ws_idx].mfws);
	gacwk(G_ws);
	if (G_mf) gacwk(G_mfws);

	/* OK - so bump up stack pointer */
	++Ws_idx;

	/* obtain color availability */
	fac = gqcf(driver);
	G_cnum = (fac->coavail == MONOCHROME) ? 2 : fac->colours;

	/* number of marker types defined */
	mkfac = gqpmf(driver);
	G_mnum = mkfac->types;

	/* aspect ratio */
	if ((num = TDtgetnum(ERx("Ga"))) < 0)
		G_aspect = DEFASPECT;
	else
		G_aspect = ((float) num) / 1000.0;

	/* flag for "indelible" graphics */
	G_indelible = TDtgetflag(ERx("Gi"));

	/* open cchart (with new) */
	cclc();
	copc(1, 2, stderr);
	cnewws(G_ws);

	return RTIOK;
}

/*{
** Name:	GTrestore_ws() -	Restore C-Chart Workstation.
**
** Description:
**	Reactivates the last workstation, changed by 'GTchange_ws()'.
**
** Inputs:
**
** Outputs:
**	FAIL return for empty stack - no workstation to recover.
**
** Side Effects:
**	Resets 'G_dev', 'G_ws', and 'G_mfws'.
**
** History:
**	12/85 (rlm)	written
*/

RTISTATUS
GTrestore_ws ()
{
	if (Ws_idx < 0)
		return (RTIFAIL);

	/* decrement stack pointer */
	--Ws_idx;

	/* switch active workstation (to old) */
	gdacwk(G_ws);
	if (G_mf) gdacwk(G_mfws);
	gacwk(Wstack[Ws_idx].ws);

	/* open cchart (with old) */
	cclc();
	copc(1, 2, stderr);
	cnewws(Wstack[Ws_idx].ws);
	gclwk(G_ws);
	if (G_mf) gclwk(G_mfws);

	/* restore old workstation */
	G_ws = Wstack[Ws_idx].ws;
	G_mfws = Wstack[Ws_idx].mfws;
	MEcopy((char *) &(Wstack[Ws_idx].dev), sizeof(GT_DEVICE), (char *) &G_dev);
	if (G_mf) gacwk(G_mfws);
	return (RTIOK);
}

/*{
** Name:	GTreplace_ws() -	Replace cchart workstation
**
** Description:
**	Replaces workstation.  Unlike GTchange_ws, does not stack it for
**	recovery.  Otherwise works like GTchange_ws.
**
*/

RTISTATUS
GTreplace_ws (dev, loc)
char *dev;
char *loc;
{
	if (GTchange_ws(dev,loc) != RTIOK)
		return (RTIOK);

	/* simply pop stack frame and throw away */
	--Ws_idx;
	return (RTIOK);
}
