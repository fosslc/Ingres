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
** Name:	mouipget.c	- MOuintget
**
** Description:
**	This file defines:
**
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
**      09-Feb-2010 (smeke01) b123226, b113797 
**          Added handling of 8-byte integers.
**/


/*{
** Name:	MOuintget	- get method for unsigned integer to string.
**
** Description:
**	Turn unsigned integers of varying sizes into strings.	
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		applied to object.
**	size		1, 2, 4 or 8.
**	object		base of object.
**	lsbuf		size of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		is written.
**
** Returns:
**	OK
**	MO_BAD_SIZE
**	MO_VALUE_TRUNCATED	buffer too short.
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	11-Mar-1998 (merja01)
**		Use generic sizes for datatypes.  Incorrectly receiving 
**		MO_BAD_SIZE on axp_osf 64 bit because of datatype size
**		differences.
**      09-Feb-2010 (smeke01) b123226, b113797 
**          Added handling of 8-byte integers.
*/
STATUS 
MOuintget( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS stat = OK;
    u_i8 ival = 0;
    char *cobj;

    cobj = (char *)object + offset;

    switch( size )
    {
    case sizeof(u_char):
	ival = *(u_char *)cobj;
	break;
    case sizeof(u_short):
	ival = *(u_short *)cobj;
	break;
    case sizeof(u_i4):
	ival = *(u_i4 *)cobj;
	break;
    case sizeof(u_i8):
	ival = *(u_i8 *)cobj; 
	break;
    default:
	stat = MO_BAD_SIZE;
	break;
    }
    
    if( stat == OK )
    {
	stat = MOulongout( MO_VALUE_TRUNCATED, ival, lsbuf, sbuf );
    }

    return( stat );
}

