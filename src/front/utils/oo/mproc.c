/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<oodefine.h>
# include	<oosymbol.h>
# include	<ooptab.h>

/**
** Name:	mproc.qc - dynamic method procedure lookup
**			by symbol (procedure name).
**
** Description:
**
**	The file defines Mproc() which is only needed in
**	object-based program which do NOT pre-load static
**	object classes with initialized method collections.
**	
** History:
**	Revision 4.0  86/01  peterk
**	Initial revision.
**      03-Feb-96 (fanra01)
**          Changed extern to GLOBALREF.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF procTabEntry	iiOOProcTable[];
GLOBALREF procTabEntry	*iiOOEndProcTable;

OOID 
(*Mproc(opnum))()
i4	opnum;
{
	register procTabEntry	*p;
	
	for ( p = iiOOProcTable ; p < iiOOEndProcTable ; ++p )
	{
		if (p->opid == opnum)
			return p->pentry;
	}
	return NULL;
}

char *
Msym(f)
OOID	(*f)();
{
	register procTabEntry	*p;

	for ( p = iiOOProcTable ; p < iiOOEndProcTable ; ++p )
		if (f == p->pentry)
			return p->psymbol;
	return NULL;
}
