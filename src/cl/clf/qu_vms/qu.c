/*
** Copyright (c) 1986, Ingres Corporation
*/

#include    <compat.h> 
#include    <gl.h>
#include    <qu.h>  

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
**  History:    $Log-for RCS$
**      16-sep-86 (thurston)
**          Upgraded to Jupiter and 5.0 CL standards.
**      16-jul-93 (ed)
**	    added gl.h
**/


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
*/

VOID
QUinit(q)
QUEUE	*q;
{
	q->q_next = q;
	q->q_prev = q;
	return;
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
*/

QUEUE	*
QUinsert(node, next)
QUEUE	*node;
QUEUE	*next;
{
#	define	insque_default
/*
# 	ifdef vax11
		asm("QUinsert *4(ap),*8(ap)");
		asm("movl 4(ap),r0");
#		undef insque_default
#	endif
*/
#	ifdef insque_default
	{
		register QUEUE	*e = node;
		register QUEUE	*p = next;

		e->q_next = p->q_next;
		e->q_prev = p;
		p->q_next->q_prev = e;
		p->q_next = e;
		return (e);
	}
#	endif
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
*/

QUEUE	*
QUremove(node)
QUEUE	*node;
{
# 	define	remque_default
/*
#	ifdef vax11
		asm("QUremove *4(ap),r0");
		asm("bvc temp");
		asm("clrl r0");
		asm("temp:");
#		undef remque_default
#	endif	
*/
#	ifdef	remque_default
	{
		register QUEUE	*e = node;

		e->q_prev->q_next = e->q_next;
		e->q_next->q_prev = e->q_prev;
		return (e == e->q_prev ? 0 : e);
	}
#	endif
}
