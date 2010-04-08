/*
**
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**
**  Name:
**      UGcpylng.c
**
**  Function:
**      IIUGclCopyLong
**
**  Arguments:
**      char    	*source;
**      u_i4 	num_bytes;
**      char    	*dest;
**
**  Result:
**      Copy 'n' bytes from 'source' to 'dest'.  Unlike MEcopy, this can
**	copy more bytes than a u_i2's worth.
**
**
**  Side Effects:
**      None
**
**  History:
**	4/91 (Mike S) Created from Sapphire CL
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>

# define MAXUI2	(2 * MAXI2 + 1)		/* Largest u_i2 */

VOID
IIUGclCopyLong(source, num_bytes, dest)
register PTR   		source;
register u_i4      num_bytes;
register PTR   		dest;
{
	while (num_bytes > MAXUI2)
	{
		MEcopy(source, (u_i2)MAXUI2, dest);
		source	= (PTR)((char *)source + MAXUI2);
		dest	= (PTR)((char *)dest + MAXUI2);
		num_bytes -= MAXUI2;
	}

	MEcopy((PTR)source, (u_i2)num_bytes, (PTR)dest);
}
