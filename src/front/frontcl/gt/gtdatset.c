/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<er.h>
# include	<iicommon.h>
# include	"gkscomp.h"
# include	<graf.h>
# include	<dvect.h>
# include	"gtdev.h"
# include	<gtdef.h>
# include	"ergt.h"
# include	<gl.h>
# include	<fe.h>

/**
** Name:	gtdatset.c -	Graphics System Data Set Module.
**
** Description:
**	Contains routines that deal with the data sets for a graph.
**
**	GTdtchop()
**	GTdataset()
**	GTmap_axis[]	map axis to id array.
**	GTdsdestroy()
**	GTdslen()
**
** History:
**	86/02/27  10:42:04  wong
**	Chart flags are now bit fields.	 Also, now uses 'GTfillrep()'.
**
**	86/03/06  17:16:48  wong
**	Set bar outline color with respect to both
**	fill and background color.
**
**	86/03/10  16:23:19  bobm
**	put in bar cluster call before autoscaling axis.
**
**	86/03/11  19:24:10  wong
**	Use bit fields for axis data and string flags.
**
**	86/03/14  17:25:41  bobm
**	changes to axis scaling for bar chart problems with various combinations
**	of axes.  This still doesn't work correctly, but now you can at least
**	get properly drawn bar charts with no axes in 1 or both dimensions.
**
**	86/03/17  13:33:50  bobm
**	more fiddling with axis / dataset association - create artificial
**	dataset if only 1 being setup, in case there are multiple axes
**
**	86/03/20  17:49:48  bobm
**	scale ALL axes on bar charts
**
**	86/03/21  17:08:08  bobm
**	fill in current axis range / increments on autoscale.
**
**	86/03/24  20:14:45  bobm
**	don't force axes on stacked bar charts
**
**	86/04/10  17:57:09  bobm
**	changes to axis / dataset assignment to fix floating bars / linechart
**	fill opposing numeric axis mismatches.
**
**	86/04/11  12:17:40  bobm
**	Forced in dataset for bars only zeros on stacked chart
**
**	86/04/14  11:15:53  bobm
**	fix index in "add_pair" when creating fake bars.
**
**	86/04/16  16:53:45  bobm
**	fixes for axis tracking on horizontal bar charts - ALWAYS
**	use left / bottom for master axes.
**
**	86/05/02  15:13:05  peterk
**	In legend set-up, if data series has a mark set, show mark in legend
**	(rather than line) by adding data series as CV_SCATTER.
**
**	7/16/86 (bruceb) fix for bugs 9226 (modification of parallel axes),
**		9181 (tiny labels on large intervals of dated axes),
**		9118 (dated axis tick marks not always on days),
**		X (precision of numeric labels, particularly on auto. axes)
**	9/12/86 (bruceb) use Pairtab[MAX_DS - 1] rather than Pairtab[0]
**		for the dummy dataset; Pairtab is filled in from the top
**		down rather than from the bottom up.
**	27-oct-86 (bruceb) fix for bug 10660
**		added some externs for cchart query functions, and changed
**		some f4's to f8's.
**	6-nov-86 (bruceb) Fix for bug 10739
**		Assign datasets to legend as BAR type when bar chart
**		(logical!?) rather than checking for marks.  This
**		avoids display of a legend with marks when doing a
**		ChangeType to bar.
**	20-may-88 (bruceb)
**		Change FEtalloc to FEreqmem.
**	6/88 (bobm)
**		Add GTnewdat().  This is called from map_frame in graflib,
**		takes care of instances where we assign data to a graph,
**		and never actually draw it (happens in 6.0).  Keeps us
**		from running out of space in add_pairs(), and having
**		crashes in GTdsdestroy due to Pairtab not being initialized.
**	8/88 (bobm)
**		Put in Fakelen, Fakeds to prevent the wrong dataset being
**		used to assign the "fake" dataset.  Also reset Xaxlen, Yaxlen
**		on the assignement of a new set of strings.
**      27-sept-88 (kevinm)
**              Added a call to cdtknd after each call to cadds.  This is a 
**              work around for a bug in cadds.
**	21-Feb-89 (Mike S)
**		Added calls to cdtflon to re-enable filled areas for line 
**		charts, which cdtknd disables.
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	1/91 (Mike S) - Initialize Xaxlen and Yaxlen once in GTnewdat, instead 
**			of doing for each dataset in make_pair.  This avoids
**			blanking out valid labels when a later dataset has gaps.
**			Bug 34462.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**      24-sep-96 (mcgem01)
**              Global data moved to gtdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for add_pair(), make_pair(), ax_scale(), ax_master(),
**	  ax_slave(), ax_type(), init_pairs(), add_lab() & ax_prec() to prevent
**	  compiler errors with GCC 4.0 due to conflict with implicit
**	  declaration.
**	21-Aug-2009 (kschendel) 121804
**	    Need fe.h to satisfy gcc 4.3.
**/

