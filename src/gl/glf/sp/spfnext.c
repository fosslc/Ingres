# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**
**  Name: spfnext.c -- SPfnext function
**
**  Description:
**
**	SPfnext function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

/*{
** Name:  "SPfnext \- fast-find next"
**
**	Return the successor of n in t; This is a fast (on average)
**	version that does not splay, which may leave the tree
**	under-balanced.
**
** Inputs:
**	n	current node
**
** Outputs:
**	none
**
** Returns:
**	next	the node following n, or NULL if n is the tail.
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
*/
    
SPBLK *
# ifdef CL_HAS_PROTOS
SPfnext(SPBLK *n)
# else
SPfnext( n )
register SPBLK * n;
# endif
{
    register SPBLK * next;
    register SPBLK * x;

    /* a long version, avoids splaying for fast average,
     * poor amortized bound
     */

    if( n == NULL )
        return( n );

    x = n->rightlink;
    if( x != NULL )
    {
        while( x->leftlink != NULL )
	    x = x->leftlink;
        next = x;
    }
    else	/* x == NULL */
    {
        x = n->uplink;
        next = NULL;
        while( x != NULL )
	{
            if( x->leftlink == n )
	    {
                next = x;
                x = NULL;
            }
	    else
	    {
                n = x;
                x = n->uplink;
            }
        }
    }

    return( next );
}
