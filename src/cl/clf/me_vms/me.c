/*
**    Copyright (c) 1990, 2000 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <cs.h>
#include    <vmsclient.h>
#include    <me.h>
#include    <ssdef.h>
#include    <libvmdef.h>
#include    <lib$routines.h>

/*
**  ME.c
**
**  History
**
**	30-Apr-90 (stevel)	
**		First written; replace VMS memory allocation with
**		native calls for performance
**	4/91 (Mike S)
**		Keep ME statistics; these can be used for debugging.
**      4/92 (Mike S)
**		Improve behavior of tag 0 allocations:
**		  Use 64K extents
**		  Put allocations larger than 64K into their own zones.
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**	8-jun-93 (ed)
**		changed to use CL_PROTOTYPED
**	08-mar-93 (walt)
**		The top of file prototypes for MEdoAlloc, MEfindzone,
**		MEgetzone, and MEmakezone need parameters if
**		CL_BUILD_WITH_PROTOS is set, to avoid Alpha compiler complaints
**		about changed definitions later when the real functions are
**		found.
**	08-jul-93 (walt)
**		Take out the prototyped definitions added 08-mar-93.  They're
**		nolonger needed now that the prototyping project is moving
**		along.
**      16-jul-93 (ed)
**	    added gl.h
**	11-aug-93 (ed)
**	    unconditional prototypes
**	22-mar-95 (albany)
**	    Removed register storage class from p_entry for AXP/VMS to shut
**	    up compiler messages.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	08-aug-2001 (kinte01)
**	    Use SIZE_TYPE for lengths.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
**	11-May-2009 (kschendel) b122041
**	    Change MEfree arg to void *, more appropriate, fewer warnings.
*/

/*
**	This block is prepended to all allocations
*/
typedef struct
{
	i4	MEsize;		/* size of node (including head) */
	i2	MEtag;		/* user provided tag for block of memory */
	i2	MEmagic;	/* Magic number, to identify allocated block */
	i4	MEzoneid;	/* Used only for single-allocation zones;
				   0 otherwise */
	i4	pad;		/* Make a round size */
} ME_NODE;

# define	MENODESIZE		(sizeof(ME_NODE))
# define 	MEMAGIC			42
# define 	MEBADMAGIC		-1

# define	MEZONE0EXTENDSIZE	128
# define	MEZONE0MAXSIZE		(512 * MEZONE0EXTENDSIZE)
/*
**	This is an entry in the hash table which translates our tags to
**	VMS zone id's.
*/
typedef struct _metagzone
{
    	struct _metagzone	*next;		/* 'Next' pointer */
	i4			zone;		/* VMS zone id */
	i4			size;		/* Amount allocated to tag. */
    	i2			tag;		/* tag */
} METAGZONE;

/*
** htab is the hash table.  Each bucket points to a linked list of
** METAGZONE for the tags that hash to this bucket.  The hash function
** is simply the value of the tag modulo 256.  Since tags are normally
** allocated sequentially, this should give as good a distribution as
** anything. We'll special-case tag zero for fast access.
*/
static METAGZONE *htab[256] ZERO_FILL;
static METAGZONE zone_zero ;
GLOBALDEF METAGZONE *MEp_zone_zero = NULL;

GLOBALDEF CS_SYNCH      MEbrk_mutex ZERO_FILL;

/* 
** freelist is a list of free METAGZONEs linked by their next fields.
** When a new METAGZONE is needed, it is taken from this list.  If the
** list is empty, then a number of METAGZONEs is allocated from dynamic
** memory and put on this list.
*/
# define FREELISTINC 50		/* Number of freelist entries to add at once */
static METAGZONE *freelist ZERO_FILL;

static STATUS
MEdoAlloc(
        i2      tag,
        i4      num,
        i4      size,
        i4      *block,
        bool    zero);

static STATUS
MEfindzone(
        i2              tag,
        METAGZONE       ***p_entry);

static STATUS
MEgetzone(
        i2              tag,
        bool            create,
        i4              *zone);

static STATUS
MEmakezone(
        i2 tag,
        i4 *zone);


