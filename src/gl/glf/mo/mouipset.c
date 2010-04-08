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
** Name:	mouipset.c	MOuintset
**
** Description:
**	This file defines:
**
**	MOuintset	- set method for unsigned integer from string
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
** Name:	MOuintset	- set method for unsigned integer from string
**
** Description:
**	Set method for taking a string and putting it into an unsigned integer
**	of varying sizes.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		applied to object.
**	lsbuf		length of input buffer.
**	sbuf		input buffer
**	size		1, 2, or 4.
**	object		base location of integet to set.
**
** Outputs:
**	object		(+offset) is set for size bytes.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED	overflow.
**	MO_BAD_SIZE		size was invalid
**
** History:
**	24-sep-92 (daveb)
**	    documented, made ival u_i4 to match str_to_uns call.
**	11-Mar-1998 (merja01)
**		Use generic sizes for datatypes.  Incorrectly receiving 
**		MO_BAD_SIZE on axp_osf 64 bit because of datatype size
**		differences.

*/

STATUS 
MOuintset( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    STATUS stat;
    u_i4 ival;
    
    char *cobj = (char *)object + offset;

    MO_str_to_uns( sbuf, &ival );
    
    switch( size )
    {
    case sizeof(u_char):

	if( ival > MAXI1 || ival < MINI1)
	    stat = MO_VALUE_TRUNCATED;
	*(u_char *)cobj = (u_char)ival;
	break;

    case sizeof(u_short):

	if( ival > MAXI2 || ival < MINI2)
	    stat = MO_VALUE_TRUNCATED;
	*(u_short *)cobj = (u_short)ival;
	break;

    case sizeof(u_i4):

	*(u_i4 *)cobj = ival;
	break;

    default:

	stat = MO_BAD_SIZE;
	break;
    }
    return( stat );
}


