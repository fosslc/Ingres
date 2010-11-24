/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef PE_HDR_INCLUDED
# define PE_HDR_INCLUDED 1

#include    <pecl.h>

/**CL_SPEC
** Name:	PE.h	- Define PE function externs
**
** Specification:
**
** Description:
**	Contains PE function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of pe.h.
**/

FUNC_EXTERN STATUS PEumask(
	    char	*pattern
);

FUNC_EXTERN STATUS PEworld(
	    char	*pattern,
	    LOCATION	*loc
);

#ifndef PEsave
FUNC_EXTERN VOID PEsave(
	    void
);
#endif

#ifndef PEreset
FUNC_EXTERN VOID PEreset(

	    void
);
#endif


# endif /* ! PE_HDR_INCLUDED */
