/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>

GLOBALREF i4  (*rf_cl_func)();

/**
** Name:    rfrep.qc -	RBF specials for rreport writer.
**
** Description:
**	This files contains routines that services the needs of RBF
**	from report writer internal routines.  In otherwords, when RBF
**	uses a report writer routine, but needs that routine to do
**	something special that the report writer would not
**	normally do (like forms actions) it calls a routine in this
**	file indirectly.
**
**	rf_rep_init()	Initialize the RW's function pointers.
**
** History:
**	Revision 6.0  87/08  wong
**	Replaced 'rf_mes_func' and 'rf_message()' functionality with
**	'FEmsg()' calls.
**
**	Revision 4.0  85/11/05  joe
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

/*{
** Name:    rf_rep_init() -	Initialize RW's functions pointers.
**
** Description:
**	The RW has some functions pointers that it keeps for RBF.
**	These pointers are used to call RBF special functions that
**	the RW shouldn't call (things like forms statements).
**
** History:
**	5-Nov-1985 (joe)
**		First Written
*/

VOID
rf_rep_init()
{
	i4	FTclose();

	rf_cl_func = FTclose;
}
