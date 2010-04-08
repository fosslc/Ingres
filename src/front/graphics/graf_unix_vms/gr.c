/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>
# include	<me.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ooclass.h>
# include	<oodefine.h>
# include	<oosymbol.h>
# include	<gropt.h>
# include	<graf.h>
# include	<ergr.h>
# include	<uigdata.h>

/**
** Name:	gr.c	- Object Methods for class Graph
**
** Description:
**	Contains most of the routines implementing object operations
**	for class Graph.  Most striaghtforward operations are found
**	in this file.  Others that require a more complex
**	implementation may be located in individual source files.
**
**	This file defines:
**
**	GRdestroy(self)
**	GRfetch(self, class)
**	GRflush(self, class)
**	GRinitDb(self, map, frame)
**	GRnameOk(self, name)
**	GRinit(g, newgraph, map, frame)
**	GRcinit(comp, gid, grobj, name, rem)
**	GRbinit(comp, gid, grobj)
**	GRlinit(comp, gid, grobj)
**	GRsinit(comp, gid, grobj)
**	GRpinit(comp, gid, grobj)
**	GRleginit(comp, gid, grobj)
**	GRtinit(comp, gid, grobj)
**	GRdepinit(self, compid, dep, lab)
**	GRaxinit(self, compid, ax)
**	GRvigGraph(g, frame, map)
**	vigSortString(map, s)
**	catSortString(g, map)
**	GRvigComp(comp)
**	GRvigLegend(comp)
**	static GRvigCDep(grax, ds, lab)
**	GRvigDepdat(d, ds, lab)
**	static GRvigCAx(grax, ax)
**	GRvigAxdat(a, ax)
**	GRvigBar(comp)
**	GRvigLine(comp)
**	GRvigScat(comp)
**	GRvigPie(comp)
**	GRvigText(comp)
**	GRoptInit(self, vopt)
**	GRvigOpt(self, vopt)
**
** History:
**	5/19/87 (peterk) - cleared out old history entries; changed
**		GRflush to flush Graph object before creating Encoding
**		in order to pick up true permanent id of Graph object.
**	20-may-88 (bruceb)
**		Changed MEalloc call into MEreqmem call.
**	14-june-1988 (danielt)
**		Changed GRinit to take the short remarks field from
**		the OO_GRAPH structure rather than from the 
**		MAP structure.
**	16-feb-89 (Mike S)
**		Make arguments that are just passed throuch "generic".
**		(for DG).
**
**	21-feb-90 (fpang)
**		Decode color assignment in GRvigText
**	21-jan-93 (pghosh)
**		Modified _flush to ii_flush, _flush0 to ii_flush0, _flushAll
**		to ii_flushAll. These are required as the front/hdr/hdr/
**		oosymbol.qsh was modified by fraser but were not reflected in 
**		this file.
**	25-Aug-1993 (fredv)
**		Included <me.h>.
**	28-Oct-1993 (donc)
**		Cast references to OOsndxxx routines to OOID, oodefine.h 
**		changed these routines declarations for return type to PTR.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**	    Fix bad spbuf declaration.
**/


/*
**	Macros for decoding/encoding hatch pattern / color into an integer
*/
#define HCENCODE(C,H) (0x8000 | ((H) << 6) | ((C) & 0x3f))
#define COLORDECODE(D) ((D) & 0x3f)
#define HATCHDECODE(D) ((0x8000 & (D)) ? ((D) >> 6) & 0x3f : (D) & 0x3f)

/*
** Label flags (DEPDAT associated)
*/
# define LAB_EXISTS 1
# define LAB_INSIDE 2
# define LAB_PLACE  7
# define LOG2_LAB_PLACE 2

/*
** Box Axes Flag
*/
# define BOX_AXES 2		/* should not conflict with bar or line chart flags */

/*
** Bar Chart Flags
*/
# define BAR_HORIZONTAL 1

/*
** Line Chart Flags
*/
# define LINE_FILL 1	/* fill in underneath lines */

/*
** Name:	axp_bits[] -	Axis Prescence Bits.
**
**	Note:  must not conflict with BAR_ or LINE_ flag settings.
*/
# define DRAX_LEFT 0x100
# define DRAX_RIGHT 0x200
# define DRAX_TOP 0x400
# define DRAX_BTM 0x800

static u_i2	axp_bits[] = {
			DRAX_LEFT,
			DRAX_RIGHT,
			DRAX_TOP,
			DRAX_BTM
};