/* local prototypes */
static add_pair();
static make_pair();
static ax_scale();
static ax_master();
static ax_slave();
static ax_type();
static init_pairs();
static add_lab();
static ax_prec();

/*
** used in ax_prec() as arbitrary values (subject to change) in
** decision of a label precision. (bab)
*/
# define	MINCUTOFF	0.0001
# define	MAXCUTOFF	0.9999
# define	MAXPREC		6

/*
** DATAPAIR structure links two datavectors together as a dataset for use
** with cchart.	 We allocate a maximum number of these, using the "call"
** item to release old entries when we need a new pair.
*/
typedef struct
{
	char *xname,*yname;	/* vector names */
	i4 xtype,ytype;	/* vector types */
	char **xlab,**ylab;	/* for string types */
	Gwpoint *ds;		/* cchart dataset pointer */
	Gint len;		/* length of data */
	Gint dlen;		/* length given to cadds */
	i2 tag;			/* storage tag for data, <0 for unused entry */
	i4 call;		/* if != Call, OK to construct new pair */
} DATAPAIR;

static DATAPAIR Pairtab [MAX_DS];	/* existing datasets */
static DATAPAIR *Ds_info [MAX_DS];	/* dataset info by index */
static i4	Call = 0;		/* counter of data remappings */
static Gwpoint	Fk_bars [MAX_BAR];

/*
** to make cchart axes work right we need to add a "fake" dataset.  This
** winds up being a duplicate of the first one we added.
*/
static Gint Fakelen;
static Gwpoint *Fakeds;

extern i4  G_datchop;

/*
**	Axis label arrays.  The datavectors for string type axes have
**	data = 1 - n.	We use the data to fill in the appropriate label.
*/
static char *Yaxlab[MAX_XSTRINGS];
static char *Xaxlab[MAX_XSTRINGS];
static i4  Yaxlen=0,Xaxlen=0;

/*
** Name:	GTmap_axis -	Graphics System Map Axis to Id Array.
**
** Description:
**	This array maps the Graphics System axis number to a C-Chart ID
**	number.
*/

GLOBALREF Gint	GTmap_axis[4]; 

void
GTdtchop (len)
i4  len;
{
	G_datchop = len;
}


void
GTnewdat()
{
	if (Call <= 0)
		init_pairs();
	++Call;
	Xaxlen = Yaxlen = 0;
}

