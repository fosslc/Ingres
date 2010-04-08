/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfinit.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<erug.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFINIT - initialize RBF.  This sets
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
**	Trace Flags:
**		200, 201.
**
**	History:
**		2/10/82 (ps)	written.
**		10/4/82 (jrc)   modified to add testing files.
**		12/22/86 (rld)  modified to use IIseterr instead of IIprinterr.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**	22-feb-90 (sylviap)
**		Added initializing yn_tbl.  Used for listpick and abbreviation
**		changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
rFinit (argv)
char		**argv;
{
	/* external declarations */

	FUNC_EXTERN	i4	r_ing_err();	/* routine to intercept
						** EQUEL errors */

	/* start of routine */

# 	ifdef	xRTR1

	TRmaketrace(argv, 'R', T_flags, TRUE, T_SIZE);	/* set up trace flags */

# 	endif

	/* Reset the global variables */


	IIseterr(r_ing_err);	/* routine to intercept error */
	rf_rep_init();

	/* initialize the yes/no table used for ListPick */
 	yn_tbl[0] = ERget (F_UG0002_Yes2);
	yn_tbl[1] = ERget (F_UG0007_No2);
	return;
}
