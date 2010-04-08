/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<me.h>
#include	<nm.h>
#ifdef	FE_ALLOC_DBG
#include	<lo.h>
#include	<si.h>
#endif
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"feloc.h"	/* includes <me.h> */

/**
** Name:	fedoreq.c - Front-End Utiltiy Internal Memory Allocation Module.
**
** Description:
**	Contains the internal memory allocation routine for the FE memory
**	allocation module.  Defines:
**
**	iiugReqMem()		allocate block of storage.
**
** History:
**
**	Revision 6.2  89/05/04  bryanp
**	Align new memory in '_newAlloc()' for BYTE_ALIGN machines.
**	12/05/88 (dkh) - Performance changes.
**
**	Revision 6.0  87/07/06  bruceb
**	Modified 'FEdoalloc()' in "fedoalloc.c".
**	07-jan-88  bruceb
**		Only MEfree if iiUGtags is non-NULL.
**
**	Revision 3.0  85/01/15  ron
**	Initial revision.
**
**      04-Mar-93 (smc)
**      Removed forward declaration of prototyped functions and
**      cast parameter of MEfree to match prototype.
**    05-Apr-93 (swm)
**        Removed conditional forward references to ME functions. The
**        correct forward references are inherited via <me.h> (which
**        is included indirectly via "feloc.h").
**	7/93 (Mike S)
**	    Major rework to use ME (in GL) to allocate tags for us.
**	    Also, don't to NMgtAt for obsolete logical every time we
**	    allocate a piece of memory!
**	15-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**  23-july-97 (rodjo04)
**      bug 83839. Increased the value of HASHSIZE to 182 so that there 
**      will not be a position overflow of hashtable in find_hash_table. 
**	3-apr-1997 (donc)
**		OpenROAD 4.0 needs higher HASHSIZE and ENTRYCLUSTER values.
**	9-apr-1997 (donc)
**		Backout last change, it may be not be needed.
**	09-Apr-1997 (kinpa04, schte01)
**        Changed hash algorithm in find_hash_entry to use modulo
**        instead of divide when calculating hash table entry. This
**        prevents indexing beyond the end of the table when the tag
**        is greater than 16K and a more even distribution.
**	03-Aug-2000 (wanfr01)
**	   Bug 102265, INGCBT 294
**	   When iiugReqMem, if a memory block needs to be created,
**	   request 8 times more (capped at MAXI2).  Future requests
**	   can then use this memory block, rather than repeatedly
**	   calling _newAlloc, which causes a performance hit.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	14-feb-2001 (somsa01)
**	    More changes involving the use of SIZE_TYPE for lengths.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
[@history_template@]...
**/

# define	MAXUI2		((u_i4)MAXI2 << 1)
# define	TAGBLKINC	128

static FE_BLK	*_newAlloc();
static PTR	_doreqMem();

static bool first_time = TRUE;

/*
** Hash table for tags we're using
*/
# define HASHSIZE 182		/* size of hash table */
# define ENTRYCLUSTER 50	/* Number of HASHENTRYs to allocate at once */

typedef struct hashentry
{
    struct hashentry *nextentry;
    FE_BLK *block;
    u_i4 tag;
} HASHENTRY;

static HASHENTRY *hashtable[HASHSIZE];
static HASHENTRY *freeentries = NULL;

static HASHENTRY ** find_hash_entry(u_i4 tag);
static HASHENTRY * make_hash_entry(u_i4 tag, HASHENTRY **entryptr);

GLOBALREF SIZE_TYPE	FE_allocsize;

