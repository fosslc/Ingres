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
** Name:	moivget	- MOivget
**
** Description:
**	This file defines:
**
**	MOivget - standard get integer value method
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      21-jan-1999 (canor01)
**          Remove erglf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
**  Name:	MOivget - standard get integer value method
**
**  Description:
**
**	The input object location is treated as a i4 and retrieved.
**	The output userbuf has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize is ignored.
**
**	The offset is treated as a byte offset to the input object .
**	They are added together to get the object location.
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	23-sep-92 (daveb)
**	    documented
**	 6-sep-93 (swm)
**	    Added comment to indicate that truncating cast of expression
**	    assigned to ival is valid.
*/

STATUS
MOivget( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS stat = OK;

    /* lint truncation warning if size of ptr > int, but code valid */
    i4 ival = (i4)((char *)object + offset);

    if( stat == OK )
	stat = MOlongout( MO_VALUE_TRUNCATED, ival, lsbuf, sbuf );

    return( stat );
}
