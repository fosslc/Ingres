/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	"gkscomp.h"
# include	<graf.h>
# include	"gtdev.h"
# include	<gtdef.h>
# include	"ergt.h"
# include       <er.h>

/**
** Name:	GTdraw.c -	component drawing routines
**
** Description:
**	C-Chart calls for drawing components.  This file defines:
**
**	NOTE:
**		unless we are actually making graphics calls, the
**		terminal is left in alpha mode.	 All of these routines
**		must set graphics, draw, and set alpha before exiting
**		unless G_idraw is set, indicating a call from inside
**		another GT routine - already in graph mode.
**
**		If G_idraw is set, meaning some other GT routine has called
**		us, we also do not call the .scr file dump routines - higher
**		level code is covering the dump.
**
**		If G_restrict is on, we preface any drawing with GTrsbox(FALSE)
**		to remove the corner marks from the screen.  Although multiple
**		GTrsbox calls are harmless, we might as well make sure we only
**		do them once per external entry point.
**
**	GTclear()	clear graphics from screen
**	GTrefresh()	draw entire current list of components
**	GTdlayer()	draw a specific layer of components
**	GTdmark()	draw all marked components
**	GTdraw()	draw a specific component
**
** History:
**		86/02/27  10:31:49  wong
**		Major changes:	support for boxed axes; support for cross axis;
**		use of 'GTfillrep()'; use of 'G_restrict' flag.
**
**		86/03/06  17:15:59  wong
**		Set bar outline color with respect to both
**		fill and background color.
**
**		86/03/07  18:05:02  wong
**		Changes to LABEL structure.
**
**		86/03/10  16:56:03  wong
**		Max. C-Chart preview level of 3 for monochrome devices.
**		Also, bar clustering moved to 'GTdataset()'.
**
**		86/03/10  19:17:03  bobm
**		grids behind data, line/scatter charts on TOP of axis,
**		set pie stem/extender color same as labels, set bullet
**		color same as text color.
**
**		86/03/11  17:56:31  wong
**		Support axis line visibility.
**
**		86/03/12  14:14:29  bobm
**		seperate hatch / color
**
**		86/03/13  12:27:24  bobm
**		individual pie slice settings
**
**		86/03/13  16:11:36  wong
**		Whether an axis is crossed does not depend on the
**		existence of the crossing axis.
**
**		86/03/17  14:17:57  wong
**		Fixed drawing of charts with boxed axes after re-ordering
**		of drawing; always draw boxed axes after plotting data.
**
**		86/03/20  17:48:18  bobm
**		set axis attributes for bar charts, whether the axis is drawn or
**		not.  This seems to need to be done to reliably get attributes
**		set on horizontal bar charts.
**
**		86/03/24  18:19:11  bobm
**		G_internal -> G_idraw
**
**		86/03/24  20:15:33  bobm
**		don't force axes on stacked bar charts
**
**		86/04/07  18:04:17  bobm
**		Don't erase extents for bottom layer on total redraw.
**
**		86/04/07  18:10:28  bobm
**		We can set No_1 in GTrefresh for initial drawing, too.
**
**		86/04/15  14:45:12  bobm
**		fix up transparency optimization - don't do it if !G_interactive
**
**		86/04/17  10:37:57  bobm
**		set tick mark color, check for background color
**		rather than BLACK on line chart fill
**
**		86/04/18  14:46:43  bobm
**		Non-fatal error handling during drawing
**
**		86/04/21  14:02:29  bobm
**		enforce MIN_MARGIN in cmrg.
**		reorder precedence of transparent / background color.
**		allow background color on colored hatch preview level.
**
**		86/04/26  15:47:21  bobm
**		call GTderr for deferred GKS errors
**
**		6/3/86 (bab)	fix for bug 9032.  if axis labels are
**			reversed, mislead Cchart as to location of tick
**			marks (outside vs. inside) so that VE code will
**			really draw the tick marks where the user wants them.
**		7/11/86 (bab)	fix for bug 8650.  if axis type is date,
**			use the label attributes for the header settings
**			since cchart ignores the header text in this case.
**		7/15/86 (bab)	fix for bugs 9118, 9181.  if dated axis,
**			don't set either the grid or the tick frequency.
**			Those values are set automatically, and setting
**			them manually overrides the proper values.
**		8/5/86 (bab)	fix for bug 9117.  in ax_set(), use
**			existence of axis for determining whether to
**			make it visible (logical), but set axis
**			attributes anyway; this is because for horiz.
**			bar charts, attributes of the 'partner' axis
**			are used to draw the current axis (and don't
**			want to draw using cchart default values).
**		03/16/88 (dkh) - Added EXdelete call to conform to coding spec.
**		02/22/89 (Mike S) 
**			DG's markers are quite big enough.  Also, default to 
**			MK_STAR for undefined marker types.
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	01-oct-91 (jillb/jroy--DGC)
**	    make function declaration consistent with its definition
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototypes for ax_cross(), pie_draw(), bar_draw(),
**	  line_draw(), ax_draw, ax_box(), margin_get() & fb_check()
**	  to prevent compiler errors with GCC 4.0 due to conflict with
**	  implicit declaration.
**/

