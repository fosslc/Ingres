#include	<compat.h>
#include	<gl.h>
#include	<si.h>

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
**  History:
**	09-jul-1999 (somsa01)
**	    Updated type of offset to be OFFSET_TYPE.
**	21-sep-2000 (mcgem01)
**	    More nat to i4 changes.
*/

#ifdef SIfseek
#undef SIfseek
#endif

STATUS
SIfseek(stream, offset, ptrname)
FILE		*stream;
OFFSET_TYPE	offset;
i4		ptrname;
{
	i2	exitstatus;

	exitstatus = fseek(stream, (long)offset, (i4)ptrname);

	if (exitstatus == -1)
	{
		return(FAIL);
	}
	else
	{
		return(OK);
	}
}
