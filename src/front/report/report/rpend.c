/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpend.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h> 
# include	<rglob.h> 

/*
**   R_P_END - process an .END command which ends a block.  The format
**	of the .END command is:
**		.END [WITHIN | WI | BLOCK | BLK | RBFxxx]
**	This ends a .WITHIN or .BLOCK command.  Either of these can
**	also be specified as a single word (i.e. .ENDWITHIN or .ENDBLOCK).
**	If neither is specified, the .END command will end the most
**	recently specified of the two.  The thought is that this 
**	command will be expanded for other block commands in the 
**	future.
**
**	The RBFxxx parameters are used by RBF.
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
**	Trace Flags:
**		100, 105.
**
**	History:
**		1/6/82 (ps)	written.
**		11/1/90 (elein) 34229
**			skip characters past END
**		10-aug-92 (rdrane)
**			The ANSI compiler complains about the enclosing WHILE
**			loop which, given the existing code, is logically
**			equivalent to an IF.  So, convert the WHILE block to
**			an IF block.
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
r_p_end()
{
	/* internal declarations */

	char		*aname;			/* name of found attribute */
	i4		type;			/* token type */
	TCMD		*stcmd;			/* return from r_pop_be */

	/* start of routine */


	if ((type = r_g_skip()) != TK_ENDSTRING)
	{
		switch (type)
		{
			case(TK_ALPHA):
				aname = r_g_name();
				CVlower(aname);
				switch(r_gt_code(aname))
				{
					case(C_WITHIN):
						r_p_ewithin();
						return;
						break;

					case(C_BLOCK):
						r_p_eflag(P_BLOCK);
						return;
						break;

					case(C_FF):
						/* for the hell of it */
						r_p_eflag(P_FF);
						return;
						break;

					case(C_UL):
						/* for the hell of it */
						r_p_eflag(P_UL);
						return;
						break;

				/* the rest are RBF internal commands */

					case(C_RBFAGGS):
						/* end of aggs */
						r_p_eflag(P_RBFAGGS);
						break;

					case(C_RBFFIELD):
						/* end of field */
						r_p_eflag(P_RBFFIELD);
						break;

					case(C_RBFHEAD):
						/* end of column head */
						r_p_eflag(P_RBFHEAD);
						break;

					case(C_RBFPBOT):
						/* end of page bottom */
						r_p_eflag(P_RBFPBOT);
						break;

					case(C_RBFPTOP):
						/* end of page top */
						r_p_eflag(P_RBFPTOP);
						break;

					case(C_RBFSETUP):
						/* end of report setup */
						r_p_eflag(P_RBFSETUP);
						break;

					case(C_RBFTITLE):
						/* end of report title */
						r_p_eflag(P_RBFTITLE);
						break;

					case(C_RBFTRIM):
						/* end of trim */
						r_p_eflag(P_RBFTRIM);
						break;

					default:
						r_p_eflag(P_NOOP);
						break;

				}
				break;

			default:
				r_p_eflag(P_NOOP);
				break;


		}
		return;
	}

	/* only a plain old .END will get here */
	Cact_tcmd->tcmd_code = P_END;
	Cact_tcmd->tcmd_val.t_v_long = P_ERROR;


	/* see if a .WITHIN is in effect */

	stcmd = r_pop_be(P_ERROR);
	if (stcmd != NULL)
	{	/* special processing of stack TCMD's */
		switch(stcmd->tcmd_code)
		{
			case(P_WITHIN):
				r_psh_be(stcmd);
				r_p_ewithin();
				break;

			default:
				/* nothing comes here */
				break;
		}
	}

	return;
}
