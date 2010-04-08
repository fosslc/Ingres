/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <me.h>
# include "distq.h"

/**
** Name:	list.c - linked list management
**
** Description:
**	This module contains the functions to construct the records
**	linklist.  The structure for that linklist is defined in the
**	records.sh include file.  It is a double link list in that from
**	one structure you can go to either the next structure or the
**	prior structure.
**
**	Init_list and new_node are called by read_q when building the
**	distribution queue in-memory list.  List_top and next_node are called
**	to traverse the list after it has been built.
**
**	Defines
**		init_list	- initialize list
**		new_node	- new node
**		list_top	- return the head of the list
**		list_bottom	- return the tail of the list
**		next_node	- next node
**		pop_list_bottom - return next node, reomve previous 'bottom'.
**
** History:
**	16-dec-96 (joea)
**		Created based on list.c in replicator library.
**	04-feb-97 (joea)
**		Eliminate 'marker' global.
**	26-mar-97 (joea)
**		Eliminate trim_node(). Merge get_list() into new_node().
**		Include distq.h instead of records.h.
**      22-Oct-2001 (inifa01) bug 105555 INGREP 97.
**		When the replicator server starts up, if the number of
**		transactions in the distribution queue is large
**		(exceeds QBM value) the server hangs.
**		Added new function 'pop_list_bottom()', to traverse and pop
**		the linked list from 'list_bottom'. [next_node() traverses
**		the list from 'list_top'.]
**/

/*
** all of these pointers may be null.  Null means that there is no top
** or bottom to the list.  If marker is null, it means that the bottom
** of the linklist has been reached.
*/

static RLIST	*top;		/* the pointer to the top of the linklist */
static RLIST	*bottom;	/* the pointer to the bottom of the linklist */
static RLIST	*atop;		/* the pointer to the top of the available list */
static RLIST	*abottom;	/* the pointer to the bottom of the available list */
static RLIST	*current;


FUNC_EXTERN void RSshutdown(STATUS status);


/*{
** Name:	init_list - initialize list
**
** Description:
**	Initializes the linklist, sets the global pointers to null.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
init_list()
{
	if (top)
	{
		if (atop)
		{
			atop->blink = bottom;
			bottom->link = atop;
			atop = top;
		}
		else
		{
			atop = top;
			abottom = bottom;
		}
	}
	else
	{
		atop = (RLIST *)NULL;
		abottom = (RLIST *)NULL;
	}
	top = (RLIST *)NULL;
	bottom = (RLIST *)NULL;

	/* top %d, atop %d */
	messageit(5, 1262, top, atop);
}


/*{
** Name:	list_top - return the head of the list
**
** Description:
**	Returns the top (start) of the list.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	A pointer to the head of the list.
**
** Side effects:
**	The static pointer current is also pointed to the head of the list.
*/
RLIST *
list_top()
{
	current = top;
	/* top %d, atop %d */
	messageit(5, 1263, top, atop);
	return (top);
}


/*{
** Name:	list_bottom - return the tail of the list
**
** Description:
**	Returns the bottom (end) of the list
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	A pointer to the tail of the list.
**
** Side effects:
**	The static pointer current is also pointed to the tail of the list.
*/
RLIST *
list_bottom()
{
	current = bottom;
	return (bottom);
}


/*{
** Name:	new_node - new node
**
** Description:
**	Create a new structure and append it to the bottom of the linklist.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	A pointer to the new node.
**
** Side effects:
**	Some of the static pointers above are changed.
*/
RLIST *
new_node()
{
	RLIST	*mark;

	/* top %d, atop %d */
	messageit(5, 1264, top, atop);

	if (atop)
	{
		mark = atop;
		atop = atop->link;
	}
	else
	{
		mark = (RLIST *)MEreqmem(0, sizeof(RLIST), TRUE,
			(STATUS *)NULL);
		if (mark == NULL)
		{
			/* Error Allocating Memory for Record List: Aborting */
			messageit(1, 1266);
			RSshutdown(FAIL);
		}
	}

	mark->link = (RLIST *)NULL;
	mark->blink = (RLIST *)NULL;

	if (bottom)	/* there is already a linklist */
	{
		bottom->link = mark;
		mark->blink = bottom;
		bottom = mark;
	}
	else		/* first structure in the linklist */
	{
		top = mark;
		bottom = mark;
	}

	/* top %d, atop %d, mark %d */
	messageit(5, 1265, top, atop, mark);
	return (mark);
}


/*{
** Name:	next_node - next node
**
** Description:
**	Move to the next structure in the linklist.  If at the bottom of the
**	linklist, return null.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	A pointer to the next node.
**
** Side effects:
**	The static pointer current is advanced.
*/
RLIST *
next_node()
{
	if (current)
		current = current->link;
	else
		current = top;

	return (current);
}

/*{
** Name:        pop_list_bottom - pop list bottom
**
** Description:
**      Move to the next structure in the linklist from 'bottom'.  If at
**      the top of the linklist, return null.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
** Returns:
**      A pointer to the next node.
**
** Side effects:
**      The static pointer current is advanced.
*/
RLIST *
pop_list_bottom()
{
        if (current) /* This should really always be true */
        {
                current = current->blink;
                current->link = (RLIST *)NULL;
                bottom = current;
        }
        else
                current = bottom;

        return (current);
}
