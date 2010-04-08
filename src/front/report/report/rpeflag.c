/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpeflag.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	 <cm.h> 

/*
**   R_P_EFLAG - turn off a flag.  This sets up an P_END structure with
**	the other structure within it.
**	There are no parameters to these commands.
**
**	Parameters:
**		code	code of command to use if no garbage.
**
**	Returns:
**		none.
**
**	Called by:
**		r_act_set, r_p_end, r_p_begin.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		104.
**
**	Trace Flags:
**		100, 102.
**
**	History:
**		1/6/82 (ps)	written.
**            11/5/90 (elein) 34229
**                    Ignore trailing words on BEGIN and BEGIN RBF*.
**		02-jul-91 (kirke)
**			Recoded while loop so that control variable
**			'type' gets an initial value.
**
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
r_p_eflag(code)
ACTION	code;
{
	/* start of routine */
	i4 type;

	Cact_tcmd->tcmd_code = P_END;
	Cact_tcmd->tcmd_val.t_v_long = code;

	while ((type = r_g_skip()) != TK_ENDSTRING)
		CMnext(Tokchar);

	return;
}
