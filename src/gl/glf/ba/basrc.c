/*
** Copyright (c) 2004 Ingres Corporation
**
*/

# include <compat.h>
# include <gl.h>
# include <lo.h>
# include <sl.h>
# include <cs.h>
# include <ll.h>
# include <me.h>
# include <mu.h>
# include <ba.h>

/* 
** Name:	BA.c	- Define Block Allocator function 
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
**	18-jan-1999 (canor01)
**	    removed dependency on headers outside the CL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	7-oct-2004 (thaju02)
**	    Change allocated to SIZE_TYPE.
**/

#define   AddFUnitToFreeList(blk, blu)\
{\
  LL_Init(&(blu)->bluVariant.lnk, (blu)); \
  LL_LinkAfter(&(blk)->blaHdrFreeUnits, &(blu)->bluVariant.lnk); \
  (blk)->blaFreeUnits++; \
  (blu)->bluFixed.blh->blhFreeUnits++; \
}	   	/* proc AddFUnitToFreeList()     */

#define   AddVUnitToFreeList(blk, blv)\
{\
  LL_Init(&(blv)->blvVariant.lnk, (blv)); \
  LL_LinkAfter(&(blk)->blaHdrFreeUnits, &(blv)->blvVariant.lnk); \
  (blk)->blaFreeUnits++; \
  (blv)->blvFixed.blh->blhFreeUnits++; \
}	   	/* proc AddVUnitToFreeList()     */

#define   RemFUnitFromFreeList(blk, blu)\
{\
  LL_Unlink(&(blu)->bluVariant.lnk); \
  (blk)->blaFreeUnits--; \
  (blu)->bluFixed.blh->blhFreeUnits--; \
}	   	/* proc RemFUnitFromFreeList()   */

#define   RemVUnitFromFreeList(blk, blv)\
{\
  LL_Unlink(&(blv)->blvVariant.lnk); \
  (blk)->blaFreeUnits--; \
  (blv)->blvFixed.blh->blhFreeUnits--; \
}	   	/* proc RemVUnitFromFreeList()   */

struct	  _BLHDEF
{
  i4  	     blhFreeUnits;	/* number of free-units in this blk */
  LNKDEF     blhLnk;		/* link for list allocated blocks   */
};
typedef struct _BLHDEF BLHDEF;

struct	  _BLUDEF
{
  struct
    {
    BLHDEF*   blh;		/* -> header of this unit's block   */
    }  bluFixed;	 	/* fixed field			    */
  union
    {
    LNKDEF    lnk;		/* to link in list of free units    */
    char      data[1];		/* user's data in the unit	    */
    } bluVariant;		/* variant field		    */
};

typedef struct _BLUDEF BLUDEF;

struct	  _BLVDEF
{
  struct
    {
    BLHDEF*   blh;		/* -> header of this unit's block   */
    i4	      blv_size;		/* size of this piece		    */
    }  blvFixed; 		/* fixed field			    */
  union
    {
    LNKDEF    lnk;		/* to link in list of free units    */
    char      data[1];		/* user's data in the unit	    */
    } blvVariant;		/* variant field		    */
};

typedef struct _BLVDEF BLVDEF;

static void BlkDealloc(BLADEF *blk, BLHDEF *blh);
static bool BlkAlloc(BLADEF *blk);
static void *sysalloc(i4 size);
static void sysfree(PTR area, i4  size);

# define ROUND_UP(x,y) (((x) + (y) - 1)/(y) * (y))