/*
** Axis flags (AXIS structure)
*/
#ifndef AX_L_CENTER
# define AX_L_CENTER	0x0001	/* center labels between tick marks */
# define AX_GRID	0x0002	/* draw axis grid lines */
# define AX_HD_SUP	0x0004	/* suppress axis header */
# define AX_HD_DOWN	0x0008	/* right or left header drawn downward */
# define AX_HD_PERP	0x0010	/* perpendicular to axis */
# define AX_LOG		0x0020	/* log axis for numeric data */
# define AX_REVERSE	0x0040	/* reverse the axis */
# define AX_CROSS	0x0080	/* cross-hair axis at 0,0 */
/* Date and string set dynamically; not required in database
# define AX_DATE	0x0100
# define AX_STRING	0x0200
*/
# define AX_HDB_SUP	0x0400	/* suppress heading background */
# define AX_LIM		0x0800	/* axis has specified range */
#endif
# define AX_G_DASHED	0x3000	/* axis grid lines class */
# define LOG2_AX_G_DASHED	12
# define AX_LN_INVIS	0x4000	/* axis line invisibility */

char	*STalloc(), *GRtxtstore();
FUNC_EXTERN bool FEestabsp(char *);
FUNC_EXTERN OO_OBJECT *IIOBdemote();

bool
GRdestroy(self)
OO_GRAPH	*self;
{
	register bool	ret = TRUE;
	register bool	mytrans;
	char		spbuf[FE_MAXNAME];

	mytrans = FEestabsp(spbuf);
	if (self->class == OC_GRAPH)
	{
	    if ( IIOBdemote(self) != NULL && self->encgraph != nil )
		ret = (bool) OOsnd(self->encgraph, _destroy, 1, 0);
	}

	if (ret)
	    ret = (bool) OOsndSuper(self, _destroy, 1, 0);

	if (self->class == OC_GRAPH)
	    IIOBpromote(self, OC_GRAPH);

	if (ret)
	    FEclearsp(mytrans);
	else
	    FEabortsp(spbuf, mytrans);
	return ret;
}

OOID
GRfetch(self, class)
register OO_GRAPH	*self;
OOID			class;
{
	register OO_CLASS	*classp = (OO_CLASS *)OOp(class);
	register OO_CLASS	*GraphClass = (OO_CLASS *)OOp(OC_GRAPH);

	if (classp->level >= GraphClass->level)
	    return BTtestF(GraphClass->level, self->data.levelbits)?
		self->ooid: (OOID)OOsndSelf(self, _decode, 0);
	else
	    return (OOID)OOsndSuper(self, _fetch, class, 0);
}

OOID
GRflush(self, class)
OO_GRAPH	*self;
PTR		class;
{
	register OO_CLASS	*classp = (OO_CLASS *)OOp(class);
	OO_CLASS		*GraphClass = (OO_CLASS *)OOp(OC_GRAPH);
	OOID			id;

	if (classp->level >= GraphClass->level)
	{
	    if ((id = (OOID)OOsndSelf(self, ii_flush0, class))
		 == nil)
		return nil;
	    self = (OO_GRAPH *)OOp(id);

	    /* have to update allreferences to the graph id in components */

	    if (self->encgraph == nil)
		self->encgraph = (OOID)OOsndSelf(self, _encode, 1,
					    self->data.inDatabase);
	    if ((OOID)OOsnd(self->encgraph, ii_flushAll) == nil)
		return nil;
	    return id;
	}
	else if (classp->level > 0)	/* i.e. not at class Object */
	    return (OOID)OOsndSuper(self, ii_flush, class);
	else
	    return self->ooid;		/* level 0 already flushed above */
}

OOID
GRinitDb(self, map, frame)
OO_GRAPH	*self;
PTR		map;
PTR		frame;
{
	/* generate a permanent database ID here if we don't yet have one */

	if (OOisTempId(self->ooid))
	{
	    OOID	permid;

	    permid = IIOOnewId();
	    if (!OOhash(permid, self))
		return nil;
	    self->ooid = permid;
	}

	return (OOID)OOsndSelf(self, _init, map, frame);
}

