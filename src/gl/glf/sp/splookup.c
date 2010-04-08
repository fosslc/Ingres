# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**
**  Name: splookup.c -- SPlookup function
**
**  Description:
**
**	SPlookup function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

/*{
** Name:  "SPlookup \- locate node by key"
**
**	Locate a node in the tree containing the same key as that in
**	the supplied node.  The returned node is splayed to the root
**	of the tree.
**
** Inputs:
**	keyblk	a node, probably not in the tree, containing a key to
**		be located.
**	t	the tree to search.
**
** Outputs:
**	none
**
** Returns:
**	node	in the tree containing the key, or NULL if not found.
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
*/

SPBLK *
# ifdef CL_HAS_PROTOS
SPlookup(SPBLK *p, SPTREE *t )
# else
SPlookup( p, t )
register SPBLK *p;
register SPTREE *t;
# endif
{
    register SPBLK *n;
    register int sct;
    register int c;

    if( t == NULL || t->root == NULL )
	return NULL;

    /* find node in the tree */
    n = t->root;
    c = ++(t->lkpcmps);
    t->lookups++;
    while( n && (sct = (*t->compare)( p->key, n->key ) ) )
    {
	c++;
	n = ( sct < 0 ) ? n->leftlink : n->rightlink;
    }
    t->lkpcmps = c;

    /* reorganize tree around this node */
    if( n != NULL )
	SPsplay( n, t );

    return( n );
}
