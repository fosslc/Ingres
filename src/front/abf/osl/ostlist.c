/*
** Copyright (c) 1984, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<ostlist.h>
#include	<fdesc.h>
#include	<oskeyw.h>

/**
** Name:	ostlist.c -	OSL Parser Target List Module.
**
** Description:
**	Contains routines used to create target list elements and to
**	process a target list.  Defines:
**
**	osmaketle()	make a dml target list element node.
**	osaddtlist()	concatenate two target list.
**	ostltrvfunc()	traverse a target list.
**	osevaltlist()	evaluate a target list.
**	ostlfree()	free a target list.
**	ostlqryall()	generate a query retrieve target list for a form object.
**	ostlall()	generate dml target list for symbol node list.
**
** History:
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Small fix in ostlqryall (followup to fix for bug 34590).
**
**	Revision 6.4
**	03/09/91 (emerson)
**		Integrated DESKTOP porting changes
**              (Changed identifier 'free' to 'oslfree').
**
**	Revision 6.3/03/00
**	11/14/90 (emerson)
**		Check for global variables (OSGLOB)
**		as well as local variables (OSVAR).  (Bug 34415).
**	11/24/90 (emerson)
**		Fix for bug 34590 in function ostlqryall.
**
**	Revision 6.0  87/06  wong
**	Abstracted 'ostlform()', 'ostltfld()', and 'ossymnode()'
**	functionality into 'ostlall()' and 'osformall()'.  87/03
**	Changes for symbol table.  87/01
**
**	Revision 5.1  86/12/04  11:02:42  wong
**	Modified to create target list for ".all" case when form object is
**	undefined (passed query.)
**	Initial revision.  (86/09/22  wong)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static FREELIST	*listp = NULL;		/* list pool */
static OSTLIST	*oslfree = NULL;		/* free list */

/*{
** Name:	osmaketle() -	Make DML Target List Element Node.
**
** Description:
**	This routine creates and returns a target list element that holds the
**	parts of a DML target list element (element name and value and possible
**	create dimension.)  The parts are represented by tree nodes and
**	placed in a the target list element structure.
**
**	Note that none of the parts should require expression evaluation (i.e.,
**	they are only name, constant or QUEL expression nodes.)
**
** Input:
**	atarget		{OSNODE *}  Node representing target.
**				(Always a VALUE node for query retrieve target lists.)
**	avalue		{OSNODE *}  Node holding target list value.
**	adim		{OSNODE *}  Node holding possible target list dimension.
**
** Returns:
**	OSNODE *	Tree node holding a target list element.
**
** History:
**	09/86 (jhw) -- Written.
*/
OSTLIST *
osmaketle (atarget, avalue, adim)
OSNODE	*atarget;
OSNODE	*avalue;
OSNODE	*adim;
{
	register OSTLIST	*tle;

	if (oslfree != NULL)
	{ /* get from free list */
		tle = oslfree;
		oslfree = oslfree->tl_next;
	}
	else if ((listp == (FREELIST *)NULL &&
			(listp = (FREELIST *)FElpcreate(sizeof(*tle))) == NULL) ||
				(tle = (OSTLIST *)FElpget(listp)) == NULL)
		osuerr(OSNOMEM, 1, ERx("osmaketle"));

	tle->tl_target = atarget;
	tle->tl_value = avalue;
	tle->tl_dim = adim;
	tle->tl_next = tle;

	return tle;
}

/*{
** Name:	osaddtlist() -	Concatenate Two Target List.
**
** Description:
**	This routine takes two target list as input and concatenates them
**	together.  Note target lists are circular lists that refer to the
**	last element of the list.
**
** Input:
**	list1	{OSTLIST *}  One target list.
**	list2	{OSTLIST *}  The other target list.
**
** History:
**	09/86 (jhw) -- Written.
**	04/87 (jhw) -- Modified to ignore query order.
**	01/90 (jhw) -- Corrected concatenation of 'list2', which was only
**		concatenating the last element, not the list.  JupBug #9816.
*/
OSTLIST *
osaddtlist (list1, list2)
register OSTLIST	*list1;
register OSTLIST	*list2;
{
	register OSTLIST	*temp;

	if (list1 == NULL)
		return list2;
	if (list2 == NULL)
		return list1;

	temp = list1->tl_next;
	list1->tl_next = list2->tl_next;
	list2->tl_next = temp;

	return list2;
}

