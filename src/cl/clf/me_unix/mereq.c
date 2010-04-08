/*
** Copyright (c) 2004 Ingres Corporation
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<meprivate.h>
# include	<me.h>
# include	<cs.h>

/**
** Name:	mereq.c -	Standard memory allocator entry points.
**
** Description:
**	This file defines:
**
**	MEreqmem	Allocate memory of size <= MAX( u_i4 ).
**
** History:
**	10-jun-87 (bab)
**		Written as replacements for MEalloc, MEcalloc, MEtalloc
**		and MEtcalloc.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**      23-may-90 (blaise)
**          Add "include <compat.h>."
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.  Delete forward and external function references;
**              these are now all in either me.h or meprivate.h.
**      08-feb-1993 (pearl)
**          Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**	18-oct-1993 (kwatts)
**	    Added a wrapper for IIMEreqmem to restore MEreqmem name for ICL
**	    Smart Disk linkage.
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	    Combined MEdoalloc and MEreqmem.  Simplify calls and remove
**	    extra stack call.
**	07-Apr-2000 (jenjo02)
**	    After releasing the free list mutex to expand the free list,
**	    rescan the free list if another thread may have freed or
**	    added memory that we can now use.
**	    Added MEshow() debugging function to TRdisplay the allocated
**	    and free lists.
**	05-nov-2004 (thaju02)
**	    For MEshow, change nodes to SIZE_TYPE.
**      16-jan-2003 (horda03) Bug 111658/INGSRV2680
**          FIxed compiler warnings. SIZE_TYPE is an unsigned
**          quantity.
**	06-May-2005 (hanje04)
**	    In MEshow, rename size_t variable to be sizet as it 
**	    clashes with type size_t on Mac OSX.
[@history_template@]...
**/

/*{
** Name:	MEreqmem	- Allocate memory of specified size.
**
** Description:
**	Returns a pointer to a block of memory of size bytes.
**	A tag must be specified, although that tag may be 0
**	as a default value.  If the 'clear' parameter is TRUE,
**	then the allocated memory is zeroed.  The fourth parameter
**	may be NULL, in which case no detailed status information
**	is returned.  Returns NULL if an error occurs.
**
** Inputs:
**	tag	id associated with the allocated region of memory.
**	size	Amount of memory to allocate.
**	clear	Specify whether to zero out the allocated memory.
**
** Outputs:
**	status	If all went well, set to OK.
**		Otherwise, set to one of ME_NO_ALLOC,
**					 ME_GONE,
**					 ME_00_PTR,
**					 ME_CORRUPTED,
**					 ME_TOO_BIG.
**		(If status == NULL, then no attempt is made to modify it.)
**
**	Returns:
**		Pointer to the allocated region of memory, or NULL on error.
**
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**	None.
**
** History:
**	10-jun-87 (bab)
**		Written as replacement for MEalloc, MEcalloc, MEtalloc
**		and MEtcalloc.
**
**		16-Dec-1986 (daveb)
**		Substantial rework for layering on top of malloc.
**		Remove dead debug code, eliminate code that assumes
**		we are increasing the process size.
**
**		14-Jul-1988 (anton)
**		Modifed to allocate memory from MEget_pages instead of
**		MEadd which was equivilent to sbrk.
**
**		12-jun-1989 (rogerk)
**		Added new argument to MEget_pages for Terminator.
**
**		24-jan-1990 (fls-sequent)
**		Added calls to acquire the ME_page_sem semaphore (if necessary)
**		before calling MEget_pages.
** 		21-may-90 (blaise)
**        	Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**    		23-may-90 (blaise)
**     		Add "include <compat.h>".
**    		31-may-90 (blaise)
**     		Added include <me.h> to pick up redefinition of MEfill().
**		21-Jan-91 (jkb)
**		Change "if( !(MEadvice & ME_INGRES_ALLOC) )" to
**		"if( MEadvice == ME_USER_ALLOC )" so it works with
**		ME_INGRES_THREAD_ALLOC as well as ME_INGRES_ALLOC
**		3/91 (Mike S) 
**		Keep allocator statistics for development debugging.
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.  Delete forward and external ME function references;
**              these are now all in either me.h or meprivate.h.
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
**	03-jun-1996 (canor01)
**	    Add synchronization and make MEstatus local to make ME 
**	    stream allocator thread-safe when used with operating-system 
**	    threads.
**	11-sep-1996 (canor01)
**	    When initially allocating pages of memory, get enough to be
**	    able to add MO objects without another call to MEget_pages().
**	06-may-97 (muhpa01)
**   	    Removed local declaration for malloc() to quiet HP compiler
**	    complaint of "inconsistent type declaration"
**	03-nov-1997 (canor01)
**	    Change MEaskedfor from a u_12 to an i4 to match maximum size
**	    that can be requested.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	19-apr-1999 (hanch04)
**	    removed define for malloc
**	07-Apr-2000 (jenjo02)
**	    After releasing the free list mutex to expand the free list,
**	    rescan the free list if another thread may have freed or
**	    added memory that we can now use.
*/