void
GTdataset (c)
GR_OBJ	*c;
{
	Glnbundl	*cqdtln();
	Gmkbundl	*cqdtmk();

	switch (c->type)
	{

	case GROB_PIE:
	{
		register PIECHART	*pie = (PIECHART *)c->attr;

		add_pair(pie->ds.field, pie->ds.field, 0, PIE, MAX_SLICE, (AXIS *) NULL);
		if (pie->lab.l_place != 0)
			add_lab(0, pie->lab.l_field);
		break;
	}

	case GROB_BAR:
	{
		register BARCHART	*bar = (BARCHART *)c->attr;
		register i4		i;

		/* if bar chart is horizontal, reverse data */
		cbror(bar->bar_horizontal ? HORIZ : VERT);

		for (i=0; i < bar->ds_num; ++i)
		{
			register DEPDAT *bds = &bar->ds[i];
			add_pair(bds->xname, bds->field, i, BAR, MAX_BAR, bar->ax);
			if (bar->lab[i].l_place != 0)
				add_lab(i, (bar->lab)[i].l_field);
		}

		/*
		** NOTE: "cluster" item is the "cluster index" for cchart
		**	which causes clustered if ZERO, stacked if 1.
		**	(>1 allows mixtures).  This makes tests look
		**	backwards, ie. if(bar->cluster) is true for
		**	a stacked bar chart.  Oh well.
		**
		**	we add a fake dataset with zero height bars if we're
		**	going to stack, a fake dataset which duplicates the
		**	first, if we're going to cluster.  In this case, we
		**	make its bars zero-width so as to not affect the
		**	ones we're really going to draw.
		*/
		if (bar->cluster)
			cadds (bar->ds_num, BAR, Fakelen, Fk_bars);
		else
			cadds (bar->ds_num, BAR, Fakelen, Fakeds);
		cdtknd(bar->ds_num, BAR); /* cadds bug work around */

		cbrcls(bar->cluster);

		if (! bar->cluster)
			cdtbrwd(bar->ds_num,0.0);

		ax_scale(bar->ax, BAR, bar->ds_num, bar->bar_horizontal);
		break;
	}

	case GROB_LINE:
	{
		register LINECHART	*line = (LINECHART *)c->attr;
		register i4		i;

		for (i=0; i < line->ds_num; ++i)
		{
			static i4	xc_type[] = {
						CV_STRAIGHT,	/* XC_STRAIGHT */
						CV_STEP,	/* XC_STEP */
						CV_BSPLINE,	/* XC_BSPLINE */
						CV_QSPLINE,	/* XC_QSPLINE */
						CV_REGRESS,	/* XC_REGRESS */
						CV_CROSS,	/* XC_CROSS */
						CV_SCATTER	/* XC_NONE */
					};

			register DEPDAT *lds = &line->ds[i];

			add_pair(lds->xname, lds->field, i,
				xc_type[lds->ds_cnct], MAX_POINTS, line->ax);
		}
		cadds (line->ds_num, CV_STRAIGHT, Fakelen, Fakeds);
		cdtknd(line->ds_num, CV_STRAIGHT); /* cadds bug work around */
		cdtflon(line->ds_num, TRUE);
		ax_scale(line->ax, CURVE, line->ds_num, FALSE);
		break;
	}

	case GROB_SCAT:
	{
		register SCATCHART	*scat = (SCATCHART *)c->attr;
		register i4		i;

		for (i=0; i < scat->ds_num; ++i)
		{
			register DEPDAT *sds = &scat->ds[i];

			add_pair(sds->xname,sds->field,i,CV_SCATTER,MAX_POINTS,scat->ax);
		}
		cadds (scat->ds_num, CV_SCATTER, Fakelen, Fakeds);
		cdtknd(scat->ds_num, CV_SCATTER); /* cadds bug work around */
		ax_scale(scat->ax, CURVE, scat->ds_num, FALSE);
		break;
	}

	case GROB_LEG:
	{
		register LEGEND *leg = (LEGEND *)c->attr;
		GR_OBJ		*lgobj = (GR_OBJ *)leg->assoc;

		if (lgobj == NULL)
			leg->items = 0;
		else
		{
			register i4	i;
			DEPDAT		*lgds;
			i4		ltype;

			switch (lgobj->type)
			{
			case GROB_SCAT:
				ltype = CV_SCATTER;
				leg->items = ((SCATCHART *)lgobj->attr)->ds_num;
				lgds = ((SCATCHART *)lgobj->attr)->ds;
				break;
			case GROB_LINE:
				ltype = CURVE;
				leg->items = ((LINECHART *)lgobj->attr)->ds_num;
				lgds = ((LINECHART *)lgobj->attr)->ds;
				break;
			case GROB_BAR:
				ltype = BAR;
				leg->items = ((BARCHART *)lgobj->attr)->ds_num;
				lgds = ((BARCHART *)lgobj->attr)->ds;
				break;
			case GROB_PIE:
				ltype = PIE;
				leg->items = 1;
				lgds = &((PIECHART *)lgobj->attr)->ds;
				break;
			}
			for (i=0; i < leg->items; ++i)
			{
				register DEPDAT		*lds = &lgds[i];
				i4			xtype;

				/* fix for bug 10739 */
				if ((ltype == CV_SCATTER) ||
				    (ltype == CURVE && lds->ds_mark != 0))
				{
					xtype = CV_SCATTER;
				}
				else	/* BAR, or line chart w/ no marks */
				{
					xtype = ltype;
				}
				add_pair(lds->xname, lds->field, i, xtype, 2,
					(AXIS *) NULL);
				cdtlgst(i, lds->desc);
				GTfillrep(i, -1, &(lds->data));

				/* Fix for Bug 11717 */
								
				if (ltype == BAR)
				{
					register Glnbundl	*bbd;

					bbd = cqdtln(i);
					bbd->colour = lds->data.color == c->bck
						? (c->bck != BLACK ? BLACK : WHITE)
						: X_COLOR(lds->data.color);
			}
				else if (ltype == CURVE)
				{
					register Gmkbundl	*mbd;
					register Glnbundl	*lbd;

					mbd = cqdtmk(i);
					mbd->type = lds->ds_mark;
					mbd->size = 1.5;
					mbd->colour = X_COLOR(lds->data.color);

					lbd = cqdtln(i);
					lbd->type = lds->ds_dash + 1;
					lbd->colour = X_COLOR(lds->data.color);
		}
				else if (ltype == CV_SCATTER && lds->ds_mark > 0)
				{
					register Gmkbundl	*mbd;

					mbd = cqdtmk(i);
					mbd->type = lds->ds_mark;
					mbd->size = 1.5;
					mbd->colour = X_COLOR(lds->data.color);
				}

				/* end Fix 11717 */

			}
		}
		break;
	}	/* end case LEGEND */

	default:
		break;
	}	/* end switch */
}

