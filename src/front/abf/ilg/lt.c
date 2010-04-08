/*
**	Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<ex.h>
#include	<me.h>
#ifndef MEcalloc
#define MEcalloc(size, status) MEreqmem(0, (size), TRUE, (status))
#endif
#include	<si.h>
#include	<er.h>
#include	<lt.h>

/**
** Name:    lt.c -	OSL Parser List Module.
**
** Description:
**	Contains the routines used to access the list abstraction used
**	by OSL.  Defines:
**
**	LTadd()		add element to a list.
**	LTrm()		remove an element from a list.
**	LTin()		see if an element is in a list.
**	LTalloc()	allocate a list element.
**	LTfree()	free a list element.
**	LTcount()	returns the number of elements in a list.
**	LTsort()	sort a list.
**	LTpush()
**	LTtop()
**	LTpop()		implement a stack abstraction with lists.
**	LTprint()	print a list.
**
** History:
**	Revision 4.0  85/07/85  joe
**	Added stack abstraction and routine to LTfree.
**
**	Revision 2.0  82/05/14  joe
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:    LTadd() -	Add Element to a List.
**
** Description:
**	Add an element to a list.
**
** Input:
**	ele	{PTR}  A reference to the element to add to the list.
**			The actual type of this element can be anything.
**	lis	{LIST **}  The list.
**	tag	{nat}  The tag of the list node to be added.
**
** Side Effects:
**	Adds a new element to the list, can affect the free pool.
**	LTpush depends on it putting the element at the head of
**	the list.  If this changes LTpush must be reimplemented.
**
** History:
**	Written 5/20/82 (jrc)
*/
VOID
LTadd (lis, ele, tag)
register LIST	**lis;
PTR		ele;
i4		tag;
{
    register LIST	*lt;

    LIST	*LTalloc();

    lt = LTalloc(tag);
    lt->lt_rec = ele;
    if (*lis != NULL)
	lt->lt_next = *lis;
    *lis = lt;
}

/*{
** Name:    LTrm() -	Remove an Element From a List.
**
** Description:
**	Remove an element from a list.
**
** Input:
**	lis	{LIST **}  The list.
**	ele	{PTR}  The element in the list node to be returned.
**			Its type is variable based on the func that does
**			the comparison.  It is up to the caller to maintain
**			consistency.
**
**	func	{nat *()}  A pointer to a function that is assumed to 
**			compare the list element with ele for equality.
**			func should return 0 on equality.
**			If func == NULL, then a straight pointer comparison
**			is done.
**
** Returns:
**	{PTR}  The element which was removed.
**		NULL is returned if the element is not find.
**
** Side Effects:
**	Removes the element from the list and frees the list node.
**
** History:
**	Written 5/20/82 (jrc)
*/
PTR
LTrm (lis, ele, func)
register LIST	**lis;
PTR		ele;
i4		(*func)();
{
    VOID	LTfree();

    for (; *lis != NULL; lis = LTnaddr(*lis))
    {
	if (func == NULL ? ((*lis)->lt_rec == ele)
			 : ((*func)(ele, ((*lis)->lt_rec)) == 0) )
	{
	    register LIST	*lt;
	    PTR			oldele;

	    lt = *lis;
	    *lis = LTnext(*lis);
	    oldele = lt->lt_rec;
	    LTfree(lt, (VOID(*)()) NULL);
	    return oldele;
	}
    } /* end for */
    return (PTR)NULL;
}

/*{
** Name:    LTin() -	Determine if Element is in List.
**
** Description:
**	Check for containment in the list.
**
** Input:
**	ele	{PTR}  A reference to the element for which to search.
**	lis	{LIST *}  The list to search.
**	func	{nat *()}  A pointer to a function that compares the element
**			with the elements in the list.
**			If func is NULL then do pointer comparsions.
**
** Returns:
**	{PTR}  A reference to the element in the list node, or NULL
**		if the element is not found.
**
** History:
**	Written 5/20/82 (jrc)
*/
PTR
LTin (ele, lis, func)
register PTR	ele;
register LIST	*lis;
register i4	(*func)();
{
    for (; lis != NULL; lis = LTnext(lis))
    {
	if (func == NULL ? (lis->lt_rec == ele)
			 : ((*func)(ele, lis->lt_rec) == 0) )
	    return lis->lt_rec;
    }
    return (PTR)NULL;
}


/*
** List elements are allocated from an maintained free list of such
** elements.  The free list is allocated in chunks of size LISTSIZE.
*/

#define		LISTSIZE 50

static LIST	*freelist = LNIL;

