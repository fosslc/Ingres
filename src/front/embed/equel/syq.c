# include	<compat.h>
# include	<er.h>
# include	<si.h>			/* for <equel.h> */
# include	<eqsym.h>
# include	<equel.h>
# include	<me.h>
# include	<ereq.h>

/*
+* Filename:	syq.c
** Purpose:	The symbol table queue manager.
**
** Defines:
**	sym_q_new	- Allocate a new node.
**	sym_q_free	- Free an allocated node.
**	sym_q_delete	- Remove a node from its chain and free it.
**	sym_q_empty	- Throw away the whole symbol table.
**	sym_q_append	- Append a node to a chain.
**	sym_q_remove	- Remove a node from a chain.
-*	sym_q_head	- Move a node to the head of its chain.
** (Data):
**	sy_qdfreelist	- Q_ELM *	- Queue free list head pointer.
**	sym_delval	- (*nat)()	- Pointer to deletion hook routine.
** Notes:
**	- We handle queues of symbol table entries.
**	- Inside this module we rename the st_hash field to be q_next.
**	- After an entry has been freed it is gone from its hash chain
**	  so we re-use the q_next field to point to the next entry on
**	  the free list.  We use a free list to reduce the number of
**	  MEreqmem calls we must make.
**	- If sym_delval is non-NULL then we call it when freeing a node.
**	  It will be passed the st_value field of the node.  This is for
**	  languages like PL/1 where the compiler allocs storage and points
**	  st_value to it.
**
** History:
**	19-Oct-84	- Initial version. (mrw)
**	29-jun-87 (bab)	Code cleanup.
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      19-mar-1996 (thoda04)
**          Corrected cast of MEreqmem()'s tag_id (1st parm).
**      06-Feb-96 (fanra01)
**          Made data extraction generic.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#define	MAXQELM	100
#define	Q_SIZE	sizeof(SYM)
#define	Q_ELM	SYM
#define	q_next	st_hash

GLOBALREF Q_ELM	*sy_qdfreelist;
GLOBALREF i4	(*sym_delval)();

# if defined(NT_GENERIC)
GLOBALREF SYM	*syHashtab[];
# else              /* NT_GENERIC */
extern SYM	*syHashtab[];
# endif             /* NT_GENERIC */

/*
+* Procedure:	sym_q_new
** Purpose:	Get a new queue element.
** Parameters:
**	None.
** Return Values:
-*	Q_ELM *	- Pointer to the newly allocated element.
** Notes:
**	- Check for out of memory failure and exit if so.
**	- Attempt to pick the element off our freelist.  If there are no
**	  more on the freelist then allocate MAXQELM's more for the freelist
**	  and return the first one.
**
** Imports modified:
**	None.
*/

Q_ELM *
sym_q_new()
{
    register  Q_ELM
		*q;
    register  i4
		i;
    GLOBALREF Q_ELM
	    *sy_qdfreelist;

    if (sy_qdfreelist == NULL)
    {
	if ((sy_qdfreelist = (Q_ELM *)MEreqmem((u_i2)0,
	    (u_i4)(MAXQELM * Q_SIZE), TRUE, (STATUS *)NULL)) == NULL)
	{
	    er_write( E_EQ0001_eqNOMEM, EQ_FATAL, 2, er_na(MAXQELM * Q_SIZE), 
		    ERx("sym_q_new()") );
	}

      /* Make array of storage into linked list */
	for (i=1, q=sy_qdfreelist; i<MAXQELM; i++, q=q->q_next)
	    q->q_next = (Q_ELM *) ( ((char *)q) + Q_SIZE );
	q->q_next = NULL;
    }

    q = sy_qdfreelist;
    sy_qdfreelist = q->q_next;
    q->q_next = NULL;				/* Disconnect from list */
    return q;
}

/*
+* Procedure:	sym_q_free
** Purpose:	Free the storage associated with a node.
** Parameters:
**	q_elm	- Q_ELM *	- Pointer to node to free.
** Return Values:
-*	None.
** Notes:
**	- For use when you've already removed it from its chain.
**	  (otherwise use "sym_q_delete").
**	- "q_elm" must have been allocated with "q_new".
**	- Simply put it at the head of the free list.
**	- If sym_delval is non-NULL, call it.
**
** Imports modified:
**	None.
*/

void
sym_q_free( q_elm )
Q_ELM	*q_elm;
{
    GLOBALREF Q_ELM
	    *sy_qdfreelist;

    q_elm->q_next = sy_qdfreelist;
    sy_qdfreelist = q_elm;
    if (sym_delval)
	_VOID_ (*sym_delval)( sym_g_vlue(q_elm) );
}

