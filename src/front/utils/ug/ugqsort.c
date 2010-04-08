/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/
#include	<stdlib.h>
#include	<compat.h>

#if 0
#include	<me.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#endif

/**
** Name:    feqsort.c -	Front-End Utility In-memory Quick Sort Routine.
**
** Description:
**      Contains a quick sort for sorting arrays in memory with a caller
**	provided comparison function, ala the UNIX utility.  Defines:
**
**	IIUGqsort()	quick sort.
**
** History:
**	Revision 6.0  87/03/20  bobm
**	Initial revision.
**
**      04-Mar-93 (smc)
**      Cast parameter of MEfree to be PTR not i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#if 0
/*
** below this length, we finish off with a bubble sort
** MUST be positive for termination in do_qs.
**
** was established by timing the routine sorting random arrays
** of integers on a pyramid.
*/

#ifdef TIME_THRESHOLD
i4  THRESHOLD;
#else
#define THRESHOLD 6
#endif

/* remember - compund statement */
#define SWAP(x,y) MEcopy (x,(u_i2) Eltsize,Tempblock); \
			MEcopy (y,(u_i2) Eltsize,x); \
			MEcopy (Tempblock,(u_i2) Eltsize,y)

static i4  Tempsize = 0;
static i4  Eltsize;
static char *Tempblock;
static i4  (*Compfunc)();
static	VOID	do_qs();
static	VOID	bubble();
#endif	

/*{
** Name:    IIUGqsort() -	quick sort
**
** Description:
**	A generic quick sort which uses a caller provided comparison
**	function to sort an array of elements of arbitrary size.
**	Precautions are taken to assure termination no matter what
**	the comparison function returns, although nonsense return
**	values will obviously lead to a nonsense ordering.
**
** Inputs:
**	char *arr		the array to be sorted
**	i4 nel			the number of elements in the array
**	i4 size		the size in bytes of a single element
**	i4 (*compare)()	pointer to the comparison function:
**
**		(*compare)(elt1,elt2)
**		char *elt1;
**		char *elt2;
**
**		elt1, elt2 will actually be the addresses of blocks
**		of size "size", starting at address "arr", ie. the
**		elements of the array, which (*compare) may coerce
**		into whatever datatype or structure is appropriate in
**		order to compare the elements.
**
**		return <0 for elt1 < elt2
**		return 0 for elt1 = elt2
**		return >0 for elt1 > elt2
**
** Outputs:
**	sorted array
**
**	Exceptions:
**		syserr for memory allocation failure
**		(an element may be allocated to use in swapping memory)
**
** History:
**	3/87 (bobm)	written
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	30-may-02 (rucmi01) Bug 107721
**		Use OS's library qsort routine instead of using do_qs.
**		This fixes bug in do_qs which is extremely slow when the array
**		items are all equal (tested on NT, UNIX, VMS).
**
**		Keep old code around with "#if 0", or "#if 1" for now.
*/
VOID
IIUGqsort (arr,nel,size,compare)
char *arr;
i4  nel;
i4  size;
i4  (*compare)();
{
#if 1
    /*
    ** Use the C-library qsort function.
    */
    size_t num = (size_t)nel;
    size_t width = (size_t)size;
    qsort(arr, num, width, compare);
#else
    /*
    ** Old code.
    */
    Eltsize = size;
    Compfunc = compare;

    /*
    ** keep Tempblock around to reuse as long as big enough.
    ** use the "generic" memory pool.
    */
    if (Eltsize > Tempsize)
    {
	if (Tempsize > 0)
	    MEfree((PTR) Tempblock);
	Tempsize = Eltsize;
	if ((Tempblock = (char *)MEreqmem(0, Tempsize, FALSE, (STATUS *)NULL))
		== NULL)
	{
	    IIUGbmaBadMemoryAllocation(ERx("IIUGqsort"));
	}
    }

    do_qs(arr,nel);
#endif
}

