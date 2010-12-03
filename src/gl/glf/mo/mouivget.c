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
** Name:	mouivget.c	MOuivget
**
** Description:
**	This file defines:
**
**	MOuivget	 - get unsigned integer
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
** Name:	MOuivget	 - get unsigned integer
**
** Description:
**	Get unsigned integer value, returned as left zero padded string.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		applied to object.
**	size		ignored
**	object		treated as u_i4.
**	lsbuf		size of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written wit
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	 6-sep-93 (swm)
**	    Added comment to indicate that truncating cast of expression
**	    assigned to ival is valid.
**	1-Dec-2010 (kschendel)
**	    Stop warnings.
*/

STATUS 
MOuivget( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS stat = OK;

    u_i4 ival = (u_i4)((SCALARP)object + offset);
    
    if( stat == OK )
	stat = MOulongout( MO_VALUE_TRUNCATED, ival, lsbuf, sbuf );

    return( stat );
}

