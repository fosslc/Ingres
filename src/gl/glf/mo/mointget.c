/*
** Copyright (c) 1986, 2005 Ingres Corporation
**
**
*/


# include <compat.h>
# include <gl.h>
# include <sp.h>
# include <mo.h>
# include "moint.h"

/**
** Name:	mointget.c	- MOintget
**
** Description:
**	This file defines:
**
**	MOintget - standard get integer at an address method
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
**      25-Jan-2005 (nansa02)
**          Added a case for i8 in MOintget but typecasted the i8 to i4 as 
**          all the MO codes are not compatible with i8.This is temporary
**          fix as scope of porting all the codes is large.Limitation of 
**          this fix being the value of the variable it can handle which
**          should not grow more than the size of i4. 
**          Bug(113797) 
**      09-Feb-2010 (smeke01) b123226, b113797 
**          Added handling of 8-byte integers. This has enabled a better
**          fix for b113797.
**/


/*{
**  Name:	MOintget - standard get integer at an address method
**
**  Description:
**
**
**	Convert the integer at the object location to a character
**	string.  The offset is treated as a byte offset to the input
**	object .  They are added together to get the object location.
**	
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize comes from the class definition, and is the length
**	of the integer, one of sizeof(i1), sizeof(i2), or
**	sizeof(i4).
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
**      09-Feb-2010 (smeke01) b123226, b113797 
**          Added handling of 8-byte integers. This has enabled a better
**          fix for b113797.
*/

STATUS 
MOintget( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS stat = OK;
    i8 ival = 0;
    char *cobj;

    cobj = (char *)object + offset;

    switch( size )
    {
    case sizeof(i1):
	ival = *(i1 *)cobj;
	break;
    case sizeof(i2):
	ival = *(i2 *)cobj;
	break;
    case sizeof(i4):
	ival = *(i4 *)cobj;
	break;
    case sizeof(i8):
	ival = *(i8 *)cobj; 
	break;
    default:
	stat = MO_BAD_SIZE;
	break;
    }
    
    if( stat == OK )
    {
	stat = MOlongout( MO_VALUE_TRUNCATED, ival, lsbuf, sbuf );
    }

    return( stat );
}
