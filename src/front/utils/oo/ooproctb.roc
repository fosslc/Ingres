/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<er.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<ooptab.h>
#include	"ooproc.h"

/**
** Name:	ooproctb.roc -	Procedure table initialization
**
** Description:
**	Generated automatically by classout.
**
** History:
**	Revision 4.0  86/01  peterk
**	Initial revision
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**/

/*
**	ooproctb.c -- procedure table initialization
*/

GLOBALDEF procTabEntry iiOOProcTable[] = {
	{0,	ERx(""),	NULL},
};
GLOBALDEF procTabEntry	*iiOOEndProcTable =
	&iiOOProcTable[(sizeof iiOOProcTable)/(sizeof iiOOProcTable[0])];
