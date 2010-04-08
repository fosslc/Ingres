/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ooproc.h -	OC_OBJECT Procedure Declarations.
**
** Description:
**	Generated automatically by classout.
**
** History:
**	Revision 4.0  86/01  peterk
**	Initial revision.
**	Revision 6.2  89/07  wong
**	Added 'iioCOalloc()', 'iiooStrAlloc()' and 'iiooMemAlloc()'.  Removed
**	Graph method routine declarations, which are in "graphics!graf!grproc.h"
**
**	8-feb-93 (blaise)
**		Changed _flush, etc. to ii_flush because of conflict with
**		Microsoft C library.
**	21-Aug-2009 (kschendel) 121804
**	    Fix some of the function declarations to fix gcc 4.3 problems.
**/
OOID	iioCOalloc();
OOID	iioCOat();
bool	iioCOxatEnd();
OOID	iioCOpatPut();
char	*iioCOcurrentName();
OOID	iioCOdo();
OOID	iioCOfirst();
OOID	iioCOinit();
bool	iioCOisEmpty();
OOID	iioCOnext();
void	iioCOprint();
OOID	iiooAlloc();
char	*iiooStrAlloc();
OO_OBJECT	*iiooMemAlloc();
int	Cdestroy();
OOID	Cfetch();
OOID	iiooClook();
int	CnameOk();
OOID	iiooCnew();
OOID	iiooC0new();
OOID	iiooCDbnew();
OOID	iiooCperform();
OOID	iiobCheckName();
OOID	iiobConfirmName();
OOID	Crename();
OOID	CsubClass();
OOID	iiooCwithName();
OOID	ENdecode();
OOID	ENdestroy();
OOID	ENfetch();
OOID	ENflush();
OOID	ENinit();
OOID	ENinit();
OOID	ENreadfile();
i4	ENwritefile();
OOID	Minit();
OOID	MinitDb();
bool	iiobAuthorized();
OOID	OBclass();
OOID	OBcopy();
OO_OBJECT *	OBcopyId();
bool	OBdestroy();
OOID	OBedit();
OOID	OBencode();
OOID	OBinit();
bool	OBisEmpty();
bool	OBisNil();
OOID	OBmarkChange();
char	*OBname();
OOID	OBnoMethod();
int	OBprint();
char	*OBrename();
OOID	OBsetPermit();
OOID	OBsetRefNull();
OOID	OBsubResp();
OOID	OBtime();
OO_REFERENCE *	REinit();
OO_REFERENCE *	REinitDb();
OOID	fetch0();
OOID	fetchAll();
OOID	fetchAt();
OOID	fetchDb();
OOID	flush0();
OOID	IIflushAll();
OOID	flushAt();
