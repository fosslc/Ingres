#include	<compat.h>
#include	<gl.h>
#include	<si.h>
#include	<errno.h>

/* SIgetrec
**
**	Get next input record
**
**	Get the next record from stream, previously opened for reading.
**	SIgetrec reads n-1 characters, or up to a full record, whichever
**	comes first, into buf.  The last char is followed by a newline.
**	FAIL is returned on error, else OK.  ON unix, a record on a file
**	is a string of bytes terminated by a newline.  ON systems with for-
**	matted files, a record may be defined differently.
**
**Copyright (c) 2004 Ingres Corporation
**		History
**			03/09/83 -- (mmm)
**				written
**			2/28/85 -- (dreyfus) return ENDFILE on EOF. Fail on error
**				OK otherwise.
*/

/* static char	*Sccsid = "@(#)SIgetrec.c	3.2  4/11/84"; */

STATUS
SIgetrec(buf,n,stream)
char	*buf;	/* read record into buf and null terminate */
i4	n;	/*(max number of characters - 1) to be read */
FILE	*stream;/* get reacord from file stream */
{
	char	*return_ch;
	char	*fgets();

	return_ch = fgets(buf, n, stream);

	if (return_ch == NULL)
	{
		return(ENDFILE);

	}
	return OK;
}
