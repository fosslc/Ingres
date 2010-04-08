/*
**Copyright (c) 2004 Ingres Corporation
*/
/* static char	Sccsid[] = "@(#)sinit.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 

/*
**   S_INIT - initialize the report specifier program.  This sets
**	up the trace calls, error processing and other system
**	functions.
**	It also sets up some of the dynamic areas of the report.
**
**	Parameters:
**		argv - command line argument list.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Changes values of global variables:
**			T_flags.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		12.0.
**
**	History:
**		6/5/81 (ps) - written.
**		12/22/86 (rld)  modified to use IIseterr instead of IIprinterr.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



s_init (argv)
char		**argv;
{
	/* external declarations */

	FUNC_EXTERN	i4	r_ing_err();	/* routine to intercept
						** EQUEL errors */
	/* start of routine */


	IIseterr(r_ing_err);	/* routine to intercept error */

	return;
}
