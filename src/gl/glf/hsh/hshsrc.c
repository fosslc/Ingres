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
# include <st.h>
# include <hsh.h>

/* 
** Name:	HSHSRC.c	- Define Hash functions 
**
** Specification:
** 	The hash module helps in allocating, deallocating, inserting, and 
**	searching a hash index.  HSH uses the block allocator BA.H to allocate
**	the individual items in the index called an entry.  Each hash index
**	is a pointer to a linked-list described by LL.H.  The entrys are
**	chained in a circular fashion so that no entry has a NULL.
**	Entries with identical hash value are in the same chain.
**      The number of slots in the index is specified when the index is
**	created (HSH_New).  The number should be a prime number larger than
**	twice the number of expected entrys.  This ensures a quick lookup.
**  Note that the hsh_entry is defined to be a pointer to a LNKDEF structure.
**	It is assumed that the caller has other fields after the LNKDEF,
**	especially the key value.  The key must be a single piece of
**	storage.  It may consist of multiple fields, e.g. long,float,char.
**	There are binary and string keys. 
**		
**	When a hash index is ceated, minimum and maximum values can be
**  	specified for the number of blocks to allocate. The minimum value
**  	ensures that a certain number of blocks are always kept allocated.
**  	The maximum value ensures that too many blocks are not allocated. If
**  	a block becomes free and more than the minimum are allocated, then
**  	that block will be deallocated.
**
** Description:
**	Contains HSH functions
**
** History:
**	25-Jun-1998 (shero03)
**	    initial creation  
**	24-nov-1998 (mcgem01)
**	    Avoid an access violation in starting the ICE server by 
**	    checking for a NULL hash block.
**	18-jan-1999 (canor01)
**	    Remove non-CL header file dependencies.
**	27-Jan-1999 (shero03)
**	    Fix an over allocation by dividing by the page size
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	7-oct-2004 (thaju02)
**	    Change allocated to SIZE_TYPE.
**  16-Aug-2005 (fanra01)
**      Bug 115073
**      A change in the memory allocation functions has exposed a memory
**      free error causing an exception during the shutdown of the icesvr.
**      Update HSH_dispose to free the hash block containing the index.
**/

/* Static function declarations */

# define ROUND_UP(x,y) (((x) + (y) - 1)/(y) * (y))

static u_i4 HashStr( const unsigned char *name);
static u_i4 HashBin( const unsigned char *name, const i4  size);


