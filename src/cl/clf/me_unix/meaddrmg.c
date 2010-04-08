/*
** Copyright (c) 1989, 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <meprivate.h>
#include    <me.h>
#include    <st.h>

/**
**
**  Name: MEADDRMG.C - routines to manage memory bit map of allocations
**
**  Description:
**	routines to manage memory bit map of allocations.
**
**	    MEisalloc(pageno, pages, isalloc)
**	    MEsetalloc(pageno, pages)
**	    ME_reg_seg(addr, segid, pages)
**	    ME_find_seg(addr, eaddr, segq)
**	    ME_rem_seg(seginf)
**	    MEfindpages(pages,pageno)
**	    MEsetpg(alloctab, pageno, pages)
**	    MEclearpg(alloctab, pageno, pages)
**	    MEalloctst(alloctab, pageno, pages, allalloc)
**	    ME_offset_to_addr(seg_key,offset,addr)
**
**
**  History:
**	14-feb-1989 (mikem)
**	    fixed problem that this routine would not allocate enough space
**	    for the alloctab field of the ME_SEG_INFO structure.  It only 
**	    allocated enough space for 32 pages (ie. 32 bits), thus causing
**	    memory trashing bugs when shared memory segments bigger than
**	    32 pages (ie. 32pages * 8192 bytes per page = 256K), were allocated
**	    for the locking/logging segment.
**	 7-jul-1989 (rogerk)
**	    Added ME_rem_seg routine to take segment info blocks out of the
**	    pool list.
**	23-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <compat.h>";
**		Add fredv's fix:
**        The result of (char & i4) is unpredictable.
**            On the machines that treat char as unsigned one byte integer,
**            the sign bit of the char is PROPAGATED; therefore, the result
**            is equivalent to (0x000000[content of char] & i4) and the three
**            high order bytes of i4 are masked off.
**            On those machines that propagate the sign bit, the result is
**            equevalent to (0xFFFFFFFF & char & i4).
**            Thus, the logical expression
**                    (char & i4) != i4 , where char=0xFF;
**            will have unpredictable truth value when i4 is greater than
**            0x000000FF, depends on the characteristic of the machine.
**        Put in a fix that works in both cases.
**	2-aug-1991 (bryanp)
**	    Added "key" and "flags" arguments to ME_reg_seg, and added new
**	    routine ME_offset_to_addr() as part of adding support for DI
**	    slave I/O directly to/from shared memory.
**      21-feb-1992 (jnash)
**          In ME_offset_to_addr, change segid to seg_key to make it 
**	    work with MMAP.
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.  Delete forward and external function references;
**		these are now all in either me.h or meprivate.h.
**      08-feb-1993 (pearl)
**          Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	20-jun-93 (vijay) 22-Jan-1992 (fredv)
**	  Integrate from ingres63p.
**        Fixed RS6000 specific bug #34639.
**        The bug was caused by the default of unsigned char on the
**        RS6000. The fix is essentially to extend my fix as explained
**        above.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      11-aug-93 (ed)
**          added missing includes
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in ME_reg_seg().
**      15-may-1995 (thoro01)
**          Added NO_OPTIM  hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	21-apr-1997 (canor01)
**	    Use STlcopy instead of MEcopy to fill in they "key" field of
**	    the ME_SEG_INFO structure to quiet warnings from Purify.
**	29-oct-1997 (canor01)
**	    Do not cast &ME_segpool directly to a ME_SEG_INFO structure.
**	    It does not contain valid info and is just used as an anchor.
**	11-nov-1997 (canor01)
**	    Fix typo in previous submission that can potentially cause
**	    bus errors.
**      24-Nov-1997 (allmi01)
**          Added #ifdef's for Silicon Graphics to cast ME_SEG_INFO *
**          pointers passed to RQUinsert into QUEUE * pointers.
**          This is required to quiet compiler errors on sgi_us5.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**      24-May-2000 (hanal04) Bug 98820 INGSRV 1094
**          Updated function comments to identify functions which require
**          the caller to hold the MEalloctab_mutex.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	25-oct-2001 (somsa01)
**	    The return value of MEsetalloc() is void.
**      16-jan-2003 (horda03) Bug 111658/INGSRV2680
**          Remove the 2GB limit for internal memory pages. Rather than
**          allocating the MEALLOCTAB for all possible pages, a new
**          MEALLOCTAB structure/bitmap will be added whenever there is
**          insufficient contiguous free pages in the existing MEALLOCTABs.
**          The only limit on memory that can be allocated to the server
**          now, will be determined by OS restriction.
**          Had to modify the way some procedures work, as SIZE_TYPE is an
**          unsigned quantity, hence the concept of a value <0 doesn't exist.
**	02-sep-2004 (thaju02)
**	    Use SIZE_TYPE for pageno, pages.
**	17-Nov-2004 (bonro01)
**	    Fix bug caused by last update.  Changing variables from a 
**	    signed i4 to an unsigned SIZE_TYPE caused a problem in the
**	    bitmap allocation routines because a calcuation which normally
**	    would have returned a negative number and caused the routine
**	    to exit was now returning a HUGE positive value.
**    18-Jun-2005 (hanje04)
**        Replace GLOBALDEF with GLOBALDEF for MEbase to stop
**        compiler error on MAC OSX.

*/


