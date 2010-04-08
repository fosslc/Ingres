/*
** Copyright (c) 2004 Ingres Corporation
*/
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
**   R_CLEAR_LINE - fill the output line with blanks
**
**	Parameters:
**		ln	LN structure of line to clear.	
**
**	Returns:
**		none.
**
**	Side Effects:
**		none, directly.
**
**	Called by:
**		r_ln_clr, r_gt_ln.
**
**	Trace Flags:
**		5.0, 5.1.
**
**	History:
**		4/8/81 (ps) - written.
**		23-feb-1993 (rdrane)
**			Explicitly initialize new LN field ln_ctrl to
**			(CTRL *)NULL (46393 and 46713).
**			Explicitly declare routine as returning VOID.
**		16-april-1997(angusm)
**		We may have many CTRL structures in our list.
**		For reports with many q0 items, this can lead
**		to range of tag ids being exhausted (bug 68456).
**		Call IIUGtagFree() to free tag ids for reuse.
**		14-nov-2008 (gupsh01)
**		    Added support for -noutf8align flag for report
**		    writer to align output in a UTF8 installation.
**		26-feb-2009 (gupsh01)
**		    Initialize new LN field ln_utf8_adj.
*/

/*	static	char	Sccsid[] = "@(#)rclrline.c	30.1	11/14/84";	*/


void
r_clear_line(ln)
LN	*ln;
{

	/* internal declarations */

	register i2	i;		/* counter of chars */
	register LN	*lln;

	/* start of routine */

	lln = ln;
	if (En_need_utf8_adj)
	  i = En_utf8_lmax;
	else
	  i = En_lmax;

	if (i > 0)
	{
		MEfill(i, ' ', lln->ln_buf);
		MEfill(i, ' ', lln->ln_ulbuf);
	}

	lln->ln_started = LN_NONE;
	lln->ln_has_ul = FALSE;
	lln->ln_utf8_adjcnt = 0;
	if  (lln->ln_ctrl != (CTRL *)NULL)
	{
		IIUGtagFree(lln->ln_ctrl->cr_tag);
		lln->ln_ctrl = (CTRL *)NULL;
	}
	lln->ln_curpos = 0;

	return;

}
