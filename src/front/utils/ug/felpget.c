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
** Name:    FElpget() -	get a node
**
**	FEndget returns an available node of the size referred to in
**	the FREELIST block passed to it, which should have been obtained
**	using FElpcreate.  The node will be pulled from the free list.  If
**	the free list is empty, a new block of memory is allocated and
**	chained into the free list first.  This routine and FElpopen use
**	the FE_allocsize variable to pass efficient sizes to
**	allocation routines.
**
**	Parameters:
**		lpp		- FREELIST structure for node pool
**
**	Returns:
**		pointer to node, or NULL for failure
**
**	Assumptions:
**		Some knowledge of how FEreqmem allocates memory
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

PTR
FElpget (lpp)
FREELIST	*lpp;	/* list pool pointer */
{
	register i4  block;		/* number of nodes to grab */
	register struct fl_node *ptr;	/* general purpose pointer */
	register i4  inc;		/* size increment */
	register i4  allowed;		/* allowed bytes in a small block */

	if (lpp->magic != FELP_MAGIC)
		return (NULL);

	if (lpp->l_free == NULL)
	{
		/*
		**	We grab memory for at least FELP_NODES nodes.
		**	If this is < small request boundary, go ahead and
		**	get as many nodes as can fit in an FE_allocsize block.
		*/
		if (FE_allocsize > sizeof(FE_BLK))
			allowed = FE_allocsize-CEILING(sizeof(FE_BLK))*PTRSIZE;
		else
			allowed = ALLOCSIZE-CEILING(sizeof(FE_BLK))*PTRSIZE;
		if ((block = FELP_NODES) * (inc = lpp->nd_size) < allowed)
			block = allowed/inc;

		if ((lpp->l_free = (struct fl_node *)FEreqmem((u_i4)lpp->l_tag,
		    (u_i4)(block * inc), FALSE, (STATUS *)NULL)) == NULL)
		{
			return (NULL);
		}
		
		/*
		**	link every "inc/PTRSIZE" pointers together
		**	into free list.  For loop counts one less than
		**	number of nodes, so that the last entry can be NULL'ed
		*/
		inc /= PTRSIZE;
		ptr = lpp->l_free;
		for (block--; block > 0; block--)
		{
			ptr->l_next = ptr + inc;
			ptr = ptr->l_next;
		}
		ptr->l_next = NULL;
	}

	/*
	**	return first node on list, bumping pointer to next
	*/
	ptr = lpp->l_free;
	lpp->l_free = ptr->l_next;
	MEfill((u_i2) lpp->nd_size, (u_char)'\0', (char *) ptr);
	return ((PTR) ptr);
}
