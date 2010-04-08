/****************************************************************************
**
** Copyright (c) 1983, 2001 Ingres Corporation
**
****************************************************************************/

#include	<compat.h>
#include	<gl.h>
#include	<si.h>

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
**	History:
**	    01-apr-2001 (mcgem01)
**		Reintroduced for backward compatibility.
*/

#ifdef SIputrec
#undef SIputrec
#endif

STATUS
SIputrec(buf, stream)
register char	*buf;
register FILE	*stream;
{
	return ( fputs(buf, stream) == EOF ? FAIL : OK );
}