/*
** GTdsdestroy is used from GTdvec routines to remove any datasets existing
** for a vector being removed.
*/
GTdsdestroy (name)
char *name;
{
	i4 i,count;

	/*
	** if Call <= 0, than GTdataset(), and hence init_pairs(),
	** has never been called, and therefore, the Pairtab[x].tag's
	** contain garbage.  prevents AV; fix for bug 11563
	*/
	if (Call > 0)
	{
	for (count=i=0; i < MAX_DS; ++i)
	{
		if (Pairtab[i].tag < 0)
			continue;
		++count;
		if (STcompare(name,Pairtab[i].xname) == 0
				|| STcompare(name,Pairtab[i].yname) == 0)
		{
			FEfree((i2)Pairtab[i].tag);
			Pairtab[i].tag = -1;
		}
	}
	}
	if (count == 0)
		Yaxlen = Xaxlen = 0;
}

/*
** GTdslen returns the length of a dataset by number, using the Ds_info
** table which was set up by GTdataset through add_pair.
*/
GTdslen(ds)
i4  ds;
{
	return ((Ds_info[ds])->dlen);
}

/*
** add_pair makes calls to cadds.  It looks through Pairtab for the
** correct dataset, creating it if it doesn't exist.  If a new pair is
** created, a null node will be filled in if possible.	Otherwise, an
** entry with "call" item != Call (an old dataset) will be used.  The
** "lim" argument applies purely to the cadds call.  If more data than
** this is in the vectors, we set it up anyway, in case we want it for
** another type of chart.  Since this is where we examine the types of
** the data vectors, we also set the type flags for axes.  G_datchop is also
** applied to the cadds call.  This is the value used to facilitate
** truncation of data from the editor.
*/
static
add_pair (xn,yn,ds_idx,type,lim,ax)
char *xn;		/* x name */
char *yn;		/* y name */
i4  ds_idx;		/* cchart index for dataset */
i4  type;		/* cchart curve type */
i4  lim;		/* cadds limit */
AXIS *ax;		/* AXIS array to set type flags in, may be NULL */
{
	register i4	i;
	i4 repidx;	/* replacement index if pair not found */

	/* try to find pair, point repidx to suitable node for new pair */
	repidx = -1;
	for (i=0; i < MAX_DS; ++i)
	{
		if (Pairtab[i].tag < 0)
		{
			repidx = i;
			continue;
		}
		if (STcompare(Pairtab[i].xname,xn) == 0 &&
				STcompare(Pairtab[i].yname,yn) == 0)
			break;
		if (repidx < 0 && Pairtab[i].call != Call)
			repidx = i;
	}

	/* if failure, build repidx dataset */
	if (i >= MAX_DS)
	{
		if (repidx < 0)
			GTsyserr(S_GT0004_DS_too_many, 0);
		make_pair (xn,yn,repidx);
		i = repidx;
	}

	/* point cchart to dataset */
	Pairtab[i].call = Call;
	if (lim > G_datchop)
		lim = G_datchop;
	if (Pairtab[i].len < lim)
		lim = Pairtab[i].len;
	Ds_info[ds_idx] = Pairtab + i;
	cadds (ds_idx, type, (Pairtab[i].dlen = lim), Pairtab[i].ds);
	cdtknd(ds_idx, type); /* cadds bug work around */
	if (type >= CURVE && type != CV_SCATTER )
		cdtflon(ds_idx, TRUE);

	/*
	** if dataset 0, set pointers for adding "fake" dataset
	*/
	if (ds_idx == 0)
	{
		Fakelen = lim;
		Fakeds = Pairtab[i].ds;
	}

	/* axis flags  - set on first dataset */
	if (ax != NULL && ds_idx == 0)
	{
		register i4	j;

		/* create zero-height bars dataset */
		if (type == BAR)
		{
			for (j = 0; j < lim; ++j)
			{
				Fk_bars[j].x = (Pairtab[i].ds)[j].x;
				Fk_bars[j].y = 0.0;
			}
		}

		for (j = 0 ; j < 4 ; ++j)
		{
			register AXIS	*ap = &ax[j];

			i4	chktype;

			if (j == LEFT_AXIS || j == RIGHT_AXIS)
			{
				chktype = Pairtab[i].ytype;
			}
			else
			{
				chktype = Pairtab[i].xtype;
			}
			ap->ax_date = ap->ax_string = FALSE;
			if (chktype == GRDV_DATE)
				ap->ax_date = TRUE;
			else if (chktype == GRDV_STRING)
				ap->ax_string = TRUE;
		}
	}
}

