/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	"erqf.h"

/**
** Name:	mqstrings.c -- string constants used by QBF.
**
** Description:
**	This file defines string constants as global character arrays
**	which may be referred to many times in the code while
**	incurring storage in data space only once.  The character
**	array variables themselves do not occupy space.	 This
**	technique is particularly effective for dealing with names
**	of forms and fields which typically must be mentioned numerous
**	times in Equel code.  It is also effective for error messages
**	which may occur in several places.
**
** History:
**	2-23-87 (peterk) - Created.
**	8-4-87	(danielt) removed _QBFJoinDefsCatalog constant 
**		(message extraction)
**/

GLOBALDEF char _abbr [] = ERx("abbr");
GLOBALDEF char _chgdisp []	= ERx("chgdisp");
GLOBALDEF char _col1 []	= ERx("column1");
GLOBALDEF char _col2 []	= ERx("column2");
GLOBALDEF char _form_type []	= ERx("form_type");
GLOBALDEF char _ii_joindefs []	= ERx("ii_joindefs");
GLOBALDEF char _ii_qbfnames []	= ERx("ii_qbfnames");
GLOBALDEF char _iiqbfinfo []	= ERx("iiqbfinfo");
GLOBALDEF char _mqbfjoin []	= ERx("mqbfjoin");
GLOBALDEF char _mqbfops []	= ERx("mqbfops");
GLOBALDEF char _mqbftbls []	= ERx("mqbftbls");
GLOBALDEF char _mqchange []	= ERx("mqchange");
GLOBALDEF char _mqcrjoin []	= ERx("mqcrjoin");
GLOBALDEF char _mqdops []	= ERx("mqdops");
GLOBALDEF char _mqjdup []	= ERx("mqjdup");
GLOBALDEF char _mqnewjoin []	= ERx("mqnewjoin");
GLOBALDEF char _mquops []	= ERx("mquops");
GLOBALDEF char _qdefname []	= ERx("qdefname");
GLOBALDEF char _qinfo1 []	= ERx("qinfo1");
GLOBALDEF char _qinfo2 []	= ERx("qinfo2");
GLOBALDEF char _qinfo3 []	= ERx("qinfo3");
GLOBALDEF char _qinfo4 []	= ERx("qinfo4");
GLOBALDEF char _qowner []	= ERx("qowner");
GLOBALDEF char _qtype []	= ERx("qtype");
GLOBALDEF char _role [] = ERx("role");
GLOBALDEF char _table []	= ERx("table");
GLOBALDEF char _tables []	= ERx("tables");
GLOBALDEF char _tblfmt []	= ERx("tblfmt");
GLOBALDEF char _tbljoins []	= ERx("tbljoins");
GLOBALDEF char _titletotwid []	= ERx("titletotwid");
GLOBALDEF char _totwidth []	= ERx("totwidth");
