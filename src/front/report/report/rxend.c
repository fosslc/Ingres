/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxend.c	30.1	11/14/84"; */

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
**   R_X_END - end a command.  The P_END structure is followed by the
**	code of the command which is to be ended.
**
**	Parameters:
**		code	code of command to end, either P_PRINT,
**			P_FF, P_UL, P_WITHIN, P_BLOCK.  If P_ERROR,
**			then the .END is to the last block.
**			For RBF, the codes are P_SETUP, P_CHEAD,
**			P_RTITLE, P_PBOT, P_PTOP which are ignored here.
**
**	Returns:
**		none.
**
**	Side Effects:
**		May update St_underling, St_ff_on...
**
**	Called by:
**		r_x_tcmd.
**
**	Error messages:
**		none.
**
**	Trace Flags:
**		150, 151.
**
**	History:
**		1/5/81 (ps)	modified to allow PL of 0.
**		1/19/84 (gac)	changed for printing expressions.
**		3/21/90 (elein) performance
**			Only call r_stp_adjst iff in adjusting
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
r_x_end(code)
i4	code;

{
	/* internal declarations */

	register TCMD	*tcmd;			/* from the stack */

	/* start of routine */



	if ((code==P_ERROR)||(code==P_WITHIN))
	{	/* undefined .END.  Pop the stack */
		tcmd = r_pop_be(code);
		if (tcmd != NULL)
		{
			code = tcmd->tcmd_code;
		}
	}

	switch(code)
	{
		case(P_PRINT):
			if( St_adjusting)
			{
				r_stp_adjust();
			}
			break;

		case(P_UL):
			St_underline = FALSE;
			break;

		case(P_FF):
			if (!St_ff_set)
			{
				St_ff_on = FALSE;
			}
			break;

		case(P_BLOCK):
			r_x_eblock();
			break;

		case(P_WITHIN):
			r_psh_be(tcmd);		/* if last one, r_x_ewithin
						** will get rid of it */
			r_x_ewithin();
			break;

		case(P_RBFAGGS):
		case(P_RBFFIELD):
		case(P_RBFHEAD):
		case(P_RBFPBOT):
		case(P_RBFPTOP):
		case(P_RBFSETUP):
		case(P_RBFTITLE):
		case(P_RBFTRIM):
			/* used by RBF, ignore here */
			break;

		default:
			break;
	}

	return;
}
