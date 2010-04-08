# include 	<compat.h>
# include	<si.h>
# include	<er.h>
# include	<equel.h>
# include	<equtils.h>
# include	<me.h>
# include	<ereq.h>

/*
+* Filename:	QMAN.C
** Purpose:	Queue Manger.
**
** Defines:	q_init( q, size )	- Initialize queue.
**		q_enqueue( q, elm )	- Add an element to a queue.
**		q_rmqueue( q )		- Remove an element form a queue.
**		q_walk( q, func )	- Walk a queue calling func.
**		q_free( q, func )	- Free queue elements to freelist.
**		q_newelm( q )		- Fetch new element from free list.
**		q_dump( q, qname )	- Dump queue links for debugging.
**
** Exports:	Macros to get at pointers in queue - Require casting.
**		q_first( cast, q )	- Macro for first element in queue.
**		q_follow( cast, elm )	- Macro for next element in queue.
**		q_new( cast, elm, q )	- Macro to get new elem from free list.
** Locals:	
-*		q_datadump( title, q )	- Static debug queue dumper.
**
** Notes:
**	1.  To use the queue manager the objects that you want queued should
**	be the first in the structure.  This is because of the allocation
**	mechanism used in q_newelm().
**	2.  Also because the queue routines will be used inside other
**	structures it goes through a macro text processor (see q.h) to return
**	to callers the original type of their elements.
**	3.  Each queued list of objects should be attached to a freelist for
**	faster and more efficient use. The freelist is also a queue, and is
**	managed by the queue manager.
**
** History:	
**		24-sep-1984	- Written (ncg)
**		13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	12-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**      18-mar-96 (thoda04)
**              Corrected cast of MEreqmem()'s tag_id (1st parm).
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define 	MAXQELM		 20

static	void	q_datadump();		/* Local to dump all queues */

/*
+* Procedure:	q_init 
** Purpose:	Initialize caller's queue.  Just sets members to QNULL.
**
** Parameters: 	qdesc	  - Q_DESC * - Pointer to queue descriptor,
**		qelemsize - i4       - Size of each queue element.
-* Returns: 	None
*/

void
q_init( qdesc, qelmsize )
register Q_DESC *qdesc;
i4		qelmsize;
{
    if ( qdesc != QDNULL )
    {
	qdesc->q_head = qdesc->q_tail = QNULL;
	qdesc->q_frlist = QNULL;  
	/* Force struct size alignment */
	qdesc->q_size = qelmsize + (qelmsize % sizeof(i4));
    }
}


/*
+* Procedure:	q_enqueue 
** Purpose:	Add a queue element to the end of a queue.  If nothing in
** 	       	the queue then make it the first.
**
** Parameters: 	qdesc - Q_DESC * - Pointer to queue descriptor,
**		qelm  - Q_ELM *	 - Pointer to element to add.
-* Returns: 	None
*/

void
q_enqueue( qdesc, qelm )
register Q_DESC	*qdesc;
register Q_ELM	*qelm;
{
    if ( qdesc == QDNULL || qelm == QNULL )
	return;
    if ( qdesc->q_head == QNULL )
    {
    	qdesc->q_head = qdesc->q_tail = qelm;
    }
    else
    {
	qdesc->q_tail->q_next = qelm;
	qdesc->q_tail = qelm;
    }
    qelm->q_next = QNULL;
}

/*
+* Procedure:	q_rmqueue 
** Purpose:	Remove a queue element from the front of the queue.
**
** Parameters: 	qdesc - Q_DESC * - Pointer to queue descriptor,
**		qelm  - Q_ELM ** - Result for removed queue element pointer.
-* Returns: 	None
**
** If nothing in the queue return NULL.
*/

void
q_rmqueue( qdesc, qelm )
register Q_DESC	*qdesc;
register Q_ELM  **qelm;
{
    register Q_ELM	*q;

    if ( qdesc == QDNULL )
    {
	q = QNULL;
    }
    else if ( (q = qdesc->q_head) != QNULL )
    {
	qdesc->q_head = qdesc->q_head->q_next;
	q->q_next = QNULL;			/* Disconnect for caller */
    } 
    *qelm = q;
}

/*
+* Procedure:	q_walk 
** Purpose:	Walk a queue and call a user supplied routine for each element.
**
** Parameters: 	qdesc - Q_DESC * - Pointer to queue descriptor,
**		qfunc - nat(*)() - Function to call when walking.
-* Returns: 	None
**
** Requires that the user supplied routine return a i4, which is zero (or
** FALSE) on error.
** This routine may not be that useful as it is called for each element 
** without the caller knowing the order of things.  For routines, where the
** queue order is important and modularity too, use q_first() and q_follow().
*/

void
q_walk( qdesc, qfunc )
register Q_DESC *qdesc;
i4		(*qfunc)();
{
    register Q_ELM	*q;
    i4			q_ok = TRUE;

    if ( qdesc != QDNULL )
    {
	for ( q = qdesc->q_head; (q != QNULL) && q_ok; q = q->q_next )
	    q_ok = (*qfunc)( q );
    }
}


