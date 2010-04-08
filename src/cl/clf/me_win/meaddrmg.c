/******************************************************************************
**
** Copyright (c) 1989, 2004 Ingres Corporation
**
**
******************************************************************************/

#include <compat.h>
#include <clconfig.h>
#include <meprivate.h>
#include <me.h>
#include <cs.h>

/******************************************************************************
**
**  Name: MEADDRMG.C - routines to manage memory bit map of allocations
**
**  Description:
**	routines to manage memory bit map of allocations.
**
**	    MEisalloc(pageno, pages, isalloc)
**	    MEsetalloc(pageno, pages)
**	    ME_reg_seg(addr, pages)
**	    ME_find_seg(addr, eaddr, segq)
**	    ME_rem_seg(seginf)
**	    MEfindpages(pages, pageno)
**	    MEsetpg(alloctab, pageno, pages)
**	    MEclearpg(alloctab, pageno, pages)
**	    MEalloctst(alloctab, pageno, pages, allalloc)
**
**  History:
**	22-aug-1995 (canor01)
**	    add ME_segpool_sem to protect global queue
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	09-feb-1996 (canor01)
**	    ME_find_seg() must have ME_segpool_sem held on entry.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	07-dec-2000 (somsa01)
**	    Use MEmax_alloc instead of ME_MAX_ALLOC.
**	06-aug-2001 (somsa01)
**	    Properly set function argument types of MEclearpg(), and
**	    cleaned up compiler warnings.
**      11-Nov-2004 (horda03) Bug 111658/INGSRV2680
**          Interface to MEfindpages() changed.
**	30-Nov-2004 (drivi01)
**	  Updated i4 to SIZE_TYPE to port change #473049 and #473322 to windows.
**
******************************************************************************/

/******************************************************************************
**
**  Forward and/or External typedef/struct references.
**
******************************************************************************/

GLOBALREF i2     MEconttab[];
GLOBALREF u_char *MEalloctab;
GLOBALREF i4     MEendtab;
GLOBALREF i4     MEmax_alloc;

# ifdef MCT
GLOBALREF CS_SEMAPHORE	ME_segpool_sem;
# endif

/******************************************************************************
**
**  Forward and/or External function references.
**
******************************************************************************/

VOID	MEsetpg(char * alloctab, i4 pageno, i4 pages);
VOID	MEclearpg(char * alloctab, i4 pageno, i4 pages);
bool	MEalloctst(char * alloctab, i4 pageno, i4 pages, bool allalloc);

/******************************************************************************
**
**  Defines of other constants.
**
******************************************************************************/

/******************************************************************************
**
** Definition of all global variables owned by this file.
**
******************************************************************************/
GLOBALREF QUEUE ME_segpool;

/******************************************************************************
**
**  Definition of static variables and forward static functions.
**
******************************************************************************/

/******************************************************************************
**
** check if a page in the break region is allocated
**
******************************************************************************/
bool
MEisalloc(SIZE_TYPE pageno, SIZE_TYPE pages, bool isalloc)
{
	return (MEalloctst(MEalloctab, pageno, pages, isalloc));
}

/******************************************************************************
**
** mark some pages within the break as allocated
**
******************************************************************************/
void
MEsetalloc(SIZE_TYPE pageno, SIZE_TYPE pages)
{
	MEsetpg(MEalloctab, pageno, pages);
}

