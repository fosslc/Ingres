/*
**	Copyright (c) 2004 Ingres Corporation
*/
#include    <qucl.h>

/**CL_SPEC
** Name:	QU.h	- Define CM function externs
**
** Specification:
**
** Description:
**	Contains CM function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	24-jan-1994 (gmanning)
**	    Enclose QUinit ifndef.
**/

#ifndef QUinit
FUNC_EXTERN VOID    QUinit(
#ifdef CL_PROTOTYPED
	    QUEUE	*q
#endif
);
#endif

FUNC_EXTERN QUEUE * QUinsert(
#ifdef CL_PROTOTYPED
	    QUEUE	*element, 
	    QUEUE	*current
#endif
);

FUNC_EXTERN QUEUE * QUremove(
#ifdef CL_PROTOTYPED
	    QUEUE	*element
#endif
);

