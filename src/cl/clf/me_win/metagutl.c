/*
**  Copyright (c) 1989, 2009 Ingres Corporation. All rights reserved.
*/

#include <compat.h>
#include <clconfig.h>
#include <meprivate.h>
#include <qu.h>
#include <cs.h>
#include <me.h>

/******************************************************************************
** Name:	metagutl.c	- Utility routines for tagged memory.
**
** Description:
**	Background:
**	    At the time this was written, the implementation of
**	    tagged memory was inefficient on frees.  MEtfree was implemented
**	    by traversing the allocated list of memory pointed to by
**	    MElist and freeing each node found that match the tag.
**	    Obviously, this could be a very slow traversal.
**
**	    To speed this up, the implementation of tagged memory was
**	    changed, and this files contains routines that help with the
**	    implementation.
**
**	Implementation:
**	    The new implementation of tagged memory uses a hash table to
**	    store chains of the allocated blocks.  Each tag has its own
**          chain of allocated blocks so that freeing a table only
**	    requires finding the chain and then freeing each node on
**	    the chain.
**
**	Defines:
**		IIME_atAddTag	A new block of allocated memory for a tag.
**
**		IIME_ftFreeTag	Free all the memory allocated for a tag.
**
** History:
**	5-dec-1989 (Joe)
**	    First Written
**	08-aug-1995 (canor01)
**	    Add semaphore protection for tag queue
**	21-mar-1996 (canor01)
**	    Free memory from calling process's heap.  Compact the
**	    heap after all the tagged memory has been freed.
**	03-jun-1996 (canor01)
**	    Internally, store the tag as an i4 instead of an i2. This makes
**	    for more efficient code on byte-aligned platforms that do fixups.
**	06-aug-2001 (somsa01)
**	    Use meprivate.h instead of melocal.h .
**	05-jun-2002 (somsa01)
**	    Synched up with UNIX by utilizing MEtaglist_mutex.
**	21-jun-2002 (somsa01)
**	    Removed call to HeapCompact().
**	 5-Aug-2005 (drivi01)
**	    Replaced HeapFree call with free.
** 	23-Sep-2009 (wonst02) SIR 121201
** 	    Initialize freelist pointer, rather than relying on link to do it.
**
******************************************************************************/

/* define's */
/* GLOBALDEF's */
/* extern's */
GLOBALREF CS_SYNCH	MEtaglist_mutex;
/* static's */

/******************************************************************************
** Name:	METAGNODE	- The elements of the hash table chains.
**
** Description:
**	Each bucket of the tagged memory hash table contains a linked
**	list of the chains of allocated memory for the tags that hash
**	to the bucket.  This structure implements the node of the linked
**	list.
**
**	met_list is the head of the chain of allocated blocks.  This is
**	a QUEUE, and is used exactly like the MElist variable, which
**	holds the blocks of memory allocated under tag 0.  As with MElist
**	the blocks contained in this QUEUE are ME_NODEs.
**	An important point here is that since the allocated blocks are
**	on a QUEUE just like MElist, MEfree works correctly when freeing
**	a block of tagged memory.  This does mean that a METAGNODE can
**	contain an empty queue.
**
**	met_tag is the tag for which this node contains the chain of
**	allocated blocks.
**
**	met_hash is the pointer to the next METAGNODE in the hash chain
**	of which this node is a part.
**
******************************************************************************/

typedef struct _metagnode {
	ME_HEAD			met_list;
	i4			met_tag;
	struct _metagnode	*met_hash;
} METAGNODE;

/******************************************************************************
**
** htab is the hash table.  Each bucket points to a linked list of
** METAGNODEs for the tags that hash to this bucket.  The hash function
** is simply the value of the tag modulo 256.  Since tags are normally
** allocated sequentially, this should give as good a distribution as
** anything.
**
******************************************************************************/

static METAGNODE *htab[256] ZERO_FILL;

/******************************************************************************
**
** freelist is a list of free METAGNODEs linked by their met_hash fields.
** When a new METAGNODE is needed, it is taken from this list.  If the
** list is empty, then a number of METAGNODEs is allocated from dynamic
** memory and put on this list.
**
******************************************************************************/

