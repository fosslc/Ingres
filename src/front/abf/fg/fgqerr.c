/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<ex.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<metafrm.h>

extern	METAFRAME *G_metaframe;

/**
** Name:	fgqerr.c -	Query generation error handler
**
** Description:
**	This file defines:
**
**	IIFGqerror		Report an error in query generation
**
** History:
**	4/28/89	(Mike S)	Initial version
**	7/14/89 (Mike S)	Add "mferr" parameter
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	IIFGqerror	- Report a query generation error.
**
** Description:
**	Report the error and jump back to return failure.
**
** Inputs:
**	msgid		ER_MSGID 	message number
**	mferr		bool		was there an error in the metaframe?
**	parcount	i4		number of (significant) parameters
**	p1, ... p8	PTR		error parameters
**
** Outputs:
**	none
**
**	Returns:
**			Doesn't return
**
**	Exceptions:
**		signals EXVALJMP to do a longjump.
**
** Side Effects:
**		Jumps back to previous FEjmp_handler caller.
**
** History:
**	4/28/89	(Mike S) 	Initial version
*/
/*VARARGS3*/
VOID
IIFGqerror( msgid, mferr, parcount, p1, p2, p3, p4, p5, p6, p7, p8 )
ER_MSGID msgid;
bool	mferr;
i4  	parcount;
PTR	p1, p2, p3, p4, p5, p6, p7, p8;
{
	extern i4  G_fgqline;		/* Line on which error occured */
	extern char *G_fgqfile;		/* File in which error occured */

	/* If there was a metaframe error, set the state bit */
	if (mferr) G_metaframe->state |= MFST_GENBAD;

	IIUGerr(msgid, 0, parcount+2, (PTR)&G_fgqline, (PTR)G_fgqfile, p1, p2, 
		p3, p4, p5, p6, p7, p8);	
	EXsignal(EXVALJMP, 0);
	/*NOTREACHED*/
}
