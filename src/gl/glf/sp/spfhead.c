# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**
**  Name: spfhead.c -- SPfhead function
**
**  Description:
**
**	SPfhead function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/


/*{
** Name:  "SPfhead \- fast-find head"
**
**	Returns a reference to the head event in the tree. avoiding
**	splaying.  Excessive use of this may leave the tree
**	under-balanced.  It just searches for and returns a pointer to
**	the bottom of the left branch.
**
** Inputs:
**	n	the current node
**
** Outputs:
**	none
**
** Returns:
**	head	the first node in the tree, or NULL if the tree is empty.
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
*/

SPBLK *
# ifdef CL_HAS_PROTOS
SPfhead(SPTREE *t)
# else
SPfhead( t )
register SPTREE * t;
# endif
{
    register SPBLK * x;

    if( NULL != ( x = t->root ) )
	while( x->leftlink != NULL )
	    x = x->leftlink;

    return( x );

}
