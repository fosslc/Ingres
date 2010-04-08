/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <sp.h>
# include <mo.h>
# include "moint.h"

/**
** Name:	mostrget.c	-- MOstrget
**
** Description:
**	This file defines:
**
**  Name:	MOstrget - standard get string buffer method
**
** History:
**	22-sep-92 (daveb)
**	    documented
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      21-jan-1999 (canor01)
**          Remove erglf.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
**  Name:	MOstrget - standard get string buffer method
**
**  Description:
**
**
**	Copy the instance string to the output buffer userbuf , to a
**	maximum length of luserbuf .  If the output is bigger than the
**	buffer, chop it off and return MO_VALUE_TRUNCATED.
**
**	The objsize from the class definition is ignored.  The object
**	plus the offset is treated as a pointer to a character string to
**	be copied.
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the object from the class definition, ignored.
**	 object
**		a pointer to a character string for the instance to copy out.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains a copy of the the string value.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		if the output buffer was too small and the value was
**		chopped off to fit.
**	 other
**		method-specific failure status as appropriate.
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS 
MOstrget( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    char *cobj = (char *)object + offset;

    return( MOlstrout( MO_VALUE_TRUNCATED, size, cobj, lsbuf, sbuf ));
}

