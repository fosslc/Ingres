/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<st.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<nm.h>
# include	<si.h>
# include	<lo.h>
# include	<graf.h>
# include	<dvect.h>
# include	"ergt.h"
# include	<cv.h>

extern GR_FRAME *G_frame;

extern FILE *fddmpfile;

# define	PERIOD '.'	/* for %f formats - internal use, no multi.h */

/*
** Argument types for scr_plot
*/
# define	SCRP_FRM 0 /* entire frame */
# define	SCRP_CMP 1 /* single component */
# define	SCRP_LYR 2 /* single layer */
# define	SCRP_MCMP 3 /* marked components */

# define	FP		fddmpfile /* shorthand notation for lazyness */
# define	CFORM		ERx("%5.3f") /* format to use for coordinates */
# define	PFXTEMP		ERx("vgscr")
# define	SCRDRIVER	ERx("printer")

static char	*Tempname = NULL;
static LOCATION Tloc;
static u_i4	D_mask;
static char	*Errpfx = ERx("!!!!G: ERROR IN SCR FILE - ");
static char	*Normpfx = ERx("%%G: ");

/*
**	All of the GTdmp routines are bracketed with a check for NULL
**	FP.  Letting all the calling routines reach these hooks so that
**	we have the reference to fddmpfile, and dump decisions in one
**	source file, but individual checks in each to avoid unneeded
**	formatting and further procedure calls seems to be a decent
**	compromise.  They also check D_mask to see if they are activated
**	or not, each GTdmp routine having a bit.  GTdmpinit is the
**	exception to these rules, being the routine which sets the mask,
**	allowing the defaults to be overidden through a variable setting.
**
** History:
**    25-Oct-2005 (hanje04)
**	  Add generic history.
**        Add prototype for oneliner(), e_oneliner(), scr_plot(),
**	  plot_tname(), dmp_struct(), dmp_st1(), dmp_obj(), dmp_bar(),
**	  dmp_scline(), dmp_pie(), dmp_leg(), dmp_trield(), dmp_axes(),
**	  dmp_farr(), dmp_iarr() & dmp_sarr() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/

/*
**	Activation bits for D_mask, and default setting.
*/

# define	AB_box		1	/* box drawings */
# define	AB_clear	2	/* screen clear */
# define	AB_frame	4	/* frame refresh */
# define	AB_mark		8	/* marked redraws */
# define	AB_layer	0x10	/* layer redraws */
# define	AB_erase	0x20	/* component erase */
# define	AB_comp		0x40	/* individual component draw */
# define	AB_msg		0x80	/* menu line messages */
# define	AB_stat		0x100	/* status line messages */
# define	AB_vlist	0x200	/* data vector list */

# define	AB_DEFAULT AB_box|AB_clear|AB_frame|AB_msg|AB_stat|AB_vlist

/* local prototypes */
static i4 oneliner();
static i4 e_oneliner();
static i4 scr_plot();
static i4 plot_tname();
static i4 dmp_struct();
static i4 dmp_st1();
static i4 dmp_obj();
static i4 dmp_bar();
static i4 dmp_scline();
static i4 dmp_pie();
static i4 dmp_leg();
static i4 dmp_trield();
static i4 dmp_axes();
static i4 dmp_farr();
static i4 dmp_iarr();
static i4 dmp_sarr();

/*
**	utility to return a string representing a box size
**	1 string - overwrites.
**
**	6/6/86 (bab)	- due to bug in the STprintf routine,
**		unable to properly process all 4 floating
**		point numbers (the last gets trashed).	So,
**		until bug is fixed, use fs1, fs2, fs3 instead
**		of fs, and perform a STcat.  When bug is fixed,
**		use the commented out code instead.
**	26-Aug-1993 (fredv)
*8		Included <st.h>.
*/
static char *
box_str (ext)
EXTENT *ext;
{
	static char	bufr[48];
	/*char		fs[48];*/
	char		fs1[30];
	char		fs2[30];
	char		fs3[30];

	STprintf (fs1, ERx("Extent (x:%s %s "), CFORM, CFORM);
	STprintf (fs2, ERx("y:%s %s)"), CFORM, CFORM);
	STprintf (bufr, fs1, ext->xlow, PERIOD, ext->xhi, PERIOD);
	STprintf (fs3, fs2, ext->ylow, PERIOD, ext->yhi, PERIOD);
	_VOID_ STcat (bufr, fs3);

	/*
	** Use this code when the aforementioned bug is fixed.
	**
	STprintf (fs, "Extent (x:%s %s y:%s %s)", CFORM, CFORM, CFORM, CFORM);
	STprintf (bufr, fs, ext->xlow, PERIOD, ext->xhi, PERIOD,
				ext->ylow, PERIOD, ext->yhi, PERIOD);
	*/
	return (bufr);
}

