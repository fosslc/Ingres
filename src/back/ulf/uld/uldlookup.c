/*
** Copyright 2004, 2008 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <st.h>
#include <iicommon.h>




/* File: uldlookup.c - Utility code-to-text lookups
**
** Description:
**	This file contains some utility lookup routines for general back-
**	end use with error messages, debug output, and the like.
**
**	These lookup routines tend to all be pretty much alike, and
**	there's no law against having a generic one.  At the time of this
**	writing, though, I've deemed it preferable to avoid callers
**	needing to import lookup tables cross-facility.
**
** History:
**	25-Jan-2004 (schka24)
**	    Invent as someplace to put query-mode text lookups.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

/* Lookup tables defined in ulddata.roc: */

GLOBALREF const INTXLATE Uld_ahd_xlatetab[];
GLOBALREF const INTXLATE Uld_mathex_xlatetab[];
GLOBALREF const INTXLATE Uld_psq_xlatetab[];
GLOBALREF const INTXLATE Uld_struct_xlatetab[];
GLOBALREF const INTXLATE Uld_syscat_xlatetab[];


/*
** Name: uld_psq_modestr - PSQ_XXX query mode lookup
**
** Description:
**	This routine is given a "query mode", defined as a parser PSQ_XXXX
**	symbol (PSQ_CREATE, etc), and returns a short text description.
**	This is mainly for error and debug messages, so speed is not
**	critical - a simple linear seach is good enough.
**
** Inputs:
**	qmode		The query mode
**
** Outputs:
**	Returns char * pointer to short description (pointer to an
**	empty string if no description found).
**
** History:
**	25-Jan-2004 (schka24)
**	    Invent to make generalizing some parser productions and their
**	    error messages a bit easier and less tedious.
*/

char *
uld_psq_modestr(i4 qmode)
{
    const INTXLATE *xlatePtr;	/* Translation table pointer */

    xlatePtr = &Uld_psq_xlatetab[0];
    while (xlatePtr->string != NULL)
    {
	if (xlatePtr->code == qmode)
	    return (xlatePtr->string);
	++ xlatePtr;
    }
    return ("");
} /* uld_psq_modestr */

/*
** Name: uld_qef_actstr - QEA_XXX Action header type lookup
**
** Description:
**	This routine is given an action header type, defined as a QEA_XXX
**	symbol (QEA_GET, etc), and returns a short text description.
**	This is mainly for error and debug messages, so speed is not
**	critical - a simple linear seach is good enough.
**
** Inputs:
**	ahd_type		The action header type
**
** Outputs:
**	Returns char * pointer to short description (pointer to an
**	empty string if no description found).
**
** History:
**	5-Feb-2004 (schka24)
**	    OP150 trace point not entirely up-to-date, fix this way.
*/

char *
uld_qef_actstr(i4 ahd_type)
{
    const INTXLATE *xlatePtr;	/* Translation table pointer */

    xlatePtr = &Uld_ahd_xlatetab[0];
    while (xlatePtr->string != NULL)
    {
	if (xlatePtr->code == ahd_type)
	    return (xlatePtr->string);
	++ xlatePtr;
    }
    return ("");
} /* uld_qef_actstr */

/*
** Name: uld_struct_xlate - Translate a structure name
**
** Description:
**	This routine takes a storage structure name (heap, hash, etc)
**	and returns the corresponding DB_xxx_STORE code.
**
**	The compressed versions (e.g. cbtree) are not recognized, because
**	the compression flag is separate from the STORE codes.  It's better
**	if the caller strips off any leading letter 'C' and sets a
**	compression flag appropriate to the context.
**
** Inputs:
**	strname		The storage structure name
**
** Outputs:
**	The DB_xxx_STORE code
**	Zero is returned if no match (as it happens, zero is not
**	a valid storage structure code)
**
** History:
**	23-Nov-2005 (kschendel)
**	    Invent to reduce duplication.
*/

int
uld_struct_xlate(char *strname)
{
    const INTXLATE *xlatePtr;	/* Translation table pointer */

    xlatePtr = &Uld_struct_xlatetab[0];
    while (xlatePtr->string != NULL)
    {
	if (STcasecmp(strname, xlatePtr->string) == 0)
	    return (xlatePtr->code);
	++ xlatePtr;
    }
    return (0);
} /* uld_struct_xlate */

/*
** Name: uld_struct_name - Translate a structure number
**
** Description:
**	This routine takes a storage structure number DB_xxx_STORE
*	and returns the corresponding structure name.  It's the opposite
**	of uld_struct_xlate.
**
** Inputs:
**	The DB_xxx_STORE code
**
** Outputs:
**	A pointer to the storage structure name
**	A pointer to "" is returned if there's no match.
**
** History:
**	11-Oct-2010 (kschendel) SIR 124544
**	    Invent for WITH-parsing.
*/

char *
uld_struct_name(i4 strnum)
{
    const INTXLATE *xlatePtr;	/* Translation table pointer */

    xlatePtr = &Uld_struct_xlatetab[0];
    while (xlatePtr->string != NULL)
    {
	if (xlatePtr->code == strnum)
	    return (xlatePtr->string);
	++ xlatePtr;
    }
    return ("");
} /* uld_struct_name */

/*
** Name: uld_syscat_namestr - System catalog name lookup
**
** Description:
**	This routine is given a system catalog table number, either
**	reltid or reltidx as appropriate, and returns the catalog name.
**	This is mainly for error and debug messages, so speed is not
**	critical - a simple linear seach is good enough.
**
** Inputs:
**	table_id		The table ID number (baseid or indexid)
**
** Outputs:
**	Returns char * pointer to catalog name (pointer to an
**	empty string if no match found).
**
** History:
**	30-Dec-2004 (schka24)
*	    Fix up another tedious switch statement while I'm in there.
*/

char *
uld_syscat_namestr(i4 table_id)
{
    const INTXLATE *xlatePtr;	/* Translation table pointer */

    xlatePtr = &Uld_syscat_xlatetab[0];
    while (xlatePtr->string != NULL)
    {
	if (xlatePtr->code == table_id)
	    return (xlatePtr->string);
	++ xlatePtr;
    }
    return ("");
} /* uld_syscat_namestr */

/*
** Name: uld_mathex_xlate - Math exception to string translation
**
** Description:
**	This routine is given an internal exception number (exarg_num),
**	and returns a short text description if it's one of the arithmetic
**	error codes.
**
** Inputs:
**	exnum		The exception number
**
** Outputs:
**	Returns char * pointer to short description, or NULL if not one
**	of the listed arithmetic exceptions
**
** History:
**	4-Dec-2008 (kschendel) b122119
**	    Add while making DMF less sensitive to math errors, might be
**	    useful for other facilities too.
*/

char *
uld_mathex_xlate(i4 exnum)
{
    const INTXLATE *xlatePtr;	/* Translation table pointer */

    xlatePtr = &Uld_mathex_xlatetab[0];
    while (xlatePtr->string != NULL)
    {
	if (xlatePtr->code == exnum)
	    return (xlatePtr->string);
	++ xlatePtr;
    }
    return (NULL);
} /* uld_mathex_xlate */
