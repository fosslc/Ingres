/*
** Copyright (c) 2004 Ingres Corporation  
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <sp.h>
# include <mo.h>
# include "moint.h"

/**
** Name:	mobget.c	MOblankget
**
** Description:
**	This file defines:
**
**	MOblankget - get buffer of blanks method
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	5-May-1993 (daveb)
**	    include <me.h> for MEfill.
**      21-jan-1999 (canor01)
**          Remove erglf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
**  Name:	MOblankget - get buffer of blanks method
**
**  Description:
**
**		This get method is for use with writable control objects.  It
**		fills in the output buffer with a string of blanks.  The offset,
**		object, and size are ignored.
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
**		Filled with blanks.
**  Returns:
**	 OK
**		if the operation succeseded
**
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS
MOblankget( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    MEfill( (i4)lsbuf, (unsigned char)' ', sbuf );
    return( OK );
}