GTdmpinit ()
{
	char	*mask;
	STATUS	rc;

	NMgtAt(ERx("II_GR_SCRMASK"), &mask);
	if (mask != (char *)NULL && *mask != '\0')
	{
		if (CVal(mask, &D_mask) == OK)
			return;
		e_oneliner(ERx("II_GR_SCRMASK non-numeric (%s)"), mask);
	}
	D_mask = AB_DEFAULT;
}

GTdmpfbox(e, col)
EXTENT	*e;
i4	col;
{
	if (FP != NULL && (D_mask & AB_box))
		oneliner(ERx("FILLED BOX %s, color %d"), box_str(e), col);
}

GTdmphbox(e)
EXTENT	*e;
{
	if (FP != NULL && (D_mask & AB_box))
		oneliner(ERx("BOX OUTLINE %s"), box_str(e));
}

GTdmpclear ()
{
	if (FP != NULL && (D_mask & AB_clear))
		oneliner(ERx("GRAPHICS CLEAR SCREEN"));
}

GTdmpframe(frame)
GR_FRAME	*frame;
{
	GR_OBJ	*ptr;

	if (FP != NULL && (D_mask & AB_frame))
	{
		oneliner(ERx("FRAME REFRESH"));
		scr_plot(SCRP_FRM);
	}
}

GTdmpmark(frame)
GR_FRAME	*frame;
{
	GR_OBJ	*ptr;

	if (FP != NULL && (D_mask & AB_mark))
	{
		oneliner(ERx("PARTIAL REDRAW"));
		scr_plot(SCRP_MCMP);
	}
}

GTdmplayer(frame, layer)
GR_FRAME	*frame;
i4		layer;
{
	GR_OBJ	*ptr;

	if (FP != NULL && (D_mask & AB_layer))
	{
		oneliner(ERx("REDRAW LAYER %d"), layer);
		scr_plot(SCRP_LYR, layer);
	}
}

GTdmperase()
{
	if (FP != NULL && (D_mask & AB_erase))
	{
		oneliner(ERx("ERASE"));
		scr_plot(SCRP_FRM);
	}
}

GTdmpcomp(c)
GR_OBJ	*c;
{
	if (FP != NULL && (D_mask & AB_comp))
	{
		oneliner(ERx("REDRAW COMPONENT"));
		scr_plot(SCRP_CMP, 0, c);
	}
}

GTdmpmsg (s)
char	*s;
{
	if (FP != NULL && (D_mask & AB_msg))
		oneliner(ERx("MESSAGE: %s"), s);
}

GTdmpstat (s)
char	*s;
{
	if (FP != NULL && (D_mask & AB_stat))
		oneliner(ERx("STATUS: %s"), s);
}

GTdmpvlist ()
{
	GR_DVEC *dv;
	GR_DVEC *GTdvscan();

	if (FP != NULL && (D_mask & AB_vlist))
	{
		oneliner(ERx("DATA VECTORS"));
		for (dv = GTdvscan((GR_DVEC *) NULL); dv != NULL; dv = GTdvscan(dv))
		{
			SIfprintf(FP, ERx("\n%s vector \"%s\", type X%x\n"), Normpfx,
					dv->name, dv->type);
			switch(dv->type & DVEC_MASK)
			{
			case DVEC_I:
				dmp_iarr(dv->ar.idat, dv->len);
				break;
			case DVEC_F:
				dmp_farr(dv->ar.fdat, dv->len);
				break;
			case DVEC_S:
				dmp_sarr(&(dv->ar.sdat), dv->len);
				break;
			}
		}
	}
}

static
oneliner(fmt, a, b, c, d, e, f)
char	*fmt;
i4	a, b, c, d, e, f;
{
	SIfprintf(FP, ERx("\n%s"), Normpfx);
	SIfprintf(FP, fmt, a, b, c, d, e, f);
	SIfprintf(FP, ERx("\n"));
}

static
e_oneliner(fmt, a, b, c, d, e, f)
char	*fmt;
i4	a, b, c, d, e, f;
{
	SIfprintf(FP, ERx("\n%s"), Errpfx);
	SIfprintf(FP, fmt, a, b, c, d, e, f);
	SIfprintf(FP, ERx("\n"));
}

