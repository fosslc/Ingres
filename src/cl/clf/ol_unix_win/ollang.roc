/*
**Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<gl.h>
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
LIBRARY = IMPCOMPATLIBDATA
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
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	10-may-1999 (walro03)
**	    Remove obsolete version string dra_us5.
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
**      05-nov-1992 (peterw)
**		Integrated change from ingres63p -
**               Changed template for dr6_us5 and dra_us5 to enable COBOL and
**		 disable FORTRAN (which is not available).
**	11-dec-92 (davel)
**		Change map to be of type OL_LANGINFO, and added supportabilty
**		level member.
*/

OL_LANGINFO	IIolLangInfo[] = {
		ERx("C"),	OL_I_DEFINED,

# if defined(dr6_us5)
		ERx("FORTRAN"),	0,
# else
		ERx("FORTRAN"),	OL_I_DEFINED,
# endif

		ERx("Pascal"),	0,
		ERx("BASIC"),	0,

# if defined(dr6_us5)
                ERx("COBOL"),	OL_I_DEFINED,
# else
                ERx("COBOL"),	0,
# endif

		ERx("PL/1"),	0,
		ERx("ADA"),	0,
		ERx("4GL"),	OL_I_DEFINED,
		ERx("SQL"),	OL_I_DEFINED
};
