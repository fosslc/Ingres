/*
** Copyright (c) 1995, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <cm.h>
#include <cs.h>

/*
** Name:	cmdata.c
**
** Description:	Global data for cm facility.
**
LIBRARY = IMPCOMPATLIBDATA
**
** History:
**      12-Sep-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	11-apr-2007 (gupsh01)
**	    Added Global to indicate if CM 
**	    refers to UTF8 characters.
**	22-may-2007 (gupsh01)
**	    Added CM_utf8casetab and CM_utf8attrtab.
**	14-jul-2008 (gupsh01)
**	    Modify CM_utf8casetab and CM_utf8attrtab to 
**	    CM_casetab_UTF8 and CM_attrtab_UTF8.
**	12-aug-2008 (gupsh01)
**	    Revert GLOBALREF declarations to extern
**	    as this breaks Windows.
**      02-feb-2009 (joea)
**          Change CM_isUTF8 to bool.
**       5-Nov-2009 (hanal04) Bug 122807
**          CM_CaseTab needs to be u_char or we get values wrapping and
**          failing integer equality tests.
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Initialize CM_isDBL
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Default CM_isDBL to doublebyte.
**
*/

extern u_i2 CM_DefAttrTab[];
extern char CM_DefCaseTab[];
extern i4   CM_BytesForUTF8[];
extern CM_UTF8ATTR CM_attrtab_UTF8[];
extern CM_UTF8CASE CM_casetab_UTF8[];

GLOBALDEF u_i2 *CM_AttrTab = CM_DefAttrTab;
GLOBALDEF u_char *CM_CaseTab = CM_DefCaseTab;
GLOBALDEF bool CM_isUTF8 = FALSE;
GLOBALDEF bool CM_isDBL = TRUE;
GLOBALDEF i4 *CM_UTF8Bytes = CM_BytesForUTF8;
GLOBALDEF CM_UTF8ATTR *CM_UTF8AttrTab = CM_attrtab_UTF8;
GLOBALDEF CM_UTF8CASE *CM_UTF8CaseTab = CM_casetab_UTF8;
