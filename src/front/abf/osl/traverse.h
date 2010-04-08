/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

/**
** Name:	traverse.h -	OSL Parser Expression Tree Traverse Module
**					Interface.
** Description:
**	Contains the definition of the tree traversal application functions
**	structure for the OSL parser, which is passed to 'osqtrvfunc()'.
**	Defines:
**
**	TRV_FUNC	tree traversal application structure.
**
** History:
**	Revision 6.1  88/08  wong
**	Initial revision.
**
**  27-nov-2001 (rodjo04) Bug 106438
**  Added new member state to stucture type TRV_FUNC. 
**/

/*}
** Name:	TRV_FUNC -	OSL Parser Tree Traverse Application Functions.
**
** Description:
**	Defines the structure of the tree traversal application functions
**	object.  Such an object defines the functions to be applied when
**	a tree is traversed.  (These functions evaluate or generate the
**	appropriate output for the expression tree.)
*/
typedef struct
{
	VOID	(*print)();	/* print function */
	VOID	(*eval)();	/* node evaluation function */
	VOID	(*pred)();	/* predicate evaluation function */
	VOID	(*tle)();	/* target list evaluation function */
	VOID	(*name)();	/* name node evaluation function */
        int      state;         /* the current state */
} TRV_FUNC;