GLOBALREF SIZE_TYPE	i_meactual;

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
**					 ME_CORRUPTED.
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
*/
PTR
MEreqmem(
	u_i2	tag,
	SIZE_TYPE size,
	bool	clear,
	STATUS	*status)
{
	PTR	block;
	STATUS	istatus;

	/*
	**  When the ME module is re-written, then the contents
	**  of this routine will likewise change.  No further use
	**  of MEdoAlloc should be propagated at that time.  (bab)
	*/
	istatus = MEdoAlloc((i2)tag, (i4)1, (i4)size, (i4 *)&block, clear);

	if (status != NULL)
		*status = istatus;

	if (istatus != OK)
		return((PTR)NULL);
	else
		return(block);
}


/*{
** Name:	MEreqlng	- Allocate memory of specified size.
**
** Description:
**	Returns a pointer to a block of memory of size bytes.
**	A tag must be specified, although that tag may be 0
**	as a default value.  If the 'clear' parameter is TRUE,
**	then the allocated memory is zeroed.  The fourth parameter
**	may be NULL, in which case no detailed status information
**	is returned.  Returns NULL if an error occurs.
**	This routine is like MEreqmem, except that the amount of
**	memory allocated may be a larger number of bytes than can
**	fit in a u_i4.
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
**					 ME_CORRUPTED.
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
*/
PTR
MEreqlng(
	u_i2		tag,
	u_i4	size,
	bool		clear,
	STATUS		*status)
{
	PTR	block;
	STATUS	istatus;

	/*
	**  When the ME module is re-written, then the contents
	**  of this routine will likewise change.  No further use
	**  of MEdoAlloc should be propagated at that time.  (bab)
	*/
	istatus = MEdoAlloc((i2)tag, (i4)1, (i4)size, (i4 *)&block, clear);

	if (status != NULL)
		*status = istatus;

	if (istatus != OK)
		return((PTR)NULL);
	else
		return(block);
}


/*
 *	Function:
 *		MEgetzone
 *
 *	Arguments:
 *		i2		tag;
 *		bool		create;
 *		i4		*zone;
 *
 *	Result:
 *		Look up 'tag' in the hash table find its corresponding VMS
 *		'zone'.  If found, save it in *zone and return OK. If not 
 *		found then return ME_BD_TAG unless bCreateIfNotFound is TRUE. 
 *		In that case we will create a new VMS zone and add an entry to 
 *		the hash table which ties a tag to a zone.
 *
 *		Returns STATUS: OK, ME_BD_TAG, ME_GONE.
 *
 *	Side Effects:
 *		Creates a VMS zone if necessary and adds an entry to the hash 
 *		table.
 */

static STATUS
MEgetzone(
	i2		tag,
	bool		create,
	i4		*zone)
{
    	METAGZONE **p_entry;
	STATUS	status;

	/* Find the zone, if it exists */
	if ((status = MEfindzone(tag, &p_entry)) == FAIL)
	{
		if (create)
		{
	    		/* It doesn't.  Try to create it */
			return MEmakezone(tag, zone);
		}
		else
		{
	    		/* It doesn't.  Return failure */
			return FAIL;
		}
	}		
	else if (status == OK)
	{
		*zone = (*p_entry)->zone;
		return OK;
	}
	else
	{
		return status;
	}		
}

/*
**	Find a zone in the hash structure.  
**
**	Inputs:
**			tag		tag number to look up
**	Outputs:
**			p_entry		pointer to hash table pointer which 
**					points to zone structure
**
**	Returns:	ME_BD_TAG	bad tag
**			FAIL		Can't find it.
**			OK		fund it
*/
static STATUS
MEfindzone(
	i2		tag,
	METAGZONE	***p_entry)
{
    	register METAGZONE **current;
    	STATUS status;

	/*
	** Since most of our calls will be for zone zero we put in some special
	** code to speed up recognizing it.
	*/
	if (tag == 0)
	{
		if (MEp_zone_zero != NULL)
		{
			*p_entry = &MEp_zone_zero;
			return OK;
		}
		else
		{
			return FAIL;
		}
	}
	else if (tag < 0)
	{
		return ME_BD_TAG;
	}

	/* Look up 'tag' in the hash table. */
	for (current = &htab[tag%256]; 
	     *current != NULL; 
	     current = &((*current)->next))
	{
		if ((*current)->tag == tag)
		{
			/* Found it */
			*p_entry = current;
			return(OK);
		}
	}

	/* We didn't find it. */
	return FAIL;
}