/*
+* Procedure:	sym_q_delete
** Purpose:	Remove "q_elm" from its chain and free it.
** Parameters:
**	q_elm		- Q_ELM *	- Node to delete.
**	q_before	- Q_ELM *	- Preceding node or NULL.
** Return Values:
-*	STATUS	- FAIL if we couldn't find it.
** Notes:
**	- If "q_before" is non-NULL it should be the predecessor of "q_elm".
**	- If "q_before" is NULL (or not the predecessor of "q_elm") then
**	  search from the begining  of the hash chain.
**	- "q_elm" must have been allocated with "q_new".
**
** Imports modified:
**	Modifies the free list.
*/

STATUS
sym_q_delete( q_elm, q_before )
Q_ELM	*q_elm,
	*q_before;
{
    if (sym_q_remove(q_elm, q_before) == OK)
    {
	sym_q_free( q_elm );
	return OK;
    }
    return FAIL;
}

/*
** sym_q_empty
**	- throw away the entire symbol table
/*
+* Procedure:	sym_q_empty
** Purpose:	Throw away the entire symbol table.
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
**	Free every node on the list.  Use this for starting over from scratch.
**
** Imports modified:
**	None.
*/

void
sym_q_empty()
{
    register u_i2
		h;
    register SYM
		*p, *next_p;
    void	sym_q_free();

    for (h=0; h<syHASH_SIZ; h++)
    {
	for (p=syHashtab[h]; p;)
	{
	    next_p = p->st_hash;
	    sym_q_free( p );
	    p = next_p;
	}
	syHashtab[h] = NULL;
    }
}

/*
+* Procedure:	sym_q_append
** Purpose:	Append the node to its hash chain.
** Parameters:
**	q_elm	- Q_ELM *	- The node to append
**	q_after	- Q_ELM *	- Its purported predecessor, or NULL.
** Return Values:
-*	None.
** Notes:
**	- Append "q_elm" after "q_after" (or at head of hash chain
**	  if "q_after" is NULL).
**	- "q_elm" must not already be on the chain!
**
** Imports modified:
**	Changes hash table links.
*/

void
sym_q_append( q_elm, q_after )
Q_ELM	*q_elm,
	*q_after;
{
    if (q_after == NULL)
    {
	register u_i2	h;

	h = q_elm->st_hval;
	q_elm->q_next = syHashtab[h];
	syHashtab[h] = q_elm;
    } else {
	q_elm->q_next = q_after->q_next;
	q_after->q_next = q_elm;
    }
}

/*
+* Procedure:	sym_q_remove
** Purpose:	Remove "q_elm" from its chain.
** Parameters:
**	q_elm		- Q_ELM *	- Node to delete.
**	q_before	- Q_ELM *	- Preceding node or NULL.
** Return Values:
-*	STATUS	- FAIL if we couldn't find it.
** Notes:
**	- If "q_before" is non-NULL it should be the predecessor of "q_elm".
**	- If "q_before" is NULL (or not the predecessor of "q_elm") then
**	  search from the begining  of the hash chain.
**	- "q_elm" had better be on that chain.
**
** Imports modified:
**	None.
*/

STATUS
sym_q_remove( q_elm, q_before )
Q_ELM	*q_elm,
	*q_before;
{
    if (q_before)
    {
	if (q_before->q_next != q_elm) 
	{
	    er_write( E_EQ0259_syQNOTSUCC, EQ_ERROR, 0 );
	    q_before = NULL;
	} else
	{
	    q_before->q_next = q_elm->q_next;
	}
    }
    if (q_before == NULL)
    {
	register u_i2	h = q_elm->st_hval;

	if (syHashtab[h] == q_elm)
	{
	    syHashtab[h] = q_elm->q_next;
	} else
	{
	    register Q_ELM	*q;

	    for (q=syHashtab[h]; q && q->q_next != q_elm; q=q->q_next);
	    if (q)
		q->q_next = q_elm->q_next;
	    else
		return FAIL;
	}
    }
    return OK;
}

/*
+* Procedure:	sym_q_head
** Purpose:	Move an element to the front of its queue.
** Parameters:
**	q_elm		- Q_ELM *	- Node to move.
** Return Values:
-*	STATUS	- FAIL if we couldn't find it.
** Notes:
**	Removes it from the queue and inserts it at the head; does
**	nothing if it's already at the head.
**
** Imports modified:
**	None.
*/

STATUS
sym_q_head( q_elm )
Q_ELM	*q_elm;
{
  /* if it's already at the head don't do anything */
    if (syHashtab[q_elm->st_hval] == q_elm)
	return OK;

    if (sym_q_remove(q_elm, NULL) == OK)
    {
	sym_q_append( q_elm, NULL );
	return OK;
    }
    return FAIL;
}
