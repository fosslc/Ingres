/*
** Copyright (c) 1990, 2008 Ingres Corporation
*/
# include       <compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <fe.h>
# include	<er.h>

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

/**
** Name:	tu.roc - READONLY definitions for tables utility.
**
** Description:
**	Define items that can be marked READONLY.
**
** History:
**	20-apr-1990 (pete)	Initial Version.
**	18-oct-1992 (mgw)	Added IITU_Def[], IITU_User[] and IITU_Null[].
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

GLOBALDEF const char
	IITU_Empty[]        = ERx(""),
	IITU_Blank[]        = ERx(" "),
	IITU_With[]	    = ERx("with"),
	IITU_Not[]	    = ERx("not"),
	IITU_NotNull[]      = ERx(" NOT NULL"),
        IITU_WithNull[]     = ERx(" WITH NULL"),
        IITU_NotDef[]       = ERx(" NOT DEFAULT"),
        IITU_WithDef[]      = ERx(" WITH DEFAULT"),
	IITU_Def[]          = ERx(" DEFAULT"),
	IITU_User[]         = ERx(" USER"),
	IITU_Null[]         = ERx(" NULL"),
        /* logical keys: */
        IITU_NotNullWithDef[]  = ERx(" NOT NULL WITH DEFAULT"),
	IITU_SysMaint[]	    = ERx("system_maintained"),
	IITU_TableKey[]	    = ERx("table_key"),
	IITU_ObjectKey[]    = ERx("object_key"),
        IITU_WithSysMaint[] = ERx("with system_maintained"),
        IITU_NotSysMaint[]  = ERx("not system_maintained");