static STATUS
MEmakezone(
	i2 tag,
	i4 *zone)
{
	i4	algorithm,algorithm_arg,flags,extend_size,initial_size;
	i4	block_size,alignment,page_limit,p1;
   	METAGZONE *current;
	STATUS	cc;

	/*
	** For tag 0, we will use the "First Fit" algorithm.
	**
	** Otherwise we'll use the "Frequent Sizes" algorithm, since
	** we expect to see the 512-byte chunk allocated by FEreqmem
	** pretty often.
	*/
	if (0 == tag)
	{
		algorithm = LIB$K_VM_FIRST_FIT;
		initial_size = 0;
		extend_size = MEZONE0EXTENDSIZE;
	}
    	else
	{
		algorithm = LIB$K_VM_FREQ_SIZES;
		algorithm_arg = 4;
		initial_size = 8;	/* Start with 4K per zone */
		extend_size = 16;	/* Increment by 8K when we run out */
	}
	flags = LIB$M_VM_BOUNDARY_TAGS;	/* Speed up deallocation */
	block_size = 8;		/* Allocate in 8-byte chunks */
	alignment = 8;		/* Allocate on 8-byte boundaries */
	page_limit = 0;		/* No limit on total allocation */
	p1 = 32;		/* Lookaside list base */

	cc = lib$create_vm_zone(zone,
		&algorithm, &algorithm_arg, &flags, &extend_size,
		&initial_size, &block_size, &alignment, &page_limit, 
		&p1);

	if (cc != SS$_NORMAL)
	{
		/* We couldn't create the zone.  We'll assume a memory problem */
		return ME_GONE;
	}

	/* We have space set aside for the tag 0 zone */
	if (0 == tag)
	{
		zone_zero.tag = 0;
		zone_zero.zone = *zone;
		MEp_zone_zero = &zone_zero;
		return OK;
	}

	/*
	** We were sucessful in creating the zone, so now we must add
	** an entry into the hash table which maps from tag->zoneid.
	*/
	if (freelist == NULL)
	{
		register METAGZONE	*next;
		register int		i;
		i4 			freesize;

		/*
		** Allocate some storage out of the "default" zone zero,
		** which should always be available.
		*/
		freesize = sizeof(METAGZONE) * FREELISTINC;
		cc = lib$get_vm(&freesize,&freelist,0);

		if (cc != SS$_NORMAL)
		{
			/* No free space at all */
			return(ME_GONE);
		}

		MEfill(sizeof(METAGZONE) * FREELISTINC, 0, (char *)freelist);

		/*
		**	Initialize the freelist
		*/
		for (i = 0, next = freelist; i < FREELISTINC - 1; i++)
		{
			next->next = next + 1;
			next = next->next;
		}
		next->next = NULL;
	}

	/*
	**	Carve the first entry off the freelist
	*/
	current = freelist;
	freelist = freelist->next;
	current->next = htab[tag%256];
	htab[tag%256] = current;


	/*
	**	Initialize it so that it links a tag and a zone.
	*/
	current->tag = tag;
	current->zone = *zone;
	current->size = 0;

	return(OK);
}


/*
 *	Function:
 *		MEdoAlloc
 *
 *	Arguments:
 *		i2	tag;
 *		i4	num;
 *		i4	size;
 *		i4	*block;
 *		bool	zero;
 *
 *	Result:
 *		Sets 'block' to pointer to memory which can hold 'num' objects
 *			of 'size' bytes, tagged by 'tag'.
 *
 *		If 'zero' initialize 'block' to all 0's.
 *
 *		If something goes wrong 'block' will be set to NULL.
 *
 *		Returns STATUS: OK, ME_NO_ALLOC, ME_GONE, ME_00_PTR, ME_BD_TAG.
 *
 *		called by MEalloc() and MEtalloc().
 *		calls MEinitLists() and MEadd().
 *
 *	Side Effects:
 *		None
 */

