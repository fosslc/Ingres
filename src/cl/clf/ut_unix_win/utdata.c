/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name:	utdata.c
**
** Description:	Global data for UT facility.
**
** History:
**      12-Sep-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**
*/
/*
**
LIBRARY = IMPCOMPATLIBDATA 
*/

#include    <compat.h>


GLOBALDEF bool		isProcess = FALSE;

/*
**  Data from ut
*/

GLOBALDEF STATUS	UTstatus = OK;

GLOBALDEF short		UTminSw = 0;
