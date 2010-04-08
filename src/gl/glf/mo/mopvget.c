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
** Name:	mopvget.c	MOpvget
**
** Description:
**	This file defines:
**
**	MOpvget	 - get pointer value
**
** History:
**	5-May-1993 (daveb)
**	    created
**      21-jan-1999 (canor01)
**          Remove erglf.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
**  Name:	MOpvget - get pointer method
**
**  Description:
**
**
**	The input object location is treated as a pointer and retrieved.
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize is ignored.
**
**	The offset is treated as a byte offset to the input object .
**	They are added together to get the object location.
**
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		ignored.
**	 object
**		a pointer.
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
**
**  History:
**	5-May-1993 (daveb)
**	    created.
*/

STATUS 
MOpvget( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS stat = OK;

    PTR pval = (PTR)((char *)object + offset);
    
    stat = MOptrout( MO_VALUE_TRUNCATED, pval, lsbuf, sbuf );

    return( stat );
}

