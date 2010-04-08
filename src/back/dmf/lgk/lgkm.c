/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <ex.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <cs.h>
#include    <er.h>
#include    <tr.h>
#include    <pm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lk.h>
#include    <lgkdef.h>

/**
**
**  Name: LGKM.C - shared memory routines for logging and locking.
**
**  Description:
**	This module implements a shared memory manager used by the logging and
**	locking systems.
**
**	The model of shared memory management implemented by this module will
**	be the following:
**
**	Both LG and LK tend to allocate large chunks of memory (we'll call them
**	"extents") and rarely release the memory.
**
**	Memory is typically formatted into arrays of pre-formatted identically
**	sized blocks which are used for control blocks.
**
**	These blocks are "allocated" and "freed" by placing them onto "in-use"
**	and "free" lists.
**
**	The shared memory pool is maintained as a linked list of extents with
**	offsets and size information maintained within each extent.  The size
**	information refers to the size of the extent not including the size of
**	the header.  The pool begins as one extent.  The first request is
**	filled by fragmenting this one extent into two extents.  The second
**	extent becomes the new memory pool.
**
**	When an extent is freed, it is placed onto the free list of extents
**	which is sorted by offset in ascending order. Adjacent extents are
**	coalesced.
**
**	    lgkm_initialize_memory(size_of_memory)
**	    lgkm_allocate_extent(size_of_extent)
**	    lgkm_deallocate_extent(extent_to_deallocate)
**
**  History:
**	Summer, 1992 (bryanp)
**	    Created for the new portable logging and locking system.
**	13-Feb-1993 (keving)
**	    Added functions to get logging and locking parameters from
**	    PM and calculate size required for lgk shared memory.
**	17-feb-1993 (andys)
**	    get_lgk_config, lgk_get_counts, lgk_calculate_size extracted 
**	    and moved to lgkparms.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in dump_extents().
**	23-sep-1996 (canor01)
**	    Moved global data definitions to lgkdata.c.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-apr-2002 (devjo01)
**	    Make sure released extents are zero'ed.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
*/

/*
**  Forward function references.
*/

static VOID	 dump_extents( LGK_MEM		   *lgk_mem);


/*
** Definition of all global variables owned by this file.
*/

GLOBALREF LGK_BASE  LGK_base;              /* information about LG 
				           ** and LK memory pool.
					   */

static PTR	Last_Valid_Addr = NULL;
static PTR	First_Valid_Addr = NULL;


