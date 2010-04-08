/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rinit.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_INIT - initialize the report formatting program.  This sets
**	up the trace calls, error processing and other system
**	functions.
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
**	History:
**		3/5/81 (ps) - written.
**		12/29/86 (rld)  modified to use IIseterr instead of IIprinterr.
**		29-dec-1992 (rdrane)
**			Use new constants in r_reset() invocations.  Removed
**			declaration of r_ing_err() since it's defined in
**			included hdr file.  (At some point, we should explicitly
**			declare this file as returning VOID).
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

VOID
r_init(argv)
char	**argv;
{
	/* start of routine */


	IIseterr(r_ing_err);	/* routine to intercept error */

	/*
	** Reset global variables
	*/
	r_reset(RP_RESET_RELMEM4,RP_RESET_RELMEM5,0);

	return;
}