/*{
** Name:	iiugReqMem() -	Allocate Block of Storage.
**
** Description:
**	The storage allocator for the FE routines, which is fundamentally
**	different from the ME allocator.  Unlike the ME allocator, this
**	allocator does not associate header info. with each block of storage
**	returned to the caller.  Instead, it simply returns an address into
**	a free block of storage large enough to accomodate the request (no
**	header.)  Storage allocated in this way can only be freed by tag
**	region and ONLY by 'FEfree()'.  Furthermore, ME routines such as
**	'MEsize()' are incompatable with the FE storage structures.
**
** Inputs:
**	tag	{u_i4}  Tag-ID associated with the region.
**	size	{u_i4}  Size of memory (in bytes) to allocate.
**	clear	{bool}  Whether storage is to be cleared.
**
** Outputs:
**	status	{STATUS *}  Set to OK or FAIL depending on whether the
**				allocation succeeded.
**
** Returns:
**	{PTR}  Pointer to the allocated memory or NULL on error.
**
** History:
**	1/15/1985 (rlk) - First written.  (As 'FEdoalloc()'.)
**	06-jul-87 (bab)
**		Modified from FEdoalloc.  Requests for 0 bytes of memory
**		are treated differently than they were for FEdoalloc;
**		FAIL is returned along with the NULL pointer.
**      9/93 (bobm)     Add purify_override_iiugReqMem
**	24-aug-1993 (rdrane)
**		Fix validation of tag - prior changes invalidated the
**		MINTAGID and MAXTAGID #define's.
*/

#ifdef BYTE_ALIGN
#define	ADJ_FCTR	(sizeof(ALIGN_RESTRICT) - 1)
#define	SIZE_ADJ(s)	(((s) + ADJ_FCTR) & ~ADJ_FCTR)
#else
#define	SIZE_ADJ(s)	(s)
#endif

/*
** for purify, turn everything into individual allocations so
** that purify can track them.
*/

#ifdef DEVEL_DEBUG

PTR
purify_override_iiugReqMem(tag,size,clear,status)
u_i4		tag;
u_i4	size;
bool		clear;
STATUS		*status;
{

	if ( tag == 0 || size == 0 )
	{
		*status = FAIL;
		return (PTR)NULL;
	}

	return MEreqmem(tag, size, clear, status);
}

#endif


