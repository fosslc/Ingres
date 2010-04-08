# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<equtils.h>
# include	<eqsym.h>

/*
+* Filename:	sycopy.c
** Purpose:	Tree copying routines (for PL/1)
**
** Defines:
**	sym_copy	- copy a symbol table tree (recursive - internal only)
-*	sym_graft	- Driver for sym_copy (called from outside)
** Notes:
**
** History:
**	11-Nov-84	- Initial version (mrw)
**	29-jun-87 (bab)	Code cleanup.
**	15-sep-93 (sandyd)
**		Fixed type mismatch on MEcopy() arguments.
**	25-oct-93 (sandyd)
**		Standard says we should cast "sizeof()" when used as argument.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

syNAME	*sym_store_name();

/*
+* Procedure:	sym_copy
** Purpose:	Copy a tree, returning a pointer to the head of the copy.
** Parameters:
**	node	- SYM *	- Tree to copy
**	reclev	- i4	- Level at which to root it
** Return Values:
-*	SYM *	- Pointer to the head of the copy.
** Notes:
**	- use a postorder traversal.
**	- adjust the child and sibling (and parent) pointers appropriately.
**	- adjust the record level too.
**	- insert each copied entry at the head of its hash chain.
**	- we WILL be guaranteed that the source tree is not recursive!
**	- WARNING: the parent pointers of the copy of the top of the tree
**	  ("node" and its siblings) will point to the original parent,
**	  not to the adoptive parent; the new parent needs to adopt them
**	  formally.
**
** Imports modified:
**	Nothing.
*/

SYM *
sym_copy( node, reclev )
SYM	*node;			/* tree to copy */
i4	reclev;			/* level at which to root it */
{
    register SYM
		*n, *s, *c;
    SYM		*sym_q_new();
    
    if (node == NULL)
	return NULL;
    c = sym_copy( node->st_child, reclev+1 );	/* child is one level deeper */
    s = sym_copy( node->st_sibling, reclev );
    n = sym_q_new();
    MEcopy( (PTR)node, (u_i2)sizeof(*n), (PTR)n );
    n->st_child = c;
    n->st_sibling = s;
    n->st_reclev = reclev;

    /*
     * Can't just do "n->st_prev = node", since my progenitor might
     * have the same name; we could end up with both of us pointing
     * to the same "st_prev".  This finds the most recent entry.
     */
    n->st_prev = sym_find( sym_str_name(n) );

  /* if we have children, make their parent be us, not their natural parent */
    for( ; c; c=c->st_sibling )
	c->st_parent = n;		/* adjust child's parent link */
    sym_q_append( n, NULL );		/* insert it at the head of its chain */
    return n;
}

/*
+* Procedure:	sym_graft
** Purpose:	Copy a tree and assume its identity.
**		"Invasion of the Body Snatchers".
** Parameters:
**	like	- SYM *	- The tree to be copied.
**	me	- SYM *	- The entry to assume "like"s identity.
** Return Values:
-*	SYM *	- "me" -- just return our arg.
** Notes:
**	Let sym_copy copy our children; then we adopt them, and assume
**	the identity of the source node.
**
** Imports modified:
**	Nothing.
*/

SYM *
sym_graft( like, me )
SYM	*like;
SYM	*me;
{
    register SYM	*p;

    if (like == NULL)
	return me;
    if( me == NULL )
	return NULL;		/* bozo */

  /* copy the children of like */
    p = sym_copy( like->st_child, me->st_reclev+1 );	/* copy the tree */
  /* I'm a daddy! */	
    me->st_child = p;

    /*
     * declaring "me" has set the st_vislim, st_block, st_reclev,
     * st_flags, st_hash, st_name, st_prev, st_hval, st_sibling,
     * and st_parent fields of me appropriately.
     * Copy the remaining fields from like.
     */
    me->st_type = like->st_type;
    me->st_value = like->st_value;
    me->st_btype = like->st_btype;
    me->st_dims = like->st_dims;
    me->st_indir = like->st_indir;
    me->st_space = like->st_space;
    me->st_dsize = like->st_dsize;

    /*
     * if we have children, make their parent be us, not their natural parent
     * (ie, adopt them).
     */
    for( ; p; p=p->st_sibling )
	p->st_parent = me;

  /* maybe it's convenient to return our arg */
    return me;
}
