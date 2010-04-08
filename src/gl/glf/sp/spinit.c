# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**
**  Name: spinit.c -- SPinit function
**
**  Description:
**
**	SPinit function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

/*{
** Name:  "SPinit \- initialize tree"
**
** Inputs:
**	q	Pointer to the tree to be initialized
**	compare	Pointer to comparison function to be used for ordering
**		the tree.
**
** Outputs:
**	none
**
** Returns:
**	t	a pointer to the initialized tree.
**
** History:
**	27-Jan-91 (daveb)
**	    Formatted to INGRES standard.
*/

SPTREE *
# ifdef CL_HAS_PROTOS
SPinit( SPTREE *t, SP_COMPARE_FUNC *compare )
# else
SPinit( t, compare )
SPTREE *t;
SP_COMPARE_FUNC *compare;
# endif
{
    t->lookups = 0;
    t->lkpcmps = 0;
    t->enqs = 0;
    t->enqcmps = 0;
    t->splays = 0;
    t->splayloops = 0;
    t->prevnexts = 0;

    t->compare = compare;
    t->root = NULL;
    t->name = NULL;
    return( t );
}
