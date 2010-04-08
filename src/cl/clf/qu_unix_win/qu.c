/*
** Copyright (c) 1987, 2002 Ingres Corporation
*/

# include    <compat.h> 
# include    <gl.h> 
# include    <ex.h>
# include    <qu.h>  

/**
**
**  Name: QU.C - Queue manipulation routines.
**
**  Description:
**      This file holds all of the queue manipulations routines as specified 
**      in the CL specification.
**
**          QUinit() - Initialize a queue.
**          QUinsert() - Insert elements in queue.
**          QUremove() - Remove elements from queue.
**
**
**  History:    
 * Revision 1.1  90/03/09  09:16:22  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:45:14  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:59:41  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:19:23  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:13:15  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:44:24  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/10  15:01:13  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/22  10:20:25  mikem
**      Upgraded to jupiter coding standards.
**      
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 CL standards.
**      22-jul-87 (mmm)
**          Upgraded UNIX CL to Jupiter and 5.0 CL standards.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-may-95 (emmag)
**	    Cleaned up the comments - somebody got their 
**	    inserts and removes muddled.
**	12-feb-1998 (canor01)
**	    Removed VMS-specific code. Add exception handler for NT.
**	03-mar-1998 (canor01)
**	    Removed QUr_* functions from NT build (until MEget_seg exists).
**	26-sep-2002 (lunbr01)  bug 106589
**	    Removed EX exception routine calls/processing. Adds overhead 
**          that seems unnecessary. calls/processing. Adds overhead ).
**/

# ifdef NT_GENERIC
static STATUS qu_handler(EX_ARGS *exargs);
static STATUS qu_handler2(EX_ARGS *exargs);
# endif /* NT_GENERIC */


/*{
** Name: QUinit() - Initialize a queue.
**
** Description:
**      "q" is a pointer to the header element of a queue.  Perform all
**      necessary initializations to set up a queue.
**
**	NOTE:  This procedure may be implemented as a macro.
**
** Inputs:
**      q                               Pointer to head element in queue.
**
** Outputs:
**      none
**
**	Returns:
**	    VOID
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 CL.  Also made this a VOID function.
**      22-jul-87 (mmm)
**          Upgraded UNIX CL to Jupiter and 5.0 CL standards.
*/
VOID
QUinit(q)
QUEUE	*q;
{
	q->q_next = q;
	q->q_prev = q;
}


/*{
** Name: QUinsert() - Insert element in queue.
**
** Description:
**      Insert element "entry" immediately after queue pointer "q".  The
**      procedure returns the value of "entry". 
**
** Inputs:
**      entry                           QUEUE element to insert into queue.
**      q                               Where to insert "entry".  "entry" will
**                                      be inserted immediately after this
**                                      QUEUE element.
**
** Outputs:
**      none
**
**	Returns:
**	    "entry"
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 CL.
**      22-jul-87 (mmm)
**          Upgraded UNIX CL to Jupiter and 5.0 CL standards.
*/
/* ARGSUSED */ 


QUEUE   *
QUinsert(entry, next)
QUEUE   *entry;
QUEUE   *next;
{
    register QUEUE  *e = entry;
    register QUEUE  *p = next;

    e->q_next = p->q_next;
    e->q_prev = p;
    p->q_next->q_prev = e;
    p->q_next = e;

    return (e);
}


/*{
** Name: QUremove() - Remove element from queue.
**
** Description:
**      "entry" is a pointer to an element in a queue.  Remove and return a
**      pointer to that element, or NULL if only the user-declared header
**      element was found. 
**
** Inputs:
**      entry                           QUEUE element to remove from queue.
**
** Outputs:
**      none
**
**	Returns:
**	    "entry",
**          or NULL if only the user-declared header element was found.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 CL.
**      22-jul-87 (mmm)
**          Upgraded UNIX CL to Jupiter and 5.0 CL standards.
*/

/* ARGSUSED */

QUEUE   *
QUremove(entry)
QUEUE   *entry;
{
    register QUEUE  *e = entry;

    e->q_prev->q_next = e->q_next;
    e->q_next->q_prev = e->q_prev;

    return (e == e->q_prev ? NULL : e);
}