/*{
** Name:		ostltrvfunc() -	Traverse a Target List.
**
** Description:
**	Traverse a target list applying the input function to each
**	element of the target list.  The list is either circular or
**	null-terminated.
**
** Input:
**	list	{OSTLIST *}  The target list.
**	ev_func	{(*)()}  The function.
**
** History:
**	09/86 (jhw) -- Written.
*/
VOID
ostltrvfunc (list, ev_func)
register OSTLIST	*list;
register i4		(*ev_func)();
{
	register OSTLIST	*lp;

	if ((lp = list) != NULL)
		do
		{
			(*ev_func)(lp->tl_target, lp->tl_value, lp->tl_dim,
							(lp->tl_next != list)
			);
		} while ((lp = lp->tl_next) != NULL && lp != list);
}

/*{
** Name:	osevaltlist() -	Evaluate a DML Target List.
**
** Description:
**	Evaluate a database target list by applying the input evaluation
**	function to each element of the list.  The target list is assumed
**	to be a circular linked list starting with the last element.
**
**	This routine frees the target list (by adding it to the free list.)
**
** Input:
**	list	{OSTLIST *}  The circular linked target list.
**	ev_func	{nat (*)()}  The evaluation function.
**
** History:
**	09/86 (jhw) -- Written.
*/
VOID
osevaltlist (list, ev_func)
register OSTLIST	*list;
register i4		(*ev_func)();
{
	register OSTLIST	*lp;

	if ((lp = list) != NULL)
	{
		list = list->tl_next;	/* set to first */
		do
		{
			lp = lp->tl_next;
			(*ev_func)(lp->tl_target, lp->tl_value, lp->tl_dim,
						(lp->tl_next != list)
			);
			lp->tl_target = NULL;
			lp->tl_value = lp->tl_dim = NULL;
		} while (lp->tl_next != NULL && lp->tl_next != list);

		/* add to free list */
		lp->tl_next = oslfree;
		oslfree = list;
	}
}

/*{
** Name:	ostlfree() -	Free a DML Target List.
**
** Description:
**	This routine frees a database target list by adding its elements
**	to the free list.
**
** Input:
**	list	{OSTLIST *}  The circular linked target list.
**
** History:
**	09/86 (jhw) -- Written.
*/
VOID
ostlfree (list)
register OSTLIST	*list;
{
	register OSTLIST	*first;

	if (list != NULL)
	{
		first = list->tl_next;
		list->tl_next = oslfree;
		oslfree = first;
	}
}

/*{
** Name:	ostlqryall() -	Generate a Query Retrieve Target List
**					for a Form Object.
** Description:
**	Generates a query retrieve target list for all the fields or columns
**	defined for a form object.  Each target list element consist of a node
**	representing the field (or column) being retrieved into and a node
**	describing the object being retrieved (a "relation . attribute" symbol
**	reference.)
**
**	These semantics support "form object . all" in query retrieve target
**	lists.
**
** Input:
**	qrytarg		{OSNODE *}  The node for the form object.
**	formobj		{OSSYM *}  The symbol table entry for the form object.
**	relation	{char *}  The assumed name of the relation from which
**					values are to be retrieved.
**
** History:
**	09/86 (jhw) -- Written.
**	07/87 (jhw) -- Added support for SQL, '*'.
**	12/89 (jhw) -- Added support for complex objects.
**	11/24/90 (emerson)
**		Added input parm qrytarg; revamped handling of handling
**		of record attributes (bug 34590).  Note: record attributes
**		are the children of formobj when formobj represents a record
**		or array.  It appears that this routine assumes that
**		relation is always '*' for such objects.
**	09/20/92 (emerson)
**		Cleaned up the handling of record attributes a bit:
**
**		First, in the case of a "*" relation, set value to NULL
**		(as for fields of a form or columns of a tablefield),
**		instead of setting value = osmkident(tp->s_name, NULL).
**		The effect of this is that the resulting query is now
**		"select col1, col2, .. from ..." instead of
**		"select col1 as col1, col2 as col2, .. from ...".
**
**		Second, in the case of a non-"*" relation, also compute value
**		as it was already being computed for forms and tablefields.
**		[Correction to the previous history log entry:  It appears that
**		a non-* relation *is* possible even when formobj represents
**		a record or array.  A non-* relation arises from the
**		undocumented syntax "qrytarg = SELECT tablename.*
**		FROM tablename ...".  It appears that this is supposed to
**		behave the same as "qrytarg = SELECT * FROM tablename ..."
**		except the resulting query is "select tablename.col1 as col1,
**		tablename.col2 as col2, ... from tablename ..." instead of
**		"select col1, col2, ... from tablename ...".]
**
**		Changed calling sequence to osqrydot again.
*/

