/*
** Copyright (c) 1988, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>

/**
** Name:	cgstrs.roc -	Code Generator Output Strings.
**
** Description:
**	Defines some common code generator output strings:
**
**	iicgCharNull[]	`(char *) NULL'
**	iicgShrtNull[]	`(i2 *) NULL'
**	iicgEmptyStr[]	`""'
**	iicgZero[]	`0'
**	iicgOne[]	`1'
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Moved iicgFormVar to cginit.c.
**
**	Revision 6.1  88/05/16  wong
**	Initial revision.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

GLOBALDEF const
	char	iicgCharNull[] = ERx("(char *) NULL"),
		iicgShrtNull[]	= ERx("(i2 *) NULL"),
		iicgEmptyStr[]	= ERx("\"\""),
		iicgZero[]	= ERx("0"),
		iicgOne[]	= ERx("1");
