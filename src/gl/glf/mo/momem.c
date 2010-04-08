/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <cs.h>
# include <me.h>
# include <sp.h>
# include <tr.h>
# include <mo.h>
# include "moint.h"

/**
** Name:	momem.c		- memory allocation for MO
**
** Description:
**	This file defines:
**
**	MO_mem_classes[]	-- classes for MO memory management.
**	MO_alloc		-- allocate memory for MO
**	MO_free			-- release memory allocated by MO_alloc.
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	5-May-1993 (daveb)
**	    include <tr.h> for TRdisplay.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      12-jun-1995 (canor01)
**          semaphore protect memory allocation (MCT)
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**      24-sep-96 (mcgem01)
**          Global data moved to modata.c
**      21-jan-1999 (canor01)
**          Remove erglf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF i4 MO_nalloc;
GLOBALREF i4 MO_bytes_alloc;
GLOBALREF i4 MO_nfree	;
GLOBALREF i4 MO_mem_limit;

GLOBALREF MO_CLASS_DEF MO_mem_classes[];



/*{
** Name:	MO_alloc	-- allocate memory for MO
**
** Description:
**
**	Allocate memory for MO use.  Must be called with mutex held.
**	Keeps track of memory spent so far, and applies an adjustable
**	limit.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	size		size of memory to allocate.
**	stat		error status to write.
**
** Outputs:
**	stat		filled in with error status, if any.
**
** Returns:
**	pointer to block of new memory, or NULL, in which case stat will
**	have been filled in with details.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/
PTR
MO_alloc( i4  size, STATUS *stat )
{
    PTR block;

    MO_nalloc++;
    
    if( MO_bytes_alloc + size >= MO_mem_limit )
    {
	*stat = MO_MEM_LIMIT_EXCEEDED;
	block = NULL;
    }
    else 
    {
	block = MEreqmem( 0, size, 0, stat );
	if ( block != NULL )
	{
	    MO_bytes_alloc += size;
	}
	else
	{
	    TRdisplay("MO alloc error:  request for %d bytes returned status %x\n",
		      size, *stat );
	}
    }
    return( block );
}


/*{
** Name:	MO_free		-- release memory allocated by MO_alloc.
**
** Description:
**	Frees the block of memory, and applies to the available memory limit.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	obj		the allocated object to release.
**	size		the size of the object.  *You* have to remember!
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

VOID
MO_free( PTR obj, i4  size )
{
    MO_nfree++;
    MEfree( obj );
    MO_bytes_alloc -= size;
}

/* end of momem.c */