/*
**  Forward and/or External typedef/struct references.
*/
GLOBALREF	i2		MEconttab[];
GLOBALREF	MEALLOCTAB	MEalloctab;
GLOBALREF       char    	*MEbase;
GLOBALREF	SIZE_TYPE	MEendtab;


/*
**  Defines of other constants.
*/


/*
** Definition of all global variables owned by this file.
*/

GLOBALDEF	QUEUE	ME_segpool;


/*
**  Definition of static variables and forward static functions.
*/

/* check if a page in the break region is allocated */
/*      24-May-2000 (hanal04) Bug 98820 INGSRV 1094
**          Caller must hold the MEalloctab_mutex.
**
** horda03 - Now select the appropriate MEalloctab for the
**           specified page number;
*/
bool
MEisalloc(
	SIZE_TYPE	pageno,
	SIZE_TYPE	pages,
	bool		isalloc)
{
    MEALLOCTAB *alloctab;
    SIZE_TYPE  diff;

    for (alloctab = &MEalloctab; alloctab; alloctab = alloctab->next)
    {
       if (pageno >= (SIZE_TYPE) ME_MAX_ALLOC)
       {
          pageno -= ME_MAX_ALLOC;

          continue;
       }

       if ( pageno + pages <= (SIZE_TYPE) ME_MAX_ALLOC)
       {
          return(MEalloctst(alloctab->alloctab, pageno, pages, isalloc));
       }
       else
       {
          diff = ME_MAX_ALLOC - pageno;

          if (!MEalloctst(alloctab->alloctab, pageno, diff, isalloc))
          {
             return FALSE;
          }

          pageno = 0;
          pages -= diff;
       }
   }

   TRdisplay( "MEisalloc - testing pages which aren't allocated\n");

   return !isalloc;
}

/* mark some pages within the break as allocated */
/*      24-May-2000 (hanal04) Bug 98820 INGSRV 1094
**          Caller must hold the MEalloctab_mutex.
*/
void
MEsetalloc(
	SIZE_TYPE	pageno,
	SIZE_TYPE	pages)
{
    MEALLOCTAB *alloctab;
    SIZE_TYPE  diff;

    for (alloctab = &MEalloctab; alloctab; alloctab = alloctab->next)
    {
       if (pageno >= (SIZE_TYPE) ME_MAX_ALLOC)
       {
          pageno -= ME_MAX_ALLOC;

          continue;
       }

       if ( pageno + pages <= (SIZE_TYPE) ME_MAX_ALLOC)
       {
          MEsetpg(alloctab->alloctab, pageno, pages);

	  return;
       }
       else
       {
          diff = ME_MAX_ALLOC - pageno;

          MEsetpg(alloctab->alloctab, pageno, diff);

          pageno = 0;
          pages -= diff;
       }
   }

   TRdisplay( "MEsetalloc - setting pages which aren't allocated\n");

   return;
}

