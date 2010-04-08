
# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**
**  Name: spprev.c -- SPprev function
**
**  Description:
**
**	SPprev function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

/*{
** Name:  "SPprev \- find previous node"
**	Return the predecessor of n in t; the predecessor becomes the
**	root of the tree.
**
** Inputs:
**	n	the current node.
**	t	the tree containing n.
**
** Outputs:
**	none
**
** Returns:
**	prev	the predecessor to n, or NULL if n is the head.
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
*/

SPBLK *
# ifdef CL_HAS_PROTOS
SPprev(SPBLK *n, SPTREE *t)
# else
SPprev( n, t )
register SPBLK * n;
register SPTREE * t;
# endif
{
    register SPBLK * prev;
    register SPBLK * x;
    
    /* splay version;
       note: deleting the last "if" undoes the amortized bound */

    t->prevnexts++;
    
    SPsplay( n, t );
    x = n->leftlink;
    if( x != NULL )
	while( x->rightlink != NULL )
	    x = x->rightlink;
    prev = x;
    if( x != NULL )
	SPsplay( x, t );
    
    return( prev );
    
}
