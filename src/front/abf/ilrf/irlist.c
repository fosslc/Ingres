/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<er.h>
# include	"irstd.h"
# include	"eror.h"
# ifdef PCINGRES
# include	<ex.h>
# endif

/**
** Name:	irlist.c - ILRF list utilities
**
** Description:
**	This file defines several small routines to manipulate
**	the frame information lists.  Only used from within ILRF.
**
**	The lists are doubly linked and circular.  ILRF code should pass through
**	these utilities to do anything which adds, removes or
**	rearranges frame information lists.  Callers may directly
**	scan the lists.  Lists are refered to by QUEUE structures
**	maintaining the head and tail of the list.
**
**	This file hides the list pool for IR_F_CTL nodes.  All passed in
**	arguments are legitimate list members.	list_add is the only legal
**	way to add a list element, and it explicitly chains it onto a list.
**	An unlinked pointer is no longer legal for anything.
**
**	The partioning of functionality in these routines may seem a bit
**	wierd - they are intended to be convenient for ILRF use, not as
**	generic list routines.
**
**	Routines:
**		list_add - add a new node to a list.
**		list_unlink - remove a node from a list.(in seperate file
**			for overlaying purposes)
**		list_swap - swap a node from one list to the other.
**		list_head - move a node to the head of its list.
**		list_tail - move a node to the tail of its list.
**		list_create - create a list.
**		list_zero - remove all nodes from a list.
**		list_free_pool - free the memory IIOR_Pool.
**
** History:
**	8/86 (rlm)	written
**	03/16/88 (dkh) - Added EXdelete call to conform to coding spec.
*/

#ifdef PCINGRES
extern EX IIAMmeMemError();
extern bool ME_recoverable;
#endif

GLOBALDEF FREELIST *IIOR_Pool ZERO_FILL;	/* allocation pool */

/*
** Add an element to a list, filling in its new predecessor.  The only way
** to get a new node.
**
** We put it on the tail.  This reflects knowledge that we expect to
** call list_tail afterwards, however, there is to be no assumption
** by the calling code as to where this routine places an element.
**
** In the PCINGRES environment, we have to handle exceptions to recover
** from allocation failures, first setting a flag to indicate this action
** from the ME routines rather than a syserr().
*/
IR_F_CTL *
list_add(list)
QUEUE *list;
{
	IR_F_CTL *frm;
# ifdef PCINGRES
	bool oldreco;
	EX_CONTEXT context;

	oldreco = ME_recoverable;
	ME_recoverable = TRUE;
	if (EXdeclare(IIAMmeMemError, &context) != OK)
	{
		EXdelete();
		ME_recoverable = oldreco;
		return (NULL);
	}
# endif

	if ((frm = (IR_F_CTL *) FElpget(IIOR_Pool)) != NULL)
	{
		/* insert frm at the end of the queue (after this call,
		   list->q_prev will point to frm) */
		QUinsert(&frm->fi_queue, list->q_prev);
	}

	return(frm);
}

/*
** put an element ALREADY IN THE LIST up to the head.
*/
list_head(list, frm)
QUEUE *list;
IR_F_CTL *frm;
{
	/* remove from current position */
	QUremove(&frm->fi_queue);

	/* insert frm at the head of the queue (after this call, list->q_next
	   will point to frm) */
	QUinsert(&frm->fi_queue, list);
}

list_tail(list, frm)
QUEUE		*list;
IR_F_CTL	*frm;
{
	/* remove from current position */
	QUremove(&frm->fi_queue);

	/* insert frm at the end of the queue (after this call, list->q_prev
	   will point to frm) */
	QUinsert(&frm->fi_queue, list->q_prev);
}

/*
** initialize a new list, create pool for list nodes on first call
*/
list_create(list)
QUEUE *list;
{
	if (IIOR_Pool == (FREELIST *)NULL)
	{
		if ((IIOR_Pool = FElpcreate(sizeof(IR_F_CTL))) == NULL)
			IIUGerr (S_OR000D_pool_alloc, UG_ERR_FATAL, 0);
	}
	QUinit(list);
}

/*
** return all the nodes in a list to the pool
*/
list_zero(list)
QUEUE *list;
{
	IR_F_CTL *frm,*next;

	for (frm = (IR_F_CTL *)list->q_next; (QUEUE *)frm != list; frm = next)
	{
		next = (IR_F_CTL *)frm->fi_queue.q_next;

		/* don't need to do QUremove since we are releasing the
		   entire list */
		FElpret(IIOR_Pool,frm);
	}
	QUinit(list);
}

/* free the pool itself */
void
list_free_pool()
{
	/* free the pool but only if its been allocated */
	if(IIOR_Pool != (FREELIST *)NULL)
		FElpdestroy(IIOR_Pool);

	/* mark it as free */
	IIOR_Pool = (FREELIST *)NULL;

#ifdef PCINGRES
	/* free the tags list FE_tags */
	FEfreeFEtags();
#endif
}

