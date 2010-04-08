/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
NO_OPTIM = vax_vms
*/
#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<me.h>
#include	<ug.h>
#include	"rbftype.h"
#include	"rbfglob.h"

/*
** This files contains routines to manipulate the Sections list.
**
**	History:
**		01-sep-89 (cmr)	written.
**		22-nov-89 (cmr)	added another arg to most routines which is the
**			address of the List; make it more general purpose.
**		30-jan-90 (cmr)	code to deal with new Sec_node field sec_under.
**		15-mar-90 (cmr) added sec_list_unlink().
**		19-oct-1990 (mgw)
**			Fixed include of local rbftype.h and rbfglob.h to ""
**			instead of <>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-jan-2004 (abbjo03)
**	    Add NO_OPTIM for vax_vms (actually axm_vms), otherwise we get an
**	    ACCVIO in sec_list_insert().
**      25-apr-2005 (zhahu02)
**          Updated sec_list_insert for vms to avoid accvio (INGCBT565/114394).
**	22-Jul-2005 (hanje04)
**	    Backed out bogus X-integ (INGCBT565/114394).
*/

Sec_node *
sec_node_alloc( type, seq, y )
i4	type;
i4	seq;
i4	y;
{
	Sec_node	*n;
	STATUS		status;

	if ( (n = (Sec_node *)MEreqmem((u_i4)1, 
		(u_i4)sizeof(Sec_node), TRUE, (STATUS *)NULL)) == NULL )
        {
		IIUGbmaBadMemoryAllocation(ERx("sec_node_alloc"));
	}

	n->sec_type = type;
	n->sec_brkseq = seq;
	n->sec_y = y;
	n->sec_under = 'n';
	n->prev = n->next = NULL;

	return(n);
}

VOID
sec_list_append( node, secList )
Sec_node	*node;
Sec_List	*secList;
{
	Sec_node	*t;

	if ( t = secList->tail )
	{
		t->next = node;
		node->prev = t;
	}	
	else
		secList->head = node;

	secList->tail = node;
	secList->count++;
}

VOID
sec_list_insert( n, secList )
Sec_node	*n;
Sec_List	*secList;
{
	Sec_node 	*p = secList->head;
	i4		section = n->sec_type;

	while ( p && (n->sec_type >= p->sec_type) )
	{
		if (( n->sec_type == p->sec_type ) /* brk hdr/ftr */
			&& (n->sec_brkseq < p->sec_brkseq ) )
			break;
		p = p->next;
	}

	if ( !p )
		sec_list_append( n, secList );
	else
	{
		if ( !p->prev )
		{
			secList->head->prev = n;
			n->next = secList->head;
			secList->head = n;
		}
		else
		{
			n->next = p;
			n->prev = p->prev;
			p->prev->next = n;
			p->prev = n;
		}
		secList->count++;
	}
}
VOID
sec_list_unlink( n, secList )
Sec_node	*n;
Sec_List	*secList;
{
	secList->count--;

	if ( n->prev )
		n->prev->next = n->next;
	else
		secList->head = n->next;

	if ( n->next )
		n->next->prev = n->prev;
	else
		secList->tail = n->prev;

}

VOID
sec_list_remove( n, secList )
Sec_node	*n;
Sec_List	*secList;
{

	sec_list_unlink( n, secList );
	MEfree( n );
}

VOID
sec_list_remove_all( secList )
Sec_List	*secList;
{
	Sec_node	*p, *q;

	p = secList->head;

	while ( p )
	{
		q = p;
		p = p->next;
		MEfree(q);
	}
	secList->head = secList->tail = NULL;
	secList->count = 0;
}

Sec_node  *
sec_list_find( section, srtseq, secList )
i4	section;
i4	srtseq;
Sec_List *secList;
{
	Sec_node	*n = secList->head;

	while ( n )
	{
		if ( n->sec_type == section && n->sec_brkseq == srtseq )
			return( n );
		n = n->next;
	}
	return( NULL );
}