/*{
** Name:	BA_New	- Define a new Block Allocator
**
** Description:
**	Define a new Block Allocator
**
** Inputs:
**    unit_size		The size of memory unit that the user wants to allocate
**    			with each call to 'blkAlloc()'.
**    min_blocks	The minimum # of blocks to keep allocated.
**    max_blocks	The maximum # of blocks to allocate.
**    			This parameters help control the growth of the allocated space.
**
** Outputs:
**
** Returns:
**    blk		The block header
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/
void BA_New(BLADEF *blk, i4  unit_size,
	 i4  min_blocks, i4  max_blocks, i4  flags)
{
	
	BLUDEF  blutmp;		/* -> Fixed len block unit   	    */
	BLUDEF  *blu=&blutmp;
	BLVDEF  blvtmp;		/* -> Variable len block unit 	    */
	BLVDEF  *blv=&blvtmp;
	i4	blhsize;
	i4	i, k, p;
	i4	wasted, minWasted = MAXI4;
	i4	units_per_block = 1;
	bool	ret = TRUE;

  /*----------------------------------------------------------------------
    When we allocate a unit, we need to add a little more to fit in the
    fixed part of the block header that is not shared with the user (i.e 
    the part that is not in the variant field).
    Also, round up the unit-size to a multiple of 8-bytes
    (to avoid alignment problems).
    --------------------------------------------------------------------*/
  if (BLA_VARIABLE & flags)
    unit_size = max(unit_size,		/* ensure variant part can fit  */
                  sizeof(blv->blvVariant));
  else
    unit_size = max(unit_size,		/* ensure variant part can fit  */
                  sizeof(blu->bluVariant));
  	
  if (BLA_VARIABLE & flags)
    unit_size += sizeof(BLVDEF) - 		/* add in fixed part    */
               sizeof(blv->blvVariant);
  else
    unit_size += sizeof(BLUDEF) - 		/* add in fixed part	*/
               sizeof(blu->bluVariant);

  unit_size = ROUND_UP(unit_size, sizeof(double)); 
  
  blhsize = ROUND_UP(sizeof(BLHDEF), sizeof(double));/* round to a 8 CMD*/
  /*----------------------------------------------------------------------
    Determine a "good" units-per-block
    We want a block that's paged aligned 
    The block can be no larger than 5 pages
    Use the block size that gives the least waste 
    ------------------------------------------------------------------- */
  for (i = 0; i < 5; i++)
  {
  	if (unit_size > ME_MPAGESIZE * (i + 1))
	  continue;
  	k = (ME_MPAGESIZE * (i + 1) - blhsize) / unit_size;
  	wasted = ME_MPAGESIZE * (i + 1) - k * unit_size;
	if (wasted < minWasted)
	{
	   units_per_block = k;
	   p = i + 1;
	   minWasted = wasted;
	}
  }

  /*----------------------------------------------------------------------
    Initialize a new 'blk' handle 
    ------------------------------------------------------------------- */
  blk->blaUnitSize = unit_size;
  blk->blaUnitsPerBlk = units_per_block;
  blk->blaBlks = 0;
  blk->blaMinBlks = max(1,min_blocks);
  blk->blaMaxBlks = max(max_blocks, min_blocks);
  blk->blaBlkPages = p; 		 
  LL_Init(&blk->blaHdrBlks, NULL);	/* init list of allocated blocks*/
  LL_Init(&blk->blaHdrFreeUnits, NULL); /* init list of free-units    */
  blk->blaFlags = flags;
  
  if (BLA_SHARED & flags)
      MUw_semaphore(&blk->blaSemaphore, "Block Allocator");

  /*----------------------------------------------------------------------
    Make sure that the specified minimum # of blocks are allocated.
    ------------------------------------------------------------------- */
  while ((blk->blaBlks < blk->blaMinBlks) && 
  	 (ret))
     ret = BlkAlloc(blk);	/* allocate blocks		    */

  return;

}		    /* proc BL_New()		    */


/*{
** Name:	BA_Dispose	- Dispose of a Block Allocator
**
** Description:
**	Dispose of a Block Allocator created from BL_New
**
** Inputs:
**    blk		The block header
**
** Outputs:
**
** Returns:
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/

void      BA_Dispose(BLADEF *blk)
{
	  BLHDEF    *blh;		/* -> block of allocation     */

  /*----------------------------------------------------------------------
    Dispose of all allocated blocks.
    ------------------------------------------------------------------- */
  if (BLA_SHARED & blk->blaFlags)
      MUp_semaphore(&blk->blaSemaphore);

  while(blh = LL_GetNextObject(&blk->blaHdrBlks))
  {
    LL_Unlink(&blh->blhLnk);		/* unlink it from the list     */
    sysfree((char *)blh, blk->blaBlkPages);/* deallocate the block     */
  }
  if (BLA_SHARED & blk->blaFlags)
      MUv_semaphore(&blk->blaSemaphore);

  /*----------------------------------------------------------------------
    Reset the 'blk' handle
    ------------------------------------------------------------------- */
  blk->blaUnitSize = 0; 		/* for assertion purposes     */
  if (BLA_SHARED & blk->blaFlags)
      MUr_semaphore(&blk->blaSemaphore);

  return;
}  	/* proc BA_Dispose()		    */