/******************************************************************************
**
** Name: ME_reg_seg()	- allocate a data structure to keep track of memory segs
**
** Description:
**	Routine allocates a ME_SEG_INFO structure to hold information about
**	the input memory segment.  The structure is inserted in "address" order
**	in a queue of ME_SEG_INFO structures.  The purpose of the data
**	structure is to manipulate sparse memory bitmaps for discrete memory
**	segments allocated outside of the brk() region.
**
** Inputs:
**	addr				beginning address of memory segment.
**	pages				number of pages in the segment.
**
** Outputs:
**	none.
**
**	Returns:
**	    OK				success
**	    not OK			MEreqmem() failure returns
**
******************************************************************************/
STATUS
ME_reg_seg(PTR addr, SIZE_TYPE pages, HANDLE sh_mem_handle, HANDLE file_handle)
{
	STATUS		status = OK;
	ME_SEG_INFO	*seginf;
	QUEUE		*q;
	u_i4		size_of_seginfo;

	/*
	 * a ME_SEG_INFO has a dynamically allocated allocation table at the
	 * end of it (allocvec).  This allocation ME_SEG_INFO must allocate
	 * the size of the structure plus tack onto the end one bit for every
	 * page in the shared memory segment being allocated.
	 */

	size_of_seginfo = (sizeof(*seginf) - sizeof(seginf->allocvec)) +
		(u_i4) (sizeof(char) * ((pages / BITSPERBYTE) + 1));

	seginf = (ME_SEG_INFO *) MEreqmem((u_i4) 0,
	                                  size_of_seginfo,
	                                  TRUE,
	                                  &status);

	if (status) {
		return (status);
	}

	seginf->npages      = pages;
	seginf->addr        = (char *) addr;
	seginf->eaddr       = pages * ME_MPAGESIZE + (char *) addr;
	seginf->mem_handle  = sh_mem_handle;
	seginf->file_handle = file_handle;

	MEsetpg(seginf->allocvec, 0, pages);

	/*
	 * need to insert in sorted order
	 */

	gen_Psem(&ME_segpool_sem);
	for (q  = ME_segpool.q_prev;
	     q != &ME_segpool;
	     q  = q->q_prev) {
		if (((ME_SEG_INFO *) q)->addr < (char *) addr) {
			break;
		}
	}
	QUinsert( &seginf->q, q);
	gen_Vsem(&ME_segpool_sem);
	return (OK);
}

/******************************************************************************
**
**
**
******************************************************************************/
ME_SEG_INFO *
ME_find_seg(char *addr, char *eaddr, QUEUE *segq)
{
	ME_SEG_INFO	*segp;
	QUEUE		*q;

	for (q  = segq->q_prev;
	     q != &ME_segpool;
	     q  = q->q_prev) {
		segp = (ME_SEG_INFO *) q;
		if ((eaddr > segp->addr) &&
		    (addr  < segp->eaddr)) {
			return (segp);
		}
	}
	return (NULL);
}

/******************************************************************************
**
**
**
******************************************************************************/
QUEUE *
ME_rem_seg(ME_SEG_INFO *seginf)
{
	QUEUE *next_q;

	/*
	 * Save pointer to next entry - this is returned to caller.
	 */

	next_q = seginf->q.q_next;

	/*
	 * Remove this entry from queue and deallocate the memory.
	 */

	(VOID) QUremove(&seginf->q);
	(VOID) MEfree((PTR) seginf);

	return (next_q);
}

/******************************************************************************
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
**      pageno                          page number of first page of
**                                      free region
**
**	Returns:
**	   TRUE                      free region found. 
**	   FALSE                     no free region.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
******************************************************************************/
bool
MEfindpages(SIZE_TYPE pages, SIZE_TYPE *pageno)
{
	char           *bp, *ep;
	SIZE_TYPE      maxcont; 
	i4	       cont;
	SIZE_TYPE      pos;
	i4             mask;

	maxcont = 0;
	for (bp = MEalloctab, ep = bp + MEmax_alloc / ME_BITS_PER_BYTE;
	     bp < ep; ++bp) {
		cont = MEconttab[(*bp) & 0xff];
		if (cont & ME_ALLSET) {
			maxcont = 0;
			continue;
		}
		if (cont & ME_ALLCLR) {
			maxcont += 8;
		} else {
			maxcont += ME_LOWCNT(cont);
		}
		if (maxcont >= pages) {
			pos = (i4)((bp - MEalloctab) * 8);
			if (cont & ME_ALLCLR) {
				pos += 8;
			} else {
				pos += ME_LOWCNT(cont);
			}
			pos -= maxcont;
			*pageno = pos;
                        return TRUE;
		}
		if (cont & ME_ALLCLR) {
			continue;
		}
		if (ME_MAXCNT(cont) >= pages) {
			mask = (1 << pages) - 1;
			for (pos = 0; pos + pages <= 8; pos++) {
				if (!(*bp & (mask << pos))) {
					pos += (i4)((bp - MEalloctab) * 8);
					*pageno = pos;
                                        return TRUE;
				}
			}
			/* this should never happen */
			return FALSE;
		}
		maxcont = ME_UPCNT(cont);
	}
	return FALSE;
}

/******************************************************************************
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
******************************************************************************/
VOID
MEsetpg(char * alloctab, SIZE_TYPE pageno, SIZE_TYPE pages)
{
	SIZE_TYPE             i, b;
	u_char          m;

	i = ME_BYTE_FROM_PAGE(pageno);
	m = 0xff;
	if (b = ME_BIT_FROM_PAGE(pageno)) {
		if (pages < ME_BITS_PER_BYTE) {
			m = (1 << pages) - 1;
		}
		alloctab[i] |= m << b;
		pages -= (pages < (ME_BITS_PER_BYTE - b)) ? pages : (ME_BITS_PER_BYTE - b) ;
		++i;
	}
	m = 0xff;
	while (pages) {
		if (pages < ME_BITS_PER_BYTE) {
			m = (1 << pages) - 1;
		}
		alloctab[i] |= m;
		pages -= ((pages < ME_BITS_PER_BYTE) ? pages : ME_BITS_PER_BYTE);
		++i;
	}
}

