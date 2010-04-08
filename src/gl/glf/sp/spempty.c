# include <compat.h>
# include <gl.h>
# include <sp.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**
**  Name: spempty.c -- SPempty function
**
**  Description:
**
**	SPempty function.
**
**  History:
**	27-Jan-92 (daveb)
**		Formatted to INGRES standard.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:  "SPempty \- is the tree empty?"
**

**	Return zero if the tree is empty, or non-zero if there are
**	nodes present.
**
** Inputs:
**	t	the tree in question.
**
** Outputs:
**	none.
***
** Returns
**	value	0 if the tree is empty, non-zero if it has nodes.
*/

i4
# ifdef CL_HAS_PROTOS
SPempty( SPTREE *t )
# else
SPempty( t )
SPTREE *t;
# endif
{
    return( t == NULL || t->root == NULL );
}