/* mark some pages within the break as unallocated */
/*      24-May-2000 (hanal04) Bug 98820 INGSRV 1094
**          Caller must hold the MEalloctab_mutex.
*/
void
MEclearalloc(
	SIZE_TYPE	pageno,
	SIZE_TYPE	pages)
{
    MEALLOCTAB *alloctab;
    SIZE_TYPE  diff;

    for (alloctab = &MEalloctab; alloctab; alloctab = alloctab->next)
    {
       if (pageno >= (SIZE_TYPE) ME_MAX_ALLOC)
       {
          pageno -= ME_MAX_ALLOC;

          continue;
       }

       if ( pageno + pages <= (SIZE_TYPE) ME_MAX_ALLOC)
       {
          MEclearpg(alloctab->alloctab, pageno, pages);

	  return;
       }
       else
       {
          diff = ME_MAX_ALLOC - pageno;

          MEclearpg(alloctab->alloctab, pageno, diff);

          pageno = 0;
          pages -= diff;
       }
   }

   TRdisplay( "MEclearalloc - clearing pages which aren't allocated\n");

   return;
}

/*{
** Name: ME_reg_seg()	- allocate a data structure to keep track of memory segs
**
** Description:
**	Routine allocates a ME_SEG_INFO structure to hold information about
**	the input memory segment.  The structure is inserted in "address" order
**	in a queue of ME_SEG_INFO structures.  The purpose of the data 
**	structure is to manipulate sparse memory bitmaps for discrete memory 
**	segments allocated outside of the brk() region.
**
**	Secondarily, DI uses this information to determine whether an
**	arbitrary address is in mapped shared memory, in order to decide
**	whether a slave process can perform an I/O operation directly into
**	the shared memory.
**
** Inputs:
**	addr				beginning address of memory segment.
**	segid				id passed to MEget_pages() for segment.
**	pages				number of pages in the segment.
**	key				segment key
**	flags				flags passed to MEget_pages()
**
** Outputs:
**	none.
**
**	Returns:
**	    OK				success
**	    not OK			MEreqmem() failure returns
**
** History:
**      12-feb-89 (mikem)
**	    fixed problem that this routine would not allocate enough space
**	    for the alloctab field of the ME_SEG_INFO structure.  It only 
**	    allocated enough space for 32 pages (ie. 32 bits), thus causing
**	    memory trashing bugs when shared memory segments bigger than
**	    32 pages (ie. 32pages * 8192 bytes per page = 256K), were allocated
**	    for the locking/logging segment.
**	2-aug-1991 (bryanp)
**	    Added "key" and "flags" arguments and used them to fill in the
**	    ME_SEG_INFO structure.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	21-apr-1997 (canor01)
**	    Use STlcopy instead of MEcopy to fill in they "key" field of
**	    the ME_SEG_INFO structure to quiet warnings from Purify.
**	29-oct-1997 (canor01)
**	    Do not cast &ME_segpool directly to a ME_SEG_INFO structure.
**	    It does not contain valid info and is just used as an anchor.
*/
STATUS
ME_reg_seg(
	PTR	addr,
	ME_SEGID segid,
	SIZE_TYPE	pages,
	char	*key,
	i4	flags)
{
    STATUS			status = OK;
    register ME_SEG_INFO	*seginf, *q;
    SIZE_TYPE			size_of_seginfo;

    /* a ME_SEG_INFO has a dynamically allocated allocation table at the end of
    ** it (allocvec).  This allocation ME_SEG_INFO must allocate the size of
    ** the structure plus tack onto the end one bit for every page in the
    ** shared memory segment being allocated.
    */

    size_of_seginfo = (sizeof(*seginf) - sizeof(seginf->allocvec)) + 
		      (sizeof(char) * ((pages / BITSPERBYTE) + 1));

    seginf = (ME_SEG_INFO *)MEreqmem(0, size_of_seginfo, TRUE, &status);
    if (status)
    {
	return(status);
    }

#ifdef xDEBUG
    TRdisplay("ME_reg_seg: %d @ %p, size %d pages, key=%s, flags=%x\n",
		segid, addr, pages, key, flags);
#endif

    seginf->segid = segid;
    seginf->npages = pages;
    seginf->addr = (char *)addr;
    seginf->eaddr = pages * ME_MPAGESIZE + (char *)addr;
    seginf->flags = flags;
    STncpy( seginf->key, key, sizeof(seginf->key) - 1 );
    seginf->key[ sizeof(seginf->key) - 1 ] = EOS;

    if (status)
    {
	return(status);
    }

    MEsetpg(seginf->allocvec, 0, pages);

    /* need to insert in sorted order */
    for (q = (ME_SEG_INFO *)ME_segpool.q_prev;
	 (QUEUE *)q != &ME_segpool;
	 q = (ME_SEG_INFO *)q->q.q_prev)
    {
	if (q->addr < (char *)addr)
	{
	    break;
	}
    }
    QUinsert((QUEUE*)(seginf), (QUEUE *)(q));
    return(OK);
}