/*
** The rest of the code in the file is unnecessary if 
** the C-library qsort routine is used.
*/
#if 0
/*
** the real quick sort logic:
**
** Grab the middle element, take one pass through the array separating
** it into two arrays, the first strictly less than the comparison
** element, the second >= to it.  The normal trick for this is to
** run a pointer from each end of the array, swapping as we find elements
** that belong in the other array, and bailing out when they cross.
** Then sort the two pieces recursively.
*/
static VOID
do_qs(arr,nel)
register char *arr;
i4  nel;
{
	register char *cmp;		/* comparison pointer */
	register char *hp,*tp;		/* head and tail pointers */
	register int hbump,tdec;	/* bump / decrement counters */

	if (nel < THRESHOLD)
	{
		bubble(arr,nel);
		return;
	}

	/*
	** Set comparison element.  We will have to reset this pointer
	** anytime we swap the comparison element, since we are pointing
	** directly into the array, rather than maintaining an element
	** off to the side.
	*/
	cmp = arr + (nel/2)*Eltsize;

	tdec = nel - 1;
	hbump = 0;
	hp = arr;
	tp = arr + (nel - 1)*Eltsize;
	for (;;)
	{
		/*
		** find the next elements to swap.
		**
		** We check against cmp to save a procedure call.
		** We know the answer is 0.  We also keep cmp >= hp,
		** which assures that the first while terminates even
		** even if Compfunc returns < 0 all the time.
		**
		** We keep hbump & tdec to avoid having to compare
		** pointers for anything except equality.
		*/
		while (hp != cmp && (*Compfunc)(hp,cmp) < 0)
		{
			++hbump;
			hp += Eltsize;
		}
		while (tp != arr && (tp == cmp || (*Compfunc)(tp,cmp) >= 0))
		{
			--tdec;
			tp -= Eltsize;
		}

		/* passed each other yet ? ( "=" can happen if ALL >= cmp) */
		if (hbump >= tdec)
			break;

		/*
		** check to see if we're swapping comparator, and if so
		** change cmp pointer.  tp can't point to comparator.
		*/
		if (hp == cmp)
			cmp = tp;
		SWAP(hp,tp);

		/*
		** Bump to avoid useless Compfunc calls.
		** Note that tp > hp at this point, and cmp is either
		** = tp, or hp didn't reach it at the top of the loop.
		** Either way cmp > hp at this point, so we can assure
		** cmp >= hp again at the top of the loop to guarantee
		** termination of the first while.
		*/
		hp += Eltsize;
		++hbump;
		--tdec;
		tp -= Eltsize;

		/*
		** save ourselves a comparison if crossed because of last swap.
		** if EQUAL, we must continue because hp must be bumped
		** if the element they both point to is < cmp
		*/
		if (hbump > tdec)
			break;
	}


	/*
	** See if we've had the bad luck to choose the smallest element
	** for our comparator.  If so, put it first, and sort the rest.
	** This is a needed special case because we fell out on the first
	** loop iteration after not bumping hp, and finding tp decremented
	** all the way back to arr.  We've legitimately made the >= piece
	** the entire array, in other words.
	*/
	if (hbump == 0)
	{
		SWAP(arr,cmp);
		do_qs(arr+Eltsize,nel-1);
		return;
	}

	/*
	** We've got non-zero length "<" and ">=" pieces.  Sort each.
	*/
	do_qs(arr,hbump);
	do_qs(hp,nel-hbump);
}

/* the bubble sort finisher - lengths <= 1 must cause no problems */
static VOID
bubble(arr,nel)
register char *arr;
i4  nel;
{
	register i4  i;
	register char *ptr, *ptrnxt, *ptrlim;
	register bool swaps;

	for (i=(nel-1)*Eltsize; i > 0; i -= Eltsize)
	{
		swaps = FALSE;
		ptrlim = arr+i;
		for (ptr = arr; ptr != ptrlim; ptr = ptrnxt)
		{
			ptrnxt = ptr + Eltsize;
			if ((*Compfunc)(ptr,ptrnxt) > 0)
			{
				SWAP(ptr,ptrnxt);
				swaps = TRUE;
			}
		}
		if (! swaps)
			break;
	}
}
#endif
