/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpulc.c	30.1	11/14/84"; */

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
**   R_P_ULC - called to check the .ULCHAR command which sets up a
**		character for underlining.  The format is:
**			.ulchar "c"
**		where c is any character.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_act_set.
**
**	Side Effects:
**		Changes codes in Cact_tcmd.
**
**	Trace Flags:
**		3.0, 3.13.
**
**	Error Messages:
**		104, 160.
**
**	History:
**		7/23/81 (ps)	written.
**		2/13/86 (mgw)	Fixed bug 7966 - Added argument to r_g_string
**		6-aug-1986 (Joe)
**			Added TK_SQUOTE. Passed string delimiter to r_g_string.
**			Took out strip slash parameter to r_g_string.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



r_p_ulc()
{
	/* internal declarations */

	char		*c;			/* temp storage of char */
	i4		tokvalue;

	/* start of routine */


	if ((tokvalue = r_g_skip()) != TK_QUOTE && tokvalue != TK_SQUOTE)
	{	/* must be quoted string */
		r_error(160, NONFATAL, Tokchar, Cact_tname, Cact_attribute, Cact_command, Cact_rtext, NULL);
		return;
	}

	c = r_g_string(tokvalue);

	if (r_g_skip() != TK_ENDSTRING)
	{
		r_error(104, NONFATAL, Cact_tname, Cact_attribute, Cact_command, Cact_rtext, NULL);
	}

	Cact_tcmd->tcmd_code = P_ULC;
	Cact_tcmd->tcmd_val.t_v_char = *c;	/* first char*/


	return;
}
