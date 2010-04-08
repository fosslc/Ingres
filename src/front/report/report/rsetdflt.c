/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<er.h>

/*
**   R_SET_DFLT - called immediately before the first output action
**	to the report.	This sets up all of the standard defaults
**	for the report from the information set up to this point,
**	such as margins, etc.
**
**	This also sets up the default positions and formats from a scan of
**	the TCMD lists going outward in the nesting, from the
**	detail, to the breaks, to the report break, and finally the page
**	break.
**
**	If the position or format cannot be determined, a default
**	value is set here.
**
**	Parameters:
**		none.  Note:  if St_no_write is FALSE on entry,
**			this routine simply returns.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_tcmd, r_x_tab, r_x_newline, r_x_print.
**
**	Side Effects:
**		Many.  Sets margins, defaults for columns,
**		and En_lc_bottom.
**		Sets St_no_write to FALSE;
**		Temporarily changes the value of Cact_tcmd.
**
**	Trace Flags:
**		140, 143.
**
**	Error Messages:
**		none.
**
**	History:
**		7/22/81 (ps)	written.
**		1/11/84 (gac)	bug 1994 fixed -- .FF now does not generate ff
**				on terminal.
**		6/6/84	(gac)	bug 582 fixed -- page breaks include<d> in
**				determining defaults.
**		6/12/84 (gac)	bug 2057 fixed -- ff now at beginning of line.
**		1/21/85 (gac)	bug 4935 fixed -- terminals without
**				initialization strings (e.g. vti920)
**				now don't get an access violation.
**		23-feb-90 (sylviap)
**			Don't send out initialization string.  Dont need to
**			set the terminal width because we're doing scrollable
**			output now. (US #607)
**		3/21/90 (elein) performance
**			Removed "if !St_no_write return" check--now done
**			one level up!
**              4/5/90 (elein) performance
**                     Since fetching attribute call needs no range
**                     check use &Ptr_att_top[] directly instead of
**                     calling r_gt_att. We don't expect brk_attribute
**                     to be A_REPORT, A_GROUP, or A_PAGE
**		4/19/90 (elein) us 707
**			Correct logic to put out ff at beginning of report.
**		2-feb-1992 (rdrane)
**			Bypass columns which are unsupported datatypes as well
**			as pre-deleted.
**		19-may-1993 (rdrane)
**			Re-parameterize calls to r_wrt() to include explicit
**			length of line to be written, and ensure that we
**			compute that length here.  This is to ensure that
**			a control sequence containing an embedded ASCII NUL
**			does not prematurely end the line.
**		21-may-1993 (rdrane)
**			Add support for suppression of the initial formfeed.
**              29-sep-2003 (horda03) Bug 111011/INGCBT 494
**                      Ensure that all characters are flushed from the SI
**                      buffer before starting to display the REPORT.
**               1-Apr-2009 (hanal04) Bug 121840
**                      Establish En_lc_top (number of lines in header) so
**                      that we can call r_x_pl() to check St_p_length is
**                      at least as big as the hdr + ftr + 1 data row.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_set_dflt()
{
	/* internal declarations */

	register ATT	*att;		/* fast ptr to attribute */
	register BRK	*brk;		/* ptr to break structures */
	register i2	natt;		/* number of attributes */
	register TCMD	*otcmd;		/* temp storage of Cact_tcmd */

	/* start of routine */


	otcmd = Cact_tcmd;	/* save aside for restore at end */

	/* go through the TCMD lists (start with the detail), then
	** the footers and headers for the breaks and on out, to
	** try to determine default positions and formats for the
	** columns in the report.  The rule is that the innermost
	** reference to a column (or aggregate for that column)
	** determines it's default position and format, unless
	** overridden by the .POS or .FORMAT commands. */

	St_right_margin = (St_rm_set ? St_r_rm : -1);
	St_left_margin = (St_lm_set ? St_r_lm : En_lmax+1);


	/* first check the detail break (only header) */


	r_scn_tcmd(Ptr_det_brk->brk_header);


	/* now go outward from the end of the break structures,
	** first checking footer, then header text to fill in
	** the rest of the default values */

	for(brk=Ptr_last_brk; brk!=NULL; brk=brk->brk_above)
	{

		r_scn_tcmd(brk->brk_footer);


		r_scn_tcmd(brk->brk_header);


	}

	/* check the page break (footer and header */

	r_scn_tcmd(Ptr_pag_brk->brk_footer);
	r_scn_tcmd(Ptr_pag_brk->brk_header);

	/* now set left and right margins (maybe) */

	if ((!St_rm_set||!St_lm_set) && (St_right_margin<=St_left_margin))
	{
		St_right_margin = St_left_margin +5;
	}

	if ((!St_rm_set) && (St_right_margin>0))
	{
		St_r_rm = St_right_margin;
	}

	if ((!St_lm_set) && (St_left_margin<St_r_rm))
	{
		St_r_lm = St_left_margin;
	}

	St_p_rm = St_right_margin = St_r_rm;
	St_p_lm = St_left_margin = St_r_lm;

	St_no_write = FALSE;

	/* put the first line at the left margin */

	r_x_tab((i2)St_r_lm,B_ABSOLUTE);

	/* if any position or fmt not set, set to default */

	for (natt=1; natt<=En_n_attribs; ++natt)
	{
		att = &(Ptr_att_top[natt-1]);
		if  ((att->pre_deleted) ||
		     (att->att_value.db_data == (PTR)NULL))
		{
			continue;
		}
		if (att->att_position < 0)
		{
			att->att_position = 0;
		}

		if (att->att_format == NULL)
		{	/* no format determined. Set default */

			r_fmt_dflt(natt);
		}

	}

	/*
	** If we're using formfeeds, check to see if we're also
	** suppressing the initial formfeed.  If we're not, then
	** write it out.
	*/
	if (((St_to_term) || (St_ff_on)) && (!St_no1stff_on))
	{
		r_wrt(En_rf_unit,ERx("\f"),1);

                /* Ensure that the formfeed is flushed, as it may
                ** corrupt the display if it is flushed after the
                ** contents of the report has been displayed.
                */
                SIflush(stdout);
	}

	/* count the number of lines in the page header and footer */
        En_lc_top = r_cnt_lines(Ptr_pag_brk->brk_header);
	En_lc_bottom = r_cnt_lines(Ptr_pag_brk->brk_footer);

        /* Make sure St_p_length is not too small for hdr and ftr */
        r_x_pl(St_p_length);

	Cact_tcmd = otcmd;	/* restore from start of routine */

	return;
}
