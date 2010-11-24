/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<me.h>
# include	<meprivate.h>
# include	<qu.h>
# define STATIC	/* are my things visible?  Null def or static */

/*
**	MEconsist.c - check consistency of allocator arena
**
**	This is an auditor for MEreqmem and friends.  It is UNDOCUMENTED
**	and UNSUPPORTED.  If it won't compile, just delete it.
**
**	Dave Brower, 1/28/85
**
**	The main entry point is 'MEconsist(),' at the end of the file.
**
**	History:
**	22-Dec-1986 (daveb)
**	Updated for 5.0 allocator rewrite.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**      23-may-90 (blaise)
**          Add "include <compat.h>."
**	20-nov-92 (pearl)
**	    Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**	    headers.
**	05-feb-92 (smc)
**	    Cast return of sbrk(0) to same type as memend.
**      08-feb-1993 (pearl)
**          Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in checklist(),
**	    and mergecheck().
**	28-oct-93 (swm)
**	    Bug #56448
**	    Added comments to guard against future use of a non-varargs
**	    function in MEprintf function pointer.
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	14-sep-1995 (sweeney)
**	    WECOV purge. qsort is void everywhere, and should be
**	    picked up from <stdlib.h> anyhow.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-nov-2010 (stephenb)
**	    Proto forward funcs.
*/

extern ME_HEAD	MElist;
extern ME_HEAD	MEfreelist;


/* function to use as a printf for error messages, the function pointer
   assigned to it must be for a VARARGS function */
static i4	(*MEprintf)();
/*
** Forward references
*/
STATIC i4
blkcmp(
	const void *a,
	const void  *b);
STATIC STATUS
checklist(
	ME_HEAD	*list,
	i4	sortp);
STATIC ME_NODE *
mem_next(
	ME_NODE * this);
STATIC STATUS
sortlist(
	ME_HEAD	* list);
STATIC STATUS
mergecheck(
	VOID);

extern char end;
STATIC char	*memstart = &end;
STATIC char	*memend;

# define SORTED		TRUE
# define NOT_SORTED	FALSE

# define NOLIST		0
# define ALIST		1
# define FLIST		2

/*
**	Max blocks supported
*/

# define MAX_ME_NODES	(1024 * 16)

/*
**	Return what should be the next node, based on the size
**	officially at this node.
**
**	Note that the size_rs field already contains the header size.
*/
STATIC ME_NODE *
mem_next(
	ME_NODE * this)
{
    return( (ME_NODE *) ((char *)this + this->MEsize ) );
}



/*
** check list for obvious corruption.
*/
STATIC STATUS
checklist(
	ME_HEAD	*list,
	i4	sortp)
{
register i4		status = 0;
register ME_NODE	*start;
register ME_NODE	*this;
register i4		cnt = 0;
ME_NODE			*prev;
char			*which;


    which = list == &MElist ? "Allocated" : "Free";

    start = (ME_NODE *)list;
    this = list->MEfirst;
    prev = NULL;
    while( this != start )
	{
		if( this < (ME_NODE *)memstart )
		{
			/* points before heap should start */			
	    (*MEprintf)("%s List element 0x%p before memstart 0x%p\n",
		    which, this, memstart);
			status |= FAIL;
	    break;
		}

		if( this > (ME_NODE *)memend )
		{
			/* points beyond heap */
	    (*MEprintf)("%s List element 0x%p is after memory end 0x%p\n",
		    which, this, memend);
			status |= FAIL;
	    break;
		}

		/* Now we know the this is in legal space */

	if( sortp && this->MEprev > this )
		{
	    /* not sorted correctly */
	    (*MEprintf)("%s List sorted wrong. 0x%p is before 0x%p\n",
		    which, this, this->MEprev );
			status |= FAIL;
			break;
		}

		cnt++;
		prev = this;
	this = this->MEnext;
	}

    /*
     * If last is not pointing at prev, or it's not an empty list,
     * then it's corrupted
     */
    if( prev && prev != list->MElast )
	{
		/* last is corrupted */
	(*MEprintf)("%s List ends at block 0x%p, not block in hdr 0x%p\n",
		which, prev, list->MElast);
		status |= FAIL;
	}

	return (status);
}


STATIC i4
blkcmp(
	const void *a,
	const void  *b)
{
    /* avoid arthmetic range problems */
    return ( ( a < b ) ? -1 : ( ( a > b ) ? 1 : 0 ) );
}

STATIC ME_NODE *MEsortidx[ MAX_ME_NODES ];

STATIC STATUS
sortlist(
	ME_HEAD	* list)
{
    register ME_NODE **ip;			/* index pointer */
    register ME_NODE *blk;			/* current element */
    register i4  i;
	register i4  cnt;


	/* build index array */
	
	cnt = 0;
    ip = MEsortidx;
    for( blk = list->MEfirst; blk != (ME_NODE *)list; blk = blk->MEnext )
	{
		*ip++ = blk;
		if( ++cnt > MAX_ME_NODES )
		{
			/* acckkk, pfffffttt!  Too big, give up. */
	    (*MEprintf)("Sort ran out of %d nodes\n", MAX_ME_NODES );
			return FAIL;
		}
	}

    if( cnt < 2 )
		return OK;

    /* sort the MEsortidx array by address of ptr */
    (void) qsort( MEsortidx, cnt, sizeof( ME_NODE* ), blkcmp );
	
    /* rebuild the list */
    QUinit( (QUEUE *)list );
    for( i = 0; i < cnt ; i++ )
	(void)QUinsert( (QUEUE *)MEsortidx[ i ], (QUEUE *)list->MElast );

	return OK;
}


