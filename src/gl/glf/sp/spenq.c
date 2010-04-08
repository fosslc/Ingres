# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

/**
**
**  Name: spenq.c -- SPenq function
**
**  Description:
**
**	SPenq function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	28-Jan-92 (daveb)
**	    Null out node links here rather than in clients.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:  "SPenq \- enqueue node after nodes with same key"
**
**	Put n in t after all other nodes with the same key; when this
**	is done, n will be the root of the splay tree representing q,
**	all nodes in t with keys less than or equal to that of n will
**	be in the left subtree, all with greater keys will be in the
**	right subtree; the tree is split into these subtrees from the
**	top down, with rotations performed along the way to shorten
**	the left branch of the right subtree and the right branch of
**	the left subtree
**
** Inputs:
**	n	node to add to the tree.
**	t	tree to insert into.
**
** Outputs:
**	none
**
** Returns:
**	n	the inserted node.
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
**	28-Jan-92 (daveb)
**	    Null out node links here rather than in clients.
*/

SPBLK *
# ifdef CL_HAS_PROTOS
SPenq(SPBLK *n, SPTREE *t)
# else
SPenq( n, t )
register SPBLK * n;
register SPTREE * t;
# endif
{
    register SPBLK * left;	/* the rightmost node in the left tree */
    register SPBLK * right;	/* the leftmost node in the right tree */
    register SPBLK * next;	/* the root of the unsplit part */
    register SPBLK * temp;

    register i4 (*cmp)(const char *, const char *) = t->compare;
    register char *key;

    n->leftlink = NULL;
    n->rightlink = NULL;
    n->uplink = NULL;

    t->enqs++;
    n->uplink = NULL;
    next = t->root;
    t->root = n;
    if( next == NULL )	/* trivial enq */
    {
        n->leftlink = NULL;
        n->rightlink = NULL;
    }
    else		/* difficult enq */
    {
        key = n->key;
        left = n;
        right = n;

        /* n's left and right children will hold the right and left
	   splayed trees resulting from splitting on n->key;
	   note that the children will be reversed! */

	t->enqcmps++;
        if ( (*cmp)( next->key, key ) > 0 )
	    goto two;

    one:	/* assert next->key <= key */

	do	/* walk to the right in the left tree */
	{
            temp = next->rightlink;
            if( temp == NULL )
	    {
                left->rightlink = next;
                next->uplink = left;
                right->leftlink = NULL;
                goto done;	/* job done, entire tree split */
            }

	    t->enqcmps++;
            if( (*cmp)( temp->key, key ) > 0 )
	    {
                left->rightlink = next;
                next->uplink = left;
                left = next;
                next = temp;
                goto two;	/* change sides */
            }

            next->rightlink = temp->leftlink;
            if( temp->leftlink != NULL )
	    	temp->leftlink->uplink = next;
            left->rightlink = temp;
            temp->uplink = left;
            temp->leftlink = next;
            next->uplink = temp;
            left = temp;
            next = temp->rightlink;
            if( next == NULL )
	    {
                right->leftlink = NULL;
                goto done;	/* job done, entire tree split */
            }

	    t->enqcmps++;

	} while( (*cmp)( next->key, key ) <= 0 ); /* change sides */

    two:	/* assert next->key > key */

	do	/* walk to the left in the right tree */
	{
            temp = next->leftlink;
            if( temp == NULL )
	    {
                right->leftlink = next;
                next->uplink = right;
                left->rightlink = NULL;
                goto done;	/* job done, entire tree split */
            }

	    t->enqcmps++;
            if( (*cmp)( temp->key, key ) <= 0 )
	    {
                right->leftlink = next;
                next->uplink = right;
                right = next;
                next = temp;
                goto one;	/* change sides */
            }
            next->leftlink = temp->rightlink;
            if( temp->rightlink != NULL )
	    	temp->rightlink->uplink = next;
            right->leftlink = temp;
            temp->uplink = right;
            temp->rightlink = next;
            next->uplink = temp;
            right = temp;
            next = temp->leftlink;
            if( next == NULL )
	    {
                left->rightlink = NULL;
                goto done;	/* job done, entire tree split */
            }

	    t->enqcmps++;

	} while( (*cmp)( next->key, key ) > 0 );	/* change sides */

        goto one;

    done:	/* split is done, branches of n need reversal */

        temp = n->leftlink;
        n->leftlink = n->rightlink;
        n->rightlink = temp;
    }

    return( n );

}
