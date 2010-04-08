# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**
**  Name: spdeq.c -- SPdeq function
**
**  Description:
**
**	SPdeq function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

/*{
** Name:  "SPdeq \- dequeue head node"
**
**	Remove and return the head node from a subtree; this deletes (and
**	returns) the leftmost node from subtree, replacing it with its right
**	subtree (if there is one); on the way to the leftmost node, rotations
**	are performed to shorten the left branch of the tree
**
** Inputs:
**	np	pointer to a variable holding the root of the subtree.
**
** Outputs:
**	none
**
** Returns:
**	node	the leftmost node in the subtree, or NULL if the
**		tree is empty.
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
*/
    
SPBLK *
# ifdef CL_HAS_PROTOS
SPdeq(SPBLK **np)
# else
SPdeq( np )
SPBLK **np;		/* pointer to a node pointer */
# endif
{
    register SPBLK * deq;		/* one to return */
    register SPBLK * next;       	/* the next thing to deal with */
    register SPBLK * left;      	/* the left child of next */
    register SPBLK * farleft;		/* the left child of left */
    register SPBLK * farfarleft;	/* the left child of farleft */

    if( np == NULL || *np == NULL )
    {
        deq = NULL;
    }
    else
    {
        next = *np;
        left = next->leftlink;
        if( left == NULL )
	{
            deq = next;
            *np = next->rightlink;

            if( *np != NULL )
		(*np)->uplink = NULL;

        }
	else for(;;)	/* left is not null */
	{
            /* next is not it, left is not NULL, might be it */
            farleft = left->leftlink;
            if( farleft == NULL )
	    {
                deq = left;
                next->leftlink = left->rightlink;
                if( left->rightlink != NULL )
		    left->rightlink->uplink = next;
		break;
            }

            /* next, left are not it, farleft is not NULL, might be it */
            farfarleft = farleft->leftlink;
            if( farfarleft == NULL )
	    {
                deq = farleft;
                left->leftlink = farleft->rightlink;
                if( farleft->rightlink != NULL )
		    farleft->rightlink->uplink = left;
		break;
            }

            /* next, left, farleft are not it, rotate */
            next->leftlink = farleft;
            farleft->uplink = next;
            left->leftlink = farleft->rightlink;
            if( farleft->rightlink != NULL )
		farleft->rightlink->uplink = left;
            farleft->rightlink = left;
            left->uplink = farleft;
            next = farleft;
            left = farfarleft;
	}
    }

    return( deq );

}