OOID
GRinit(g, map, frame)
register OO_GRAPH	*g;
register GR_MAP		*map;
register GR_FRAME	*frame;
{
	register i4		i;
	register GR_OBJ		*grobj;

	if ((OOID)OOsndSuper(g, _init, STalloc(map->gr_name), 0, IIuiUser, 1,
	    g->short_remark, g->create_date, g->alter_date,
	    g->long_remark) == nil)
		return nil;

	/* initialize OO_GRAPH *struct from GR_MAP */
	map->id		= g->ooid;
	g->viewname	= STalloc(map->gr_view);
	g->grtype	= STalloc(map->gr_type);
	g->indep	= STalloc(map->indep);
	g->dep		= STalloc(map->dep);
	g->series	= STalloc(map->series);
	g->label	= STalloc(map->label);
	g->flags	= map->flags;
	catSortString(g, map);
	g->encgraph	= nil;

	/* initialize new component(s) from GR_FRAME */
	i = 0;
	for ( grobj = frame->head ; grobj != NULL ; grobj = grobj->next )
	    ++i;

	if ( i == 0 )
		g->components = nil;
	else
	{
	    g->components =
	        (OOID)OOsnd(OC_COLLECTION, _new, OC_UNDEFINED, g->data.tag, i);
	    i = 0;
	    for (grobj = frame->head; grobj != NULL; grobj = grobj->next)
	    {
		register OOID	gc;

		/*
		** create new component objects with TempId's
		** using _new
		*/
		gc = (OOID)OOsnd(grobj->type, _new, OC_UNDEFINED,
				g->ooid, grobj, i+1, 0
		);
		(OOID)OOsnd(g->components, _atPut, i++, gc, 0);
	    }
	}

	return g->ooid;
}

OOID
GRcinit(comp, gid, grobj, seq, name, rem)
register OO_GRCOMP	*comp;
i4			gid;
register GR_OBJ		*grobj;
i4			seq;
PTR			name;
PTR			rem;
{
	if ( (OOID)OOsndSuper(comp, _init, name, 0, IIuiUser, 1, rem,
			(char *)NULL, (char *)NULL, (char *)NULL) == nil )
	    return nil;

	comp->graph	= gid;
	grobj->id	= comp->ooid;

	comp->seq	= seq;
	comp->xlow	= grobj->ext.xlow;
	comp->xhi	= grobj->ext.xhi;
	comp->ylow	= grobj->ext.ylow;
	comp->yhi	= grobj->ext.yhi;
	comp->mgleft	= grobj->margin.left;
	comp->mgright	= grobj->margin.right;
	comp->mgtop	= grobj->margin.top;
	comp->mgbottom	= grobj->margin.bottom;
	comp->bckcolor	= grobj->bck;
	comp->flags	= grobj->goflgs;

	/* initialize depdats and axdats collections */
	comp->depdats = comp->axdats = nil;

	/* don't bother with legends OC_COLLECTION */
	comp->legends = UNKNOWN;

	return comp->ooid;
}

OOID
GRbinit(comp, gid, grobj, seq)
register OO_GRBAR	*comp;
i4			gid;
register GR_OBJ		*grobj;
i4			seq;
{
	register BARCHART	*attr = (BARCHART *)grobj->attr;
	register i4		i;

	if ( (OOID)OOsndSuper(comp, _init, gid, grobj, seq, ERx("aBarChart"),
			(char *)NULL) == nil )
	    return nil;

	comp->baseline = attr->baseline;
	comp->chartval = attr->cluster;
	comp->indep = STalloc(attr->indep);
	comp->flag2 = attr->bar_horizontal ? BAR_HORIZONTAL : 0;
	if (attr->box_axes)
		comp->flag2 |= BOX_AXES;
	if (comp->dsset = attr->ds_set)
	{
		comp->depdats =
			(OOID)OOsnd( OC_COLLECTION, _new, OC_UNDEFINED,
				comp->data.tag,
				attr->ds_set
			);
	}
	for ( i = 0 ; i < comp->dsset ; ++i )
	{
		register OOID	dep;

		dep = (OOID)OOsnd(OC_GRDEPDAT, _new, OC_UNDEFINED,
				comp->ooid, &attr->ds[i], &attr->lab[i], 0
		);
		(OOID)OOsnd(comp->depdats, _atPut, i, dep, 0);
	}
	comp->axdats =
           (OOID)OOsnd(OC_COLLECTION, _new, OC_UNDEFINED, comp->data.tag, 4);
	for ( i = 0 ; i < 4 ; ++i )
	{ /* for each axis */
		register OOID	ax;

		ax = (OOID)OOsnd( OC_GRAXDAT, _new, OC_UNDEFINED,
				comp->ooid, &attr->ax[i], 0
		);
		(OOID)OOsnd(comp->axdats, _atPut, i, ax, 0);
		if (attr->ax[i].ax_exists)
			comp->flag2 |= axp_bits[i];
	}
	return comp->ooid;
}