static STATUS
MEdoAlloc(
	i2	tag,
	i4	num,
	i4	size,
	i4	*block,
	bool	zero)
{
	i4	fullsize;
	ME_NODE	*addr;
	i4	zone;
	STATUS	cc;
	bool	separate_zone = FALSE;

	if (block == NULL)
		return ME_00_PTR;

	if ((size <= 0) || (num <= 0))
		return ME_NO_ALLOC;

	/*
	**	Look up the supplied tag in our hash table to find the corresponding
	**	VMS "zone". If we can't find one then we must create one.
	*/
	*block = NULL;				/* Assume failure */
	fullsize = num * size + MENODESIZE;

	if ((cc = MEgetzone(tag, TRUE, &zone)) != OK)
		return cc;
	if (tag == 0 && fullsize > MEZONE0MAXSIZE)
	{
		/* Make a separate zone for this allocation */
		if (lib$create_vm_zone(&zone) != SS$_NORMAL)
			return FAIL;
		separate_zone = TRUE;
	}

	if (lib$get_vm(&fullsize,&addr,&zone) != SS$_NORMAL)
		return ME_GONE;


	/* We've got the memory */
	*block = (i4)addr + MENODESIZE;

	if (zero)
		MEfill(fullsize, 0, (char *)addr);

	addr->MEsize = fullsize;
	addr->MEtag = tag;
	addr->MEmagic = MEMAGIC;
	if (separate_zone)
		addr->MEzoneid = zone;
	else
		addr->MEzoneid = 0;
	i_meactual += fullsize;
	if (tag != 0)
	{
	    METAGZONE	**tzone;

	    if (MEfindzone(tag, &tzone) != OK)
		return FAIL;
	    (*tzone)->size += fullsize;
	}
	else
	{
		zone_zero.size += fullsize;
	}

	return(OK);
}

/*
 *	Name:
 *		MEfree.c
 *
 *	Function:
 *		MEfree
 *
 *	Arguments:
 *		void *	block;
 *
 *	Result:
 *		Frees the block of memory pointed to by 'block'.
 *
 *		First insures that the address passed in is in the process' data space.
 *
 *		Returns STATUS: OK, ME_00_FREE, ME_BD_TAG, ME_GONE.
 *
 *	Side Effects:
 *		None
 */

STATUS
MEfree(
	void	*block)
{
	METAGZONE **tzone;
	ME_NODE	*node;
	i4 size;

	if (block == NULL)
		return ME_00_FREE;

	/*
	** All blocks have been preceded by a fullword containing their 'tag'
	** length, and a magic number.  Check for the magic number to see if
	** this is an allocated block, then use the tag to find the VMS zone 
	** for the block.
	*/
	node = (ME_NODE*)((char *)block - MENODESIZE);

	if (MEMAGIC != node->MEmagic)
		return FAIL;	/* Too bad there's no explicit code for this */

	/* 
	** VMS takes a long time to decide that a freed block can't be freed
	** again.  We'll invalidate the magic number in case this one gets freed
	** a second time.
	*/
	node->MEmagic = MEBADMAGIC;

	size = node->MEsize;
	if (MEfindzone(node->MEtag, &tzone) != OK)
		return FAIL;
	if (node->MEzoneid != 0)
	{
		/* This is a one-allocation zone; delete the zone */
		if (lib$delete_vm_zone(&node->MEzoneid) != SS$_NORMAL)
			return FAIL;
	}
	else
	{
		if (lib$free_vm(&node->MEsize, &node, &(*tzone)->zone) 
			!= SS$_NORMAL)
		{
			return FAIL;
		}			

	}

	i_meactual -= size;
	(*tzone)->size -= size;
	return OK;
}



/*
 *	Function:
 *		MEtfree
 *
 *	Arguments:
 *		i2	tag;
 *
 **	Since we're not currently allowing MEtfree(0), we needn't worry about
 **	single-allocation zones for large allocations.
 **
 *	Result:
 *		Free all blocks of memory on allocated list with MEtag value 
 **		of 'tag'.
 *
 *		Returns STATUS: OK, ME_BD_TAG.
 *
 *	Side Effects:
 *		None
**	History:
**	    11-aug-93
**		changed to match me.h prototype
 */

STATUS
MEtfree(
	u_i2	tag)
{
	STATUS status;
	METAGZONE **tzone;
	METAGZONE *freenode;

	if (tag <= 0)
		return ME_BD_TAG;

	/* 
	** Get the zone which corresponds to the tag.  If there isn't one,
	** it might mean that this tag was already freed.  It isn't an error
	** to do that twice, so we'll return OK, just in case.
	*/
	if ((status = MEfindzone(tag, &tzone)) != OK)
		return OK;

	/*
	**	Delete the entire zone (and all the memory it owns)
	*/
	status = lib$delete_vm_zone(&(*tzone)->zone);

	/*
	**	Clip the METAGZONE entry out of the hash table and
	**	chain it back onto the freelist for reuse.
	*/
	i_meactual -= (*tzone)->size;
	freenode = *tzone;
	*tzone = (*tzone)->next;
	freenode->next = freelist;
	freelist = freenode;

	return(OK);
}

