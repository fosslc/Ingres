/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include <ex.h>

/**
** Name:	iammerr.c - exception handler.
**
** Description:
**	Exception handler used for catching ME failures.  Only applicable
**	to PCINGRES environment.
*/

/*{
** Name:	IIAMmeMemError - exception handler
**
** Description:
**	Catches the exception for ME allocation failure on PCINGRES.  Passes
**	all others on up to next handler.
*/

EX
IIAMmeMemError (exa)
EX_ARGS *exa;
{
#ifdef PCINGRES
	if (exa->exarg_num == ME_OUTOFMEM)
		return (EXDECLARE);
#endif
	return (EXRESIGNAL);
}