ME_SEG_INFO *
ME_find_seg(
	char	*addr,
	char	*eaddr,
	QUEUE	*segq)
{
    register ME_SEG_INFO	*segp;

    for (segp = (ME_SEG_INFO *)segq->q_prev;
	 (QUEUE *)segp != &ME_segpool;
	 segp = (ME_SEG_INFO *)segp->q.q_prev)
    {
	if (eaddr > segp->addr && addr < segp->eaddr)
	    return(segp);
    }
    return(NULL);
}

QUEUE *
ME_rem_seg(
	ME_SEG_INFO	*seginf)
{
    QUEUE	*next_q;

    /* Save pointer to next entry - this is returned to caller. */
    next_q = seginf->q.q_next;

    /* Remove this entry from queue and deallocate the memory. */
    (VOID) QUremove(&seginf->q);
    (VOID) MEfree((PTR)seginf);

    return (next_q);
}

/*
** Name: ME_offset_to_addr()	- convert a (seg_key,offset) pair to an addr
**
** Description:
**	The DI slave processes call this routine to convert a (seg_key,offset)
**	pair to an address. When DI slaves are asked to perform I/O directly
**	to or from shared memory, the actual shared memory address is passed
**	as a (seg_key,offset) pair and the slave must convert it back to a
**	process-specific address.
**
** Inputs:
**	seg_key			- shared memory segment ID
**	offset			- offset into the indicated segment
**
** Outputs:
**	addr			- the resulting address
**
** Returns:
**	OK			- address converted successfully
**	anything else		- no such segment, or bad offset, or...
**
** History:
**	2-aug-1991 (bryanp)
**	    Created.
**	21-feb-1992 (jnash)
**	    Change segid to seg_key to make it work with MMAP.
*/
STATUS
ME_offset_to_addr(
	char		*seg_key,
	SIZE_TYPE	offset,
	PTR		*addr)
{
    ME_SEG_INFO     *seginfo;
    bool            found = FALSE;


    for (seginfo = (ME_SEG_INFO *)ME_segpool.q_prev;
            (QUEUE *)seginfo != &ME_segpool;
            seginfo = (ME_SEG_INFO *)seginfo->q.q_prev)
    {
        if (STcompare((char *)seginfo->key, seg_key) == 0)
        {
            found = TRUE;
            break;
        }
    }
    if (found)
    {
        *addr = (PTR)((char *)seginfo->addr + offset);
        return (OK);
    }
    else
    {
        return (FAIL);
    }

}

