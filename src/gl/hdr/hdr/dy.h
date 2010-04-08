/*
**	Copyright (c) 2004 Ingres Corporation
*/
#include    <dycl.h>

/**CL_SPEC
** Name:	DY.h	- Define DY function externs
**
** Specification:
**
** Description:
**	Contains DY function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**/

FUNC_EXTERN STATUS  DYinit(
#ifdef CL_PROTOTYPED
	    LOCATION	    *parm1,
	    bool	    parm2
#endif
);

FUNC_EXTERN STATUS  DYload(
#ifdef CL_PROTOTYPED
	    SYMTAB	    *parm1,
	    LOCATION	    *parm2[], 
	    char	    *parm3[]
#endif
);

