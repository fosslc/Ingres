/*
**	Copyright (c) 2004 Ingres Corporation
*/
/**
** Name:	g4defs.h 
**
** Description:
**	Definitions by the EXEC 4GL interface
**
** History:
**      10/92 (Mike S) Initial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define g4_check		g4_check_proto

FUNC_EXTERN i4	 g4_check(i4 func, char *request, bool isobject);
