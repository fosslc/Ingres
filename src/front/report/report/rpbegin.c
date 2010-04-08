/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpbegin.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h> 
# include	<rglob.h> 

/*
**   R_P_BEGIN - process a .BEGIN command which starts a block.  The format
**	of the .BEGIN command is:
**		.BEGIN WITHIN | WI | BLOCK | BLK | RBFxxxx
**	This starts a .WITHIN or .BLOCK command.  Either of these can
**	also be specified as a single word (i.e. .BEGINWITHIN or .BEGINBLOCK).
**	The thought is that this 
**	command will be expanded for other block commands in the 
**	future.
**
**	The RBFxxx parameters are
**	used internally by RBF.
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
**		10-apr-90 (cmr) For begin rbfaggs strip out the following
**				agg # so that a match occurs in r_gt_code().
**				It's a NOP for other begin codes.
**		11/5/90 (elein) 34229
**			Ignore trailing words on BEGIN and BEGIN RBF*.
**			by calling rpflag/rpeflag.
**		10-aug-92 (rdrane)
**			The ANSI compiler complains about the enclosing WHILE
**			loop which, given the existing code, is logically
**			equivalent to an IF.  So, convert the WHILE block to an
**			IF block.
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
r_p_begin()
{
	/* internal declarations */

	char		*aname;			/* name of found attribute */
	char		*delim;
	i4		type;			/* token type */

	/* start of routine */


	if ((type = r_g_skip()) != TK_ENDSTRING)
	{
		switch (type)
		{
			case(TK_ALPHA):
				aname = r_g_name();
				CVlower(aname);
				/* for begin rbfaggs strip agg # */
				/* nop for others		 */
				switch(r_gt_code(aname))
				{
					case(C_WITHIN):
						r_p_within();
						break;

					case(C_BLOCK):
						r_p_flag(P_BLOCK);
						break;

					case(C_FF):
						/* for the hell of it */
						r_p_flag(P_FF);
						break;

					case(C_UL):
						/* for the hell of it */
						r_p_flag(P_UL);
						break;

				/* the rest are RBF internal commands */

					case(C_RBFAGGS):
						/* start of aggs */
						r_p_flag(P_RBFAGGS);
						break;

					case(C_RBFFIELD):
						/* start of field */
						r_p_flag(P_RBFFIELD);
						break;

					case(C_RBFHEAD):
						/* start of column head */
						r_p_flag(P_RBFHEAD);
						break;

					case(C_RBFPBOT):
						/* start of page bottom */
						r_p_flag(P_RBFPBOT);
						break;

					case(C_RBFPTOP):
						/* start of page top */
						r_p_flag(P_RBFPTOP);
						break;

					case(C_RBFSETUP):
						/* start of report setup */
						r_p_flag(P_RBFSETUP);
						break;

					case(C_RBFTITLE):
						/* start of report title */
						r_p_flag(P_RBFTITLE);
						break;

					case(C_RBFTRIM):
						/* start of trim */
						r_p_flag(P_RBFTRIM);
						break;

					default:
						r_p_flag(P_NOOP);
						break;
				}
				break;

			default:
				r_p_flag(P_NOOP);
				break;

		}
		return;
	}

	/* only a plain old .BEGIN will get here */

	Cact_tcmd->tcmd_code = P_BEGIN;

	return;
}