static
scr_plot(type, iarg, comp)
i4	type;
i4	iarg;
GR_OBJ	*comp;
{
	FILE	*oldfp;

	/*
	** very important to prevent circular calls here.
	** we temporarily make FP null to prevent the graphics plot calls
	** from calling further dump routines.
	*/

	dmp_struct(type, iarg, comp);
	if (Tempname == NULL)
		plot_tname();
	oldfp = FP;
	FP = NULL;
	if (GTchange_ws(SCRDRIVER, Tempname) != OK)
		GTsyserr(S_GT0018_ws_open,0);
	switch (type)
	{
	case SCRP_FRM:
		GTrefresh();
		break;
	case SCRP_CMP:
		GTdraw(comp);
		break;
	case SCRP_LYR:
		GTdlayer(iarg);
		break;
	case SCRP_MCMP:
		GTdmark();
		break;
	}
	FP = oldfp;
	GTrestore_ws();
	SIcat(&Tloc, FP);
	LOdelete (&Tloc);
}

static
plot_tname()
{
	static char	loc_buf[MAX_LOC + 1];

	/*
	**	This is using a side effect of LOuniq() to fill in loc_buf.
	*/
	LOfroms(PATH & FILENAME, loc_buf, &Tloc);
	LOuniq(PFXTEMP, ERx("tmp"), &Tloc);
	LOtos(&Tloc, &Tempname);
}

static
dmp_struct(type, iarg, comp)
i4	type;
i4	iarg;
GR_OBJ	*comp;
{
	register GR_OBJ *ptr;
	register i4	i;

	i = 0;

	switch (type)
	{
	case SCRP_FRM:
		for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
		{
			++i;
			dmp_st1(ptr, i);
		}
		break;
	case SCRP_LYR:
		for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
		{
			++i;
			if (ptr->layer == iarg)
				dmp_st1(ptr, i);
		}
		break;
	case SCRP_MCMP:
		for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
		{
			++i;
			if (ptr->scratch != 0)
				dmp_st1(ptr, i);
		}
		break;
	case SCRP_CMP:
		dmp_st1(comp, -1);
		break;
	default:
		e_oneliner(ERx("dmp_struct type %d"), type);
		break;
	}
}

static
dmp_st1(ptr, idx)
GR_OBJ	*ptr;
i4	idx;
{
	GR_TEXT *txt;

	if (idx > 0)
		SIfprintf(FP, ERx("%scomponent %d on chain: "), Normpfx, idx);

	switch (ptr->type)
	{
	case GROB_BAR:
		SIfprintf(FP, ERx("BARCHART\n"));
		dmp_obj(ptr);
		dmp_bar(ptr->attr);
		break;
	case GROB_LINE:
		SIfprintf(FP, ERx("LINECHART\n"));
		dmp_obj(ptr);
		dmp_scline(ptr->attr);
		break;
	case GROB_PIE:
		SIfprintf(FP, ERx("PIECHART\n"));
		dmp_obj(ptr);
		dmp_pie(ptr->attr);
		break;
	case GROB_SCAT:
		SIfprintf(FP, ERx("SCATTERCHART\n"));
		dmp_obj(ptr);
		dmp_scline(ptr->attr);
		break;
	case GROB_LEG:
		SIfprintf(FP, ERx("LEGEND\n"));
		dmp_obj(ptr);
		dmp_leg(ptr->attr);
		break;
	case GROB_TRIM:
		txt = (GR_TEXT *) ptr->attr;
		if ((txt->name) != NULL && *(txt->name) != '\0')
			SIfprintf(FP, ERx("FIELD\n"));
		else
			SIfprintf(FP, ERx("TRIM\n"));
		dmp_obj(ptr);
		dmp_trield(ptr->attr);
		break;
	default:
		SIfprintf(FP, ERx("UNKNOWN COMPONENT (%d)\n"), ptr->type);
		dmp_obj(ptr);
	}
}

