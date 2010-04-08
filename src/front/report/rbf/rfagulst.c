/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
** Name:	rfagulst.c - routines for manipulating the copt agu_list.
**
** Description:
**	This file defines:
**	rFagu_append	-	Append an unique agg field to the agu_list.
**	rFagu_remove 	-	Remove an unique agg field from the agu_list.
**
** History:
**	19-apr-90 (cmr)	written.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
VOID
rFagu_append(f, att)
FIELD	*f;
i4	att;
{
	LIST	*list, *p, *q;
	COPT	*copt;

	list = lalloc(LFIELD);
	list->lt_field = f;
	list->lt_utag = DS_PFIELD;
	/* append this field to the agu_list */
	copt = rFgt_copt(att);
	p = copt->agu_list;
	while (p)
	{
		q = p;
		p = p->lt_next;
	}
	if (p == copt->agu_list)	/* empty list */
		copt->agu_list = list;
	else
		q->lt_next = list;
	return;
}
VOID
rFagu_remove(agg_u, ord)
LIST	*agg_u;
i4	ord;
{
	LIST	*p, *q;
	COPT	*copt;

	copt = rFgt_copt(ord);
	p = copt->agu_list;
	while (p && p != agg_u)
	{
		q = p;
		p = p->lt_next;
	}
	if (!p)		/* empty list or agg_u not found */
		return;
	if (p == copt->agu_list)	/* one and only agg on the list */
		copt->agu_list = NULL;
	else
		q->lt_next = p->lt_next;
	lfree(p);
	return;
}
