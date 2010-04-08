/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)renvset.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 	/* include rcons.h also */
# include	 <rglob.h> 
# include	<er.h>
# include	<ug.h>

/*
**   R_ENV_SET - read in the REPORTS system table and set up the
**	fields from it.  If no report is found, assume default.
**	can be set up from a table of that name.
**
**	Parameters:
**		none.
**
**	Returns:
**		TRUE	if report found.
**		FALSE	if no report found.
**
**	Called by:
**		r_rep_load.
**
**	Side Effects:
**		Sets up most of the En_ fields, En_rtype.
**		Also sets up the En_ren.
**
**	Error messages:
**		4.
**
**	Trace Flags:
**		1.0, 1.4.
**
**	History:
**		3/5/81 (ps)	written.
**		12/29/81 (ps)	modified for 2 table version.
**		9/12/82 (ps)	add ref to En_ren.
**		1/8/85 (rlm)	tagged memory allocation.
**		19-dec-88 (sylviap)
**			Changed memory allocation to use FEreqmem.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

bool
r_env_set()
{
	/* internal declarations */


	/* start of routine */


	/* find out whether report is user or dba owned */

        if ((En_ren = (REN *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(sizeof(REN)), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_env_set"));
	}

        if ((En_ren->ren_type = (char *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(sizeof(2)), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_env_set"));
	}

	En_ren->ren_report = En_report;

	if (r_chk_rep(En_ren) == NULL)
	{
		/* report doesn't exist for user or dba */
		return (FALSE);
	}

	/* Now transfer the fields to other globals */

	En_rep_owner = En_ren->ren_owner;
	En_acount = En_ren->ren_acount;
	En_qcount = En_ren->ren_qcount;
	En_scount = En_ren->ren_scount;
	En_bcount = En_ren->ren_bcount;
	En_rtype = *(En_ren->ren_type);


	return(TRUE);
}
