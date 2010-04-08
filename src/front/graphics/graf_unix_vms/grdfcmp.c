/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<ergr.h>
# include	<graf.h>

/**
** Name:	grdfcmp.c -	Graphics System Assign Default Attributes
**					to a Component.
** Description:
**	Contains a routine that fills in default attributes in a Graphic
**	Systems object structure, depending on its class.
**
**	GRdef_attr()		assign default attributes to a component.
**
** History:
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for def_axis(), def_margin(), def_lab() & def_ds()
**	  to prevent compiler errors with GCC 4.0 due to conflict with
**	  implicit declaration.
**/


/* local prototypes */
static def_axis();
static def_margin();
static def_lab();
static def_ds();

/*{
** Name:	GRdef_attr() -	Assign Default Attributes to a Component.
**
** Description:
**	sets the attributes of a component to default values.  For a piece of
**	trim, the text attribute will be left pointing to ""
**
** Inputs:
**	comp	component to fill
**
** Outputs:
**	Returns:
**		none
**	Exceptions:
**
** Side Effects:
**
** History:
**	8/85 (rlm)	written
*/

GRdef_attr (comp)
register GR_OBJ *comp;
{
	comp->goflgs = GRF_CLEAR|GRF_BORDER;
	comp->bck = BLACK;
	def_margin(&(comp->margin));
	comp->legend = NULL;

	switch (comp->type)
	{
	case GROB_TRIM:
	{
		register GR_TEXT *txt = (GR_TEXT *)comp->attr;

		/*
		** NOTE: we don't do anything with actual text pointer -
		** when NEW trim is created, user will be entering text.
		*/
		txt->font = TRI_COMPLEX;
		txt->ch_color = WHITE;
		txt->name = txt->column = ERx("");
		break;
	}
	case GROB_LEG:
	{
		register LEGEND *leg = (LEGEND *)comp->attr;

		leg->ttl_font = TRI_COMPLEX;
		leg->txt_font = TRI_COMPLEX;
		leg->ttl_color = WHITE;
		leg->txt_color = WHITE;
		leg->lg_flags = 0;
		break;
	}
	case GROB_PIE:
	{
		register PIECHART *pie = (PIECHART *)comp->attr;
		register FILLREP *fill;
		register i4  i;

		def_lab(&(pie->lab), ERx("TEXT PERCENT%"), TRUE);
		pie->rot = 0.0;
		pie->exstart = pie->exend = 0;
		pie->exfact = 0.0;
		pie->p_flags = 0;

		/*
		** not using outline color because it will always be
		** possible to create a piechart with the first and
		** last slice the same color - the outline will
		** distinguish those slices
		*/
		fill = pie->slice;
		for (i=0; i < MAX_SLICE; ++i)
		{
			fill->color = (i%6) + 2;
			fill->hatch = (i%38) + 2;
			++fill;
		}
		break;
	}
	case GROB_BAR:
	{
		register BARCHART *bar = (BARCHART *)comp->attr;
		register i4		i;

		def_axis(bar->ax);
		def_ds(bar->ds, MAX_B_DS, FALSE);
		for (i=0; i < MAX_B_DS; ++i)
			def_lab(bar->lab + i, ERx("DB_TXT_TYPE VALUE"), FALSE);
		bar->cluster = 0;
		bar->baseline = 0.0;
		bar->bar_horizontal = FALSE;
		bar->box_axes = FALSE;
		break;
	}
	case GROB_SCAT:
	case GROB_LINE:
	{
		register LINECHART *line = (LINECHART *)comp->attr;

		def_axis(line->ax);
		def_ds(line->ds, MAX_L_DS, comp->type == GROB_SCAT);
		line->line_fill = FALSE;
		line->box_axes = FALSE;
		break;
	}
	}	/* end switch */
}

static
def_axis (ax)
register AXIS	ax[4];
{
	register i4	i;

	ax[LEFT_AXIS].ax_exists = ax[BTM_AXIS].ax_exists = TRUE;

	for (i=0; i < 4; ++i)
	{
		register AXIS	*axis = &ax[i];

		axis->margin = DEF_AX_MARGIN;
		axis->ax_labels.lb_supr = ALL_LABS;
		axis->ax_labels.lb_num_fmt = 0;
		axis->ax_labels.lb_date_fmt = 0;
		axis->ticks = 1.0;
		axis->ax_header.hd_just = AXHJ_CENTER;
		axis->tick_loc = AXT_OUT;
		axis->grids = 1.0;
		axis->r_low = 0.0;
		axis->r_hi = 1.0;
		axis->r_inc = 0.01;
		axis->ax_labels.lb_prec = 0;
		axis->ax_header.hd_font = UNI_SIMPLEX;
		axis->ax_labels.lb_font = CARTO_SIMPLEX;
		axis->ax_header.hd_b_clr = BLACK;
		axis->ax_header.hd_t_clr = WHITE;
		axis->ax_labels.lb_clr = WHITE;
		axis->g_clr = WHITE;
		axis->ax_g_dash = SOLID;
		axis->ax_ln_invis = FALSE;
		axis->a_flags = AX_HD_SUP|AX_HDB_SUP;
		axis->ax_header.hd_text = ERx("");   /* safeguard */
	}	/* end for */
}

static
def_margin (mg)
register MARGIN	 *mg;
{
	mg->left = mg->right = mg->top = mg->bottom = DEF_MARGIN;
}

static
def_lab (lab, fs, exists)
register LABEL	*lab;
char		*fs;
bool		exists;
{
	lab->l_fmt = fs;
	lab->l_clr = WHITE;
	lab->l_font = UNI_SIMPLEX;
	lab->l_place = exists ? EXTERNAL : 0;
	lab->l_field = ERx("");
}

static
def_ds (ds, num, mark)
register DEPDAT *ds;
i4  num;
bool mark;
{
	register i4	i;

	/*
	** default depdat choices:
	**	fill and data colors will all be unique for first three
	**	datasets, after which we must start repeating.	This scheme
	**	also assures an odd color number for fill under even colors,
	**	and vice-versa, which is useful on 4 color terminals (2 colors
	**	not counting "white" and "black").
	**
	**	since fill hatch (lines only) and data hatch (bars only)
	**	don't appear together, they are set to the same values.
	**
	**	dash cycles, starting with a solid line.
	**
	**	lines are all straight
	**
	**	if we are filling in marks, cycle, starting with 2 = "plus"
	**	to make the "dot" choice (1) last.  Then interchange 2 and
	**	three to prefer star over plus.
	*/
	for (i=0; i < num; ++i)
	{
		ds->data.color = (i % 6) + 2;
		ds->fill.hatch = ds->data.hatch = (i % 38) + 2;
		ds->fill.color = ((i+3) % 6) + 2;
		if (! mark)
		{	/* set up for lines */
			ds->ds_mark = 0;
			ds->ds_cnct = XC_STRAIGHT;
		}
		else
		{	/* set up for marks */
			register i4	m;

			m = ((i+1)%6) + 1;
			if (m == 2 || m == 3)
				m = (3 - m) + 2;
			ds->ds_mark = m;
			ds->ds_cnct = XC_NONE;
		}
		ds->ds_dash = i % 4;

		++ds;
	}
}
