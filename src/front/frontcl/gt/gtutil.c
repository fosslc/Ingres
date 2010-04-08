
/*
**    Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include <graf.h>

/**
** Name:    util.c	- misc GT utilities
**
** Description:
**	marking and other miscellaneous GT utilities
**
**	GTmrkclr	clear marks on component list
**	GTmrkb		marks bunch on component list
**	GTovlp		checks extents for overlap
**	GTunch		remove component from list
**	GTsetwarn	set last warning message
**
** History:    $Log - for RCS$
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

extern GR_FRAME *G_frame;
extern char G_last[];

/*{
** Name:	GTovlp	- check two extents for overlap
**
** Description:
**	determines if two extents overlap.  The routine "cheats" a bit
**	to avoid deciding that two extents which share a border overlap.
**	The cheat factor is a line width, or .01, if lines are extremely fat.
**
** Inputs:
**	ex1,ex2		extents
**
** Outputs:
**	returns:
**		TRUE if overlap, else FALSE
**
** Side Effects:
**	none
**
** History:
**	8/85 (rlm)	written
*/

bool
GTovlp (ex1,ex2)
EXTENT *ex1,*ex2;
{
	register float xadd,yadd;

	xadd = G_frame->lx;
	yadd = G_frame->ly;

	if (xadd > .01)
		xadd = .01;
	if (yadd > .01)
		yadd = .01;

	if ((ex1->xhi - xadd) < ex2->xlow || (ex1->xlow + xadd) > ex2->xhi)
		return (FALSE);
	if ((ex1->yhi - yadd) < ex2->ylow || (ex1->ylow + yadd) > ex2->yhi)
		return (FALSE);
	return (TRUE);
}

/*{
** Name:	GTmrkclr	- clear marks on component list
**
** Description:
**		clears marks on component list
**
** Inputs:
**	none
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
GTmrkclr ()
{
	GR_OBJ *ptr;

	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
		ptr->scratch = 0;
}

/*{
** Name:	GTmrkb	- mark overlapping group of components
**
** Description:
**		marks components which overlap an input extent, or other
**		components overlapping the extent recursively.  Confines
**		marking and recursive search to a range of layers.
**
** Inputs:
**	ext	initial extent
**	low,hi	range of layers
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
GTmrkb (ext,low,hi)
EXTENT *ext;		/* extent */
i4  low,hi;		/* range of layers to mark */
{
	GR_OBJ *ptr;

	for (ptr = G_frame->head; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->scratch == 0 && ptr->layer >= low &&
				ptr->layer <= hi && GTovlp(&(ptr->ext),ext))
		{
			ptr->scratch = 1;
			GTmrkb (&(ptr->ext),low,hi);
		}
	}
}

/*{
** Name:	GTunch		- removes a component from list
**
** Description:
**	removes a component from the list, updating G_frame->head or
**	G_frame->tail if these are the components being removed, and fixing
**	previous / next links.
**
** Inputs:
**	c	component to remove
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
GTunch (c)
GR_OBJ *c;
{
	if (c->prev == NULL)
		G_frame->head = c->next;
	else
		(c->prev)->next = c->next;
	if (c->next == NULL)
		G_frame->tail = c->prev;
	else
		(c->next)->prev = c->prev;
}

void
GTsetwarn (s)
char *s;
{
	STcopy (s,G_last);
}
