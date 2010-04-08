/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>
# include       <er.h>

/*
**   R_ADVANCE - gets the next RTEXT command.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_p_ifs.
**
**	Side Effects:
**		Sets Cact_type, Cact_attribute, Cact_command, Cact_rtext
**		Advances current action (r_gch_act).
**
**	Trace Flags:
**		none.
**
**	History:
**		12/5/83 (gac)	written.
**		12/1/84 (rlm)	modified to use r_gch_act for ACT reduction
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
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
r_advance()
{
	ACT *next,*r_gch_act();

	if ((next = r_gch_act((i2) 0,(i4) 1)) != NULL)
	{
		Cact_type = next->act_section;
		Cact_attribute = next->act_attid;
		Cact_command = next->act_command;
		Cact_rtext = next->act_text;
	}
	else
	{
		r_syserr(E_RW0007_r_advance_unexp_end);
	}

	return;
}