/*{
** Name: lgkm_allocate_extent()	- allocate an extent.
**
** Description:
**	Allocate an extent from the shared memory pool already initialized.
**	lgkm_initialize_mem() must have already been called to prepare the
**	shared memory pool.
**
**	The memory allocation algorithm is a very simple one, and is based
**	on the assumption that there will rarely be any free's - which is
**	how lg and lk tend to use the system.  LG and LK tend to allocate
**	memory for their different objects, and then place them on free 
**	queues once they have been allocated.
**
**	The memory pool is maintained as a linked list of blocks with ptr's
**	and size info maintained within each block.  The size info refers
**	to the size of the block not including the size of the hdr.  The
**	pool begins as one block.  The first request is filled by the head
**	of the block which is fragmented into two blocks.  The second block
**	becomes the new memory pool.  
**
** Inputs:
**      size				size in bytes of memory to be allocated.
**
** Outputs:
**	none.
**
**	Returns:
**	    success	- pointer to an LGK_EXT that fulfills the request
**	    failure     - NULL.
**
** History:
**	Summer, 1992 (bryanp)
**	    Created for the new portable logging and locking system.
*/
LGK_EXT *
lgkm_allocate_ext(SIZE_TYPE size)
{
    LGK_EXT         *ext;
    LGK_EXT         *ext_prev;
    LGK_EXT         *new_ext;
    SIZE_TYPE	    aligned_size;
    LGK_MEM	    *lgk_mem = (LGK_MEM *)LGK_base.lgk_mem_ptr;

    if (CSp_semaphore(TRUE, &lgk_mem->mem_ext_sem)) 
	return((LGK_EXT *) NULL);

    /* only allocate in multiples of sizeof(LGK_EXT) */

    aligned_size = (((size / sizeof(LGK_EXT)) + 1) * sizeof(LGK_EXT)); 

    /* Get pointer to first extent on free list */

    ext      = (LGK_EXT *)((char *)lgk_mem + lgk_mem->mem_ext.ext_next);
    ext_prev = &(lgk_mem->mem_ext);

    /* search free list for first extent that meets size need */

    while (ext != (LGK_EXT *)lgk_mem)
    {
	if (ext->ext_size >= aligned_size)
	{
	    break;
	}
	else
	{
	    ext_prev = ext;
	    ext      = (LGK_EXT *)((char *)lgk_mem + ext->ext_next);
	}
    }

    /* if we found a candidate fragment it and return it caller */

    if (ext != (LGK_EXT *)lgk_mem)
    {
	if (ext->ext_size > aligned_size)
	{
	    /* The new extent needed is smaller than the current extent found
	    ** so fragment it 
	    */

	    new_ext = (LGK_EXT *)(((char *)ext) + sizeof(LGK_EXT) +
						aligned_size);

	    new_ext->ext_size = ext->ext_size - aligned_size - sizeof(LGK_EXT);

	    /* replace current ext with smaller "sub-extent" */

	    new_ext->ext_next  = ext->ext_next;
	    ext_prev->ext_next = (i4)((char *)new_ext - (char *)lgk_mem);
	}
	else if (ext->ext_size == aligned_size)
	{
	    /* unlink this extent from the list */

	    ext_prev->ext_next = ext->ext_next;
	}

	/* initialize fields of allocated extent */

	ext->ext_next = 0;
	ext->ext_size = aligned_size;
    }
    else
    {
	ext = (LGK_EXT *)NULL;
    }

    if (CSv_semaphore(&lgk_mem->mem_ext_sem)) 
	return((LGK_EXT *) NULL);

    return(ext);
}

