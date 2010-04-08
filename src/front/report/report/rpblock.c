/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpblock.c	30.1	11/14/84"; */

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
**   R_P_BLOCK - process the .BLOCK command.  
**	There are no parameters to the command.
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
**		none.
**
**	Error Messages:
**		104.
**
**	Trace Flags:
**		100.
**
**	History:
**		1/6/82 (ps)	written.
*/



r_p_block()
{
	/* start of routine */


	if (r_g_skip() != TK_ENDSTRING)
	{	/* garbage on line */
		r_error(104, NONFATAL, Cact_tname, Cact_attribute,
			Cact_command, Cact_rtext, NULL);
		return;
	}

	Cact_tcmd->tcmd_code = P_BLOCK;

	return;
}
