/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"felp.h"
# include	"feloc.h"

/*{
** Name:    FElpcreate() -	create a list pool
**
**	FElpcreate initializes a status structure which will later be used
**	to dynamically manage a free list of nodes via calls to FElpget
**	and FElpret.  The status structure basically contains the size
**	of the nodes on the list, used only when allocating another chain
**	of them onto the free list, a free list pointer, and a tag. Since
**	passing bad status pointers might not cause immediate failures,
**	and be hard to trace, the structure also contains an element which
**	is set to an unlikely value by this routine, a value which will
**	be checked for in FElpdestroy, FElpget and FElpret.
**
**	Parameters:
**		size		- size of the nodes on this list.
**
**	Returns:
**		FREELIST pointer to associate with the pool, NULL
**		for failure
**
**	Assumptions:
**		Some knowledge of how FEtalloc allocates memory
**		is reflected in here to make efficient use of it.
**
**	Side Effects:
**		None.
**
**	History:
**		1/14/85 (rlm) - First written.
**		13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**		29-jul-1992 (rdrane)
**			Modify declaration of FE_allocsize from "extern i4" to
**			"GLOBALREF i4".
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF i4 FE_allocsize;

FREELIST *
FElpcreate (size)
i2		size;		/* size of node */
{
	TAGID	tag;			/* storage tag */
	PTR	st;			/* returned storage address */
	register FREELIST *lpp;		/* structure created and returned */
	register i4  block;		/* storage block size */
	register struct fl_node *ptr;	/* pointer to link free list */

	if (size <= 0)
		return (NULL);

	size = CEILING(size) * PTRSIZE;

	/*
	**	get a list pool pointer structure
	**	we also link the rest of an FE_allocsize block of memory
	**	into an initial free list, since FEreqmem will give it
	**	to us anyway
	*/
	tag = FEgettag();
	if (FE_allocsize > sizeof(FE_BLK))
		block = FE_allocsize;
	else
		block = ALLOCSIZE;
	block -= CEILING(sizeof(FREELIST))*PTRSIZE + CEILING(sizeof(FE_BLK))*PTRSIZE;
	if (block < size)
		block = 0;	/* nodes too large */
	else
		block = (block/size)*size;
	block += CEILING(sizeof(FREELIST))*PTRSIZE;
	if ((st = FEreqmem((u_i4)tag, (u_i4)block, FALSE,
	    (STATUS *)NULL)) == NULL)
	{
		return (NULL);
	}

	lpp = (FREELIST *) st;
	lpp->nd_size = size;
	lpp->l_tag = tag;

	block -= CEILING(sizeof(FREELIST))*PTRSIZE;
	block /= size;
	if (block > 0)
	{
		ptr = (struct fl_node *) st;
		ptr += CEILING(sizeof(FREELIST));
		size /= PTRSIZE;
		lpp->l_free = ptr;
		for (block--; block > 0; block--)
		{
			ptr->l_next = ptr + size;
			ptr = ptr->l_next;
		}
		ptr->l_next = NULL;
	}
	else
		lpp->l_free = NULL;
	lpp->magic = FELP_MAGIC;
	return (lpp);
}
