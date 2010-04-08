/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>		/* 6-x_PC_80x86 */
#include	<pc.h> 
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>

/**
** Name:    fetmpname.c -	Front-End Utility Temporary Relation Name Module
**
** Description:
**	Contains the routine used to return a unique name for a temporary
**	relation or view.  Defines:
**
**	FEtempname()	return a unique name for a relation or view.
**
** History:
**	Revision 6.0  87/08/03  wong
**	Temporary names all now begin with "ii".
**
**	Revision 3.0  84/28/11  grant
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:    FEtempname() -  	Return a Unique Name for a Relation or View.
**
** Description:
**	Returns a unique name for a temporary relation or view with the
**	following format:
**
**		IInnnppppppppsss
**
**	where
**		nnn	is a serial number in decimal starting from 0 that is
**			incremented each time this routine is called.
**		ppppp*	is the process id in hexadecimal.
**		sss	is the suffix string passed in.
**
** Input:
**	name	{char *}  Buffer into which to put name (must be
**			  at least FE_MAXNAME in length.)
**	suffix	{char *}  String to distinguish calling frontend
**			  (e.g. "sql", "rep", "gbf")
**
** History:
**	Written: 11/28/84 (gac)
**	10/02/86 (sandyd) - Added "ii" prefix for use in distributed
**				database, since DBE parser chokes on "0"
**				tablenames.
**	08/03/87 (jhw) - Made previous change apply to all names.
*/

static i4	serialno = 0;

#define	SERIALLEN	3
#define	PIDLEN		8
#define	SUFFIXLEN	3

VOID
FEtempname (name, suffix)
char	*name;
char	*suffix;
{
    char	pidstring[20];

    PCunique(pidstring);
    STprintf(name, ERx("ii%0*d%.*s%.*s"),
	SERIALLEN, serialno++ % 1000, PIDLEN, pidstring, SUFFIXLEN, suffix
    );
}
