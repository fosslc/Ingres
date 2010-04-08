/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    cgil.h -	Code Generator IL Interface.
**
** Description:
**	Contains definitions for the OSL code generator's IL-oriented
**	interface.  See also il.h in interp.
**
** History:
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL):
**		Add macro ILNULLSID to support long SIDs
**	02/26/91 (emerson)
**		Added return type for IICGgdaGetDataAddr
**		(new function for bug 36097).
**
**	Revision 6.3  90/04  wong
**	Added missing declaration for 'IICGvltValType()'.
**
**	Revision 6.0  87/06  arthur
**	Intial revision.
*/

/*
** Table mapping IL op codes to code generating routines.
*/

/* Re-define IIILtab on DG so IIINTERP and OSLSQL can co-exist */
# ifdef DGC_AOS
# define IIILtab IIdgILtab
# endif

GLOBALREF IL_OPTAB	IIILtab[];

/*
** Return types of commonly used routines
*/
IL		*IICGgilGetIl();
IL		*IICGnilNextIl();
char		*IICGgvlGetVal();
char		*IICGgadGetAddr();
char		*IICGgdaGetDataAddr();
DB_DT_ID	IICGvltValType();

/*::
** Name:    ILNULLVAL() -	Return TRUE if a VALUE is NULL
*/
# define ILNULLVAL(val)		((val) == 0)

/*::
** Name:	ILNULLSID() -   Return TRUE if a SID is null.
**
** Description:
**	Returns TRUE if the SID is null, FALSE otherwise.
**	(A SID represents the offset of a new IL statement,
**	relative to the current statement.  A null SID indicates
**	that the "new" statement is simply the current statement).
**	
**	THIS IS A MACRO
**
** Inputs:
**	stmt		A pointer to the opcode of a IL statement.
**	sid		An IL operand from that statement, representing a SID.
**
** Outputs:
**	Returns:
**		TRUE	if the SID is null.
**		FALSE   otherwise.
** History:
**	01/09/91 (emerson)
**		First Written
**
** Discussion:
**	This macro was developed as part of support for long SIDs.
**	Currently, we don't need to look at (*stmt & ILM_LONGSID),
**	because if ILM_LONGSID is set, then operand 2 (sid) is the negation of
**	the 1-relative index of an integer constant, and thus cannot be zero.
**	But if it ever becomes possible for an IL operand of 0 to represent
**	an integer constant, then we'll have to check (*stmt & ILM_LONGSID):
**	If ILM_LONGSID is set, then the SID can't be null, even if ((sid) == 0).
*/
# define ILNULLSID(stmt, sid)	((sid) == 0)

/*::
** Name:    ILINTVAL() -	Return integer value or default, as string.
*/
# define ILINTVAL(val, default)	((ILNULLVAL(val)) ? (default) : (IICGgvlGetVal((val), DB_INT_TYPE)))

/*::
** Name:    ILCHARVAL() -	Return character string value or default value.
*/
# define ILCHARVAL(val, default)	((ILNULLVAL(val)) ? (default) : (IICGgvlGetVal((val), DB_CHR_TYPE)))

/*::
** Name:    ILSTRARG() -	Output String Value as Procedure Argument.
*/
# define ILSTRARG(val, default)	{ \
	if ( ILNULLVAL(val) ) \
		IICGocaCallArg( default ); \
	else \
		IICGstaStrArg( val ); \
}

/*::
** Name:    ILISCONST() -	Return TRUE if VALUE is a constant
*/
# define ILISCONST(val)		((val) < 0)