/*{
** Name: MEfindpages()	- find some free pages
**
** Description:
**	Search the allocation bitmap for a length of clear bits.
**	This will match up with what pages of memory are free for
**	allocation.
**
** Inputs:
**	pages				number of pages needed
**
** Outputs:
**	pageno                          page number of first page of
**                                      free region
**
**	Returns:
**          TRUE			free region found.
**          FALSE			no free region.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-feb-88 (anton)
**          Created.
**      24-May-2000 (hanal04) Bug 98820 INGSRV 1094
**          Caller must hold the MEalloctab_mutex.
*/
bool
MEfindpages(
	SIZE_TYPE	pages,
        SIZE_TYPE       *pageno)
{
    register char	*bp, *ep;
    register SIZE_TYPE	maxcont;
    register i4         cont;
    i4			mask;
    MEALLOCTAB  *alloc_tab;
    static   i4 recurse = 0;
    i4          cnt = 0;
    STATUS      res;
    
    maxcont = (SIZE_TYPE) 0;
    for(alloc_tab=&MEalloctab; alloc_tab; alloc_tab=alloc_tab->next, cnt++)
    {
       for (bp = alloc_tab->alloctab, ep = bp + ME_MAX_ALLOC / ME_BITS_PER_BYTE;
	    bp < ep; ++bp)
       {
	   cont = MEconttab[(*bp) & 0xff];
	   if (cont & ME_ALLSET)
	   {
	       maxcont = (SIZE_TYPE) 0;
	       continue;
	   }
	   if (cont & ME_ALLCLR)
	   {
	       maxcont += (SIZE_TYPE) 8;
	   }
	   else
	   {
	       maxcont += ME_LOWCNT(cont);
	   }
	   if (maxcont >= pages)
	   {
	       *pageno = ((bp - alloc_tab->alloctab) * 8) + (ME_MAX_ALLOC * cnt);
	       if (cont & ME_ALLCLR)
	       {
		   *pageno += 8;
	       }
	       else
	       {
		   *pageno += ME_LOWCNT(cont);
	       }
	       *pageno -= maxcont;
	       return(TRUE);
	   }
	   if (cont & ME_ALLCLR)
	   {
	       continue;
	   }
	   if ((SIZE_TYPE) ME_MAXCNT(cont) >= pages)
	   {
	       mask = (1 << pages) - 1;
	       for (*pageno = 0; *pageno+pages <= (SIZE_TYPE) 8; (*pageno)++)
	       {
		   if (!(*bp & (mask << *pageno)))
		   {
		       *pageno += ((bp - alloc_tab->alloctab) * 8) + (ME_MAX_ALLOC * cnt);
		       return(TRUE);
		   }
	       }
	       /* this should never happen */
	       return(-1);
	   }
	   maxcont = ME_UPCNT(cont);
       }

       /* To reach here means there isn't enough contiguous pages in the
       ** list of allocated pages, so allocate another set of mapped memory
       ** pages.
       */
       if (!alloc_tab->next)
       {
	  /* Need another MEALLOCTAB. MEnewalloctab() will use MEfindpages() to
	  ** locate pages where the nee MEALLOCTAB will reside. If there aren't
	  ** enough pages in the current MEALLOCTABs, then an infinite loop
	  ** could occur, with MEfindpages() calling MEnewalloctab calling
	  ** MEfindpages etc., to prevent this, use local static recurse. Only
	  ** one thread at a time can call MEfindpages().
	  */

          if (recurse) break;

          recurse++;
          res = MEnewalloctab(alloc_tab);
          recurse--;

	  if (res) break;
       }
    }

    return(FALSE);
}

