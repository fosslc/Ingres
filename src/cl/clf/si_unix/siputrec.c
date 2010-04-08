#include	<compat.h>
#include	<gl.h>
#include	<si.h>

#ifdef SIputrec
#undef SIputrec
#endif

/* SIputrec
**
**	Write output record
**
**	Copy the Null terminated string in buf to stream, which has been
**	previously opened for writing.  The null is not copied.  On systems
**	with formatted files, this call adds a record to the file.
**	On unix, the bytes are just appended to the stream, and terminated
**	by a newline.
**
**	FAIL is returned on error, else OK.
**
**Copyright (c) 2004 Ingres Corporation
**		History
**			03/09/83 -- (mmm)
**				written
**			06/27/86 -- (daveb)
**				Check error return of fputs, don't look
**				at errno.  Errno is meaningless for this
**				operation.
**      12-Jun-2001 (hanje04)
**          Reintroduced MEfill, SIftell etc. as they are required by
**          OpenRoad 4.0
*/

STATUS
SIputrec(buf, stream)
register char	*buf;
register FILE	*stream;
{
	return ( fputs(buf, stream) == EOF ? FAIL : OK );
}

