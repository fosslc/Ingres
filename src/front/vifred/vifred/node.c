/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<er.h>
# include	<ug.h>
# include	"ervf.h"

/*
** node.c
**
** Contains all routines which deal with the node structures.
**
** History:
**	12/21/84 (drh)	Changed to allocate using line table storage tag.
**			Added ndinit to initialize the node free list after
**			tagged storage free.
**	01/16/85 (drh)	Use FEtcalloc instead of MEtcalloc for node allocation.
**	13-jul-87 (bab) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


/*
** Return the node pointer for the node which is adjacent
** to nd on line y
*/


VFNODE *
vflnNext(nd, y)
VFNODE	*nd;
i4	y;
{
	return (*vflnAdrNext(nd, y));
}

/*
** return the address of the node pointer for the node adjacent to
** nd on line y
*/


VFNODE **
vflnAdrNext(nd, y)
VFNODE	*nd;
i4	y;
{
	POS	*ps = nd->nd_pos;
	i4	i;
	LIST	*adj;

	for (adj = nd->nd_adj, i = ps->ps_begy;
	     adj != NULL && i < y; i++, adj = adj->lt_next)
		;
	if (adj == NULL)
		syserr(ERget(S_VF0082_vflnNext__Can_t_get_a), y, nd);
	return (&adj->lt_node);
}

# define	NODEEXTENT	50

static VFNODE	*freelist = NULL;

/*
** get a new node, and allocate an adjacency list 
*/
VFNODE *
ndalloc(ps)
POS	*ps;
{
	VFNODE	*ndget();
	LIST	**lp;
	i4	i;
	VFNODE	*nd;

	nd = ndget();
	nd->nd_pos = ps;
	lp = &nd->nd_adj;
	for (i = ps->ps_begy; i <= ps->ps_endy; i++)
	{
		*lp = lalloc(LNODE);
		lp = &((*lp)->lt_next);
	}
	return (nd);
}

/*
** get a free node.. from free list if available, else allocate more
**	freelist nodes and return one of those
*/
VFNODE *
ndget()
{
	register VFNODE *nodp;
	register i4	i;

	if (freelist == NNIL)
	{
		freelist = (VFNODE *)FEreqmem((u_i4)vflinetag,
		    (u_i4)(NODEEXTENT * sizeof(VFNODE)), TRUE, (STATUS *)NULL);
		if (freelist == NULL)
			IIUGbmaBadMemoryAllocation(ERx("ndalloc"));
		for (nodp = freelist, i = 1; i < NODEEXTENT; i++, nodp++)
			nodp->nd_adj = (LIST *) (nodp + 1);
	}
	nodp = freelist;
	freelist = (VFNODE *) nodp->nd_adj;
	nodp->nd_adj = NULL;
	return (nodp);
}

/*
** free up a node element and it's adjacency list
*/

VOID
ndfree(nd)
register VFNODE *nd;
{
	register char	*cp;
	register i4	i;
	LIST		*lt,
			*lp;

	for (lt = nd->nd_adj; lt != NULL;)
	{
		lp = lt;
		lt = lt->lt_next;
		lfree(lp);
	}
	for (cp = (char *)nd, i = 0; i < sizeof(VFNODE); i++)
		*cp = '\0';
	nd->nd_adj = (LIST *) freelist;
	freelist = nd;
}

/*
**  Initialize the free list after free of all line table storage
*/

VOID
ndinit()
{
	freelist = NULL;
	return;
}