/*
** get datavectors and build a dataset.	 If a vector is string data,
** 0, 1, 2 ... dummy data is inserted.	This routine also allocates
** space for new data, and frees old.
*/
static
make_pair (xn, yn, idx)
char *xn;
char *yn;
i4  idx;
{
	register GR_DVEC	*xv,*yv;		/* data vectors */
	register DATAPAIR	*pt = &Pairtab[idx];	/* data pair */
	register i4		i, len;

	GR_DVEC *GTdvfetch();

	if ((xv = GTdvfetch(xn)) == NULL || (yv = GTdvfetch(yn)) == NULL)
		GTsyserr(S_GT0005_DS_no_vectors, 2, xn, yn);

	if (pt->tag >= 0)
		FEfree((i2)pt->tag);
	pt->tag = FEgettag();

	pt->xname = xn;
	pt->yname = yn;

	len = pt->len = min(xv->len, yv->len);
	if ((pt->ds = (Gwpoint *)FEreqmem((u_i4)pt->tag,
		(u_i4)(len * sizeof(Gwpoint)), (bool)FALSE,
		(STATUS *)NULL)) == NULL)
	{
		GTsyserr (S_GT0006_DS_pair_alloc, 0);
	}

	/* now transfer data */
	pt->ytype = yv->type;
	switch (pt->ytype & DVEC_MASK)
	{
	case DVEC_S:
		pt->ylab = yv->ar.sdat.str;
		for (i=0; i < len; ++i)
		{
			register i4	j;

			j = (yv->ar.sdat.val)[i];
			(pt->ds)[i].y = j;
			--j;
			if (j >= 0 && j < MAX_XSTRINGS)
			{
				for ( ; Yaxlen < j; ++Yaxlen)
					Yaxlab[Yaxlen] = ERx("");
				Yaxlab[j] = (yv->ar.sdat.str)[i];
				if (Yaxlen <= j)
					Yaxlen = j+1;
			}
		}
		break;
	case DVEC_I:
		for (i=0; i < len; ++i)
			(pt->ds)[i].y = (yv->ar.idat)[i];
		break;
	case DVEC_F:
		for (i=0; i < len; ++i)
			(pt->ds)[i].y = (yv->ar.fdat)[i];
		break;
	default:
		GTsyserr(S_GT0007_DS_bad_Y_type, 0);
	}	/* end switch */
	pt->xtype = xv->type;
	switch (pt->xtype & DVEC_MASK)
	{
	case DVEC_S:
		pt->xlab = xv->ar.sdat.str;
		for (i=0; i < len; ++i)
		{
			register i4	j;

			j = (xv->ar.sdat.val)[i];
			(pt->ds)[i].x = j;
			--j;
			if (j >= 0 && j < MAX_XSTRINGS)
			{
				for ( ; Xaxlen < j; ++Xaxlen)
					Xaxlab[Xaxlen] = ERx("");
				Xaxlab[j] = (xv->ar.sdat.str)[i];
				if (Xaxlen <= j)
					Xaxlen = j+1;
			}
		}
		break;
	case DVEC_I:
		for (i=0; i < len; ++i)
			(pt->ds)[i].x = (xv->ar.idat)[i];
		break;
	case DVEC_F:
		for (i=0; i < len; ++i)
			(pt->ds)[i].x = (xv->ar.fdat)[i];
		break;
	default:
		GTsyserr(S_GT0008_DS_bad_X_type, 0);
	}	/* end switch */
}

