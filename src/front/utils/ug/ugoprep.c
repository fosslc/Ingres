/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
/* Needed if FRS operators ever directly referenced 
#include	<adf.h>
#include	<ft.h>
#include	<fmt.h>
#include	<frame.h>
*/

/**
** Name:	ugoprep.c -	Front-End Return (FRS) Operator Representation.
**
** Description:
**	Contains the routine used to map between FRS query operators and
**	their textual representation.  Defines:
**
**	iiugOpRep()	return FRS operator representation.
**
** History:
**	Revision  6.1  88/08/03  wong
**	Initial revision.
**	01/04/89 (dkh) - Removed "READONLY" qualification on "_Ops"
**			 declaration to regain sharability of ii_libqlib on VMS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:	iiugOpRep() -	Return FRS Operator Representation.
**
** Description:
**	Returns the textual representation of a query operator for a field.
**	(Somewhat backwards since the user originally typed in this string!)
**
** Inputs:
**	op	{nat}  FRS query operator, fdNOP, fdEQ, etc.
**
** Returns:
**	{char *}  Reference to string representing query operator.
**
** History:
**	08/03/88 (jhw) -- Written.
**	01/04/89 (dkh) - Removed "READONLY" qualification on "_Ops"
**			 declaration to regain sharability of ii_libqlib on VMS.
*/

static char	*_Ops[] = {
		ERx(""),	/* fdNOP */
		ERx("="),	/* fdEQ */
		ERx("!="),	/* fdNE */
		ERx("<"),	/* fdLT */
		ERx(">"),	/* fdGT */
		ERx("<="),	/* fdLE */
		ERx(">=")	/* fdGE */
};

char *
iiugOpRep ( op )
i4	op;
{
	return ( op <= 0  ||  op >= (sizeof(_Ops)/sizeof(char *)) )
		? _Ops[0] : _Ops[op];
}
