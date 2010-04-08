/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>

/**
** Name:    fedbpipes.c -	Return Channel Connection Flag for DBMS Module.
**
** Description:
**	Contains a routine that returns a channel connection flag (i.e., the
**	"-X" flag) for the DBMS.  Defines:
**
**	FEdbpipes()	return channel connection flag for the DBMS.
**
** History:
**	Revision ??  unknown
**	Initial revision.
**/

/*{
** Name:    FEdbpipes() -	Return Channel Connection Flag for the DBMS.
**
** Description:
**	Returns the channel connection flag passed to other Front-Ends to
**	share the DBMS session active for the program that calls this routine.
**
** Returns:
**	{char *}  String containing the channel connection flag.
*/
char *
FEdbpipes()
{
    char	*IIxflag();

    return STalloc(IIxflag());
}

