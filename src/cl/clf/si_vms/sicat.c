# include	  <compat.h>
#include    <gl.h>
# include	  <LO.h>
# include	  <SI.h>

/* SIcat
**
**	Copy file
**
**	Copy file with location pointed to by "in" to previously opened
**	stream "out".  "Out" is usually standard output.
**
**		Copyright (c) 1983 Ingres Corporation
**		History
**			03/09/83 -- (mmm)
**				written
**			10/24/83 -- (dd) VMS CL
**      16-jul-93 (ed)
**	    added gl.h
**	22-mar-95 (albany)
**	    Removed register storage class from input_file for AXP/VMS to
**	    shut up compiler messages.
*/

/* static char	*Sccsid = "@(#)SIcat.c	1.2  6/6/83"; */

SIcat(in,out)
LOCATION	*in;	/* file location */
FILE		*out;	/* file pointer */
{
	FILE	*input_file;
	register i2	c;
	FILE		*fopen();

	SIopen(in, "r", &input_file);

	if (input_file == NULL)
	{
		return(FAIL);
	}

	while ((c = SIgetc(input_file)) != EOF)
	{
		SIputc(c, out);
	}

	SIclose(input_file);

	return(OK);

}
