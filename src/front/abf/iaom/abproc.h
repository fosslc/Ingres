/*
**	Copyright (c) 1989, 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	amproc.h -	OC_APPL procedure extern declarations.
**
** Description:
**	Generated automatically by classout.
**
** History:
**	Revision 6.2  89/01  wong
**	Initial revision.
**	24-Aug-2009 (kschendel) 121804
**	    Some types are wrong, some routines don't exist.
**/

bool	iiamAuthorized();
bool	iiamDestroy();

OOID	iiamCkApplName();
OOID	iiamCkFrmName();
OOID	iiamCkProcName();
OOID	iiamCkGlobalName();
OOID	iiamCkRattName();

OOID	iiamDbApplName();
OOID	iiamDbRecordName();
OOID	iiamDbGlobalName();
OOID	iiamDbRattName();

OOID	iiamWithName();