static
ax_scale (ax, kind, num, horiz)
register AXIS	*ax;
Gint kind;
i4  num;
bool horiz;
{
	Gipoint *actax, *cqdtax();
	Gint i;
	i4 xm,ym,xs,ys;
	f8 rhi_save, rlow_save, rinc_save;

	GTax_rat (ax,&xm,&ym,&xs,&ys);

	if (horiz)
	{
		xm = BTM_AXIS;
		ym = LEFT_AXIS;
		xs = TOP_AXIS;
		ys = RIGHT_AXIS;
	}

	for (i=0; i < num; ++i)
	{
		actax = cqdtax(i);
		actax->y = GTmap_axis[ym];
		actax->x = GTmap_axis[xm];
		cdtax(i,actax);
	}

	actax = cqdtax(num);
	actax->y = GTmap_axis[ys];
	actax->x = GTmap_axis[xs];
	cdtax(num,actax);

	/*
	** bug 9241:
	** if axis was a string or date axis, restore original
	** axis scaling numbers so that user values are not
	** changed on the axis attribute screens in vigraph.
	** ax_master modifies them, and the change needs to be
	** maintained across the associated call to ax_slave.
	*/
	rhi_save = ax[ym].r_hi;
	rlow_save = ax[ym].r_low;
	rinc_save = ax[ym].r_inc;
	ax_master (ax+ym,GTmap_axis[ym],kind);
	ax_slave (ax+ym,GTmap_axis[ym],ax+ys,GTmap_axis[ys],kind);
	if (ax[ym].ax_string || ax[ym].ax_date)
	{
		ax[ym].r_hi = rhi_save;
		ax[ym].r_low = rlow_save;
		ax[ym].r_inc = rinc_save;
	}
	rhi_save = ax[xm].r_hi;
	rlow_save = ax[xm].r_low;
	rinc_save = ax[xm].r_inc;
	ax_master (ax+xm,GTmap_axis[xm],kind);
	ax_slave (ax+xm,GTmap_axis[xm],ax+xs,GTmap_axis[xs],kind);
	if (ax[xm].ax_string || ax[ym].ax_date)
	{
		ax[xm].r_hi = rhi_save;
		ax[xm].r_low = rlow_save;
		ax[xm].r_inc = rinc_save;
	}
}

static
ax_master(axis, id, kind)
register AXIS	*axis;
register Gint	id;
Gint		kind;
{
	ax_type (axis, id, kind);

	/*
	** Scale axis:
	**	This is a MASTER axis, so we autoscale, unless
	**	AX_LIM is set.	When we autoscale, we fill in resulting
	**	limits, both for user, and for reference by SLAVE axis.
	*/

	caxasc(id, CNONE);	/* turn off autoscale at draw time */

	/*
	** 7/17/86 (bab)
	** Manually scale the labeled axes now so that VE's
	** autoscaling algorithm doesn't set odd step sizes.
	*/
	if (axis->ax_string)
	{
		Cbound	axb;
		i4	numlabs;	/* number of labels to show */

		if (id == GLEFT || id == GRIGHT)
			numlabs = Yaxlen;
		else
			numlabs = Xaxlen;
		/*
		** if truncation factor is set to a non-zero value, or
		** during a plot (when it's set to the full value)
		*/
		if (G_datchop > 0)
			numlabs = min(G_datchop, numlabs);

		/* set cchart axis limits and struct referenced by slave axis */
		if (kind == BAR)
		{
			/* bar charts have space either side of the bars */
			axb.strt = 0;
			axb.end = numlabs + 1;
		}
		else
		{
			/* scatter charts have no space at the axis edges */
			axb.strt = 1;
			axb.end = numlabs;
		}
		axis->r_low = axb.strt;
		axis->r_hi = axb.end;
		caxend(id, &axb);
		axis->r_inc = 1;		/* show every label */
		caxstsz(id, (Gfloat)axis->r_inc);
		axis->ax_labels.lb_prec = 0;	/* rather meaningless */
		caxlbpr(id, (Gint)axis->ax_labels.lb_prec);
	}
	else if ((axis->a_flags & AX_LIM) == 0 || axis->ax_date)
	{
		Cbound	*ab, *cqaxend();
		Gfloat	cqaxstsz();
		i4	precision;

		casc(id, kind); /* do autoscale */

		ab = cqaxend(id);
		axis->r_low = ab->strt;
		axis->r_hi = ab->end;
		axis->r_inc = cqaxstsz(id);
		if (axis->a_flags & AX_LOG)
		{
			/*
			** can't override the auto-precision, used
			** by VE, on log axes--precision varies
			*/
			axis->ax_labels.lb_prec = cqlbpr(id);
		}
		else
		{
			ax_prec(axis->r_inc, &precision);
			axis->ax_labels.lb_prec = precision;
			caxlbpr(id, (Gint)axis->ax_labels.lb_prec);
		}
	}
	else
	{	/* set axis limits */
		Cbound	axb;

		axb.strt = axis->r_low;
		axb.end = axis->r_hi;
		caxend(id, &axb);
		caxstsz(id, (Gfloat)axis->r_inc);
		caxlbpr(id, (Gint)axis->ax_labels.lb_prec);
	}
}