static
dmp_obj (obj)
GR_OBJ	*obj;
{
	SIfprintf(FP, ERx("\t%s\n"), box_str(&(obj->ext)));

	SIfprintf(FP, ERx("\tMargins: Left "));
	SIfprintf(FP, CFORM, obj->margin.left, PERIOD);
	SIfprintf(FP, ERx(", Right "));
	SIfprintf(FP, CFORM, obj->margin.right, PERIOD);
	SIfprintf(FP, ERx(", Top "));
	SIfprintf(FP, CFORM, obj->margin.top, PERIOD);
	SIfprintf(FP, ERx(", Bottom "));
	SIfprintf(FP, CFORM, obj->margin.left, PERIOD);
	SIfprintf(FP, ERx("\n"));

	switch (obj->goflgs & (GRF_CLEAR|GRF_BORDER))
	{
	case 0:
		SIfprintf(FP, ERx("\tTransparent, Unboxed\n"));
		break;
	case GRF_BORDER:
		SIfprintf(FP, ERx("\tTransparent, Boxed\n"));
		break;
	case GRF_CLEAR:
		SIfprintf(FP, ERx("\tOpaque, Unboxed\n"));
		break;
	default:
		SIfprintf(FP, ERx("\tOpaque, Boxed\n"));
		break;
	}

	SIfprintf(FP, ERx("\tBackground color: %d\n"), obj->bck);
	if (obj->legend != NULL)
		SIfprintf(FP, ERx("\tLegend associated\n"));
}

static
dmp_bar (bar)
BARCHART	*bar;
{
	SIfprintf(FP, ERx("\tds_num = %d, ds_set = %d\n"), bar->ds_num, bar->ds_set);
	if (bar->bar_horizontal)
		SIfprintf(FP, ERx("\tHorizontal,"));
	else
		SIfprintf(FP, ERx("\tVertical,"));
	if (bar->box_axes)
		SIfprintf(FP, ERx(" Boxed axes,"));
	SIfprintf(FP, ERx(" Cluster index: %d\n"), bar->cluster);
	dmp_axes(bar->ax);
	dmp_ds(bar->ds, bar->ds_num, MAX_B_DS);
}

static
dmp_scline (scl)
LINECHART	*scl;
{
	SIfprintf(FP, ERx("\tds_num = %d, ds_set = %d\n"), scl->ds_num, scl->ds_set);
	if (scl->line_fill)
		SIfprintf(FP, ERx("\tFilled"));
	else
		SIfprintf(FP, ERx("\tUnfilled"));
	if (scl->box_axes)
		SIfprintf(FP, ERx(", Boxed axes\n"));
	else
		SIfprintf(FP, ERx("\n"));
	dmp_axes(scl->ax);
	dmp_ds(scl->ds, scl->ds_num, MAX_L_DS);
}

static
dmp_pie (pie)
PIECHART	*pie;
{
	i4	i;

	SIfprintf(FP, ERx("\tRotation: %3.1f, Explosion: %5.3f - slices %d to %d\n"),
			pie->rot, PERIOD, pie->exfact, PERIOD,
			pie->exstart, pie->exend);
	dmp_lab(&(pie->lab), 1, 1);
	dmp_ds(&(pie->ds), 1, 1);
	for (i=0; i < MAX_SLICE; ++i)
		SIfprintf(FP, ERx("\tSlice %d: color %d, hatch %d\n"), i,
				(pie->slice)[i].color, (pie->slice)[i].hatch);
}

static
dmp_leg (leg)
LEGEND	*leg;
{
	if (leg->title != NULL && *(leg->title) != '\0')
		SIfprintf(FP, ERx("\tTitle: %s, "), leg->title);
	else
		SIfprintf(FP, ERx("\tNO TITLE, "));
	if (leg->assoc != NULL)
		SIfprintf(FP, ERx("associated with type %d\n"), (leg->assoc)->type);
	else
		SIfprintf(FP, ERx("Unassociated\n"));
	SIfprintf(FP, ERx("\tColor: title %d, text %d.	Font: title %d, text %d\n"),
				leg->ttl_color, leg->txt_color,
				leg->ttl_font, leg->txt_font);
}

static
dmp_trield(txt)
GR_TEXT *txt;
{
	SIfprintf(FP, ERx("\tText: %s\n"), txt->text);
	if (txt->column != NULL && *(txt->column) != '\0')
		SIfprintf(FP, ERx("\tColumn: %s\n"), txt->column);
	if (txt->name != NULL && *(txt->name) != '\0')
		SIfprintf(FP, ERx("\tInternal Name: %s\n"), txt->name);
}

