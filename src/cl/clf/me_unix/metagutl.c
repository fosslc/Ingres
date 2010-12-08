/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include  <cs.h>
# include	<meprivate.h>
# include	<me.h>
# include	<qu.h>

extern ME_HEAD      MElist;

/**
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
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**      08-feb-1993 (pearl)
**          Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**      30-May-96 (stial01)
**          New advice ME_TUXEDO_ALLOC should behave like ME_INGRES_ALLOC
**      03-jun-1996 (canor01)
**          Add synchronization and make MEstatus local to make ME
**          stream allocator thread-safe when used with operating-system
**          threads.
**	12-feb-1997 (canor01)
**	    Initialize local MEstatus in IIME_ftFreeTag().
**	29-may-1997 (canor01)
**	    Clean up some compiler warnings.
**      22-apr-1999 (fanra01)
**          Made free list static as there is a name conflict when loaded from
**          Netscape enterprise server.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**      15-nov-2010 (stephenb)
**          wrap IIME_purify_tag_exit in ifdef xPURITY.
**/

/* # define's */
/* GLOBALDEF's */

/* extern's */

# ifdef OS_THREADS_USED
GLOBALREF CS_SYNCH      MEtaglist_mutex;
GLOBALREF CS_SYNCH      MEfreelist_mutex;
# endif /* OS_THREADS_USED */

/*}
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
** History:
**	5-dec-1989 (Joe)
**	    First Written
*/
typedef struct _metagnode
{
    ME_HEAD		met_list;
    i2			met_tag;
    struct _metagnode	*met_hash;
} METAGNODE;

/*
** htab is the hash table.  Each bucket points to a linked list of
** METAGNODEs for the tags that hash to this bucket.  The hash function
** is simply the value of the tag modulo 256.  Since tags are normally
** allocated sequentially, this should give as good a distribution as
** anything.
*/
GLOBALDEF METAGNODE *htab[256] ZERO_FILL;

/* 
** freelist is a list of free METAGNODEs linked by their met_hash fields.
** When a new METAGNODE is needed, it is taken from this list.  If the
** list is empty, then a number of METAGNODEs is allocated from dynamic
** memory and put on this list.
*/
static METAGNODE *freelist;