/*{
** Name: lgkm_deallocate_ext()	- Deallocate an extent.
**
** Description:
**	Deallocate the given extent.
**
**	The list of free extents is always maintained sorted, ordered by
**	increasing offset of extent.  The extent to be deallocated is inserted
**	into the free list in its correct sorted place.
**
**	Extents freed are zero'ed, since memory starts in a zero'ed state,
**	and routines grabbing additional extents depend on this, and the
**	return of an extent to the free extent pool is an uncommon operation
**	compared to allocating a new extent.
**	
** Inputs:
**	ext				extent to deallocate.
**
** Outputs:
**	none
**
**	Returns:
**	    VOID
**
** History:
**	Summer, 1992 (bryanp)
**	    Created for the new portable logging and locking system.
**	26-apr-2002 (devjo01)
**	    Zero out freed extents.  Simplify coalesce logic.
**	26-jan-2004 (penga03)
**	    Changed MEfilllng to MEfill. 		
*/
VOID
lgkm_deallocate_ext(LGK_EXT *input_ext)
{
    LGK_EXT         *ext_prev;
    LGK_EXT         *ext;
    STATUS	    ret_val = OK;
    LGK_MEM	    *lgk_mem = (LGK_MEM *)LGK_base.lgk_mem_ptr;

    /*
    ** Zero out memory being released.  Allocation routines, and
    ** routines which extend lists for LG & LK, expect that the
    ** newly grabbed memory is zero'ed.  This is almost always
    ** the case, since with very few (one?) exception(s), an extent 
    ** once allocated is never freed.  The one exception that comes
    ** to mind is the allocation and free of a log context for
    ** recovery of a failed cluster node.   Since this is a rare
    ** event, we are putting the burden of clearing a freed extent
    ** on 'lgkm_deallocate_ext'.  This is done outside the scope
    ** of the 'mem_ext_sem', since we wish to minimize the time
    ** this is held.
    */
    MEfill( input_ext->ext_size, '\0', (PTR)(input_ext + 1) );

    if (ret_val = CSp_semaphore(TRUE, &lgk_mem->mem_ext_sem)) 
    {
	TRdisplay("lgkm_deallocate_ext(): CSp_semaphore() failed, ret = %d\n", 
		  ret_val);
    }

    /* Get pointer to first extent on free list */
	
    ext      = (LGK_EXT *)((char *)lgk_mem + lgk_mem->mem_ext.ext_next);
    ext_prev = &(lgk_mem->mem_ext);

    /*
    ** Extent free list is ordered by address (from least address to greater
    ** address).  This loop assumes that ordering and finds where the freed
    ** extent fits.
    */

    while (ext != (LGK_EXT *)lgk_mem && (ext < input_ext))
    {
	ext_prev = ext;
	ext      = (LGK_EXT *)((char *)lgk_mem + ext->ext_next);
    }

    input_ext->ext_next = (i4)((char *)ext - (char *)lgk_mem);
    ext_prev->ext_next = (i4)((char *)input_ext - (char *)lgk_mem);

    /* now coalesce if possible */

    /* First try with block before */
    if ( ( (PTR)(ext_prev + 1) + ext_prev->ext_size ) == (PTR)input_ext )
    {
	/* add input_ext to previous ext. */
	ext_prev->ext_size += sizeof(LGK_EXT) + input_ext->ext_size; 
	ext_prev->ext_next = input_ext->ext_next;
	MEfill( sizeof(LGK_EXT), '\0', (PTR)input_ext );
	/* Treat new larger block as input extent */
	input_ext = ext_prev;
    }

    /* Now try combining freed block with block past input extent */
    if ( ( (PTR)(input_ext + 1) + input_ext->ext_size ) == (PTR)ext )
    {
	/* add following extent to 'input_ext'. */
	input_ext->ext_size += sizeof(LGK_EXT) + ext->ext_size;
	input_ext->ext_next = ext->ext_next;
	MEfill( sizeof(LGK_EXT), '\0', (PTR)ext );
    }

    if (ret_val = CSv_semaphore(&lgk_mem->mem_ext_sem)) 
    {
	TRdisplay("lgkm_deallocate_ext(): CSv_semaphore() failed, ret = %d\n", 
		  ret_val);
    }

    return;
}

/*{
** Name: lgkm_initialize_mem()	- Allocate and initialize memory for LG and LK.
**
** Description:
**	Set up memory pool for use by lgkm_{allocate,deallocate}_ext().
**	Initial pool is simply a single element on a (-1) terminated linked
**	list.
**
** Inputs:
**	amount				amount of memory to allocate.
**
** Outputs:
**	none.
**
**	Returns:
**	    FAIL	- shared memory not mapped in current process
**	    OK		- success.
**
** History:
**	Summer, 1992 (bryanp)
**	    Created for the new portable logging and locking system.
**	17-aug-2004 (thaju02)
**	    Server fails to come up, E_CL0F14. Change amount 
**	    from i4 to SIZE_TYPE.
*/
STATUS
lgkm_initialize_mem(SIZE_TYPE amount, PTR first_free_byte)
{
    LGK_EXT	*ext;
    LGK_MEM	*lgk_mem = (LGK_MEM *)LGK_base.lgk_mem_ptr;

    if (lgk_mem == (LGK_MEM *)0) 
    {
	return(FAIL);
    }

    if (CSp_semaphore(TRUE, &lgk_mem->mem_ext_sem)) 
	return(FAIL);
    else
    {
	/* align 1st extent on an LGK_EXT aligned boundary */

	ext = (LGK_EXT *) first_free_byte;
	ext = (LGK_EXT *)ME_ALIGN_MACRO((PTR)ext, sizeof(LGK_EXT));

	lgk_mem->mem_ext.ext_next = (i4)((char *)ext - (char *)lgk_mem);
	lgk_mem->mem_ext.ext_size = 0;

	ext->ext_next = 0;
	ext->ext_size = amount - 
			(((char *) ext) - ((char *) first_free_byte)) -
			(sizeof(LGK_EXT));

    }

    /* For debug (s/b part of LGK_base?) */
    First_Valid_Addr = (PTR)(ext + 1);
    Last_Valid_Addr = First_Valid_Addr + ext->ext_size;

    if (CSv_semaphore(&lgk_mem->mem_ext_sem)) 
	return(FAIL);

# if 0
    TRdisplay("%@ LGK memory initialized.  Base = %p, Start = %p, end = %p\n",
	      lgk_mem, First_Valid_Addr, Last_Valid_Addr );
# endif
    return(OK);
}