extern GR_FRAME *G_frame;	/* Graphics frame structure */

extern Gws	*G_ws;		/* GKS workstation pointer (referenced) */

extern bool G_idraw;
extern bool G_interactive;
extern bool G_plotref;
extern bool	G_restrict;	/* Graphics restrict locator flag */
extern Gint	GTmap_axis[4];

static bool ax_cross();
static bar_set();
static line_set();
static scat_set();
static mark_set();
static pie_set();
static leg_set();
static trim_set();
static ax_set();
static pie_draw();
static bar_draw();
static line_draw();
static ax_draw();
static ax_box();
static margin_get();
static fb_check();

extern i4  (*G_mfunc) ();

extern i4  G_reflevel;

static bool No_1;	/* no need to clear extents in bottom layer */

/*{
** Name:	GTclear - clear graphics screen
**
** Description:
**	clears graphics from display.  FRS "clear screen" does not erase
**	graphics on some devices.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Side Effects:
**	none, although we may not currently realize the problem if
**	some device driver clears text as well, since this is currently
**	called only on leaving a form.
**
** History:
**	8/85 (rlm)	written
*/

void
GTclear ()
{
	GTdmpclear ();
	(*G_mfunc)(GRAPHMODE);
	No_1 = TRUE;
	gclrwk (G_ws,ALWAYS);
	(*G_mfunc)(ALPHAMODE);
}

/*{
** Name:	GTrefresh	- draw all components
**
** Description:
**	draws the entire list of visible components, ie. the entire
**	graphics screen.
**	The current layer may be less than the number of layers that exist,
**	in which case some components on the list are not drawn.  This
**	check is not used if we are non-interactive
**
**	This
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Side Effects:
**
** History:
**	8/85 (rlm)	written
*/

void
GTrefresh()
{
	GR_OBJ *ptr;
	i4 sv_level;
	void GTdlayer();
	void GTdraw();

	/* no "internal" calls to GTrefresh */
	GTdmpframe(G_frame);
	(*G_mfunc)(GRAPHMODE);
	if (G_restrict)
		GTrsbox(FALSE);
	G_plotref = FALSE;
	G_idraw = TRUE;

	/* if we're "refreshing" we must have a clear screen */
	No_1 = TRUE;

	/* reset preview level if "restrict" on */
	if (G_interactive && G_restrict)
	{
		sv_level = G_frame->preview;
		G_frame->preview = G_reflevel;
	}

	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
	{
		if (G_interactive)
		{
			if (ptr->layer > G_frame->clayer)
				continue;
			if (G_restrict && ptr == G_frame->cur)
				continue;
		}
		GTdraw(ptr);
	}

	No_1 = FALSE;

	/* restore preview level if "restrict" on, draw current component */
	if (G_interactive && G_restrict)
	{
		G_frame->preview = sv_level;
		GTdraw(G_frame->cur);
	}

	(*G_mfunc)(ALPHAMODE);
	G_idraw = FALSE;
}

/*{
** Name:	GTdlayer	- draw all components on a given layer
**
** Description:
**	draws all components on a given layer.	They will be drawn in
**	order from the list, although this doesn't matter.
**
** Inputs:
**	l	layer to draw
**
** Outputs:
**	none
**
** Side Effects:
**
** History:
**	8/85 (rlm)	written
*/

void
GTdlayer (l)
i4  l;		/* layer to draw */
{
	GR_OBJ *ptr;
	void GTdraw();
	bool oldmode;

	oldmode = G_idraw;
	if (!G_idraw)
	{
		GTdmplayer (G_frame,l);
		(*G_mfunc)(GRAPHMODE);
		if (G_restrict)
			GTrsbox(FALSE);
	}
	G_idraw = TRUE;

	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
		if (ptr->layer == l)
			GTdraw (ptr);

	G_idraw = oldmode;
	if (!G_idraw)
	{
		No_1 = FALSE;
		(*G_mfunc)(ALPHAMODE);
	}
}

/*{
** Name:	GTdbunch	- draw overlapping component group
**
** Description:
**	draws all components in an overlapping "glob" between two
**	given layers, inclusive.  The "glob" is defined as those components
**	in the layer range which overlap a given extent, or recursively
**	overlap a "glob" member.
**
** Inputs:
**	ext	EXTENT defining intial overlap check
**	low	lower layer to include, inclusive
**	hi	upper layer to include, inclusive
**
** Outputs:
**	none
**
** Side Effects:
**
** History:
**	8/85 (rlm)	written
*/

