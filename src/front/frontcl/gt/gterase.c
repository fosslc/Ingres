
/*
**    Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<graf.h>
# include	<gtdef.h>

/**
** Name:    erase.c	- component erasing routines
**
** Description:
**	calls for erasing components.  Note that there are no cchart / GKS
**	calls in here - the actual work is done through "box" and
**	"draw" calls.  This file defines:
**
**	GTelayer	erases all components in a given layer
**	GTerase	erases a single component
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
**        Move prototype for elist_split() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**/

FUNC_EXTERN GR_OBJ *GRgetobj ();

extern GR_FRAME *G_frame;

extern bool G_idraw;

extern i4  (*G_mfunc)();

static	GR_OBJ *elist_split();
/*
**	Local overlap utility.  Really Gtovlp without the "cheat" to
**	avoid overlapping adjacent objects.  If somebody else turns out
**	to need this functionality, GTovlp ought to get a flag parameter.
**	Actually we "cheat" by a small amount the OTHER direction for
**	round off error.
*/
static bool
loc_ovlp (ex1,ex2)
EXTENT *ex1,*ex2;
{
	if ((ex1->xhi + .001) < ex2->xlow || (ex1->xlow - .001) > ex2->xhi)
		return (FALSE);
	if ((ex1->yhi + .001) < ex2->ylow || (ex1->ylow - .001) > ex2->yhi)
		return (FALSE);
	return (TRUE);
}

/*{
** Name:	GTelayer	- erase all components on the current layer
**
** Description:
**	erases the current (top) layer of the display.  This routine is
**	really the "uncover" operation
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Side Effects:
**	Decrements current layer
**
** History:
**	8/85 (rlm)	written
*/

void
GTelayer ()
{
	GR_OBJ *ptr;

	/* erase all the components */
	GTxactbeg();
	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
		if (ptr->layer == G_frame->clayer)
			GTerase (ptr);

	/* mark all those underneath something erased */
	GTmrkclr();
	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
		if (ptr->layer == G_frame->clayer)
			GTmrkb(&(ptr->ext),0,G_frame->clayer-1);

	--G_frame->clayer;

	/* redraw everything that's been uncovered */
	GTdmark();
	GTxactend();
	GTdmperase();
}

/*{
** Name:	GTerase	- erase a component
**
** Description:
**	erases a component.  Erases only those areas of the component
**	which are currently visible.  Does this by constructing a temporary
**	list of NULL components whose extents define the visible areas.
**	Does not redraw components, so that redraw may be optimized on
**	multiple erasures.
**
** Inputs:
**	c	component to erase
**
** Outputs:
**	none
**
** Side Effects:
**	none
**
** History:
**	8/85 (rlm)	written
**	01-oct-91 (jillb/jroy--DGC)
**	    make function declaration consistent with its definition
*/

GTerase(c)
GR_OBJ *c;
{
	GR_OBJ *list;		/* list of extents to erase */
	GR_OBJ *pr;		/* list next item */
	GR_OBJ *nx;		/* list predecessor */
	GR_OBJ *ptr,*p2;	/* general pointer variables */
	GR_OBJ *GRgetobj();
	bool omode;

	/* initial list is extent of entire component */
	list = GRgetobj(GROB_NULL);
	list->next = NULL;
	list->prev = NULL;
	list->ext.yhi = c->ext.yhi;
	list->ext.ylow = c->ext.ylow;
	list->ext.xhi = c->ext.xhi;
	list->ext.xlow = c->ext.xlow;

	/*
		We look down the list of all components, starting after
		the one being erased, and see if each one overlaps with any
		of the extents on the current list.  If it does, the list
		item will be split into multiple extents
	*/
	for (ptr = c->next; ptr != NULL; ptr = ptr->next)
		for (p2 = list; p2 != NULL; p2 = nx)
		{
			/*
				check p2 list item for overlap with
				ptr component (if visible).  Need to set
				nx variable for loop end, since we may
				play games with the list item.
			*/
			nx = p2->next;
			pr = p2->prev;
			if (ptr->layer <= G_frame->clayer && loc_ovlp(&(ptr->ext),&(p2->ext)))
			{
				/*
					overlaps.  split list item into
					0 - 4 different extents, and
					replace back into list.  Mark ptr
					as needing its box redrawn
				*/
				if (pr == NULL)
				{
					list = elist_split (p2,&(ptr->ext));
					list->prev = NULL;
				}
				else
				{
					pr->next = elist_split (p2,&(ptr->ext));
					(pr->next)->prev = pr;
				}
			}
		}

	/* erase visible portions indicated by list and return list storage */
	omode = G_idraw;
	if (!omode)
	{
		(*G_mfunc) (GRAPHMODE);
		G_idraw = TRUE;
	}
	for (ptr = list; ptr != NULL; ptr = nx)
	{
		nx = ptr->next;
		GTflbox (&(ptr->ext),0);
		GRretobj (ptr);
	}
	if (!omode)
	{
		(*G_mfunc) (ALPHAMODE);
		GTdmperase ();
		G_idraw = FALSE;
	}
}

