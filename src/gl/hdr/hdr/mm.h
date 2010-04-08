/*
** mm.h - interface to the generic memory allocator
**
** Description:
**
**	MM provides a general purpose allocator built upon the CL's
**	MEget_pages().  It has the following features:
**
**	-	Memory is allocated out of pools.  Each pool is created
**		by MMmkpoll() and destroyed by MMrmpool().  A default
**		pool, (MM_POOL *)0, is always available.
**
**	-	Each pool has a selectable allocation algorithm, called
**		its "strategy."  The four strategies are defined below.
**
**	-	Pools are layered: each pool draws memory from its parent
**		pool, except for the "page" strategy, which draws directly
**		from MEget_pages().
**
**	-	Each pool can call a user-supplied function to free memory
**		if the pool reaches its maximum.
**
**	-	Each pool has a configurable initial, incremental, and
**		maximum size.
**
**	-	Creation of pools are little more than the cost of the
**		allocations themselves.
**
**	The four strategies are:
**
**	MM_DEFAULT_STRATEGY
**		Like a normal allocator, allocates and frees any sized
**		objects in any order.  Rounds requested size up to alignment
**		boundaries.  Does not return memory to the parent pool until 
**		the pool is removed.  Uses 8 bytes per requested object plus
**		16 bytes from each object requested of parent pool.
**
**	MM_MICRO_STRATEGY
**		A micro-allocator and mark-allocator.  Like a micro-allocator,
**		it allocates any number of any sized objects quickly, without
**		rounding the requested size.  Like a mark-allocator, freeing
**		any object also frees all object allocated subsequent to that
**		object.  Does not return memory to the parent pool until the
**		pool is removed.  Uses 16 bytes from each object requested
**		of parent pool.
**
**	MM_BLOCK_STRATEGY
**		Allocates and frees quickly single sized objects only.  The
**		object size is determined on the first call to MMalloc().
**		Useful for control blocks.  Does not return memory to the 
**		parent pool until the pool is removed.  Uses 8 bytes from
**		each object requested of the parent pool.  
**
**	MM_PAGE_STRATEGY
**		A wrapper for MEget_pages(), allocates and frees whole
**		pages of memory as requested.  Rounds requested size up
**		to ME_MPAGESIZE.  Returns memory with MEfree_pages() as
**		soon as the pages are freed.  Does not use a parent pool.
**
**	Three default pools exist, but only one is available the user:
**
**	1)	An MM_DEFAULT_STRATEGY pool to be used when a pool
**		of (MM_POOL *)0 is handed to MMalloc() or MMfree().
**
**	2)	An MM_BLOCK_STRATEGY pool used by MMmkpool() and MMrmpool()
**		for MM_POOL control blocks.  It should be emphasized that
**		these control blocks do not come from the parent pool.
**
**	3)	An MM_PAGE_STRATEGY pool, used by the two pools above.
**
**	Pools may be layered freely, with the following provisions:
**
**	-	The MM_PAGE_STRATEGY directly calls MEget_pages(),
**		and ignores its parent parameter.
**
**	-	The MM_MICRO_STRATEGY cannot be layered upon, because
**		multiple allocators using such a parent pool would
**		interfere with each other.
**
**	-	All strategies except MM_PAGE_STRATEGY use some memory
**		of each object allocated from the parent, and thus cannot
**		pass on whole pages to their children.  For example,
**		using MM_BLOCK_STRATEGY on top of MM_PAGE_STRATEGY for 
**		blocks that are ME_MPAGESIZE in size would force
**		MM_BLOCK_STRATEGY to ask for an extra page from
**		its parent merely to accomodate its overhead.
**
** History:
**	1-Nov-93 (seiwald)
**	    Documented.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Name: MM_POOL - definition of a pool, opaque to user
**
** Description:
**	MMmkpool creates a memory pool and returns a pool pointer
**	of type (MM_POOL *) to the user.  The user is required to
**	carry this handle around and pass it to MMalloc(), MMfree(),
**	and MMrmpool().
**
**	Its actual contents are described in mmint.h, for use internally
**	within the MM routines.
*/

typedef struct _MM_POOL MM_POOL;

/*
** Name: MM_FULL_FUNC() - full pool callback function
**
** Description:
**	If MMalloc() detects that a pool is full it ask the user to 
**	relinquish memory by calling the function (*full_func)(),
**	provided by the user to MMmkpool().  If this function returns 
**	OK, the allocation is reattempted.  Otherwise the allocation
**	fails.
*/

typedef STATUS MM_FULL_FUNC( MM_POOL *pool, i4 reqlen );