OOID
GRlinit(comp, gid, grobj, seq)
register OO_GRLINE	*comp;
i4			gid;
register GR_OBJ		*grobj;
i4			seq;
{
	register LINECHART	*attr = (LINECHART *)grobj->attr;
	register i4		i;

	if ( (OOID)OOsndSuper(comp, _init, gid, grobj, seq, ERx("aLineChart"),
			(char *)NULL) == nil )
	    return nil;

	comp->baseline = attr->baseline;
	comp->indep = STalloc(attr->indep);
	comp->flag2 = attr->line_fill ? LINE_FILL : 0;;
	if (attr->box_axes)
		comp->flag2 |= BOX_AXES;
	if (comp->dsset = attr->ds_set)
	{
		comp->depdats =
			(OOID)OOsnd( OC_COLLECTION, _new, OC_UNDEFINED,
				comp->data.tag, attr->ds_set
			);
	}
	for ( i = 0 ; i < comp->dsset ; ++i )
	{
		register OOID	dep;

		dep = (OOID)OOsnd( OC_GRDEPDAT, _new, OC_UNDEFINED,
				comp->ooid, &attr->ds[i], 0, 0
		);
		(OOID)OOsnd(comp->depdats, _atPut, i, dep, 0);
	}
	comp->axdats =
	   (OOID)OOsnd(OC_COLLECTION, _new, OC_UNDEFINED, comp->data.tag, 4);
	for ( i = 0 ; i < 4 ; ++i )
	{ /* for each axis */
		register OOID	ax;

		ax = (OOID)OOsnd( OC_GRAXDAT, _new, OC_UNDEFINED,
				comp->ooid, &attr->ax[i], 0
		);
		(OOID)OOsnd(comp->axdats, _atPut, i, ax, 0);
		if (attr->ax[i].ax_exists)
			comp->flag2 |= axp_bits[i];
	}
	return comp->ooid;
}

OOID
GRsinit(comp, gid, grobj, seq)
register OO_GRSCAT	*comp;
i4			gid;
register GR_OBJ		*grobj;
i4			seq;
{
	register SCATCHART	*attr = (SCATCHART *)grobj->attr;
	register i4		i;

	if ( (OOID)OOsndSuper(comp, _init, gid, grobj, seq, ERx("aScatChart"),
			(char *)NULL) == nil )
	    return nil;

	comp->baseline = attr->baseline;
	comp->indep = STalloc(attr->indep);
	comp->flag2 = attr->line_fill ? LINE_FILL : 0;;
	if (attr->box_axes)
		comp->flag2 |= BOX_AXES;
	if (comp->dsset = attr->ds_set)
	{
		comp->depdats =
		(OOID)OOsnd( OC_COLLECTION, _new, OC_UNDEFINED,
				comp->data.tag, attr->ds_set
			);
	}
	for ( i = 0 ; i < comp->dsset ; ++i )
	{
		register OOID	dep;

		dep = (OOID)OOsnd( OC_GRDEPDAT, _new, OC_UNDEFINED,
				comp->ooid, &attr->ds[i], 0, 0
		);
		(OOID)OOsnd(comp->depdats, _atPut, i, dep, 0);
	}
	comp->axdats =
	    (OOID)OOsnd(OC_COLLECTION, _new, OC_UNDEFINED, comp->data.tag, 4);
	for ( i = 0 ; i < 4 ; ++i )
	{ /* for each axis */
		register OOID	ax;

		ax = (OOID)OOsnd( OC_GRAXDAT, _new, OC_UNDEFINED,
				comp->ooid, &attr->ax[i], 0
		);
		(OOID)OOsnd(comp->axdats, _atPut, i, ax, 0);
		if (attr->ax[i].ax_exists)
			comp->flag2 |= axp_bits[i];
	}
	return comp->ooid;
}

OOID
GRpinit(comp, gid, grobj, seq)
register OO_GRPIE	*comp;
i4			gid;
register GR_OBJ		*grobj;
i4			seq;
{
	register PIECHART	*attr = (PIECHART *)grobj->attr;
	register OOID		dep;
	register char		*cslice;
	register FILLREP	*aslice, *maxslice;

	if ( (OOID)OOsndSuper(comp, _init, gid, grobj, seq, ERx("aPieChart"),
			(char *)NULL) == nil )
	    return nil;

	comp->depdats =
	   (OOID)OOsnd(OC_COLLECTION, _new, OC_UNDEFINED, comp->data.tag, 1);
	dep = (OOID)OOsnd(OC_GRDEPDAT, _new, OC_UNDEFINED,
		 comp->ooid, &attr->ds,
	    &attr->lab, 0);
	(OOID)OOsnd(comp->depdats, _atPut, 0, dep, 0);
	comp->rotate	= attr->rot;
	comp->flag2	= attr->p_flags;
	comp->expfirst	= attr->exstart;
	comp->explast	= attr->exend;
	comp->explosion = attr->exfact;
	aslice = attr->slice;
	comp->slice = (char *)MEreqmem((u_i4)0, (u_i4)(MAX_SLICE*2 + 1),
	    				(bool)FALSE, (STATUS *)NULL
	);
	cslice = comp->slice;
	for (maxslice = &attr->slice[MAX_SLICE]; aslice < maxslice; aslice++)
	{
	    *cslice++ = aslice->color + '0';
	    *cslice++ = aslice->hatch + '0';
	}
	*cslice = '\0';

	return comp->ooid;
}

