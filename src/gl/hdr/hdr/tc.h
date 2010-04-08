/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef TC_HDR_INCLUDED
# define TC_HDR_INCLUDED 1

#include    <tccl.h>

/**CL_SPEC
** Name:	TC.h	- Define TC function externs
**
** Specification:
**
** Description:
**	Contains TC function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of tc.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN STATUS  TCclose(
#ifdef CL_PROTOTYPED
	TCFILE	    *desc
#endif
); 

FUNC_EXTERN bool    TCempty(
#ifdef CL_PROTOTYPED
	TCFILE	    *desc
#endif
); 

FUNC_EXTERN STATUS  TCflush(
#ifdef CL_PROTOTYPED
	TCFILE	    *desc
#endif
); 

FUNC_EXTERN i4       TCgetc(
#ifdef CL_PROTOTYPED
	TCFILE	    *desc,
	i4	    seconds
#endif
); 

FUNC_EXTERN STATUS  TCopen(
#ifdef CL_PROTOTYPED
	LOCATION    *location,
	char	    *mode,
	TCFILE	    **desc
#endif
);

FUNC_EXTERN STATUS  TCputc(
#ifdef CL_PROTOTYPED
	i4	    achar,
	TCFILE	    *desc
#endif
); 

# endif /* ! TC_HDR_INCLUDED */
