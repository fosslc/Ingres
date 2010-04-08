/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <si.h> 
# include	 <lo.h> 
# include	 <rcons.h> 
# include	<er.h>

/*
**   R_FOPEN - open a file for reading.
**
**	Parameters:
**		name - file name.
**
**	Returns:
**		OK	if no problems occur..
**		FAIL	if problems occur.
**
**	Trace Flags:
**		2.0, 2.14.
**
**	Error Messages:
**		none.
**
**	History:
**		6/1/81 (ps) - written.
**		4/6/83 (mmm)- added locations and fiddled with returns
**		11-sep-1992 (rdrane)
**			Explicitly return STATUS - don't confuse with
**			an integer value!
*/



STATUS
r_fopen(name, fp)
char	*name;
FILE	**fp;
{
	/* external declarations */


	/* internal declarations */

	STATUS		status;
	LOCATION	loc;

	/* start of routine */

	
	LOfroms(PATH & FILENAME, name, &loc);


	status = SIopen(&loc, ERx("r"), fp);


	if (status != OK)
	{
		*fp = NULL;
	}

	return(status);

}
