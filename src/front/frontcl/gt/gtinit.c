/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<er.h>
# include	<st.h>
# include	"gkscomp.h"
# include	<nm.h>
# include	<ft.h>
# include	<termdr.h>
# include	<graf.h>
# include	<grerr.h>
# include	<kst.h>
# include	"gtdev.h"
# include       <gtdef.h>
# include	"ergt.h"
# include       <gl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <ug.h>
# include	<cv.h>

/**
** Name:    gtinit.c -		Graphics System Initialize Module (interactive)
**
** Description:
**	Initialize the Graphics system interface (GT) for interactive use.
**
**	GTinit		initialize GT, interactive
**
**
** History:
**	86/02/27  10:53:19  wong
**		Changed to use 'G_restrict'; 'gpavail' and 'locate' removed
**		from "GR_FRAME" structure.
**
**	86/03/24  18:00:26  bobm
**		GTstat -> GTstl
**
**	86/04/18  14:47:50  bobm
**		non-fatal error message routine for drawing errors
**
**	86/04/26  15:48:09  bobm
**		call GTdmpinit
**
**	6/86 Store term and TTYLOC for GTplot	bobm
**	1/89 Make "hold redraw" flag global 	Mike S
**	02/18/89 (dkh) - Yet more performance changes.
**	3/25/89 (Mike S) - Add GTlock calls for DG
**	7/3/89 (Mike S) - Separate out GTwinmap
**	10/19/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	28-mar-90 (bruceb)
**		Added locator support to err_func().
**      21-jan-93 (pghosh)
**              Modified COORD to IICOORD. This change was supposed to be 
**              done by leighb when the ftdefs.h file was modified to have 
**              IICOORD.
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**      04-may-97 (mcgem01)
**              Now that all hard-coded references to TERM_INGRES have
**              been removed from the message files, make the appropriate 
**		system terminal type substitutions in the error messages.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Move prototype for calphamode(), cgraphmode(), mode_func() &
**	  err_func() to prevent compiler errors with GCC 4.0 due to conflict
**	  with implicit declaration.
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**/

extern GR_FRAME *G_frame;	/* Graphics frame structure (set) */

extern Gws	*G_ws;		/* GKS workstation pointer (referenced) */

/* Graphics TERMCAP strings */
extern char *G_tc_cm, *G_tc_GE, *G_tc_GL;

extern i4	G_lines;	/* Graphics lines on device */
extern i4	G_cols;		/* Graphics columns on device */

extern bool	G_plotref;	/* Graphics plot refresh flag */
extern bool	G_restrict;	/* Graphics locator restrict flag */
extern bool G_mref;
extern bool G_interactive;
extern bool G_holdrd;
extern i4   G_homerow;

extern char *G_myterm;
extern char *G_ttyloc;

extern u_i2 FTgtflg;

extern i4  IITDflu_firstlineused;

extern char *FEtstout;	/* .res file name */

extern i4  (*G_mfunc) ();	/* pointer to mode switch function */
extern i4  (*G_efunc) ();	/* pointer to error message function */

static EXTENT Subext;


GLOBALREF	VOID	(*iigtclrfunc)();
GLOBALREF	VOID	(*iigtreffunc)();
GLOBALREF	VOID	(*iigtctlfunc)();
GLOBALREF	VOID	(*iigtsupdfunc)();
GLOBALREF	bool	(*iigtmreffunc)();
GLOBALREF	VOID	(*iigtslimfunc)();
GLOBALREF	i4	(*iigteditfunc)();

FUNC_EXTERN	VOID	GTclear();
FUNC_EXTERN	VOID	GTrefresh();
FUNC_EXTERN	VOID	GTctl();
FUNC_EXTERN	VOID	GTstlupdt();
FUNC_EXTERN	bool	GTmref();
FUNC_EXTERN	VOID	GTscrlim();
FUNC_EXTERN	i4	GTedit();
FUNC_EXTERN	KEYSTRUCT	*GTgetch();
FUNC_EXTERN	VOID	IITDpciPrtCoordInfo();
FUNC_EXTERN	VOID	IITDposPrtObjSelected();
FUNC_EXTERN	bool	TDtgetflag();