/*
** Name: MMmkpool - create a pool
**
** Create an allocation pool with the specified attributes, and return
** a pointer that may be used on future allocation and release requests.
**
** Inputs:
**
**    strategy, one of:
**
**	MM_DEFAULT_STRATEGY 	many objects, individually freed
**	MM_MICRO_STRATEGY 	many objects, freed by destroying pool only
**	MM_BLOCK_STRATEGY 	many same sized objects, freed individually
**	MM_PAGE_STRATEGY 	big objects, freed individually
**
**    parent
**		either the pool handle of a parent pool, or 0 for
**		direct use of MEget_pages.
**
**    init_pool_size	
**		The pool is initialized with this many bytes, possibly 0.
**
**    max_pool_size
**		The pool may not grow larger than this size in bytes.
**		0 means unlimited expansion is allowed.
**
**    expand_increment
**		Expand the pool in nominally this many byte increments.	
**		(The increment may be rounded up). 0 means whatever
**		seems appropriate.
**
**    full_func
**		Function called when memory in the pool is exhausted,
**		either by running into pool maximum, or by underlying
**		allocator failure.  This function may call MMfree to try to free
**		enough memory for the allocation to succeed.  If it returns
**		OK, we'll try to make the allocation again.  If it returns
**		not-OK, we won't even try.  NULL means don't call anything
**		on a space exhaustion error.
**
** Outputs:
**
**    out_pool	filled in with a PTR identifying the pool.
**
**    err	filled in with system specific error status on failure.
**
** Returns:
**    OK or system-specific error code.
*/

STATUS
MMmkpool( i4		strategy,
	MM_POOL		*parent,
	i4		init_pool_size,
	i4 	max_pool_size,
	i4 	expand_increment,
	MM_FULL_FUNC	*full_func,
	MM_POOL		**out_pool,
	CL_ERR_DESC	*err );

# define	MM_DEFAULT_STRATEGY	0
# define	MM_PAGE_STRATEGY	1
# define	MM_MICRO_STRATEGY	2
# define	MM_BLOCK_STRATEGY	3

/*
** Name: MMrmpool() - destroy pool created with MMmkpool()
**
** If the requested pool still has child pools, its actual deletion
** is deferred until the children pools have been removed.  Once
** all references are gone, all memory allocated from parent is
** freed and the pool control block itself is freed.
**
** Inputs:
**	pool	pool handle as returned by MMmkpool().  It is not
**		permitted to remove pool 0 (the default pool).
** Outputs:
**	err	CL error status.
**
** Returns:
**    OK or system-specific error code.
*/

STATUS
MMrmpool( MM_POOL *pool, CL_ERR_DESC *err );

/*
** Name: ifdef MM_DEBUG - compile debugging allocator
**
** Description:
** 	If the calling code is compiled with MM_DEBUG, MMalloc() and 
**	MMfree() are wrapped with versions that add debugging at 
**	considerable cost:
**
**	-	MMalloc() surrounds the allocated block with a header
**		and a tail, both containing magic numbers guarding 
**		overwrites.  The header also contains the pool pointer,
**		the value of __FILE__ and __LINE__ at the time of the
**		allocation.
**
**	-	MMfree() verifies the free is for the proper pool, 
**		checks the guarding magic numbers, and then fills the
**		allocated block with -1s.
*/

# ifdef MM_DEBUG
# define MMalloc( p,sz,st,er ) 	MM_dballoc( p,sz,st,er,__FILE__,__LINE__ )
# define MMfree( p,o,er ) 	MM_dbfree( p,o,er )
# else 
# define MMalloc( p,sz,st,er ) 	MM_alloc( p,sz,st,er )
# define MMfree( p,o,er ) 	MM_free( p,o,er )
# endif

/*
** Name: MMalloc() - allocate memory from a pool
**	 MMcalloc() - allocate and clear memory
**
** Inputs:
**	pool	pool handle as returned by MMmkpool(), or 0 to use
**		the default pool.
**
**	size	The amount of memory requested.   For MM_BLOCK_STRATEGY,
**		this must be the same for each call to MMalloc() for
**		the pool.  For MM_DEFAULT_STRATEGY and MM_BLOCK_STRATEGY,
**		this is rounded up to the machine alignment boundary.
**		For MM_PAGE_STRATEGY, this is rounded up to ME_MPAGESIZE.
**
** Outputs:
**	stat	INGRES error STATUS; set when MMalloc() returns NULL.
**
**	err	CL error status.
**
** Returns:
**	Pointer to allocated block or NULL on error.
*/

PTR
MM_alloc( MM_POOL	*pool,
	 i4	size,
	 STATUS		*stat,
	 CL_ERR_DESC	*err );	

PTR
MM_dballoc( MM_POOL	*pool,
	 i4	size,
	 STATUS		*stat,
	 CL_ERR_DESC	*err,
	 char		*file,
	 i4		line );	

# define MMcalloc( p,si,sz,er ) MMclear( MMalloc( p,si,sz,er ), sz )

/*
** Name: MMfree() - frees memory allocated with MMalloc()
**
** Inputs:
**	pool	pool handle as returned by MMmkpool(), or 0 to use
**              the default pool.  
**
**	obj	The object to be freed.  For MM_MICRO_STRATEGY, all
**		objects allocated subsequently to obj are freed as
**		well.
**
** Outputs:
**	err	CL error status.
**
** Returns:
**    OK or system-specific error code.
*/

STATUS
MM_free( MM_POOL *pool, PTR obj, CL_ERR_DESC *err );

STATUS
MM_dbfree( MM_POOL *pool, PTR obj, CL_ERR_DESC *err );

/*
** Name: MMclear() - clear memory allocated by MMalloc()
**
** Description:
**	This routine exists for the benefit of MMcalloc().
**	after a failed buffer allocatio
*/

PTR
MMclear( PTR obj, i4 size );