/*{
** Name:	HSH_New	- Allocate and Initialize a new Hash Blk and Index
**
** Description:
**	Allocate a new Hash Index
**	Note the if the max_blocks is 0, then the block allocator is not acive
**
** Inputs:
**   num_slots 	prime number > 2 * expected number of entrys
**   key_offset	offset to the key in the entry
**   entry_size	size of a entry (avg. for VL entrys)
**   min_blocks	minumum # of blocks to allocate 
**   max_blocks	maximum # of blocks to allocate 
**   sem_name	name given to the index semaphore
**   flag	Shared / Variable  (see BA.H)
**
** Outputs:
**    HSH_Hdr	-> the hash header  
**
** Returns:
**
** History:
**	25-Jun-1998 (shero03)
**	    initial creation  
*/
HSHDEF *HSH_New(		/* Allocate a Hash blk & index; init the blk */
	i4     num_slots,   	/* number of slots in the index */
	i4     key_offset,	/* offset from the start of the entry to the key */
	i4	entry_size,	/* size of a entry (avg. for VL entrys) */
	i4	min_blocks,	/* minumum # of blocks to allocate   */
	i4	max_blocks,	/* maximum # of blocks to allocate   */
	char    *sem_name, 	/* name given to the Hash Index Semaphore */
	i4	flag		/* Flags Shared / Variable / 	     */
)
{
	HSHDEF		*hsh_blk = NULL;
   	CL_ERR_DESC	lerr;
    	STATUS   	status;
    	LNKDEF		**lindex;
    	PTR		area;
	SIZE_TYPE	allocated;
    	i4	   	size;
	i4		lflag;

    /* allocate the hash index */
    size = ROUND_UP(
    	 	sizeof(PTR)*(num_slots) + sizeof(HSHDEF) + 4, ME_MPAGESIZE);
    size = size / ME_MPAGESIZE;

    status = MEget_pages(ME_MZERO_MASK,
    		         size,
    		         NULL,
    		         &area, &allocated, &lerr);
    if (status == OK)
    {	
        hsh_blk = (HSHDEF *)area;
        hsh_blk->hshIndex = lindex = (LNKDEF**)(area + sizeof(HSHDEF));
    	/*
    	** Set last entry to the special value
    	*/
    	lindex += num_slots;
    	*lindex = (LNKDEF*)HSH_LAST_ENTRY;

    	hsh_blk->hshEntryNum = 0;
        hsh_blk->hshSlotNum = num_slots;
	hsh_blk->hshKeyOffset = key_offset;
	hsh_blk->hshIndexSize = size;
	hsh_blk->hshFlag = flag;

	/* Initialize the block allocator */
	if (max_blocks > 0)
	{	
	    lflag = 0;
	    if (HSH_SHARED & flag)
	        lflag |= BLA_SHARED;
	    if (HSH_VARIABLE & flag)
	        lflag |= BLA_VARIABLE;
    	    BA_New(&hsh_blk->hshBlaHdr, entry_size,
	    	 min_blocks, max_blocks, lflag);
	    hsh_blk->hshFlag |=	HSH_USEBLA;
	}
    	
	/* initialize the semaphore for the index	*/
	if (HSH_SHARED & flag)
    	    MUw_semaphore( &hsh_blk->hshSemaphore, sem_name);
	}

    return hsh_blk;
}   	/* proc  HSH_New */

/*{
** Name:	HSH_Dispose	- Dispose of a Hash Index
**
** Description:
**	Dispose of a Hash Index created from HSH_New
**
** Inputs:
**    hsh_blk  The block header
**
** Outputs:
**
** Returns:
**
** History:
**	25-Jun-1998 (shero03)
**	    initial creation  
**  16-Aug-2005 (fanra01)
**      Correct memory disposal the hash block.
*/

void HSH_Dispose(
	HSHDEF	*hsh_blk	 	/* Hash Index header	     */
)
{
    CL_ERR_DESC		lerr;

    if (hsh_blk == NULL)
        return;
    
    /* Get a Lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUr_semaphore( &hsh_blk->hshSemaphore );
 
    /* Free all allocated entries */
    if (HSH_USEBLA & hsh_blk->hshFlag)
        BA_Dispose(&hsh_blk->hshBlaHdr);
    
    /* Free the index */
    /*
    ** Free the allocated memory, hsh_blk->hsh_index is some offset from
    ** the start of the block.
    */
    MEfree_pages((PTR)hsh_blk, hsh_blk->hshIndexSize, &lerr);

    return;
}  	/* proc HSH_Dispose */

/*
**  Name: HSH_Link - Insert an existing entry into a hash index
**
**  Description:
**      Hash to the correct index list
**	link to the head of the list
**
**  Inputs:
**	hsh_blk		- the address of the hash block
**	key		- addr(key)
**	keylen		- length(key)
**	hsh_entry	- the address of the entry to link in
**
**  Outputs:
**
**  Returns:
**
**  History:
**	25-Jun-1998 (shero03)
**	    initial creation  
*/