static	i4	mode_func();
static	i4	err_func();
static  calphamode();
static  cgraphmode();

/*{
** Name:	GTinit() -	Initialize GT, interactive.
**
** Description:
**	initializes cchart and GT library variables
**	for editing.  This routine is not called by a program
**	interested in doing a non-interactive plot.
**
** Inputs:
**	pointer to frame.
**	graphic object mask.
**	initial extent limits.
**
** Outputs:
**	Returns:
**		STATUS	OK or FAIL
**
** Side Effects:
**	Sets G_interactive, G_frame, and G_plotref.
**
** History:
**	8/85 (rlm)	written
**	3/89 (Mike S)	calculate transforms when top line is unavailable.
**	10/19/89 (dkh) - Added change to eliminate duplicate FT files
**			 in GT directory.
**	01-oct-91 (jillb/jroy--DGC)
**	    make function declaration consistent with its definition
*/

RTISTATUS
GTinit (frm, mask, ext)
GR_FRAME *frm;
u_i2 mask;
EXTENT *ext;
{
	char	*STalloc();

	GTdmpinit();	/* initialize scr dump routines */

	/* Interactive */

	G_interactive = TRUE;
	G_plotref = FALSE;
	G_restrict = FALSE;
	G_mfunc = mode_func;		/* set mode function */
	G_efunc = err_func;		/* error message function */

	/* Get terminal name */
	NMgtAt(ERx("TERM_INGRES"), &G_myterm);
	if (G_myterm == (char *)NULL || *G_myterm == '\0')
		NMgtAt(TERM_TYPE, &G_myterm);
	if (G_myterm == (char *)NULL || *G_myterm == '\0')
	{
		IIUGerr(E_GT0019_NO_TERM, UG_ERR_ERROR, 1, SystemTermType);
		return RTIFAIL;
	}
	G_myterm = STalloc(G_myterm);
	CVlower(G_myterm);

	/* redirect graphics i/o, if requested */
	NMgtAt(ERx("II_GROUT_RED"), &G_ttyloc);
	if (G_ttyloc == NULL || *G_ttyloc == '\0')
		/* if test output file, open to "stdout" */
		G_ttyloc = (FEtstout != NULL) ? ERx("-") : TTYLOC;
	else
		G_ttyloc = STalloc(G_ttyloc);

	/* Start Graphics System & Device
	**	('G_mfunc' must be set before this call.)
	*/
	if (GTopen(G_myterm, G_ttyloc, TRUE) != RTIOK)
		return RTIFAIL;

	if (G_indelible)
	{	/* Can't edit on indelible device */
		GTerror(BAD_EDEVICE, 1, G_myterm);
		return RTIFAIL;
	}

	G_holdrd = TDtgetflag(ERx("Gh"));

	/* Initialize frame */

	G_frame = frm;
	G_frame->gomask = mask;
	G_frame->subex = &Subext;
	G_frame->lim = ext;
	G_frame->lnum = 0;
	G_frame->cur = G_frame->head = G_frame->tail = NULL;
	
	/*
	**  Set up special function pointers for GT routines
	**  called by FT code.
	*/
	iigtclrfunc = GTclear;
	iigtreffunc = GTrefresh;
	iigtctlfunc = GTctl;
	iigtsupdfunc = GTstlupdt;
	iigtmreffunc = GTmref;
	iigtslimfunc = GTscrlim;
	iigteditfunc = GTedit;

	return (RTIOK);
}