/*{
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
** History:
**	5-dec-1989 (Joe)
**	    First Written
*/
VOID
IIME_atAddTag(
	i4	tag,
	ME_NODE	*node)
{
    register METAGNODE	**first;

# ifdef OS_THREADS_USED
    CS_synch_lock( &MEtaglist_mutex );
# endif /* OS_THREADS_USED */

    /*
    ** Note that first is a pointer to a pointer.
    ** The loop will cause a return from the routine if the tag already
    ** has an METAGNODE in the hash table.
    ** If the loop finishes, then first will point to the pointer
    ** that must contain the METAGNODE.
    */
    for (first = &(htab[tag%256]);
	 *first != NULL;
	 first = &((*first)->met_hash))
    {
	if ((*first)->met_tag == tag)
	{
	    (void)QUinsert((QUEUE *) node, (QUEUE *) (*first)->met_list.MElast);
# ifdef OS_THREADS_USED
	    CS_synch_unlock( &MEtaglist_mutex );
# endif /* OS_THREADS_USED */
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
    QUinit((QUEUE *)&((*first)->met_list));
    (void)QUinsert((QUEUE *) node, (QUEUE *) (*first)->met_list.MElast);
# ifdef OS_THREADS_USED
    CS_synch_unlock( &MEtaglist_mutex );
# endif /* OS_THREADS_USED */
    return;
}

GLOBALREF SIZE_TYPE i_meactual;
GLOBALREF SIZE_TYPE i_meuser;

/*{
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
** History:
**	5-dec-1989 (Joe)
**	    First Written
**      30-May-96 (stial01)
**          New advice ME_TUXEDO_ALLOC should behave like ME_INGRES_ALLOC
**	12-feb-1997 (canor01)
**	    Initialize local MEstatus.
**      27-Jan-1999 (fanra01)
**          Add thread alloc case for tag free.  Otherwise our memory is
**          returned to the system heap causing wonderfully esoteric execution.
*/
STATUS
IIME_ftFreeTag(
	i4	tag )
{
    register METAGNODE	**first;
    STATUS MEstatus = OK;

# ifdef OS_THREADS_USED
    CS_synch_lock( &MEtaglist_mutex );
# endif /* OS_THREADS_USED */
    for (first = &(htab[tag%256]);
	 *first != NULL;
	 first = &((*first)->met_hash))
    {
	if ((*first)->met_tag == tag)
	{
	    register ME_NODE	*this;
	    register ME_NODE	*next;
	    register METAGNODE	*freenode;

	    for (this = (*first)->met_list.MEfirst;
		 this != NULL && this != (ME_NODE *) &((*first)->met_list);)
	    {
		next = this->MEnext;
		if ( MEstatus == OK )
		{
		    i_meactual -= this->MEsize;
		    i_meuser -= this->MEaskedfor;
		    (void)QUremove( (QUEUE *) this );
		    if( (MEadvice == ME_INGRES_ALLOC )
			|| (MEadvice == ME_INGRES_THREAD_ALLOC)
			|| (MEadvice == ME_TUXEDO_ALLOC) )
		    {
# ifdef OS_THREADS_USED
			CS_synch_lock( &MEfreelist_mutex );
# endif /* OS_THREADS_USED */
			MEstatus = MEfadd(this, TRUE);
# ifdef OS_THREADS_USED
			CS_synch_unlock( &MEfreelist_mutex );
# endif /* OS_THREADS_USED */
		    }
		    else
			free( (char *)this );
		}
		if (MEstatus == OK)
		    this = next;
		else
		    break;
	    }
	    freenode =  *first;
	    *first = freenode->met_hash;
	    freenode->met_hash = freelist;
	    freelist = freenode;
# ifdef OS_THREADS_USED
	    CS_synch_unlock( &MEtaglist_mutex );
# endif /* OS_THREADS_USED */
	    return MEstatus;
	}
    }
# ifdef OS_THREADS_USED
    CS_synch_unlock( &MEtaglist_mutex );
# endif /* OS_THREADS_USED */
    return ME_NO_TFREE;
}

/*
** purify override code.
**
** This stuff is only invoked when linked with purify - IIME_purify_tag_exit
** is called explicitly by purify_override_PCexit, which purify recognizes
** as an override for PCexit.  The reason we do this is to explicitly NULL
** out all of the alloc'ed list pointers on exit, so that purify can
** detect leaks properly (purify does leak detection by scanning the
** programs data space at exit, looking for anything which looks like a
** pointer - if we don't NULL the chain, it thinks there's still pointers
** to everything, so it doesn't find any leaks.)
*/

static int
kill_q(lp)
ME_HEAD *lp;
{
        int count;
        ME_NODE *this;
        ME_NODE *next;

        /*
        ** we clobber the queue head pointers as well.
        ** at this stage we don't care.  We do return 1 less
        ** than count, to account for this.
        */
        count = 0;
        for (this = lp->MEfirst; this != NULL; this = next)
        {
                next = this->MEnext;
                this->MEnext = this->MEprev = NULL;
                ++count;
        }

        return count > 0 ? count - 1 : 0;
}
#ifdef xPURIFY
/*{
** Name:        IIME_purify_tag_exit()
**
** Description:
**      NULL ME pointers to allocated memory at exit, when running with purify.
**      As an added diagnostic, also prints how many blocks were allocated
**      on each tag (also lets you know that the purify override really
**      happened).
**
** Inputs:
**      none
**
** Side Effects:
**      Destroys ME allocation queue.  Used only at PCexit, and only under
**      purify.
**
** History:
**      8/93 (bobm)     First Written
*/

VOID
IIME_purify_tag_exit(void)
{
        int n;
        int i;
        METAGNODE *tagp;

        if ((n = kill_q(&MElist)) > 0)
                printf("%d allocated blocks still on ME untagged list at exit.\n",n);

        for (i=0; i <256; ++i)
        {
                for (tagp = htab[i]; tagp != NULL; tagp = tagp->met_hash)
                {
                        if ((n = kill_q(&(tagp->met_list))) > 0)
                                printf(
"%d allocated blocks still on ME tag %d at exit.\n",n,tagp->met_tag);
                }
        }
}
#endif /*xPURIFY */