/*{
** Name:	BA_UnitAlloc	- Allocate a fixed or variable length unit
**
** Description:
**	Allocate a fixed or variable length unit from a block
**
** Inputs:
**    blk		The block header
**    size		The size of memory needed (Ignored for Fixed len units)
**
** Outputs:
**
** Returns:
**	address of unit
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
**	25-Jun-1998 (shero03)
**	    Combine FUnitAlloc and VUnitALloc
*/
PTR BA_UnitAlloc(BLADEF *blk, i4  size)
{
	  LNKDEF   *entry;		/* -> free element	        */
	  LNKDEF   *list;		/* -> free element list hdr     */
	  BLVDEF   *blv; 		/* -> Variable len block unit   */
	  BLVDEF   *blvnew;		/* -> block free piece remainder*/
	  BLUDEF   *blu; 		/* -> Fixed len block unit   */
	  PTR	   unit = NULL;		/* -> user's data	        */
	  i4	   lsize;		/* size of free area needed     */
	  bool	   ret;

  if (BLA_VARIABLE & blk->blaFlags)
  {	  /* Variable Length Units */
      
  	if (size == 0)
      return NULL;			/* invalid size		     */

  	lsize = max(size, sizeof(blv->blvVariant)); /* ensure a minimum len   */
  	lsize += sizeof(BLVDEF) - sizeof(blv->blvVariant); /* add in fixed    */
  	lsize = ROUND_UP(lsize, sizeof(double));/* compute size of area      */

  	/*----------------------------------------------------------------------
    Go through the free list looking for a piece of storage at least the 
    requested size.
    ------------------------------------------------------------------- */
 	if (BLA_SHARED & blk->blaFlags)
      MUp_semaphore(&blk->blaSemaphore);

  	list = &blk->blaHdrFreeUnits;
  	entry = LL_GetNext(list);

  	while (TRUE) 
  	{
      blv = (BLVDEF*)(LL_GetObject(entry));	/* look at the bvh */
      if (blv)
      {
      	  if (blv->blvFixed.blv_size >= lsize) 	/* large enough to use? */
		  {
	      	if (blv->blvFixed.blv_size > lsize + blk->blaUnitSize)
	      	{
	      		blvnew = (BLVDEF*)((char*)blv + lsize); /* -> new area*/
		  		blvnew->blvFixed.blh = blv->blvFixed.blh;
		  		blvnew->blvFixed.blv_size = blv->blvFixed.blv_size - lsize;
		  		AddVUnitToFreeList(blk, blvnew); /* create a new unit */
		  		blv->blvFixed.blv_size = lsize;  /* adjust the size   */
	      	}
          
		  RemVUnitFromFreeList(blk, blv);/* remove entry from list  */
          unit = (PTR)(blv->blvVariant.data);  /* -> user's data    */
	      break;
		  }
      }
      	      		    
      entry = LL_GetNext(entry);	/* go to the next entry */
      if (entry == list)			/* if no more entries   */
          ret = BlkAlloc(blk);      /* allocate another blk */
      if (!ret)                     /* if that fails        */
      	  break;                    /* return a null entry  */
	}
  }
  else
  {	/* Fixed length Units */   
  	/*----------------------------------------------------------------------
   	 Make sure that free-list is not empty by allocating a new block if
   	 necessary.
    ------------------------------------------------------------------- */
  	if (!blk->blaFreeUnits)		/* no free-units?	     */
   	 ret = BlkAlloc(blk);      	/* allocate a new block	     */

  	if (!ret)				/* still no free-units?	     */
   	 return NULL;

  	if (BLA_SHARED & blk->blaFlags)
   	   MUp_semaphore(&blk->blaSemaphore);

  	/*--------------------------------------------------------------------
   	 Unlink a unit from the top of the free-list.
   	 ------------------------------------------------------------------  */
  	
	blu = (BLUDEF*)LL_GetNextObject(&blk->blaHdrFreeUnits);
  	RemFUnitFromFreeList(blk, blu);	/* remove this from free-list   */
  	unit = (PTR)(blu->bluVariant.data);
  }

  if (BLA_SHARED & blk->blaFlags)
  	MUv_semaphore(&blk->blaSemaphore);

  return unit;			 	/* return the unit's data area  */

}     			/* proc BA_UnitAlloc()		    */