OSTLIST *
ostlqryall ( qrytarg, formobj, relation )
register OSNODE	*qrytarg;
register OSSYM	*formobj;
char		*relation;
{
	register OSSYM		*tp;
	register OSTLIST	*tlist = NULL;

	if (formobj == NULL)
	{
		U_ARG	objp;
		U_ARG	attrp;

		objp.u_cp = relation;
		attrp.u_cp = _All;
		tlist = osmaketle( osmknode(ATTR, &objp, &attrp, (U_ARG*)NULL),
				   (OSNODE*)NULL, (OSNODE*)NULL
		);
		return tlist;
	}

	if ( formobj->s_kind == OSFORM )
		tp = formobj->s_fields;
	else if ( formobj->s_kind == OSTABLE )
		tp = formobj->s_columns;
	else if ( ( formobj->s_kind == OSVAR
			|| formobj->s_kind == OSGLOB
			|| formobj->s_kind == OSRATTR )
		&& (formobj->s_flags & (FDF_RECORD|FDF_ARRAY)) != 0 )
	{ /* class or array object */
		tp = formobj->s_attributes;
	}
	else
		osuerr(OSBUG, 1, ERx("ostlqryall"));


	for ( ; tp != NULL ; tp = tp->s_sibling )
	{
		if ( tp->s_kind == OSFIELD 
		   || tp->s_kind == OSCOLUMN
		   || ( tp->s_kind == OSRATTR
		     && !(tp->s_flags & (FDF_RECORD|FDF_ARRAY|FDF_READONLY)) ) )
		{ 
			/* fields, columns, or simple attributes only */
			register OSTLIST	*tle;
			OSNODE			*ele;
			U_ARG			symp;
			OSNODE			*value = NULL;

			symp.u_symp = tp;
			if ( *relation != '*' )
			{
				U_ARG	namp;
				U_ARG	attp;

				namp.u_cp = relation;
				attp.u_cp = tp->s_name;
				value = osmknode( ATTR, &namp, &attp,
					(U_ARG*)NULL );
			}
			if ( tp->s_kind == OSRATTR )
			{
				ele = os_lhs( osqrydot( qrytarg, tp ) );
			}
			else
			{
				ele = osmknode( VALUE, &symp,
					(U_ARG*)NULL, (U_ARG*)NULL );
			}
			tle = osmaketle( ele, value, (OSNODE *)NULL );
			tlist = ( tlist == NULL )
					? tle : osaddtlist(tlist, tle);
		}
	} /* end for */
	
	return tlist;
}

/*{
** Name:	ostlall() -	Generate Target List for Symbol Node List.
**
** Description:
**	This routine generates a target list for all the fields of a form or
**	the columns of a table field row passed in as a node list of value
**	references.
**
**	Each target list element for a field consist of a node holding the name
**	of the attribute (which is the same as the name of the field or column)
**	and a node referencing the field value for the form or the column value
**	for the table field row.
**
**	These semantics support "form . all" or "table field . all"
**	in DML target lists.
**
** Input:
**	nlist	{OSNODE *}  Node list of form field or table field column
**				references.
**
** Returns:
**	{OSTLIST *}	A DML target list for the form or table field row.
**
** History:
**	09/86 (jhw) -- Written.
**	03/87 (jhw) -- Generalized for form as well as table fields.
*/

OSTLIST *
ostlall (nlist)
OSNODE	*nlist;
{
	register OSTLIST *tlist = NULL;

	if (nlist != NULL)
	{
		register OSNODE	*lp;

		for (lp = nlist ; lp != NULL ; lp = lp->n_next)
		{
			register OSTLIST	*tle;
			register OSNODE		*np;

			np = lp->n_ele;
			tle = osmaketle(
				osmkident(
				    (np->n_tfref == NULL
					? np->n_sym : np->n_tfref)->s_name,
					(OSNODE *)NULL
				),
				np, (OSNODE *)NULL
			);
			tlist = (tlist == NULL ? tle : osaddtlist(tlist, tle));
			lp->n_ele = NULL;
		}
		ostrfree(nlist);
	}

	return tlist;
}