void
GTdbunch (ext,low,hi)
EXTENT *ext;		/* initial overlap extent */
i4  low,hi;		/* range of layers */
{
	void GTdmark();
	bool oldmode;

	oldmode = G_idraw;
	if (!G_idraw)
	{
		(*G_mfunc)(GRAPHMODE);
		if (G_restrict)
			GTrsbox(FALSE);
	}
	G_idraw = TRUE;

	GTmrkclr ();
	GTmrkb (ext,low,hi);
	if (!oldmode)
		GTdmpmark(G_frame);
	GTdmark ();

	G_idraw = oldmode;
	if (!G_idraw)
	{
		No_1 = FALSE;
		(*G_mfunc)(ALPHAMODE);
	}
}

/*{
** Name:	GTdmark - draw marked components
**
** Description:
**	draws components marked by a non-zero "scratch" item
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Side Effects:
**
** History:
**	8/85 (rlm)	written
*/

void
GTdmark ()
{
	GR_OBJ *ptr;
	void GTdraw();
	bool oldmode;

	oldmode = G_idraw;
	if (!G_idraw)
	{
		GTdmpmark (G_frame);
		(*G_mfunc)(GRAPHMODE);
		if (G_restrict)
			GTrsbox(FALSE);
	}
	G_idraw = TRUE;

	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
		if (ptr->scratch != 0)
			GTdraw (ptr);

	G_idraw = oldmode;
	if (!G_idraw)
	{
		No_1 = FALSE;
		(*G_mfunc)(ALPHAMODE);
	}
}

/*{
** Name:	GTdraw	- draw a component
**
** Description:
**	draws a graphics component
**
**	CAUTION: exception handler set / deleted in this routine.
**
** Inputs:
**	c	component to draw
**
** Outputs:
**	none
**
** History:
**	8/85 (rlm)	written
**	2mar89 (Mike S)	Fix to bug 4426
**			Call GTclrerr to clear old error.
*/

void
GTdraw (c)
GR_OBJ *c;
{
	Gwrect vw;
	bool oldmode;
	COLOR bck;
	EXTENT drw;
	Gint pvw;
	EX_CONTEXT context;
	i4 FEjmp_handler();

	/*
	** NOTE:
	**	since we are setting an exception handler, remember to
	**	EXdelete before any "normal" returns
	*/
	if (EXdeclare(FEjmp_handler,&context) != RTIOK)
	{
		EXdelete();
		G_idraw = FALSE;
		return;
	}

	GTclrerr();
	oldmode = G_idraw;
	if (!G_idraw)
	{
		GTdmpcomp(c);
		(*G_mfunc)(GRAPHMODE);
		if (G_restrict)
			GTrsbox(FALSE);
	}
	G_idraw = TRUE;

	GTreset ();

	/* C-Chart's preview levels are actually 0 - 4. */
	pvw = G_frame->preview - 1;
	if (pvw > 2 && G_cnum <= 2)
		pvw = 2;

	/*
	** it is important that we set background color back to black
	** for monochrome devices.  If we try to fill with color >= 1
	** on these devices, we "whiteout" the entire box.
	*/
	bck = pvw >= 3 ? c->bck : BLACK;

	fb_check(c,bck);

	if (G_wline && ((G_frame->gomask | c->goflgs) & GRF_BORDER) != 0)
		GToutbox(&(c->ext));

	if (c->type != GROB_TRIM && c->type != GROB_LEG)
		GTxetodrw(&(c->ext), &drw);
	else
	{
		drw.xlow = drw.ylow = 0.0000001;
		drw.xhi = drw.yhi = .9999999;
	}

	vw.ur.x = drw.xhi;
	vw.ur.y = drw.yhi;
	vw.ll.x = drw.xlow;
	vw.ll.y = drw.ylow;

	cvwpt(&vw);
	caspect(G_ws, (Gfloat)G_aspect);
	cprev(pvw);

	GTdataset (c);

	switch (c->type)
	{

	case GROB_PIE:
		pie_set ((PIECHART *) c->attr, &(c->margin));
		pie_draw ((PIECHART *) c->attr);
		break;

	case GROB_BAR:
		bar_set((BARCHART *) c->attr, &(c->margin), c->bck);
		bar_draw((BARCHART *) c->attr, &c->ext, &c->margin);
		break;

	case GROB_LINE:
		line_set((LINECHART *) c->attr, &(c->margin), c->bck);
		line_draw((LINECHART *) c->attr, &c->ext, &c->margin);
		break;

	case GROB_SCAT:
		scat_set((SCATCHART *) c->attr, &(c->margin));
		line_draw((LINECHART *) c->attr, &c->ext, &c->margin);
		break;

	case GROB_LEG:
		leg_set((LEGEND *) c->attr, &(c->ext), &(c->margin), c->bck);

		/* so simple we don't bother with a "draw" call */
		clgon(TRUE);
		cdlgd();
		break;

	case GROB_TRIM:
		trim_set((GR_TEXT *) c->attr, &(c->ext), &(c->margin));

		/* so simple we don't bother with a "draw" call */
		cdmsg(0);
		break;

	default:
		if (c->type != GROB_NULL)
			GTsyserr (S_GT0015_Bad_object,0);
		break;
	}

	if (!G_wline && ((G_frame->gomask | c->goflgs) & GRF_BORDER) != 0)
		GToutbox(&(c->ext));

	G_idraw = oldmode;
	if (!G_idraw)
	{
		No_1 = FALSE;
		(*G_mfunc)(ALPHAMODE);
	}

	/* remove exception handler */
	EXdelete ();
	GTderr ();
}

