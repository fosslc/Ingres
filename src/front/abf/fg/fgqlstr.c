/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<fglstr.h>
# include	<erfg.h>
# include       <er.h>

/**
** Name:	fgqlstr.c -	Allocate an LSTR structure for a query
**
** Description:
**	This file defines:
**
**	IIFGaql_allocQueryLstr - Allocate LSTR for query
**
** History:
**	15 may 1989 (Mike S)	Initial version
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN VOID 	IIFGqerror();
FUNC_EXTERN LSTR 	*IIFGals_allocLinkStr();

/*{
** Name:	IIFGaql_allocQueryLstr - Allocate LSTR for query
**
** Description:
**	Allocate an LSTR for a QUERY. If allocation fails, call
**	IIFGqerror to display the error.
**
** Inputs:
**	parent		LSTR *	parent structure
**	follow		i4	how to end line	
**
** Outputs:
**	none
**
**	Returns:
**			LSTR *	Allocated structure
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	15 May 1989 (Mike S)	Initial version
*/
LSTR *
IIFGaql_allocQueryLstr(parent, follow)
LSTR 	*parent;
i4	follow;
{
	LSTR *ptr;

	ptr = IIFGals_allocLinkStr(parent, follow);
	if (ptr == NULL)
		IIFGqerror(E_FG0022_Query_NoDynMem, FALSE, 0);
	return(ptr);
}
