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
** Name:	mostpget.c	-	MOstrpget
**
** Description:
**	This file defines:
**
**  	MOstrpget - standard get string pointer method
**
**  History:
**	24-sep-92 (daveb)
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
**      02_april-2004 (zhahu02)
**          Updated to prevent SIGSEGV (INGSRV2571/b112081)
**      26-sep-2005 (huazh01)
**          back out the above change. It is fixed 
**          differently.
**/


/*{
**  Name:	MOstrpget - standard get string pointer method
**
**  Description:
**
**
**	Copy the string pointed to by instance object to the output
**	userbuf , to a maximum length of luserbuf .  If the output is
**	bigger than the buffer, chop it off and return
**	MO_VALUE_TRUNCATED.
**
**	The offset and objsize from the class definition are ignored.
**	The object is considered a pointer to a string pointer, and will
**	be dereferenced to find the source string.
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the object from the class definition, ignored.
**	 object
**		treated as a pointer to a string pointer.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the output buffer.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		if the output buffer was too small, and the value was chopped
**		off to fit.
**	 other
**		method-specific failure status as appropriate.
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS
MOstrpget( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS stat = OK;
    char *objp = (char *)object + offset;
    char *cobj = *(char **)objp;

    if( cobj != NULL )
	stat = MOstrout( MO_VALUE_TRUNCATED, cobj, lsbuf, sbuf );

    return( stat );
}

