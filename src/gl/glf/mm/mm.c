/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <me.h>
# include <mm.h>
# include "mmint.h"

/*
 * Name: mm.c - user entry points for MM allocator system

Generic implementation of all allocators:

    All allocators are built upon each other, and call through
    MMalloc() to get more memory.  No allocator calls another's alloc
    or free directly.

    Allocators maintain a list of memory chunks they have requested of
    their parent, so that they can release all memory on pool removal.
    This list is stored on the pool's alloc_list in a pool specific
    format.

    If the allocator supports freeing individual objects, it may also
    maintain a list of memory chunks returned.
History:
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	14-Jul-2006 (kschendel)
**	    MEfill no longer limited to i2.
**      03-nov-2010 (joea)
**          Move declaration of internal functions to mhint.h.
 */


/*
 * Name: MMmkpool - create a memory pool using a given strategey and parent
 */

STATUS
MMmkpool( i4		strategy,
	MM_POOL		*parent,
	i4		init_size,
	i4 	max_size,
	i4 	expand_size,
	MM_FULL_FUNC	*full_func,
	MM_POOL		**out_pool,
	CL_ERR_DESC	*err )
{
    STATUS	stat;
    MM_POOL	*pool;
    PTR		p;

    /* No illegal layering */

    if( parent && parent->mk_func == MM_mcr_mk )
	return FAIL;

    /* Allocate a pool structure */

    pool = (MM_POOL *)MMalloc( MM_pool_block, sizeof( *pool ), &stat, err );
    
    if( !pool )
	return stat;

    /* Just copy arguments */

    pool->parent = parent ? parent : MM_pool_default;
    pool->pool_size = init_size;
    pool->max_size = max_size;
    pool->expand_size = expand_size;
    pool->full_func = full_func;
    pool->alloc_list = 0;
    pool->free_list = 0;
    pool->item_size = 0;
    pool->current_size = 0;
    pool->reference_count = 1;
    pool->rm_func = 0;
    *out_pool = pool;

    /* Reference parent */
    /* Pool can only be freed with MMrmpool() now */

    pool->parent->reference_count++;

    /* Find strategy funcs */

    switch( strategy )
    {
    case MM_DEFAULT_STRATEGY:
	pool->mk_func = MM_dfl_mk;
	pool->alloc_func = MM_dfl_alloc;
	pool->free_func = MM_dfl_free;
	pool->rm_func = MM_dfl_rm;
	break;

    case MM_PAGE_STRATEGY:
	pool->mk_func = MM_pag_mk;
	pool->alloc_func = MM_pag_alloc;
	pool->free_func = MM_pag_free;
	pool->rm_func = MM_pag_rm;
	break;

    case MM_MICRO_STRATEGY:
	pool->mk_func = MM_mcr_mk;
	pool->alloc_func = MM_mcr_alloc;
	pool->free_func = MM_mcr_free;
	pool->rm_func = MM_mcr_rm;
	break;

    case MM_BLOCK_STRATEGY:
	pool->mk_func = MM_blk_mk;
	pool->alloc_func = MM_blk_alloc;
	pool->free_func = MM_blk_free;
	pool->rm_func = MM_blk_rm;
	break;

    default:
	MMrmpool( pool, err );
	return FAIL;
    }

    /* Let the pool initialize itself */

    if( ( stat = (*pool->mk_func)( pool, err ) ) != OK )
    {
	MMrmpool( pool, err );
	return stat;
    }

    return OK;
}

STATUS
MMrmpool( MM_POOL 	*pool, 
	CL_ERR_DESC 	*err )
{
    STATUS	status;
    MM_POOL	*parent;

    /* Don't allow removing the default pool */

    if( !pool )
	return FAIL;

    /* Don't delete if still reference */

    if( --pool->reference_count )
	return OK;

    /* Free all memory allocated of parent. */

    if( pool->rm_func )
	if( status = ( *pool->rm_func )( pool, err ) )
	    return status;

    /* Remove control block. */

    parent = pool->parent;

    if( status = MMfree( MM_pool_block, (PTR)pool, err ) )
	return status;

    /* Recursively remove parent */

    return MMrmpool( parent, err );
}

PTR
MM_alloc( MM_POOL	*pool,
	 i4	size,
	 STATUS		*stat,
	 CL_ERR_DESC	*err )
{
    if( !pool )
	pool = MM_pool_default;

    /*
    printf( "MMalloc %d from %x\n", size, pool );
    */

    for(;;)
    {
	PTR	p;

	/* Call allocator for pool's strategy */

	if( !pool->max_size || pool->current_size + size <= pool->max_size )
	{
	    if( p = ( *pool->alloc_func )( pool, size, stat, err ) )
	    {
		pool->current_size += size;
		return p;
	    }
	}

	/* Allocation failed - ask client to return space */

	if( !pool->full_func || ( *pool->full_func )( pool, size ) != OK )
	    return 0;
    }
}

STATUS
MM_free( MM_POOL 	*pool,
	PTR 		obj,
	CL_ERR_DESC 	*err )
{
    STATUS status;

    if( !pool )
	pool = MM_pool_default;

    return ( *pool->free_func )( pool, obj, err );
}

PTR
MMclear( PTR		obj,
	i4		size )
{
    /* May be called with null pointer from failed MMalloc() */

    if( obj )
    {
	MEfill(size, '\0', obj );
    }

    return obj;
}

