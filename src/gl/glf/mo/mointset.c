/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <cv.h>
# include <sp.h>
# include <mo.h>

/**
** Name:	mointset.c	- MOintset
**
** Description:
**	This file defines:
**
**	MOintset - standard set integer method
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	5-May-1993 (daveb)
**	    include <cv.h> for CVal.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
**  Name:	MOintset - standard set integer method
**
**  Description:
**
**
**	Convert a caller's string to an integer for the instance.
**
**	The offset is treated as a byte offset to the input object .
**	They are added together to get the object location.
**	
**	The input objsize is the size of the output integer, as in
**	sizeof(i1), sizeof(i2), or sizeof(i4).  The object location is
**	taken to be the address of the integer.  The userbuf contains
**	the string to convert, and luserbuf.  is ignored.
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 luserbuf
**		the length of the user buffer, ignored.
**	 userbuf
**		the user string to convert.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer.
**  Outputs:
**	 object
**		Modified by the method.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_BAD_SIZE
**		if the object size isn't something that can be handled.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS 
MOintset( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    STATUS stat;
    i4 ival;
    
    char *cobj = (char *)object + offset;

    stat = CVal( sbuf, &ival );
    
    if( stat == OK )
    {
	switch( size )
	{
	case sizeof(i1):
	    if( ival > MAXI1 || ival < MINI1)
		stat = MO_VALUE_TRUNCATED;
	    *(i1 *)cobj = (i1)ival;
	    break;
	case sizeof(i2):
	    if( ival > MAXI2 || ival < MINI2)
		stat = MO_VALUE_TRUNCATED;
	    *(i2 *)cobj = (i2)ival;
	    break;
	case sizeof(i4):
	    *(i4 *)cobj = (i4)ival;
	    break;
	default:
	    stat = MO_BAD_SIZE;
	    break;
	}
    }
    return( stat );
}

