/*
** Copyright (C) 1983, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<si.h>
#include	<lo.h>
#include	<errno.h>

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
**	    The last submission to this file accidentally put the wrong
**	    version into piccolo for UNIX.
**	18-May-2001 (hanje04)
**	    Backing out QEP to OpenROAD trace window change (40551)
**	    GetOpenRoadStyle() is front end call and should not be in SI
**	4-Dec-2008 (kschendel)
**	    Declare errno correctly (i.e. via errno.h).
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
	i4		numwritten;
	STATUS		LOerrno();
	                        /* Max to stdout at one time is 2048 */

	errno = 0;

	/*Fwrite appends 1 item of data of size typesize beginning at
	**pointer to the named output stream. It returns the number of
	**items actually written.
	*/
	numwritten = fwrite(pointer, 1, numofbytes, stream);
	*count = numwritten;

	if ((numwritten == 0) && (numofbytes > 0))
	{
		/* fwrite returns 0 on error */
		status = LOerrno(errno);
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
    FILE	*stream;

    switch (std_stream)
    {
	case SI_STD_IN:
	    stream = stdin;
	    break;

	case SI_STD_OUT:
	    stream = stdout;
	    break;

	case SI_STD_ERR:
	    stream = stderr;
	    break;
    }

    SIfprintf(stream, "%s", str);
    SIflush(stream);

    return(TRUE);
}
