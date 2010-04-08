# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**
**  Name: spfprev.c -- SPfprev function
**
**  Description:
**
**	SPfprev function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

/*{
** Name:  "SPfprev \- fast-find previous"
**
**	Return the predecessor of n in t; This is a fast (on average)
**	version that does not splay, which may leave the tree
**	under-balanced.
**
** Inputs:
**	n	the current node
**
** Outputs:
**	none
**
** Returns:
**	prev	the previous node, or NULL if n is the head.
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
*/

SPBLK *
# ifdef CL_HAS_PROTOS
SPfprev(SPBLK *n)
# else
SPfprev( n )
register SPBLK * n;
# endif
{
    register SPBLK * prev;
    register SPBLK * x;

    /* a long version,
     * avoids splaying for fast average, poor amortized bound
     */

    if( n == NULL )
        return( n );

    x = n->leftlink;
    if( x != NULL )
    {
        while( x->rightlink != NULL )
	    x = x->rightlink;
        prev = x;
    }
    else
    {
        x = n->uplink;
        prev = NULL;
        while( x != NULL )
	{
            if( x->rightlink == n )
	    {
                prev = x;
                x = NULL;
            }
	    else
	    {
                n = x;
                x = n->uplink;
            }
        }
    }

    return( prev );
}

