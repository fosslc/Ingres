# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**
**  Name: spftail.c -- SPftail function
**
**  Description:
**
**	SPftail function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

/*{
** Name:  "SPftail \- fast-find tail"
**
**	Return the last node in a tree, without reorganizing.  This
**	may leave the tree underbalanced.
**
** Inputs:
**	t	tree in question
**
** Outputs:
**	none
**
** Returns:
**	tail	the last node in the tree, or NULL if the tree is empty.
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
*/

SPBLK *
# ifdef CL_HAS_PROTOS
SPftail( SPTREE *t )    
# else
SPftail( t )
SPTREE *t;
# endif
{
    register SPBLK * x;

    if( NULL != ( x = t->root ) )
	while( x->rightlink != NULL )
	    x = x->rightlink;

    return( x );
}
