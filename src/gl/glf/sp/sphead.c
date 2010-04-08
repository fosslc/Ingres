# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

/**
**
**  Name: sphead.c -- SPhead function
**
**  Description:
**
**	SPhead function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (canor01)
**	    Clean up prototype for SPdeq().
*/

/*{
** Name:  "SPhead - find head"
**
**	Returns the head node in the tree t; t->root ends up pointing
**	to the head node, and the old left branch of t is shortened,
**	as if t had been splayed about the head element; this is done
**	by dequeueing the head and then making the resulting queue the
**	right son of the head returned by SPdet;
**
**	SPfhead is immediately faster, but doesn't help the amortized
**	performance.
**
** Inputs:
**	t	tree in question
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

/* original comment:
 *
 * SPhead() --	return the "lowest" element in the tree.
 *
 *	returns a reference to the head event in the event-set q,
 *	represented as a splay tree; t->root ends up pointing to the head
 *	event, and the old left branch of t is shortened, as if q had
 *	been splayed about the head element; this is done by dequeueing
 *	the head and then making the resulting queue the right son of
 *	the head returned by SPdet; an alternative is provided which
 *	avoids splaying but just searches for and returns a pointer to
 *	the bottom of the left branch
 */

FUNC_EXTERN SPBLK *SPdeq(SPBLK **np);

SPBLK *
SPhead( SPTREE *t )
{
    register SPBLK * x;
    
    /* splay version, good amortized bound */
    x = SPdeq( &t->root );
    if( x != NULL )
    {
        x->rightlink = t->root;
        x->leftlink = NULL;
        x->uplink = NULL;
        if( t->root != NULL )
	    t->root->uplink = x;
    }
    t->root = x;
    
    /* alternative version, bad amortized bound,
       but faster on the average */
    
# if 0
    x = t->root;
    while( x->leftlink != NULL )
	x = x->leftlink;
# endif
    
    return( x );
    
}
