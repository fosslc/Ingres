/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <sp.h>
# include <mo.h>

/**
** Name:	monoset.c	- MOnoset
**
** Description:
**	This file defines:
**
**	MOnoset - standard set method for read-only objects
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (canor01)
**	    Remove erglf.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/
    

/*{
**  Name:	MOnoset - standard set method for read-only objects
**
**  Description:
**
**
**		Don't do anything, and return MO_NO_WRITE.
**  Inputs:
**	 offset
**		ignored.
**	 luserbuf
**		ignored.
**	 userbuf
**		ignored.
**	 objsize
**		ignored.
**	 object
**		ignored.
**  Outputs:
**	 object
**		ignored.
**  Returns:
**	 MO_NO_WRITE
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS 
MOnoset( i4  flags, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    return( MO_NO_WRITE );
}

