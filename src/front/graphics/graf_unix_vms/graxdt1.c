/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ergr.h>
# include	<ooclass.h>
# include	<graf.h>

/**
** Name:	graxdat1.qc	- vig operation code for type GrAxDt1
**
** Description:
**	Contains routines for translating GrAxDt1 objects
**	(old style axis descriptors) to equivalent vig AXIS structure.
**
**	This file defines:
**
**	GRvigAxdat(a, ax)
**
** History:
** Revision 40.101  86/04/24  11:19:25	peterk
** initial revision.
**/

# define LOG2_AX_G_DASHED	12
# define AX_LN_INVIS	0x4000	/* axis line invisibility */

/*
** GRvig1Axd	- vig translation for old-style Axis descriptors
**		with integer grids and ticks members. Can't use
**		same code since 2 different type structures
**		(GRAXDAT and GRAXDT1) have member at different
**		offsets due to different data types of grids and ticks
**		and elimination of "major" attribute in new GRAXDAT.
**
**		This code and the GrAxDt1 type can be removed once we
**		are sure no old-style axis descriptors remain in any
**		stored graph.  We can ensure this by putting
**		conversion code into next version of convgraph.
*/

GRvig1Axd(a, ax)
register	OO_GRAXDT1	*a;
register	AXIS		*ax;
{
	char	*GRtxtstore();
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
