/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"caerror.h"

/**
** Name:	caerror.sc
**
** Defines
**		errexit()
**
** Input:
**	abortflag	{bool}  TRUE if within transaction, else FALSE
**
** Returns:
**	Never.  (Exits via 'PCexit()'.)
**
** Called by:
**		various
**
** Error Messages:
**	Calls 'IIUGerr()' to announce that no changes will be made to
**	the database
**
** History:
**	Written 11/30/83 (agh)
**
**	5-oct-1987 (Joe)
**		Call FEexits to close up any FE things started.
**		This was done so that FEing_exit won't be called
**		unless FEingres was called.
**
**	22-jun-87  (wong)  Changed to use SQL (ROLLBACK for ABORT.)
*/

VOID
errexit ( abortflag )
bool	abortflag;	/* TRUE if we are within a transaction */
{
	IIUGerr( EXITMSG, 0, 0 );
	if ( abortflag )
		exec sql rollback work;

	FEexits( (char *)NULL );
	PCexit( FAIL );
	/*NOTREACHED*/
}