GLOBALREF SIZE_TYPE        i_meactual;
GLOBALREF SIZE_TYPE        i_meuser;
# ifdef OS_THREADS_USED
GLOBALREF CS_SYNCH      MEfreelist_mutex;
GLOBALREF CS_SYNCH      MElist_mutex;
# endif /* OS_THREADS_USED */


PTR
MEreqmem(
	u_i2	tag,
	SIZE_TYPE size,
	bool	zero,
	STATUS	*status)
{
    PTR	block=NULL;
    register ME_NODE *node;		/* the node to return */
    register ME_NODE *start;		/* for searching free list */
    register ME_NODE *this;		/* block to get node from */
    register ME_NODE *frag;		/* fragment block */
    
    ME_NODE 	*tmp;			/* not register for MEadd */
    SIZE_TYPE	nsize;			/* nsize node to obtain */
    SIZE_TYPE	fsize;			/* size of 'this' fragment block */
    SIZE_TYPE	newstuff;		/* size to add to process, or  0 */
    SIZE_TYPE	prev_actual;		/* rescan free list? */
    SIZE_TYPE	alloc_pages;
    CL_ERR_DESC	err_code;

    STATUS	MEstatus = OK;
    
    i_meuser += size;
    
    if (!size)
	MEstatus = ME_NO_ALLOC;
    
    if( !MEsetup )
        MEinitLists();
    
    /*
    **	Try to do the allocation.
    */
    if( MEstatus == OK )
    {
	nsize = SIZE_ROUND( size );
	/*
	** Get memory with malloc().
	*/
	if( MEadvice == ME_USER_ALLOC )
	{
	    if( (node = (ME_NODE *)malloc( nsize )) == NULL )
	    	MEstatus = ME_GONE;
	}
	/*
	** Get block from private free list.
	*/
	else
	{
# ifdef OS_THREADS_USED
	    CS_synch_lock( &MEfreelist_mutex );
# endif /* OS_THREADS_USED */

	    /*
	    **  Look on free list for 1st block big enough
	    **  to hold request.  This linear search can be slow.
	    */
	    start = (ME_NODE *)&MEfreelist;
	    this = MEfreelist.MEfirst;
	    while ( this != NULL && this != start && this->MEsize < nsize )
		this = this->MEnext;
	    
	    if( this == NULL )
	        MEstatus = ME_CORRUPTED;
	    
	    /*
	    ** At this point, we are in one of three states:
	    ** 1)  Corrupted memory; MEstatus != OK
	    ** 2)  this is good node, this != start
	    ** 3)  No good node; this == start;
	    */
	    if ( MEstatus == OK )
	    {
		/*
		** If nothing on free list is big enough
		** get one or more standard blocks from system,
		** take what is needed and add remainder
		** to free list.
		*/
		if (this != start)
		{
		    /* take right off the free list */
		    newstuff = 0;
		}
		else	/* this == start */
		{
		    /*
		     * Expand the free list by calling getpages
		     * newstuff holds the number of pages needed
		     */
		    newstuff = (nsize + ME_MPAGESIZE-1)/ME_MPAGESIZE;
		    /* if first time allocation, get enough for MO overhead */
		    if ( (prev_actual = i_meactual) == (SIZE_TYPE) 0 )
			newstuff += 4;
# ifdef OS_THREADS_USED
	            CS_synch_unlock( &MEfreelist_mutex );
# endif /* OS_THREADS_USED */
		    MEstatus = MEget_pages(ME_SPARSE_MASK, newstuff, NULL, 
			(PTR *)&tmp, &alloc_pages, &err_code);
# ifdef OS_THREADS_USED
	            CS_synch_lock( &MEfreelist_mutex );
# endif /* OS_THREADS_USED */
		    if (MEstatus == OK)
		    {
			/* now we need to find where to put this new memory
			   on the sorted free list - we search in reverse */
			tmp->MEsize = newstuff * ME_MPAGESIZE;
			this = MEfreelist.MElast;
			while (start != this && this != NULL &&
			       this > tmp)
			    this = this->MEprev;
			if (this != start && NEXT_NODE(this) == tmp)
			{
			    this->MEsize += tmp->MEsize;
			}
			else
			{
			    (void)QUinsert( (QUEUE *) tmp, (QUEUE *)this );
			    this = tmp;
			}
			if (this->MEnext != start &&
			    NEXT_NODE(this) == this->MEnext)
			{
			    this->MEsize += this->MEnext->MEsize;
			    (void)QUremove( (QUEUE *) this->MEnext);
			}
			/*
			** While the free list mutex was released, another
			** thread may have freed up a big enough piece of
			** memory for our needs, or may have extended the
			** free list.
			** If that's the case, research the free list;
			** we'll find either a right-sized node or 
			** the new memory we just added to the free list.
			*/
			if ( prev_actual != i_meactual )
			{
			    this = MEfreelist.MEfirst;
			    while ( this != NULL && this != start && this->MEsize < nsize )
				this = this->MEnext;
		
			    if( this == NULL )
				MEstatus = ME_CORRUPTED;
			}
		    }
		    else
			if (MEstatus == ME_OUT_OF_MEM)
			    MEstatus = ME_GONE;
		}

		/*
		** At this point, we can be in two states.
		** 1)  Corrupted memory, MEstatus != OK
		** 2)  'this' is an OK node from the free list.
		*/
		
		if ( MEstatus == OK )
		{
		    node = this;
		    
		    /*
		    ** if this is correct size or would
		    **   leave useless block in chain
		    **	just move block to allocated list
		    ** else
		    **	grab what is needed from 'this'
		    **	  block and then update 'this'
		    */
		    
		    fsize = node->MEsize - nsize;
		    if ( fsize <= sizeof(ME_NODE) )
		    {
			(void)QUremove( (QUEUE *) node );
			
			/* fudge size in node to eat leftover amount. */
			fsize = 0;
			nsize = node->MEsize;
		    }
		    else	/* make fragment block */
		    {
			/*
			** Make a leftover block after the
			** allocated space in node, in 'this'
			*/
			frag = (ME_NODE *)((char *) node + nsize );
			frag->MEsize = fsize;
			frag->MEtag = 0;
			
			/* remove node, add fragment to free list */
			(void)QUremove( (QUEUE *) node );
			MEstatus = MEfadd( frag, FALSE );
			
		    }  /* fragment left over */
		    /* Increment meactual while mutex held */
		    i_meactual += nsize;
		}  /* Got a node */
	    }  /* free list search OK */
# ifdef OS_THREADS_USED
	    CS_synch_unlock( &MEfreelist_mutex );
# endif /* OS_THREADS_USED */
	}  /* ME_USER_ALLOC */
	
	/*
	** At this point we are in one of two states:
	** 1.  Corrupted, MEstatus != OK.
	** 2.  Have a 'node' to use, from freelist or malloc.
	**     The freelist is consistant, but the allocated list is
	**     not setup for the node. "nsize" is the actual size of "node".
	*/
	
	if( MEstatus == OK )
	{
	    /* splice into allocated object queue */
	    if (0 == tag)
	    {
# ifdef OS_THREADS_USED
	    	CS_synch_lock( &MElist_mutex );
# endif /* OS_THREADS_USED */
	    	(void)QUinsert( (QUEUE *) node, (QUEUE *) MElist.MElast );
# ifdef OS_THREADS_USED
		CS_synch_unlock( &MElist_mutex );
# endif /* OS_THREADS_USED */
	    }
	    else
	    {
		IIME_atAddTag(tag, node);
	    }
	    /* Set values in block to be returned */
	    node->MEtag = tag;
	    node->MEsize = nsize;
	    node->MEaskedfor = size;
	    
	    /* Fill in the returned pointer */
	    block = (PTR)((char *)node + sizeof(ME_NODE));
	    
	    if (zero)
		MEfill( (nsize - sizeof(ME_NODE)), 0, block);
	}  /* got node OK */
    }
    if (status != NULL)
	*status = MEstatus;
    if (MEstatus != OK)
	return((PTR)NULL);
    else
	return(block);
}

