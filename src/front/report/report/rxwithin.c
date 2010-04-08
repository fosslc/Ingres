/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxwithin.c	30.1	11/14/84"; */

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
**   R_X_WITHIN - set up the pointers used in the .WITHIN blocks.
**
**	Parameters:
**		tcmd	first .WI command.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_tcmd, r_scn_tcmd.
**
**	Side Effects:
**		Sets St_cwithin, St_ccoms, Cact_tcmd.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		150, 154.
**
**	History:
**		1/11/82 (ps)	written.
**		11-Nov-2008 (gupsh01)
**		    Add support for -noutf8align flag.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_x_within(tcmd)
register TCMD	*tcmd;
{
	/* start of routine */



	St_cwithin = tcmd;

	for (St_ccoms = St_cwithin; 
		(St_ccoms!=NULL)&&(St_ccoms->tcmd_code==P_WITHIN);
		(St_ccoms=St_ccoms->tcmd_below))
			;

	r_psh_be(tcmd);

	r_nxt_within((ATTRIB)tcmd->tcmd_val.t_v_long);

	Cact_tcmd = St_ccoms;

	if (En_need_utf8_adj)
	{
	   if (St_cline->ln_curpos == 0 )
	   {
	     En_utf8_adj_cnt = 0;
	     En_utf8_set_in_within = FALSE;
	   }
	   else if (En_utf8_adj_cnt)
	   {
	     St_right_margin += En_utf8_adj_cnt;
	     St_cline->ln_curpos += En_utf8_adj_cnt;
	     En_utf8_set_in_within = TRUE;
	   }
	}
	return;
}
