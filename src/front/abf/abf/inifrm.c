/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<uf.h>

/**
** Name:	inifrm.c -	ABF Form System Frame Initialization Routine.
**
**	Contains routines that initialize the Forms System, the frames, and
**	the menu items used by ABF.  Defines:
**
**	IIABiniFrm()	initialize abf forms runtime system.
**
** History:
**	Revision 2.0  82/07  joe
**	Initial revision.
*/

GLOBALREF char	*IIabExename;

/*{
** Name:	IIABiniFrm() -	Initialize ABF Forms Runtime System.
**
** Input:
**	normal	{bool}	(Not used.)
**
** Called by:
**	'IIabf()'
**
** History:
**	7/82 Written (jrc)
**	2/86 (prs)	Add support for RUNGRAPH frames.
*/

VOID
IIABiniFrm ( normal )
bool	normal;
{
	IIARfrsInit( FALSE );	/* ABF Run-time System starts Form System */
	FEcopyright(IIabExename, ERx("1982"));
}