/*{
** Name: MEsetpg()	- Mark pages as allocated
**
** Inputs:
**	alloctab			allocation bitmap table
**      pageno				first page to be marked
**	pages				number of pages to mark
**
** Outputs:
**	none
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-feb-88 (anton)
**          Created.
*/
VOID
MEsetpg(
	char	*alloctab,
	SIZE_TYPE	pageno,
	SIZE_TYPE	 pages)
{
    register SIZE_TYPE	i, b;
    register i4	m;
    
    i = ME_BYTE_FROM_PAGE(pageno);
    m = -1;
    if (b = ME_BIT_FROM_PAGE(pageno))
    {
	if (pages < (SIZE_TYPE) ME_BITS_PER_BYTE)
	{
	    m = (1 << pages) - 1;
	}
	alloctab[i] |= m << b;
	if (pages <= ME_BITS_PER_BYTE - b) return;

	pages -= ME_BITS_PER_BYTE - b;
	++i;
    }
    m = -1;
    while (pages >= (SIZE_TYPE) ME_BITS_PER_BYTE)
    {
	alloctab[i] |= m;
	pages -= ME_BITS_PER_BYTE;
	++i;
    }

    if (pages)
    {
	m = (1 << pages) - 1;
	alloctab[i] |= m;
    }
}

/*{
** Name: MEclearpg()	- Mark pages as free
**
** Inputs:
**	alloctab			allocation bitmap table
**      pageno				first page to be marked
**	pages				number of pages to mark
**
** Outputs:
**	none
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-feb-88 (anton)
**          Created.
*/
VOID
MEclearpg(
	char		*alloctab,
	SIZE_TYPE	pageno,
	SIZE_TYPE	pages)
{
    register SIZE_TYPE	i, b;
    register i4	m;
    
    i = ME_BYTE_FROM_PAGE(pageno);
    m = -1;
    if (b = ME_BIT_FROM_PAGE(pageno))
    {
	if (pages < (SIZE_TYPE) ME_BITS_PER_BYTE)
	{
	    m = (1 << pages) - 1;
	}
	alloctab[i] &= ~(m << b);

        if (pages <= ME_BITS_PER_BYTE - b) return;

	pages -= ME_BITS_PER_BYTE - b;
	++i;
    }
    m = -1;
    while (pages >= (SIZE_TYPE) ME_BITS_PER_BYTE)
    {
	alloctab[i] &= ~m;
	pages -= ME_BITS_PER_BYTE;
	++i;
    }
    if (pages)
    {
        m = (1 << pages) - 1;
	alloctab[i] &= ~m;
    }
}

/*{
** Name: MEalloctst()	- Check pages for allocation status
**
** Inputs:
**	alloctab			allocation bitmap table
**      pageno				first page to be checked
**	pages				number of pages to check
**	allalloc			check for allocated
**			if TRUE check for all allocated
**			if FALSE check for all NOT allocated
**
** Outputs:
**	none
**
**	Returns:
**	    TRUE			allocation condition not met
**	    FALSE			condition met
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-feb-88 (anton)
**          Created.
**	20-jun-93 (vijay) 22-Jan-1992 (fredv)
**	  Integrate from ingres63p.
**        Fixed RS6000 specific bug #34639.
**        The bug was caused by the default of unsigned char on the
**        RS6000. The fix is essentially to extend my fix as explained
**        above.
*/
bool
MEalloctst(
	char		*alloctab,
	SIZE_TYPE	pageno,
	SIZE_TYPE	pages,
	bool		allalloc)
{
    register SIZE_TYPE	i, b;
    register i4		m;
    register i4         tmptab;
    
    i = ME_BYTE_FROM_PAGE(pageno);
    if (b = ME_BIT_FROM_PAGE(pageno))
    {
	m = -1;
	if (pages < (SIZE_TYPE) ME_BITS_PER_BYTE)
	{
	    m = (1 << pages) - 1;
	}
	m <<= b;
	tmptab = (alloctab[i] & 0x80 ? alloctab[i] | 0xFFFFFF00 : alloctab[i]);
        if ((tmptab & m) != (allalloc ? m : 0))
	    return(TRUE);

        if (pages <= ME_BITS_PER_BYTE - b)
	{
	   /* All required bits checked and match the alloc test */
	   
	   return FALSE;
        }

	pages -= ME_BITS_PER_BYTE - b;
	++i;
    }
    m = -1;
    while (pages >= (SIZE_TYPE) ME_BITS_PER_BYTE)
    {
	/* Fix bug 34639 */
	tmptab = (alloctab[i] & 0x80 ? alloctab[i] | 0xFFFFFF00 : alloctab[i]);
	if ((tmptab & m) != (allalloc ? m : 0))
	{
	    return(TRUE);
	}
	pages -= ME_BITS_PER_BYTE;
	++i;
    }

    if (pages)
    {
        m = (1 << pages) - 1;
	tmptab = (alloctab[i] & 0x80 ? alloctab[i] | 0xFFFFFF00 : alloctab[i]);
	if ((tmptab & m) != (allalloc ? m : 0))
	{
	    return(TRUE);
        }
    }
    return(FALSE);
}


