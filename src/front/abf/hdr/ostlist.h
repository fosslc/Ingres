/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    ostlist.h -		OSL Parser Target List Structure Definition.
**
** Description:
**	Contains the definition of a target list structure for the OSL parser.
**	A target list is a circular linked list referenced by the last element
**	in the list.
**
** History:
**	Revision 6.0  87/03/18  15:30:52  wong
**	Removed declarations for 'ostltfld()' and 'ostlform()'.
**	Added declaration for 'ostlall()'.
**
**	Revision 5.1  86/10/15  15:31:41  wong
**	Initial revision.
**/

/*}
** Name:    OSTLIST -		OSL Parser Target List Structure.
**
** Description:
**	A target list element contains references to its target and its value
**	(and optionally, its dimension for CREATE target lists.)  A target is
**	a node representing the target identifier (for query retrieve target
**	lists it is always a node holding a symbol table entry.)  A target value
**	is always a node representing a QUEL expression.
*/
typedef struct _target_list
{
	OSNODE	*tl_target;	/* target (node representing target) */
	OSNODE	*tl_value;	/* target list value */
	OSNODE	*tl_dim;	/* value dimension for
				**	create target list
				*/
	struct _target_list	*tl_next;
} OSTLIST;

/* Function Declarations */

OSTLIST	*osmaketle();
OSTLIST	*osaddtlist();

OSTLIST	*ostlqryall();
OSTLIST	*ostlall();
