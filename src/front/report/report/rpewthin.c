/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpewithin.c	30.1	11/14/84"; */

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
**   R_P_EWITHIN - end a .WITHIN block of commands.  This pops the
**	stack and changes some of the program variables.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_p_end, r_act_set, r_p_within, r_end_type.
**
**	Side Effects:
**		May pop the stack, may set St_cwithin, St_ccoms.
**
**	Error Messages:
**		104, 166.
**
**	Trace Flags:
**		100, 103.
**
**	History:
**		1/8/82 (ps)	written.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_p_ewithin()
{
	/* internal declarations */

	TCMD	*stcmd;			/* tcmd struct from stack */

	/* start of routine */


	/* add a .TOP command before the end of the .WITHIN */

	Cact_tcmd->tcmd_code = P_TOP;
	Cact_tcmd = r_new_tcmd(Cact_tcmd);

	Cact_tcmd->tcmd_code = P_END;
	Cact_tcmd->tcmd_val.t_v_long = P_WITHIN;

	stcmd = r_pop_be(P_WITHIN);


	if ((stcmd == NULL) || (stcmd->tcmd_code != P_WITHIN))		
	{
		r_error(166, NONFATAL, Cact_tname, Cact_attribute, NULL);
	}

	St_cwithin = NULL;
	St_ccoms = NULL;

	if (r_g_skip() != TK_ENDSTRING)
	{	/* garbage at end of line */
		r_error(104, NONFATAL, Cact_tname, Cact_attribute, 
			Cact_command, Cact_rtext, NULL);
	}

	/* add an .ENDBLOCK command after the .ENDWITHIN */

	Cact_tcmd = r_new_tcmd(Cact_tcmd);
	Cact_tcmd->tcmd_code = P_END;
	Cact_tcmd->tcmd_val.t_v_long = P_BLOCK;

	return;
}
