/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	"gkscomp.h"
# include	<graf.h>

/**
** Name:    gtscomp.c	- component drawing routines
**
** Description:
**	We used to use cchart locator here.  All we are actually finding
**	anymore is axis areas.  Not using cchart locator means we don't
**	have to redraw to be able to locate subcomponents.
**
**	GTsubcomp - determine subcomponent
**
** History:
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for ax_chk() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**/

/* local prototypes */
extern GR_FRAME *G_frame;
static i4 ax_chk();

GTsubcomp (lx,ly)
float lx,ly;
{
	i4 otype;
	EXTENT *ex;
	float diffx,diffy;
	i4 idx;
	i4 *attr;

	if (G_frame->cur == NULL)
	{
		G_frame->subcomp = SCM_NULL;
		return;
	}

	otype = (G_frame->cur)->type;

	/*
	** convert lx / ly to 0.0 - 1.0 with reference to object, rather
	** than entire screen.
	*/
	ex = &((G_frame->cur)->ext);
	attr = (G_frame->cur)->attr;
	diffx = ex->xhi - ex->xlow + 0.000001;
	diffy = ex->yhi - ex->ylow + 0.000001;
	lx = (lx - ex->xlow) / diffx;
	ly = (ly - ex->ylow) / diffy;

	switch (otype)
	{
	case GROB_SCAT:
		G_frame->subcomp = ax_chk(((SCATCHART *) attr)->ax,lx,ly,&idx);
		G_frame->subidx = idx;
		break;
	case GROB_LINE:
		G_frame->subcomp = ax_chk(((LINECHART *) attr)->ax,lx,ly,&idx);
		G_frame->subidx = idx;
		break;
	case GROB_BAR:
		G_frame->subcomp = ax_chk(((BARCHART *) attr)->ax,lx,ly,&idx);
		G_frame->subidx = idx;
		break;
	default:
		G_frame->subcomp = SCM_NULL;
		break;
	}
}

static i4
ax_chk(ax,x,y,i)
AXIS *ax;
float x,y;
i4  *i;
{
	if (ax[BTM_AXIS].ax_exists && y < ax[BTM_AXIS].margin &&
		( !ax[LEFT_AXIS].ax_exists || x >  ax[LEFT_AXIS].margin) &&
		( !ax[RIGHT_AXIS].ax_exists || x < (1.0 - ax[RIGHT_AXIS].margin)))
		{
			*i = BTM_AXIS;
			return(SCM_AXIS);
		}
	if (ax[LEFT_AXIS].ax_exists && x < ax[LEFT_AXIS].margin &&
		( !ax[BTM_AXIS].ax_exists || y >  ax[BTM_AXIS].margin) &&
		( !ax[TOP_AXIS].ax_exists || y < (1.0 - ax[TOP_AXIS].margin)))
		{
			*i = LEFT_AXIS;
			return(SCM_AXIS);
		}
	if (ax[TOP_AXIS].ax_exists && y > (1.0 - ax[TOP_AXIS].margin) &&
		( !ax[LEFT_AXIS].ax_exists || x >  ax[LEFT_AXIS].margin) &&
		( !ax[RIGHT_AXIS].ax_exists || x < (1.0 - ax[RIGHT_AXIS].margin)))
		{
			*i = TOP_AXIS;
			return(SCM_AXIS);
		}
	if (ax[RIGHT_AXIS].ax_exists && x > (1.0 - ax[RIGHT_AXIS].margin) &&
		( !ax[BTM_AXIS].ax_exists || y > ax[BTM_AXIS].margin) &&
		( !ax[TOP_AXIS].ax_exists || y < (1.0 - ax[TOP_AXIS].margin)))
		{
			*i = RIGHT_AXIS;
			return(SCM_AXIS);
		}
	return (SCM_NULL);
}
