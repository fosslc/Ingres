/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef _HSH_HDR
# define _HSH_HDR 1


/* 
** Name:	HSH.c	- Define Hash function externs
**
** Specification:
** 	The hash module helps in allocating, deallocating, inserting, and 
**	searching a hash index.  HSH uses the block allocator BA.H to allocate
**	the individual items in the index called an entry.  Each hash index
**	is a pointer to a linked-list described by LL.H.  The entrys are
**	chained in a circular fashion so that no entry has a NULL.
**
**	Entries with identical hash value are in the same chain.
**      The number of slots in the index is specified when the index is
**	created (HSH_New).  The number should be a prime number larger than
**	twice the number of expected entrys.  This ensures a quick lookup.

**	Note that the hsh_entry is defined to be a pointer to a LNKDEF structure.
**	It is assumed that the caller has other fields after the LNKDEF,
**	especially the key value.  The key must be a single piece of storage.
**	It may consist of multiple fields, e.g. long,float,long. 
**		
**	When a hash index is ceated, minimum and maximum values can be
**  	specified for the number of blocks to allocate. The minimum value
**  	ensures that a certain number of blocks are always kept allocated.
**  	The maximum value ensures that too many blocks are not allocated. If
**  	a block becomes free and more than the minimum are allocated, then
**  	that block will be deallocated.
**
** Description:
**	Contains HSH function externs
**
** History:
**	25-Jun-1998 (shero03)
**	    initial creation  
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

struct _HSHDEF
{
	i4	hshSlotNum;		/* Number of Hash Slots (s/b a prime) */
	i4	hshEntryNum;		/* Number of entrys in the index  */
	i4	hshKeyOffset;		/* Offset to the key  */
	i4	hshIndexSize;		/* Size of the index */
	i4	hshFlag;		/* Flag indicators */
	LNKDEF	**hshIndex;		/* Address of Hash Slots */
	BLADEF	hshBlaHdr;		/* Block allocator */
	MU_SEMAPHORE hshSemaphore;	/* Hash semaphore  */
};

# define HSH_SHARED	1	/* support multiple simultaneous req */
# define HSH_VARIABLE   2	/* Variable length entrys	     */
# define HSH_STRKEY     4	/* Key is a string     */
# define HSH_USEBLA	8	/* Reserved use the block allocator */

typedef struct _HSHDEF HSHDEF;

# define HSH_LAST_ENTRY 0xFFFFFFFF	/* invalid address */

LNKDEF *HSH_AllocEntry(    		/* Allocate an entry */
	HSHDEF	*hsh_blk,		/* Hash Index header  */
	char	*hsh_key,		/* Hash Key	*/
	i4	hsh_keylen,		/* Hash Key Length */
	i4	hsh_entrylen		/* Hash Entry Length */
);

bool HSH_AreEntries(			/* True if there are entries */
	HSHDEF	*hsh_blk		/* Hash Index header  */
);

void HSH_DeallocEntry(    		/* Deallocate an entry */
	HSHDEF	*hsh_blk,		/* Hash Index header  */
	LNKDEF	*hsh_entry		/* Entry to free */
);

void HSH_Delete(		/* Unchain an entry from the index & free it */
	HSHDEF	*hsh_blk,	 	/* Hash Index header */
	char	*hsh_key,		/* Hash Key	*/
	i4	hsh_keylen,		/* Hash Key Length */
	LNKDEF	*hsh_entry		/* Entry to remove */
);

void HSH_Dispose(			/* Free all entrys and reset the hdr */
	HSHDEF	*hsh_blk		/* Hash Index header  */
);

LNKDEF *HSH_Find(	 		/* Locate an indexed entry */
	HSHDEF	*hsh_blk,		/* Hash Index header  */
	char	*hsh_key,		/* Hash Key	*/
	i4	hsh_keylen		/* Hash Key Length */
);

void HSH_GetLock(			/* Obtain a Hash index lock */
	HSHDEF	*hsh_blk		/* Hash Index header  */
);

LNKDEF *HSH_Insert(	 	/* Allocate a entry and add it to the index */
	HSHDEF	*hsh_blk,		/* Hash Index header	     */
	char	*hsh_key,		/* Hash Key	*/
	i4	hsh_keylen,		/* Hash Key Length */
	i4	hsh_entrylen		/* Hash Entry Length */
);

void HSH_Link(		 		/* Insert the entry into the index */
	HSHDEF	*hsh_blk,		/* Hash Index header  */
	char	*hsh_key,		/* Hash Key	*/
	i4	hsh_keylen,		/* Hash Key Length */
	LNKDEF	*hsh_entry		/* Entry to link into the index */
);

HSHDEF *HSH_New(	 		/* Allocate a Hash index & init the hd*/
	i4 	num_slots, 		/* number of slots in the index */
	i4 	key_offset,		/* offset to the key */
	i4	entry_size,		/* size of a entry (avg. for VL entrys */
	i4	min_blocks,		/* minumum # of blocks to allocate   */
	i4	max_blocks,		/* maximum # of blocks to allocate   */
	char 	*sem_name, 		/* name given to the Index Semphore */
	i4	flag	   		/* Flags Shared / Variable / 	     */
);

void HSH_Next(				/* Locate the next indexed entry */
	HSHDEF	*hsh_blk,		/* Hash Index header  */
	LNKDEF	***list, 		/* Current index entry	*/
	LNKDEF	**entry,		/* Current Hash entry	*/
	LNKDEF	**nentry		/* Next Hash Entry	*/
);

void HSH_ReleaseLock(			/* Release a Hash index lock */
	HSHDEF	*hsh_blk		/* Hash Index header  */
);

LNKDEF *HSH_Select(			/* Find and unlink a entry */
	HSHDEF	*hsh_blk,		/* Hash Index header  */
	char	*hsh_key,		/* Hash Key	*/
	i4    	hsh_keylen		/* Hash Key Length */
);

void HSH_Unlink(			/* Remove the entry from the index */
	HSHDEF	*hsh_blk,		/* Hash Index header	     */
	char	*hsh_key,		/* Hash Key	*/
	i4    	hsh_keylen,		/* Hash Key Length */
	LNKDEF	*hsh_entry		/* Entry to link into the index */
);

# endif /* !_HSH_HDR */
