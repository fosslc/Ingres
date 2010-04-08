/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <me.h>
# include <st.h>
# include <iplist.h>
# include <erst.h>
# include <er.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <pc.h>

/*
**  Name: iplist.c -- linked list handling functions
**
**  Description:
**    The functions in this package manipulate linked lists.  A linked list
**    has the following structure.
**
**
**             +--------+       +---//---+       +--------> NULL
**             |        |       |        |       |
**             |       \|/      |       \|/      | 
**    +-----+--.--+    +-----+--.--+    +-----+--.--+
**    | CAR | CDR |    | CAR | CDR |    | CAR | CDR |
**    +--.--+-----+    +--.--+-----+    +--.--+-----+
**       |                |                |
**      \|/              \|/              \|/
**    (data)            (data)           (data)
**    
**    (The terminlogy "car" and "cdr" is a bit of Lisp-ism left over from
**    the author's shameful youth.)  The data items may be anything (e.g. 
**    character strings), including other lists; the only requirement is
**    that they be allocated with MEreqmem (because they're quite blindly
**    freed with MEfree).
**
**    A list consists of a "list header" structure, plus an arbitrary number
**    of list elements as above.  Memory for these structures is allocated
**    and freed as needed by functions in this package; external callers
**    do not need to concern themselves with memory management.  A list header
**    contains pointers to the following:
**   
**      -- The "head", or first element in the list.  NULL in the case of
**         an empty list.
**      -- The "tail", or last element in the list; NULL for an empty list.
**      -- The pointer to the last-accessed element in the list.  That is, the 
**         address of the thing that points to the most recently accessed
**         element, e.g. the cdr of the previous element.
**      -- The "current" element in the list, for the benefit of functions
**         which traverse lists in order via repeated calls.
**
**  Entry points:
**    ip_alloc_blk() -- Wrapper around MEreqmem
**    ip_list_init() -- Initialize a list header
**    ip_list_is_empty() -- Indicate whether a list is empty
**    ip_list_destroy() -- Destroy a list and re-initialize the header
**    ip_list_rewind() -- Set "current" pointer to head of list
**    ip_list_scan() -- Return successive elements of a list
**    ip_list_read_rewind() -- Initialize for a read-only scan of a list
**    ip_list_read() -- Scan list with pointer provided by caller
**    ip_list_unlink() -- Remove an element from a list
**    ip_list_remove() -- Remove and destroy an element from a list
**    ip_list_insert_before() -- Insert a new element before current element
**    ip_list_insert_after() -- Insert a new element after current element
**    ip_list_append() -- Append a new element to the end of a list
**    ip_list_curr() -- Return pointer to current element's data item
**    ip_stash_string() -- Allocate memory and copy a character string to it
**
**  History:
**    
**	23-July-1992 (jonb)
**	    Comments and history added.
**	16-mar-1993 (jonb)
**	    Brought comment style into conformance with standards.  Fixed
**          bug in ip_list_unlink.
**	22-mar-1993 (jonb)
**	    Added ip_list_read_rewind and ip_list_read.
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	01-oct-99 (loera01) Bug 98738 Cross-integration of 443595 from oping12
**	    In ip_listele_destroy(), eliminated recursive calls, which may
**	    lead to stack overflow on 16-bit machines.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added header files to eliminate gcc 4.3 warnings.
*/

/*  ip_alloc_blk() -- allocate a fixed-size block of memory
*/

PTR
ip_alloc_blk(i4 size)
{
    PTR rtn;
    STATUS stat;

    rtn = MEreqmem(0,size,FALSE,&stat);
    if (stat == OK)
	return rtn;

    IIUGerr(E_ST0154_mem_alloc_error,0,1,(PTR) &stat);
    PCexit(FAIL);
}


/*  ip_list_alloc() -- allocate a list element and initialize it.
*/

static LISTELE *
ip_list_alloc()
{
    LISTELE *new = (LISTELE *) ip_alloc_blk(sizeof(LISTELE));

    new->car = NULL;		/* The list is currently empty */
    new->cdr = NULL;
    new->cookie = COOKIE;	/* Flag that this is a list element */
    return new;
}

/*  ip_listele_destroy -- destroy a list element.
**
**  Actually, this can be passed a pointer to any arbitrary chunk of 
**  allocated memory.  If it's a list element -- the clue is the magic
**  cookie that all list elements have in them -- then recursive calls
**  destroy the things that they point at.
*/

static VOID
ip_listele_destroy(LISTELE *lp)
{
    LISTELE *lp_local;

    while (lp != NULL)
    {
	if ((i4)COOKIE == 
	    *(i4 *)(ME_ALIGN_MACRO((PTR)&lp->cookie, sizeof(lp->cookie))))
	{
                MEfree(lp->car);
                lp_local = lp;
		lp = lp->cdr;
                MEfree(lp_local);
 	}
        else
	{
            MEfree((PTR)lp);
            lp = NULL;
	}
    }
    return;
}

/*  ip_list_init() -- initialize a list header to look like an empty list.
*/

VOID
ip_list_init(LIST *ls)
{
    ls->head = ls->tail = NULL;
    ip_list_rewind(ls);
}

/*  ip_list_is_empty() -- is the list empty?
*/

bool
ip_list_is_empty(LIST *ls)
{
    return (ls->head == NULL);
}

/*  ip_list_destroy() -- destroy an entire list
*/

VOID
ip_list_destroy(LIST *ls)
{
    ip_listele_destroy(ls->head);
    ip_list_init(ls);
}

/* ip_list_rewind() -- set the list position pointers to the beginning of
** the list.
*/