/*{
** Name:	GTwinmap() -	Map graphics window to screen window.
**
** Description:
**	Map the GKS window to a portion of the alpha screen for editing.  This 
**	routine is not called by a program interested in doing a non-interactive
**	plot; it must be called after TEopen determines whether lines at the 
**	top of the screen are reserved.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**		STATUS	OK or FAIL
**
** Side Effects:
**	Sets G_frame, G_homerow
**
** History:
**	7/89 (Mike S)	
**		Separated from GTinit.
**			
*/
STATUS	
GTwinmap()
{
	float whole, midfract, topfract, botfract;
	i4 tmpnum;
	
	/* get graphics window */

	if ((G_frame->lrow = TDtgetnum(ERx("Gr"))) < 0)
		G_frame->lrow = 0;
	if ((G_frame->lcol = TDtgetnum(ERx("Gc"))) < 0)
		G_frame->lcol = 0;
	if ((G_frame->hrow = TDtgetnum(ERx("GR"))) < 0)
		G_frame->hrow = G_MROW;
	if ((G_frame->hcol = TDtgetnum(ERx("GC"))) < 0)
		G_frame->hcol = G_cols - 1;

	/* Check for reserved rows at the top of the screen */
	G_homerow = 0;
	if ( IITDflu_firstlineused != 0)
	{
		G_homerow = IITDflu_firstlineused;
		G_frame->lrow += G_homerow;
	}

	if (G_frame->lcol >= G_frame->hcol || G_frame->lrow >= G_frame->hrow)
	{
		GTerror(BAD_DEVWIN, 1, G_myterm);
		return RTIFAIL;
	}

	/*
	**	The screen is ordinarily divided into three windows:
	**		Top reserved area (currently only on DG when 
	**		  running under CEO)
	**		Drawing area
	**		Status and menu lines.
	**
	*/
	if (G_frame->hrow >= G_SROW)
	{
		/* calculate window sizes */
		whole = (float)(G_frame->hrow);
		botfract = ((float)(G_frame->hrow - G_SROW + 1)) / whole;
		topfract = ((float)(G_homerow)) / whole;
		midfract = 1.0 - topfract - botfract;

		/* Map our world to the drawing area, preserving aspect ratio */
		G_frame->ym = G_frame->xm = midfract;
		G_frame->yb = botfract;
		G_frame->xb = 0.0;

		/* now adjust rows and columns of character window by ratio */
		G_frame->hrow = G_SROW - 1;
		midfract *= (float) (G_frame->hcol - G_frame->lcol + 1);
		midfract += (float) G_frame->lcol;
		G_frame->hcol = midfract;	/* truncating */
	}
	else
	{
		G_frame->ym = G_frame->xm = 1.0;
		G_frame->yb = G_frame->xb = 0.0;
	}

	/* calculate size of alpha cursor, line width in graphics coordinates */
	G_frame->csx = 1.0/((float) (G_frame->hcol - G_frame->lcol + 1));
	G_frame->csy = 1.0/((float) (G_frame->hrow - G_frame->lrow + 1));
	if ((tmpnum = TDtgetnum(ERx("Gx"))) < 0)
		tmpnum = 10;
	G_frame->lx = G_frame->csx / (float) tmpnum;
	if ((tmpnum = TDtgetnum(ERx("Gy"))) < 0)
		tmpnum = 10;
	G_frame->ly = G_frame->csy / (float) tmpnum;

	return RTIOK;
}

/*
**	error message function
*/
static
err_func (msg)
char *msg;
{
	char			buf[120];
	register KEYSTRUCT	*k;
	IICOORD			done;

	IIUGfmt(buf, 120, ERget(F_GT000C_RET_format), 1, msg);
	GTmsg(buf);

	done.begy = done.endy = G_MROW;
	done.begx = 0;
	done.begx = COLS - 1;
	IITDpciPrtCoordInfo(ERx("err_func"), ERx("done"), &done);

	for (;;)
	{
	    k = GTgetch();
	    if (k->ks_ch[0] == '\n' || k->ks_ch[0] == '\r')
	    {
		break;
	    }
	    if (k->ks_fk == KS_LOCATOR)
	    {
		if (k->ks_p1 == done.begy)
		{
		    IITDposPrtObjSelected(ERx("done"));
		    break;
		}
		else
		{
		    FTbell();
		}
	    }
	}
	if ((FTgtflg & FTGTEDIT) != 0 && G_interactive)
	{
		FTgtrefmu();
		G_mref = FALSE;
	}
	else
	{
		GTmsg(ERx(""));
	}
}

