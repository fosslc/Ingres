/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <er.h>
# include       <ex.h>
# include       <ug.h>
# include       <ooclass.h>
# include       <metafrm.h>
# include       <erfg.h>
# include       "framegen.h"

/**
** Name:	fgerr.c - process errors for 4gl code generation.
**
** Description:
**	<comments>.  This file defines:
**
**	IIFGerr		Process errors.
**
** History:
**	July-24-1989	(pete)	Initial Version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
GLOBALREF METAFRAME *G_metaframe;

/* static's */

/*{
** Name:	IIFGerr - process 4gl code generation errors.
**
** Description:
**	Print and error message and then signal an exception and possibly
**	also set a bit in the G_metaframe if the metaframe structure has
**	an internal consistency problem.
**
** Inputs:
**	ER_MSGID        msgid		Message id for the error to print.
**	i4             severity	[ FGFAIL | FGCONTINUE ] | [ FGSETBIT ]
**	i4             parcount	Number of parameters that follow.
**	PTR             a1 - a10
**
** Outputs:
**	NONE
**
**	Returns:
**		VOID
**
**	Exceptions:
**		EXDECLARE or EXCONTINUE
**
** Side Effects:
**	NONE
**
** History:
**	July-24-1989	(pete)	Initial Version
**      17-apr-1991     (pete)
**              Fix invalid exceptions in EXsignal call (was signalling
**              EXCONTINUE and EXDECLARE).
*/
/*VARARGS3*/
VOID
IIFGerr (msgid, severity, parcount, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
ER_MSGID        msgid;
i4              severity;
i4              parcount;
PTR             a1, a2, a3, a4, a5, a6,  a7, a8, a9, a10;
{
	IIUGerr(msgid, UG_ERR_ERROR,
		  parcount, a1, a2, a3, a4, a5, a6,  a7, a8, a9, a10);

	/*
	** Now, either continue processing where we were before this
	** routine was called, or run the EXdeclare block in IIFGstart,
	** based on the value of 'severity'. Also, if the appropriate bit
	** is set in 'severity', then set a bit in METAFRAME.
	*/
	if (severity & FGCONTINUE)
	{
	    EXsignal (EXVALJMP,  0); /* warning message. continue processing*/
	}
	else	/* FGFAIL */
	{
	    if ( severity & FGSETBIT )
		G_metaframe->state |= MFST_GENBAD;

	    EXsignal (EXVFLJMP,  0);  /* run EXdeclare block & return FAIL */
	}
}
