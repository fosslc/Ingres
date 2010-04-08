/*
**	Copyright (c) 1990, 2004 Ingres Corporation
*/

#include	<compat.h>

/**
** Name:	iamdbses.sc -	IAM DBMS Session Set-up Module.
**
** Description:
**	Contains routines used to set-up and clear locking state for the
**	DBMS session.  Defines:
**
**	IIAMsetDBsession()	set DBMS session lock mode state.
**	IIAMclrDBsession()	clear DBMS session lock mode state.
**
** History:
**	Revision 6.3/03/00  90/04  wong
**	Initial revision.
**/

bool	IIUIdcn_ingres();

VOID
IIAMsetDBsession()
{
	if ( IIUIdcn_ingres() )
	{ /* ... for INGRES only ... */
		exec sql set lockmode session where readlock = nolock;
	}
}

VOID
IIAMclrDBsession()
{
	if ( IIUIdcn_ingres() )
	{ /* ... for INGRES only ... */
		exec sql set lockmode session where readlock = system;
	}
}
