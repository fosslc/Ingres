/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<mm.h>
# include	<me.h>
# include	"mmint.h"

/*
** Name: mmstblk.c - allocator alloc/free/rm for the block allocator
**
** Description:
**
** 	Implements MM_BLOCK_STRATEGY:
**
**	MM_blk_alloc quickly dequeues the first block on the pool's
**	free_list and returns it, because it can assume all requests
**	are of the same size.  If the free_list is empty MM_blk_alloc
**	allocates a big block from its parent, chains that big block 
**	onto the pool's alloc_list, breaks it up into blocks of the 
**	requested size, and puts them on the free_list.
**
**	MM_blk_free frees an individual block by putting it on the
**	pool's free_list.
**
**	MM_blk_rm removes the whole pool by freeing everything on the
**	alloc_list.
*/

STATUS
MM_blk_mk(
	MM_POOL		*pool,
	CL_ERR_DESC 	*err )
{
	pool->service_size = pool->parent->service_size;
	pool->service_cost = sizeof( MM_BLOCK );
	return OK;
}

PTR
MM_blk_alloc(
	MM_POOL		*pool,
	i4		size,
	STATUS 		*stat,
	CL_ERR_DESC 	*err )
{
    char	*p;
    MM_BLOCK	*block;
    MM_BLOCK	*bigblock;
    i4 	bigsize;

    /* Verify size using pool's private info */

    if( pool->item_size != size )
    {
	if( pool->item_size )
	{
	    *stat = FAIL;
	    return NULL;
	}
	
	pool->item_size = size;
    }

    /* If nothing on free list, allocate a large block and divvy it up */

    if( !pool->free_list )
    {
	/* Allocate a large block from the parent. */
	/* The three sizing steps are: *
	/* To minimize waste we try to find the least common */
	/* multiple of our size and the parent's service_size.  To keep */
	/* things from getting rediculous, we'll stop at 90% efficiency */
	/* or 4 * the parent's serive_size.  XXX implement  */

	bigsize = size + sizeof( MM_BLOCK );
	bigsize = max( bigsize, pool->expand_size );
	bigsize = MM_ROUND_SIZE( bigsize,
				 pool->parent->service_size,
				 pool->parent->service_cost );
	
	if( !( p = MMalloc( pool->parent, bigsize, stat, err ) ) )
	    return NULL;

	/* Chain it onto our alloc list. */

	bigblock = (MM_BLOCK *)p;
	bigblock->size = bigsize;
	bigblock->next = pool->alloc_list;
	pool->alloc_list = bigblock;

	/* Chop it into pieces and put it on our free list. */

	p += sizeof( MM_BLOCK );

	for( ; p + size <= (char *)bigblock + bigsize; p += size )
	{
	    block = (MM_BLOCK *)p;

	    block->size = size;
	    block->next = pool->free_list;
	    pool->free_list = block;
	}
    }

    /* Return first item from free list. */

    block = pool->free_list;
    pool->free_list = block->next;

    return (PTR)block;
}

STATUS
MM_blk_free( 
	MM_POOL		*pool,
	PTR 		obj,
	CL_ERR_DESC 	*err )
{
    MM_BLOCK	*block;

    /* no sanity check! */
    
    block = (MM_BLOCK *)obj;

    block->next = pool->free_list;
    pool->free_list = block;

    return OK;
}

STATUS
MM_blk_rm( 
	MM_POOL		*pool,
	CL_ERR_DESC 	*err )
{
    STATUS	status;
    MM_BLOCK	*block = pool->alloc_list;

    /* Free all blocks requested of parent */

    while( block )
    {
	MM_BLOCK	*next_block = block->next;

	if( status = MMfree( pool->parent, (PTR)block, err ) )
	    return status;

	block = next_block;
    }

    return OK;
}

MM_POOL
MM_pool_block[1] =
    {
	MM_pool_page, 0, 0, ME_MPAGESIZE, 0, 
	ME_MPAGESIZE, sizeof( MM_BLOCK ), 1,
	0, MM_blk_alloc, MM_blk_free, MM_blk_rm,
	0, 0, 0, 0
    } ;

