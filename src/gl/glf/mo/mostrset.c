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
** Name:	mostrset.c	- MOstrset
**
** Description:
**	This file defines:
**
**  	MOstrset - standard set string buffer method
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
**  Name:	MOstrset - standard set string buffer method
**
**  Description:
**
**	Copy a caller's string to the buffer for the instance.
**
**	The object plus the offset is treated as a pointer to a
**	character buffer having objsize bytes.  The input luserbuf is
**	ignored.  The string in userbuf is copied up to objsize; if it
**	wouldn't fit, returns MO_VALUE_TRUNCATED, otherwise OK.
**
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 luserbuf
**		the length of the user buffer, ignored.
**	 userbuf
**		the user string to convert.
**	 objsize
**		the size of the output buffer.
**	 object
**		interpreted as a character buffer.
**  Outputs:
**	 object
**		gets a copy of the input string.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		if the output buffer wasn't big enough.
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS
MOstrset( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    char *cobj = (char *)object + offset;

    return( MOstrout( MO_VALUE_TRUNCATED, sbuf, size, cobj ));
}