# ifndef NT_GENERIC
/*{
** Name: QUr_init() - Initialize a relative pointer queue.
**
** Description:
**      "q" is a pointer to the header element of a queue.  Perform all
**      necessary initializations to set up a queue.
**
** Inputs:
**      q                               Pointer to head element in queue.
**
** Outputs:
**      none
**
**	Returns:
**	    VOID
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-Nov-97 (bonro01)
**          Added Relative queue functions to chain cross-process semaphores
*/
VOID
QUr_init(q)
QUEUE	*q;
{
	q->q_next = (QUEUE *)0;
	q->q_prev = (QUEUE *)0;
}

/*{
** Name: QUr_insert() - Insert element in relative queue.
**
** Description:
**      Insert element "entry" immediately after queue pointer "q".  The
**      procedure returns the value of "entry". 
**
** Inputs:
**      entry                           QUEUE element to insert into queue.
**      q                               Where to insert "entry".  "entry" will
**                                      be inserted immediately after this
**                                      QUEUE element.
**
** Outputs:
**      none
**
**	Returns:
**	    "entry"
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-Nov-97 (bonro01)
**          Added Relative queue functions to chain cross-process semaphores
*/
/* ARGSUSED */ 


QUEUE   *
QUr_insert(entry, next)
QUEUE   *entry;
QUEUE   *next;
{
	QUEUE  *e = entry;
	QUEUE  *p = next;
	QUEUE  *m;
	char key[256];

	MEget_seg(e, &key, &m);
    e->q_next = p->q_next == 0 ? 0 : (QUEUE *) ((char *)m+(long)p->q_next - (long)e);
    e->q_prev = (QUEUE *)0;
    if ( p->q_next == 0 )
        p->q_prev = (QUEUE *)((char *)e - (char *)m);
    else {
        ((QUEUE *)((char *)m+(long)p->q_next))->q_prev = (QUEUE *)((char *)e - (long)((char *)m+((long)p->q_next)));
    }
    p->q_next = (QUEUE *)((char *)e - (char *)m);

	return (e);
}

/*{
** Name: QUr_remove() - Remove element from relative queue.
**
** Description:
**      "entry" is a pointer to an element in a queue.  Remove and return a
**      pointer to that element, or NULL if only the user-declared header
**      element was found. 
**
** Inputs:
**      entry                           QUEUE element to remove from queue.
**
** Outputs:
**      none
**
**	Returns:
**	    "entry",
**          or NULL if only the user-declared header element was found.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-Nov-97 (bonro01)
**          Added Relative queue functions to chain cross-process semaphores
*/

/* ARGSUSED */

QUEUE   *
QUr_remove(entry, next)
QUEUE   *entry;
QUEUE   *next;
{
	void *m;
	register QUEUE  *e = entry;
	register QUEUE  *p = next;

    if ( e->q_next == 0 && e->q_prev == 0 )
    {
        p->q_next = 0;
        p->q_prev = 0;
    }
    else if ( e->q_next == 0 )
    {
        p->q_prev = (QUEUE *)((char *)p->q_prev + (long)e->q_prev);
        ((QUEUE *)((char *)e+(long)e->q_prev))->q_next = 0;
    }
    else if ( e->q_prev == 0 )
    {
        p->q_next = (QUEUE *)((char *)p->q_next + (long)e->q_next);
        ((QUEUE *)((char *)e+(long)e->q_next))->q_prev = 0;
    }
    else
    {
        ((QUEUE *)((char *)e+(long)e->q_prev))->q_next = (QUEUE *)((long)(((QUEUE *)((char *)e+(long)e->q_prev))->q_next) + (long)e->q_next);
        ((QUEUE *)((char *)e+(long)e->q_next))->q_prev = (QUEUE *)((long)(((QUEUE *)((char *)e+(long)e->q_next))->q_prev) + (long)e->q_prev);
    }

	return (p->q_next == 0 && p->q_prev == 0 ? 0 : e);
}
# endif /* ! NT_GENERIC */
