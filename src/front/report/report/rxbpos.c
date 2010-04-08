/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxbpos.c	30.1	11/14/84"; */

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
**   R_X_BPOS - set the block position either to the top of the block,
**	or after the last line, or set the postion within the line.  
**	The TOP sets to the first line, and
**	the BOTTOM sets to the line after the last line in the buffer.
**	The LINEEND command sets the position to the minimum of the
**	current right margin, or one position beyond the last non
**	blank character on the line.
**
**	Parameters:
**		P_TOP	if TOP command.
**		P_BOTTOM  if BOTTOM command.
**		P_LINEEND	if LINEEND command.
**
**	Returns:
**		none.
**
**	Called By:
**		r_x_tcmd.
**
**	Side Effects:
**		Changes the St_cline pointer and St_pos_number.
**		May change the ln->curpos ptr within the St_cline.
**
**	Trace Flags:
**		150, 154.
**
**	History:
**		5/5/82 (ps)	written.
**		11/19/83 (gac)	added position_number.
**		26-feb-2009 (gupsh01) 
**		    Reset En_utf8_adj_cnt.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_x_bpos(type)
i2	type;
{
	/* internal declarations */

	register LN	*nln;				/* next line marker */

	/* start of routine */

	switch (type)
	{
		case(P_TOP):
			St_cline = St_c_lntop;	/* first line */
			St_l_number = St_top_l_number;
			if (En_need_utf8_adj)
	  		    En_utf8_adj_cnt = St_cline->ln_utf8_adjcnt;
			break;

		case(P_BOTTOM):
			if (St_cline->ln_started == LN_NONE)
			{	/* if we are already at the bottom, go to the top and work down */
				St_cline = St_c_lntop;
				St_l_number = St_top_l_number;
				if (En_need_utf8_adj)
	  		    	  En_utf8_adj_cnt = St_cline->ln_utf8_adjcnt;
			}

			for (nln = r_gt_ln(St_cline); nln->ln_started != LN_NONE; nln = r_gt_ln(nln))
			{
				St_l_number++;
				if (St_cline->ln_has_ul && (St_ulchar != '_'))
				{
					St_l_number++;
				}

				St_cline = nln;
				if (En_need_utf8_adj)
	  		    	    En_utf8_adj_cnt = St_cline->ln_utf8_adjcnt;
			}
			break;

		case(P_LINEEND):
			St_cline->ln_curpos = St_right_margin;
			while (((St_cline->ln_curpos-1) >= St_left_margin)
			    && (*(St_cline->ln_buf+(St_cline->ln_curpos-1)) == ' '))
			{
				(St_cline->ln_curpos)--;
			}
			break;

		case(P_LINESTART):
			St_cline->ln_curpos = St_left_margin;
			break;
	}

	St_pos_number = St_cline->ln_curpos;


	return;
}
