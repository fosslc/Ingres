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
** FElpdestroy - destroy a list pool
**
**	FElpdestroy is called to indicate that a pool of list nodes will
**	no longer be used.  It frees the tagged storage associated with the
**	pool.
**
**	Parameters:
**		lpp	- FREELIST pointer associated with pool
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
**		8/92 (Mike S) - Use IIUGtagFree to free up tag. (Bug 46156)
**		1-oct-92 (leighb) DeskTop Porting Change:
**			Correct above change to not use 'lpp' after it 
**			has been freed.  Using a freed segment crashes in
**			Protected Mode on an 80x86.
*/

STATUS
FElpdestroy (lpp)
FREELIST	*lpp;	/* list pool pointer */
{
	TAGID	tag;					 

	if (lpp->magic != FELP_MAGIC)
		return (FAIL);
	tag = lpp->l_tag;				 

	/*
	**	zero magic number and tag number, just in case
	*/
	lpp->magic = 0;					 
	lpp->l_tag = 0;					 

	IIUGtagFree(tag);
	return OK;
}
