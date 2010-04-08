/*
**	Copyright (c) 2004, 2008 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    oskeyw.h -	OSL/QUEL Miscellenous Keyword/Operator
**					String Declarations.
** Description:
**	Contains the declarations of the keyword/operator strings for OSL/QUEL.
**
** History:
**	Revision 6.0  87/06  wong
**	Added 'Activate' and `LIKE' and `NULL' operator strings.
**	Added distributed DBMS keywords.  03/87  wong.
**
**	Revision 5.1  86/11/04  14:09:33  wong
**	Initial revision.
**	21-Jan-2008 (kiria01) b119806
**	    Extended grammar for postfix operators beyond IS NULL removes
**	    the need for the _IsNull globals.
**	    Added osw_compare_pfx for prefix match.
*/

GLOBALCONSTREF char	_And[];
GLOBALCONSTREF char	_Or[];
GLOBALCONSTREF char	_Not[];
GLOBALCONSTREF char	_All[];
GLOBALCONSTREF char	_Explanation[];
GLOBALCONSTREF char	_Activate[];
GLOBALCONSTREF char	_Subject[];
GLOBALCONSTREF char	_File[];
GLOBALCONSTREF char	_Entry[];
GLOBALCONSTREF char	_iiNext[];
GLOBALCONSTREF char	_Menu[];

GLOBALCONSTREF char	_Link[];
GLOBALCONSTREF char	_Permanent[];
GLOBALCONSTREF char	_Temporary[];

GLOBALCONSTREF char	_UnaryMinus[];
GLOBALCONSTREF char	_Plus[];
GLOBALCONSTREF char	_Minus[];
GLOBALCONSTREF char	_Mul[];
GLOBALCONSTREF char	_Div[];
GLOBALCONSTREF char	_Exp[];
GLOBALCONSTREF char	_Less[];
GLOBALCONSTREF char	_Greater[];
GLOBALCONSTREF char	_Equal[];
GLOBALCONSTREF char	_NotEqual[];
GLOBALCONSTREF char	_LessEqual[];
GLOBALCONSTREF char	_GreatrEqual[];
GLOBALCONSTREF char	_Like[];
GLOBALCONSTREF char	_NotLike[];
GLOBALCONSTREF char	_Escape[];
GLOBALCONSTREF char	_PMEncode[];

/* keyword comparison macro */
#define osw_compare(k, s) ((k) == (s) || (CMcmpnocase((k), (s)) == 0 && STbcompare((k), 0, (s), 0, TRUE) == 0))
/* keyword prefix comparison macro */
#define osw_compare_pfx(k, s, l) ((k) == (s) || (CMcmpnocase((k), (s)) == 0 && STbcompare((k), l, (s), l, TRUE) == 0))