void HSH_Link(				/* Insert the entry into the index */
	HSHDEF	*hsh_blk,		/* Hash Index header  */
	char	*hsh_key,		/* Hash Key	*/
	i4	hsh_keylen,		/* Hash Key Length */
	LNKDEF	*hsh_entry		/* Entry to link into the index */
)
{
    LNKDEF	**lindex;
    u_i4	h;

    LL_Init(hsh_entry, hsh_entry);    /*  Initialize the list     */
        
    /* Compute the hash value */
    if (HSH_STRKEY & hsh_blk->hshFlag)
        h = HashStr((unsigned char *)hsh_key) % hsh_blk->hshSlotNum;
    else
        h = HashBin((unsigned char *)hsh_key, hsh_keylen) % hsh_blk->hshSlotNum; 
    lindex = hsh_blk->hshIndex + h;

    if (HSH_SHARED & hsh_blk->hshFlag)
        MUp_semaphore( &hsh_blk->hshSemaphore ); /* get a lock */
	    	
    if (*lindex == NULL)
        *lindex = hsh_entry;        /* put this as 1st entry  */
    else
        LL_LinkAfter(*lindex, hsh_entry);/* insert new item 1st*/
    
    hsh_blk->hshEntryNum++;	/* increment the entry count */

    /* Release the lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUv_semaphore( &hsh_blk->hshSemaphore ); 

    return;
}		/* proc - HSH_Link */

/*
**  Name: HSH_UnLink - Remove a entry from a hash index
**
**  Description:
**      Hash to the correct index list
**	Find the entry in the list
**	Unlink the entry
**
**  Inputs:
**	hsh_blk		- the address of the hash block
**	key		- addr(key)
**	keylen		- length(key)
**	hsh_entry	- the address of the entry to link in
**
**  Outputs:
**
**  Returns:
**
**  History:
**	25-Jun-1998 (shero03)
**	    initial creation  
*/                      
void HSH_Unlink(			/* Remove the entry from the index */
	HSHDEF	*hsh_blk,		/* Hash Index header	     */
	char	*hsh_key,		/* Hash Key	*/
	i4    	hsh_keylen,		/* Hash Key Length */
	LNKDEF	*hsh_entry		/* Entry to unlink */
)
{
    LNKDEF 	*lentry;
    LNKDEF	**lindex;
    u_i4  	h;

    /* Compute the hash value */
    if (HSH_STRKEY & hsh_blk->hshFlag)
    	h = HashStr((unsigned char *)hsh_key) % hsh_blk->hshSlotNum;
    else
    	h = HashBin((unsigned char *)hsh_key, hsh_keylen) % hsh_blk->hshSlotNum; 
    lindex = hsh_blk->hshIndex + h;

    /* Get a Lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUp_semaphore( &hsh_blk->hshSemaphore ); 

    /* find the item in the chain*/
    lentry = (LNKDEF *)LL_Find(*lindex, hsh_entry);
    if (lentry == hsh_entry)
    {
        if (LL_Last(lentry))   		/* if last entry, then       */
           *lindex = NULL;              /* clear the hash index      */
	else
	   *lindex = LL_GetNext(hsh_entry);	/* reset 1st entry */
        
	LL_Unlink(hsh_entry);	       	/* unlink the entry 	     */
        hsh_blk->hshEntryNum--;		/* decrement the entry count */
    }

    /* Release the lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUv_semaphore( &hsh_blk->hshSemaphore ); 

    return;
}		/* proc - HSH_Unlink */

/*
**  Name: HSH_Next - Return the next entry from a hash index
**
**  Description:
**	It is assumed that the caller has a shared lock on the hash index
**      If the entry is NULL, start at the first list
**      Else start at the next entry
**      If there are no more entries in this list advance to the next list
**      If there are no more lists, return NULL
**      Else return this entry and the next entry.
**
**  Inputs:
**	hsh_blk	- the address of the hash block
**	list 	- the address of the current index slot
**	     	  initially the address of the hash index
**	entry	- the address of the current entry
**                initially NULL
**  nentry      - the address of the next entry
**                used to start the next scan
**
**  Outputs:
**	list	- the address of the current index slot
**	entry	- the address of the current entry
**           	NULL when there are no more entries
**
**  Returns:
**
**  History:
**	25-Jun-1998 (shero03)
**	    initial creation  
*/

void HSH_Next(			/* Locate the next indexed entry */
	HSHDEF	*hsh_blk,	/* Hash Index header  */
	LNKDEF	***list, 	/* Current index entry	*/
	LNKDEF	**entry, 	/* Current Hash entry	*/
	LNKDEF	**nentry 	/* Next Hash Entry	*/
)
{
    LNKDEF	*lentry;
    LNKDEF	**llist;

    if (*entry == NULL)         /* Is this the first time in ?	*/
    {
        llist = hsh_blk->hshIndex; /* point to the first list   */
        lentry = NULL;          /* set to be empty              */
    }
    else
    {	                            
        llist = *list;		/* point to the the current list */
        lentry = *nentry;       /* step to the next entry       */
    }

    while (TRUE)
    {
        if (lentry == NULL)	/* At the end of the list?	*/
        {
            llist++;            /* point to the next list	*/
            lentry = *llist;    /* point to the 1st item	*/
            if (lentry == (LNKDEF*)HSH_LAST_ENTRY)
            {
                *entry = NULL;  /* return a NULL                */
                break;          /* leave the next loop          */
            }
        }
        else
        {
            *list = llist;       /* return the current list      */
            *entry = lentry;     /* return the current entry     */
	    if (LL_Last(lentry)) /* if this is the last entry	*/
	    	*nentry = NULL;	 /* set next to be null	 	*/
	    else
	    	*nentry = LL_GetNext(lentry);/* step to the next entry  */
            break;               /* leave the next loop	 	*/
        }
    }

    return;
}		/* proc - HSH_Next */

/*
**  Name: HSH_Insert      - Allocate a new Key and chain it into the Hash index
**
**  Description:
**      Allocate a piece of storage to hold the new entry
**	Get the hash index for this Key
**	Insert the entry at the head of the proper chain
**
**  Inputs:
**	hsh_blk		- the address of the hash block
**	key		- addr(key)
**	keylen		- length(key)
**	hsh_entrylen	- the length of the entry to create
**
**  Outputs:
**
**  Returns:
**      The addr(Key entry)
**
**  History:
**	25-Jun-1998 (shero03)
**	    initial creation  
*/

LNKDEF *HSH_Insert(	       	/* Allocate a entry & add it to the index */
	HSHDEF	*hsh_blk,		/* Hash Index header	     */
	char	*hsh_key,		/* Hash Key	*/
	i4	hsh_keylen,		/* Hash Key Length */
	i4	hsh_entrylen		/* Hash Entry Length */
)
{
    u_i4	h;
    LNKDEF	*lentry;
    LNKDEF	**lindex;
    char	*pKey;

    lentry = (LNKDEF*)BA_UnitAlloc(&hsh_blk->hshBlaHdr, hsh_entrylen);  
    if (!lentry)
	return(NULL);

    LL_Init(lentry, lentry);	/* initialize the list pointers	     */
    pKey = (char*)lentry + hsh_blk->hshKeyOffset;
    MEcopy((PTR)hsh_key, hsh_keylen, (PTR)pKey); /* copy the key */

    /* compute hash index*/
    if (HSH_STRKEY & hsh_blk->hshFlag)
    	h = HashStr((unsigned char *)hsh_key) % hsh_blk->hshSlotNum;
    else
    	h = HashBin((unsigned char *)hsh_key, hsh_keylen) % hsh_blk->hshSlotNum; 
    lindex = hsh_blk->hshIndex + h;

    if (HSH_SHARED & hsh_blk->hshFlag)
        MUp_semaphore( &hsh_blk->hshSemaphore ); /* get a lock */

    if (*lindex == NULL)
        *lindex = lentry;        /* put this as first entry   */
    else
        LL_LinkAfter(*lindex, lentry);/* insert the new item  */
    
    hsh_blk->hshEntryNum++;	/* increment the entry count */

    /* Release the lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUv_semaphore( &hsh_blk->hshSemaphore ); 

    return (lentry);	    	/* return the addr(new entry)    */
}  /* proc - HSH_Insert */

/*
**  Name: HSH_Delete      - Remove an Entry from the Hash index and free it
**
**  Description:
**      Locate the entry in the Hash index
**	Unchain the entry and release its storage
**
**  Inputs:
**	hsh_blk		- the address of the hash block
**	key		- addr(key)
**	keylen		- length(key)
**	hsh_entry	- the entry to delete
**
**  Outputs:
**
**  Returns:
**
**  History:
**	29-Jun-1998 (shero03)
**	    initial creation  
*/

void HSH_Delete(		/* Unchain an entry from the index & free it */
	HSHDEF	*hsh_blk,	 	/* Hash Index header */
	char	*hsh_key,		/* Hash Key	*/
	i4	hsh_keylen,		/* Hash Key Length */
	LNKDEF	*hsh_entry		/* Entry to remove */
)
{
    u_i4	h;
    LNKDEF	*lentry;
    LNKDEF	**lindex;

    /* compute hash index*/
    if (HSH_STRKEY & hsh_blk->hshFlag)
    	h = HashStr((unsigned char *)hsh_key) % hsh_blk->hshSlotNum; 
    else
    	h = HashBin((unsigned char *)hsh_key, hsh_keylen) % hsh_blk->hshSlotNum; 
    lindex = hsh_blk->hshIndex + h;
    
    /* Get a Lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUp_semaphore( &hsh_blk->hshSemaphore ); 
    
    /* Find the entry via the key */
    if (HSH_STRKEY & hsh_blk->hshFlag)        
        lentry = (LNKDEF*)LL_FindStr(*lindex, hsh_blk->hshKeyOffset, hsh_key);
    else
        lentry = (LNKDEF*)LL_FindBin(*lindex, hsh_blk->hshKeyOffset,
    			   	 hsh_keylen, hsh_key);

    if (lentry)		    /* Found the entry; unhook it from the chain */
    {
        if (LL_Last(lentry))  	       	/* if last entry, then       */
           *lindex = NULL;              /* clear the hash index      */
        else
	   *lindex = LL_GetNext(lentry); /* reset 1st entry */
        
	LL_Unlink(lentry);		/* unlink the entry 	     */
        hsh_blk->hshEntryNum--;	/* decrement the entry count */
    }

    /* Release the lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUv_semaphore( &hsh_blk->hshSemaphore );
    
    /* Free the entry */
    BA_UnitDealloc(&hsh_blk->hshBlaHdr, (char*)lentry);  

    return;
}  /* proc - HSH_Delete */

/*
**  Name: HSH_Select - Locate an entry via the Hash index and Unlink it
**
**  Description:
**	Get the hash index for this connection key
**	Search the chain for the matching entry
**	If found remove it from the chain
**	Return the entry if found, null otherwise
**
**  Inputs:
**	hsh_blk		- the address of the hash block
**	key		- addr(key)
**	keylen		- length(key)
**
**  Outputs:
**
**  Returns:
**      The addr(entry)
**
**  History:
**	25-Jun-1998 (shero03)
**	    initial creation  
*/

LNKDEF *HSH_Select(			/* Find and unlink a entry */
	HSHDEF	*hsh_blk,		/* Hash Index header  */
	char	*hsh_key,		/* Hash Key	*/
	i4    	hsh_keylen		/* Hash Key Length */
)
{
    u_i4		h;
    LNKDEF		**lindex;
    LNKDEF		*lentry = NULL;

    /* compute hash index*/
    if (HSH_STRKEY & hsh_blk->hshFlag)
    	h = HashStr((unsigned char *)hsh_key) % hsh_blk->hshSlotNum;
    else
    	h = HashBin((unsigned char *)hsh_key, hsh_keylen) % hsh_blk->hshSlotNum; 
    lindex = hsh_blk->hshIndex + h;

    /* Get a Lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUp_semaphore( &hsh_blk->hshSemaphore ); 
    
    /* Find the entry via the key */
    if (HSH_STRKEY & hsh_blk->hshFlag)           
        lentry = (LNKDEF*)LL_FindStr(*lindex, hsh_blk->hshKeyOffset, hsh_key);
    else
        lentry = (LNKDEF *)LL_FindBin(*lindex, hsh_blk->hshKeyOffset,
    			   	 hsh_keylen, hsh_key);

    if (lentry)		    /* Found the entry; unhook it from the chain */
    {
        if (LL_Last(lentry))  	       	/* if last entry, then       */
           *lindex = NULL;              /* clear the hash index      */
        else
	   *lindex = LL_GetNext(lentry); /* reset 1st entry */
        
	LL_Unlink(lentry);		/* unlink the entry 	     */
        hsh_blk->hshEntryNum--;		/* decrement the entry count */
    }

    /* Release the lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUv_semaphore( &hsh_blk->hshSemaphore );

    return (lentry);
}   	/* proc - HSH_Select */


/*
**  Name: HSH_Find - Locate a Key name via the Hash index
**
**  Description:
**	Get the hash index for this Key
**	Search the chain for the matching name
**	Return the entry if found, null otherwise
**
**  Inputs:
**	hsh_blk		- the address of the hash block
**	key	 	- addr(key)
**	keylen		- length(key)
**
**  Outputs:
**
**  Returns:
**      The addr(entry)
**
**  History:
**	25-Jun-1998 (shero03)
**	    initial creation  
*/

LNKDEF *HSH_Find(	 		/* Locate an indexed entry */
	HSHDEF	*hsh_blk,		/* Hash Index header  */
	char	*hsh_key,		/* Hash Key	*/
	i4	hsh_keylen		/* Hash Key Length */
)
{
    u_i4		h;
    LNKDEF		*lentry = NULL;
    LNKDEF		**lindex;

    /* compute hash index*/
    if (HSH_STRKEY & hsh_blk->hshFlag)
    	h = HashStr((unsigned char *)hsh_key) % hsh_blk->hshSlotNum; 
    else
    	h = HashBin((unsigned char *)hsh_key, hsh_keylen) % hsh_blk->hshSlotNum; 
    lindex = hsh_blk->hshIndex + h;

    /* Get a Lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUp_semaphore( &hsh_blk->hshSemaphore );
    
    /* Find the entry via the key */
    if (HSH_STRKEY & hsh_blk->hshFlag)           
        lentry = (LNKDEF*)LL_FindStr(*lindex, hsh_blk->hshKeyOffset, hsh_key);
    else
        lentry = (LNKDEF *)LL_FindBin(*lindex, hsh_blk->hshKeyOffset,
    			   	 hsh_keylen, hsh_key);

    /* Release the lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUv_semaphore( &hsh_blk->hshSemaphore );

    return (lentry);
}		/* proc - HSH_Find */

/*
**  Name: HSH_GetLock      - Obtain the Hash Index Lock
**
**  Description:
**      Get a lock with the hash index semaphore
**
**  Inputs:
**	hsh_blk		- the address of the hash block
**
**  Outputs:
**
**  Returns:
**
**  History:
**	29-Jun-1998 (shero03)
**	    initial creation  
*/

void HSH_GetLock(		/* Obtain the Hash index lock */
	HSHDEF	*hsh_blk	/* Hash Index header */
)
{
    /* Get a Lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUp_semaphore( &hsh_blk->hshSemaphore ); 

    return;
}  /* proc - HSH_GetLock */

/*
**  Name: HSH_ReleaseLock      - Release the Hash Index Lock
**
**  Description:
**      Get a lock with the hash index semaphore
**
**  Inputs:
**	hsh_blk		- the address of the hash block
**
**  Outputs:
**
**  Returns:
**
**  History:
**	29-Jun-1998 (shero03)
**	    initial creation  
*/

void HSH_ReleaseLock(		/* Release the Hash index lock */
	HSHDEF	*hsh_blk	/* Hash Index header */
)
{
    /* Release the lock */
    if (HSH_SHARED & hsh_blk->hshFlag)
        MUv_semaphore( &hsh_blk->hshSemaphore );

    return;
}  /* proc - HSH_ReleaseLock */

/*
**  Name: HSH_AreEntries      - Are there any Entries?
**
**  Description:
**      Return TRUE if there are entries in the index
**
**  Inputs:
**	hsh_blk		- the address of the hash block
**
**  Outputs:
**
**  Returns:
**      Return TRUE if there are entries in the index
**
**  History:
**	29-Jun-1998 (shero03)
**	    initial creation  
*/

bool HSH_AreEntries(		/* Are there any entries? */
	HSHDEF	*hsh_blk	/* Hash Index header */
)
{
    if (hsh_blk != NULL && hsh_blk->hshEntryNum > 0)
    	return TRUE;

    return FALSE;
}  /* proc - HSH_AreEntries */

/*
**  Name: HSH_AllocEntry - Allocate a new hash entry
**
**  Description:
**      Allocate a piece of storage to hold the new entry
**
**  Inputs:
**	hsh_blk		- the address of the hash block
**	key		- addr(key)  
**			if NULL, the key information is ignored
**	keylen		- length(key)
**	hsh_entrylen	- the length of the entry to create
**
**  Outputs:
**
**  Returns:
**      The addr(Hash entry)
**
**  History:
**	29-Jun-1998 (shero03)
**	    initial creation  
*/

LNKDEF *HSH_AllocEntry(    		/* Allocate an entry */
	HSHDEF	*hsh_blk,		/* Hash Index header  */
	char	*hsh_key,		/* Hash Key	*/
	i4	hsh_keylen,		/* Hash Key Length */
	i4	hsh_entrylen		/* Hash Entry Length */
)
{
    LNKDEF	*lentry;
    char	*pKey;

    lentry = (LNKDEF*)BA_UnitAlloc(&hsh_blk->hshBlaHdr, hsh_entrylen);  

    LL_Init(lentry, lentry);    /*  Initialize the list     */
    if (hsh_key)
    {	
    	pKey = (char*)lentry + hsh_blk->hshKeyOffset;
    	MEcopy((PTR)hsh_key, hsh_keylen, (PTR)pKey); /* copy the key */
    }

    return (lentry);	    	/* return the addr(new entry)    */
}  /* proc - HSH_AllocEntry */

/*
**  Name: HSH_DeallocEntry - Deallocate a hash entry
**
**  Description:
**      Release the piece of storage hold the entry
**
**  Inputs:
**	hsh_blk		- the address of the hash block
**	hsh_entry	- the entry to free
**
**  Outputs:
**
**  Returns:
**
**  History:
**	29-Jun-1998 (shero03)
**	    initial creation  
*/

void HSH_DeallocEntry(    		/* Deallocate an entry */
	HSHDEF	*hsh_blk,		/* Hash Index header  */
	LNKDEF	*hsh_entry		/* Entry to free */
)
{
    BA_UnitDealloc(&hsh_blk->hshBlaHdr, (char*)hsh_entry);  
    return;
}  /* proc - HSH_DeallocEntry */

/*
**  Name: HashStr      - Hash a string into a number
**
**  Description:
**      This is the Elf hash algorithm suggested in DDJ as a good general
**	hash routine.
**
**  Inputs:
**      name		- string to convert
**
**  Outputs:
**
**  Returns:
**      Hash index
**
**  History:
**	25-Jun-1998 (shero03)
**	    initial creation  
*/
static u_i4 HashStr( const unsigned char *name)
{
    u_i4    h = 0, g;

    while (*name)
    {
    	h = (h << 4) + *name++;
	if (g = h & 0xF0000000)
	    h ^= g >> 24;
	h &= ~g;
    }
    return h;
}		/* proc - HashStr */

/*
**  Name: HashBin      - Hash a binary field into a number
**
**  Description:
**      This is the Perl 5 hash algorithm suggested by Tim's Free Algorithm
**	www.cerfnet.com/~timb/alg_hash.htm
**
**  Inputs:
**      field		- binary field to convert
**
**  Outputs:
**
**  Returns:
**      Hash index
**
**  History:
**	25-Jun-1998 (shero03)
**	    initial creation  
*/
static u_i4 HashBin( const unsigned char *field, const i4  size)
{
    u_i4    h = ~0U;
    i4	     i;

    for (i = 0; i < size; i++)
    	h = h * 33 + *field++;

    return h;
}		/* proc - HashBin */
