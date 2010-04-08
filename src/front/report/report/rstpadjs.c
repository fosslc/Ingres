/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rstpadjst.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<cm.h>

/*
**   R_STP_ADJUST - stop any adjusting (centering, right or left justification)
**	that might be going on (indicated by the St_adjusting flag), and
**	add what's in the adjusting buffer into the printing buffer,
**	after doing the indicated adjusting.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_tcmd, r_x_newline, r_x_adjust.
**
**	Side Effects:
**		Turns off the St_adjusting flag, and pops the environment.
**
**	Trace Flags:
**		5.0, 5.11.
**
**	Error Messages:
**		305.
**
**	History:
**		7/20/81 (ps)	written.
**		23-mar-1987 (yamamoto)
**			Modified for double byte characters.
**		06/09/88 (dkh) - Fixed problem with uninitialized var ulc.
**		3/21/90 (elein) performance
**			Removed check for St_adjusting--done by calling routine;
**		8/14/91 (elein) b35372
**			If zero characters, it is still a legitimate line.  Set
**			ln_started before outputing characters.  Because this
**			flag was not getting set, multi-line formatted buffers
**			were not printing.
**		8/18/92 (lucius) b46270
**			Corrected the problem of underlining centralised 
**			double-bye report header.
**		21-sep-1992 (rdrane)
**			Backout 18-aug-1992 change for b46270.  The instances
**			of CMbytedec() and CMbyteinc() which were added had
**			incorrect parameter order and/or types.
**		04-jun-93 (dkhor)
**			Re-work fix to b46270 (backout by rdrane above).
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
r_stp_adjust()
{
	/* internal declarations */

	register LN	*aln;			/* fast ptr to adjust line buf */
	register LN	*rln;			/* fast ptr to corresponding printing buf */
	register char	*c,*rc;			/* char ptrs to aln and rln bufs*/
	register char	*ulc, *rulc;		/* char ptrs to aln and rln ulbufs */
	i4		start;			/* start position in buffer */
	i4		nchars;			/* length of adjust line */
	i4		position;		/* position assoc with adjust */

	/* start of routine */


	r_pop_env();			/* retrieve previous environment */

	aln = Ptr_cln_top;
	rln = NULL;


	position = r_eval_pos(St_a_tcmd,St_cline->ln_curpos,FALSE);

	for (; (aln!=NULL) && (aln->ln_started!=LN_NONE); aln=aln->ln_below)
	{
		if (rln == NULL)
		{	/* prime to the first of the printing bufs */
			rln = St_cline;
		}
		else
		{	/* go to the next one */
			rln = r_gt_ln(rln);
		}

		/* get rid of leading and trailing blanks */

		STtrmwhite(aln->ln_buf);
		start = 0;
		c = aln->ln_buf;
		while (CMspace(c) || *c == '\t')
		{
			CMbyteinc(start, c);
			CMnext(c);
		}

		nchars = STlength(c);

		if (aln->ln_has_ul)
		{	/* underlining in centering buffer */
			ulc = aln->ln_ulbuf + start;
		}

		/* now find exact position of string in print buffer */

		switch (St_a_tcmd->tcmd_code)
		{
			case(P_CENTER):
				start = position - (nchars/2);
				break;

			case(P_LEFT):
				start = position;
				break;

			case(P_RIGHT):
				start = position - nchars;
				break;
		}

		/* check for errors */

		while (start < 0)
		{
			CMbyteinc(start, c);
			CMbytedec(nchars, c);
			CMbyteinc(ulc, c);
			CMnext(c);
		}

		while (start >= St_right_margin)
		{	/* wrap around if it starts past end */
			rln->ln_started = max(rln->ln_started,LN_WRAP);
			rln = r_gt_ln(rln);
			start -= St_right_margin;
		}

 
		if (nchars < 0)
		{
			break;		/* nothing in this line */
		}

		/* add to printing line buffer */

		rc = rln->ln_buf + start;
		rln->ln_curpos = start;
		rulc = rln->ln_ulbuf + start;
		rln->ln_started = max(rln->ln_started, LN_WRAP);

		while (nchars > 0 && rln->ln_curpos < En_lmax) 
		{
			CMbytedec(nchars, c);
			CMbyteinc(rln->ln_curpos, c);
			if (!CMspace(c))
			{
				CMcpychar(c, rc);
				if (aln->ln_has_ul)
				{
					if (!CMspace(ulc))
					{
						rln->ln_has_ul = TRUE;
					}
					CMcpyinc(ulc, rulc);
					/* 
					** if *c is the start of a DBL, then
					** we are NOT copying the right number
					** of underline from ulc to rulc.  
					** Note that ulc and rulc contain only
					** the single-byte underline char (_).
					** Hence the additional test below.
					*/
					 if (CMdbl1st(c))
						CMcpyinc(ulc, rulc);
				}
			}
			else
			{
				CMnext(rulc);

				/*
				**  Since ulc only matters when
				**  aln->ln_has_ul is TRUE, we
				**  don't need to increment ulc if
				**  aln->ln_has_ul is FALSE.
				*/
				if (aln->ln_has_ul)
				{
					CMnext(ulc);
				}
			}

			CMnext(c);
			CMnext(rc);
		}
	}

	St_adjusting = FALSE;


	return;
}