PTR
iiugReqMem ( u_i4 tag, SIZE_TYPE size, bool clear, STATUS *status )
{
    SIZE_TYPE		reqsize;	/* Aligned size of request */
    SIZE_TYPE		treqsize;	/* Aligned size of request */
    register FE_BLK	*blkp;		/* Ptr into a 'iiUGtags[]' lists */
    PTR			block;
    HASHENTRY		*entry;
    HASHENTRY		**entryptr;

#ifdef	FE_ALLOC_DBG
    GLOBALREF bool	FE_firstcall;
    GLOBALREF FILE	*FE_dbgfile;
    GLOBALREF char	*FE_dbgname;
#ifdef  FE_ALLOC_DBGSUM
    GLOBALREF i4	FE_totbyt;
    GLOBALREF i4	FE_totblk;
#endif /* FE_ALLOC_DBGSUM */

    /*	If first call to 'iiugReqMem()', initialize debug file */

    if (FE_firstcall)
    {
	LOCTYPE		loctyp;
	char		locbuf[MAX_LOC];
	LOCATION	loc;

	FE_firstcall = FALSE;

	loctyp = FILENAME;
	STcopy(FE_dbgname, locbuf);
	LOfroms(loctyp, locbuf, &loc);
	SIopen(&loc, ERx("w"), &FE_dbgfile);
    }
#endif /* FE_ALLOC_DBG */

    /*
    ** Make sure FE_allocsize has been defined and is valid (the total
    ** allocation size must be greater than the header size.
    */

    if ( first_time && FE_allocsize <= (sizeof(FE_BLK)) )
    {
	char	*tmp;

	NMgtAt(ERx("FEALLSIZE"), &tmp);
    	if ( tmp == NULL || *tmp == EOS || CVal(tmp, (i4*)&FE_allocsize) != OK )
	    FE_allocsize = ALLOCSIZE;
	first_time = FALSE;
    }

    /*  Ensure that request is valid */
    if (tag == 0 || size == 0)
    {
	*status = FAIL;
	return (PTR)NULL;
    }

    /*	Align the storage for the request */

    reqsize = SIZE_ADJ(size);

# ifdef	FE_ALLOC_DBGLIST
    SIfprintf(FE_dbgfile,
	ERx("\n----- iiugReqMem -----\n\tTag = %x   Size = %x (aligned = %x)   clear = %x\n"),
	(i4)tag, size, reqsize, clear
    );
# endif

    /*
    ** Find the FE_BLK to allocate from.
    */
    entryptr = find_hash_entry(tag);
    if (*entryptr == NULL)
    {
	entry = make_hash_entry(tag, entryptr);
	if (entry == NULL)
	    return (PTR)NULL;
    }
    else
    {
	entry = *entryptr;
    }
     
    /*
    ** Check if there is a FE_BLK (on the list associated with the
    ** specified tag) having enough space to satisfy the request.
    ** If not, allocate a new FE_BLK and place it into the list.
    */
    /*	First, check for a empty list of storage blocks */

    if ( entry->block == NULL)
    {
	entry->block = _newAlloc(tag, reqsize, status);
	blkp = entry->block;
    }
    else
    { /* Scan the list for a FE_BLK having enough free space */

	blkp = entry->block;
	while ( blkp->fe_next != NULL && blkp->fe_bytfree < reqsize )
	{
	    blkp = blkp->fe_next;
	}

	if ( blkp->fe_bytfree < reqsize )
	{
	    if (reqsize*8 < MAXI2)
	       treqsize = reqsize * 8;
	    else
	       treqsize = reqsize;
	    blkp->fe_next = _newAlloc(tag, treqsize, status);
	    blkp = blkp->fe_next;
	}
    }
# ifdef DEBUGPRINT
    SIprintf("Calling _doreqMem for tag %d ...\n", tag);
# endif
    block = _doreqMem(blkp, reqsize, clear);

#ifdef  FE_ALLOC_DBG
    fe_dbg2_trace(tag);
#endif

    *status = (block == (PTR)NULL) ? FAIL: OK;

    return block;
}

/*{
** Name:	IIUGfreetagblock - Free the memory associated with a tag block
**
** Description:
**	Find the tag block in the hash table.  Free its memory, and put
**	the hash table entry on the free list.
**
** Inputs:
**	tag	u_i4	tag number.
**
** Returns:
**	STATUS		OK	success
**			FAIL	could not find hash entry
**
** Re-entrancy:
**	Not interruptible, since it maintains a linked list.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	7/93 (Mike S) Initial version
*/
STATUS
IIUGfreetagblock(u_i4 tag)
{
    HASHENTRY **entry;
    HASHENTRY *block;
    STATUS retstat;

    if (tag == 0)
	return FAIL;		/* Invalid tag */

    /*
    ** Free the memory associated with this tag.  We may not find a hash
    ** entry for it, if no memory was allocated on it via the FE allocator.
    */
    retstat = MEtfree(tag);
    if (retstat != OK && retstat != ME_NO_TFREE)
	return retstat;

    entry = find_hash_entry(tag);
    if (entry == NULL || *entry == NULL)
	return OK;

# ifdef FE_ALLOC_DBG
	if ( FE_dbgfile != NULL )
		SIfprintf( FE_dbgfile, 
			   ERx("tag = %d, FE_BLK = %p\n"),
			   tag, (*entry)->block);
# endif


    /* Relink the tag entry onto the free list */
    block = *entry;
    *entry = block->nextentry;
    block->nextentry = freeentries;
    freeentries = block;

    return OK;
}

/*
** find_hash_entry
**
** Find the hash entry for a tag.  On success, this returns a pointer
** to the hash list pointer to the entry.  On failure, it returns a pointer
** to the NULL pointer which terminates the hash chain.  Thus, the return is
** useful to a caller who wnats to find the entry, delete it, or add it.
*/
static HASHENTRY **
find_hash_entry(u_i4 tag)
{
    HASHENTRY **entry;
    /* changed this operation to get remainder */
    int		tmptag = tag % HASHSIZE;

    entry = &hashtable[tmptag];

    for (;*entry != NULL; entry = &(*entry)->nextentry)
    {
	if ((*entry)->tag == tag)
	    return entry;
    }

    return entry;
}