static METAGNODE *freelist = NULL;

/******************************************************************************
** Name:	IIME_adAddTag	- Add a node of allocated memory to a tag.
**
** Description:
**	This routine is called when a new block of dynamic memory is being
**	allocated under a tag.  It is called by MEdoAlloc.  The job
**	of this routine is to store the allocated memory so that it
**	will be freed with the other blocks allocated under this tag when
**	MEtfree is called.
**
**	It works by checking the hash table for an METAGNODE for this tag.
**	If none is found, a new METAGNODE is allocated for this tag.
**	Then the block of memory is put on the QUEUE for the METAGNODE.
**
** Inputs:
**	tag		The tag under which this block of memory is
**			being allocated.
**
**	node		The block of memory being allocated.
**
** Side Effects:
**	This will take a node off freelist, and if necessary will allocate
**	dynamic memory.
**
******************************************************************************/
VOID
IIME_atAddTag(i4 tag,
              ME_NODE * node)
{
    register METAGNODE	**first;

    CS_synch_lock(&MEtaglist_mutex);

    /*
    ** Note that first is a pointer to a pointer.  The loop will cause
    ** a return from the routine if the tag already has an METAGNODE in
    ** the hash table.  If the loop finishes, then first will point to
    ** the pointer that must contain the METAGNODE.
    */
    for (first = &(htab[tag % 256]);
	*first != NULL;
	first = &((*first)->met_hash))
    {
	if ((*first)->met_tag == tag)
	{
	    (void)QUinsert((QUEUE *)node, (QUEUE *)(*first)->met_list.MElast);
	    CS_synch_unlock(&MEtaglist_mutex);
	    return;
	}
    }

    if (freelist == NULL)
    {
	register METAGNODE	*next;
	register int		i;

	freelist = (METAGNODE *) 
			   MEreqmem(0, sizeof(METAGNODE)*50, TRUE, NULL);
	for (i = 0, next = freelist; i < 49; i++)
	{
	    next->met_hash = next + 1;
	    next = next->met_hash;
	}
	next->met_hash = NULL;
    }

    *first = freelist;
    freelist = freelist->met_hash;
    (*first)->met_hash = NULL;
    (*first)->met_tag = tag;
    QUinit((QUEUE *)&(*first)->met_list);
    (void)QUinsert((QUEUE *)node, (QUEUE *)(*first)->met_list.MElast);
    CS_synch_unlock(&MEtaglist_mutex);
}

/******************************************************************************
** Name:	IIME_ftFreeTag	- Free all allocated memory for a tag.
**
** Description:
**	This routine is called by MEtfree to free all the allocated
**	memory for a tag.
**
**	It works by finding the METAGNODE for the tag in the hash table
**	and then traversing the QUEUE of allocated blocks freeing
**	each block.
**
** Inputs:
**	tag		The tag whose memory is to be freed.
**
** Outputs:
**	Returns:
**		OK if all the allocated memory for the tag was freed.
**		ME_NO_TFREE if the tag does not have a record in the hash table.
**		other failure status if the nodes can't be freed.
**
** Side Effects:
**	Will return the METAGNODE for the tag to freelist.
**
******************************************************************************/
STATUS
IIME_ftFreeTag(
	       i4 tag)
{
    register METAGNODE	**first;

    CS_synch_lock(&MEtaglist_mutex);

    for (first = &(htab[tag % 256]);
	 *first != NULL;
	 first = &((*first)->met_hash))
    {
	if ((*first)->met_tag == tag)
	{
	    register ME_NODE	*this;
	    register ME_NODE	*next;
	    register METAGNODE	*freenode;

	    for (this = (*first)->met_list.MEfirst;
		 this != NULL && this != (ME_NODE *)&((*first)->met_list);)
	    {
		next = this->MEnext;
		(void)QUremove((QUEUE *)this);

		free((char *)this);
		this = next;
	    }

	    freenode = *first;
	    *first = freenode->met_hash;
	    freenode->met_hash = freelist;
	    freelist = freenode;
	    CS_synch_unlock(&MEtaglist_mutex);
	    return OK;
	}
    }

    CS_synch_unlock(&MEtaglist_mutex);
    return ME_NO_TFREE;
}
