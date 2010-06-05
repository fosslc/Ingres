/*
** Copyright (c) 1989, 2009 Ingres Corporation
*/

# include <compat.h>
#include    <gl.h>
# include <cm.h>

/**
**
**  Name: CMGLOBS.C -- Universal symbols for CM
**
**  Description:
**	The file contains data which must be known outside the CL.  The
**	front-end CL transfer vector (front!frontcl!sharesys!cl.mar)
**	contains copies of this data.  When the front-end CL shareable image
**	is linked, this module will not be included.
**
**
**  History:
**	4/1 (Mike S) Initial version
**      16-jul-93 (ed)
**	    added gl.h
**	11-apr-2007 (gupsh01)
**	    Added CM_isUTF8.
**	03-jun-2008 (gupsh01)
**	    Update for UTF8 changes for VMS.
**      02-feb-2009 (joea)
**          Change CM_isUTF8 to bool.
**       5-Nov-2009 (hanal04) Bug 122807
**          CM_CaseTab needs to be u_char or we get values wrapping and
**          failing integer equality tests.
**	03-Dec-2009 (wanfr01) Bug 122994
**	    Initialize CM_isDBL
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Default CM_isDBL to doublebyte.
**/

/*
** the default, compiled in translation.
*/
GLOBALREF u_i2 CM_DefAttrTab[];
GLOBALREF char CM_DefCaseTab[];
GLOBALREF bool CM_isUTF8;
GLOBALREF bool CM_isDBL;
GLOBALREF i4   CM_BytesForUTF8[];
GLOBALREF CM_UTF8ATTR CM_attrtab_UTF8[];
GLOBALREF CM_UTF8CASE CM_casetab_UTF8[];

GLOBALDEF {"iicl_gvar$not_ro"} noshare u_i2 *CM_AttrTab = CM_DefAttrTab;
GLOBALDEF {"iicl_gvar$not_ro"} noshare u_char *CM_CaseTab = CM_DefCaseTab;
GLOBALDEF {"iicl_gvar$not_ro"} noshare bool CM_isUTF8 = FALSE;
GLOBALDEF {"iicl_gvar$not_ro"} noshare bool CM_isDBL = TRUE;
GLOBALDEF {"iicl_gvar$not_ro"} noshare i4 *CM_UTF8Bytes = CM_BytesForUTF8;
GLOBALDEF {"iicl_gvar$not_ro"} noshare CM_UTF8ATTR *CM_UTF8AttrTab = CM_attrtab_UTF8;
GLOBALDEF {"iicl_gvar$not_ro"} noshare CM_UTF8CASE *CM_UTF8CaseTab = CM_casetab_UTF8;