/*
**	Check for lost memory and overlaps.  Lists are sorted
**	and already checked for obvious corruption
*/

STATIC STATUS
mergecheck(
	VOID)
{
	register i4  status = 0;

	register ME_NODE * prev;
	register i4  prevlist;

	register ME_NODE * this;
	register i4  thislist;

	register ME_NODE * should;

    register ME_NODE * ablk = MElist.MEfirst;
    register ME_NODE * fblk = MEfreelist.MEfirst;

	this = NULL;
	thislist = NOLIST;
	do
	{
		prev = this;
		prevlist = thislist;

		/* figure the next block from the chain addresses */

	if( ablk < fblk )	/* next block is from the allocated list */
		{
			this = ablk;
			thislist = ALIST;
	    ablk = ablk->MEnext;

	    /* no alist */
	    if( this == MElist.MEfirst )
		continue;
				}
		else if ( ablk == fblk )
		{
	    /* no list at all, all is OK, so quit */
	    if ( this == MElist.MEfirst || this == MElist.MEfirst )
		    break;

			(*MEprintf)("Block 0x%p in both lists!, I give up!\n", ablk );
			return( status | FAIL );
		}
	else	/* ablk > fblk, block is from the free list */
		{
	    /* shouldn't happen, so quit from loop */
	    if ( this == (ME_NODE *)&MElist || this == (ME_NODE *)&MEfreelist
	        || fblk == (ME_NODE *)&MEfreelist )
		    break;

			this = fblk;
	    fblk = fblk->MEnext;
			thislist = FLIST;

	    /*
	    ** non-coalesced freelist blocks
	    */
	    if( prevlist == FLIST && (ME_NODE *)mem_next(prev) == this )
	    {
		(*MEprintf)(
		"Bad coalesce: prev 0x%p + %d == MEnext 0x%p, this 0x%p=\n",
			prev, prev->MEsize, prev->MEnext, this );
		status |= FAIL;
		}
	}

	/*
	** Have this block, ablk and fblk point to next in each chain or null.
	*/

		if( prevlist == NOLIST && this != (ME_NODE *)memstart )
		{
			/* corrupt heads; one should be at beg of heap */
			(*MEprintf)("Neither head is at the memstart\n");
			status |= FAIL;
		}

	if( prevlist != NOLIST )
	{
		should = mem_next( prev );
		if( should < this )
		{
			/*
			** Lost memory:  there is hole between
			** the previous element and this one.
			*/
		(*MEprintf)(
		"Lost memory:  prev 0x%p ends at 0x%p, should be at 0x%p\n",
			prev, should, this
		);
		}
		else if ( should == this )
		{
			/* everything is ducky. */
		}
	    else if( should > this && this != (ME_NODE *)&MElist &&
	        this != (ME_NODE *)&MEfreelist )
		{
			/*
			** Overlap between prev and this
			*/

			if( prevlist == thislist )
			{

				/* corrupt single list */		
			(*MEprintf)(
  "List corrupt:  blocks 0x%p and 0x%p in the %s list overlap.\n",
				prev, this,
				((prevlist == ALIST) ? "allocated" : "free")
			);
			}
			else
			{
				/* both lists corrupt */
			(*MEprintf)(
  "Lists corrupt:  block 0x%p in the %s list overlaps 0x%p in the %s list\n",
			    prev, prevlist == ALIST ? "allocated" : "free",
			    this, thislist == ALIST ? "allocated" : "free"
			);
			}
		(*MEprintf)(
		"block 0x%p, size 0x%x, next 0x%p, should be 0x%p\n",
			prev, prev->MEsize, prev->MEnext, mem_next( prev ) );
		(*MEprintf)(
		"block 0x%p, size 0x%x, next 0x%p, should be 0x%pv\n",
			this, this->MEsize, this->MEnext, mem_next( this ) );
			status |= FAIL;

	    }	/* should > this */

	}	/* prev valid */

    } while( this < (ME_NODE *)memend
		&& this != (ME_NODE *)&MElist
		&& this != (ME_NODE *)&MEfreelist );

	return (status);
}


STATUS
MEconsist(
	i4 (*printf)())
{
	register i4  status = 0;
	register i4  rval;

	MEprintf = printf;	/* MEprintf must be a VARARGS function */
	memend = (char *)sbrk(0);

	/* check for bad pid, obvious inconsistency */
    if ( OK != checklist( &MElist, NOT_SORTED ) )
    {
	status |= FAIL;
	MExdump( ME_D_ALLOC, MEprintf );
    }
    if( OK != checklist( &MEfreelist, SORTED ) )
    {
	status |= FAIL;
	MExdump( ME_D_FREE, MEprintf );
    }

	/* get alloc list sorted for merge check */

    /* this can be very slow... */
	if ( status || (rval = sortlist( &MElist ) ) ) 
	{
		(*MEprintf)("Can't sort alist, skipping merge check\n");
/*	    status |= rval; */
    }
    if( status || ( rval = mergecheck() ) != OK )
    {
	    MExdump( ME_D_BOTH, MEprintf );
		status |= rval;
	}
	return (status);

}
