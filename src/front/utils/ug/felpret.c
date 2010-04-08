/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
#include	"felp.h"

/*
** FElpret - return a node to a free list
**
**	FElpret returns a node to the freelist pointed to by the
**	list pool pointer passed to it.  The pointer should have been obtained
**	from FElpopen, and the returned node obtained from FElpget.
**
**	Parameters:
**		lpp		- FREELIST structure for node pool
**		node		- node to return to free list.
**
**	Returns:
**		OK or FAIL
**
**	Assumptions:
**		None.
**
**	Side Effects:
**		None.
**
**	History:
**		1/14/85 (rlm) - First written.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS
FElpret (lpp,node)
FREELIST	*lpp;	/* structure to initialize */
i4		*node;		/* returned node */
{
	register struct fl_node *ptr;

	if (lpp->magic != FELP_MAGIC)
		return (FAIL);

	ptr = (struct fl_node *) node;
	ptr->l_next = lpp->l_free;
	lpp->l_free = ptr;
	return (OK);
}