OOID
GRleginit(comp, gid, grobj, seq)
register OO_GRLEGEND	*comp;
i4			gid;
register GR_OBJ		*grobj;
register i4		seq;
{
	register LEGEND *attr = (LEGEND *)grobj->attr;

	if ((OOID)OOsndSuper(comp, _init, gid, grobj, seq, ERx("aGrLegend"),
	    STalloc(attr->title)) == nil)
		return nil;

	comp->flag2	= attr->lg_flags;
	comp->tfont	= attr->ttl_font;
	comp->tcolor	= attr->ttl_color;
	comp->dfont	= attr->txt_font;
	comp->dcolor	= attr->txt_color;

	/*
	** just set comp->agrcomp to nil for now;
	** the real association will be set up by 'GRlgchk()' when the
	** graph is read in from the database.
	*/

	comp->agrcomp = nil;

	return comp->ooid;
}

OOID
GRtinit(comp, gid, grobj, seq)
register OO_GRTEXT	*comp;
i4			gid;
register GR_OBJ		*grobj;
register i4		seq;
{
	register GR_TEXT	*attr = (GR_TEXT *)grobj->attr;

	if ((OOID)OOsndSuper(comp, _init, gid, grobj, seq, STalloc(attr->name),
	    STalloc(attr->text)) == nil)
		return nil;

	comp->colum	= STalloc(attr->column);
	comp->font	= attr->font;
	comp->color	= attr->ch_color;

	return comp->ooid;
}

OOID
GRdepinit(self, compid, dep, lab)
register OO_GRDEPDAT	*self;
register OOID		compid;
register DEPDAT		*dep;
register LABEL		*lab;
{
	if ( (OOID)OOsndSuper(self, _init, ERx("aGrDepDat"), 0, (char *)NULL, 1,
	    		STalloc(dep->desc),
			(char *)NULL, (char *)NULL, (char *)NULL) == nil )
		return nil;

	self->agrcomp	= compid;
	self->dfield	= STalloc(dep->field);
	self->fcolor	= HCENCODE(dep->fill.color,dep->fill.hatch);
	self->dcolor	= HCENCODE(dep->data.color,dep->data.hatch);
	self->mark	= dep->ds_mark; /* actually a bit field */
	self->dash	= dep->ds_dash; /* actually a bit field */
	self->cnct	= dep->ds_cnct; /* actually a bit field */

	if (lab) {
	    self->lformat	= STalloc(lab->l_fmt);
	    self->lfield	= STalloc(lab->l_field);
	    self->flags		= lab->l_place << LOG2_LAB_PLACE;
	    self->lcolor	= lab->l_clr;
	    self->lfont		= lab->l_font;
	}

	return self->ooid;
}