/*{
** Name:	BA_UnitDealloc	- Deallocate a unit 
**
** Description:
**	Add the unit back to the free chain
**
** Inputs:
**    blk		The block header
**    unit_ptr		address of unit
**
** Outputs:
**
** Returns:
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/

void      BA_UnitDealloc(BLADEF *blk,char *unit_ptr)
{
	  BLUDEF*   blu;		/* -> Fixed len block-unit      */
	  BLVDEF*   blv;		/* -> Variable len block-unit   */
	  BLHDEF*   blh;		/* -> block header	        */

  /*----------------------------------------------------------------------
    Link the unit into the free-unit list.
    ------------------------------------------------------------------- */
  if (BLA_SHARED & blk->blaFlags)
      MUp_semaphore(&blk->blaSemaphore);
  if (BLA_VARIABLE & blk->blaFlags)
  {
      blv = (BLVDEF*)			/* locate the unit's header	     */
        ((char *)unit_ptr - sizeof(blv->blvFixed));
      AddVUnitToFreeList(blk, blv);	
      blh = blv->blvFixed.blh;		/* locate header of this unit's blk  */
  }
  else
  {
      blu = (BLUDEF*)			/* locate the unit's header	     */
        ((char *)unit_ptr - sizeof(blu->bluFixed));
      AddFUnitToFreeList(blk, blu);	
      blh = blu->bluFixed.blh;		/* locate header of this unit's blk  */
  }

  if (BLA_SHARED & blk->blaFlags)
      MUv_semaphore(&blk->blaSemaphore);
  
  /*----------------------------------------------------------------------
    If we this block became completely free, and we have more than the
    minimum number of blocks, then deallocate the block.
    ----------------------------------------------------------------------   */
  if ((blh->blhFreeUnits ==  /* all units are free? 	    */
       blk->blaUnitsPerBlk)     &&
      (blk->blaBlks > blk->blaMinBlks)) /* and more than minimum alloc'ed?   */
  {
    BlkDealloc(blk, blh);		/* deallocate this block	     */
  }

  return;
}			/* proc BA_UnitDealloc()		    */

/*{
** Name:	blkAlloc	- Allocate a block of storage  
**
** Description:
**	Allocate a new block of storage
**	Called when the free-units list becomes empty to allocate a fresh set
**	of units.
**
** Inputs:
**    blk		The block header
**    unit_ptr		address of unit
**
** Outputs:
**
** Returns:
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/

bool      BlkAlloc(BLADEF *blk)
{
	  BLHDEF    *blh;		/* -> block header		     */
	  char      *unit_p;		/* -> allocation unit		     */
	  i4	    i;			/* index			     */

  /*-----------------------------------------------------------------------
    Check if we will exceed the max number of blocks
    -----------------------------------------------------------------------  */
   if (blk->blaBlks >= blk->blaMaxBlks)
     return FALSE;	/* trying to alloc too many blocks  */

  /*-----------------------------------------------------------------------
    Allocate a new block.
    -----------------------------------------------------------------------  */
  blh = (BLHDEF*)sysalloc(blk->blaBlkPages); /* allocate it		     */

  if (!blh)
      return FALSE;			/* memory wasn't allocated	     */

  /*-----------------------------------------------------------------------
    Initialise the block-header and link it to the blocks-list.
    -----------------------------------------------------------------------  */
  blh->blhFreeUnits = 0;		/* clear the free unit count 	     */
  LL_Init(&blh->blhLnk, blh);
  if (BLA_SHARED & blk->blaFlags)
      MUp_semaphore(&blk->blaSemaphore);	/* Lock the blk list	     */
  LL_LinkBefore(&blk->blaHdrBlks, &blh->blhLnk);
  blk->blaBlks++;			/* increment the block count	     */
  					/* -> beginning of usable memory     */
  unit_p = (char *)blh + ROUND_UP(sizeof(BLHDEF), sizeof(double)); 

  if (BLA_VARIABLE & blk->blaFlags)
  {
      /* 
      ** Add a sinle entry in the free list for this whole area
      */
      ((BLVDEF*)unit_p)->blvFixed.blh = blh;/* set -> to block-header       */
      ((BLVDEF*)unit_p)->blvFixed.blv_size =    /* set to all free space    */
      	(blk->blaBlkPages * ME_MPAGESIZE) -
        ROUND_UP(sizeof(BLHDEF), sizeof(double)); 
      AddVUnitToFreeList(blk, (BLVDEF*)unit_p);
  }
  else
  {
      /*---------------------------------------------------------------------
        Add all units in this block to the free-list.
       --------------------------------------------------------------------- */
      for ( i = 0; i < blk->blaUnitsPerBlk;
          unit_p += blk->blaUnitSize, i++)
      {
        ((BLUDEF*)unit_p)->bluFixed.blh = blh; /* set -> to block-header     */
        AddFUnitToFreeList(blk, (BLUDEF*)unit_p);
      }
  }

  if (BLA_SHARED & blk->blaFlags)
      MUv_semaphore(&blk->blaSemaphore);	/* Release the blk list	    */

  return TRUE;
}		   	/* proc BlkAlloc()		    */

