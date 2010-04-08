#include	<compat.h>
#include	<gl.h>
#include	<si.h>
#include	<errno.h>

/*SIread
**
**	Read data from stream
**
**	Read an item of type (*pointer) from stream f (already opened
**	for read) into space starting at *pointer.  Return number of bytes
**	read in count.
**
**		Copyright (c) 1997 Ingres Corporation
**		History
**			03/09/83 -- (mmm)
**				written
**			16-apr-1997 (canor01)
**				Return ENDFILE on partial read at end-of-file.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
*/

STATUS
SIread(stream, numofbytes, count, pointer)
register FILE		*stream;
register i4     	numofbytes;
register i4     	*count;
char			*pointer;
{
	STATUS		status;
	extern int	errno;		/* system supplied var of type int */
	i4		numread;
	STATUS		LOerrno();

	errno = 0;

	/*fread:
	**	fread reads, into a block beginning at pointer, "numofbytes" of
	**	bytes from the named input stream. It returns 
	**	the number of items actually read.
	*/
	numread = fread(pointer, 1, numofbytes, stream);
	*count = numread;

	if (*count == 0)
	{
		switch(errno)
		{
		  case 0:
#		  ifdef UTS
		  case ENOTTY:	/* UTS always tells you "this is not a tty" */
#		  endif /* UTS */
			status = ENDFILE;
			break;

		  default:
			status = LOerrno((i4) errno);
			break;

		}

	}
	else if (*count < numofbytes)
	{
		/* fread did not read total number of bytes */
		if ( feof(stream) )
		    status = ENDFILE;
		else
		    status = FAIL;

	}
	else
	{
		status = OK;
	}

	return(status);

}
