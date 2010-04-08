/*
**	static	char	Sccsid[] = "@(#)node.h	1.2	2/6/85";
*/

/*
** node.h
**
** Describes the structure of the graph node used in the Lines
** table.
**
** Copyright (c) 2004 Ingres Corporation
*/


/*
** the nd_adj field points to a list of type LNODE
** There is one list node for each line the position occupies.
*/
typedef struct nodeType
{
	i2	nd_tag;			/* not used */
	i2	nd_mask;		/* level marker used for traversal */
	POS	*nd_pos;		/* the position this node points to */
	LIST	*nd_adj;		/* The adjacency list */
} VFNODE;

# define	NNIL		((VFNODE *) 0)
