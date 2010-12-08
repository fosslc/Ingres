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
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**/

FUNC_EXTERN STATUS  TCclose(
	TCFILE	    *desc
); 

FUNC_EXTERN bool    TCempty(
	TCFILE	    *desc
); 

FUNC_EXTERN STATUS  TCflush(
	TCFILE	    *desc
); 

FUNC_EXTERN i4       TCgetc(
	TCFILE	    *desc,
	i4	    seconds
); 

FUNC_EXTERN STATUS  TCopen(
	LOCATION    *location,
	char	    *mode,
	TCFILE	    **desc
);

FUNC_EXTERN STATUS  TCputc(
	i4	    achar,
	TCFILE	    *desc
); 

# endif /* ! TC_HDR_INCLUDED */
