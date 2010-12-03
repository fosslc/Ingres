/*
**Copyright (c) 2004 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<si.h>

#ifdef SIfseek
#undef SIfseek
#endif
/*
** Forward references
*/
STATUS	SIfseek(FILE *, OFFSET_TYPE, i2);

/*SIfseek
**
**	Reposition a file pointer.
**
**	SIfseek sets the position of the next input or output
**	operation on the stream.  The new position is at the
**	signed distance offset bytes from the beginning, the
**	current position, or the end of the file, according as 
**	ptrname has the value 0, 1, or 2.
**
**      03-jun-1999 (hanch04)
**          Added LARGEFILE64 for files > 2gig
**      12-Jun-2001 (hanje04)
**          Reintroduced MEfill, SIftell etc. as they are required by
**          OpenRoad 4.0
**      15-nov-2010 (stephenb)
**          Proto SIfseek for local undef.
*/

STATUS
SIfseek(stream, offset, ptrname)
FILE	*stream;
OFFSET_TYPE	offset;
i2	ptrname;
{
	i2	exitstatus;

#ifdef LARGEFILE64
	exitstatus = fseeko64(stream, (OFFSET_TYPE)offset, (i4)ptrname);
#else
	exitstatus = fseek(stream, (OFFSET_TYPE)offset, (i4)ptrname);
#endif /* LARGEFILE64 */

	if (exitstatus == -1)
	{
		return(FAIL);
	}
	else
	{
		return(OK);
	}
}