/*
** Debugging aid to display the allocated and free nodes
**
**    While in debugger, "call MEshow()"
**
**    Lists each allocated node and the 1st 16 bytes of the
**    associated memory.
*/
VOID
MEshow()
{
    ME_NODE *node;
    PTR	    block;
    ME_NODE *start;
    
    SIZE_TYPE	nodes, sizet, askedfor_t;

    TRdisplay("Allocated Nodes:\n");

    nodes = sizet = askedfor_t = 0;

    start = (ME_NODE *)&MElist;
    node = MElist.MEfirst;

    while ( node != NULL && node != start )
    {
	block = (PTR)((char *)node + sizeof(ME_NODE));

	TRdisplay("Block %p, MEsize %6d MEaskedfor %6d : %4.4{%x %} %#.1{%c%}\n",
	    block, node->MEsize, node->MEaskedfor,
	    block, 0,
	    (node->MEaskedfor) > (SIZE_TYPE) 16 ? 16 : node->MEaskedfor, block, 0);

	sizet += node->MEsize;
	askedfor_t += node->MEaskedfor;
	nodes++;
	node = node->MEnext;
    }

    TRdisplay("Allocated nodes %d, size %d askedfor %d\n\n",
	nodes, sizet, askedfor_t); 


    
    TRdisplay("Free Nodes:\n");

    nodes = sizet = askedfor_t = 0;

    start = (ME_NODE *)&MEfreelist;
    node = MEfreelist.MEfirst;

    while ( node != NULL && node != start )
    {
	TRdisplay("Node %p, MEsize %6d\n",
	    node, node->MEsize );
	sizet += node->MEsize;
	nodes++;
	node = node->MEnext;
    }

    TRdisplay("Free nodes %d, size %d\n\n", nodes, sizet); 
}
