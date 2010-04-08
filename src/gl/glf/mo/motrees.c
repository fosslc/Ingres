/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <sp.h>
# include <st.h>
# include <mo.h>
# include "moint.h"

/**
** Name:	motrees.c	- MOsptree_attach and management classes for SPTREEs
**
** Description:
**	This file defines:
**
**	MO_tree_classes[]	-- classes for tree objects.
**	MOsptree_attach		-- make an SP tree known to MO
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	4-Dec-1992 (daveb)
**	    Change exp.gl.glf to exp.glf for brevity
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      24-sep-96 (mcgem01)
**              Global data moved to modata.c
**      21-jan-1999 (canor01)
**          Remove erglf.h
**/


GLOBALREF char index_class[];

GLOBALREF MO_CLASS_DEF MO_tree_classes[];


/*{
** Name:	MOsptree_attach		-- make an SP tree known to MO
**
** Description:
**	Attach the named SPTREE to MO so it will be known.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	t		the named tree to attach.  It must have
**			a non-null name field pointing to stable
**			memory.
** Outputs:
**	none.
**
** Returns:
**	 OK
**		is the object was attached successfully.
**	 MO_NO_CLASSID
**		if the classid is not defined.
**	 MO_ALREADY_ATTACHED
**		the object has already been attached.
**	 MO_NO_MEMORY
**		if the MO_MEM_LIMIT would be exceeded.
**	 other
**		system specific error status, possibly indicating allocation
**		failure.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

STATUS
MOsptree_attach( SPTREE *t )
{
    STATUS stat = OK;
    stat = MOattach( 0, index_class, t->name, (PTR)t );
    return( stat );
}