/*{
** Name:    LTalloc() -	Allocate New List Element.
**
** Description:
**	Allocate a new list element from the free list.
**	The element is given the proper tag.
**
** Parameters:
**	tag	{nat}  Tag for the type of list node this is.
**
** Returns:
**	{LIST *}  A reference to a new list element that has been zeroed, and
**		has the tag portion initialized.
**
** Side Effects:
**	Changes the free list, may call calloc.
**
** Error Messages:
**	Raises exception if out of memory.
**
** History:
**	Written (jrc)
*/
LIST *
LTalloc (tag)
i4	tag;
{
    register LIST	*lp;

    if (freelist == LNIL)
    {
	register i4	i;
	STATUS		stat;

	if ((freelist = (LIST*)MEcalloc(LISTSIZE*sizeof(LIST), &stat)) == NULL)
	    EXsignal(EXFEMEM, 1, ERx("LTalloc()"));

	/* set up the 'next' pointers. */
	for (i = 0 ; i < (LISTSIZE - 1); ++i)
	{
	    freelist[i].lt_next = &freelist[i + 1];
	}
    }

    lp = freelist;
    freelist = lp->lt_next;
    lp->lt_next = LNIL;

    return lp;
}

/*{
** Name:    LTfree() -	Free List Element.
**
** Description:
**	Frees up a list element
**
** Input:
**	lt	{LIST *}  The list element to free.
**	func	{VOID (*)()}  Routine to call on each element freed.
**
** Side Effects:
**	Adds an element to the free list.
**
** History:
**	Written 5/14/82 (jrc)
*/
VOID
LTfree (lt, func)
register LIST	*lt;
VOID		(*func)();
{
    if (lt != NULL)
    {
	(*func)(lt->lt_rec);
	MEfill(sizeof(*lt), '\0', (PTR) lt);
	lt->lt_next = freelist;
	freelist = lt;
    }
}

/*{
** Name:    LTcount() -	Return Count of Elements in List.
**
** Description:
**	Counts the number of elements in a list.
**
** Input:
**	lt	{LIST *}  The list to count.
** Returns:
**	{nat}  The number of elements in the list.
**
** History:
**	Written 5/14/82 (jrc)
*/
i4
LTcount (lt)
register LIST	*lt;
{
    register i4	i = 0;

    for (; lt != NULL; lt = LTnext(lt))
	++i;
    return i;
}

/*{
** Name:    LTsort() -	Sort List.
**
** Description:
**	Sort a list using insertion sort.
**
** Input:
**	lp	{LIST **}  The list to be sorted.
**	func	{nat (*)()}  A pointer to a function that compares the elements
**			in the list.
**
** History:
**	Written 7/28/82 (jrc)
*/
VOID
LTsort (lp, func)
register LIST	**lp;
i4		(*func)();
{
    for (; *lp != NULL; lp = LTnaddr(*lp))
    {
	register LIST	**lp1;

	for (lp1 = &((*lp)->lt_next) ; *lp1 != NULL ; lp1 = LTnaddr(*lp1))
	{
	    /*
	    ** for the following comments,
	    ** old is *lp
	    ** new is *lp1
	    */
	    if ((*func)((*lp)->lt_rec, (*lp1)->lt_rec) > 0)
	    {
		register LIST	*lt;

		lt = (*lp)->lt_next;
		if ((*lp)->lt_next == *lp1)
		{
		    /*
		    ** lt == new
		    */
		    (*lp)->lt_next = lt->lt_next;
		    lt->lt_next = *lp;
		    lp1 = &(lt->lt_next);
		}
		else
		{
		    /*
		    ** lt is old->lt_next
		    */
		    (*lp)->lt_next = (*lp1)->lt_next;
		    (*lp1)->lt_next = lt;
		    lt = *lp1;
		    *lp1 = *lp;
		}
		*lp = lt;
	    }
	} /* end for */
    } /* end for */
}

/*
** LT stack routines
** Implement a stack abstraction with lists.
*/

VOID
LTpush (s, ele, tag)
LIST	**s;
PTR	ele;
i4	tag;
{
    LTadd(s, ele, tag);
}

PTR
LTpop (s)
register LIST	**s;
{
    PTR	rval;

    rval = LTtop(*s);
    *s = (*s)->lt_next;
    return rval;
}

PTR
LTtop (s)
LIST	*s;
{
    return s == NULL ? (PTR)NULL : s->lt_rec;
}

/*{
** Name:    LTprint() -	Print List.
**
** Description:
**	Print a list according to the function passed.
**	Used in debugging.
**
** Input:
**	lt	{LIST *}  The list to print.
**	func	{nat (*)()}  The function used to print the list elements.
**			If func is NULL then the address of the element
**			is printed.
**
** History:
**	Written 9/1/82 (jrc)
**	11-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
*/
VOID
LTprint (lt, func)
register LIST	*lt;
char		*(*func)();
{
    if (lt == NULL)
	SIfprintf(stderr, ERx("\rThe list is empty\n"));
    else
    {
	SIfprintf(stderr, ERx("\rThe list 0x%p contains:\n"), lt);
	for (; lt != NULL; lt = LTnext(lt))
	{
	    if (func == NULL)
		SIfprintf(stderr, ERx("\r\telement is 0x%p\n"), lt->lt_rec);
	    else
		SIfprintf(stderr, ERx("\r\telement is %s\n"),
			(*func)(lt->lt_rec)
		);
	}
	SIfprintf(stderr, ERx("\rThe end of list.\n"));
	SIflush(stderr);
    }
}