static
bar_set (bar, marg, bck)
register BARCHART	*bar;
MARGIN			*marg;
COLOR			bck;
{
	register Gint	i;

	Glnbundl	*cqdtln();
	Ctxbd		*cqilbtx();

	ax_set(bar->ax, marg);

	for (i = 0 ; i < bar->ds_num ; ++i)
	{
		register LABEL		*blp = &bar->lab[i];
		register DEPDAT		*bdp = &bar->ds[i];
		register Glnbundl	*lbd;

		/* Set bar fill and outline */
		GTfillrep(i, -1, &(bdp->data));
		lbd = cqdtln(i);
		lbd->colour = bdp->data.color == bck ? (bck != BLACK ? BLACK : WHITE)
						: X_COLOR(bdp->data.color);
		cdtln(i, lbd);

		/* Set bar label */
		if (blp->l_place == 0)
			cilbon(i, ALL, FALSE);
		else
		{
			register Ctxbd	*txptr;

			cilbon(i, ALL, TRUE);
			txptr = cqilbtx(i, 0);
			txptr->clr = X_COLOR(blp->l_clr);
			txptr->fnt = blp->l_font;
			cilbtx(i, ALL, txptr);
			cilbpl(i, ALL, (Gint) blp->l_place);
			cilbf(i, blp->l_fmt);
		}
	}	/* end for */

	cbase((Gfloat) bar->baseline);
}

static
line_set (line, marg,bck)
register LINECHART	*line;
MARGIN		*marg;
COLOR		bck;
{
	register Gint	i;

	Glnbundl	*cqdtln();

	ax_set(line->ax, marg);

	for (i=0; i < line->ds_num; ++i)
	{
		register Glnbundl	*lbd;

		lbd = cqdtln(i);
		lbd->colour = X_COLOR((line->ds)[i].data.color);
		lbd->type = (line->ds)[i].ds_dash + 1;
		cdtln(i, lbd);
	}

	mark_set(line->ds, line->ds_num);

	if (line->line_fill)
	{
		for (i=0; i < line->ds_num; ++i)
		{
			if ((line->ds)[i].fill.color != bck)
			{
				if (i == 0)
					ccfl(0, F_BELOW);
				else
				{
					ccfl(i, F_BETWEEN);
					csflb(i, i-1);
				}
				GTfillrep(i, -1, &((line->ds)[i].fill));
			}
		}	/* end for */
	}
}

static
scat_set (scat, marg)
SCATCHART *scat;
MARGIN *marg;
{
	ax_set(scat->ax, marg);
	mark_set(scat->ds, scat->ds_num);
}

static
mark_set (ds, num)
register DEPDAT ds[];
i4		num;
{
	register Gint	i;

	Gmkbundl	*cqdtmk();

	for (i = 0 ; i < num ; ++i)
	{
		register DEPDAT  *ds_p = &ds[i];
		register Gmktype mtype = ds_p->ds_mark;

		if (mtype <= 0)
			cdtmkon(i, FALSE);
		else
		{	/* set marker for dataset */
			register Gmkbundl	*bundl;
			
			cdtmkon(i, TRUE);
			bundl = cqdtmk(i);
			bundl->type = (mtype <= G_mnum) ? mtype : MK_STAR;
			bundl->colour = X_COLOR(ds_p->data.color);
# ifdef DGC_AOS
			bundl->size = 1.0;	/* standard sized markers */
# else
			bundl->size = 1.5;	/* 1 & 1/2 times default size */
# endif
			cdtmk(i, bundl);
		}
	}
}