static
ax_slave(master, mid, slave, sid, kind)
register AXIS	*master;
Gint		mid;
register AXIS	*slave;
register Gint	sid;
Gint		kind;
{
	Cbound	axb;
	Gfloat	cqaxstfq();

	ax_type (slave, sid, kind);

	/*
	** Scale axis:
	**	this is a SLAVE axis.  We always manually scale it.  If it
	**	is to be "autoscaled", we use the results from the master
	**	for all scale parameters, otherwise just for the range, so
	**	you may have different increments / precisions on different
	**	axes.
	*/

	caxasc(sid, CNONE);	/* turn off runtime autoscaling */

	slave->r_low = master->r_low;
	slave->r_hi = master->r_hi;

	/* no need to special case for dated axes as of 7/16/86 */
	if ((slave->a_flags & AX_LIM) == 0 || (slave->a_flags & AX_LOG) ||
					slave->ax_string)
	{
		slave->r_inc = master->r_inc;
		slave->ax_labels.lb_prec = master->ax_labels.lb_prec;
	}
	axb.strt = slave->r_low;
	axb.end = slave->r_hi;
	caxend(sid, &axb);
	caxstsz(sid, (Gfloat)slave->r_inc);
	caxlbpr(sid, (Gint)slave->ax_labels.lb_prec);
	if (slave->a_flags & AX_LOG)
	{
		caxlblg(sid, cqaxlblg(mid));
		/* caxlg(sid,cqaxlg(mid)); */
	}

	/* changes for bug 9226 */
	/* set axis label step freq.  done here to avoid global vars */
	caxstpfrq(sid, cqaxstfq(mid));

	if (slave->ax_date)
	{
		Gfloat	cqgfq();
		Gfloat	cqtkfq();
		Cbound	*cqaxact();

		caxact(sid, cqaxact(mid));

		/*
		** set tick and grid frequency here rather than
		** in gtdraw.c (where 'if dated_axis' DON'T set them)
		** since using the inquire functions here (rather than
		** axis->ticks and axis->grids) and need to know
		** that this is a slave axis and what the master
		** axis is. (bab)
		*/
		cgfq(sid, cqgfq(mid));
		ctkfq(sid, TK_MAJOR, cqtkfq(mid, TK_MAJOR));

		caxlbyo(sid, cqaxlbyo(mid));	/* set year visability */
		caxmf(sid, cqaxmf(mid));	/* set month fmt */
		caxmt(sid, cqaxmt(mid));	/* set month type */
		caxmo(sid, cqaxmo(mid));	/* set month visability */
		caxdf(sid, cqaxdf(mid));	/* set day fmt */
		caxdt(sid, cqaxdtp(mid));	/* set day type */
		caxdo(sid, cqaxdo(mid));	/* set day visability */
	}
	/* end of changes for bug 9226 */
}

static
ax_type (axis,id, kind)
register AXIS	*axis;
register Gint	id;
Gint		kind;
{
	i4	numlabs;	/* number of labels to show */

	/* Set up axis type (check date, then string, first) */
	if (axis->ax_date)
		caxknd(id, MONTHAX);
	else if (axis->ax_string)
	{
		register i4	j;
		register i4	k;

		/*
		** For VE change.  7/17/86
		** have the axis labels set from the
		** [XY]axlab arrays rather than from
		** the 0th dataset.  same for the length.
		*/
		if (id == GRIGHT || id == GLEFT)
		{
			axis->ax_labels.lb_len = Yaxlen;
			axis->ax_labels.lb = Yaxlab;
		}
		else
		{
			axis->ax_labels.lb_len = Xaxlen;
			axis->ax_labels.lb = Xaxlab;
		}
		caxknd(id, LABELAX);

		numlabs = axis->ax_labels.lb_len;
		/*
		** ( >0 ) means truncation is set, or during plot
		** (where G_datchop is set to full value)
		*/
		if (G_datchop > 0)
			numlabs = min(G_datchop, numlabs);

		/*
		** pad the string labels by one for bar charts
		** to match VE expectations.
		** k is initialized to 1 to start setting real
		** labels one after the dummy ("") label.
		*/
		if (kind == BAR)
		{
			caxlbst(id, 0, ERx(""));
			k = 1;
		}
		else
		{
			k = 0;
		}

		for (j=0; j < numlabs; ++j, ++k)
			caxlbst(id, k, (axis->ax_labels.lb)[j]);
	}
	else if ((axis->a_flags & AX_LOG))
		caxknd(id, LOGAX);
	else
		caxknd(id, LINEARAX);
}

