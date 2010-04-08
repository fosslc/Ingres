/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpeprint.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_P_EPRINT - set up the TCMD structures from an endprint command.
**	If a .PRINTLN is in effect, write out the newline.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Error messages:
**		none.
**
**	Trace Flags:
**		3.0, 3.5.
**
**	History:
**		1/5/82 (ps)	written.
**		1/19/84 (gac)	changed for printing expressions.
**		8/11/89 (elein) garnet
**			Added item for NewLine tcmd.  Should be NONE.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_p_eprint()
{
	ITEM *item;
	/* start of routine */




	r_p_eflag(P_PRINT);

	if (St_prline)
	{	/* put in a newline command */
		Cact_tcmd = r_new_tcmd(Cact_tcmd);
		Cact_tcmd->tcmd_code = P_NEWLINE;
		Cact_tcmd->tcmd_val.t_v_long = NL_DFLT;
		item = (ITEM *)MEreqmem(0,sizeof(ITEM), TRUE, (STATUS *)NULL);
		Cact_tcmd->tcmd_item = item;
		(Cact_tcmd->tcmd_item)->item_type = I_NONE;
	}

	St_prline = FALSE;

	return;
}
