/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef _BA_HDR
# define _BA_HDR 1


/* 
** Name:	BA.c	- Define Block Allocator function externs
**
** Specification:
** 	The block-allocation module helps in allocating and deallocating 
**	memory units in a "block" fashion; i.e, instead of allocating one
**	structure at a time, a bunch of structures are allocated in one shot.
**	The module automatically takes care of memory alignment
**	(i.e each allocated structure will be correctly aligned in memory).
**	There is separate algorithm for fixed and variable length units.
**	When a new block is allocated, either the area is formatted as
**	separate pieces of free fixed length memory, or it is formatted as
**	a single piece of free variable length memory.  When a piece of fixed
**	memory is requested, the first of the free list is unchained and
**	given to the caller.  When a piece of variable memory is requested,
**	the free chain is searched for a piece large enough to satisfy the 
**	request.  If one is found, the requested piece is carved from the 
**	free area.  The length is updated.  The free chain is kept in
**	address sequence for VL units, so that two separate pieces can be 
**	globbed together to form a single larger space.
**
**	When a block-allocator is created, minimum and maximum values can be
**  	specified for the number of blocks to allocate. The minimum value
**  	ensures that a certain number of blocks are always kept allocated.
**  	The maximum value ensures that too many blocks are not allocated. If
**  	a block becomes free and more than the minimum are allocated, then
**  	that block will be deallocated.
**
** Description:
**	Contains BA function externs
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on blksrc in SQLBASE) 
**	25-Jun-1998 (shero03)
**	    Combine VUnitAlloc and FUnitAlloc
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

struct _BLADEF
{
	i4	blaUnitSize;		/* Size of each memory unit	     */
	i4	blaUnitsPerBlk;		/* number of units per block	     */
	i4	blaBlkPages;		/* number of pages in a block	     */
	i4	blaBlks;		/* number of blocks allocated	     */	      
	i4	blaMinBlks;		/* minimum # of blocks to allocate   */
	i4	blaMaxBlks;		/* maximum # of blocks to allocate   */
	i4	blaFreeUnits;		/* number of free units		     */
	i4	blaFlags;		/* Shared / Variable		     */
	LNKDEF	blaHdrBlks;		/* hdr of list of allocated blocks   */
	LNKDEF	blaHdrFreeUnits;	/* hdr of list of free units	     */
	MU_SEMAPHORE blaSemaphore;	/* block structure semaphore	     */
};

typedef struct _BLADEF BLADEF;

# define BLA_SHARED	1		/* support multiple simultaneous req */
# define BLA_VARIABLE   2		/* Variable length units	     */

void BA_New(             /* Initialize a new block allocator	*/
	BLADEF	*blk_header,		/* block allocator header	     */
	i4	unit_size,		/* size of a unit (avg. for VL units */
	i4	min_blocks,		/* minumum # of blocks to allocate   */
	i4	max_blocks,		/* maximum # of blocks to allocate   */
	i4	flag			/* Flags Shared / Variable / 	     */
);

void BA_Dispose(			/* Free all blocks and reset alloc.  */
	BLADEF	*blk_header		/* block allocator header	     */
);

PTR BA_UnitAlloc(			/* Allocate a F/VL unit from a block    */
	BLADEF	*blk_header,	/* block allocator header	     */
	i4	unit_size			/* requested unit size (ignored for F) */
);

void BA_UnitDealloc(			/* Deallocate a F/VL unit from a blk */
	BLADEF	*blk_header,		/* block allocator header	     */
	PTR	unit_addr		/* Address of unit to deallocate     */
);

# endif /* !_BA_HDR */
