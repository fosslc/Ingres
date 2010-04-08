/*
**Copyright (c) 2004 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<si.h>

#ifdef SIftell
#undef SIftell
#endif

/*SIftell
**
**	return file pointer position.
**
**	SIftell returns the current position on the stream.
**
**      03-jun-1999 (hanch04)
**          Added LARGEFILE64 for files > 2gig
*/

STATUS
SIftell(stream, offset)
FILE	*stream;
OFFSET_TYPE *offset;
{
#ifdef LARGEFILE64
	*offset = ftello64(stream);
#else
	*offset = ftell(stream);
#endif /* LARGEFILE64 */

	if (*offset < 0L)
	{
		return(FAIL);
	}
	else
	{
		return(OK);
	}
}
