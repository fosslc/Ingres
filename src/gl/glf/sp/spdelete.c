# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

/**
**
**  Name: spdelete.c -- SPdelete function
**
**  Description:
**
**	SPdelete function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (canor01)
**	    Clean up compiler warning.
*/

/*{
** Name:  "SPdelete - delete node by key"
**
**	The node is deleted from the tree; the resulting splay tree has been
**	splayed around its new root, which is the successor of the node.
**
** Inputs:
**	n	the node to delete
**	t	the tree containing the node.
**
** Outputs:
**	None
**
** Returns:
**	none
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
*/

FUNC_EXTERN SPBLK *SPdeq(SPBLK **np);

VOID
# ifdef CL_HAS_PROTOS
SPdelete( SPBLK *n, SPTREE *t )
# else
SPdelete( n, t )
register SPBLK * n;
register SPTREE * t;
# endif
{
    register SPBLK * x;
    
    SPsplay( n, t );
    x = SPdeq( &t->root->rightlink );
    if( x == NULL )		/* empty right subtree */
    {
        if( (t->root = t->root->leftlink) != NULL )
	    t->root->uplink = NULL;
    }
    else			/* non-empty right subtree */
    {
        x->uplink = NULL;
        x->leftlink = t->root->leftlink;
        x->rightlink = t->root->rightlink;
        if( x->leftlink != NULL )
	    x->leftlink->uplink = x;
        if( x->rightlink != NULL )
	    x->rightlink->uplink = x;
        t->root = x;
    }
}

