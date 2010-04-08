/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
 * Name: mmint.h - definitions private to MM implementation
 */

/*
 * Name: MM_ROUND_SIZE - round request size up for parent's benefit
 *
 * This reads:
 *	Add the parent's per-request cost to the requested size,
 *	round up to the parent's preferred service size, and then
 *	substract back off the parent's per-request cost.
 */

# define MM_ROUND_SIZE( n, sz, cost ) \
	( ( ( ( ( n + cost - 1 ) / sz ) + 1 ) * sz ) - cost )

/*
 * Name: MM_MK_FUNC - a pool's create function
 */

typedef STATUS MM_MK_FUNC( 
		MM_POOL		*pool,
		CL_ERR_DESC 	*err );

/*
 * Name: MM_ALLOC_FUNC - a pool's alloc function
 */

typedef PTR MM_ALLOC_FUNC( 
		MM_POOL		*pool,
		i4		size,
		STATUS 		*stat,
		CL_ERR_DESC 	*err );

/*
 * Name: MM_FREE_FUNC - a pool's free function
 */

typedef STATUS MM_FREE_FUNC(
		MM_POOL		*pool,
		PTR 		obj,
		CL_ERR_DESC 	*err );

/*
 * Name: MM_RM_FUNC - a pool's remove function
 */

typedef STATUS MM_RM_FUNC(
		MM_POOL		*pool,
		CL_ERR_DESC 	*err );

/*
 * Name: MM_BLOCK - block header for chunks held by a pool
 */

typedef struct _MM_BLOCK MM_BLOCK ;

struct _MM_BLOCK {
	MM_BLOCK	*next;
	i4		size;
} ;

/*
 * Name: MM_POOL - control structure for memory pool - one per user's tag
 *
 * This is divided up into subsections:
 *
 *	First, the information basically provided to MMmk_pool():
 * 
 *	parent		where this pool goes for more memory
 *	pool_size	initial size - no real reason to keep, but...
 *	max_size	maximum size, enfored by MMalloc()
 *	expand_size	how much to grow the pool by each time
 *	full_func	user function to call to avoid blowing max_size
 *
 *	Next, information for parent and child pools to communicate:
 *
 *	reference_count	how many times this pool is referred to
 *	service_size	the pool's preferred per-request size
 *	service_cost	the pool's per-request overhead
 *
 *	Next, the function points for pool manipulation:
 *
 *	alloc_func	called only by MMalloc()
 *	free_func	called only by MMfree()
 *	rm_func		called by MMrm_pool()
 *
 *	Finally, the pool private data; see the strategy implementation
 *	descriptions for the actual use of this data:
 *
 *	current_size	total memory in pool
 *	alloc_list	(usually) the list of blocks allocated from parent
 *	free_list	(usually) the list of blocks returned 
 *	item_size	individual size for block pool
 */

struct _MM_POOL {
	MM_POOL		*parent;
	i4		pool_size;
	i4		max_size;
	i4		expand_size;
	MM_FULL_FUNC	*full_func;

	i4		service_size;
	i4		service_cost;	
	i4		reference_count;

	MM_MK_FUNC	*mk_func;
	MM_ALLOC_FUNC	*alloc_func;
	MM_FREE_FUNC	*free_func;
	MM_RM_FUNC	*rm_func;

	i4		current_size;
	MM_BLOCK	*alloc_list;	/* pool specific fmt */
	MM_BLOCK	*free_list;	/* pool specific fmt */
	i4		item_size;	/* for block pool only */
} ;

/*
 * Definition of default pools in from mmdflt.c, mmblock.c, mmpage.c
 */

extern MM_POOL MM_pool_default[];
extern MM_POOL MM_pool_block[];
extern MM_POOL MM_pool_page[];

