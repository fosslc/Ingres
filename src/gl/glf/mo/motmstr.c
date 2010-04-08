/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <sp.h>
# include <mo.h>
# include <pc.h>
# include <tm.h>

/**
** Name:	motmstr.c	- MOtimestampget
**
** Description:
**	This file defines:
**
**	MOtimestampget - standard get method for converting the number of 
**	seconds since 1970 to a formatted string
**
** History:
**	09-Sep-96 (johna)
**	    Created
**      21-jan-1999 (canor01)
**          Remove erglf.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
**  Name:	MOtimestampget - standard get method for converting the 
**		number of seconds since 1970 to a formatted string
**
**
**  Inputs:
**      offset          ignored
**      objsize         ignored
**      object          pointer to i4 containing # of seconds.
**      luserbuf        length of userbuf.
**  Outputs:
**	 userbuf	filled with the formatted string
**		
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**  History:
**      09-Sep-96 (johna)
**          Created
*/

STATUS 
MOtimestampget ( 
	i4 offset, 
	i4 objsize, 
	PTR object, 
	i4 luserbuf, 
	char *userbuf )
{
	char	l_str[TM_SIZE_STAMP];
	char	*cobj;
	i4	secs;

	*userbuf = EOS;

	cobj = (char *) object;
	secs =  *(i4 *)cobj;
	TMcvtsecs(secs, l_str);

	return( MOstrout( MO_VALUE_TRUNCATED, l_str, luserbuf, userbuf ) );
}
		

