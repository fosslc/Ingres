/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<ossrt.h>

/**
** Name:    ossrt.c -	OSL Parser Sort List Module.
**
** Description:
**	Contains routines used to create and maintain sort lists for
**	the OSL parser.  (This also includes key and location/relation
**	specifications.)  Defines:
**
**	osmksrtnode()	make a sort node.
**	ostrvsrtlist()	traverse sort/key list.
**	osevalsklist()	evaluate sort/key list.
**	ossrtfree()	free sort/key list.
**
** History:
**	Revision 6.4
**	03/09/91 (emerson)
**		Integrated DESKTOP porting changes
**		(Changed identifier 'free' to 'oslfree').
**
**	Revision 6.0  87/06  wong
**	Abstracted 'ostrvsrtlist()' from 'osevalsklist()'.
**	Deleted 'ossrtcount()'.
**
**	Revision 5.1  86/10/17  16:15:57  wong
**	Modifed for OSL translator.  (86/09)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static FREELIST		*listp = NULL;		/* list pool */
static OSSRTNODE	*oslfree = NULL;	/* free list */

/*{
** Name:    osmksrtnode() -	Create Sort/Key Specification Node.
**
** Description:
**	Create a sort/key specification list element and fill it in with
**	the input values.
**
** Inputs:
**	name		{OSNODE *}  Name of the column on which to sort.
**	direction	{OSNODE *}  Direction (ascending or descending) of sort.
**
** Returns:
**	{OSSRTNODE *}  A sort list element.
**
** History:
**	09/86 (jhw) -- Modified for OSL translator.
*/
OSSRTNODE *
osmksrtnode (name, direction)
OSNODE	*name;
OSNODE	*direction;
{
    register OSSRTNODE	*sn;

    /*
    ** Allocate a sortnode.
    */
    if (oslfree != NULL)
    { /* get from free list */
	sn = oslfree;
	oslfree = oslfree->srt_next;
    }
    else if ((listp == NULL &&
	    (listp = FElpcreate(sizeof(OSSRTNODE))) == NULL) ||
		(sn = (OSSRTNODE *)FElpget(listp)) == NULL)
    {
	osuerr(OSNOMEM, 1, ERx("osmksrtnode"));
    }

    sn->srt_name = name;
    sn->srt_direction = direction;
    sn->srt_next = sn;

    return sn;
}

/*{
** Name:    ostrvsrtlist() -	Traverse a DML Sort/Key List.
**
** Description:
**	Traverse a database sort list applying the input function to each
**	element of the list.  The sort list is assumed to be a circular
**	linked list starting with the last element.
**
** Input:
**	list	{OSTLIST *}  The circular linked sort list.
**	ev_func	{nat (*)()}  The evaluation function.
**	dml	{DB_LANG}  DML language, DB_QUEL or DB_SQL.
**
** History:
**	06/87 (jhw) -- Abstracted from 'osevalsklist()' and added 'dml'
**			parameter to specify 'separator'.
*/
VOID
ostrvsrtlist (list, ev_func, dml)
register OSSRTNODE	*list;
register i4		(*ev_func)();
DB_LANG			dml;
{
    register OSSRTNODE	*lp;
    register char	*separator = dml == DB_QUEL ? ERx(":") : ERx(" ");

    if ((lp = list) == NULL)
	return;

    list = list->srt_next;	/* set to first */
    do
    {
	lp = lp->srt_next;
	(*ev_func)(lp->srt_name, lp->srt_direction, separator,
		(lp->srt_next != list)
	);
    } while (lp->srt_next != list);
}

/*{
** Name:    osevalsklist() -	Evaluate a DB Sort/Key List.
**
** Description:
**	Evaluate a database sort list by applying the input evaluation
**	function to each element of the list.  The sort list is assumed
**	to be a circular linked list starting with the last element.
**
**	This routine frees the sort list (by adding it to the free list.)
**
** Input:
**	list	{OSTLIST *}  The circular linked sort list.
**	ev_func	{nat (*)()}  The evaluation function.
**	dml	{DB_LANG}  DML language, DB_QUEL or DB_SQL.
**
** History:
**	09/86 (jhw) -- Written.
**	06/87 (jhw) -- Abstracted list walker to 'ostrvsrtlist()' and
**			added 'dml' parameter.
*/
VOID
osevalsklist (list, ev_func, dml)
register OSSRTNODE	*list;
register i4		(*ev_func)();
DB_LANG			dml;
{
    if (list == NULL)
	return;

    ostrvsrtlist(list, ev_func, dml);

    { /* add to free list */
	register OSSRTNODE	*first = list->srt_next;

    	list->srt_next = oslfree;
    	oslfree = first;
    }
}

/*{
** Name:    ossrtfree() -	Free a DB Sort/Key Specification List.
**
** Description:
**	This routine frees a database sort/key specification list by
**	adding its elements to the free list.
**
** Input:
**	list	{OSSRTNODE *}  The circular linked sort list.
**
** History:
**	09/86 (jhw) -- Written.
*/
VOID
ossrtfree (list)
register OSSRTNODE	*list;
{
    register OSSRTNODE	*first;

    first = list->srt_next;
    list->srt_next = oslfree;
    oslfree = first;
}
