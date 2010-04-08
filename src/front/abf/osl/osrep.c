/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<me.h>
#include	<st.h>
#include	<er.h>
#include	<oserr.h>

/**
** Name:    osrep.c -	OSL Scanner Replacement Line Module.
**
** Description:
**	Contains routines used by the scanner to save replacement lines.
**	Defines:
**
**	osrepfindline(n)
**	osreprm()
**	osrepaddline()
**
** History:
**	Revision 5.1  86/11/04  15:19:13  wong
**	Call 'oscerr()' directly for memory error.
**	Extracted from "scandump.c".  86/10/17
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Procedures making up replace mode.
** These buffer up lines in the following structure.
*/

typedef struct rep_line {
	i4		l_lineno;
	struct rep_line	*l_next;
	char		l_line[1];	/* This is really the size of the line*/
} REP_LINE;

/*
** These two pointers point to the head and the tail of the
** list of lines.
*/

static REP_LINE	*rep_start,
		*rep_end;

/*
** osrepalloc
*/

static REP_LINE *
osrepalloc(l, n)
char	*l;
i4	n;
{
    register REP_LINE	*ret;
    STATUS		stat;

    /*
    ** The total size allocated is size of the structure
    ** (which) already has room for one character in it, and the
    ** size of the string.
    */
    if ((ret = (REP_LINE *)MEreqmem(0, sizeof(*ret)+STlength(l), TRUE, &stat))
		== NULL)
	osuerr(OSNOMEM, 1, ERx("osrepalloc()"));
    STcopy(l, ret->l_line);
    ret->l_lineno = n;
    return ret;
}
	
/*
** osrepfree
*/
static VOID
osrepfree(l)
REP_LINE	*l;
{
	MEfree(l);
}


/*{
** osrepaddline
**
**	Add a line to the list.
*/
osrepaddline(l, lineno)
char	*l;
i4	lineno;
{
	REP_LINE	*n;

	n = osrepalloc(l, lineno);
	if (rep_start == NULL)
		rep_start = n;
	else
		rep_end->l_next = n;
	rep_end = n;
}

/*{
** osreprm
**
**	Remove all lines that are less then the passed
**	line number.
*/
osreprm(n)
i4  n;
{
	REP_LINE	*l;

	while (rep_start != NULL && rep_start->l_lineno < n)
	{
		l = rep_start;
		rep_start = l->l_next;
		osrepfree(l);
	}
}

/*{
** osrepfindline
**
**	Find a line and return a pointer to its character rep.
*/
char *
osrepfindline (n)
i4	n;
{
    register REP_LINE	*l;

    for (l = rep_start; l != NULL && l->l_lineno != n; l = l->l_next)
	;
    return l == NULL ? NULL : l->l_line;
}