VOID
ip_list_rewind(LIST *ls)
{
    ls->lastp = &ls->head;
    ls->curr = NULL;
}

/* ip_list_scan()
**
** Return successive data items from the list.  The first call following
** a call to ip_list_rewind sets the "carp" argument to point to the
** first data item, and so forth.  The pointer returns NULL when the list
** is exhausted.  The function returns a pointer to each actual list
** element, but that isn't usually what's interesting.
*/

LISTELE *
ip_list_scan(LIST *ls, PTR *carp)
{
    if (ls->curr != NULL)
	ls->lastp = &ls->curr->cdr;
    ls->curr = *(ls->lastp);
    if (ls->curr != NULL)
	*carp = ls->curr->car;
    else
	*carp = NULL;
    return ls->curr;
}

/* ip_list_read_rewind() -- initialize a read-only scan of the list */

VOID
ip_list_read_rewind(LIST *ls, PTR *currp)
{
    *currp = (PTR)ls->head;
}

/* ip_list_read() -- return successive elements of a list.  Same idea as
** ip_list_scan, with two exceptions: the "current position" pointer is
** supplied by the calling program, and the scan is read-only; it's expected
** that the calling program won't try to delete or add anything to or from
** the list.
*/

LISTELE *
ip_list_read(LIST *ls, PTR *carp, PTR *currp)
{
    LISTELE *curr = (LISTELE *)*currp;

    if (curr != NULL)
    {
	*carp = curr->car;
	curr = curr->cdr;
    }
    else
    {
	*carp = NULL;
    }

    *currp = (PTR) curr;
    return curr;
}


/*  ip_list_unlink()
**
**  Removes the current element from the list, and sets the position
**  pointers to point to the previous element.
*/

VOID
ip_list_unlink(LIST *ls)
{
    LISTELE *newcurr;
    PTR carp;

    bool unlinking_tail = (ls->curr == ls->tail);

    *(ls->lastp) = newcurr = ls->curr->cdr;
    ls->curr->cdr = NULL;

    if (ip_list_is_empty(ls))
    {
	ip_list_init(ls);
    }
    else
    {
	if (unlinking_tail)
	{
	    for ( ip_list_rewind(ls); ip_list_scan(ls,&carp); )
	    {
		ls->tail = ls->curr;
	    }
	}

	for ( ip_list_rewind(ls); ip_list_scan(ls,&carp); )
	{
	    if (ls->curr->cdr == newcurr)
		break;
	}
    }
	
}

/*  ip_list_remove -- unlink and destroy a single element from the list.
*/

VOID 
ip_list_remove(LIST *ls)
{
    LISTELE *killme = ls->curr;

    ip_list_unlink(ls);
    ip_listele_destroy(killme);
}

/*
**  ip_list_insert_before 
**  
**  Insert a new element before the current location on the list, as indicated
**  by the position pointers "lastp" and "curr" in the header.  Make the
**  newly inserted element the current location.
*/

VOID
ip_list_insert_before(LIST *ls, PTR newcar)
{
    LISTELE *new = ip_list_alloc();

    new->car = newcar;
    *(ls->lastp) = new;
    new->cdr = ls->curr;
    if (new->cdr == NULL)
      ls->tail = new;
    ls->lastp = &new->cdr;
}

/*
**  ip_list_insert_after -- same idea as ip_list_insert_before
*/

VOID 
ip_list_insert_after(LIST *ls, PTR newcar)
{
    LISTELE *new = ip_list_alloc();

    new->car = newcar;
    new->cdr = ls->curr->cdr;
    if (new->cdr == NULL)
      ls->tail = new;
    ls->curr->cdr = new;
}


/*  ip_list_append -- add a new element onto the end of the list
*/

VOID 
ip_list_append(LIST *ls, PTR newcar)
{
    LISTELE *new = ip_list_alloc();

    new->car = newcar;
    if (ls->tail == NULL)
    {
	ls->head = new;
    }
    else
    {
        ls->tail->cdr = new;
	ls->lastp = &ls->tail->cdr;
    }
    ls->tail = ls->curr = new;
}

/* ip_list_curr 
**
** Return a pointer to the data item associated with the current list
** element.
*/

PTR
ip_list_curr(LIST *ls)
{
    return ls->curr->car;
}


/* 
**  ip_stash_string
**
**  Allocate a block of memory and copy a character string to it.  Return
**  a pointer to the new copy of the string.
*/

char *
ip_stash_string(char *s)
{
    char *new;

    if (s == NULL)
	return NULL;
    new = (char *) ip_alloc_blk(1+STlength(s));
    STcopy(s,new);
    return new;
}

/*  ip_list_assign_new() -- stash a string and point a pointer to it 
**
**  This function stashes the string in allocated memory, and sets a
**  character pointer to point to where it's been stashed.
*/

VOID
ip_list_assign_new(char **sptr, char *str)
{
    if (str == NULL || *str == EOS)  /* Is the new thing null or empty? */
    {
        *sptr = NULL;   /* Just stash a null pointer */
    }
    else
    {
        *sptr = (char *) ip_alloc_blk(1+STlength(str));  /* new area */
        STcopy(str,*sptr);  /* Stick the string in there */
    }
}

/*  ip_list_assign_string() -- save a string and point a pointer to it
**
**  Wrapper around ip_list_assign_new that also checks to see if anything
**  is currently pointed at by the pointer.  If so, it's deallocated
**  first, and then the new value is stashed and pointed at.
*/
 
VOID
ip_list_assign_string(char **sptr, char *str)
{
    if (*sptr != NULL)  /* Currently pointing at anything? */
        MEfree(*sptr);  /* Release the area we won't be pointing at any more */
    ip_list_assign_new(sptr,str);
}