static
pie_set (pie, marg)
register PIECHART	*pie;
register MARGIN		*marg;
{
	register i4	i;
	float dx,dy;
	Gfloat rad;
	Gwpoint loc;
	Glnbundl *lbd;
	i4 len;
	Glnbundl *cqilptln();

	Ctxbd	*cqilbtx();

	/*
	** use margins to determine location and radius for pie
	** also use explosion factor and allow a fixed factor for labels
	*/
	dx = (1.0 - marg->left - marg->right)/2.0;
	dy = (1.0 - marg->top - marg->bottom)/2.0;
	loc.x = marg->left + dx;
	loc.y = marg->bottom + dy;
	cpiloc(0, &loc);
	rad = min(dx, dy) * (1.0 - pie->exfact/2.0);
	if (pie->lab.l_place != 0)
		rad *= .7;
	if (rad <= 0.05)
		rad = 0.05;
	cpirad(0, rad);

	cpian(0, (Gfloat)pie->rot);
	if (pie->exfact > 0.009)
	{
		for (i = pie->exstart ; i <= pie->exend ; ++i)
			ciexp(0, i, (Gfloat)pie->exfact);
	}
	if (pie->lab.l_place == 0)
		cilbon(0, ALL, FALSE);
	else
	{
		register Ctxbd	*txptr;

		txptr = cqilbtx(0, 0);
		lbd = cqilptln(0, 0);

		lbd->colour = txptr->clr = X_COLOR(pie->lab.l_clr);
		txptr->fnt = pie->lab.l_font;
		cilbtx(0, ALL, txptr);
		cilbon(0, ALL, TRUE);
		cilbf(0, pie->lab.l_fmt);
		ciptln(0, ALL, lbd);
		ciln(0, ALL, lbd);
	}
	len = GTdslen(0);
	for (i=0; i < len; ++i)
		GTfillrep(0, i, pie->slice + i);
}

static
leg_set (leg, ext, mg, bck)
LEGEND *leg;
EXTENT *ext;
MARGIN *mg;
COLOR bck;
{
	Gwpoint xy;
	Gtxalign al;
	Gint i;
	Ctxbd *txbd, *cqlgtltx(), *cqdtlgtx();
	float dx, dy;
	EXTENT drw;
	Glnbundl *lbd;
	GR_OBJ *lgobj;
	DEPDAT *lgds;
	Glnbundl	*cqdtln();

	GTxetodrw (ext,&drw);

	/* legend title */
	clgtlst(leg->title);

	txbd = cqlgtltx();
	txbd->clr = X_COLOR(leg->ttl_color);
	txbd->fnt = leg->ttl_font;
	if (txbd->fnt == ASCIIFT)
		txbd->ht = 0.009;
	clgtltx(txbd);

	/* legend items */
	for (i=0; i < leg->items; ++i)
	{
		txbd = cqdtlgtx(i);
		txbd->clr = X_COLOR(leg->txt_color);
		txbd->fnt = leg->txt_font;
		cdtlgtx(i, txbd);
	}

	/*
	** set marks, line colors, etc for associated datasets
	** NOTE: THIS IS NOT, REPEAT NOT setup by the chart setup routine
	** We have to make these calls again for the legend, because
	** the legend was entered as a separate group of datasets. 
	** This was done to avoid restrictions on legend placement.
	*/
	lgobj = (GR_OBJ *) leg->assoc;
	if (lgobj != NULL)
	{
		switch (lgobj->type)
		{
		case GROB_SCAT:
			lgds = ((SCATCHART *)lgobj->attr)->ds;
			mark_set(lgds, leg->items);
			break;
		case GROB_LINE:
			lgds = ((LINECHART *)lgobj->attr)->ds;
			for (i=0; i < leg->items; ++i)
			{
				lbd = cqdtln(i);
				lbd->colour = X_COLOR(lgds[i].data.color);
				lbd->type = lgds[i].ds_dash + 1;
				cdtln(i, lbd);
			}
			mark_set(lgds, leg->items);
			break;
		case GROB_BAR:
			lgds = ((BARCHART *)lgobj->attr)->ds;
			for (i = 0 ; i < leg->items; ++i)
			{
				/* Set bar fill and outline */
				GTfillrep(i, -1, &(lgds[i].data));
				lbd = cqdtln(i);
				lbd->colour = lgds[i].data.color == bck ? (bck != BLACK ? BLACK : WHITE)
						: X_COLOR(lgds[i].data.color);
				cdtln(i, lbd);
			}
			break;
		default:
			break;
		}
	}

	clgfon(TRUE);	/* force absolute legend size */

	al.hor = TH_LEFT;
	al.ver = TV_BOTTOM;
	clgal(&al);

	/* legend location */
	dx = drw.xhi - drw.xlow;
	dy = drw.yhi - drw.ylow;
	xy.x = drw.xlow + dx * mg->left;
	xy.y = drw.ylow + dy * mg->bottom;
	clgloc(&xy);

	/* legend size */
	xy.x = dx * (1.0 - mg->left - mg->right);
	xy.y = dy * (1.0 - mg->top - mg->bottom);
	clgsz(&xy);
}

