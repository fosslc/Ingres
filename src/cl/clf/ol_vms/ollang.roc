/*
**	Copyright (c) 1989, 2003 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include    <gl.h>
#include	<ol.h>
#include	<er.h>

/**
** Name:	ollang.roc -	Compatibility Module Host-Language Name Map.
**
** Description:
**	Contains the host-language name map for the compatibility library.
**	Defines:
**
**	IIolLangInfo	host-language name map.
**
** History:
**	Revision 6.2  89/02  wong
**	Initial revision.
**	Revision 6.5
**	11-dec-92 (davel)
**		replaced old IIolLang map with new IIolLangInfo map, which
**		defines language name regardless of level of support
**		on the platform, and in a separate word defines the level
**		of supportability.
**      16-jul-93 (ed)
**	    added gl.h
**/

/*:
** Name:	IIolLangInfo[] -	OL Supported Host-Language Name Map.
**
** Description:
**	A map of the names and support level of the host-languages supported 
**	by the OL CL module.  This map is indexed by the host-language
**	types defined in <ol.h>, which defines all host-languages supported by
**	INGRES on all systems.
**
**	Note that "4GL" and "SQL" are not really host-languages, but are
**	included for consistency.
**
** History:
**	02/89 (jhw) -- Created.
**	09/89 (jhw) -- Added conditional compilation for various systems.
**	11-dec-92 (davel)
**		Change map to be of type OL_LANGINFO, and added supportabilty
**		level member.
**	07-feb-2003 (abbjo03)
**	    Remove PL/1 as a supported language.
*/

GLOBALDEF	OL_LANGINFO	IIolLangInfo[] = {
		ERx("C"),	OL_I_DEFINED,
		ERx("FORTRAN"),	OL_I_DEFINED,
		ERx("Pascal"),	OL_I_DEFINED,
		ERx("BASIC"),	OL_I_DEFINED|OL_I_HAS_DECIMAL,
                ERx("COBOL"),	OL_I_DEFINED|OL_I_HAS_DECIMAL,
		ERx("PL/1"),	0,
		ERx("ADA"),	OL_I_DEFINED,
		ERx("4GL"),	OL_I_DEFINED,
		ERx("SQL"),	OL_I_DEFINED
};
