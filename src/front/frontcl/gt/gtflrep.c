/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<er.h>
# include	"gkscomp.h"
# include	<graf.h>
# include	<gtdef.h>
# include	"gtdev.h"

/**
** Name:	gtflrep.c -	Graphics System DataSet Fill Representation.
**
** Description:
**	Contains a routine that sets the fill representation for a C-Chart
**	dataset.  This file defines:
**
**	GTfillrep()	set dataset fill representation.
**
** History:
**		Revision 41.0  89/2/19 Mike S.
**		Conditionalize hatch patterns for DG.
**
**		Revision 40.4  86/04/09  13:18:13  bobm
**		LINT changes
**		
**		Revision 40.3  86/03/13  12:28:27  bobm
**		settings for individual data points as well as entire dataset
**		
**		Revision 40.2  86/03/12  14:16:01  bobm
**		seperate hatch / color - use FILLREP structure
**		
**		Revision 40.1  86/03/04  18:11:57  bobm
**		Change argument list to pass a color instead of a dependent
**		data structure.  Map color to hatch pattern through array.
**		
**		Revision 40.0  86/02/26  23:17:56  wong
**		Initial revision.
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**		
**/

extern GR_FRAME	*G_frame;

/*
** array of hatches corresponding to color indices > 2.
*/
#ifndef DGC_AOS
static COLOR	X_hatch[38] = {
			/*
			** First, by going in contrasting groups of 5, we get a
			** good relationship with colors, which work modulo 6.
			** The first group of 5 is the widest spaced, and we
			** pack closer as we descend groups of 5.
			*/
			 7,19,13,39,33,
			 6,18,12,38,32,
			 5,17,11,37,31,
			 4,16,10,36,30,
			 3,15, 9,35,29,
			 2,14, 8,34,28,

			/* finish out with the oddballs */
			20,21,22,23,24,25,26,	/* increasingly light slash
							reverse slashes */
			27			/* very heavy lattice */
};
#endif
/*	For the DG, we map hatch patterns 2-n cyclically into 1-8 thus:
**	Vigraph hatch number:	2 3 4 5 6 7 8 9 10 11 ...
**	GKS hatch number:	1 2 3 4 5 6 7 8 1  2
**	Pattern 1 (HF_TIGHT) becomes SOLID.  The mapping is done in
**	dg_gsetfillstyleindex.
*/
/*{
** Name:	GTfillrep() -	Set C-Chart DataSet Fill Representation.
**
** Description:
**	Sets the C-Chart dataset fill representation.  This is essentially
**	the color, but for monochrome devices or hatch preview levels, the
**	hatch style is set as well.  Hatch styles 0 or 1 correspond to
**	hollow or solid fill styles.  The routine may set attributes either
**	for an entire dataset (ds_pt < 0), or in individual element of a
**	a dataset (pie slices, for instance)
**
** Inputs:
**	ds_n		The dataset number.
**	ds_pt		The dataset point number.  <0 to specify entire
**			dataset.
**	fill		The FILLREP used to fill the dataset representation.
**
** History:
**	02/86 (jhw) -- Written.
**	03/86 (rlm) -- Hatch pattern and individual element modification.
*/

void
GTfillrep (ds_n, ds_pt, fill)
register Gint	ds_n;
Gint		ds_pt;
register FILLREP *fill;
{
	register Gflbundl	*bundl;

	Gflbundl	*cqdtfl();
	Gflbundl	*cqifl();

	/* get appropriate fill bundle */
	if (ds_pt < 0)
		bundl = cqdtfl(ds_n);
	else
		bundl = cqifl(ds_n,ds_pt);

	/* set attributes "filtered" by OUR view of the world */
	bundl->colour = X_COLOR(fill->color);
	if (G_cnum <= 2 || G_frame->preview == 3 || G_frame->preview == 4)
	{
		if (fill->hatch == BLACK)
			bundl->inter = HOLLOW;
		else if (fill->hatch == WHITE)
		{
			bundl->style = HF_TIGHT;
			bundl->inter = SOLID;
		}
		else
		{
			bundl->inter = HATCH;
#ifdef DGC_AOS
			bundl->style = fill->hatch;
#else
			bundl->style = X_hatch[(fill->hatch - 2) % 38];
#endif
		}
	}

	/* set appropriate fill bundle */
	if (ds_pt < 0)
		cdtfl(ds_n, bundl);
	else
		cifl(ds_n, ds_pt, bundl);
}
