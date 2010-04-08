/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<ostlist.h>
# include	<ossrt.h>

/**
** Name:	osquery.h -	OSL Parser Query Definitions File.
**
** Description:
**	This file contains definitions for the query structures used
**	to support OSL queries (single or master-detail.)
**
** History:
**	Revision 6.0  wong
**	Added query structure elements for SQL.  03/87  wong
**
**	Revision 5.1  86/10/17  16:35:05  arthur
**	Modified.  09/86 wong.
**
**	Revision 4.0  85/08/28  10:10:31  joe
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
** Name:	OSTLNODE -	OSL Compiler Target List ARGV Element.
**
** Description:
**	Structure type definition for target list ARGV element used
**	during query generation for the OSL compiler.  It contains
**	a list of symbols for fields referenced within the target list
**	for a query retrieve.
*/
typedef struct _ostlnode
{
	OSSYM				*tl_sym;	/* symbol for field */
	struct _ostlnode	*tl_next;	/* next in linked list */
} OSTLNODE;

/*}
** Name:	OSQRY -		OSL Parser Query Structure.
**
** Description:
**	Structure type definition holding parsed query ("retrieve" or "select".)
*/

typedef struct _osqry	OSQRY;

struct _osqry
{
	u_i4	qn_type;
	union {
		struct {
			OSSRTNODE	*rn_sortlist;	/* 1st element in sort list */
			OSQRY		*rn_body;		/* query body */
			DB_LANG		rn_dml;			/* DML language for query */
			BITFLD		rn_repeat : 1;	/* REPEAT flag */
			BITFLD		rn_cursor : 1;	/* CURSOR flag */
#define			qn_sortlist	qn_un.qn_qry.rn_sortlist
#define			qn_body		qn_un.qn_qry.rn_body
#define			qn_dml		qn_un.qn_qry.rn_dml
#define			qn_repeat	qn_un.qn_qry.rn_repeat
#define			qn_cursor	qn_un.qn_qry.rn_cursor
		} qn_qry;
		struct {
			char		*rn_unique;		/* unique keyword (e.g., ALL, UNIQUE) */
			OSTLIST		*rn_tlist;		/* target list */
			OSNODE		*rn_from;		/* from list (SQL) */
			OSNODE		*rn_where;		/* expression tree for where clause */
			OSNODE		*rn_groupby;	/* group by list (SQL) */
			OSNODE		*rn_having;		/* having clause (SQL) */
#define			qn_unique	qn_un.qn_query.rn_unique
#define			qn_tlist	qn_un.qn_query.rn_tlist
#define			qn_from		qn_un.qn_query.rn_from
#define			qn_where	qn_un.qn_query.rn_where
#define			qn_groupby	qn_un.qn_query.rn_groupby
#define			qn_having	qn_un.qn_query.rn_having
		} qn_query;
		struct {	/* SQL only */
			char	*rn_op;		/* operator type */
			OSQRY	*rn_left;	/* left child */
			OSQRY	*rn_right;	/* right child */
#define			qn_op		qn_un.qn_union.rn_op
#define			qn_left		qn_un.qn_union.rn_left
#define			qn_right	qn_un.qn_union.rn_right
		} qn_union;
	} qn_un;
	struct _osqry   *qn_next;		/* Next pointer for free list */
};

/* Function declarations */

OSQRY	*osmkquery();
OSQRY	*osmkqry();

/*
** Name:	QRY_CONTEXT -	Query Context Enumeration Type Definitions.
**
** Description:
**	Constants passed to 'osqrytemp()' and 'osqrygen()' to indicate
**	context of query.
*/
#define		OS_SUBMENU	1	/* A query for a sub menu */
#define		OS_CALLF	2	/* A query for a call frame */
#define		OS_QRYSTMT	3	/* A singleton query. */
#define		OS_QRYLOOP	4	/* A query loop. */
