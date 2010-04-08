# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

/**
**
**  Name: spnext.c -- SPnext function
**
**  Description:
**
**	SPnext function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (canor01)
**	    Clean up prototype for SPdeq().
*/

FUNC_EXTERN SPBLK *SPdeq(SPBLK **np);

/*{
** Name:  "SPnext \- find next node"
**
**	Return the successor of n in t; the successor becomes the root
**	of the tree.
**
** Inputs:
**	n	the current node
**	t	the tree containing t
**
** Outputs:
**	none
**
** Returns:
**	next	the successor node to n, or NULL if it is the tail.
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
*/

SPBLK *
SPnext(SPBLK *n, SPTREE *t)
{
    register SPBLK * next;
    register SPBLK * x;

    t->prevnexts++;

    /* splay version */
    SPsplay( n, t );
    x = SPdeq( &n->rightlink );
    if( x != NULL )
    {
        x->leftlink = n;
        n->uplink = x;
        x->rightlink = n->rightlink;
        n->rightlink = NULL;
        if( x->rightlink != NULL )
	    x->rightlink->uplink = x;
        t->root = x;
        x->uplink = NULL;
    }
    next = x;

    /* shorter slower version;
       deleting last "if" undoes the amortized bound */

# if 0
    SPsplay( n, t );
    x = n->rightlink;
    if( x != NULL )
	while( x->leftlink != NULL )
	    x = x->leftlink;
    next = x;
    if( x != NULL )
	SPsplay( x, t );
# endif

    return( next );

}
