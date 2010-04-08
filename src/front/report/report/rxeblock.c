/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxeblock.c	30.1	11/14/84"; */

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
**   R_X_EBLOCK - end the .BLOCK command.  This may be nested, so
**	this routine will decrement the counter rather than end
**	it directly.  If the end of the block is found and there
**	are multiple lines in the line buffers (at least one
**	newline was executed in the block of commands), then
**	write it out.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_end.
**
**	Side Effects:
**		Changes value of St_c_cblck.  May write out the
**		line buffers.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		150, 154.
**
**	History:
**		1/11/82 (ps)	written.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_x_eblock()
{
	/* internal declarations */

	register LN	*ln;			/* fast pointer */

	/* start of routine */



	if (--St_c_cblk <= 0)
	{	/* end of blocks.  If at least one line finished
		** in the LN structures, print it out */

		St_c_cblk = 0;
		St_l_number = St_top_l_number;

		for(ln=St_c_lntop; (ln!=NULL) && (ln->ln_started!=LN_NONE);
			ln=ln->ln_below)
		{
			if (ln->ln_started==LN_NEW)
			{	/* at least one line finished.  Print it out. */
				r_x_newline(1);
				break;
			}
		}
	}

	return;
}
