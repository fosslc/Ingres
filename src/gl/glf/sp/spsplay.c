# include <compat.h>
# ifdef sqs_ptx
# undef abs
# endif
# ifdef sgi_us5 
# undef abs
# endif
# include <gl.h>
# include <sp.h>
# ifndef VMS
# include <systypes.h>
# endif
# if !defined(usl_us5) && !defined(ris_u64)
# include <stdlib.h> /* for abort() */
# endif /* usl_us5 ris_u64  */

/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

/**
**
**  Name: spsplay.c -- SPsplay function
**
**  Description:
**
**	SPsplay function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (canor01)
**	    Clean up compiler warnings.
**	10-mar-1999 (walro03)
**	    Undefine abs for Sequent (sqs_ptx).
**	12-mar-1999 (somsa01)
**	    The previous change caused "Duplicate type specifier ..." errors
**	    from sys/types.h on HP-UX 10.20 . Thus, included systypes.h
**	    as well.
**	15-mar-1999 (popri01)
**	    stdlib causes compile errors for Unixware, so omit it.
**      18-May-1999 (linke01)
**          undefined abf for sgi_us5 to avoid confliction with 
**          /usr/include/stdlib.h 
**	31-mar-1999 (kinte01)
**	    put the include for systypes.h inside an ifndef VMS as this
**	    header doesn't exist on VMS
**      21-May-1999 (hweho01)
**          stdlib causes compile errors for ris_u64, so omit it.
*/

/*{
** Name:  "SPsplay \- reorganize tree around node"
**
**	The tree is reorganized so that n is the root of the splay
**	tree t; results are unpredictable if n is not in t to start
**	with; t is split from n up to the old root, with all nodes to
**	the left of n ending up in the left subtree, and all nodes to
**	the right of n ending up in the right subtree; the left branch
**	of the right subtree and the right branch of the left subtree
**	are shortened in the process
**
**	Assumes that n is not NULL and is in t.
**
** Inputs:
**	n	the node to be made the root.
**	t	the tree containing n.
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
*/

VOID
#ifdef CL_HAS_PROTOS
SPsplay(SPBLK *n, SPTREE *t)
#else
SPsplay( n, t )
register SPBLK * n;
SPTREE * t;
# endif
{
    register SPBLK * up;	/* points to the node being dealt with */
    register SPBLK * prev;	/* a descendent of up, already dealt with */
    register SPBLK * upup;	/* the parent of up */
    register SPBLK * upupup;	/* the grandparent of up */
    register SPBLK * left;	/* the top of left subtree being built */
    register SPBLK * right;	/* the top of right subtree being built */

    left = n->leftlink;
    right = n->rightlink;
    prev = n;
    up = prev->uplink;

    t->splays++;

    while( up != NULL )
    {
	t->splayloops++;

        /* walk up the tree towards the root, splaying all to the left of
	   n into the left subtree, all to right into the right subtree */

        upup = up->uplink;
        if( up->leftlink == prev )	/* up is to the right of n */
	{
            if( upup != NULL && upup->leftlink == up )  /* rotate */
	    {
                upupup = upup->uplink;
                upup->leftlink = up->rightlink;
                if( upup->leftlink != NULL )
		    upup->leftlink->uplink = upup;
                up->rightlink = upup;
                upup->uplink = up;
                if( upupup == NULL )
		    t->root = up;
		else if( upupup->leftlink == upup )
		    upupup->leftlink = up;
		else
		    upupup->rightlink = up;
                up->uplink = upupup;
                upup = upupup;
            }
            up->leftlink = right;
            if( right != NULL )
		right->uplink = up;
            right = up;

        }
	else				/* up is to the left of n */
	{
            if( upup != NULL && upup->rightlink == up )	/* rotate */
	    {
                upupup = upup->uplink;
                upup->rightlink = up->leftlink;
                if( upup->rightlink != NULL )
		    upup->rightlink->uplink = upup;
                up->leftlink = upup;
                upup->uplink = up;
                if( upupup == NULL )
		    t->root = up;
		else if( upupup->rightlink == upup )
		    upupup->rightlink = up;
		else
		    upupup->leftlink = up;
                up->uplink = upupup;
                upup = upupup;
            }
            up->rightlink = left;
            if( left != NULL )
		left->uplink = up;
            left = up;
        }
        prev = up;
        up = upup;
    }

# ifdef DEBUG
    if( t->root != prev )
    {
/*	fprintf(stderr, " *** bug in splay: n not in t *** " ); */
	abort();
    }
# endif

    n->leftlink = left;
    n->rightlink = right;
    if( left != NULL )
	left->uplink = n;
    if( right != NULL )
	right->uplink = n;
    t->root = n;
    n->uplink = NULL;
}
