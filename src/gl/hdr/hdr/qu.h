/*
**	Copyright (c) 2004 Ingres Corporation
*/
#ifndef QU_H_INCLUDED
#define QU_H_INCLUDED

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
**	11-Nov-2010 (kschendel)
**	    Nested-inclusion protection.
**	29-Nov-2010 (frima01) SIR 124685
**	    Removed CL_PROTOTYPED from declarations and added
**	    prototypes for relative queue functions.
**/

#ifndef QUinit
FUNC_EXTERN VOID    QUinit(
	    QUEUE	*q
);
#endif

FUNC_EXTERN QUEUE * QUinsert(
	    QUEUE	*element, 
	    QUEUE	*current
);

FUNC_EXTERN QUEUE * QUremove(
	    QUEUE	*element
);

FUNC_EXTERN VOID    QUr_init(
	    QUEUE	*q
);

FUNC_EXTERN QUEUE * QUr_insert(
	    QUEUE	*entry, 
	    QUEUE	*next
);

FUNC_EXTERN QUEUE * QUr_remove(
	    QUEUE	*entry, 
	    QUEUE	*next
);

#endif /* QU_H_INCLUDED */