/*
 *	Function:
 *		MEsize
 *
 *	Arguments:
 *		i4	block;
 *		i4	*size;
 *
 *	Result:
 *		return in 'size' the number of (usable) bytes in block of memory
 *			whose address is stored in 'block' which must have been
 *			allocated by ME[t]alloc().
 *
 *		Returns STATUS: OK, ME_00_PTR.
 *
 *	Side Effects:
 *		None
 */

STATUS
MEsize(
	PTR	block,
	SIZE_TYPE *size)
{
	ME_NODE	*node;

	if (block == NULL)
		return ME_00_PTR;

	node = (ME_NODE *)(block - MENODESIZE);
	if (MEMAGIC != node->MEmagic)
		return FAIL;

	*size  = node->MEsize - MENODESIZE;

	return(OK);
}

/*
**	Obsolete functions
*/

/*
 *	Function
 *	    MEalloc
 *
 *	Arguments:
 *		i4	num;
 *		i4	size;
 *		i4	*block;
 *
 *	Result:
 *		Sets passed in pointer 'block' to memory which can hold 'num'
 *			objects of 'size' bytes. 
 *
 *		Returns STATUS: OK, ME_NO_ALLOC, ME_GONE, ME_00_PTR, ME_CORRUPTED.
 *
 *	Side Effects:
 *		None
 *
 */

STATUS
MEalloc(
	i4	num,
	i4	size,
	i4	*block)
{

	return(MEdoAlloc((i2)0, num, size, block, FALSE));
}

/*
 *
 *	Function:
 *		MEcalloc
 *
 *	Arguments:
 *		i4	num;
 *		i4	size;
 *		i4	*block;
 *
 *	Result:
 *		Sets passed in pointer 'block' to memory which can hold 'num'
 *			objects of 'size' bytes. 
 *		Zero the resultant 'block'.
 *
 *		Returns STATUS: OK, ME_NO_ALLOC, ME_GONE, ME_00_PTR, ME_CORRUPTED.
 *
 *	Side Effects:
 *		None
 */

STATUS
MEcalloc(
	i4	num,
	i4	size,
	i4	*block)
{

	return(MEdoAlloc((i2)0, num, size, block, TRUE));
}

/*
 *	Function:
 *		MEtalloc
 *
 *	Arguments:
 *		i2	tag;
 *		i4	num;
 *		i4	size;
 *		i4	*block;
 *
 *	Result:
 *		Sets 'block' to pointer to memory which can hold 'num' objects
 *			of 'size' bytes, tagged by 'tag'.
 *
 *		If something goes wrong 'block' will be set to NULL.
 *
 *		Returns STATUS: OK, ME_NO_ALLOC, ME_GONE, ME_00_PTR, ME_CORRUPTED, ME_BD_TAG.
 *
 *	Side Effects:
 *		None
 */

FUNC_EXTERN STATUS
MEtalloc(
#ifdef CL_PROTOTYPED
	i2	tag,
	i4	num,
	i4	size,
	i4	*block)
#else
        tag,num,size,block)
        i2	tag;
	i4	num;
	i4	size;
	i4	*block;
#endif
{

	return((tag <= 0) ? 
			ME_BD_TAG : MEdoAlloc(tag, num, size, block, FALSE));
}

/*
 *	Function:
 *		MEtcalloc
 *
 *	Arguments:
 *		i2	tag;
 *		i4	num;
 *		i4	size;
 *		i4	*block;
 *
 *	Result:
 *		Sets 'block' to pointer to memory which can hold 'num' objects
 *			of 'size' bytes, tagged by 'tag'.
 *
 *		Set 'block' to 0's.
 *
 *		If something goes wrong 'block' will be set to NULL.
 *
 *		Returns STATUS: OK, ME_NO_ALLOC, ME_GONE, ME_00_PTR, ME_CORRUPTED, ME_BD_TAG.
 *
 *	Side Effects:
 *		None
 */

FUNC_EXTERN STATUS
MEtcalloc(
#ifdef CL_PROTOTYPED
        i2      tag,
        i4      num,
        i4      size,
        i4      *block)
#else
        tag,num,size,block)
        i2      tag;
        i4      num;
        i4      size;
        i4      *block;
#endif

{
	return((tag <= 0) ? ME_BD_TAG : MEdoAlloc(tag, num, size, block, TRUE));
}
