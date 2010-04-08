/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<mm.h>
# include	<me.h>
# include	"mmint.h"

/*
** Name: mmmicro.c - allocator alloc/free/rm for the micro allocator
**
** Description:
**
** 	Implements MM_MICRO_STRATEGY:
**
**	MM_mcr_alloc maintains a stream of memory, comprised of 
**	blocks chained onto alloc_list and free_list, and returns
**	the requested amount of memory from the last block on the
**	alloc_list.  If alloc_list is empty or the block there is
**	too small, a block is moved from the free_list to the
**	alloc_list.  If that block is too small, a new block is
**	instead allocated from the parent pool.
**
**	MM_mcr_free frees the requested object and everything after by
**	scanning the alloc_list for the block containing the object,
**	noting the new end of the block, and moving all blocks after 
**	that one onto the free_list.
**
**	MM_mcr_rm free all blocks on free_list and alloc_list.
**
** History:
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
*/

typedef struct {
	MM_BLOCK	hdr;
	i4		used;
} MM_MCR_BLOCK;

STATUS
MM_mcr_mk(
	MM_POOL		*pool,
	CL_ERR_DESC 	*err )
{
	pool->service_size = pool->parent->service_size;
	pool->service_cost = sizeof( MM_MCR_BLOCK );
	return OK;
}

PTR
MM_mcr_alloc(
	MM_POOL		*pool,
	i4		size,
	STATUS 		*stat,
	CL_ERR_DESC 	*err )
{
    MM_MCR_BLOCK	*block = (MM_MCR_BLOCK *)pool->alloc_list;
    i4 		bigsize;
    PTR			p;

    /* If not enough room in current block, find another */

    if( !block || block->used + size > block->hdr.size )
    {
	/* Check out the one on the free list */
	/* If it isn't big enough, allocate another */

	block = (MM_MCR_BLOCK *)pool->free_list;

	if( !block || sizeof( MM_MCR_BLOCK ) + size > block->hdr.size )
	{
	    /* Allocate a large block from the parent. */

	    /* Sizing steps are: */
	    /* - requested size plus our overhead */
	    /* - moved up to requested expand_size */
	    /* - rounded up to optimum size for parent */

	    bigsize = size + sizeof( MM_MCR_BLOCK );
	    bigsize = max( bigsize, pool->expand_size );
	    bigsize = MM_ROUND_SIZE( bigsize,
				     pool->parent->service_size,
				     pool->parent->service_cost );

	    if( !( p = MMalloc( pool->parent, bigsize, stat, err ) ) )
		return 0;

	    block = (MM_MCR_BLOCK *)p;
	    block->hdr.size = bigsize;
	}
	else
	{
	    /* Use this block from the free list. */

	    pool->free_list = block->hdr.next;
	}

	/* Chain acquired block onto our alloc list. */

	block->used = sizeof( MM_MCR_BLOCK );
	block->hdr.next = pool->alloc_list;
	pool->alloc_list = &block->hdr;
    }

    /* Return chunk from end of block. */

    p = (PTR)((char *)block + block->used);

    block->used += size;

    return p;
}

STATUS
MM_mcr_free( 
	MM_POOL		*pool,
	PTR 		obj,
	CL_ERR_DESC 	*err )
{
    MM_MCR_BLOCK	*block;
    MM_MCR_BLOCK	*next;

    /* Free everything allocated after obj by travelling up the alloc */
    /* chain until we find the block containing obj. */

    for( block = (MM_MCR_BLOCK *)pool->alloc_list; block; block = next )
    {
	if( obj >= (PTR)( block + sizeof( MM_MCR_BLOCK ) ) &&
	    obj < (PTR)( block + block->used ) )
	{
	    /* Found block containing obj.  Reset used to point there. */

	    block->used = (char *)obj - (char *)block;

	    return OK;
	}

	/* Move this block from the alloc list to the free list. */

	next = (MM_MCR_BLOCK *)block->hdr.next;
	block->hdr.next = pool->free_list;
	pool->free_list = &block->hdr;
	pool->alloc_list = &next->hdr;
    }

    /* Uh, unknown object - and we just freed everything. */
    /* Fortunately, that's the way we'll document it. */

    return OK;
}

STATUS
MM_mcr_rm( 
	MM_POOL		*pool,
	CL_ERR_DESC 	*err )
{
    STATUS	status;
    MM_BLOCK	*block;
    MM_BLOCK	*next;

    /* Free all blocks requested of parent */

    for( block = pool->alloc_list; block; block = next )
    {
	next = block->next;

	if( status = MMfree( pool->parent, (PTR)block, err ) )
	    return status;
    }

    for( block = pool->free_list; block; block = next )
    {
	next = block->next;

	if( status = MMfree( pool->parent, (PTR)block, err ) )
	    return status;
    }

    return OK;
}