static
trim_set (trim, ext, mg)
GR_TEXT *trim;
EXTENT *ext;
MARGIN *mg;
{
	Gtxalign al;
	Ctxbd *txbd,*cqmsgtx();
	Gwpoint xy;
	float dx, dy;
	EXTENT drw;
	Gflbundl *bltfrep;
	Gflbundl *cqblfl();
	Glnbundl *bltlrep;
	Glnbundl *cqblln();

	GTxetodrw (ext,&drw);

	cmsgst (0,trim->text);

	txbd = cqmsgtx (0);
	txbd->fnt = trim->font;
	txbd->clr = X_COLOR(trim->ch_color);
	cmsgtx (0,txbd);

	bltfrep = cqblfl();
	bltfrep->colour = X_COLOR(trim->ch_color);
	cblfl(bltfrep);
	bltlrep = cqblln();
	bltlrep->colour = X_COLOR(trim->ch_color);
	cblln(bltlrep);

	cmsgadj (0,TRUE);

	al.hor = TH_LEFT;
	al.ver = TV_BOTTOM;
	cmsgal (0,&al);

	dx = drw.xhi - drw.xlow;
	dy = drw.yhi - drw.ylow;
	xy.x = drw.xlow + dx * mg->left;
	xy.y = drw.ylow + dy * mg->bottom;
	cmsgloc (0,&xy);

	xy.x = dx * (1.0 - mg->left - mg->right);
	xy.y = dy * (1.0 - mg->top - mg->bottom);
	cmsgsz (0,&xy);
}