/*
	local routine to split a list item into a linked list of 0-4 NULL
	objects, which cover area not overlapped by the given extent, ie:

	----------------------         ----------------------
	|               lptr |         | 0    |  3  |   2   |
	|                    |         |      |     |       |
	|      -------       |   --->  |------|-----|-------|
	|      | ext |       |         |      |XXXXX|       |
	|      -------       |         |------|-----|-------|
	|                    |         |      |  1  |       |
	----------------------         ----------------------

	with some pieces not being needed cases where ext extends past
	borders of list item.  Routine returns pointer to item which replaces
	list item.  "next" linkage will have been preserved.  Boxes are
	overlapped in corners because adjustment is made to inside box
	dimensions to avoid erasing outlines, meaning that a small area
	is left by, say the upper rectangle.  The left and right rectangle
	overlaps will take care of it.
*/


static
GR_OBJ
*elist_split (lptr,ext)
GR_OBJ *lptr;		/* list pointer */
EXTENT *ext;		/* extent */
{
	i4 i;
	EXTENT split [4];	/* 4 new created rectangles */
	GR_OBJ *head,*new,*GRgetobj();

	head = lptr->next;

	/* left hand rectangle */
	split[0].xlow = lptr->ext.xlow;
	split[0].xhi = ext->xlow - G_frame->lx;
	split[0].ylow = lptr->ext.ylow;
	split[0].yhi = lptr->ext.yhi;

	/* lower rectangle */
	split[1].xlow = lptr->ext.xlow;
	split[1].xhi = lptr->ext.xhi;
	split[1].ylow = ext->yhi + G_frame->ly;
	split[1].yhi = lptr->ext.yhi;

	/* right hand rectangle */
	split[2].xlow = ext->xhi + G_frame->lx;
	split[2].xhi = lptr->ext.xhi;
	split[2].ylow = lptr->ext.ylow;
	split[2].yhi = lptr->ext.yhi;

	/* upper rectangle */
	split[3].xlow = lptr->ext.xlow;
	split[3].xhi = lptr->ext.xhi;
	split[3].ylow = lptr->ext.ylow;
	split[3].yhi = ext->ylow - G_frame->ly;

	/*
		return existing item (we saved the tail), and replace
		with new linked list of all rectangles which define
		legal extents
	*/
	GRretobj(lptr);
	for (i=0; i < 4; ++i)
	{
		if (split[i].xlow > split[i].xhi)
			continue;
		if (split[i].ylow > split[i].yhi)
			continue;
		new = GRgetobj(GROB_NULL);
		new->ext.xlow = split[i].xlow;
		new->ext.ylow = split[i].ylow;
		new->ext.xhi = split[i].xhi;
		new->ext.yhi = split[i].yhi;
		new->next = head;
		if (head != NULL)
			head->prev = new;
		head = new;
	}

	return (head);
}