OOID
GRaxinit(self, compid, ax)
register OO_GRAXDAT	*self;
register OOID		compid;
register AXIS		*ax;
{
	if ( (OOID)OOsndSuper(self, _init, ERx("aGrAxDat"), 0, (char *)NULL, 1,
			(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL)
		    == nil )
		return nil;

	self->agrcomp	= compid;
	self->ticks	= ax->ticks;
	self->grids	= ax->grids;
	self->rnglo	= ax->r_low;
	self->rnghi	= ax->r_hi;
	self->inc	= ax->r_inc;
	self->margin	= ax->margin;
	self->header	= STalloc(ax->ax_header.hd_text);
	self->labsup	= ax->ax_labels.lb_supr;
	self->numfmt	= ax->ax_labels.lb_num_fmt;
	self->datefmt	= ax->ax_labels.lb_date_fmt;
	self->hdjust	= ax->ax_header.hd_just;
	self->tickloc	= ax->tick_loc;
	self->dsidx	= ax->ds_idx;
	self->prec	= ax->ax_labels.lb_prec;
	self->flags	= (ax->a_flags & ~AX_G_DASHED)| (ax->ax_g_dash << LOG2_AX_G_DASHED);
	if (ax->ax_ln_invis)
	    self->flags |= AX_LN_INVIS;
	self->hfont	= ax->ax_header.hd_font;
	self->lfont	= ax->ax_labels.lb_font;
	self->hbcolor	= ax->ax_header.hd_b_clr;
	self->htcolor	= ax->ax_header.hd_t_clr;
	self->labcolor	= ax->ax_labels.lb_clr;
	self->lincolor	= ax->g_clr;
	return self->ooid;
}

OOID
GRvigGraph(g, frame, map)
register OO_GRAPH	*g;
register GR_FRAME	*frame;
register GR_MAP		*map;
{
	register OOID	col, comp;
	register GR_OBJ *grobj;

	map->id		= g->ooid;
	map->gr_name	= GRtxtstore(g->name);
	map->gr_view	= GRtxtstore(g->viewname);
	map->gr_type	= GRtxtstore(g->grtype);
	map->gr_desc	= GRtxtstore(g->short_remark);
	map->indep	= GRtxtstore(g->indep);
	map->dep	= GRtxtstore(g->dep);
	map->series	= GRtxtstore(g->series);
	map->label	= GRtxtstore(g->label);
	map->flags	= g->flags;
	vigSortString(map, GRtxtstore(g->sortstring));

	/* translate the component(s) into GR_OBJ(s) */

	if ((col = g->components) != nil) {
	    for (comp = (OOID)OOsnd(col, _first, 0);
		!(OOID)OOsnd(col, _atEnd, 0);
		comp = (OOID)OOsnd(col, _next, 0)) {
		    grobj = (GR_OBJ *)OOsnd(comp, _vig, 0);
		    grobj->next = NULL;

		    if (frame->tail == NULL) {
			grobj->prev = NULL;
			frame->tail = frame->head = grobj;
		    }
		    else {
			grobj->prev = frame->tail;
			frame->tail = frame->tail->next = grobj;
		    }
	    }
	}
	frame->cur = frame->head;
	/* now go back and link LEGENDs to associated GR_OBJs */
	GRlgchk(frame);
	return g->ooid;
}

vigSortString(map, s)
register GR_MAP *map;
register char	*s;
{
	register GR_SORT	*p = map->sortcols;

	for (;; p++) {
	    p->s_colname = s;
	    while (*s != '+' && *s != '-' && *s) s++;

	    if (*s) {
		p->s_dir = *s;
		*s++ = '\0';
		continue;
	    }
	    else
		break;
	}
	map->ntosort = p - map->sortcols;
}

catSortString(g, map)
OO_GRAPH	*g;
register GR_MAP *map;
{
	register GR_SORT	*p = map->sortcols;
	char			buf[MAXSORTSTRING];
	register char		*s = buf, *t;

	while (p < &map->sortcols[map->ntosort]) {
		t = p->s_colname;
		while (*s++ = *t++);
		s[-1] = p->s_dir;
		p++;
	}
	*s = '\0';
	if ((g->sortstring = STalloc(buf)) == NULL)
	    IIUGerr(S_GR0001_STalloc_err, UG_ERR_ERROR, 0);
}

GR_OBJ *
GRvigComp(comp)
register OO_GRCOMP	*comp;
{
	register GR_OBJ *grobj;

	GR_OBJ	*GRgetobj();

	/*
	** Allocate the GR_OBJ structure here.
	** The subClass operation should call this first (as super).
	*/

	grobj = GRgetobj(comp->class);
	GRdef_attr(grobj);

	grobj->id = comp->ooid;
	grobj->ext.xlow = comp->xlow;
	grobj->ext.xhi	= comp->xhi;
	grobj->ext.ylow = comp->ylow;
	grobj->ext.yhi = comp->yhi;
	grobj->margin.left = comp->mgleft;
	grobj->margin.right = comp->mgright;
	grobj->margin.top = comp->mgtop;
	grobj->margin.bottom = comp->mgbottom;
	grobj->bck = comp->bckcolor;
	grobj->goflgs = comp->flags;
	grobj->legend = (LEGEND *)NULL; /* gets filled in later */

	return grobj;
}

GR_OBJ *
GRvigLegend(comp)
register OO_GRLEGEND	*comp;
{
	register LEGEND *attr;
	register GR_OBJ *grobj = (GR_OBJ *)OOsndSuper(comp, _vig, 0); /* OO_GRCOMP *xlation */

	attr = (LEGEND *)grobj->attr;
	attr->title = GRtxtstore(comp->short_remark);
	attr->lg_flags = comp->flag2;
	attr->ttl_font = comp->tfont;
	attr->ttl_color = comp->tcolor;
	attr->txt_font = comp->dfont;
	attr->txt_color = comp->dcolor;
	/* attr->assoc will be filled in later */

	return grobj;
}

static
GRvigCDep(grax, ds, lab)
OO_GRAX *grax;
register DEPDAT *ds;
register LABEL	*lab;
{
	register OOID	col = grax->depdats;
	register OOID	d;

	if (col == nil) return;

	for (d = (OOID)OOsnd(col, _first, 0);
	    !(OOID)OOsnd(col, _atEnd, 0);
	    d = (OOID)OOsnd(col, _next, 0)) {
		(OOID)OOsnd(d, _vig, ds, lab);
		ds++;
		if (lab) lab++;
	}
}

GRvigDepdat(d, ds, lab)
register OO_GRDEPDAT	*d;
register DEPDAT *ds;
register LABEL	*lab;
{
	ds->field = GRtxtstore(d->dfield);
	ds->desc = GRtxtstore(d->short_remark);
	ds->fill.color = COLORDECODE(d->fcolor);
	ds->fill.hatch = HATCHDECODE(d->fcolor);
	ds->data.color = COLORDECODE(d->dcolor);
	ds->data.hatch = HATCHDECODE(d->dcolor);
	ds->ds_mark = d->mark;	/* actually a bit field */
	ds->ds_dash = d->dash;	/* actually a bit field */
	ds->ds_cnct = d->cnct;	/* actually a bit field */

	if (lab) {
	    lab->l_fmt = GRtxtstore(d->lformat);
	    lab->l_field = GRtxtstore(d->lfield);
	    lab->l_place = (d->flags & LAB_INSIDE) != 0 ? MIDDLE
				: ((d->flags & LAB_EXISTS) != 0 ? EXTERNAL
						: d->flags >> LOG2_LAB_PLACE);
	    lab->l_clr = d->lcolor;
	    lab->l_font = d->lfont;
	}
}

static
GRvigCAx(grax, flag2, ax)
OO_GRAX *grax;
i4	flag2;
register AXIS	*ax;
{
	register OOID	col = grax->axdats;
	register OOID	a;
	register i4	i;

	if (col == nil) return;

	i = 0;
	for (a = (OOID)OOsnd(col, _first, 0);
	    !(OOID)OOsnd(col, _atEnd, 0);
	    a = (OOID)OOsnd(col, _next, 0)) {
		(OOID)OOsnd(a, _vig, ax);
		ax->ax_exists = (flag2 & axp_bits[i++]) != 0;
		++ax;
	}
}

GRvigAxdat(a, ax)
register OO_GRAXDAT	*a;
register AXIS		*ax;
{
	ax->ticks = a->ticks;
	ax->grids = a->grids;
	ax->r_low = a->rnglo;
	ax->r_hi = a->rnghi;
	ax->r_inc = a->inc;
	ax->margin = a->margin;
	ax->ax_header.hd_text = GRtxtstore(a->header);
	ax->ax_labels.lb_supr= a->labsup;
	ax->ax_labels.lb_num_fmt = a->numfmt;
	ax->ax_labels.lb_date_fmt = a->datefmt;
	ax->ax_header.hd_just = a->hdjust;
	ax->tick_loc = a->tickloc;
	ax->ds_idx = a->dsidx;
	ax->ax_labels.lb_prec = a->prec;
	ax->a_flags = a->flags;
	ax->ax_g_dash = a->flags >> LOG2_AX_G_DASHED;
	ax->ax_ln_invis = (a->flags & AX_LN_INVIS) != 0;
	ax->ax_header.hd_font = a->hfont;
	ax->ax_labels.lb_font = a->lfont;
	ax->ax_header.hd_b_clr = a->hbcolor;
	ax->ax_header.hd_t_clr = a->htcolor;
	ax->ax_labels.lb_clr = a->labcolor;
	ax->g_clr = a->lincolor;
}

GR_OBJ *
GRvigBar(comp)
register OO_GRBAR	*comp;
{
	register BARCHART	*attr;
	register GR_OBJ *grobj = (GR_OBJ *)OOsndSuper(comp, _vig, 0); /* OO_GRCOMP *xlation */

	attr = (BARCHART *) grobj->attr;

	attr->baseline = comp->baseline;
	attr->cluster = comp->chartval;
	attr->indep = GRtxtstore(comp->indep);
	attr->bar_horizontal = (comp->flag2 & BAR_HORIZONTAL) != 0;
	attr->box_axes = (comp->flag2 & BOX_AXES) != 0;
	attr->ds_set = comp->dsset;
	GRvigCDep(comp, attr->ds, attr->lab);
	GRvigCAx(comp, comp->flag2, attr->ax);
	return grobj;
}

GR_OBJ *
GRvigLine(comp)
register OO_GRLINE	*comp;
{
	register LINECHART	*attr;
	register GR_OBJ *grobj = (GR_OBJ *)OOsndSuper(comp, _vig, 0); /* OO_GRCOMP *xlation */

	attr = (LINECHART *) grobj->attr;

	attr->baseline = comp->baseline;
	attr->indep = GRtxtstore(comp->indep);
	attr->line_fill = (comp->flag2 & LINE_FILL) != 0;
	attr->box_axes = (comp->flag2 & BOX_AXES) != 0;
	attr->ds_set = comp->dsset;
	GRvigCDep(comp, attr->ds, 0);
	GRvigCAx(comp, comp->flag2, attr->ax);
	return grobj;
}

GR_OBJ *
GRvigScat(comp)
register OO_GRSCAT	*comp;
{
	register SCATCHART	*attr;
	register GR_OBJ *grobj = (GR_OBJ *)OOsndSuper(comp, _vig, 0); /* OO_GRCOMP *xlation */

	attr = (SCATCHART *) grobj->attr;

	attr->baseline = comp->baseline;
	attr->indep = GRtxtstore(comp->indep);
	attr->line_fill = (comp->flag2 & LINE_FILL) != 0;
	attr->box_axes = (comp->flag2 & BOX_AXES) != 0;
	attr->ds_set = comp->dsset;
	GRvigCDep(comp, attr->ds, 0);
	GRvigCAx(comp, comp->flag2, attr->ax);
	return grobj;
}

GR_OBJ *
GRvigPie(comp)
register OO_GRPIE	*comp;
{
	register PIECHART	*attr;
	register GR_OBJ *grobj = (GR_OBJ *)OOsndSuper(comp, _vig, 0); /* OO_GRCOMP *xlation */
	register char	*cslice;
	register FILLREP	*aslice, *maxslice;

	attr = (PIECHART *) grobj->attr;
	GRvigCDep(comp, &attr->ds, &attr->lab);
	attr->rot = comp->rotate;
	attr->p_flags = comp->flag2;
	attr->exstart = comp->expfirst;
	attr->exend = comp->explast;
	attr->exfact = comp->explosion;
	if (cslice = comp->slice)
	{
	    maxslice = &attr->slice[MAX_SLICE];
	    for (aslice = attr->slice; aslice < maxslice; aslice++)
	    {
		if (*cslice)
		    aslice->color = *cslice++ - '0';
		else
		    break;
		if (*cslice)
		    aslice->hatch = *cslice++ - '0';
		else
		    break;
	    }
	}
	return grobj;
}

GR_OBJ *
GRvigText(comp)
register OO_GRTEXT	*comp;
{
	register GR_TEXT	*attr;
	register GR_OBJ *grobj = (GR_OBJ *)OOsndSuper(comp, _vig, 0); /* OO_GRCOMP *xlation */

	attr = (GR_TEXT *) grobj->attr;

	attr->name = GRtxtstore(comp->name);
	attr->text = GRtxtstore(comp->short_remark);
	attr->column = GRtxtstore(comp->colum);
	attr->font = comp->font;
	attr->ch_color = COLORDECODE(comp->color);
	return grobj;
}

/*
**	GRoptInit -- initialize OO_GROPT *object from vig GR_OPT structure
*/

OOID
GRoptInit(self, vopt)
register OO_GROPT	*self;
register GR_OPT		*vopt;
{
	char	*_now = ERx("now");
	char	*cre = self->create_date? self->create_date: _now;
	(OOID)OOsndSuper(self, _init, IIuiUser, 0, IIuiUser, 
		   1, _, cre, _now, 0);

	self->plotdev	= STalloc(vopt->plot_type);	/* plot device type */
	self->plotloc	= STalloc(vopt->plot_loc);	/* plot device location */
	self->fc	= vopt->fc_char;	/* font compare char */
	self->mainlevel = vopt->lv_main;	/* main layout level */
	self->editlevel = vopt->lv_edit;	/* edit preview level */
	self->layerlevel	= vopt->lv_lyr; /* layering preview level */
	self->plotlevel = vopt->lv_plot;	/* plotting preview level */
	self->dtrunc	= vopt->dt_trunc;	/* data truncation length */
	self->flags	= vopt->o_flags;	/* y/n type options */
	return self->ooid;
}

OOID
GRvigOpt(self, vopt)
register OO_GROPT	*self;
register GR_OPT		*vopt;
{
	vopt->id	= self->ooid;		/* object database id */
	vopt->plot_type = STalloc(self->plotdev);	/* plot device type */
	vopt->plot_loc	= STalloc(self->plotloc);	/* plot device location */
	STcopy(self->fc, vopt->fc_char);	/* font compare char */
	vopt->lv_main	= self->mainlevel;	/* main layout level */
	vopt->lv_edit	= self->editlevel;	/* edit preview level */
	vopt->lv_lyr	= self->layerlevel;	/* layering preview level */
	vopt->lv_plot	= self->plotlevel;	/* plotting preview level */
	vopt->dt_trunc	= self->dtrunc; /* data truncation length */
	vopt->o_flags	= self->flags;	/* y/n type options */

	return self->ooid;
}