/*{
** Name: dump_extents()	- dump the extent database
**
** Description:
**	Debugging routine to dump the entire extent database.
**
**	Useful for debugging problems when the LG or LK system runs out of
**	memory.
**
** Inputs:
**	lgk_mem				LGK_MEM pointer.
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID
**
** History:
**	Summer, 1992 (bryanp)
**	    Created for the new portable logging and locking system.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/
static VOID
dump_extents(
LGK_MEM		   *lgk_mem)
{
    LGK_EXT		*ext;
    LGK_EXT		*next_extent;

    ext = (LGK_EXT *)((char *)lgk_mem + lgk_mem->mem_ext.ext_next);

    TRdisplay("dump of list of extents:\n");

    while (ext != (LGK_EXT *)lgk_mem)
    {
	if (ext == &(lgk_mem->mem_ext))
	    TRdisplay("dummy hdr:\t");
	else
	    TRdisplay("          \t");

	next_extent = (LGK_EXT *)((char *)ext + ext->ext_next);

	TRdisplay("ext addr %p: ext_next %p; ext_size %d\n",
	       ext, next_extent, ext->ext_size);
	ext = next_extent;
    }

    return;
}



/*{
** Name: lgkm_valid()	- Check address for validity within LGK segment.
**
** Description:
**	Debugging routine to check an LGK memory object for validity.
**
** Inputs:
**	module 				Module where called.
**	lineno 				Line # where called.
**	addr 				Address to check.
**	sz				# of byte in structure.
**
** Outputs:
**	none.
**
**	Returns:
**	    OK if memory in range [ addr .. addr + sz - 1 ] is in
**	    allocated space within the LGK shared memory segment.
**
** History:
**	24-may-2002 (devjo01)
**	    Created to help debug a stale memory reference problem.
*/
STATUS
lgkm_valid( char * module, i4 lineno, PTR addr, SIZE_TYPE sz )
{
    LGK_EXT	*ext;
    LGK_MEM	*lgk_mem = (LGK_MEM *)LGK_base.lgk_mem_ptr;
    PTR		 endaddr = addr + sz;

    for ( ; ; )
    {
	/* Check if uninitialized */
	if (lgk_mem == (LGK_MEM *)0) 
	    break;

	/* Check for address range wrap */
	if (addr > endaddr)
	    break;

	/* Completely off the wall address? */
	if ( addr < First_Valid_Addr || endaddr > Last_Valid_Addr )
	    break;

	/* Check if within currently freed area */
	ext = (LGK_EXT *)((char *)lgk_mem + lgk_mem->mem_ext.ext_next);
	while (ext != (LGK_EXT *)lgk_mem)
	{
	    if ( endaddr < (PTR)ext )
		return OK;

	    /* Is start address within a freed area? */
	    if ( (addr >= (PTR)ext) &&
		 (addr < (((PTR)(ext + 1)) + ext->ext_size)) )
		break;
	    /* Is ending address within a freed area. */
	    if ( endaddr < (((PTR)(ext + 1)) + ext->ext_size) )
		break;
		 
	    ext = (LGK_EXT *)((char *)ext + ext->ext_next);
	}
	/* If all free zones missed, then OK */
	if (ext == (LGK_EXT *)lgk_mem)
	    return OK;
	break;
    }
    TRdisplay( "%@ LGKm check failure @ %s:%d Addr = %p, sz = %d\n",
	       module, lineno, addr, sz );
    CS_breakpoint();
    return(FAIL);
}