static
ax_set (ax, marg)
register AXIS	*ax;
MARGIN		*marg;
{
	register i4	i;
	Cmarg		mg;
	bool		dated_axis;

	Ctxbd		*cqaxlbtx(),
			*cqaxhdtx();
	Gflbundl	*cqaxhdfl();
	Glnbundl	*cqaxln(),
			*cqgln(),
			*cqtkln();
	Gbool		cqaxrev();

	mg.left = ax[LEFT_AXIS].ax_exists ? ax[LEFT_AXIS].margin : marg->left;
	mg.right = ax[RIGHT_AXIS].ax_exists ? ax[RIGHT_AXIS].margin : marg->right;
	mg.top = ax[TOP_AXIS].ax_exists ? ax[TOP_AXIS].margin : marg->top;
	mg.bottom = ax[BTM_AXIS].ax_exists ? ax[BTM_AXIS].margin : marg->bottom;

	if (mg.left < MIN_MARGIN)
		mg.left = MIN_MARGIN;
	if (mg.right < MIN_MARGIN)
		mg.right = MIN_MARGIN;
	if (mg.top < MIN_MARGIN)
		mg.top = MIN_MARGIN;
	if (mg.bottom < MIN_MARGIN)
		mg.bottom = MIN_MARGIN;

	cmrg(&mg);

	for (i = 0 ; i < 4 ; ++i)
	{
		register AXIS		*axis = &ax[i];
		register Gint		id = GTmap_axis[i];
		register Glnbundl	*ln;

		/* determine/set axis visibility */
		if (! axis->ax_exists)
			caxon(id, FALSE);
		else
			caxon(id, TRUE);

		/*
		** set all axis attributes whether visible or not
		** (changed for bug 9117)
		*/
		caxrev(id, ((axis->a_flags & AX_REVERSE) != 0));

		if (axis->ax_date)
			dated_axis = TRUE;	/* date data */
		else
			dated_axis = FALSE;

		/* Set axis line */
		if (axis->ax_ln_invis)
			caxlnon(id, FALSE);	/* no axis line */
		else
		{
			ln = cqaxln(id);
			ln->colour = X_COLOR(axis->g_clr);
			if (!ax_cross(ax, i))
				caxln(id, ln);
			else
			{	/* cross axis */
				static Gint cross_map[4] = {
						CROSSY, /* GLEFT */
						CROSSY, /* GRIGHT */
						CROSSX, /* GUP */
						CROSSX	/* GDOWN */
				};

				register Gint	xory = cross_map[id];

				ccsaxis(xory, TRUE);
				ccsln(xory, ln);
				caxlnon(id, FALSE);	/* no axis line */
			}
		}

		/* Set axis labels */
		caxlbon(id, (Gint)axis->ax_labels.lb_supr);
		if (axis->ax_labels.lb_supr != NO_LABS)
		{	/* axis label attributes */
			register Ctxbd	*bptr;

			caxcntr(id, ((axis->a_flags & AX_L_CENTER) != 0));

			bptr = cqaxlbtx(id);
			bptr->clr = X_COLOR(axis->ax_labels.lb_clr);
			bptr->fnt = axis->ax_labels.lb_font;
			caxlbtx(id, bptr);
		}

		if ((axis->a_flags & AX_GRID) == 0)
			cgon(id, FALSE);
		else
		{
			cgon(id, TRUE);
			/* b9118. dated axes set their own grid freq */
			if (!dated_axis)
				cgfq(id, (Gfloat)axis->grids);
			ln = cqgln(id);
			ln->colour = X_COLOR(axis->g_clr);
			ln->type = axis->ax_g_dash + 1;
			cgln(id, ln);
		}

		if (axis->tick_loc == AXT_NONE)
			ctkon(id, TK_MAJOR, FALSE);
		else
		{	/* axis ticks */
			u_i2	tickloc;

			ctkon(id, TK_MAJOR, TRUE);
			/*
			** fix for bug 9032.
			** if the axis labels are reversed,
			** fool VE code about the location of the
			** tick marks (outside vs. inside)
			** so that the tick marks will be drawn
			** where the user wants them, not where
			** VE wants them.
			*/
			tickloc = axis->tick_loc;
			if (cqaxrev(id))
			{
				if (tickloc == AXT_OUT)
					tickloc = AXT_IN;
				else if (tickloc == AXT_IN)
					tickloc = AXT_OUT;
			}
			ctkknd(id, TK_MAJOR, (Gint) tickloc);
			/* b9118. dated axes set their own tick freq */
			if (!dated_axis)
				ctkfq(id,
					TK_MAJOR, (Gfloat)axis->ticks);
			if (id == GLEFT || id == GRIGHT)
				/* better size than C-Chart default */
				ctksz(id, TK_MAJOR, 0.01);
			ln = cqtkln(id, TK_MAJOR);
			ln->colour = X_COLOR(axis->g_clr);
			ctkln(id, TK_MAJOR, ln);
		}

		/*
		** do nothing with axis header attributes if
		** header is being suppressed and NOT a dated
		** axis, OR if labels are being suppressed and
		** it IS a dated axis.
		** Part of fix for bug 8650
		*/
		if ((((axis->a_flags & AX_HD_SUP) != 0) && !dated_axis)
			|| ((axis->ax_labels.lb_supr == NO_LABS)
				&& dated_axis))
		{	/* no axis header */
			caxhdon(id, FALSE);
			caxhdbon(id, FALSE);
		}
		else
		{	/* axis header */
			register Ctxbd	*bptr;

			caxhdst(id, axis->ax_header.hd_text);

			bptr = cqaxhdtx(id);
			/* Part of fix for bug 8650 */
			if (!dated_axis)	/* normal header */
			{
				bptr->clr =
				    X_COLOR(axis->ax_header.hd_t_clr);
				bptr->fnt = axis->ax_header.hd_font;
			}
			else	/* use label attributes */
			{
				bptr->clr =
				    X_COLOR(axis->ax_labels.lb_clr);
				bptr->fnt = axis->ax_labels.lb_font;
			}
			caxhdtx(id, bptr);

			caxhdon(id, TRUE);
			if (((axis->a_flags & AX_HDB_SUP) != 0)
				|| dated_axis)	/* for bug 8650 */
			{
				caxhdbon(id, FALSE);
			}
			else
			{	/* axis header background */
				register Gflbundl	*bundl;

				caxhdbon(id, TRUE);
				bundl = cqaxhdfl(id);
				bundl->colour = X_COLOR(axis->ax_header.hd_b_clr);
				caxhdfl(id, bundl);
			}

			/* do only for real hdrs.  for bug 8650 */
			if (!dated_axis)
			{
				switch (axis->ax_header.hd_just)
				{
				case AXHJ_LEFT:
					caxhdjs(id, LEFTJUST);
					break;
				case AXHJ_CENTER:
					caxhdjs(id, CENTERJUST);
					break;
				default:
					caxhdjs(id, RIGHTJUST);
					break;
				}
				if (id == GLEFT || id == GRIGHT)
				{
					/*
					** set axis header orientation/
					**   text path left or right
					**   only.  Set path 1st, then
					**   orientation as follows due
					**   to C-Chart bug.
					**
					**   DOWNPATH, HORIZ	parallel
					**   NORMALPATH, VERT	parallel
					**   NORMALPATH, HORIZ	perp.
					**   DOWNPATH, VERT	perp.
					*/
					caxhdpt(id, ((axis->a_flags & AX_HD_DOWN) != 0) ? DOWNPATH : NORMALPATH);
					if ((axis->a_flags & AX_HD_DOWN) != 0)
						caxhdor(id, ((axis->a_flags & AX_HD_PERP) == 0) ? HORIZ : VERT);
					else
						caxhdor(id, ((axis->a_flags & AX_HD_PERP) != 0) ? HORIZ : VERT);
				}
			}
		}
	}	/* end for */
}