static
dmp_axes(ax)
AXIS	*ax;
{
	i4	i;
	char	*p;

	SIfprintf(FP, ERx("\n\tAXES (normal LEFT,RIGHT,TOP,BOTTOM)\n"));
	for (i=0; i < 4; ++i)
	{
		if (ax->ax_exists)
			SIfprintf(FP, ERx("\n\tPRESENT\n"));
		else
			SIfprintf(FP, ERx("\n\tNOT PRESENT\n"));
		SIfprintf(FP, ERx("\tMargin: %5.3f\n"), ax->margin, PERIOD);
		SIfprintf(FP, ERx("\tFlags: X%x\n"), ax->a_flags);
		SIfprintf(FP,
			ERx("\tRange:%18.6g to%18.6g by %18.6g, precision %d\n"),
			ax->r_low, PERIOD, ax->r_hi, PERIOD, ax->r_inc,
			PERIOD, ax->ax_labels.lb_prec);
		SIfprintf(FP, ERx("\tTicks: %5.3f per step, location %d\n"),
			ax->ticks, PERIOD, ax->tick_loc);
		SIfprintf(FP, ERx("\tGrids: %5.3f per step, type %d\n"), ax->grids,
			PERIOD, ax->ax_g_dash);
		SIfprintf(FP, ERx("\tLine Color: %d\n"), ax->g_clr);
		p = ax->ax_header.hd_text;
		if (!(ax->a_flags & AX_HD_SUP) && p != NULL && *p != '\0')
			SIfprintf(FP, ERx("\tHeader: %s\n"), p);
		SIfprintf(FP, ERx("\tHeader font, color: %d %d (background %d)\n"),
				ax->ax_header.hd_font, ax->ax_header.hd_b_clr,
				ax->ax_header.hd_t_clr);
		SIfprintf(FP, ERx("\tHeader justification: %d\n"),
				ax->ax_header.hd_just);
		SIfprintf(FP, ERx("\tLabel font, color: %d %d\n"),
				ax->ax_labels.lb_font, ax->ax_labels.lb_clr);
		++ax;
	}
}

dmp_ds (ds, nact, tot)
DEPDAT	*ds;
i4	nact, tot;
{
	i4	i;

	SIfprintf(FP,
		ERx("\n\tDATASETS -  strings : fillreps : connection mark dash\n"));

	for (i=0; i < tot; ++i)
	{
		if (i < nact)
		{
			SIfprintf(FP, ERx("\t%d - X=%s Y=%s D=%s :"), i+1,
					ds->xname, ds->field, ds->desc);
		}
		else
		{
			SIfprintf(FP, ERx("\t%d - :"), i+1);
		}
		SIfprintf(FP, ERx(" F(%d,%d) D(%d,%d) :"),
			ds->fill.color, ds->fill.hatch,
			ds->data.color, ds->data.hatch);
		SIfprintf(FP, ERx(" C%d, M%d, D%d\n"),
			ds->ds_cnct, ds->ds_mark, ds->ds_dash);
		++ds;
	}
}

dmp_lab (lb, nact, tot)
LABEL	*lb;
i4	nact, tot;
{
	i4	i;

	SIfprintf(FP, ERx("\n\tDATA LABELS - strings : font color\n"));

	for (i=0; i < tot; ++i)
	{
		if (i < nact)
		{
			SIfprintf(FP, ERx("\t%d - F=%s V=%s :"),
				i+1, lb->l_fmt, lb->l_field);
		}
		else
		{
			SIfprintf(FP, ERx("\t%d - :"), i+1);
		}
		SIfprintf(FP, ERx("F%d, C%d\n"), lb->l_font, lb->l_clr);
		++lb;
	}
}

static
dmp_farr(f, len)
float	*f;
i4	len;
{
	i4	i, j;

	for (i=0; i < len; i += 6)
	{
		SIfprintf(FP, ERx("%s"), Normpfx);
		j = len - i;
		if (j > 6)
			j = 6;
		for ( ; j > 0; j--)
		{
			SIfprintf(FP, ERx("%18.6g"), *f, PERIOD);
			++f;
		}
		SIfprintf(FP, ERx("\n"));
	}
}

static
dmp_iarr(ia, len)
i4	*ia;
i4	len;
{
	i4	i, j;

	for (i=0; i < len; i += 10)
	{
		SIfprintf(FP, ERx("%s"), Normpfx);
		j = len - i;
		if (j > 10)
			j = 10;
		for ( ; j > 0; j--)
		{
			SIfprintf(FP, ERx("%9d"), *ia);
			++ia;
		}
		SIfprintf(FP, ERx("\n"));
	}
}

static
dmp_sarr(sv, len)
DV_SVAL *sv;
i4	len;
{
	i4	i, j;
	i4	*x;
	char	**s;

	x = sv->val;
	s = sv->str;

	for (i=0; i < len; i += 4)
	{
		SIfprintf(FP, ERx("%s"), Normpfx);
		j = len - i;
		if (j > 4)
			j = 4;
		for ( ; j > 0; j--)
		{
			SIfprintf(FP, ERx("\"%s\"=%d "), *s, *x);
			++s;
			++x;
		}
		SIfprintf(FP, ERx("\n"));
	}
}
