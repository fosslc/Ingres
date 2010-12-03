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
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**/

FUNC_EXTERN STATUS  DYinit(
	    LOCATION	    *parm1,
	    bool	    parm2
);

FUNC_EXTERN STATUS  DYload(
	    SYMTAB	    *parm1,
	    LOCATION	    *parm2[], 
	    char	    *parm3[]
);

