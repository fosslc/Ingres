/*
** Copyright 1990, 2008 Ingres Corporation
*/
#ifndef EXHDEF_H
#define EXHDEF_H

/*
**	VMS Exit handler defintion
**
** History:
**	26-nov-2008 (joea)
**	    Add wrapper to prevent multiple inclusion.
**      08-oct-2008 (stegr01)
**          Add member alignment pragmas*/

#pragma member_alignment save
#pragma nomember_alignment

typedef struct _exh$
{
	struct _exh$	*exh$gl_link;
	long		(*exh$g_func)();
	long		exh$l_argcount;
	long		*exh$gl_value;
	long		exh$l_status;
}	$EXHDEF;

#pragma member_alignment restore

#endif