/******************************************************************************
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
******************************************************************************/
VOID
MEclearpg(char * alloctab, SIZE_TYPE pageno, SIZE_TYPE pages)
{
	SIZE_TYPE             i, b;
	u_char          m;

	i = ME_BYTE_FROM_PAGE(pageno);
	m = 0xff;
	if (b = ME_BIT_FROM_PAGE(pageno)) {
		if (pages < ME_BITS_PER_BYTE) {
			m = (1 << pages) - 1;
		}
		alloctab[i] &= ~(m << b);
		pages -= (pages < (ME_BITS_PER_BYTE - b)) ? pages : (ME_BITS_PER_BYTE - b) ;
		++i;
	}
	m = 0xff;
	while (pages) {
		if (pages < ME_BITS_PER_BYTE) {
			m = (1 << pages) - 1;
		}
		alloctab[i] &= ~m;
		pages -= ((pages < ME_BITS_PER_BYTE) ? pages : ME_BITS_PER_BYTE);
		++i;
	}
}

/******************************************************************************
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
******************************************************************************/
bool
MEalloctst(char * alloctab, SIZE_TYPE pageno, SIZE_TYPE pages, bool allalloc)
{
	SIZE_TYPE           i, b;
	u_char          m;
	u_char          tmptab;

	i = ME_BYTE_FROM_PAGE(pageno);
	if (b = ME_BIT_FROM_PAGE(pageno)) {
		m = -1;
		if (pages < ME_BITS_PER_BYTE) {
			m = (1 << pages) - 1;
		}
		m <<= b;
		tmptab = (alloctab[i] & 0x80 ? alloctab[i] | 0xFFFFFF00 : alloctab[i]);
		if ((tmptab & m) != (allalloc ? m : 0)) {
			return (TRUE);
		}
		pages -= (pages < (ME_BITS_PER_BYTE - b)) ? pages : (ME_BITS_PER_BYTE - b) ;
		++i;
	}
	m = -1;
	while (pages) {
		if (pages < ME_BITS_PER_BYTE) {
			m = (1 << pages) - 1;
		}
		if ((alloctab[i] & m) != (u_char) (allalloc ? m : 0)) {
			return (TRUE);
		}
		pages -= ((pages < ME_BITS_PER_BYTE) ? pages : ME_BITS_PER_BYTE);
		++i;
	}
	return (FALSE);
}

# if defined(ME_DEBUG)
/*
** Name: MEverify_conttab -- verify MEconttab
**
** Description:
**    This function can be used to verify correctness of
**    the MEconttab array. The array contains a hint for
**    every integer in the range between 0 and 255. If the
**    integer is a byte from the page allocation bitmap,
**    then the hint contains
**        - length of the leftmost contiguous area (UPCNT)
**        - length of the rightmost contiguous area (LOWCNT)
**        - length of the maximum contiguous area  (MAXCNT)
**    within the stretch of 8 memory pages represented by
**    the byte.
** Inputs:
** Outputs:
** Returns:
**    OK                   MEconttab[] is good
**    FAIL                 otherwise
** History:
**    22-aug-1997 (tchdi01)
**        created
*/ 
STATUS
MEverify_conttab()
{
    u_char i, tmp;
    i4 cnt, j, low, max, up;

    for (i = 1; i < 0xff; i++)
    {
	low = max = up = -1;
	tmp = i; cnt = j = 0;
	do
	{
	    if (tmp & 0x1)
	    {
		if (low < 0)
		{
		    low = cnt;
		}
		if (cnt > max) max = cnt;
		cnt = 0;
	    }
	    else
	    {
		cnt ++;
	    }
	    
	    tmp >>= 1; j++;

	} while (j != 8);
	
	up = cnt;
	if (up > max) max = cnt;

	if (low != ME_LOWCNT(MEconttab[i]) ||
	    up  != ME_UPCNT(MEconttab[i])  ||
	    max != ME_MAXCNT(MEconttab[i]))
	{
	    return (FAIL);
	}
    }

    return (OK);
}
# endif /* ME_DEBUG */	
	     


