#include	<compat.h>
#include	<gl.h>
#include	<si.h>

/*SIftell
**
**	return file pointer position.
**
**	SIftell returns the current position on the stream.
**
**  History:
**	09-jul-1999 (somsa01)
**	    Updated type of offset to be OFFSET_TYPE.
*/

#ifdef SIftell
#undef SIftell
#endif

STATUS
SIftell(stream, offset)
FILE		*stream;
OFFSET_TYPE	*offset;
{
	*offset = ftell(stream);

	if (*offset < 0L)
	{
		return(FAIL);
	}
	else
	{
		return(OK);
	}
}
