/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<mm.h>
# include	<me.h>
# include	"mmint.h"

#include <stdio.h>

/*
**
** History:
**
**	22-apr-1996 (lawst01)
**	   Windows 3.1 port changes - added fcn prototypes, changed vars
**	   from int to i4.
**
** Name: mmstdfl.c - allocator alloc/free/rm for the default allocator
**
** Description:
**
** 	Implements MM_DEFAULT_STRATEGY:
**
**	MM_dfl_alloc is a cheesy, garden variety allocator.  It gets
**	memory by scanning free_list, returning the smallest chunk it
**	can find.  If it can't find any such chunk, it gets memory from
**	the block at the head of the alloc_list.  If that block is
**	depleted, it allocates another block from the parent, puts it
**	on the alloc_list, and gets memory from there.
**
**	MM_dfl_free frees memory by chaining it onto the free_list.
**	It does no coalescing.
**
**	MM_dfl_rm removes the pool by freeing everything on the
**	alloc_list.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	1-Dec-2010 (kschendel)
**	    More warning fixes (include stdio).
*/
int MM_dfl_stat (MM_POOL *);
STATUS MM_dfl_free (MM_POOL *, PTR, CL_ERR_DESC *);

/*
** Name: MM_DFL_BLOCK - keeps track of allocated bigblocks
*/

i4 alloced = 0;
i4 wastesiz = 0;
i4 requests = 0;

typedef struct {
	MM_BLOCK	hdr;
	i4		used;
} MM_DFL_BLOCK ;

STATUS
MM_dfl_mk(
	MM_POOL		*pool,
	CL_ERR_DESC 	*err )
{
	pool->service_size = pool->parent->service_size;
	pool->service_cost = sizeof( MM_DFL_BLOCK );
	return OK;
}

PTR
MM_dfl_alloc(
	MM_POOL		*pool,
	i4		size,
	STATUS 		*stat,
	CL_ERR_DESC 	*err )
{
    MM_DFL_BLOCK *bigblock;
    i4	minsize;
    i4		i;

    alloced++;

    if( !(++requests % 100000) )
	MM_dfl_stat( pool );

    /* Round up to powers of 2, quartered */
    /* This breaks all requests up into approx 30 * 4 buckets */

    for( i = 2; ; i++ )
	if( ( minsize = ( 5 << i ) ) >= size ||
	    ( minsize = ( 6 << i ) ) >= size ||
	    ( minsize = ( 7 << i ) ) >= size ||
	    ( minsize = ( 8 << i ) ) >= size )
		break;

    /* Scan free list for appropriate chunk */
    /* If we find one exactly at minsize, that's our winner, */
    /* but we'll settle for any chunk smaller than bestsize. */
    /* Otherwise, we leave the free list alone and go to the well. */
    
    {
	MM_BLOCK *chunk = 0;
	MM_BLOCK *p, *pp = 0, *spp = 0;
	i4 bestsize = minsize * 32;

	for( p = pool->free_list; p && bestsize > minsize; pp = p, p = p->next )
	    if( p->size >= size && p->size < bestsize )
	{
	    spp = pp;
	    chunk = p;
	    bestsize = p->size;
	}

	if( chunk )
	{
	    if( spp )
		spp->next = chunk->next;
	    else
		pool->free_list = chunk->next;
	    chunk->next = (MM_BLOCK *)(SCALARP)( chunk->size - size );
	    wastesiz += chunk->size - size;
	    return (PTR)( chunk + 1 );
	}
    }

    /* Check the last bigblock allocated for room at its tail. */
    /* If the piece there is large enough, we'll cut our chunk */
    /* and leave the rest behind.  If the piece is not big enough, */
    /* well make it into a chunk and put it on the free chain.  We */
    /* have to do this because we can only cut new chunks from the */
    /* tail of the last allocated bigblock, and we're about to */
    /* allocate a new bigblock. */

    bigblock = (MM_DFL_BLOCK *)pool->alloc_list;

    if( bigblock )
    {
	MM_BLOCK *chunk;
	i4 chunksize;
	
	chunk = (MM_BLOCK *)((char *)bigblock + bigblock->used );
	chunksize = bigblock->hdr.size - bigblock->used - sizeof( *chunk );

	/* If the remaining room is big enough, cut us off a piece. */
	/* If not, put it on the free list since this bigblock is no */
	/* longer going to be accessible at the head of alloc_list. */

	if( minsize <= chunksize )
	{
	    bigblock->used += minsize + sizeof( MM_BLOCK );
	    chunk->size = minsize;
	    chunk->next = 0;
	    return (PTR)( chunk + 1 );
	}
	else if( chunksize > 0 )
	{
	    chunk->size = chunksize;
	    chunk->next = 0;
	    bigblock->used += chunksize + sizeof( MM_BLOCK );
	    MM_dfl_free( pool, (PTR)( chunk + 1 ), err );
	}
    }

    /* Beggar - must get more memory */
    /* Allocate a new bigblock (of at least the size we need) and */
    /* cut our chunk from it. */

    {
	MM_BLOCK *chunk;
	i4	bigsize;

	/* Allocate a large block from the parent. */

	/* Sizing steps are: */
	/* - requested size plus our overhead */
	/* - moved up to requested expand_size */
	/* - rounded up to optimum size for parent */

	bigsize = size + sizeof( MM_DFL_BLOCK );
	bigsize = max( bigsize, pool->expand_size );
	bigsize = MM_ROUND_SIZE( bigsize,
				 pool->parent->service_size,
				 pool->parent->service_cost );

	bigblock = (MM_DFL_BLOCK *)
	    MMalloc( pool->parent, bigsize, stat, err );

	if( !bigblock )
	    return 0;

	bigblock->used = sizeof( MM_DFL_BLOCK );
	bigblock->hdr.size = bigsize;
	bigblock->hdr.next = pool->alloc_list;
	pool->alloc_list = &bigblock->hdr;

	chunk = (MM_BLOCK *)((char *)bigblock + bigblock->used );
	bigblock->used += minsize + sizeof( MM_BLOCK );
	chunk->size = minsize;
	chunk->next = 0;
	return (PTR)( chunk + 1 );
    }
}

STATUS
MM_dfl_free( 
	MM_POOL		*pool,
	PTR 		obj,
	CL_ERR_DESC 	*err )
{
    MM_BLOCK	*chunk = ((MM_BLOCK *)obj) - 1;

    alloced--;
    wastesiz -= (i4)(SCALARP)(chunk->next);

    chunk->next = pool->free_list;
    pool->free_list = chunk;
    return(OK);
}

STATUS
MM_dfl_rm( 
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

    return OK;
}

int
MM_dfl_stat(
	MM_POOL		*pool )
{
    MM_BLOCK	*block;
    int freecnt = 0;
    i4 freesize = 0;

    for( block = pool->free_list; block; block = block->next )
    {
	freecnt++;
	freesize += block->size;
    }
    printf("free blocks/size %d/%d\n", freecnt, freesize );
    printf("alloc blocks/waste %d/%d\n", alloced, wastesiz );
    return(OK);
}

MM_POOL
MM_pool_default[1] =
    {
	MM_pool_page, 0, 0, ME_MPAGESIZE, 0, 
	ME_MPAGESIZE, sizeof( MM_DFL_BLOCK ), 1,
	0, MM_dfl_alloc, MM_dfl_free, MM_dfl_rm,
	0, 0, 0, 0
    } ;
