/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<mu.h>


/**
** Name:	MEtag.c - tag allocation and deallocation routines
**
** Description:
**	This file defines:
**
**	MEgettag	Get a tag
**	MEfreetag	Free a tag 	
**
** History:
**	7/93 (Mike S) Initial version
**      28-jul-1993 (huffman)
**         Alpha does not forgive prototyping.  And all others will make
**         use of this prototyping.
**	8/93 (Mike S) 
**	   Remove 'dofree' parameter from MEfreetag.  It's up to the caller 
**	   to free any allocated memory.
**	   Use semaphores to make these re-entrant.
**      12-jun-1995 (canor01)
**          semaphore protect memory allocation (MCT)
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**
**/

/* # define's */
# define MINTAGID	100		/* Minimum tag MEgettag will return */
# define MAXTAGID	MAXI2		/* Maximium tag MEgettag will return */

# define NODECLUSTER	50		/* Nodes to allocate at once */

# define METAG_SEMAPHORE_NAME	"METAG"	/* Semaphore name */

/* typedefs */
typedef struct metagnode
{
    u_i2 tag;			/* this node's tag */
    struct metagnode *next;	/* next node in list */
} METAGNODE;

/* forward references */
/* extern's */
/* static's */
static METAGNODE *freetaglist = NULL;
static METAGNODE *freenodelist = NULL;

static u_i2 nexttag = MINTAGID;

static MU_SEMAPHORE MEtag_sem ZERO_FILL;
static bool tag_sem_init = FALSE;

static VOID	init_tag_sem();

/*{
** Name:	MEgettag	- Get an ME tag	
**
** Description:
**	Get a currently unused ME tag for memory allocation.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**		u_i2	ME tag.	
**			>0 	Success
**			0 	Failure (we're out of tags or a semaphore error)
**
** Re-entrancy:
**	Re-entrant
**
** Exceptions:
**	none
**
** Side Effects:
**	None.
**
** History:
**	7/93 (Mike S) initial version
*/
u_i2
MEgettag(void)
{
    u_i2 tag;
    STATUS stat;

    /* Insure our semaphore has been iniialized */
    if (!tag_sem_init)
	init_tag_sem();

    /*
    ** Now lock our data strucutres.
    */
    if ((stat = MUp_semaphore(&MEtag_sem)) != OK)
    {
	/* Some error getting semaphore. */
	return 0;
    }

    if (freetaglist == NULL)
    {
	/* None free; return the next available one */
	if (nexttag < MAXTAGID)
	    tag = nexttag++;
	else
	    tag = 0;
    }
    else
    {
	METAGNODE *node = freetaglist;

	/* 
	** Move this node from the list of free tags to the list of 
	** unused nodes.
	*/
	freetaglist = node->next;
	node->next = freenodelist;
	freenodelist = node;
	tag = node->tag;

    }

    (VOID)MUv_semaphore(&MEtag_sem);	/* Unlock things */
    return tag;
}



/*{
** Name:	MEfreetag - Release an ME tag
**
** Description:
**	Release a tag back to ME for re-use.  The memory associated with this
**	tag must be freed; the second argument reflects whether the caller
**	has already done this.
**
** Inputs:
**	tag	u_i2	tag to free
**
** Returns:
**	STATUS
**		OK	Success
**		other	We freed the tag, but can't put it onto the free list.
**			Or, a semaphore error.
**			Or, an invalid tag ID.
**
** Re-entrancy:
**	Re-entrant
**
** Exceptions:
**	none
**
** Side Effects:
**	May allocate memory.
**
** History:
**	7/93 (Mike S) Initial version
**	8/93 (Mike S) 
**		Remove check that the tag isn't already free. It can be 
**		expensive, and isn't likely to be useful.
*/
#ifdef CL_PROTOTYPED

STATUS
MEfreetag(u_i2 tag)

#else  /* CL_PROTOTYPED */

STATUS
MEfreetag(tag)
          u_i2 tag;

#endif  /* CL_PROTOTYPED */

{
    METAGNODE 	*node;
    STATUS 	rval;

    /* Insure our semaphore has been iniialized */
    if (!tag_sem_init)
	init_tag_sem();

    if (tag < MINTAGID || tag > MAXTAGID)
	return ME_BD_TAG;	/* Invalid tag */

    /* First, lock the free lists */
    if ((rval = MUp_semaphore(&MEtag_sem)) != OK)
    {
	/* Some error getting semaphore. */
	return rval;
    }

    /* Get (or make) a node to represent the free tag */
    if (freenodelist == NULL)
    {
	METAGNODE *nodelist;

	/* Get some nodes and put them onto the freenode list */
	nodelist = (METAGNODE *)MEreqmem(0, sizeof(METAGNODE) * NODECLUSTER, 
			    		 (bool)FALSE, &rval);
	if (nodelist == NULL)
	{
	    /* 
	    ** We're almost out of memory; we're going to lose track of the
	    ** tag, which is bad, but there isn't much we can do about it.
	    */
	}
	else
	{
	    METAGNODE *nodeptr;

	    nodelist->next = NULL;
	    for (nodeptr = nodelist + 1; 
		 nodeptr < nodelist + NODECLUSTER; 
		 nodeptr++)
	    {
		nodeptr->next = nodeptr - 1;
	    }
	    freenodelist = &nodelist[NODECLUSTER-1];
	}
    }

    if (freenodelist != NULL)
    {
	/* Put this tag onto the free tag list */
	node = freenodelist;
	freenodelist = freenodelist->next;
	node->tag = tag;
	node->next = freetaglist;
	freetaglist = node;
    }

    (VOID)MUv_semaphore(&MEtag_sem);
    return rval;
}


/*{
** Name:	init_tag_sem - initialize semaphore 
**
** Description:
**	This initializes the semaphore used by MEfreetag and MEgettag
**	to access the list fo free tags.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
**
** Re-entrancy:
**	The purpose of this routine is to make MEfreetag and MEgettag
**	re-entrant.  In fact, there is a problem if this routine is 
**	interrupted and run from another thread, since the semaphore
**	could be initialized twice, but we're ignoring that for now.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	8/93 (Mike S) Initial version
*/
VOID
init_tag_sem()
{
    (VOID) MUi_semaphore(&MEtag_sem);
    MUn_semaphore(&MEtag_sem, METAG_SEMAPHORE_NAME);
    tag_sem_init = TRUE;
}
