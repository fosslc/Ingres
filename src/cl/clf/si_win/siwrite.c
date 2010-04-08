/*
** Copyright (C) 1983, 2000 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<si.h>
#include	<st.h>
#include	<lo.h>
#include	<st.h>
#include	<errno.h>
#include	"silocal.h"

/*SIwrite
**
**	Write data from stream.
**
**	Write an item of type (*pointer) to stream f (already opened
**	for write) starting from *pointer. Return number of bytes
**	written in count.
**
**	History
**	03/09/83 -- (mmm)
**	    written
**	8-jun-93 (ed)
**	    moved compat.h to beginning of list
**  13-feb-2001 (lewhi01)
**      QEP and PRINTQRY output sent to OpenROAD trace window.
**	28-jul-2000 (somsa01)
**	    Added SIstd_write().
**	10-aug-2000 (somsa01)
**	    Slightly changed argument to SIstd_write().
*/

/* static char	*Sccsid = "@(#)SIwrite.c	3.3	9/19/84"; */

STATUS
SIwrite(numofbytes, pointer, count, stream)
i4	numofbytes;
char	*pointer;
i4	*count;
FILE	*stream;
{
	STATUS 		status;
	extern int	errno;		/* system supplied var of type int */
	i4		numwritten;
	STATUS		LOerrno();
	                        /* Max to stdout at one time is 2048 */
	char		buf[STDOUT_MAXBUFSIZ+1];

	errno = 0;

  /* lewhi01 - 96123
    ** check for stream being NULL (meaning stdout) and
	** if it is, call SIprintf instead, assuming a null-terminated
	** string.
	** Cannot assume a null-terminated string! - copy to buf.
  */
  if ((GetOpenRoadStyle()) && (stream == stdout))
  {
	STlcopy(pointer, buf, min(numofbytes, sizeof(buf) - 1));
	SIprintf("%s", buf);
  }
  else
  {
	/*Fwrite appends 1 item of data of size typesize beginning at
	**pointer to the named output stream. It returns the number of
	**items actually written.
	*/
	numwritten = fwrite(pointer, 1, numofbytes, stream);
	*count = numwritten;

	if ((numwritten == 0) && (numofbytes > 0))
	{
		/* fwrite returns 0 on error */
		status = LOerrno((i4) errno);
	}
	else if (*count < numofbytes)
	{
		/* not all the bytes could be written */
		status = FAIL;
	}
	else
	{

		status = OK;

	}
  }
	return(status);
}


/*
**
**  Name: SIstd_write()	- Write to standard stream
**
**  Description:
**	This function takes a null-terminated string and writes it to the
**	appropriate standard stream (stdin, stdout, stderr).
**
**  Inputs:
**	std_stream	- SI_STD_IN, SI_STD_OUT, or SI_STD_ERR
**	*str		- Pointer to a null-terminated string
**
**  Outputs:
**	none
**	Returns:
**	    return from WriteFile()
**	Exceptions:
**	    none
**
**  Side Effects:
**	none
**
**  History:
**	28-jul-2000 (somsa01)
**	    Created.
*/

STATUS
SIstd_write(i4 std_stream, char *str)
{
    ULONG	bytes_written;
    HANDLE	stream;
    BOOL	status;

    switch (std_stream)
    {
	case SI_STD_IN:
	    stream = GetStdHandle(STD_INPUT_HANDLE);
	    break;

	case SI_STD_OUT:
	    stream = GetStdHandle(STD_OUTPUT_HANDLE);
	    break;

	case SI_STD_ERR:
	    stream = GetStdHandle(STD_ERROR_HANDLE);
	    break;
    }

    status = WriteFile(stream, str, STlength(str), &bytes_written, NULL);
    if (status == TRUE)
	FlushFileBuffers(stream);

    return(status);
}