/*
**	mode switch function ( G_mfunc will point to this )
*/
static
mode_func (mode)
i4  mode;
{
	static bool	Alphaflag = TRUE;
	i4	erarg;

	switch (mode)
	{
	case ALPHAMODE:
		if (!Alphaflag)
		{
			Alphaflag = TRUE;
			calphamode();
			if ((FTgtflg & FTGTEDIT) != 0 && G_interactive)
			{
				GTstlpop(TRUE);
				GTstlupdt(FALSE);
				FTgtrefmu();
				G_mref = FALSE;
			}
		}
		break;
	case GRAPHMODE:
		if (Alphaflag)
		{
			if ((FTgtflg & FTGTEDIT) != 0 && G_interactive)
			{
				GTmsg(ERx(""));
				GTstlpush(ERget(F_GT0010_Working),TRUE);
				GTstlupdt(FALSE);
			}
			Alphaflag = FALSE;
			cgraphmode();
		}
		break;
	case LOCONMODE:
		cgraphmode();
		break;
	case LOCOFFMODE:
		calphamode();
		break;
	default:
		erarg = mode;
		GTsyserr(S_GT000C_Bad_mode,1, (PTR) &erarg);
	}
}

static
calphamode()
{
	i4	GTchout();
	Gint	Valphamode();
	extern i4  G_homerow;

	if (G_interactive && G_tc_GL != NULL && *G_tc_GL != '\0')
	{
		guwk(G_ws, PERFORM);
		TDtputs(G_tc_GL, 1, GTchout);
	}
	else
	{
		gesc(Valphamode, G_ws);
		guwk(G_ws, PERFORM);
	}

	if (G_interactive)
	{
# ifdef DGC_AOS
		GTcsunl();
# else
		GTcursync();
		GTaflush ();
# endif
	}
}

static
cgraphmode()
{
	char	*TDtgoto();
	i4	GTchout();
	Gint	Vgraphmode();

	extern i4  	G_homerow;

	if (G_interactive)
	{
# ifdef DGC_AOS
		GTlock(TRUE, 0, 0);
# endif
		TDtputs(TDtgoto(G_tc_cm, G_homerow, 0), 1, GTchout);
		GTaflush();
	}

	if (G_interactive && G_tc_GE != NULL && *G_tc_GE != '\0')
	{
		if (!G_holdrd)
			guwk(G_ws, PERFORM);
		TDtputs(G_tc_GE, 1, GTchout);
		GTaflush();
	}
	else
	{
		gesc(Vgraphmode, G_ws);
		if (!G_holdrd)
			guwk(G_ws, PERFORM);
	}
}



/*{
** Name:	GTctl - Control and syncrhonize forms and graphics display.
**
** Description:
**	called on entering or leaving a form with graphics.
**
** Inputs:
**	disp = FALSE means leaving - clear graphics from screen.
**	disp = TRUE should be called from initialize block of form.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	10/19/89 (dkh) - Moved here from ft!ftgtctl.c
*/
VOID
GTctl(disp,iact)
bool disp;		/* graphics display on form */
bool iact;		/* interactive graphics form */
{
	if (disp)
	{
		GTpark ((float) 0.0,(float) 1.0);
		if (iact)
		{
			FTgtflg = FTGTACT | FTGTREF | FTGTEDIT;
			GTstlinit ();
		}
		else
		{
			FTgtflg = FTGTACT | FTGTREF;
			GTstloff ();
		}
	}
	else
	{
		FTgtflg = 0;
		GTclear ();
		GTstloff ();
	}
}