/*
+* Procedure:	q_free 
** Purpose:	Free a queue onto a passed free list which is also a queue. 
**
** Parameters: 	qdesc - Q_DESC * - Pointer to queue descriptor,
**		qfunc - nat(*)() - Function (or Null) to call for each element 
**				   before freeing.
-* Returns: 	None
**
** Possibly reset all the values of the queued object.
** Requires that the caller has a local Q_DESC  freelist.
*/

void
q_free( qdesc, qfunc )
register Q_DESC *qdesc;
i4		(*qfunc)();
{
    if ( qdesc == QDNULL || qdesc->q_head == QNULL )
	return;

    if ( qfunc )
	q_walk( qdesc, qfunc );

    if ( qdesc->q_frlist != QNULL )			/* Free list > 0 */
        qdesc->q_tail->q_next = qdesc->q_frlist;	/* Q = Q + Free list */
    qdesc->q_frlist = qdesc->q_head;			/* Free list = Q */
    qdesc->q_head = qdesc->q_tail = QNULL;		/* Q = 0 */
}

/*
+* Procedure:	q_newelm 
** Purpose:	Get a new queue element and return it to caller.
**
** Parameters: 	qdesc - Q_DESC * - Pointer to queue descriptor,
** Returns: 	Q_ELM *		 - New element (to be filled, and hopefully 
-*				   added to queue).
**
** Attempt to pick the element off the caller's freelist.  If there are no
** more on the freelist then allocate MAXQELM's more for the freelist and return
** the first one. This should always be called via the q_new() macro.
** 
** Requires that the Q_ELM part is the very first part of the structure that
** is on the freelist.
*/

Q_ELM *
q_newelm( qdesc )
register Q_DESC	*qdesc;
{
    register  Q_ELM *q;
    register  i4     i;

    if ( qdesc->q_frlist == QNULL )
    {
	if ((qdesc->q_frlist = (Q_ELM *)MEreqmem((u_i2)0,
	    (u_i4)(MAXQELM * qdesc->q_size), TRUE, (STATUS *)NULL)) == NULL)
	{
	    er_write(E_EQ0001_eqNOMEM, EQ_FATAL, 2, 
		    er_na(MAXQELM * qdesc->q_size), ERx("q_newelm()"));
	}
	/* Make array of storage into linked list */
	for ( i = 1, q = qdesc->q_frlist; i < MAXQELM; i++, q = q->q_next )
	    q->q_next = (Q_ELM *) ( ((char *)q) + qdesc->q_size );
	q->q_next = QNULL;
    }

    q = qdesc->q_frlist;
    qdesc->q_frlist = q->q_next;
    q->q_next = QNULL;				/* Disconnect from list */
    return q;
}

/*
+* Procedure:	q_dump 
** Purpose:	Dump a queue (just the links) for debugging.
**
** Parameters: 	qdesc - Q_DESC * - Pointer to queue descriptor,
**		qname - char *   - Caller's name.
-* Returns: 	None
*/

# define  MAXQWORDS	5

void
q_dump( qdesc, qname )
register Q_DESC *qdesc;
char		*qname;
{
    register	FILE	*df = eq->eq_dumpfile;

    if ( qdesc == QDNULL )
	return;
    SIfprintf( df,  ERx("\nQ_DUMP of queue \"%s\":\n"), qname );
    SIfprintf( df,  ERx("Q->elmsize = %d, "), qdesc->q_size ); 
    SIfprintf( df,  ERx("Q->head = %p, Q->tail = %p, Q->freelist = %p\n"), 
	       qdesc->q_head, qdesc->q_tail, qdesc->q_frlist );

    q_datadump( ERx("Queue Data:"), qdesc->q_head );
    q_datadump( ERx("Queue Freelist:"), qdesc->q_frlist );
    SIflush( df );
}

/*
+* Procedure:	q_datadump 
** Purpose:	Dump real queue links of list of objects
**
** Parameters: 	title - char *   - Title of queue,
**		q     - Q_ELM *  - First queue element in list.
-* Returns: 	None
*/

static void
q_datadump( title, q )
char		*title;
register Q_ELM	*q;
{
    register FILE	*df = eq->eq_dumpfile;
    register i4	i;
    i4			wordcount = 0;

    SIfprintf( df, ERx("%s\n"), title );
    for ( i = 0; q != QNULL; i++, q = q->q_next )
    {
	SIfprintf( df, ERx("q[%d] = %p"), i, q );
	if ( q->q_next != QNULL )
	    SIputc( ',', df );
	if ( ++wordcount == MAXQWORDS )
	{
	    SIfprintf( df, ERx("\n") );
	    wordcount = 0;
	}
	else
	{
	    SIputc( '\t', df );
	}
    }
    if ( wordcount != 0 )
	SIfprintf( df, ERx("\n") );
    SIflush( df );
}