/*
**	Axis rationalizer.  Make SURE that we don't have scale-specified
**	axes opposite autoscaled, or log opposite normal.  The left and bottom,
**	if visible, take preference.  We have to rationalize, even if not
**	visible, because of bar charts.	 Passes back which axes are "masters"
**	and "slaves"
*/
GTax_rat (ax, xm, ym, xs, ys)
register AXIS	*ax;
i4		*xm, *ym, *xs, *ys;
{
	register AXIS *master, *slave;

	if (ax[LEFT_AXIS].ax_exists || ! ax[RIGHT_AXIS].ax_exists)
	{
		master = ax + LEFT_AXIS;
		slave = ax + RIGHT_AXIS;
		*ym = LEFT_AXIS;
		*ys = RIGHT_AXIS;
	}
	else
	{
		master = ax + RIGHT_AXIS;
		slave = ax + LEFT_AXIS;
		*ys = LEFT_AXIS;
		*ym = RIGHT_AXIS;
	}
	slave->a_flags &= ~(AX_LIM|AX_LOG);
	slave->a_flags |= master->a_flags & (AX_LIM|AX_LOG);
	slave->r_hi = master->r_hi;
	slave->r_low = master->r_low;

	if (ax[BTM_AXIS].ax_exists || ! ax[TOP_AXIS].ax_exists)
	{
		master = ax + BTM_AXIS;
		slave = ax + TOP_AXIS;
		*xm = BTM_AXIS;
		*xs = TOP_AXIS;
	}
	else
	{
		master = ax + TOP_AXIS;
		slave = ax + BTM_AXIS;
		*xs = BTM_AXIS;
		*xm = TOP_AXIS;
	}
	slave->a_flags &= ~(AX_LIM|AX_LOG);
	slave->a_flags |= master->a_flags & (AX_LIM|AX_LOG);
	slave->r_hi = master->r_hi;
	slave->r_low = master->r_low;
}

static
init_pairs()
{
	register i4	i;

	for (i=0; i < MAX_DS; ++i)
		Pairtab[i].tag = -1;
}

static
add_lab(idx, fld)
register i4	idx;
char		*fld;
{
	register i4		i;
	register GR_DVEC	*lv;	/* data vector */
	register i4		len;
	i4			erarg;

	GR_DVEC *GTdvfetch();

	len = GTdslen(idx);

	if (fld == NULL || *fld == '\0')
		cilbst(idx, ALL, ERx(""));
	else if ((lv = GTdvfetch(fld)) == NULL)
		GTsyserr(S_GT0009_DS_bad_labfield, 1, (PTR) fld);
	else if ((erarg = lv->type) != GRDV_STRING)
		GTsyserr(S_GT000A_DS_bad_lab_type, 2, (PTR) fld, (PTR) &erarg);
	else if (lv->len < len)
		GTsyserr(S_GT000B_DS_lab_too_short, 2, (PTR) fld, (PTR) &erarg);
	else for (i = 0 ; i < len ; ++i)
		cilbst(idx, i, (lv->ar.sdat.str)[i]);
}


/*
**  Determine a precision to use for the labels of numeric axes.
**  can't allow autoscale to set the precision since a subsequent
**  use on the slave axes on machines with bad floating point
**  accuracy will result in great 'precision'
**  despite that of the master axes.
**  Assumes that all stepsizes will have only one significant
**  digit.
**
**  7/30/86 (bab)	written.
**  27-oct-86 (bab)	fix for bug 10660
**	changed step and stepsize from f4 to f8.
*/
static
ax_prec(stepsize, precision)
f8	stepsize;
i4	*precision;
{
	register i4	prec = 0;
	register f8	step = stepsize;

	if (step < MINCUTOFF)
	{
		prec = MAXPREC;
	}
	else
	{
		while (step < MAXCUTOFF && prec < MAXPREC)
		{
			prec++;
			step = step * 10;
		}
	}
	*precision = prec;
}
