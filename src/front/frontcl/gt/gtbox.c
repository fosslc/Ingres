/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	"gkscomp.h"
# include	<graf.h>
# include	"gtdev.h"
# include	<gtdef.h>

/**
** Name:    GTbox.c	- box drawing routines
**
** Description:
**	GKS level box draw / filling routines
**
**		NOTE: These routines must enter graphics mode to draw, and
**		leave workstation in alpha mode, unless G_idraw is set,
**		indicating that some other GT routine has already set graph
**		mode.
**
**		If G_idraw is set, meaning some other GT routine has called
**		us, we also do not call the .scr file dump routines - higher
**		level code is covering our dump.
**
**	routines:
**
**	GTflbox		fill a box with a solid color
**	GToutbox	draw an outline around a box area
**	GThchbox	draw a box filled with a hatch pattern
**
** History:
**	10/20/89 (dkh) - Added changes to eliminate duplicate FT files
**			 in GT directory.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for do_box() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**/

/* local prototypes */
static do_box();

extern Gws	*G_ws;		/* GKS workstation pointer (referenced) */

extern bool G_idraw;

extern i4  (*G_mfunc)();

/*{
** Name:	GTflbox	- draw a box filled with a given color
**
** Description:
**	draws a rectangle in SOLID style with appropriate fill color
**
** Inputs:
**	e	extent of box
**	col	fill color - number in range 0 to NUM_COLORS - 1.
**		translated to appropriate device color via X_COLOR
**
** Outputs:
**	none
**
** Side Effects:
**	none
**
** History:
**	8/85 (rlm)	written
*/

void
GTflbox(e,col)
EXTENT *e;
i4  col;
{
	void GThchbox();

	if (!G_idraw)
		GTdmpfbox(e,col);

	/* if monochrome, fill with a hatch pattern */
	if (G_cnum <= 2)
		GThchbox(e,col);
	else
		do_box(e,col,1,SOLID);
}

/*{
** Name:	GToutbox	- draw a rectangular border
**
** Description:
**	draws a rectangle in HOLLOW style, line color 1 (outline color)
**
** Inputs:
**	e	extent of box
**
** Outputs:
**	none
**
** Side Effects:
**	none
**
** History:
**	8/85 (rlm)	written
*/

void
GToutbox(e)
EXTENT *e;
{
	if (!G_idraw)
		GTdmphbox(e);
	do_box(e,1,1,HOLLOW);
}

/*{
** Name:	GThchbox	- draw a hatched box
**
** Description:
**	draws a rectangle in hatched style, with an outline if not
**	actually a SOLID color.  We DON'T do the outline in this case,
**	because solid fills on monochrome devices wind up here, and
**	we fill with 0 for an erase - don't want to leave borders on
**	monochrome devices.
**
** Inputs:
**	e	extent of box
**
** Outputs:
**	none
**
** Side Effects:
**	none
**
** History:
**	3/86 (rlm)	written
*/

void
GThchbox(e,hat)
EXTENT *e;
i4  hat;
{
	if (!G_idraw)
		GTdmphbox(e);
	switch(hat)
	{
	case 0:
		do_box(e,0,1,SOLID);

		/*
		** Special hack for the alpha driver - VE is leaving
		** bits of the box around when we try to erase something -
		** redraw the border in color 0 on "wide-line" devices
		*/
		if (G_wline)
			do_box(e,0,1,HOLLOW);
		break;
	case 1:
		do_box(e,1,1,SOLID);
		break;
	default:
		do_box(e,1,hat,HATCH);
		do_box(e,1,1,HOLLOW);
		break;
	}
}


/*
	local box utility
*/
static
do_box(e,col,hat,style)
EXTENT *e;		/* extent */
i4  col;		/* color */
i4  hat;		/* hatch */
Gflinter style;		/* style */
{
	Gwpoint rect[4];
	Gwrect vw;
	EXTENT drw;

	/* graphics mode */
	if (!G_idraw)
		(*G_mfunc) (GRAPHMODE);

	/*
	**	set viewport to entire screen (through cchart!)
	*/
	vw.ur.x = 1.0;
	vw.ur.y = 1.0;
	vw.ll.x = vw.ll.y = 0.0;

	cvwpt (&vw);
	caspect(G_ws, (Gfloat)G_aspect);

	/* fill style, color and hatch */
	gsfais (style);
	gsfaci (X_COLOR(col));
	gsfasi (hat);

	/* construct polygon array defining box */
	GTxetodrw (e,&drw);
	rect[0].x = drw.xlow;
	rect[0].y = drw.ylow * G_aspect;
	rect[1].x = drw.xlow;
	rect[1].y = drw.yhi * G_aspect;
	rect[2].x = drw.xhi;
	rect[2].y = drw.yhi * G_aspect;
	rect[3].x = drw.xhi;
	rect[3].y = drw.ylow * G_aspect;

	/* do the fill */
	gfa(4,rect);

	/* recover alpha mode */
	if (!G_idraw)
		(*G_mfunc) (ALPHAMODE);
}
