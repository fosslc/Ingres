/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfclose.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <si.h> 
# include	 <rcons.h> 

/*
**   R_FCLOSE - close a file.
**
**	Parameters:
**		fp - a file pointer.
**
**	Returns:
**		returns OK if ok.
**		-1 if problems occur.
**
**	Trace Flags:
**		2.0, 2.14.
**
**	Error Messages:
**		none.
**
**	History:
**		6/1/81 (ps) - written.
**		4/5/83 (mmm)- use compat I/O
*/


STATUS
r_fclose(fp)
FILE	*fp;
{
	/* external declarations */


	/* start of routine */



	SIclose(fp);
	return(OK);
}