/*{
** Name: MEnewalloctab()	- Allocate a new MEALLOCTAB
**
**      Find a memory location which can hold a MEALLOCTAB
**      structure. If there are insufficent contiguous free
**      pages in allocated memory, then the 
** Inputs:
**	alloc_tab		Pointer to the last allocated MEALLOCTAB
**
** Outputs:
**	none
**
**	Returns:
**	    OK			Memory allocated
**	    ME_OUT_OF_MEM       Failed to allocate MEALLOCTAB
**
**	Exceptions:
**	    none
**
** Side Effects:
**	New MEALLOCTAB appended to end of MEalloctab link list.
**      Server memory allocation may increase.
**
** History:
**      26-jan-2004 (horda03) Bug 111658/INGSRV2680
**          Created.
*/
STATUS
MEnewalloctab(alloctab)
MEALLOCTAB *alloctab;
{
   bool       found;
   i4        tabsize;
   SIZE_TYPE page_no;
   SIZE_TYPE  pages;
   MEALLOCTAB *new_alloc;

   /* Find pages to store the new MEALLOCTAB */

   tabsize = ME_MAX_ALLOC / ME_BITS_PER_BYTE;
   pages   = (tabsize + sizeof( MEALLOCTAB ) + ME_MPAGESIZE - 1) / ME_MPAGESIZE;

   if ( found = MEfindpages( pages, &page_no ))
   {
      if ( page_no + pages > MEendtab)
      {
         /* The required pages for the MEALLOCTAB structure and bit map extend 
         ** beyond the current extent of allocated pages. As the server's memory
         ** may have been extended outside of ME control, add the required memory
         ** to the end of the current memory allocation.
         */
         TRdisplay( "Adding new ALLOCTAB in memory not yet allocated (Page %p/%p)\n",
		    page_no, MEendtab);
         found = FALSE;
      }
   }


   if (!found)
   {
      /* No page available in the current list of pages, so extend the
      ** server memory.
      */
      TRdisplay( "Adding new ALLOCTAB in fresh memory\n");

      new_alloc = (MEALLOCTAB *) sbrk((i4)sizeof(MEALLOCTAB) + tabsize);

      if (new_alloc == (MEALLOCTAB *) (SIZE_TYPE) -1) return ME_OUT_OF_MEM;
   }
   else
   {
     TRdisplay( "Adding new ALLOCTAB in to Page %d\n", page_no);

     /* Indicate pages in use */

     MEsetalloc( page_no, pages );

     new_alloc = (MEALLOCTAB *) (MEbase + (SIZE_TYPE)(ME_MPAGESIZE * page_no));
   }

   MEfill( tabsize + sizeof(MEALLOCTAB), '\0', new_alloc);

   new_alloc->alloctab = (char *) (new_alloc) + sizeof(MEALLOCTAB);

   
   /* Add new_alloc to end of MEalloctab link list */

   alloctab->next = new_alloc;

   return OK;
}
