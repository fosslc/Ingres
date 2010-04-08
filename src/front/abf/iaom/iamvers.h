/*
**	Copyright (c) 1990, 2004 Ingres Corporation
*/

/**
** Name:	iamvers.h - Version of encoded intermediate language
**
** Description:
**	This file defines the following constant
**
**	IAOM_IL_VERSION version of encoded intermediate language
**
** History:
**	11/08/90 (emerson) Initial verison
**		IAOM_IL_VERSION set to 6303.
**	01/09/91 (emerson)
**		IAOM_IL_VERSION set to 63030001.
**		Change to IL format: add ILM_LONGSID modifier flag to IL opcode,
**		to remove 32K limit on IL (allow up to 2G of IL).
**		Note: IAOM_IL_VERSION is now being treated as an 8-digit number
**		(instead of a 4-digit number) for finer granularity:
**		abcdefgh represents change gh within release a.b/cd/ef;
**		note that IAOM_IL_VERSION is used as a i4, 
**		which allows up to 9 decimal digits.
**	04/18/91 (emerson)
**		IAOM_IL_VERSION set to 64000000.
**		Change to IL format for release 6.4:
**		Local procedures, alerters, and LOADTABLE.
**	04-may-92 (davel)
**		IAOM_IL_VERSION set to 64020000.
**		Change to IL format for release 6.4/02:
**		New op codes for array processing (e.g. ARRAYREF, RELEASEOBJ,
**		and ARRENDUNLD), as well as new modifier bits for ARRAYREF and
**		DOT.
**	09/20/92 (emerson)
**		IAOM_IL_VERSION set to 64030000.
**		Change to IL format for release 6.4/03:
**		New opcodes (QRYROWGET and QRYROWCHK)
**		and modifier bits (ILM_BRANCH_ON_SUCCESS).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Name:	IAOM_IL_VERSION - version of encoded intermediate language
**
** Description:
**	This constant specifies the version of ABF which encoded
**	the intermediate language (IL).  When developing a new release,
**	this constant should be set to a new (higher) value if and only if
**	the new release can generate encoded IL that can't be interpreted
**	by previous releases.  This enables those old interpreters
**	to put out an error message (or possibly recompile the program
**	with an old compiler) instead of producing unpredictable results.
*/
# define	IAOM_IL_VERSION		64030000
