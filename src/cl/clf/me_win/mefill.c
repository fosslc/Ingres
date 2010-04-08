/*****************************************************************************
**
** Copyright (c) 1983, 2004 Ingres Corporation
**
**
*****************************************************************************/

#include <compat.h>
#include <memory.h>
#include <me.h>

# ifdef MEfill
# undef MEfill
# undef MEfilllng
# endif


/*****************************************************************************
**	Name:
**		MEfill.c
**
**	Function:
**		MEfill
**
**	Arguments:
**		u_i2		n;
**		unsigned char 	val;
**		PTR		mem;
**
**	Result:
**		Fill 'n' bytes starting at 'mem' with the unsigned
**		character 'val'.
**
**	Side Effects:
**		None
**
**	History:
**	    17-oct-95 (emmag)
**		Use macro for MEfill instead of this function call.
**	    17-jun-1998 (canor01)
**	 	Use correct calling parameters in MEfilllng dummy function
**		(i4 instead of u_i2).
**	    30-Nov-2004 (drivi01)
**	  	Updated i4 to SIZE_TYPE to port change #473049 to windows.
*****************************************************************************/
VOID
MEfill( SIZE_TYPE n, uchar val, PTR mem)
{
	memset(mem, val, n);
}
