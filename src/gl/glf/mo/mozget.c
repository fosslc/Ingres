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
** Name:	mozget.c	MOzeroget
**
** Description:
**	This file defines:
**
**	MOzeroget - get buffer with zero value method
**
** History:
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
**/


/*{
**  Name:	MOzeroget - get buffer with zero value method
**
**  Description:
**
**	This get method is for use with writable control objects.  It
**	writes a buffer of the string of the value zero, as if MOivget
**	had been called with object of zero.
**
**  Inputs:
**	 offset
**		ignored.
**	 objsize
**		ignored.
**	 object
**		ignored.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		Filled with stirng value of zero.
**  Returns:
**	 OK
**		if the operation succeseded
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS
MOzeroget( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    (void) MOivget( offset, size, (PTR)0, lsbuf, sbuf );
    return( OK );
}

