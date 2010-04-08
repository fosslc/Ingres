/*****************************************************************************
**
**	Copyright (c) 1983, 1991 Ingres Corporation
**
*****************************************************************************/

#include <compat.h>
#include <memory.h>
#include <me.h>

#ifndef __IBMC__
#pragma intrinsic(memcpy, memset)
#endif

/*****************************************************************************
**	Name:
**		MEmove.c
**
**	Function:
**		MEmove
**
**	Arguments:
**		i2		source_length;
**		char	*source;
**		c1		pad_char;
**		i2		dest_length;
**		char	*dest;
**
**	Result:
**
**	History:
**	    27-may-97 (mcgem01)
**		Clean up compiler warnings.
**
*****************************************************************************/
VOID
MEmove(u_i2    slen,
       PTR   source,
       char  pad_char,
       u_i2    dlen,
       PTR   dest)
{
	if (!dlen ||
	    !source ||
	    !dest)
		return;

	if (slen >= dlen)
		memcpy(dest,
		       source,
		       dlen);
	else {
		if (slen)
			memcpy(dest,
			       source,
			       slen);
		memset((char *) dest + slen,
		       pad_char,
		       dlen - slen);
	}
}
