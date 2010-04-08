/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <adf.h>

#if !defined (LIBQ_HDR_INCLUDED)
#define LIBQ_HDR_INCLUDED
/**
**  Name: libq.h - LIBQ prototypes
**
**  Description:
**	This file contains prototypes for LIBQ routines that need to be
**	visible outside of front!embed.
**
**  History:
**	20-may-2004
**	    Created.
**      08-Jan-2010 (maspa05) bug 123127
**          Added declaration for IILQasGetThrAdfcb
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

FUNC_EXTERN	void   *IILQint(i4 intval);
FUNC_EXTERN  	i4 	IItest_err(void);
FUNC_EXTERN 	ADF_CB *IILQasGetThrAdfcb(VOID);

#endif
