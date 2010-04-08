/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rlnclr.c	30.1	11/14/84"; */

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
**   R_LN_CLR - clear out the linked list of LN structures.  This sets
**	the ln_started flag to LN_NONE for each line in the linked list,
**	but does not actually blank out the lines (Why should we always clear
**	them when they are used rather infrequently?).  This follows to
**	the end of the currently defined linked list (which may be is
**	defined by the longest text yet found) and sets the flags.
**
**	Parameters:
**		ln	start of linked list of LN structures to clear.
**
**	Returns:
**		none.
**
**	Called by:
**		r_do_page, r_x_newline, r_x_center, r_x_right, r_x_left.
**
**	Side Effects:
**		Changes the St_cline and St_c_lntop.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		5.0, 5.1.
**
**	History:
**		7/20/81 (ps)	written.
**		11/19/83 (gac)	added position_number.
**		23-feb-1993 (rdrane)
**			Explicitly initialize new LN field ln_ctrl to
**			(CTRL *)NULL after freeing anything which may
**			be allocated (46393 and 46713).
**			Explicitly declare routine as returning VOID.
**		16-april-1997(angusm)
**		We may have many CTRL structures in our list.
**		For reports with many q0 items, this can lead
**		to range of tag ids being exhausted (bug 68456).
**		Call IIUGtagFree() to free tag ids for reuse.
**		26-feb-2009 (gupsh01)
**		    Readjust the utf8 adjustment count from
**		    ln_utf8_adjcnt field to support adjustment
**		    for multiline formatting.
*/



VOID
r_ln_clr(ln)
register LN	*ln;
{
	/* start of routine */



	if (ln == (LN *)NULL)
	{
		return;
	}

	r_clear_line(ln);
	St_cline = St_c_lntop = ln;
	St_pos_number = St_cline->ln_curpos;
	if (En_need_utf8_adj)
	  En_utf8_adj_cnt = St_cline->ln_utf8_adjcnt;

	for (; ln != (LN *)NULL; ln = ln->ln_below)
	{
		ln->ln_started = LN_NONE;
		ln->ln_has_ul = FALSE;
		if  (ln->ln_ctrl != (CTRL *)NULL)
		{
			IIUGtagFree(ln->ln_ctrl->cr_tag);
			ln->ln_ctrl = (CTRL *)NULL;
		}
	}

	return;
}