static bool
ax_cross (axes, axis)
register AXIS	axes[];
register i4	axis;
{
	static u_i1	cross_axes[4][2] = {
				{BTM_AXIS, TOP_AXIS},	/* for LEFT */
				{BTM_AXIS, TOP_AXIS},	/* for RIGHT */
				{LEFT_AXIS, RIGHT_AXIS},/* for TOP */
				{LEFT_AXIS, RIGHT_AXIS} /* for BOTTOM */
	};

	register i4	i;

	for (i = 0 ; i < 2 ; ++i)
	{
		register AXIS	*ap = &axes[cross_axes[axis][i]];

		if ((ap->a_flags & AX_CROSS) != 0 &&
				!ap->ax_date && !ap->ax_string)
			return TRUE;
	}

	return FALSE;
}

static
pie_draw (pie)
PIECHART *pie;
{
	cdpie(0);
	if (pie->lab.l_place != 0)
		cdplb(0);
}

static
bar_draw (bar, bc_ext, bc_mrg)
register BARCHART	*bar;
EXTENT			*bc_ext;
MARGIN			*bc_mrg;
{
	register u_i4	i;

	/* check grids first, in order to draw underneath bars */
	for (i=0; i < 4; ++i)
	{
		register AXIS	*axis = &bar->ax[i];

		if (axis->ax_exists && (axis->a_flags & AX_GRID) != 0)
		{
			register Gint	id = GTmap_axis[i];

			cdgrd(id);
			cgon(id, FALSE);
		}
	}

	for (i=0; i < bar->ds_num; ++i)
		cdb((Gint) i);
	ax_draw(bar->ax);
	/* Draw boxed axes after plotting data! */
	if (bar->box_axes)
		ax_box(bar->ax, bc_ext, bc_mrg);
}

static
line_draw (line, lc_ext, lc_mrg)
register LINECHART	*line;
EXTENT			*lc_ext;
MARGIN			*lc_mrg;
{
	register u_i4	i;

	ax_draw(line->ax);
	for (i=0; i < line->ds_num; ++i)
		cdcrv((Gint) i);
	/* Draw boxed axes after plotting data! */
	if (line->box_axes)
		ax_box(line->ax, lc_ext, lc_mrg);
}

static
ax_draw (axes)
register AXIS	axes[];
{
	register i4	i;

	for (i = 0 ; i < 4 ; ++i)
		if (axes[i].ax_exists)
			cdax(GTmap_axis[i]);
}

static
ax_box (axes, ob_ext, ob_mrg)
register AXIS	axes[];
register EXTENT *ob_ext;	/* object extents */
MARGIN		*ob_mrg;
{
	Cmarg	mg;
	f8	xscale;
	f8	yscale;
	EXTENT	axes_ext;

	margin_get(axes, ob_mrg, &mg);
	xscale = ob_ext->xhi - ob_ext->xlow;
	yscale = ob_ext->yhi - ob_ext->ylow;
	axes_ext.xlow = ob_ext->xlow + (mg.left * xscale);
	axes_ext.xhi = ob_ext->xhi - (mg.right * xscale);
	axes_ext.ylow = ob_ext->ylow + (mg.bottom * yscale);
	axes_ext.yhi = ob_ext->yhi - (mg.top * yscale);
	GToutbox(&axes_ext);
}

static
margin_get (axes, margins, mrg)
register AXIS	axes[4];
register MARGIN *margins;
register Cmarg	*mrg;
{
# ifdef Power6
	/* CCI Power 6 generates bad assembler expression
	** for "double" at offset 0 from 'mrg'
	*/
	Gfloat	temp;
	temp = axes[RIGHT_AXIS].ax_exists ? axes[RIGHT_AXIS].margin
						: margins->right;
	mrg->right = temp;
# else
	mrg->right = axes[RIGHT_AXIS].ax_exists ? axes[RIGHT_AXIS].margin
						: margins->right;
# endif
	mrg->left = axes[LEFT_AXIS].ax_exists ? axes[LEFT_AXIS].margin
						: margins->left;
	mrg->top = axes[TOP_AXIS].ax_exists ? axes[TOP_AXIS].margin
						: margins->top;
	mrg->bottom = axes[BTM_AXIS].ax_exists ? axes[BTM_AXIS].margin
						: margins->bottom;
}

/*
** fill box check.  Do the background color box / erase if needed
*/
static fb_check(c,bck)
GR_OBJ *c;
COLOR bck;
{

	/* can't fill backgrounds on "indelible" devices */
	if (G_indelible)
		return;

	/* if "transparent" forget it */
	if (((G_frame->gomask | c->goflgs) & GRF_CLEAR) == 0)
		return;

	/* if black background, repress the fill on empty screen */
	if (bck == BLACK)
	{
		/* drawing on clear screen and interactive and bottom layer */
		if (No_1 && G_interactive && c->layer == 0)
			return;
	}

	GTflbox(&(c->ext), bck);
}
