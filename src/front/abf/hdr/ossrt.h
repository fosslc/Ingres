/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    ossrt.h -	OSL Parser Sort List Module Interface Definitions File.
**
** Description:
**	This file contains the definition of the sort specification structure
**	for the OSL parser that holds sort/order specifications (or key) for
**	DML statements.  A sort/order or key list is a circular linked list
**	referenced by the last element in the list.
**
** History:
**	Revision 6.0  87/05  wong
**	Added support for "order by" clause.
**
**	Revision 5.1  86/10/17  16:35:09  arthur
**	Extracted from "osquery.h".  86/09  wong
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
** Name:    OSSRTNODE -	OSL Parser Sort List Element/Key Structure Definition.
**
** Description:
**	Defines the structure of a sort (or order) list element or
**	key specification.
*/
typedef struct _ossrtn
{
    OSNODE	    *srt_name;	/* name of column
				** (for key or on which to sort.)
				*/
    OSNODE	    *srt_direction;	/* sort direction or relation name */
    u_i4	    srt_type : 2;	/* sort or order semantics */
#define			SRT_SORT 1
#define			SRT_ORDER 2
    struct _ossrtn  *srt_next;
} OSSRTNODE;

/* Function Declarations */

OSSRTNODE	*osmksrtnode();
