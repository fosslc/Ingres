/*
** Copyright (c) 1984, 2004, 2008 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
#include	<iicommon.h>
#include	<oslconf.h>
#include	<oskeyw.h>

/**
** Name:    oskeyw.roc -	OSL Parser Keyword Tables.
**
** Description:
**	Contains definitions of keyword constant strings for the OSL parser.
**	Defines:
**
**	_And, _Or ...	miscelleneous keyword or operator strings.
**
** History:
**	Revision 5.1  86/11/04  14:55:35  wong
**	Initial revision.
**	21-Jan-2008 (kiria01) b119806
**	    Extending grammar for postfix operators beyond IS NULL
**	    removes the need for the _IsNull globals.
**	26-Aug-2009 (kschendel) 121804
**	    Need iicommon.h to satisfy gcc 4.3.
**/

/*
** Name:    _And, ... -     Miscelleneous OSL Parser Keyword/Operator Strings.
*/

GLOBALCONSTDEF char	_And[] =	ERx("and");
GLOBALCONSTDEF char	_Or[] =		ERx("or");
GLOBALCONSTDEF char	_Not[] =	ERx("not");
GLOBALCONSTDEF char	_All[] =	ERx("all");
GLOBALCONSTDEF char	_Explanation[] =ERx("explanation");
GLOBALCONSTDEF char	_Activate[] =	ERx("activate");
GLOBALCONSTDEF char	_Subject[] =	ERx("subject");
GLOBALCONSTDEF char	_File[] =	ERx("file");
GLOBALCONSTDEF char	_iiNext[] =	ERx("next");
GLOBALCONSTDEF char	_Menu[] =	ERx("menu");
GLOBALCONSTDEF char	_Entry[] =	ERx("entry");

GLOBALCONSTDEF char	_Link[] = 	ERx("link");
GLOBALCONSTDEF char	_Permanent[] =	ERx("permanent");
GLOBALCONSTDEF char	_Temporary[] =	ERx("temporary");

GLOBALCONSTDEF char	_UnaryMinus[] =	ERx(" -");
GLOBALCONSTDEF char	_Plus[] =	ERx("+");
GLOBALCONSTDEF char	_Minus[] =	ERx("-");
GLOBALCONSTDEF char	_Mul[] =	ERx("*");
GLOBALCONSTDEF char	_Div[] =	ERx("/");
GLOBALCONSTDEF char	_Exp[] =	ERx("**");
GLOBALCONSTDEF char	_Less[] =	ERx("<");
GLOBALCONSTDEF char	_Greater[] =	ERx(">");
GLOBALCONSTDEF char	_Equal[] =	ERx("=");
GLOBALCONSTDEF char	_NotEqual[] =	ERx("!=");
GLOBALCONSTDEF char	_LessEqual[] =	ERx("<=");
GLOBALCONSTDEF char	_GreatrEqual[] =ERx(">=");

GLOBALCONSTDEF char	_Like[] =	ERx("like");
GLOBALCONSTDEF char	_NotLike[] =	ERx("not like");

GLOBALCONSTDEF char	_Escape[] =	ERx("escape");
GLOBALCONSTDEF char	_PMEncode[] =	ERx("ii_pmencode#");
