/*
**	trmdmp.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	trmdmp.c - Dump a trim structure.
**
** Description:
**	Dump a trim structure to "stdout".  File defines:
**	- FDtrmdmp - Dump a trim structure.
**
** History:
**	2/16/82 (ps)	written.
**	02/15/87 (dkh) - Added header.
**	08/14/87 (dkh) - ER changes.
**	12-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	"fdfuncs.h"
# include	<si.h>
# include	<er.h>


/*{
** Name:	FDtrmdmp - Dump a trim structure.
**
** Description:
**	Print out a trim structure while debugging.
**
** Inputs:
**	t	Pointer to a TRIM structure.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	2/16/82 (ps)	written.
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDtrmdmp(t)
register TRIM	*t;
{
	SIprintf(ERx("\tAddress of TRIM structure:%p\r\n"), t);

	if (t == NULL)
		return;

	SIprintf(ERx("\t\ttrmx:%d"), t->trmx);
	SIprintf(ERx("\t\ttrmy:%d\r\n"), t->trmy);

	if (t->trmstr != NULL)
	{
		SIprintf(ERx("\t\ttrmstr:%s\r\n"), t->trmstr);
	}

	SIflush(stdout);
	return;
}