/*
** make_hash_entry(tag, entryptr);
**
** Make a new hash table entry at the place pointed to by 
** entryptr.
*/
static HASHENTRY *
make_hash_entry(u_i4 tag, HASHENTRY **entryptr)
{
    if (freeentries == NULL)
    {
	/* There are no free hash table enteries, so make some */
	HASHENTRY *entrylist;

	entrylist = (HASHENTRY *)MEreqmem(0, sizeof(HASHENTRY) * ENTRYCLUSTER,
					  (bool)FALSE, (STATUS *)NULL);
	if (entrylist == NULL)
	{
	    /* Give up */
	    return NULL;
	}
	else
	{
	    HASHENTRY *loopptr;

	    entrylist->nextentry = NULL;
	    for (loopptr = entrylist + 1;
		 loopptr < entrylist + ENTRYCLUSTER;
		 loopptr++)
	    {
		loopptr->nextentry = loopptr - 1;
	    }
	    freeentries = &entrylist[ENTRYCLUSTER-1];
	}
    }

    /* We have a free entry now; place it correctly */
    *entryptr = freeentries;
    freeentries = freeentries->nextentry;
    (*entryptr)->nextentry = NULL;
    (*entryptr)->block = NULL;
    (*entryptr)->tag = tag;

    return (*entryptr);
}

/*
** Name:	_doreqMem() -	Reset internal pointers and clear memory.
**
** Description:
**	Return pointer to memory and modify free list pointers.
**
** Inputs:
**	blk	{FE_BLK *}  Block of memory from which to grab the request.
**	reqsize	{u_i4}  Requested amount of memory
**				(adjusted for alignment.)
**	clear	{bool}  Indicates if memory is to be cleared out.
**
** Returns:
**	{PTR}  Pointer to the requested memory.
**
** History:
**	1/15/1985 (rlk) - First written.
**	06-jul-87 (bab)	Modified from code in "fedoalloc.c"
*/
static PTR
_doreqMem ( blk, reqsize, clear )
register FE_BLK	*blk;
SIZE_TYPE	reqsize;
bool		clear;
{
    register PTR	block;
    SIZE_TYPE		save_reqsize = reqsize;

    block = NULL;

    /*	Check the block */

    if (blk == NULL)
	return block;

    /*
    ** Have a FE_BLK satisfying the request.
    ** 	1) Get a pointer to the free storage,
    ** 	2) update the size remaining,
    ** 	3) update the free pointer,
    **	4) clear the storage if requested,
    **	5) return.
    **
    ** The requested size has already been adjusted for alignment.
    */

    block = blk->fe_freeptr;
    blk->fe_bytfree -= reqsize;
    blk->fe_freeptr = (PTR)((char *)blk->fe_freeptr + reqsize);

    if (clear)
    {
	register PTR	clrblk = block;

	while (reqsize > 0)
	{
	    register SIZE_TYPE	clrsize;

	    clrsize = min(reqsize, MAXUI2);
	    MEfill(clrsize, '\0', clrblk);
	    reqsize -= clrsize;
	    clrblk = (PTR)((char *)clrblk + clrsize);
	}
    }

# ifdef DEBUGPRINT
    SIprintf("%d bytes at %x-%x\n", save_reqsize, (i4)block, (i4)block + save_reqsize - 1);
# endif
    return block;
}