/*{
** Name:	blkDeAlloc	- Deallocates a block of storage  
**
** Description:
**	Deallocate a new block of storage
**	Called when all the units of a block are free and thus
**	the block can be deallocated.
**
** Inputs:
**    blk		The block header
**    blh		The block
**
** Outputs:
**
** Returns:
**
** History:
**	23-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/
static void BlkDealloc(BLADEF *blk, BLHDEF *blh)
{
	  i4       blhsize;    		/* size for block header	     */
	  i4   	   size;		/* size to allocate		     */
	  char     *unit_p;		/* -> allocation unit		     */
	  i4	   i;			/* index			     */

  /*-----------------------------------------------------------------------
    Compute sizes
    -----------------------------------------------------------------------  */
  blhsize = ROUND_UP(sizeof(BLHDEF), sizeof(double));/* rnd up to factor of 8*/
  size = blhsize +			/* compute allocation size	     */
         (blk->blaUnitSize * blk->blaUnitsPerBlk);

  /*-----------------------------------------------------------------------
    Remove all units belonging to this block from the free-list.
    -----------------------------------------------------------------------  */
  unit_p = (char *)blh + blhsize; 	/* -> beginning of array of units    */
  if (BLA_SHARED & blk->blaFlags)
      MUp_semaphore(&blk->blaSemaphore);

  if (BLA_VARIABLE & blk->blaFlags)
  {
      RemVUnitFromFreeList(blk, (BLVDEF*)unit_p);
  }
  else 
  {
      for (i = 0; i < blk->blaUnitsPerBlk; 
          unit_p += blk->blaUnitSize, i++)
      { 
          RemFUnitFromFreeList(blk, (BLUDEF*)unit_p);
      }
  }

  if (BLA_SHARED & blk->blaFlags)
      MUv_semaphore(&blk->blaSemaphore);

  /*-----------------------------------------------------------------------
    Unlink the block from the list of blocks and deallocate it.
    -----------------------------------------------------------------------  */
  LL_Unlink(&blh->blhLnk);		/* unlink from list		     */
  blk->blaBlks--;			/* decrement the count of blocks     */
  sysfree((char *)blh, blk->blaBlkPages);/* deallocate the whole block	     */

  return;
}	     	/* proc BlkDealloc()	    */

/*{
** Name:	sysalloc	- Allocates a block of storage  
**
** Description:
**	request storage from the operating system
**
** Inputs:
**    num_pages	The size of storage needed in pages
**
** Outputs:
**
** Returns:
**      The address of the area.
**	NULL if no storage is available or if there is a problem during
**	allocation
**	
** History:
**	24-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
**      18-jan-1999 (canor01)
**          replace DB_STATUS with STATUS.
*/
static void *sysalloc(i4 num_pages)
{
	STATUS		status = OK;
	PTR		area;
	SIZE_TYPE	allocated;
	CL_ERR_DESC	err;

	status = MEget_pages(ME_MZERO_MASK,
			     num_pages, 
			     NULL,
			     &area, &allocated, &err); 
	if (status == OK)
	    return (void *)area;
	else
	    return NULL;

}	     	/* proc sysalloc()	    */

/*{
** Name:	sysfree	- Free a block of storage  
**
** Description:
**	free storage that had been requested from sysalloc
**
** Inputs:
**    area	The area to be freed
**    num_pages The size of the object in pages
**
** Outputs:
**
** Returns:
**	
** History:
**	24-Feb-1998 (shero03)
**	    initial creation (based on lnksrc in SQLBASE) 
*/
static void sysfree(PTR area, i4  num_pages)
{
	CL_ERR_DESC	err;

	MEfree_pages( area, num_pages, &err);
	return;

}	     	/* proc sysfree()	    */

