/*	SCCS ID = %W%  %G%	*/

/*
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		UT.c -- UT definitions module.
**
**	Function:
**		None
**
**	Arguments:
**		None
**
**	Result:
**		define the UT module global UTstatus.
**		define the UT module global UT_CO_DEBUG.
**
**	Side Effects:
**		None
**
**	History:
**		03/83 -- (gb)
**			written
**
**		12/84 (jhw) -- Moved definition of `UT_CO_DEBUG' from
**			"UTcompile.c".
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	10-jun-95 (emmag)
**	    Add declaration of UTminSw as part of FRS fix for NT
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	23-sep-1996 (canor01)
**	    Move global data definitions to utdata.c
*/


# include	<compat.h>
# include	<gl.h>



GLOBALREF STATUS	UTstatus;
GLOBALREF short		UTminSw;

# ifdef UT_DEBUG
bool		UT_CO_DEBUG = FALSE;
# endif
