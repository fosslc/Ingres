# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**
**  Name: spinstall.c -- SPinstall function
**
**  Description:
**
**	SPinstall function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	28-Jan-92 (daveb)
**	    Null out node links in SPenq rather than here.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

/*{
** Name:  "SPinstall \- install node, checking for duplicates"
**
**	Insert a node into a tree if one with the same key does not
**	already exist.  Returns either the inserted node or the old
**	node with the same key.
**
** Inputs:
**	keyblk	a node not currently in the tree with a key to be inserted.
**	t	the destination tree.
**
** Outputs:
**	none
**
** Returns:
**	node	Either an existing node with the same key, or
**		the input node.
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
**	28-Jan-92 (daveb)
**	    Null out node links in SPenq rather than here.
*/

SPBLK *
# ifdef CL_HAS_PROTOS
SPinstall(SPBLK *n, SPTREE *t)
# else
SPinstall( n, t )
register SPBLK *n;
register SPTREE *t;
# endif
{
    SPBLK *lp;
    if( NULL == ( lp = SPlookup( n, t ) ) )
    {
	SPenq( n, t );
	return( n );
    }
    return( lp );
}

