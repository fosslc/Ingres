/*
** Copyright (c) 1984, 2004 Ingres Corporation
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
#include	<osquery.h>
#include	<osmem.h>

/**
** Name:    osqrys.c -	OSL Parser Query Structure Module.
**
** Description:
**	Contains routines used to create and free OSL query nodes ("retrieve"
**	or "select") structures.  Defines:
**
**	osmkquery()	make a query element node.
**	osmkqry()	make a query node.
**	osqryfree()	free a query node.
**
** History:
**	Revision 6.0  87/06  wong
**	Added support of SQL query elements.  03/87  wong
**	Changed to support SQL parenthesized UNION queries.  02/88 wong
**
**	Revision 5.1  86/10/17  16:15:47  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static OSQRY	*qry_alloc();
/*{
** Name:	osmkquery() -	Make a Query Element Node.
**
** Description:
**	Allocates a query element node and sets its values.  A query element
**	node is either the body of the query or an expression whose elements
**	are composed of other query element nodes.
**
** Input:
**	type	{nat}  Node type.
**	op		{char *}  Query node operator or unique modifier.
**	op1		{PTR}  The first operand, either an target list (OSTLIST) or
**					another query element (OSQRY.)
**	op2		{PTR}  The second operand, either an where clause (OSNODE) or
**					another query element (OSQRY) or NULL.
**	from	{OSNODE *}  Node list for from clause (SQL.)
**	groupby	{OSNODE *}  Optional node list for group-by clause (SQL.)
**	having	{OSNODE *}  Optional node for having clause (SQL.)
**
** Returns:
**	{OSQRY *}  Reference to allocated query node.
**
** History:
**	09/86 (jhw) -- Written.
**	03/87 (jhw) -- Added support SQL query elements.
**	02/88 (jhw) -- Changed to support SQL "UNION" query expressions.
*/
/*VARARGS3*/
OSQRY *
osmkquery (type, op, op1, op2, from, groupby, having)
i4		type;
char	*op;
PTR		op1;
PTR		op2;
OSNODE	*from;
OSNODE	*groupby;
OSNODE	*having;
{
	register OSQRY	*qp = qry_alloc();

	qp->qn_type = type;
	if (type == tkQUERY)
	{
		qp->qn_unique = op;
		qp->qn_tlist = ( op1 != NULL )
			? ((OSTLIST *)op1)->tl_next /* point at first */
			: NULL;
		qp->qn_where = (OSNODE *)op2;
		if (from == NULL)
			qp->qn_from = qp->qn_groupby = qp->qn_having = NULL;
		else
		{
			qp->qn_from = from;
			qp->qn_groupby = groupby;
			qp->qn_having = having;
		}
	}
	else if (type == PARENS)
	{
		qp->qn_left = (OSQRY *)op1;
	}
	else if (type == OP)
	{
		qp->qn_op = op;
		qp->qn_left = (OSQRY *)op1;
		qp->qn_right = (OSQRY *)op2;
	}

	return qp;
}

/*{
** Name:	osmkqry() -		Create a Query Node.
**
** Description:
**	Allocates a query node and sets its values.
**
** Input:
**	dml		{DB_LANG}  The DML for the query.
**	qry		{OSQRY *}  The body of the query.
**	sort	{OSSRTNODE *}  Optional node for query sort clause.
**
** Returns:
**	{OSQRY *}  Reference to allocated query node.
**
** History:
**	02/88 (jhw) -- Written.
**	08/88 (jhw) -- Make query node type "tkID" (for 'osqryfree()' below.)
*/
OSQRY *
osmkqry (dml, qry, sort)
DB_LANG		dml;
OSQRY		*qry;
OSSRTNODE	*sort;
{
	register OSQRY	*qp = qry_alloc();

	qp->qn_type = tkID;
	qp->qn_dml = dml;
	qp->qn_body = qry;
	qp->qn_sortlist = sort;

	qp->qn_repeat = qp->qn_cursor = FALSE;

	return qp;
}

/*
** Name:	qry_alloc() -	Allocate Query Node Structure.
**
** Description:
**	Allocate a query node structure from the memory pool.
**
** Returns:
**	{OSQRY *}  A reference to a query node structure.
**
** History:
**	02/88 (jhw) -- Written.
*/

static OSQRY	*free_list = NULL;	/* free list */

static OSQRY *
qry_alloc ()
{
	register OSQRY	*qp;

	if (free_list == NULL)
	{
		STATUS	stat;
#define			NQRY	4

		if ( (qp = (OSQRY*)osAlloc(NQRY * sizeof(*qp), &stat)) == NULL )
			osuerr(OSNOMEM, 1, ERx("osmkquery"));
		free_list = qp;
		while (qp < &free_list[NQRY-1])
		{
			qp->qn_next = qp + 1;
			++qp;
		}
		qp->qn_next = NULL;
	}
	qp = free_list;
	free_list = free_list->qn_next;
	qp->qn_next = NULL;

	return qp;
}

/*{
** Name:	osqryfree() -	Free a Query Node.
**
** Description:
**	Adds a query structure to the free list.
**
** Input:
**	rnode	{OSQRY *}  The node.
**
** History:
**	01/86 (joe) -- Written.
**	06/87 (jhw) -- Check for NULL query node.
**	08/88 (jhw) -- Free parts of query and note special case for "tkID."
*/
VOID
osqryfree (rnode)
register OSQRY	*rnode;
{
	while ( rnode != NULL )
	{
		rnode->qn_next = free_list;
		free_list = rnode;
		if ( rnode->qn_type == tkQUERY )
		{ /* a query node */
			ostlfree(rnode->qn_tlist);
			if ( rnode->qn_from != NULL )
				ostrfree( rnode->qn_from );
			if ( rnode->qn_where != NULL )
				ostrfree( rnode->qn_where );
			if ( rnode->qn_having != NULL )
				ostrfree( rnode->qn_having );
			if ( rnode->qn_groupby != NULL )
				ostrfree( rnode->qn_groupby );
			rnode  = NULL;
		}
		else if ( rnode->qn_type == tkID )
		{ /* a `head' query node */
			rnode = rnode->qn_body;
		}
		else if ( rnode->qn_type == PARENS )
		{
			rnode = rnode->qn_left;
		}
		else if ( rnode->qn_type == OP )
		{
			osqryfree(rnode->qn_right);
			rnode = rnode->qn_left;
		}
		else
		{
			rnode = NULL;
		}
	}
}