/*
** Name:	_newAlloc() -	Allocate new memory when none to re-use.
**
** Description:
**	Allocate more memory when none exists on the free list to re-use.
**
** Inputs:
**	tag	{u_i4}  Tag ID to be associated with this memory.
**	size	{u_i4}  Amount of memory to be allocated (adjusted for
**				alignment.)
**
** Outputs:
**	status	{STATUS *}  ME allocation status.
**
** Returns:
**	{PTR}  Pointer to the allocated memory.
**
** History:
**	1/15/1985 (rlk) - First written.
**	06-jul-87 (bab)	Modified from code in "fedoalloc.c"
**	4-May-89 (bryanp)
**		Align memory for BYTE_ALIGN machines.
*/
static FE_BLK *
_newAlloc ( tag, size, status )
u_i4		tag;
SIZE_TYPE	size;
STATUS		*status;
{
    SIZE_TYPE		mereqsize;
    SIZE_TYPE		hdr_size;
    register FE_BLK	*block;

    /*
    ** Get a new block from ME tagged with the specified tag.
    ** The size of the new block is the standard size (if the request
    ** will fit) or is exactly as large as necessary to satisfy the
    ** request (if the request will not fit).
    **
    ** The requested size has already been adjusted for alignment, however,
    ** round the header size up to ensure that 'fe_freeptr' will be aligned too.
    */

    hdr_size = SIZE_ADJ( sizeof(*block) );
    mereqsize = ( size > FE_allocsize - hdr_size )
			? (size + hdr_size) : FE_allocsize;

    if ((block = (FE_BLK *)MEreqmem((u_i2)tag, mereqsize, FALSE, status)) != NULL)
    { /* Initialize the FE_BLK header */
	block->fe_bytfree = mereqsize - hdr_size;
	block->fe_freeptr = (PTR)((char *)block + hdr_size);
	block->fe_next = NULL;
    }

#ifdef	FE_ALLOC_DBGLIST
    SIfprintf(FE_dbgfile,
	ERx("\n\tNewalloc:  Address = %p  Size = %x\n\t\tFreeptr = %p  Bytes free = %x\n"),
		block, mereqsize, block->fe_freeptr, block->fe_bytfree
    );
#endif

#ifdef	FE_ALLOC_DBGSUM
    ++FE_totblk;
    FE_totbyt = FE_totbyt + mereqsize;
    SIfprintf(FE_dbgfile,
	ERx("----- Newalloc -----\n\tTotal blocks allocated = %x   Total bytes allocated = %x\n"),
	FE_totblk, FE_totbyt
    );
#endif

# ifdef DEBUGPRINT
    SIprintf("New block for tag %d: %d bytes at %x-%x\n", tag, mereqsize, (i4)block, (i4)block + mereqsize - 1);
# endif
    return block;
}

#ifdef	FE_ALLOC_DBG
static
fe_dbg2_trace (tag)
TAGID	tag;
{
    register FE_BLK	*feblk;
    register i4	unused;
		
    extern FILE	*FE_dbgfile;

    feblk = find_hash_entry(tag)->block;

#ifdef	FE_ALLOC_DBGLIST
    SIfprintf(FE_dbgfile, ERx("\n\tdoreqmem:  iiUGtags[%d] = %p\n"),index,feblk);
#endif

#ifdef	FE_ALLOC_DBGSUM
    unused = 0;
#endif

    while (feblk != NULL)
    {
#ifdef	FE_ALLOC_DBGLIST
	SIfprintf(FE_dbgfile, ERx("\t\tNODE %p:\n"), feblk);
	SIfprintf(FE_dbgfile,
		ERx("\t\tFree bytes = %x   Freeptr = %p   Nextptr = %p\n"),
		feblk->fe_bytfree, feblk->fe_freeptr, feblk->fe_next
	);
#endif

#ifdef	FE_ALLOC_DBGSUM
	unused += feblk->fe_bytfree;
# endif
	feblk = feblk->fe_next;
    } /* end while */

#ifdef	FE_ALLOC_DBGSUM
    SIfprintf(FE_dbgfile, ERx("\t\tTag %x:  unused bytes = %x\n"), tag, unused);
#endif
    return;
}
#endif

